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

#include "OgreStableHeaders.h"

#include "OgreAsyncTextureTicket.h"

#include "OgreTextureBox.h"
#include "OgrePixelFormatGpuUtils.h"

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
    }
    //-----------------------------------------------------------------------------------
    void AsyncTextureTicket::download( TextureGpu *textureSrc, uint8 mipLevel,
                                       bool accurateTracking, TextureBox *srcBox )
    {
        TextureBox srcTextureBox;
        const TextureBox fullSrcTextureBox( std::max( 1u, textureSrc->getWidth() >> mipLevel ),
                                            std::max( 1u, textureSrc->getHeight() >> mipLevel ),
                                            std::max( 1u, textureSrc->getDepth() >> mipLevel ),
                                            textureSrc->getNumSlices(),
                                            PixelFormatGpuUtils::getBytesPerPixel(
                                                textureSrc->getPixelFormat() ),
                                            textureSrc->_getSysRamCopyBytesPerRow( mipLevel ),
                                            textureSrc->_getSysRamCopyBytesPerImage( mipLevel ) );

        if( !srcBox )
            srcTextureBox = fullSrcTextureBox;
        else
        {
            srcTextureBox = *srcBox;
            srcTextureBox.bytesPerRow   = fullSrcTextureBox.bytesPerRow;
            srcTextureBox.bytesPerPixel = fullSrcTextureBox.bytesPerPixel;
            srcTextureBox.bytesPerImage = fullSrcTextureBox.bytesPerImage;
        }

        if( mStatus != Mapped )
        {
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         "Cannot download to mapped texture. You must call unmap first!",
                         "AsyncTextureTicket::download" );
        }

        assert( mipLevel < textureSrc->getNumMipmaps() );
        assert( mPixelFormatFamily == PixelFormatGpuUtils::getFamily( textureSrc->getPixelFormat() ) );
        assert( fullSrcTextureBox.fullyContains( srcTextureBox ) );
        assert( srcTextureBox.width == mWidth );
        assert( srcTextureBox.height == mHeight );
        assert( srcTextureBox.getDepthOrSlices() == mDepthOrSlices );
        assert( textureSrc->getMsaa() <= 1u && "Cannot download from an MSAA texture!" );

        mNumInaccurateQueriesWasCalledInIssuingFrame = 0;

        mStatus = Downloading;
    }
    //-----------------------------------------------------------------------------------
    TextureBox AsyncTextureTicket::map(void)
    {
        assert( mStatus == Ready );

        TextureBox retVal = mapImpl();
        mStatus = Mapped;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void AsyncTextureTicket::unmap(void)
    {
        assert( mStatus == Mapped );
        unmapImpl();
        mStatus = Ready;
    }
    //-----------------------------------------------------------------------------------
    uint32 AsyncTextureTicket::getWidth(void) const
    {
        return mWidth;
    }
    //-----------------------------------------------------------------------------------
    uint32 AsyncTextureTicket::getHeight(void) const
    {
        return mHeight;
    }
    //-----------------------------------------------------------------------------------
    uint32 AsyncTextureTicket::getDepthOrSlices(void) const
    {
        return mDepthOrSlices;
    }
    //-----------------------------------------------------------------------------------
    uint32 AsyncTextureTicket::getDepth(void) const
    {
        return (mTextureType != TextureTypes::Type3D) ? 1u : mDepthOrSlices;
    }
    //-----------------------------------------------------------------------------------
    uint32 AsyncTextureTicket::getNumSlices(void) const
    {
        return (mTextureType != TextureTypes::Type3D) ? mDepthOrSlices : 1u;
    }
}
