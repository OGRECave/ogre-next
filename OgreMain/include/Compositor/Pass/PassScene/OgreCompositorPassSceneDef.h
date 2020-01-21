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

#ifndef __CompositorPassSceneDef_H__
#define __CompositorPassSceneDef_H__

#include "OgreHeaderPrefix.h"

#include "../OgreCompositorPassDef.h"

#include "OgreVisibilityFlags.h"
#include "OgreMaterialManager.h"
#include "OgreRenderSystem.h"
#include "OgreRoot.h"
namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Effects
    *  @{
    */

    enum ShadowNodeRecalculation
    {
        /// Forces recalculation of the shadow node for this pass. Not recommended.
        /// Only useful if you're doing very low level manipulation via listeners
        /// and the compositor can't detect changes.
        SHADOW_NODE_RECALCULATE,

        /// Forces the PassScene to not recalculate the ShadowNode even if it should.
        /// Useful if you know what you're doing and want to use the ShadowNode of
        /// a different pass with a different camera.
        SHADOW_NODE_REUSE,

        /// Default option. The first PassScene to use a Camera will update its shadow node.
        /// All subsequent passes (even from different nodes) using the same camera will
        /// not update the shadow node again until a PassScene uses a different camera with
        /// the same shadow node. The Compositor will log a performance warning on debug
        /// builds if it detects the same Shadow node will be updated twice for the same
        /// camera.
        SHADOW_NODE_FIRST_ONLY,

        SHADOW_NODE_CASTER_PASS     //Set automatically only when this pass is used by a ShadowNode
    };

    class _OgreExport CompositorPassSceneDef : public CompositorPassDef
    {
    public:
        /// Viewport's visibility mask while rendering our pass
        uint32                  mVisibilityMask;
        /// Used by Forward+ to determine which lights should be used (via Light::setVisibilityFlags)
        uint32                  mLightVisibilityMask;
        IdString                mShadowNode;
        ShadowNodeRecalculation mShadowNodeRecalculation; //Only valid if mShadowNode is not empty
        /// When empty, uses the default camera.
        IdString                mCameraName;
        /** When empty, it implies mCameraName == mLodCameraName; except for shadow nodes.
            For shadow nodes, when empty, it will use the receiver's lod camera.
        */
        IdString                mLodCameraName;
        /// When empty, it implies mCameraName == mCullCameraName.
        IdString                mCullCameraName;

        /// Only used if mPrePassMode == PrePassUse
        IdStringVec     mPrePassTexture;
        IdString        mPrePassDepthTexture;
        IdString        mPrePassSsrTexture;

        IdString        mDepthTextureNoMsaa;
        IdString        mRefractionsTexture;

        /// This is a depth pre-pass. Note: Implementations may write
        /// to colour too for hybrid deferred & forward rendering.
        /// If you modify this, you probably want to modify
        /// mReadOnlyDepth & mReadOnlyStencil too
        PrePassMode     mPrePassMode;
        /// Generate Normals for a GBuffer in RTV output 1,
        /// This flag is ignored mPrePassMode if mPrePassMode != PrePassNone
        bool            mGenNormalsGBuf;

        /// First Render Queue ID to render. Inclusive
        uint8           mFirstRQ;
        /// Last Render Queue ID to render. Not inclusive
        uint8           mLastRQ;

        /// Enable ForwardPlus during the pass (if Forward3D or ForwardClustered systems
        /// were created). Disabling optimizes performance when you don't need it.
        bool            mEnableForwardPlus;

        /** When true, the camera will be rotated 90°, -90° or 180° depending on the value of
            mRtIndex and then restored to its original rotation after we're done.
        */
        bool            mCameraCubemapReorient;

        /** When true, which Lod index is current will be updated. Reasons to set this to false:
             1. You don't use LOD (i.e. you're GPU bottleneck). Setting to false helps CPU.
             2. LODs have been calculated in a previous pass. This happens if previous pass(es)
                all used the same lod camera and all RenderQueue IDs this pass will use have
                been rendered already and updated their lod lists.
        @remarks
            Automatically set to false for shadow nodes that leave mLodCameraName empty
        */
        bool            mUpdateLodLists;

        /** Multiplier to the Lod value. What it means depends on the technique.
            You'll probably want to avoid setting it directly and rather use
            LodStrategy::transformBias

            @see    LodStrategy::transformBias
        */
        Real            mLodBias;

        /// When true, will render in instanced stereo mode, thus outputting left & right eyes
        /// at the same time
        bool            mInstancedStereo;

        /// When true, the frustum culling is skipped in this pass. To cull objects, data from
        /// the most recent frustum culling execution are used.
        bool            mReuseCullData;

        /// Same as CompositorPassDef::mFlushCommandBuffers, but executed after the shadow node
        /// Note you may end up flushing twice if the shadow node also has flushing of its own
        ///
        /// Does not do anything if mShadowNodeRecalculation is set to SHADOW_NODE_REUSE
        /// (whether explicitly or automatically determined)
        bool            mFlushCommandBuffersAfterShadowNode;

        /// Used for baking lightmaps and similar stuff.
        /// When set to 0xFF it is disabled.
        /// Otherwise, the selected UV set will be used to bake the texture with the render results.
        /// If the mesh doesn't have such UV, then the max UV will be used.
        /// If the mesh doesn't have UVs, it is up to the Hlms implementation
        /// what to do (probably raise an exception)
        uint8           mUvBakingSet;

        /// When mUvBakingSet is set, tells whether we should bake the whole render
        /// result or just the lighting intensity, in order to create a lightmap.
        ///
        /// Typically, when baking with this setting, it's because you're going
        /// to use this texture with HlmsPbsDatablock as an emissive map and calling
        /// HlmsPbsDatablock::setUseEmissiveAsLightmap with true.
        bool            mBakeLightingOnly;

        /// When mUvBakingSet is enabled, this defines the UV offset (in pixels).
        /// Without conservative rasterization, the baking won't render on pixels that
        /// are being partially touched by the triangle.
        ///
        /// This causes severe artifacts when the baked result is used
        /// (background colour of the texture leaks into the triangle).
        ///
        /// Without conservative rasterization, a sound solution is to render multiple times
        /// with pixel (or subpixel) offsets in order to add some padding around the borders
        /// of each triangle in the lightmap.
        ///
        /// MSAA can also be used instead, but certain types of baking (e.g. GBuffer baking)
        /// don't work well because the MSAA sample locations are not in the center.
        ///
        /// See https://ndotl.wordpress.com/2018/08/29/baking-artifact-free-lightmaps/
        Vector2         mUvBakingOffset;

        /// The material scheme used for this pass. If no material scheme is set then
        /// it will use the default scheme
        String          mMaterialScheme;

        CompositorPassSceneDef( CompositorTargetDef *parentTargetDef ) :
            CompositorPassDef( PASS_SCENE, parentTargetDef ),
            mVisibilityMask( VisibilityFlags::RESERVED_VISIBILITY_FLAGS ),
            mLightVisibilityMask( VisibilityFlags::RESERVED_VISIBILITY_FLAGS ),
            mShadowNodeRecalculation( SHADOW_NODE_FIRST_ONLY ),
            mPrePassMode( PrePassNone ),
            mGenNormalsGBuf( false ),
            mFirstRQ( 0 ),
            mLastRQ( (uint8)-1 ),
            mEnableForwardPlus( true ),
            mCameraCubemapReorient( false ),
            mUpdateLodLists( true ),
            mLodBias( 1.0f ),
            mInstancedStereo( false ),
            mReuseCullData( false ),
            mFlushCommandBuffersAfterShadowNode( false ),
            mUvBakingSet( 0xFF ),
            mBakeLightingOnly( false ),
            mUvBakingOffset( Vector2::ZERO ),
            mMaterialScheme(MaterialManager::DEFAULT_SCHEME_NAME)
        {
            //Change base defaults
            mIncludeOverlays = true;
            //retrieve the rendersystem default scheme name, which can be different from DEFAULT_SCHEME_NAME if RS doesn't have fixed function support.
            RenderSystem* rs = Root::getSingleton().getRenderSystem();
            mMaterialScheme = rs->_getDefaultViewportMaterialScheme();
        }

        void setVisibilityMask( uint32 visibilityMask )
        {
            mVisibilityMask = visibilityMask & VisibilityFlags::RESERVED_VISIBILITY_FLAGS;
        }

        void setLightVisibilityMask( uint32 visibilityMask )
        {
            mLightVisibilityMask = visibilityMask & VisibilityFlags::RESERVED_VISIBILITY_FLAGS;
        }

        void setUseDepthPrePass( const IdStringVec &textureName, IdString depthTextureName,
                                 IdString ssrTexture )
        {
            mPrePassMode = PrePassUse;
            mPrePassTexture = textureName;
            mExposedTextures.insert( mExposedTextures.end(), textureName.begin(), textureName.end() );
            mPrePassDepthTexture = depthTextureName;
            mExposedTextures.push_back( depthTextureName );
            mPrePassSsrTexture = ssrTexture;
            mExposedTextures.push_back( ssrTexture );

            mReadOnlyDepth = true;
            mReadOnlyStencil = true;
        }

        void setUseRefractions( IdString depthTextureName, IdString refractionsTexture )
        {
            mDepthTextureNoMsaa = depthTextureName;
            mExposedTextures.push_back( depthTextureName );
            mRefractionsTexture = refractionsTexture;
            mExposedTextures.push_back( refractionsTexture );

            mReadOnlyDepth = true;
            mReadOnlyStencil = true;
        }
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
