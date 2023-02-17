/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2017 Torus Knot Software Ltd

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

#include "OgreStableHeaders.h"

#include "OgreForwardPlusBase.h"

#include "OgreCamera.h"
#include "OgreDecal.h"
#include "OgreHlms.h"
#include "OgreInternalCubemapProbe.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreViewport.h"
#include "Vao/OgreReadOnlyBufferPacked.h"
#include "Vao/OgreVaoManager.h"

namespace Ogre
{
    // N variables * 4 (vec4 or padded vec3) * 4 (bytes per float)
    const uint32 ForwardPlusBase::MinDecalRq = 0u;
    const uint32 ForwardPlusBase::MaxDecalRq = 4u;
    const uint32 ForwardPlusBase::MinCubemapProbeRq = 5u;
    const uint32 ForwardPlusBase::MaxCubemapProbeRq = 8u;
    const size_t ForwardPlusBase::NumBytesPerLight = c_ForwardPlusNumFloat4PerLight * 4u * 4u;
    const size_t ForwardPlusBase::NumBytesPerDecal = c_ForwardPlusNumFloat4PerDecal * 4u * 4u;
    const size_t ForwardPlusBase::NumBytesPerCubemapProbe =
        c_ForwardPlusNumFloat4PerCubemapProbe * 4u * 4u;

