/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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

#include "Cubemaps/OgrePccPerPixelGridPlacement.h"

#include "Cubemaps/OgreParallaxCorrectedCubemapAuto.h"

#include "OgreAsyncTextureTicket.h"
#include "OgreTextureGpuManager.h"

#define TODO_d3d11_path
#define TODO_dpm_path

namespace Ogre
{
    PccPerPixelGridPlacement::PccPerPixelGridPlacement() :
        mFullRegion( Vector3::ZERO, Vector3::UNIT_SCALE ),
        mOverlap( Vector3( 1.5f ) ),
        mPcc( 0 )
    {
        mNumProbes[0] = 3u;
        mNumProbes[1] = 3u;
        mNumProbes[2] = 3u;
    }
    //-------------------------------------------------------------------------
    PccPerPixelGridPlacement::~PccPerPixelGridPlacement() {}
    //-------------------------------------------------------------------------
    void PccPerPixelGridPlacement::setParallaxCorrectedCubemapAuto( ParallaxCorrectedCubemapAuto *pcc )
    {
        OGRE_ASSERT_LOW( mAsyncTicket.empty() && "Cannot change in the middle of buildBegin/buildEnd" );
        mPcc = pcc;
    }
    //-------------------------------------------------------------------------
    void PccPerPixelGridPlacement::setNumProbes( uint32 numProbes[3] )
    {
        OGRE_ASSERT_LOW( mAsyncTicket.empty() && "Cannot change in the middle of buildBegin/buildEnd" );
        for( size_t i = 0u; i < 3u; ++i )
            mNumProbes[i] = numProbes[i];
    }
    //-------------------------------------------------------------------------
    void PccPerPixelGridPlacement::setFullRegion( const Aabb &fullRegion )
    {
        OGRE_ASSERT_LOW( mAsyncTicket.empty() && "Cannot change in the middle of buildBegin/buildEnd" );
        mFullRegion = fullRegion;
    }
    //-------------------------------------------------------------------------
    void PccPerPixelGridPlacement::setOverlap( const Vector3 &overlap )
    {
        OGRE_ASSERT_LOW( mAsyncTicket.empty() && "Cannot change in the middle of buildBegin/buildEnd" );
        mOverlap = overlap;
    }
    //-------------------------------------------------------------------------
    uint32 PccPerPixelGridPlacement::getMaxNumProbes() const
    {
        return mNumProbes[0] * mNumProbes[1] * mNumProbes[2];
    }
    //-------------------------------------------------------------------------
    void PccPerPixelGridPlacement::processProbeDepth( TextureBox box, size_t probeIdx, size_t sliceIdx )
    {
        CubemapProbe *probe = mPcc->getProbes()[probeIdx];
        const PixelFormatGpu pixelFormat = mPcc->getBindTexture()->getPixelFormat();

        ColourValue colourVal[6];
        for( size_t i = 0u; i < 6u; ++i )
            colourVal[i] = box.getColourAt( 0u, 0u, sliceIdx + i, pixelFormat );

        const Vector3 probeAreaHalfSize( mOverlap * mFullRegion.mHalfSize /
                                         Vector3( mNumProbes[0], mNumProbes[1], mNumProbes[2] ) );

        const Vector3 probeShapeCenter = probe->getProbeShape().mCenter;

        const Vector3 camCenter( probe->getProbeCameraPos() );
        const Vector3 camCenterLS( probe->getInvOrientation() * ( camCenter - probeShapeCenter ) );
        Vector3 probeAreaMax;
        Vector3 probeAreaMin;
        probeAreaMax.x = camCenterLS.x + ( mFullRegion.mHalfSize.x - camCenterLS.x ) *
                                             colourVal[CubemapSide::PX].a * 2.0f;
        probeAreaMin.x = camCenterLS.x + ( -mFullRegion.mHalfSize.x - camCenterLS.x ) *
                                             colourVal[CubemapSide::NX].a * 2.0f;
        probeAreaMax.y = camCenterLS.y + ( mFullRegion.mHalfSize.y - camCenterLS.y ) *
                                             colourVal[CubemapSide::PY].a * 2.0f;
        probeAreaMin.y = camCenterLS.y + ( -mFullRegion.mHalfSize.y - camCenterLS.y ) *
                                             colourVal[CubemapSide::NY].a * 2.0f;
        probeAreaMax.z = camCenterLS.z + ( mFullRegion.mHalfSize.z - camCenterLS.z ) *
                                             colourVal[CubemapSide::PZ].a * 2.0f;
        probeAreaMin.z = camCenterLS.z + ( -mFullRegion.mHalfSize.z - camCenterLS.z ) *
                                             colourVal[CubemapSide::NZ].a * 2.0f;
        probeAreaMin += probeShapeCenter;
        probeAreaMax += probeShapeCenter;

        Aabb probeShape;
        probeShape.setExtents( probeAreaMin, probeAreaMax );
        probeShape.mHalfSize *= 1.05f;
        Aabb area( camCenter, probeAreaHalfSize );
        probe->set( camCenter, area, Vector3( 0.5f ), Matrix3::IDENTITY, probeShape );
    }
    //-------------------------------------------------------------------------
    Vector3 PccPerPixelGridPlacement::getProbeNormalizedCenter( size_t probeIdx ) const
    {
        const uint32 xPos = probeIdx % mNumProbes[0];
        const uint32 yPos = ( probeIdx / mNumProbes[0] ) % mNumProbes[1];
        const uint32 zPos = static_cast<uint32>( probeIdx / ( mNumProbes[0] * mNumProbes[1] ) );

        const Vector3 normalizedCenter( ( xPos + 0.5f ) / static_cast<float>( mNumProbes[0] ),
                                        ( yPos + 0.5f ) / static_cast<float>( mNumProbes[1] ),
                                        ( zPos + 0.5f ) / static_cast<float>( mNumProbes[2] ) );
        return normalizedCenter;
    }
    //-------------------------------------------------------------------------
    void PccPerPixelGridPlacement::buildStart( uint32 resolution, Camera *camera,
                                               PixelFormatGpu pixelFormat, float camNear, float camFar )
    {
        OGRE_ASSERT_LOW( mPcc && "Call setParallaxCorrectedCubemapAuto first!" );

        const uint32 maxNumProbes = getMaxNumProbes();

        mPcc->setEnabled( false, resolution, resolution, maxNumProbes, pixelFormat );
        mPcc->setEnabled( true, resolution, resolution, maxNumProbes, pixelFormat );
        mPcc->setUpdatedTrackedDataFromCamera( camera );

        const Vector3 probeAreaHalfSize( mOverlap * mFullRegion.mHalfSize /
                                         Vector3( mNumProbes[0], mNumProbes[1], mNumProbes[2] ) );
        const Vector3 regionOrigin( mFullRegion.getMinimum() );

        for( size_t i = 0u; i < maxNumProbes; ++i )
        {
            CubemapProbe *probe = mPcc->createProbe();
            const Vector3 normalizedCenter( getProbeNormalizedCenter( i ) );

            const Vector3 camCenter( normalizedCenter * mFullRegion.mHalfSize * 2.0f + regionOrigin );
            Aabb area( camCenter, probeAreaHalfSize );
            probe->setTextureParams( resolution, resolution );
            probe->initWorkspace( camNear, camFar );
            probe->set( camCenter, area, Vector3( 0.05f ), Matrix3::IDENTITY, mFullRegion );
        }

        mPcc->updateAllDirtyProbes();

        TextureGpu *cubemapTex = mPcc->getBindTexture();
        TextureGpuManager *textureManager = cubemapTex->getTextureManager();
        AsyncTextureTicket *asyncTicket = textureManager->createAsyncTextureTicket(
            1u, 1u, cubemapTex->getNumSlices(), TextureTypes::TypeCube, cubemapTex->getPixelFormat() );
        asyncTicket->download( cubemapTex, cubemapTex->getNumMipmaps() - 1u, false );
        mAsyncTicket.push_back( asyncTicket );
    }
    //-------------------------------------------------------------------------
    void PccPerPixelGridPlacement::buildEnd( void )
    {
        OGRE_ASSERT_LOW( mPcc && "Call setParallaxCorrectedCubemapAuto first!" );
        OGRE_ASSERT_LOW( !mAsyncTicket.empty() && "Call buildStart first!" );

        const uint32 maxNumProbes = getMaxNumProbes();

        TextureGpu *cubemapTex = mPcc->getBindTexture();
        TextureGpuManager *textureManager = cubemapTex->getTextureManager();

        FastArray<AsyncTextureTicket *>::const_iterator itor = mAsyncTicket.begin();
        FastArray<AsyncTextureTicket *>::const_iterator endt = mAsyncTicket.end();

        while( itor != endt )
        {
            AsyncTextureTicket *asyncTicket = *itor;

            if( asyncTicket->canMapMoreThanOneSlice() )
            {
                const TextureBox box = asyncTicket->map( 0u );
                for( size_t i = 0u; i < maxNumProbes; ++i )
                    processProbeDepth( box, i, i * 6u );
                asyncTicket->unmap();
            }
            else
            {
                for( size_t i = 0; i < asyncTicket->getNumSlices(); ++i )
                {
                    const TextureBox box = asyncTicket->map( (uint32)i );
                    TODO_d3d11_path;
                    asyncTicket->unmap();
                }
            }

            TODO_dpm_path;

            textureManager->destroyAsyncTextureTicket( asyncTicket );

            ++itor;
        }

        mAsyncTicket.clear();
        mPcc->updateAllDirtyProbes();
    }
}  // namespace Ogre
