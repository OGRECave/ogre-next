/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#ifndef __CompositorPass_H__
#define __CompositorPass_H__

#include "OgreHeaderPrefix.h"

#include "Compositor/Pass/OgreCompositorPassDef.h"
#include "OgrePixelFormatGpu.h"

namespace Ogre
{
    class RenderTarget;
    typedef TextureGpu* CompositorChannel;
    class CompositorNode;
    struct RenderTargetViewDef;
    struct RenderTargetViewEntry;
    typedef vector<TextureGpu*>::type TextureGpuVec;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Effects
    *  @{
    */
    struct CompositorTexture
    {
        IdString    name;
        TextureGpu  *texture;

        CompositorTexture( IdString _name, TextureGpu *_texture ) :
                name( _name ), texture( _texture ) {}

        bool operator == ( IdString right ) const
        {
            return name == right;
        }
    };

    typedef vector<CompositorTexture>::type CompositorTextureVec;

    struct BoundUav
    {
        GpuTrackedResource              *rttOrBuffer;
        ResourceAccess::ResourceAccess  boundAccess;
    };

    /** Abstract class for compositor passes. A pass can be a fullscreen quad, a scene
        rendering, a clear. etc.
        Derived classes are responsible for performing an actual job.
        Note that passes do not own RenderTargets, therefore we're not responsible
        for destroying it.
    @author
        Matias N. Goldberg
    @version
        1.0
    */
    class _OgreExport CompositorPass : public CompositorInstAlloc
    {
        CompositorPassDef const *mDefinition;
    protected:
        static const Quaternion CubemapRotations[6];

        RenderPassDescriptor    *mRenderPassDesc;
        /// Contains the first valid texture in mRenderPassDesc, to be used for reference
        /// (e.g. width, height, etc). Could be colour, depth, stencil, or nullptr.
        TextureGpu              *mAnyTargetTexture;
        uint8                   mAnyMipLevel;

        uint32          mNumPassesLeft;

        CompositorNode  *mParentNode;

        CompositorTextureVec    mTextureDependencies;

        typedef vector<ResourceTransition>::type ResourceTransitionVec;
        ResourceTransitionVec   mResourceTransitions;
        /// In OpenGL, only the first entry in mResourceTransitions contains a real
        /// memory barrier. The rest is just kept for debugging purposes. So
        /// mNumValidResourceTransitions is either 0 or 1.
        /// In D3D12/Vulkan/Mantle however,
        /// mNumValidResourceTransitions = mResourceTransitions.size()
        uint32                  mNumValidResourceTransitions;

        /// MUST be called by derived class.
        void initialize( const RenderTargetViewDef *rtv, bool supportsNoRtv=false );

        /// Modifies mRenderPassDesc
        void setupRenderPassDesc( const RenderTargetViewDef *rtv );
        /**
        @param renderPassTargetAttachment
        @param rtvEntry
        @param linkedColourAttachment
            When setting depth & stencil, we'll use this argument for finding
            a depth buffer in the same depth pool (unless the depth buffer is
            explicit)
        */
        void setupRenderPassTarget( RenderPassTargetBase *renderPassTargetAttachment,
                                    const RenderTargetViewEntry &rtvEntry,
                                    bool isColourAttachment,
                                    TextureGpu *linkedColourAttachment=0, uint16 depthBufferId = 0,
                                    bool preferDepthTexture = false,
                                    PixelFormatGpu depthBufferFormat = PFG_UNKNOWN );

        virtual bool allowResolveStoreActionsWithoutResolveTexture(void) const { return false; }
        /// Called by setupRenderPassDesc right before calling renderPassDesc->entriesModified
        /// in case derived class wants to make some changes.
        virtual void postRenderPassDescriptorSetup( RenderPassDescriptor *renderPassDesc ) {}

        void setRenderPassDescToCurrent(void);

        void populateTextureDependenciesFromExposedTextures(void);

        void executeResourceTransitions(void);

    public:
        CompositorPass( const CompositorPassDef *definition, CompositorNode *parentNode );
        virtual ~CompositorPass();

        void profilingBegin(void);
        void profilingEnd(void);

        virtual void execute( const Camera *lodCameraconst ) = 0;

        void addResourceTransition( ResourceLayoutMap::iterator currentLayout,
                                    ResourceLayout::Layout newLayout,
                                    uint32 readBarrierBits );

        /** Emulates the execution of a UAV to understand memory dependencies,
            and adds a memory barrier / resource transition if we need to.
        @remarks
            Note that an UAV->UAV resource transition is just a memory barrier.
        @param boundUavs [in/out]
            An array of the currently bound UAVs by slot.
            The derived class CompositorPassUav will write to them as part of the
            emulation. The base implementation reads from this value.
        @param uavsAccess [in/out]
            A map with the last access flag used for each RenderTarget. We need it
            to identify RaR situations, which are the only ones that don't need
            a barrier (and also WaW hazards, when explicitly allowed by the pass).
            Note: We will set the access to ResourceAccess::Undefined to signal other
            passes  that the UAV hazard already has a barrier (just in case there was
            one already created).
        @param resourcesLayout [in/out]
            A map with the current layout of every RenderTarget used so far.
            Needed to identify if we need to change the resource layout
            to an UAV.
        */
        virtual void _placeBarriersAndEmulateUavExecution( BoundUav boundUavs[64],
                                                           ResourceAccessMap &uavsAccess,
                                                           ResourceLayoutMap &resourcesLayout );
        void _removeAllBarriers(void);

        /// @See CompositorNode::notifyRecreated
        virtual bool notifyRecreated( const TextureGpu *channel );
        virtual void notifyRecreated( const UavBufferPacked *oldBuffer, UavBufferPacked *newBuffer );

        /// @See CompositorNode::notifyDestroyed
        virtual void notifyDestroyed( TextureGpu *channel );
        virtual void notifyDestroyed( const UavBufferPacked *buffer );

        /// @See CompositorNode::_notifyCleared
        virtual void notifyCleared(void);

        void resetNumPassesLeft(void);

        Vector2 getActualDimensions(void) const;

        CompositorPassType getType() const  { return mDefinition->getType(); }

        RenderPassDescriptor* getRenderPassDesc(void) const { return mRenderPassDesc; }

        const CompositorPassDef* getDefinition(void) const  { return mDefinition; }

		const CompositorNode* getParentNode(void) const		{ return mParentNode; }

        const CompositorTextureVec& getTextureDependencies(void) const  { return mTextureDependencies; }
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
