/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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

#include "OgreMetalAsyncTextureTicket.h"
#include "OgreMetalTextureGpu.h"
#include "OgreMetalMappings.h"
#include "Vao/OgreMetalVaoManager.h"

#include "OgreMetalDevice.h"
#include "OgreStringConverter.h"

#include "OgreTextureBox.h"
#include "OgreTextureGpuManager.h"
#include "OgrePixelFormatGpuUtils.h"

#import "Metal/MTLBlitCommandEncoder.h"

namespace Ogre
{
    MetalAsyncTextureTicket::MetalAsyncTextureTicket( uint32 width, uint32 height,
                                                      uint32 depthOrSlices,
                                                      TextureTypes::TextureTypes textureType,
                                                      PixelFormatGpu pixelFormatFamily,
                                                      MetalVaoManager *vaoManager,
                                                      MetalDevice *device ) :
        AsyncTextureTicket( width, height, depthOrSlices, textureType, pixelFormatFamily ),
        mVboName( 0 ),
        mDownloadFrame( 0 ),
        mAccurateFence( 0 ),
        mVaoManager( vaoManager ),
        mDevice( device )
    {
        const uint32 rowAlignment = 4u;
        const size_t sizeBytes = PixelFormatGpuUtils::getSizeBytes( width, height, depthOrSlices,
                                                                    1u, mPixelFormatFamily,
                                                                    rowAlignment );

        MTLResourceOptions resourceOptions = MTLResourceCPUCacheModeDefaultCache |
                                             MTLResourceStorageModeShared;
        mVboName = [mDevice->mDevice newBufferWithLength:sizeBytes options:resourceOptions];
        if( !mVboName )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Out of GPU memory or driver refused.\n"
                         "Requested: " + StringConverter::toString( sizeBytes ) + " bytes.",
                         "MetalAsyncTextureTicket::MetalAsyncTextureTicket" );
        }
    }
    //-----------------------------------------------------------------------------------
    MetalAsyncTextureTicket::~MetalAsyncTextureTicket()
    {
        if( mStatus == Mapped )
            unmap();

        mVboName = 0;
        mAccurateFence = 0;
    }
    //-----------------------------------------------------------------------------------
    void MetalAsyncTextureTicket::downloadFromGpu( TextureGpu *textureSrc, uint8 mipLevel,
                                                   bool accurateTracking, TextureBox *srcBox )
    {
        AsyncTextureTicket::downloadFromGpu( textureSrc, mipLevel, accurateTracking, srcBox );

        mDownloadFrame = mVaoManager->getFrameCount();

        mAccurateFence = 0;

        TextureBox srcTextureBox;
        TextureBox fullSrcTextureBox( textureSrc->getEmptyBox( mipLevel ) );

        if( !srcBox )
            srcTextureBox = fullSrcTextureBox;
        else
        {
            srcTextureBox = *srcBox;
            srcTextureBox.bytesPerRow   = fullSrcTextureBox.bytesPerRow;
            srcTextureBox.bytesPerPixel = fullSrcTextureBox.bytesPerPixel;
            srcTextureBox.bytesPerImage = fullSrcTextureBox.bytesPerImage;
        }

        if( textureSrc->hasAutomaticBatching() )
        {
//            fullSrcTextureBox.sliceStart= textureSrc->getInternalSliceStart();
//            fullSrcTextureBox.numSlices = textureSrc->getTexturePool()->masterTexture->getNumSlices();

            srcTextureBox.sliceStart += textureSrc->getInternalSliceStart();
        }

        assert( dynamic_cast<MetalTextureGpu*>( textureSrc ) );
        MetalTextureGpu *srcTextureMetal = static_cast<MetalTextureGpu*>( textureSrc );

        __unsafe_unretained id<MTLBlitCommandEncoder> blitEncoder = mDevice->getBlitEncoder();

        size_t bytesPerRow  = srcTextureBox.bytesPerRow;
        size_t bytesPerImage= srcTextureBox.bytesPerImage;

        if( mPixelFormatFamily == PFG_PVRTC2_2BPP || mPixelFormatFamily == PFG_PVRTC2_2BPP_SRGB ||
            mPixelFormatFamily == PFG_PVRTC2_4BPP || mPixelFormatFamily == PFG_PVRTC2_4BPP_SRGB ||
            mPixelFormatFamily == PFG_PVRTC_RGB2  || mPixelFormatFamily == PFG_PVRTC_RGB2_SRGB  ||
            mPixelFormatFamily == PFG_PVRTC_RGB4  || mPixelFormatFamily == PFG_PVRTC_RGB4_SRGB  ||
            mPixelFormatFamily == PFG_PVRTC_RGBA2 || mPixelFormatFamily == PFG_PVRTC_RGBA2_SRGB ||
            mPixelFormatFamily == PFG_PVRTC_RGBA4 || mPixelFormatFamily == PFG_PVRTC_RGBA4_SRGB )
        {
            bytesPerRow = 0;
            bytesPerImage = 0;
        }

        MTLOrigin mtlOrigin = MTLOriginMake( srcTextureBox.x, srcTextureBox.y, srcTextureBox.z );
        MTLSize mtlSize     = MTLSizeMake( srcTextureBox.width, srcTextureBox.height,
                                           srcTextureBox.depth );

        for( NSUInteger i=0; i<srcTextureBox.numSlices; ++i )
        {
            [blitEncoder copyFromTexture:srcTextureMetal->getFinalTextureName()
                             sourceSlice:srcTextureBox.sliceStart
                             sourceLevel:mipLevel
                            sourceOrigin:mtlOrigin
                              sourceSize:mtlSize
                                toBuffer:mVboName
                       destinationOffset:srcTextureBox.bytesPerImage * i
                  destinationBytesPerRow:bytesPerRow
                destinationBytesPerImage:bytesPerImage];
        }

#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        [blitEncoder synchronizeResource:mVboName];
#endif

        if( accurateTracking )
        {
            mAccurateFence = dispatch_semaphore_create( 0 );
            __weak dispatch_semaphore_t blockSemaphore = mAccurateFence;
            [mDevice->mCurrentCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
            {
                if( blockSemaphore )
                    dispatch_semaphore_signal( blockSemaphore );
            }];
            //Flush now for accuracy with downloads.
            mDevice->commitAndNextCommandBuffer();
        }
    }
    //-----------------------------------------------------------------------------------
    TextureBox MetalAsyncTextureTicket::mapImpl( uint32 slice )
    {
        waitForDownloadToFinish();

        TextureBox retVal;

        retVal = TextureBox( mWidth, mHeight, getDepth(), getNumSlices(),
                             PixelFormatGpuUtils::getBytesPerPixel( mPixelFormatFamily ),
                             getBytesPerRow(), getBytesPerImage() );

        if( PixelFormatGpuUtils::isCompressed( mPixelFormatFamily ) )
            retVal.setCompressedPixelFormat( mPixelFormatFamily );

        retVal.data = [mVboName contents];
        retVal.data = retVal.at( 0, 0, slice );
        retVal.numSlices -= slice;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void MetalAsyncTextureTicket::unmapImpl(void)
    {
    }
    //-----------------------------------------------------------------------------------
    void MetalAsyncTextureTicket::waitForDownloadToFinish(void)
    {
        if( mStatus != Downloading )
            return; //We're done.

        if( mAccurateFence )
        {
            mAccurateFence = MetalVaoManager::waitFor( mAccurateFence, mDevice );
        }
        else
        {
            mVaoManager->waitForSpecificFrameToFinish( mDownloadFrame );
            mNumInaccurateQueriesWasCalledInIssuingFrame = 0;
        }

        mStatus = Ready;
    }
    //-----------------------------------------------------------------------------------
    bool MetalAsyncTextureTicket::queryIsTransferDone(void)
    {
        if( !AsyncTextureTicket::queryIsTransferDone() )
        {
            //Early out. The texture is not even finished being ready.
            //We didn't even start the actual download.
            return false;
        }

        bool retVal = false;

        if( mStatus != Downloading )
        {
            retVal = true;
        }
        else if( mAccurateFence )
        {
            //Ask GL API to return immediately and tells us about the fence
            long result = dispatch_semaphore_wait( mAccurateFence, DISPATCH_TIME_NOW );
            if( result == 0 )
            {
                mAccurateFence = 0;
                if( mStatus != Mapped )
                    mStatus = Ready;
                retVal = true;
            }
        }
        else
        {
            if( mDownloadFrame == mVaoManager->getFrameCount() )
            {
                if( mNumInaccurateQueriesWasCalledInIssuingFrame > 3 )
                {
                    //Use is not calling vaoManager->update(). Likely it's stuck in an
                    //infinite loop checking if we're done, but we'll always return false.
                    //If so, switch to accurate tracking.
                    mAccurateFence = dispatch_semaphore_create( 0 );
                    __weak dispatch_semaphore_t blockSemaphore = mAccurateFence;
                    [mDevice->mCurrentCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
                    {
                        if( blockSemaphore )
                            dispatch_semaphore_signal( blockSemaphore );
                    }];
                    //Flush now for accuracy with downloads.
                    mDevice->commitAndNextCommandBuffer();

                    LogManager::getSingleton().logMessage(
                                "WARNING: Calling AsyncTextureTicket::queryIsTransferDone too "
                                "often with innacurate tracking in the same frame this transfer "
                                "was issued. Switching to accurate tracking. If this is an accident, "
                                "wait until you've rendered a few frames before checking if it's done. "
                                "If this is on purpose, consider calling AsyncTextureTicket::download()"
                                " with accurate tracking enabled.", LML_CRITICAL );
                }

                ++mNumInaccurateQueriesWasCalledInIssuingFrame;
            }

            //We're downloading but have no fence. That means we don't have accurate tracking.
            retVal = mVaoManager->isFrameFinished( mDownloadFrame );
            ++mNumInaccurateQueriesWasCalledInIssuingFrame;
        }

        return retVal;
    }
}
