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

#ifndef __CompositorPassDef_H__
#define __CompositorPassDef_H__

#include "OgrePrerequisites.h"

#include "OgreIdString.h"
#include "OgreRenderPassDescriptor.h"
#include "OgreResourceTransition.h"

#include "ogrestd/vector.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class CompositorNodeDef;
    class CompositorPassTargetBarrierDef;

    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Effects
     *  @{
     */
    enum CompositorPassType
    {
        PASS_INVALID = 0,
        PASS_SCENE,
        PASS_QUAD,
        PASS_CLEAR,
        PASS_STENCIL,
        PASS_RESOLVE,
        PASS_DEPTHCOPY,
        PASS_UAV,
        PASS_MIPMAP,
        PASS_IBL_SPECULAR,
        PASS_SHADOWS,
        PASS_TARGET_BARRIER,
        PASS_WARM_UP,
        PASS_COMPUTE,
        PASS_CUSTOM
    };

    extern const char *CompositorPassTypeEnumNames[PASS_CUSTOM + 1u];

    class CompositorTargetDef;

    /** Interface to abstract all types of pass definitions (see CompositorPassType):
            + PASS_SCENE (see CompositorPassSceneDef)
            + PASS_QUAD (see CompositorPassQuadDef)
            + PASS_CLEAR (see CompositorPassClearDef)
            + PASS_STENCIL (see CompositorPassStencilDef)
            + PASS_DEPTHCOPY (see CompositorPassDepthCopy)
            + PASS_UAV (see CompositorPassUavDef)
            + PASS_COMPUTE (see CompositorPassComputeDef)
            + PASS_SHADOWS (see CompositorPassShadowsDef)
            + PASS_MIPMAP (see CompositorPassMipmapDef)

        This class doesn't do much on its own. See the derived types for more information
        A definition is shared by all pass instantiations (i.e. Five CompositorPassScene can
        share the same CompositorPassSceneDef) and are assumed to remain const throughout
        their lifetime.
    @par
        Modifying a definition while there are active instantiations is undefined. Some
        implementations may see the change (eg. changing CompositorPassSceneDef::mFirstRQ)
        immediately while not see others (eg. changing CompositorPassSceneDef::mCameraName)
        Also crashes could happen depending on the changes being made.
    */
    class _OgreExport CompositorPassDef : public OgreAllocatedObj
    {
        CompositorPassType mPassType;

        CompositorTargetDef *mParentTargetDef;

    public:
        struct ViewportRect
        {
            float mVpLeft;
            float mVpTop;
            float mVpWidth;
            float mVpHeight;
            float mVpScissorLeft;
            float mVpScissorTop;
            float mVpScissorWidth;
            float mVpScissorHeight;

            ViewportRect() :
                mVpLeft( 0 ),
                mVpTop( 0 ),
                mVpWidth( 1 ),
                mVpHeight( 1 ),
                mVpScissorLeft( 0 ),
                mVpScissorTop( 0 ),
                mVpScissorWidth( 1 ),
                mVpScissorHeight( 1 )
            {
            }
        };
        /// Viewport's region to draw
        ViewportRect mVpRect[16];
        uint32       mNumViewports;

        /// Shadow map index it belongs to (only filled in passes owned by Shadow Nodes)
        uint32 mShadowMapIdx;

        /// Number of times to perform the pass before stopping. -1 to never stop.
        uint32 mNumInitialPasses;

        /// Custom value in case there's a listener attached (to identify the pass)
        uint32 mIdentifier;

        ColourValue              mClearColour[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        float                    mClearDepth;
        uint32                   mClearStencil;
        LoadAction::LoadAction   mLoadActionColour[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        LoadAction::LoadAction   mLoadActionDepth;
        LoadAction::LoadAction   mLoadActionStencil;
        StoreAction::StoreAction mStoreActionColour[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        StoreAction::StoreAction mStoreActionDepth;
        StoreAction::StoreAction mStoreActionStencil;

        /// Ignore mLoadAction*/mStoreAction*
        ///
        /// Useful when doing multiple passes and you want to continue
        /// using the same pass semantics opened by a previous pass
        ///
        /// Be careful with this setting. It can silently break a lot of stuff
        ///
        /// Only valid for PASS_QUAD and PASS_SCENE
        bool mSkipLoadStoreSemantics;

        /** Will issue a warning (by raising an exception) if Ogre is forced to flush
            the RenderTarget, which is very bad for performance on mobile, and can
            cause serious performance problems in Desktop if using MSAA, and also
            cause correctness problems (i.e., bad rendering) if store action is
            StoreAction::MultisampleResolve.
        @remarks
            Flushes are caused by splitting rendering to the same RenderTarget
            in multiple passes while rendering to a different RenderTarget in the middle.
            It's not always possible to avoid it, but if so, consider doing it.
        @par
            No warning will be issued if the RenderTargets getting flushed have their
            LoadAction set to LoadAction::Clear (or LoadAction::ClearOnTilers on tilers).
        */
        bool mWarnIfRtvWasFlushed;

        /// When false will not really bind the RenderTarget for rendering and
        /// use a null colour buffer instead. Useful for depth prepass, or if
        /// the RTT is actually an UAV.
        /// Some passes may ignore this setting (e.g. Clear passes)
        bool mColourWrite;
        bool mReadOnlyDepth;
        bool mReadOnlyStencil;

        /** TODO: Refactor OgreOverlay to remove this design atrocity.
            A custom overlay pass is a better alternative (or just use their own RQ)
        */
        bool mIncludeOverlays;

        /// Whether to flush the command buffer at the end of the pass.
        /// This can incur in a performance overhead (see OpenGL's glFlush and
        /// D3D11' ID3D11DeviceContext::Flush) for info.
        /// Usually you want to leave this off. However for VR applications that
        /// must meet VSync, profiling may show your workload benefits from
        /// submitting earlier so the GPU can start right away executing
        /// rendering commands.
        ///
        /// The main reason to use this is in CPU-bound scenarios where
        /// the GPU starts too late after sitting idle.
        bool mFlushCommandBuffers;

        uint8 mExecutionMask;
        uint8 mViewportModifierMask;

        /// Only used if mShadowMapIdx is valid (if pass is owned by Shadow Nodes). If true,
        /// we won't force the viewport to fit the region of the UV atlas on the texture,
        /// and respect mVp* settings instead.
        bool mShadowMapFullViewport;

        IdStringVec mExposedTextures;

        struct UavDependency
        {
            /// The slot must be in range [0; 64) and ignores the starting
            /// slot (@see CompositorPassUavDef::mStartingSlot)
            uint32 uavSlot;

            /// The UAV pass already sets the texture access.
            /// However two passes in a row may only read from it,
            /// thus having this information is convenient (without
            /// needing to add another bind UAV pass)
            ResourceAccess::ResourceAccess access;
            bool                           allowWriteAfterWrite;

            UavDependency( uint32 _uavSlot, ResourceAccess::ResourceAccess _access,
                           bool _allowWriteAfterWrite ) :
                uavSlot( _uavSlot ),
                access( _access ),
                allowWriteAfterWrite( _allowWriteAfterWrite )
            {
            }
        };
        typedef vector<UavDependency>::type UavDependencyVec;
        UavDependencyVec                    mUavDependencies;

        String mProfilingId;

    public:
        CompositorPassDef( CompositorPassType passType, CompositorTargetDef *parentTargetDef ) :
            mPassType( passType ),
            mParentTargetDef( parentTargetDef ),
            mNumViewports( 1u ),
            mShadowMapIdx( ~0U ),
            mNumInitialPasses( ~0U ),
            mIdentifier( 0U ),
            mClearDepth( 1.0f ),
            mClearStencil( 0 ),
            mLoadActionDepth( LoadAction::Load ),
            mLoadActionStencil( LoadAction::Load ),
            mStoreActionDepth( StoreAction::StoreOrResolve ),
            mStoreActionStencil( StoreAction::StoreOrResolve ),
            mSkipLoadStoreSemantics( false ),
            mWarnIfRtvWasFlushed( false ),
            mColourWrite( true ),
            mReadOnlyDepth( false ),
            mReadOnlyStencil( false ),
            mIncludeOverlays( false ),
            mFlushCommandBuffers( false ),
            mExecutionMask( 0xFF ),
            mViewportModifierMask( 0xFF ),
            mShadowMapFullViewport( false )
        {
            for( int i = 0; i < OGRE_MAX_MULTIPLE_RENDER_TARGETS; ++i )
            {
                mClearColour[i] = ColourValue::Black;
                mLoadActionColour[i] = LoadAction::Load;
                mStoreActionColour[i] = StoreAction::StoreOrResolve;
            }
        }
        virtual ~CompositorPassDef();

        void setAllClearColours( const ColourValue &clearValue );
        void setAllLoadActions( LoadAction::LoadAction loadAction );
        void setAllStoreActions( StoreAction::StoreAction storeAction );

        CompositorPassType         getType() const { return mPassType; }
        uint32                     getRtIndex() const;
        const CompositorTargetDef *getParentTargetDef() const;
    };

    typedef vector<CompositorPassDef *>::type CompositorPassDefVec;

    class _OgreExport CompositorTargetDef : public OgreAllocatedObj
    {
        /// Name is local to Node! (unless using 'global_' prefix)
        IdString mRenderTargetName;
        String   mRenderTargetNameStr;

        CompositorPassDefVec mCompositorPasses;

        /// Used for cubemaps and 3D textures.
        uint32 mRtIndex;

        /// Used by shadow map passes only. Determines which light types are supposed
        /// to be run with the current shadow casting light. i.e. usually point lights
        /// need to be treated differently, and only directional lights are compatible
        /// with PSSM. This bitmask contains:
        ///     mShadowMapSupportedLightTypes & 1u << Light::LT_DIRECTIONAL
        ///     mShadowMapSupportedLightTypes & 1u << Light::LT_POINT
        ///     mShadowMapSupportedLightTypes & 1u << Light::LT_SPOTLIGHT
        uint8 mShadowMapSupportedLightTypes;

        CompositorPassTargetBarrierDef *mTargetLevelBarrier;

        CompositorNodeDef *mParentNodeDef;

    public:
        CompositorTargetDef( const String &renderTargetName, uint32 rtIndex,
                             CompositorNodeDef *parentNodeDef );
        ~CompositorTargetDef();

        IdString getRenderTargetName() const { return mRenderTargetName; }
        String   getRenderTargetNameStr() const { return mRenderTargetNameStr; }

        uint32 getRtIndex() const { return mRtIndex; }

        void  setShadowMapSupportedLightTypes( uint8 types ) { mShadowMapSupportedLightTypes = types; }
        uint8 getShadowMapSupportedLightTypes() const { return mShadowMapSupportedLightTypes; }

        /** When enabled, we will gather all passes contained in this target def
            and issue one barrier for all of them; instead of having (potentially,
            worst case) one barrier for every pass.

            This is much more efficient, but it can't always be done because
            we assume subsequent passes will only change the same resource once
            (or multiple times to the same destination layout).

            This setting is of particular importance on mobile.
        @param bBarrier
        */
        void setTargetLevelBarrier( bool bBarrier );
        bool getTargetLevelBarrier() const { return mTargetLevelBarrier != 0; }

        /** Reserves enough memory for all passes (efficient allocation)
        @remarks
            Calling this function is not obligatory, but recommended
        @param numPasses
            The number of passes expected to contain.
        */
        void setNumPasses( size_t numPasses ) { mCompositorPasses.reserve( numPasses ); }

        CompositorPassDef *addPass( CompositorPassType passType, IdString customId = IdString() );

        const CompositorPassTargetBarrierDef *getTargetLevelBarrierDef() const
        {
            return mTargetLevelBarrier;
        }

        const CompositorPassDefVec &getCompositorPasses() const { return mCompositorPasses; }

        /// @copydoc CompositorManager2::getNodeDefinitionNonConst
        CompositorPassDefVec &getCompositorPassesNonConst() { return mCompositorPasses; }

        const CompositorNodeDef *getParentNodeDef() const { return mParentNodeDef; }
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