    ForwardPlusBase::ForwardPlusBase( SceneManager *sceneManager, bool decalsEnabled,
                                      bool cubemapProbesEnabled ) :
        mVaoManager( 0 ),
        mSceneManager( sceneManager ),
        mDebugMode( false ),
        mFadeAttenuationRange( true ),
        mEnableVpls( false ),
        mDecalsEnabled( decalsEnabled ),
        mCubemapProbesEnabled( cubemapProbesEnabled ),
#if !OGRE_NO_FINE_LIGHT_MASK_GRANULARITY
        mFineLightMaskGranularity( true ),
#endif
        mDecalFloat4Offset( 0u ),
        mCubemapProbeFloat4Offset( 0u )
    {
    }
    //-----------------------------------------------------------------------------------
    ForwardPlusBase::~ForwardPlusBase() { _releaseManualHardwareResources(); }
    //-----------------------------------------------------------------------------------
    void ForwardPlusBase::_releaseManualHardwareResources()
    {
        CachedGridVec::iterator itor = mCachedGrid.begin();
        CachedGridVec::iterator endt = mCachedGrid.end();

        while( itor != endt )
        {
            CachedGridBufferVec::iterator itBuf = itor->gridBuffers.begin();
            CachedGridBufferVec::iterator enBuf = itor->gridBuffers.end();

            while( itBuf != enBuf )
            {
                if( itBuf->gridBuffer )
                {
                    if( itBuf->gridBuffer->getMappingState() != MS_UNMAPPED )
                        itBuf->gridBuffer->unmap( UO_UNMAP_ALL );
                    mVaoManager->destroyTexBuffer( itBuf->gridBuffer );
                    itBuf->gridBuffer = 0;
                }

                if( itBuf->globalLightListBuffer )
                {
                    if( itBuf->globalLightListBuffer->getMappingState() != MS_UNMAPPED )
                        itBuf->globalLightListBuffer->unmap( UO_UNMAP_ALL );
                    mVaoManager->destroyReadOnlyBuffer( itBuf->globalLightListBuffer );
                    itBuf->globalLightListBuffer = 0;
                }

                ++itBuf;
            }

            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void ForwardPlusBase::_changeRenderSystem( RenderSystem *newRs )
    {
        _releaseManualHardwareResources();

        mVaoManager = 0;

        if( newRs )
        {
            mVaoManager = newRs->getVaoManager();
        }
    }
    //-----------------------------------------------------------------------------------
    size_t ForwardPlusBase::calculateBytesNeeded( size_t numLights, size_t numDecals,
                                                  size_t numCubemapProbes )
    {
        size_t totalBytes = numLights * NumBytesPerLight;
        if( numDecals > 0u )
        {
            totalBytes = alignToNextMultiple( totalBytes, NumBytesPerDecal );
            totalBytes += numDecals * NumBytesPerDecal;
        }
        if( numCubemapProbes > 0u )
        {
            totalBytes = alignToNextMultiple( totalBytes, NumBytesPerCubemapProbe );
            totalBytes += numCubemapProbes * NumBytesPerCubemapProbe;
        }

        return totalBytes;
    }
    //-----------------------------------------------------------------------------------
    void ForwardPlusBase::fillGlobalLightListBuffer( Camera *camera,
                                                     TexBufferPacked *globalLightListBuffer )
    {
        // const LightListInfo &globalLightList = mSceneManager->getGlobalLightList();
        const size_t numLights = mCurrentLightList.size();

        size_t numDecals = 0;
        size_t numCubemapProbes = 0;
        size_t actualMaxDecalRq = 0;
        size_t actualMaxCubemapProbeRq = 0;
        const VisibleObjectsPerRq &objsPerRqInThread0 = mSceneManager->_getTmpVisibleObjectsList()[0];
        if( mDecalsEnabled )
        {
            actualMaxDecalRq = std::min<size_t>( MaxDecalRq, objsPerRqInThread0.size() );
            for( size_t rqId = MinDecalRq; rqId <= actualMaxDecalRq; ++rqId )
                numDecals += objsPerRqInThread0[rqId].size();
        }
        if( mCubemapProbesEnabled )
        {
            actualMaxCubemapProbeRq = std::min<size_t>( MaxCubemapProbeRq, objsPerRqInThread0.size() );
            for( size_t rqId = MinCubemapProbeRq; rqId <= actualMaxCubemapProbeRq; ++rqId )
                numCubemapProbes += objsPerRqInThread0[rqId].size();
        }

        if( !numLights && !numDecals && !numCubemapProbes )
            return;

        {
            size_t accumOffset = numLights * c_ForwardPlusNumFloat4PerLight;
            if( numDecals > 0u )
                accumOffset = alignToNextMultiple( accumOffset, c_ForwardPlusNumFloat4PerDecal );
            mDecalFloat4Offset = static_cast<uint16>( accumOffset );
            accumOffset += numDecals * c_ForwardPlusNumFloat4PerDecal;
            if( numCubemapProbes > 0u )
                accumOffset = alignToNextMultiple( accumOffset, c_ForwardPlusNumFloat4PerCubemapProbe );
            mCubemapProbeFloat4Offset = static_cast<uint16>( accumOffset );
        }

        Matrix4 viewMatrix = camera->getViewMatrix();
        Matrix3 viewMatrix3;
        viewMatrix.extract3x3Matrix( viewMatrix3 );

        const float invHeightLightProfileTex = Root::getSingleton().getLightProfilesInvHeight();

        float *RESTRICT_ALIAS lightData =
            reinterpret_cast<float * RESTRICT_ALIAS>( globalLightListBuffer->map(
                0, calculateBytesNeeded( numLights, numDecals, numCubemapProbes ) ) );
        LightArray::const_iterator itLights = mCurrentLightList.begin();
        LightArray::const_iterator enLights = mCurrentLightList.end();

        while( itLights != enLights )
        {
            const Light *light = *itLights;

            Vector3 lightPos = light->getParentNode()->_getDerivedPosition();
            lightPos = viewMatrix * lightPos;

            // vec3 lights[numLights].position
            *lightData++ = lightPos.x;
            *lightData++ = lightPos.y;
            *lightData++ = lightPos.z;
            *lightData++ = static_cast<float>( light->getType() );

            // vec3 lights[numLights].diffuse
            ColourValue colour = light->getDiffuseColour() * light->getPowerScale();
            *lightData++ = colour.r;
            *lightData++ = colour.g;
            *lightData++ = colour.b;
#if !OGRE_NO_FINE_LIGHT_MASK_GRANULARITY
            *reinterpret_cast<uint32 * RESTRICT_ALIAS>( lightData ) = light->getLightMask();
#endif
            ++lightData;

            // vec3 lights[numLights].specular
            colour = light->getSpecularColour() * light->getPowerScale();
            *lightData++ = colour.r;
            *lightData++ = colour.g;
            *lightData++ = colour.b;
            ++lightData;

            // vec3 lights[numLights].attenuation;
            Real attenRange = light->getAttenuationRange();
            Real attenLinear = light->getAttenuationLinear();
            Real attenQuadratic = light->getAttenuationQuadric();
            *lightData++ = attenRange;
            *lightData++ = attenLinear;
            *lightData++ = attenQuadratic;
            *lightData++ = 1.0f / attenRange;

            const uint16 lightProfileIdx = light->getLightProfileIdx();

            // vec3 lights[numLights].spotDirection;
            Vector3 spotDir = viewMatrix3 * light->getDerivedDirection();
            *lightData++ = spotDir.x;
            *lightData++ = spotDir.y;
            *lightData++ = spotDir.z;
            *lightData++ = ( static_cast<float>( lightProfileIdx ) + 0.5f ) * invHeightLightProfileTex;

            // vec3 lights[numLights].spotParams;
            Radian innerAngle = light->getSpotlightInnerAngle();
            Radian outerAngle = light->getSpotlightOuterAngle();
            *lightData++ = 1.0f / ( cosf( innerAngle.valueRadians() * 0.5f ) -
                                    cosf( outerAngle.valueRadians() * 0.5f ) );
            *lightData++ = cosf( outerAngle.valueRadians() * 0.5f );
            *lightData++ = light->getSpotlightFalloff();
            ++lightData;

            ++itLights;
        }

        // Align to the start of decals
        // Alignment happens in increments of float4, hence the "<< 2u"
        lightData += ( mDecalFloat4Offset - numLights * c_ForwardPlusNumFloat4PerLight ) << 2u;

        const Matrix4 viewMat = camera->getViewMatrix();

        for( size_t rqId = MinDecalRq; rqId <= actualMaxDecalRq; ++rqId )
        {
            MovableObject::MovableObjectArray::const_iterator itor = objsPerRqInThread0[rqId].begin();
            MovableObject::MovableObjectArray::const_iterator endt = objsPerRqInThread0[rqId].end();

            while( itor != endt )
            {
                OGRE_ASSERT_HIGH( dynamic_cast<Decal *>( *itor ) );
                Decal *decal = static_cast<Decal *>( *itor );

                const Matrix4 worldMat = decal->_getParentNodeFullTransform();
                Matrix4 invWorldView = viewMat.concatenateAffine( worldMat );
                invWorldView = invWorldView.inverseAffine();

#if !OGRE_DOUBLE_PRECISION
                memcpy( lightData, invWorldView[0], sizeof( float ) * 12u );
                lightData += 12u;
#else
                for( size_t i = 0; i < 3u; ++i )
                {
                    *lightData++ = static_cast<float>( invWorldView[i][0] );
                    *lightData++ = static_cast<float>( invWorldView[i][1] );
                    *lightData++ = static_cast<float>( invWorldView[i][2] );
                    *lightData++ = static_cast<float>( invWorldView[i][3] );
                }
#endif
                memcpy( lightData, &decal->mDiffuseIdx, sizeof( uint32 ) * 4u );
                lightData += 4u;

                ++itor;
            }
        }

        // Align to the start of cubemap probes
        // Alignment happens in increments of float4, hence the "<< 2u"
        lightData += ( mCubemapProbeFloat4Offset - mDecalFloat4Offset -
                       numDecals * c_ForwardPlusNumFloat4PerDecal )
                     << 2u;

        viewMatrix = camera->getViewMatrix();
        Matrix3 invViewMatrix3 = viewMatrix3.Inverse();

        for( size_t rqId = MinCubemapProbeRq; rqId <= actualMaxCubemapProbeRq; ++rqId )
        {
            MovableObject::MovableObjectArray::const_iterator itor = objsPerRqInThread0[rqId].begin();
            MovableObject::MovableObjectArray::const_iterator endt = objsPerRqInThread0[rqId].end();

            while( itor != endt )
            {
                OGRE_ASSERT_HIGH( dynamic_cast<InternalCubemapProbe *>( *itor ) );
                InternalCubemapProbe *probe = static_cast<InternalCubemapProbe *>( *itor );

                // See ParallaxCorrectedCubemapBase::fillConstBufferData for reference
                Matrix3 probeInvOrientation(
                    probe->mGpuData[0][0], probe->mGpuData[0][1], probe->mGpuData[0][2],
                    probe->mGpuData[1][0], probe->mGpuData[1][1], probe->mGpuData[1][2],
                    probe->mGpuData[2][0], probe->mGpuData[2][1], probe->mGpuData[2][2] );
                const Matrix3 viewSpaceToProbeLocal = probeInvOrientation * invViewMatrix3;
                const Vector3 probeShapeCenter( probe->mGpuData[0][3], probe->mGpuData[1][3],
                                                probe->mGpuData[2][3] );
                Vector3 cubemapPosVS( probe->mGpuData[5][3], probe->mGpuData[5][3],
                                      probe->mGpuData[5][3] );
                cubemapPosVS = viewMatrix * cubemapPosVS;
                Vector3 probeShapeCenterVS = viewMatrix * probeShapeCenter;

                // float4 row0_centerX;
                *lightData++ = viewSpaceToProbeLocal[0][0];
                *lightData++ = viewSpaceToProbeLocal[0][1];
                *lightData++ = viewSpaceToProbeLocal[0][2];
                *lightData++ = probeShapeCenterVS.x;

                // float4 row1_centerY;
                *lightData++ = viewSpaceToProbeLocal[0][3];
                *lightData++ = viewSpaceToProbeLocal[0][4];
                *lightData++ = viewSpaceToProbeLocal[0][5];
                *lightData++ = probeShapeCenterVS.y;

                // float4 row2_centerZ;
                *lightData++ = viewSpaceToProbeLocal[0][6];
                *lightData++ = viewSpaceToProbeLocal[0][7];
                *lightData++ = viewSpaceToProbeLocal[0][8];
                *lightData++ = probeShapeCenterVS.z;

                // float4 halfSize;
                *lightData++ = probe->mGpuData[3][0];
                *lightData++ = probe->mGpuData[3][1];
                *lightData++ = probe->mGpuData[3][2];
                *lightData++ = probe->mGpuData[3][3];

                // float4 cubemapPosLS;
                *lightData++ = probe->mGpuData[4][0];
                *lightData++ = probe->mGpuData[4][1];
                *lightData++ = probe->mGpuData[4][2];
                *lightData++ = probe->mGpuData[4][3];

                // float4 cubemapPosVS;
                *lightData++ = cubemapPosVS.x;
                *lightData++ = cubemapPosVS.y;
                *lightData++ = cubemapPosVS.z;
                *lightData++ = probe->mGpuData[5][3];

                // float4 probeInnerRange;
                *lightData++ = probe->mGpuData[6][0];
                *lightData++ = probe->mGpuData[6][1];
                *lightData++ = probe->mGpuData[6][2];
                *lightData++ = probe->mGpuData[6][3];

                // float4 probeOuterRange;
                *lightData++ = probe->mGpuData[7][0];
                *lightData++ = probe->mGpuData[7][1];
                *lightData++ = probe->mGpuData[7][2];
                *lightData++ = probe->mGpuData[7][3];

                ++itor;
            }
        }

        globalLightListBuffer->unmap( UO_KEEP_PERSISTENT );
    }
    //-----------------------------------------------------------------------------------
    bool ForwardPlusBase::getCachedGridFor( Camera *camera, CachedGrid **outCachedGrid )
    {
        const CompositorShadowNode *shadowNode = mSceneManager->getCurrentShadowNode();

        const uint32 visibilityMask = camera->getLastViewport()->getLightVisibilityMask();

        CachedGridVec::iterator itor = mCachedGrid.begin();
        CachedGridVec::iterator endt = mCachedGrid.end();

        while( itor != endt )
        {
            if( itor->camera == camera && itor->reflection == camera->isReflected() &&
                Math::Abs( itor->aspectRatio - camera->getAspectRatio() ) < 1e-6f &&
                itor->visibilityMask == visibilityMask && itor->shadowNode == shadowNode )
            {
                bool upToDate = itor->lastFrame == mVaoManager->getFrameCount();
                itor->lastFrame = mVaoManager->getFrameCount();

                if( upToDate )
                {
                    if( itor->lastPos != camera->getDerivedPosition() ||
                        itor->lastRot != camera->getDerivedOrientation() )
                    {
                        // The Grid Buffers are "up to date" but the camera was moved
                        //(e.g. through a listener, or while rendering cubemaps)
                        // So we need to generate a new buffer for them (we can't map
                        // the same buffer twice in the same frame)
                        ++itor->currentBufIdx;
                        if( itor->currentBufIdx >= itor->gridBuffers.size() )
                            itor->gridBuffers.push_back( CachedGridBuffer() );

                        upToDate = false;
                    }
                }
                else
                {
                    itor->currentBufIdx = 0;
                }

                itor->lastPos = camera->getDerivedPosition();
                itor->lastRot = camera->getDerivedOrientation();

                *outCachedGrid = &( *itor );

                return upToDate;
            }

            ++itor;
        }

        // If we end up here, the entry doesn't exist. Create a new one.
        CachedGrid cachedGrid;
        cachedGrid.camera = camera;
        cachedGrid.lastPos = camera->getDerivedPosition();
        cachedGrid.lastRot = camera->getDerivedOrientation();
        cachedGrid.reflection = camera->isReflected();
        cachedGrid.aspectRatio = camera->getAspectRatio();
        cachedGrid.visibilityMask = visibilityMask;
        cachedGrid.shadowNode = mSceneManager->getCurrentShadowNode();
        cachedGrid.lastFrame = mVaoManager->getFrameCount();
        cachedGrid.currentBufIdx = 0;
        cachedGrid.gridBuffers.resize( 1 );

        mCachedGrid.push_back( cachedGrid );

        *outCachedGrid = &mCachedGrid.back();

        return false;
    }
    //-----------------------------------------------------------------------------------
    bool ForwardPlusBase::getCachedGridFor( const Camera *camera,
                                            const CachedGrid **outCachedGrid ) const
    {
        const uint32 visibilityMask = camera->getLastViewport()->getLightVisibilityMask();

        CachedGridVec::const_iterator itor = mCachedGrid.begin();
        CachedGridVec::const_iterator endt = mCachedGrid.end();

        while( itor != endt )
        {
            if( itor->camera == camera && itor->reflection == camera->isReflected() &&
                Math::Abs( itor->aspectRatio - camera->getAspectRatio() ) < 1e-6f &&
                itor->visibilityMask == visibilityMask &&
                itor->shadowNode == mSceneManager->getCurrentShadowNode() )
            {
                bool upToDate = itor->lastFrame == mVaoManager->getFrameCount() &&
                                itor->lastPos == camera->getDerivedPosition() &&
                                itor->lastRot == camera->getDerivedOrientation();

                *outCachedGrid = &( *itor );

                return upToDate;
            }

            ++itor;
        }

        return false;
    }
    //-----------------------------------------------------------------------------------
    void ForwardPlusBase::deleteOldGridBuffers()
    {
        // Check if some of the caches are really old and delete them
        CachedGridVec::iterator itor = mCachedGrid.begin();
        CachedGridVec::iterator endt = mCachedGrid.end();

        const uint32 currentFrame = mVaoManager->getFrameCount();

        while( itor != endt )
        {
            if( itor->lastFrame + 3 < currentFrame )
            {
                CachedGridBufferVec::iterator itBuf = itor->gridBuffers.begin();
                CachedGridBufferVec::iterator enBuf = itor->gridBuffers.end();

                while( itBuf != enBuf )
                {
                    if( itBuf->gridBuffer )
                    {
                        if( itBuf->gridBuffer->getMappingState() != MS_UNMAPPED )
                            itBuf->gridBuffer->unmap( UO_UNMAP_ALL );
                        mVaoManager->destroyTexBuffer( itBuf->gridBuffer );
                        itBuf->gridBuffer = 0;
                    }

                    if( itBuf->globalLightListBuffer )
                    {
                        if( itBuf->globalLightListBuffer->getMappingState() != MS_UNMAPPED )
                            itBuf->globalLightListBuffer->unmap( UO_UNMAP_ALL );
                        mVaoManager->destroyReadOnlyBuffer( itBuf->globalLightListBuffer );
                        itBuf->globalLightListBuffer = 0;
                    }

                    ++itBuf;
                }

                itor = efficientVectorRemove( mCachedGrid, itor );
                endt = mCachedGrid.end();
            }
            else
            {
                ++itor;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    bool ForwardPlusBase::isCacheDirty( const Camera *camera ) const
    {
        CachedGrid const *outCachedGrid = 0;
        return getCachedGridFor( camera, &outCachedGrid );
    }
    //-----------------------------------------------------------------------------------
    TexBufferPacked *ForwardPlusBase::getGridBuffer( const Camera *camera ) const
    {
        CachedGrid const *cachedGrid = 0;

#ifdef NDEBUG
        getCachedGridFor( camera, &cachedGrid );
#else
        bool upToDate = getCachedGridFor( camera, &cachedGrid );
        assert( upToDate && "You must call ForwardPlusBase::collectLights first!" );
#endif

        return cachedGrid->gridBuffers[cachedGrid->currentBufIdx].gridBuffer;
    }
    //-----------------------------------------------------------------------------------
    ReadOnlyBufferPacked *ForwardPlusBase::getGlobalLightListBuffer( const Camera *camera ) const
    {
        CachedGrid const *cachedGrid = 0;

#ifdef NDEBUG
        getCachedGridFor( camera, &cachedGrid );
#else
        bool upToDate = getCachedGridFor( camera, &cachedGrid );
        assert( upToDate && "You must call ForwardPlusBase::collectLights first!" );
#endif

        return cachedGrid->gridBuffers[cachedGrid->currentBufIdx].globalLightListBuffer;
    }
    //-----------------------------------------------------------------------------------
    void ForwardPlusBase::setHlmsPassProperties( const size_t tid, Hlms *hlms )
    {
        // ForwardPlus should be overriden by derived class to set the method in use.
        hlms->_setProperty( tid, HlmsBaseProp::ForwardPlus, 1 );
        hlms->_setProperty( tid, HlmsBaseProp::ForwardPlusDebug, mDebugMode );
        hlms->_setProperty( tid, HlmsBaseProp::ForwardPlusFadeAttenRange, mFadeAttenuationRange );
        hlms->_setProperty( tid, HlmsBaseProp::VPos, 1 );

        hlms->_setProperty( tid, HlmsBaseProp::Forward3D,
                            static_cast<int32>( HlmsBaseProp::Forward3D.getU32Value() ) );
        hlms->_setProperty( tid, HlmsBaseProp::ForwardClustered,
                            static_cast<int32>( HlmsBaseProp::ForwardClustered.getU32Value() ) );

        if( mEnableVpls )
            hlms->_setProperty( tid, HlmsBaseProp::EnableVpls, 1 );

        Viewport *viewport = mSceneManager->getCurrentViewport0();
        if( viewport->coversEntireTarget() && !hlms->_getProperty( tid, HlmsBaseProp::InstancedStereo ) )
            hlms->_setProperty( tid, HlmsBaseProp::ForwardPlusCoversEntireTarget, 1 );

#if !OGRE_NO_FINE_LIGHT_MASK_GRANULARITY
        if( mFineLightMaskGranularity )
            hlms->_setProperty( tid, HlmsBaseProp::ForwardPlusFineLightMask, 1 );
#endif
    }
}  // namespace Ogre
