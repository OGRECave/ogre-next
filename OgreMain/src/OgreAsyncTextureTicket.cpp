/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#include "OgreStableHeaders.h"

#include "OgreAsyncTextureTicket.h"

#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreTextureBox.h"

namespace Ogre
{
    AsyncTextureTicket::AsyncTextureTicket( uint32 width, uint32 height, uint32 depthOrSlices,
                                            TextureTypes::TextureTypes textureType,
                                            PixelFormatGpu pixelFormatFamily ) :
        mStatus( Ready ),
        mWidth( width ),
        mHeight( height ),
        mDepthOrSlices( depthOrSlices ),
        mTextureType( textureType ),
        mPixelFormatFamily( pixelFormatFamily ),
        mNumInaccurateQueriesWasCalledInIssuingFrame( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    AsyncTextureTicket::~AsyncTextureTicket()
    {
        if( mStatus == Mapped )
            unmap();

        if( mDelayedDownload.textureSrc )
        {
            mDelayedDownload.textureSrc->removeListener( this );
            mDelayedDownload = DelayedDownload();
        }
    }
    //-----------------------------------------------------------------------------------
    void AsyncTextureTicket::downloadFromGpu( TextureGpu *textureSrc, uint8 mipLevel,
                                              bool accurateTracking, TextureBox *srcBox )
    {
        TextureBox srcTextureBox;
        const TextureBox fullSrcTextureBox( textureSrc->getEmptyBox( mipLevel ) );

        if( !srcBox )
            srcTextureBox = fullSrcTextureBox;
        else
        {
            srcTextureBox = *srcBox;
            srcTextureBox.bytesPerRow = fullSrcTextureBox.bytesPerRow;
            srcTextureBox.bytesPerPixel = fullSrcTextureBox.bytesPerPixel;
            srcTextureBox.bytesPerImage = fullSrcTextureBox.bytesPerImage;
        }

        assert( mipLevel < textureSrc->getNumMipmaps() );
        assert( mPixelFormatFamily == PixelFormatGpuUtils::getFamily( textureSrc->getPixelFormat() ) );
        assert( fullSrcTextureBox.fullyContains( srcTextureBox ) );
        assert( srcTextureBox.width == mWidth );
        assert( srcTextureBox.height == mHeight );
        assert( srcTextureBox.getDepthOrSlices() == mDepthOrSlices );
        assert( ( !textureSrc->isMultisample() || !textureSrc->hasMsaaExplicitResolves() ||
                  textureSrc->isOpenGLRenderWindow() ) &&
                "Cannot download from an explicitly resolved MSAA texture!" );
        OGRE_ASSERT_LOW( !textureSrc->isTilerMemoryless() &&
                         "Cannot download from a memoryless texture!" );
    }
    //-----------------------------------------------------------------------------------
    void AsyncTextureTicket::notifyTextureChanged( TextureGpu *texture,
                                                   TextureGpuListener::Reason reason, void *extraData )
    {
        if( reason == ReadyForRendering )
        {
            mDelayedDownload.textureSrc->removeListener( this );
            downloadFromGpu( mDelayedDownload.textureSrc, mDelayedDownload.mipLevel,
                             mDelayedDownload.accurateTracking,
                             mDelayedDownload.hasSrcBox ? &mDelayedDownload.srcBox : 0 );
            mDelayedDownload = DelayedDownload();
        }
        else if( reason == LostResidency )
        {
            mDelayedDownload.textureSrc->removeListener( this );
            mDelayedDownload = DelayedDownload();
            LogManager::getSingleton().logMessage(
                "WARNING: AsyncTextureTicket was waiting on texture to become "
                "ready to download its contents, but Texture '" +
                    texture->getNameStr() + "' lost residency!",
                LML_CRITICAL );
            mStatus = Ready;
        }
    }
    //-----------------------------------------------------------------------------------
    void AsyncTextureTicket::download( TextureGpu *textureSrc, uint8 mipLevel, bool accurateTracking,
                                       TextureBox *srcBox, bool bImmediate )
    {
        if( mDelayedDownload.textureSrc )
        {
            mDelayedDownload.textureSrc->removeListener( this );
            mDelayedDownload = DelayedDownload();
        }

        if( mStatus == Mapped )
        {
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         "Cannot download to mapped texture. You must call unmap first!",
                         "AsyncTextureTicket::download" );
        }

        if( ( bImmediate && ( !textureSrc->_isDataReadyImpl() &&
                              textureSrc->getResidencyStatus() == GpuResidency::Resident ) ) ||
            ( !textureSrc->isDataReady() &&
              textureSrc->getNextResidencyStatus() == GpuResidency::Resident &&
              textureSrc->getPendingResidencyChanges() <= 1u ) )
        {
            // Texture is not resident but soon will be, or is resident but not yet ready.
            // Register ourselves to listen for when that happens, we'll download then.
            textureSrc->addListener( this );
            mDelayedDownload = DelayedDownload( textureSrc, mipLevel, accurateTracking, srcBox );
        }
        else if( textureSrc->getResidencyStatus() != GpuResidency::Resident )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Only Resident textures can be downloaded via AsyncTextureTicket. "
                         "Trying to download texture: '" +
                             textureSrc->getNameStr() + "'",
                         "AsyncTextureTicket::download" );
        }
        else
        {
            // Download now!
            downloadFromGpu( textureSrc, mipLevel, accurateTracking, srcBox );
        }

        mNumInaccurateQueriesWasCalledInIssuingFrame = 0;

        mStatus = Downloading;
    }
    //-----------------------------------------------------------------------------------
    TextureBox AsyncTextureTicket::map( uint32 slice )
    {
        assert( mStatus != Mapped );
        assert( slice < getNumSlices() );

        if( mDelayedDownload.textureSrc )
            mDelayedDownload.textureSrc->waitForData();

        TextureBox retVal = mapImpl( slice );
        mStatus = Mapped;

        if( PixelFormatGpuUtils::isCompressed( mPixelFormatFamily ) )
            retVal.setCompressedPixelFormat( mPixelFormatFamily );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void AsyncTextureTicket::unmap()
    {
        assert( mStatus == Mapped );
        unmapImpl();
        mStatus = Ready;
    }
    //-----------------------------------------------------------------------------------
    uint32 AsyncTextureTicket::getWidth() const { return mWidth; }
    //-----------------------------------------------------------------------------------
    uint32 AsyncTextureTicket::getHeight() const { return mHeight; }
    //-----------------------------------------------------------------------------------
    uint32 AsyncTextureTicket::getDepthOrSlices() const { return mDepthOrSlices; }
    //-----------------------------------------------------------------------------------
    uint32 AsyncTextureTicket::getDepth() const
    {
        return ( mTextureType != TextureTypes::Type3D ) ? 1u : mDepthOrSlices;
    }
    //-----------------------------------------------------------------------------------
    uint32 AsyncTextureTicket::getNumSlices() const
    {
        return ( mTextureType != TextureTypes::Type3D ) ? mDepthOrSlices : 1u;
    }
    //-----------------------------------------------------------------------------------
    PixelFormatGpu AsyncTextureTicket::getPixelFormatFamily() const { return mPixelFormatFamily; }
    //-----------------------------------------------------------------------------------
    uint32 AsyncTextureTicket::getBytesPerRow() const
    {
        return (uint32)PixelFormatGpuUtils::getSizeBytes( mWidth, 1u, 1u, 1u, mPixelFormatFamily, 4u );
    }
    //-----------------------------------------------------------------------------------
    size_t AsyncTextureTicket::getBytesPerImage() const
    {
        return PixelFormatGpuUtils::getSizeBytes( mWidth, mHeight, 1u, 1u, mPixelFormatFamily, 4u );
    }
    //-----------------------------------------------------------------------------------
    bool AsyncTextureTicket::queryIsTransferDone() { return mDelayedDownload.textureSrc == 0; }
}  // namespace Ogre
