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

#include "OgreD3D11AsyncTextureTicket.h"
#include "OgreD3D11TextureGpu.h"
#include "OgreD3D11Mappings.h"
#include "OgreD3D11Device.h"
#include "Vao/OgreD3D11VaoManager.h"

#include "OgreTextureBox.h"
#include "OgreTextureGpuManager.h"
#include "OgrePixelFormatGpuUtils.h"

#include "OgreLogManager.h"

namespace Ogre
{
    D3D11AsyncTextureTicket::D3D11AsyncTextureTicket( uint32 width, uint32 height,
                                                      uint32 depthOrSlices,
                                                      TextureTypes::TextureTypes textureType,
                                                      PixelFormatGpu pixelFormatFamily,
                                                      D3D11VaoManager *vaoManager ) :
        AsyncTextureTicket( width, height, depthOrSlices, textureType,
                            pixelFormatFamily ),
        mStagingTexture( 0 ),
        mDownloadFrame( 0 ),
        mAccurateFence( 0 ),
        mVaoManager( vaoManager ),
        mMappedSlice( 0 ),
        mIsArray2DTexture( false )
    {
        D3D11Device &device = mVaoManager->getDevice();

        HRESULT hr;
        if( mDepthOrSlices > 1u &&
            mWidth <= D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION &&
            mHeight <= D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION )
        {
            D3D11_TEXTURE3D_DESC desc;
            memset( &desc, 0, sizeof( desc ) );

            desc.Width      = static_cast<UINT>( mWidth );
            desc.Height     = static_cast<UINT>( mHeight );
            desc.Depth      = static_cast<UINT>( mDepthOrSlices );
            desc.MipLevels  = 1u;
            desc.Format     = D3D11Mappings::getFamily( mPixelFormatFamily );
            desc.Usage              = D3D11_USAGE_STAGING;
            desc.BindFlags          = 0;
            desc.CPUAccessFlags     = D3D11_CPU_ACCESS_READ;
            desc.MiscFlags          = 0;

            if( PixelFormatGpuUtils::isCompressed( mPixelFormatFamily ) )
            {
                desc.Width  = std::max( desc.Width, 4u );
                desc.Height = std::max( desc.Height, 4u );
            }

            ID3D11Texture3D *texture = 0;
            hr = device->CreateTexture3D( &desc, 0, &texture );
            mStagingTexture = texture;
        }
        else
        {
            D3D11_TEXTURE2D_DESC desc;
            memset( &desc, 0, sizeof( desc ) );

            desc.Width      = static_cast<UINT>( mWidth );
            desc.Height     = static_cast<UINT>( mHeight );
            desc.MipLevels  = 1u;
            desc.ArraySize  = static_cast<UINT>( mDepthOrSlices );
            desc.Format     = D3D11Mappings::getFamily( mPixelFormatFamily );
            desc.SampleDesc.Count   = 1u;
            desc.Usage              = D3D11_USAGE_STAGING;
            desc.BindFlags          = 0;
            desc.CPUAccessFlags     = D3D11_CPU_ACCESS_READ;
            desc.MiscFlags          = 0;

            if( PixelFormatGpuUtils::isCompressed( mPixelFormatFamily ) )
            {
                desc.Width  = std::max( desc.Width, 4u );
                desc.Height = std::max( desc.Height, 4u );
            }

            ID3D11Texture2D *texture = 0;
            hr = device->CreateTexture2D( &desc, 0, &texture );
            mStagingTexture = texture;
            mIsArray2DTexture = true;
        }

        if( FAILED(hr) || device.isError() )
        {
            SAFE_RELEASE( mStagingTexture );
            String errorDescription = device.getErrorDescription( hr );
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Error creating AsyncTextureTicket\nError Description:" + errorDescription,
                            "D3D11AsyncTextureTicket::D3D11AsyncTextureTicket" );
        }
    }
    //-----------------------------------------------------------------------------------
    D3D11AsyncTextureTicket::~D3D11AsyncTextureTicket()
    {
        if( mStatus == Mapped )
            unmap();

        SAFE_RELEASE( mStagingTexture );
        SAFE_RELEASE( mAccurateFence );
    }
    //-----------------------------------------------------------------------------------
    void D3D11AsyncTextureTicket::downloadFromGpu( TextureGpu *textureSrc, uint8 mipLevel,
                                                   bool accurateTracking, TextureBox *srcBox )
    {
        AsyncTextureTicket::downloadFromGpu( textureSrc, mipLevel, accurateTracking, srcBox );

        mDownloadFrame = mVaoManager->getFrameCount();

        SAFE_RELEASE( mAccurateFence );

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
            fullSrcTextureBox.sliceStart= textureSrc->getInternalSliceStart();
            fullSrcTextureBox.numSlices = textureSrc->getTexturePool()->masterTexture->getNumSlices();

            srcTextureBox.sliceStart += textureSrc->getInternalSliceStart();
        }

        const TextureTypes::TextureTypes textureType = textureSrc->getInternalTextureType();

        D3D11_BOX srcBoxD3d;

        srcBoxD3d.left  = srcTextureBox.x;
        srcBoxD3d.top   = srcTextureBox.y;
        srcBoxD3d.front = srcTextureBox.z;
        srcBoxD3d.right = srcTextureBox.x + srcTextureBox.width;
        srcBoxD3d.bottom= srcTextureBox.y + srcTextureBox.height;
        srcBoxD3d.back  = srcTextureBox.z + srcTextureBox.depth;

        if( PixelFormatGpuUtils::isCompressed( mPixelFormatFamily ) )
        {
            uint32 blockWidth =
                    PixelFormatGpuUtils::getCompressedBlockWidth( mPixelFormatFamily, false );
            uint32 blockHeight=
                    PixelFormatGpuUtils::getCompressedBlockHeight( mPixelFormatFamily, false );
            srcBoxD3d.right     = alignToNextMultiple( srcBoxD3d.right, blockWidth );
            srcBoxD3d.bottom    = alignToNextMultiple( srcBoxD3d.bottom, blockHeight );
        }

        UINT srcSlicePos = srcTextureBox.sliceStart;

        //These are the possibilities:
        //  Volume -> Array
        //  Array  -> Array
        //  Volume -> Volume
        //  Array  -> Volume
        //If we're an Array, we need to copy 1 slice at a time, no matter what type src is.
        UINT numSlices = srcTextureBox.numSlices;
        if( mIsArray2DTexture )
        {
            numSlices = srcTextureBox.getDepthOrSlices();
            srcBoxD3d.back = srcBoxD3d.front + 1u;
        }
        UINT zPos = 0;
        UINT dstSlicePos = 0;

        assert( dynamic_cast<D3D11TextureGpu*>( textureSrc ) );
        D3D11TextureGpu *srcTextureD3d = static_cast<D3D11TextureGpu*>( textureSrc );

        D3D11Device &device = mVaoManager->getDevice();
        ID3D11DeviceContextN *context = device.GetImmediateContext();

        for( UINT slice=0; slice<numSlices; ++slice )
        {
            const UINT srcSubResourceIdx = D3D11CalcSubresource( mipLevel, srcSlicePos,
                                                                 textureSrc->getNumMipmaps() );
            const UINT dstSubResourceIdx = D3D11CalcSubresource( 0, dstSlicePos, 1u );

            context->CopySubresourceRegion( mStagingTexture, dstSubResourceIdx,
                                            0, 0, zPos, srcTextureD3d->getFinalTextureName(),
                                            srcSubResourceIdx, &srcBoxD3d );
            if( textureType == TextureTypes::Type3D )
            {
                ++srcBoxD3d.front;
                ++srcBoxD3d.back;
            }
            else
            {
                ++srcSlicePos;
            }
            if( !mIsArray2DTexture )
                ++zPos;
            else
                ++dstSlicePos;
        }

        if( device.isError() )
        {
            String errorDescription = device.getErrorDescription();
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Error after calling CopySubresourceRegion\n"
                         "Error Description:" + errorDescription,
                         "D3D11StagingTexture::upload" );
        }

        if( accurateTracking )
            mAccurateFence = mVaoManager->createFence();
    }
    //-----------------------------------------------------------------------------------
    TextureBox D3D11AsyncTextureTicket::mapImpl( uint32 slice )
    {
        waitForDownloadToFinish();

        D3D11Device &device = mVaoManager->getDevice();
        ID3D11DeviceContextN *context = device.GetImmediateContext();

        mMappedSlice = slice;
        const UINT subresourceIdx = D3D11CalcSubresource( 0, slice, 1u );

        D3D11_MAPPED_SUBRESOURCE mappedSubresource;
        context->Map( mStagingTexture, subresourceIdx, D3D11_MAP_READ, 0, &mappedSubresource );

        TextureBox retVal( mWidth, mHeight, getDepth(), getNumSlices(),
                           PixelFormatGpuUtils::getBytesPerPixel( mPixelFormatFamily ),
                           mappedSubresource.RowPitch, mappedSubresource.DepthPitch );
        retVal.data = mappedSubresource.pData;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void D3D11AsyncTextureTicket::unmapImpl(void)
    {
        const UINT subresourceIdx = D3D11CalcSubresource( 0, mMappedSlice, 1u );

        D3D11Device &device = mVaoManager->getDevice();
        ID3D11DeviceContextN *context = device.GetImmediateContext();
        context->Unmap( mStagingTexture, subresourceIdx );
    }
    //-----------------------------------------------------------------------------------
    bool D3D11AsyncTextureTicket::canMapMoreThanOneSlice(void) const
    {
        return !mIsArray2DTexture;
    }
    //-----------------------------------------------------------------------------------
    void D3D11AsyncTextureTicket::waitForDownloadToFinish(void)
    {
        if( mStatus != Downloading )
            return; //We're done.

        if( mAccurateFence )
        {
            mAccurateFence = mVaoManager->waitFor( mAccurateFence );
            SAFE_RELEASE( mAccurateFence );
        }
        else
        {
            mVaoManager->waitForSpecificFrameToFinish( mDownloadFrame );
            mNumInaccurateQueriesWasCalledInIssuingFrame = 0;
        }

        mStatus = Ready;
    }
    //-----------------------------------------------------------------------------------
    bool D3D11AsyncTextureTicket::queryIsTransferDone(void)
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
            //Ask D3D11 API to return immediately and tells us about the fence
            if( mVaoManager->queryIsDone( mAccurateFence ) )
            {
                SAFE_RELEASE( mAccurateFence );
                if( mStatus != Mapped )
                    mStatus = Ready;
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
                    mAccurateFence = mVaoManager->createFence();

                    D3D11Device &device = mVaoManager->getDevice();
                    ID3D11DeviceContextN *context = device.GetImmediateContext();
                    context->Flush();

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
