/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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

#include "OgreStagingTexture.h"
#include "OgrePixelFormatGpuUtils.h"
#include "Vao/OgreVaoManager.h"

#include "OgreLogManager.h"

namespace Ogre
{
    StagingTexture::StagingTexture( VaoManager *vaoManager, PixelFormatGpu formatFamily ) :
        mVaoManager( vaoManager ),
        mLastFrameUsed( vaoManager->getFrameCount() - vaoManager->getDynamicBufferMultiplier() ),
        mFormatFamily( formatFamily )
  #if OGRE_DEBUG_MODE
        ,mMapRegionStarted( false )
        ,mUserQueriedIfUploadWillStall( false )
  #endif
    {
    }
    //-----------------------------------------------------------------------------------
    StagingTexture::~StagingTexture()
    {
    }
    //-----------------------------------------------------------------------------------
    bool StagingTexture::supportsFormat( uint32 width, uint32 height, uint32 depth, uint32 slices,
                                         PixelFormatGpu pixelFormat ) const
    {
        return true;
    }
    //-----------------------------------------------------------------------------------
    bool StagingTexture::uploadWillStall(void)
    {
#if OGRE_DEBUG_MODE
        mUserQueriedIfUploadWillStall = true;
#endif
        return !mVaoManager->isFrameFinished( mLastFrameUsed );
    }
    //-----------------------------------------------------------------------------------
    void StagingTexture::startMapRegion(void)
    {
#if OGRE_DEBUG_MODE
        assert( !mMapRegionStarted && "startMapRegion already called!" );
        mMapRegionStarted = true;

        if( mVaoManager->getFrameCount() - mLastFrameUsed < mVaoManager->getDynamicBufferMultiplier() &&
            !mUserQueriedIfUploadWillStall )
        {
            LogManager::getSingleton().logMessage(
                "PERFORMANCE WARNING: Calling StagingTexture::startMapRegion too soon, "
                "it will very likely stall depending on HW and OS setup. You need to wait"
                " VaoManager::getDynamicBufferMultiplier frames after having called "
                "StagingTexture::upload before calling startMapRegion again.",
                LML_CRITICAL );
        }
#endif

        mVaoManager->waitForSpecificFrameToFinish( mLastFrameUsed );
    }
    //-----------------------------------------------------------------------------------
    TextureBox StagingTexture::mapRegion( uint32 width, uint32 height, uint32 depth, uint32 slices,
                                          PixelFormatGpu pixelFormat )
    {
        assert( supportsFormat( width, height, depth, slices, pixelFormat ) );
#if OGRE_DEBUG_MODE
        assert( mMapRegionStarted && "You must call startMapRegion first!" );
#endif

        return mapRegionImpl( width, height, depth, slices, pixelFormat );
    }
    //-----------------------------------------------------------------------------------
    void StagingTexture::stopMapRegion(void)
    {
#if OGRE_DEBUG_MODE
        assert( mMapRegionStarted && "You didn't call startMapRegion first!" );
        mMapRegionStarted = false;
#endif
    }
    //-----------------------------------------------------------------------------------
    void StagingTexture::upload( const TextureBox &srcBox, TextureGpu *dstTexture,
                                 uint8 mipLevel, const TextureBox *dstBox,
                                 bool skipSysRamCopy )
    {
#if OGRE_DEBUG_MODE
        assert( !mMapRegionStarted && "You must call stopMapRegion before you can upload!" );
        mUserQueriedIfUploadWillStall = false;
#endif
        const TextureBox fullDstTextureBox( std::max( 1u, dstTexture->getWidth() >> mipLevel ),
                                            std::max( 1u, dstTexture->getHeight() >> mipLevel ),
                                            std::max( 1u, dstTexture->getDepth() >> mipLevel ),
                                            dstTexture->getNumSlices(),
                                            PixelFormatGpuUtils::getBytesPerPixel(
                                                dstTexture->getPixelFormat() ),
                                            dstTexture->_getSysRamCopyBytesPerRow( mipLevel ),
                                            dstTexture->_getSysRamCopyBytesPerImage( mipLevel ) );

        assert( !dstBox || srcBox.equalSize( *dstBox ) && "Src & Dst must be equal" );
        assert( !dstBox || fullDstTextureBox.fullyContains( *dstBox ) );
        assert( fullDstTextureBox.fullyContains( srcBox ) );
        assert( mipLevel < dstTexture->getNumMipmaps() );
        assert( srcBox.x == 0 && srcBox.y == 0 && srcBox.z == 0 && srcBox.sliceStart == 0 );
        assert(( !srcBox.bytesPerRow || (srcBox.bytesPerImage % srcBox.bytesPerRow) == 0) &&
               "srcBox.bytesPerImage must be a multiple of srcBox.bytesPerRow!" );
        assert( belongsToUs( srcBox ) &&
                "This srcBox does not belong to us! Was it created with mapRegion? "
                "Did you modify it? Did it get corrupted?" );

        if( dstTexture->getMsaa() > 1u )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Cannot upload to texture '" + dstTexture->getNameStr() +
                         "' because it's using MSAA",
                         "StagingTexture::upload" );
        }

        if( dstTexture->getResidencyStatus() == GpuResidency::OnStorage )
        {
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         "Cannot upload to texture '" + dstTexture->getNameStr() +
                         "' which is in GpuResidency::OnStorage mode",
                         "StagingTexture::upload" );
        }

        uint8 *sysRamCopyBase = dstTexture->_getSysRamCopy( mipLevel );
        if( sysRamCopyBase && !skipSysRamCopy )
        {
            const size_t sysRamCopyBytesPerRow = dstTexture->_getSysRamCopyBytesPerRow( mipLevel );
            const size_t sysRamCopyBytesPerImage = dstTexture->_getSysRamCopyBytesPerImage( mipLevel );

            if( !dstBox || dstBox->equalSize( fullDstTextureBox ) )
            {
                const uint32 depthOrSlices = dstTexture->getDepthOrSlices();
                for( size_t z=0; z<depthOrSlices; ++z )
                {
                    uint8 *dstPtr = sysRamCopyBase + sysRamCopyBytesPerImage * z;
                    uint8 *srcPtr = static_cast<uint8*>( srcBox.data ) + srcBox.bytesPerImage * z;
                    memcpy( dstPtr, srcPtr, sysRamCopyBytesPerRow * dstTexture->getHeight() );
                }
            }
            else
            {
                const uint32 xPos       = dstBox ? dstBox->x : 0;
                const uint32 yPos       = dstBox ? dstBox->y : 0;
                const uint32 zPos       = dstBox ? dstBox->z : 0;
                const uint32 slicePos   = dstBox ? dstBox->sliceStart : 0;
                const uint32 width      = dstBox ? dstBox->width  :
                                                   std::max( 1u, (dstTexture->getWidth() >> mipLevel) );
                const uint32 height     = dstBox ? dstBox->height :
                                                   std::max( 1u, (dstTexture->getHeight() >> mipLevel) );
                const uint32 depth      = dstBox ? dstBox->depth  :
                                                   std::max( 1u, (dstTexture->getDepth() >> mipLevel) );
                const uint32 numSlices  = dstBox ? dstBox->numSlices : dstTexture->getNumSlices();
                const uint32 zPosOrSlice    = std::max( zPos, slicePos );
                const uint32 depthOrSlices  = std::max( depth, numSlices );

                const size_t bytesPerPixel =
                        PixelFormatGpuUtils::getBytesPerPixel( dstTexture->getPixelFormat() );

                for( size_t z=zPosOrSlice; z<zPosOrSlice + depthOrSlices; ++z )
                {
                    uint8 *dstPtr = sysRamCopyBase + sysRamCopyBytesPerImage * z + bytesPerPixel * xPos;
                    uint8 *srcPtr = static_cast<uint8*>( srcBox.data )  + srcBox.bytesPerImage * z;

                    for( size_t y=yPos; y<yPos + height; ++y )
                    {
                        memcpy( dstPtr, srcPtr, width * bytesPerPixel );
                        dstPtr += sysRamCopyBytesPerRow;
                        srcPtr += srcBox.bytesPerRow;
                    }
                }
            }
        }

        mLastFrameUsed = mVaoManager->getFrameCount();
    }
}
