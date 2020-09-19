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

#include "ogrestd/map.h"

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

        BarrierSolver &mBarrierSolver;
        ResourceTransitionArray mResourceTransitions;

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

        /// Does the same as setRenderPassDescToCurrent, but only setting a single Viewport.
        /// (setRenderPassDescToCurrent does much more)
        ///
        /// Needed due to catch-22 problem:
        ///     1. Some algorithms (like LOD) need a Viewport set
        ///     2. We can't call setRenderPassDescToCurrent until that algorithm has run
        ///     3. The algorithm thus can't run if setRenderPassDescToCurrent isn't set
        ///
        /// See https://forums.ogre3d.org/viewtopic.php?p=548046#p548046
        void setViewportSizeToViewport( size_t vpIdx, Viewport *outVp );
        void setRenderPassDescToCurrent(void);

        void populateTextureDependenciesFromExposedTextures(void);

        void executeResourceTransitions(void);

        void notifyPassEarlyPreExecuteListeners(void);
        void notifyPassPreExecuteListeners(void);
        void notifyPassPosExecuteListeners(void);

        /// @see BarrierSolver::resolveTransition
        void resolveTransition( TextureGpu *texture, ResourceLayout::Layout newLayout,
                                ResourceAccess::ResourceAccess access, uint8 stageMask );
        /// @see BarrierSolver::resolveTransition
        void resolveTransition( GpuTrackedResource *bufferRes, ResourceAccess::ResourceAccess access,
                                uint8 stageMask );

        /** Bakes most of the memory barriers / resource transition that will be needed
            during execution.

            Some passes may still generate more barriers/transitions that need to be placed
            dynamically. These passes must update resourceStatus without inserting a barrier
            into mResourceTransitions
        @param boundUavs [in/out]
            An array of the currently bound UAVs by slot.
            The derived class CompositorPassUav will write to them as part of the
            emulation. The base implementation reads from this value.
        @param resourceStatus [in/out]
            A map with the last access flags used for each GpuTrackedResource.
            We need it to identify how it was last used and thus what barrier
            we need to insert
        */
        void analyzeBarriers( void );

    public:
        CompositorPass( const CompositorPassDef *definition, CompositorNode *parentNode );
        virtual ~CompositorPass();

        void profilingBegin(void);
        void profilingEnd(void);

        virtual void execute( const Camera *lodCameraconst ) = 0;

        /// @See CompositorNode::notifyRecreated
        virtual bool notifyRecreated( const TextureGpu *channel );
        virtual void notifyRecreated( const UavBufferPacked *oldBuffer, UavBufferPacked *newBuffer );

        /// @See CompositorNode::notifyDestroyed
        virtual void notifyDestroyed( TextureGpu *channel );
        virtual void notifyDestroyed( const UavBufferPacked *buffer );

        /// @See CompositorNode::_notifyCleared
        virtual void notifyCleared(void);

        virtual void resetNumPassesLeft(void);

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
