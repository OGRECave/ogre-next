/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#include "Compositor/Pass/OgreCompositorPassDef.h"
#include "OgrePixelFormatGpu.h"

#include "ogrestd/map.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class RenderTarget;
    typedef TextureGpu *CompositorChannel;
    class CompositorNode;
    struct RenderTargetViewDef;
    struct RenderTargetViewEntry;
    typedef vector<TextureGpu *>::type TextureGpuVec;

    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Effects
     *  @{
     */
    struct CompositorTexture
    {
        IdString    name;
        TextureGpu *texture;

        CompositorTexture( IdString _name, TextureGpu *_texture ) : name( _name ), texture( _texture ) {}

        bool operator==( IdString right ) const { return name == right; }
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
    class _OgreExport CompositorPass : public OgreAllocatedObj
    {
        CompositorPassDef const *mDefinition;

    protected:
        static const Quaternion CubemapRotations[6];

        RenderPassDescriptor *mRenderPassDesc;
        /// See getAnyTargetTexture().
        TextureGpu *mAnyTargetTexture;
        uint8       mAnyMipLevel;

        uint32 mNumPassesLeft;

        CompositorNode *mParentNode;

        CompositorTextureVec mTextureDependencies;

        BarrierSolver          &mBarrierSolver;
        ResourceTransitionArray mResourceTransitions;

        /// MUST be called by derived class.
        void initialize( const RenderTargetViewDef *rtv, bool supportsNoRtv = false );

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
        void setupRenderPassTarget( RenderPassTargetBase        *renderPassTargetAttachment,
                                    const RenderTargetViewEntry &rtvEntry, bool isColourAttachment,
                                    TextureGpu *linkedColourAttachment = 0, uint16 depthBufferId = 0,
                                    bool           preferDepthTexture = false,
                                    PixelFormatGpu depthBufferFormat = PFG_UNKNOWN );

        virtual bool allowResolveStoreActionsWithoutResolveTexture() const { return false; }
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
        void setRenderPassDescToCurrent();

        void populateTextureDependenciesFromExposedTextures();

        void executeResourceTransitions();

        void notifyPassEarlyPreExecuteListeners();
        void notifyPassPreExecuteListeners();
        void notifyPassPosExecuteListeners();

        /// @see BarrierSolver::resolveTransition
        void resolveTransition( TextureGpu *texture, ResourceLayout::Layout newLayout,
                                ResourceAccess::ResourceAccess access, uint8 stageMask );
        /// @see BarrierSolver::resolveTransition
        void resolveTransition( GpuTrackedResource *bufferRes, ResourceAccess::ResourceAccess access,
                                uint8 stageMask );

    public:
        CompositorPass( const CompositorPassDef *definition, CompositorNode *parentNode );
        virtual ~CompositorPass();

        /** Bakes all of the memory barriers / resource transition that will be needed
            before executing a GPU command like rendering, copying/blit or compute.
        @param bClearBarriers
            True to do mResourceTransitions.clear();
        */
        virtual void analyzeBarriers( const bool bClearBarriers = true );

        void profilingBegin();
        void profilingEnd();

        virtual void execute( const Camera *lodCameraconst ) = 0;

        /// @see CompositorNode::notifyRecreated
        virtual bool notifyRecreated( const TextureGpu *channel );
        virtual void notifyRecreated( const UavBufferPacked *oldBuffer, UavBufferPacked *newBuffer );

        /// @see CompositorNode::notifyDestroyed
        virtual void notifyDestroyed( TextureGpu *channel );
        virtual void notifyDestroyed( const UavBufferPacked *buffer );

        /// @see CompositorNode::_notifyCleared
        virtual void notifyCleared();

        virtual void resetNumPassesLeft();

        Real getViewportAspectRatio( size_t vpIdx );

        Vector2 getActualDimensions() const;

        CompositorPassType getType() const { return mDefinition->getType(); }

        RenderPassDescriptor *getRenderPassDesc() const { return mRenderPassDesc; }

        const CompositorPassDef *getDefinition() const { return mDefinition; }

        const CompositorNode *getParentNode() const { return mParentNode; }

        /// Contains the first valid texture in mRenderPassDesc, to be used for reference
        /// (e.g. width, height, etc). Could be colour, depth, stencil, or nullptr.
        const TextureGpu *getAnyTargetTexture() const { return mAnyTargetTexture; }

        const ResourceTransitionArray &getResourceTransitions() const { return mResourceTransitions; }
        ResourceTransitionArray       &_getResourceTransitionsNonConst() { return mResourceTransitions; }

        const CompositorTextureVec &getTextureDependencies() const { return mTextureDependencies; }
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
