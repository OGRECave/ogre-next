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

#include "OgreStagingTexture.h"

#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "Vao/OgreVaoManager.h"

namespace Ogre
{
    StagingTexture::StagingTexture( VaoManager *vaoManager, PixelFormatGpu formatFamily ) :
        mVaoManager( vaoManager ),
        mLastFrameUsed( vaoManager->getFrameCount() - vaoManager->getDynamicBufferMultiplier() ),
        mFormatFamily( formatFamily ),
        mMapRegionStarted( false )
#if OGRE_DEBUG_MODE
        ,
        mUserQueriedIfUploadWillStall( false )
#endif
    {
    }
    //-----------------------------------------------------------------------------------
    StagingTexture::~StagingTexture() {}
    //-----------------------------------------------------------------------------------
    bool StagingTexture::uploadWillStall()
    {
#if OGRE_DEBUG_MODE
        mUserQueriedIfUploadWillStall = true;
#endif
        return !mVaoManager->isFrameFinished( mLastFrameUsed );
    }
    //-----------------------------------------------------------------------------------
    void StagingTexture::startMapRegion()
    {
        assert( !mMapRegionStarted && "startMapRegion already called!" );
        mMapRegionStarted = true;

#if OGRE_DEBUG_MODE
        // We only warn if uploadWillStall wasn't called. Because if you didn't wait
        // getDynamicBufferMultiplier frames and never called uploadWillStall to check,
        // you're risking certain machines (with slower GPUs) to stall even if yours doesn't.
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
        assert( mMapRegionStarted && "You must call startMapRegion first!" );
        if( ( depth > 1u || slices > 1u ) && ( width > 2048 || height > 2048 ) )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Textures larger than 2048x2048 can only be mapped one "
                         "slice at a time due to limitations in the D3D11 API!",
                         "StagingTexture::mapRegion" );
        }

        TextureBox retVal = mapRegionImpl( width, height, depth, slices, pixelFormat );
        if( PixelFormatGpuUtils::isCompressed( pixelFormat ) )
            retVal.setCompressedPixelFormat( pixelFormat );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void StagingTexture::stopMapRegion()
    {
        assert( mMapRegionStarted && "You didn't call startMapRegion first!" );
        mMapRegionStarted = false;
    }
    //-----------------------------------------------------------------------------------
    void StagingTexture::upload( const TextureBox &srcBox, TextureGpu *dstTexture, uint8 mipLevel,
                                 const TextureBox *cpuSrcBox, const TextureBox *dstBox,
                                 bool skipSysRamCopy )
    {
        assert( !mMapRegionStarted && "You must call stopMapRegion before you can upload!" );
#if OGRE_DEBUG_MODE
        mUserQueriedIfUploadWillStall = false;
#endif
        const TextureBox fullDstTextureBox(
            std::max( 1u, dstTexture->getInternalWidth() >> mipLevel ),
            std::max( 1u, dstTexture->getInternalHeight() >> mipLevel ),
            std::max( 1u, dstTexture->getDepth() >> mipLevel ), dstTexture->getNumSlices(),
            PixelFormatGpuUtils::getBytesPerPixel( dstTexture->getPixelFormat() ),
            dstTexture->_getSysRamCopyBytesPerRow( mipLevel ),
            dstTexture->_getSysRamCopyBytesPerImage( mipLevel ) );

        assert( ( !dstBox || srcBox.equalSize( *dstBox ) ) && "Src & Dst must be equal" );
        assert( !dstBox || fullDstTextureBox.fullyContains( *dstBox ) );
        assert( mipLevel < dstTexture->getNumMipmaps() );
        assert( ( !srcBox.bytesPerRow || ( srcBox.bytesPerImage % srcBox.bytesPerRow ) == 0 ) &&
                "srcBox.bytesPerImage must be a multiple of srcBox.bytesPerRow!" );
        assert( belongsToUs( srcBox ) &&
                "This srcBox does not belong to us! Was it created with mapRegion? "
                "Did you modify it? Did it get corrupted?" );
        assert( ( !cpuSrcBox || srcBox.equalSize( *cpuSrcBox ) ) && "Src & cpuSrcBox must be equal" );

        if( dstTexture->isTilerMemoryless() )
        {
            OGRE_EXCEPT(
                Exception::ERR_INVALIDPARAMS,
                "Cannot upload to texture '" + dstTexture->getNameStr() + "' because it's memoryless.",
                "StagingTexture::upload" );
        }

        if( dstTexture->isMultisample() )
        {
            OGRE_EXCEPT(
                Exception::ERR_INVALIDPARAMS,
                "Cannot upload to texture '" + dstTexture->getNameStr() + "' because it's using MSAA",
                "StagingTexture::upload" );
        }

        if( dstTexture->getResidencyStatus() == GpuResidency::OnStorage )
        {
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         "Cannot upload to texture '" + dstTexture->getNameStr() +
                             "' which is in GpuResidency::OnStorage mode",
                         "StagingTexture::upload" );
        }

        if( !cpuSrcBox && !skipSysRamCopy )
        {
            if( dstTexture->getResidencyStatus() == GpuResidency::OnSystemRam ||
                dstTexture->getGpuPageOutStrategy() == GpuPageOutStrategy::AlwaysKeepSystemRamCopy )
            {
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                             "Cannot upload to texture '" + dstTexture->getNameStr() +
                                 "'. The parameter cpuSrcBox must not be null",
                             "StagingTexture::upload" );
            }
        }

        uint8 *sysRamCopyBase = dstTexture->_getSysRamCopy( mipLevel );
        if( sysRamCopyBase && !skipSysRamCopy )
        {
            TextureBox dstSysRamBox;

            if( !dstBox )
                dstSysRamBox = dstTexture->_getSysRamCopyAsBox( mipLevel );
            else
            {
                const PixelFormatGpu pixelFormat = dstTexture->getPixelFormat();

                dstSysRamBox = *dstBox;
                dstSysRamBox.data = sysRamCopyBase;
                dstSysRamBox.bytesPerPixel = PixelFormatGpuUtils::getBytesPerPixel( pixelFormat );
                dstSysRamBox.bytesPerRow = dstTexture->_getSysRamCopyBytesPerRow( mipLevel );
                dstSysRamBox.bytesPerImage = dstTexture->_getSysRamCopyBytesPerImage( mipLevel );

                if( PixelFormatGpuUtils::isCompressed( pixelFormat ) )
                    dstSysRamBox.setCompressedPixelFormat( pixelFormat );
            }

            dstSysRamBox.copyFrom( *cpuSrcBox );
        }

        mLastFrameUsed = mVaoManager->getFrameCount();
    }
}  // namespace Ogre
