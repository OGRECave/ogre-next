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

#include "OgreNULLAsyncTextureTicket.h"
#include "OgreNULLTextureGpu.h"
#include "Vao/OgreNULLVaoManager.h"

#include "OgreTextureBox.h"
#include "OgreTextureGpuManager.h"
#include "OgrePixelFormatGpuUtils.h"

namespace Ogre
{
    NULLAsyncTextureTicket::NULLAsyncTextureTicket( uint32 width, uint32 height,
                                                    uint32 depthOrSlices,
                                                    TextureTypes::TextureTypes textureType,
                                                    PixelFormatGpu pixelFormatFamily ) :
        AsyncTextureTicket( width, height, depthOrSlices, textureType,
                            pixelFormatFamily ),
        mVboName( 0 )
    {
        const uint32 rowAlignment = 4u;
        const size_t sizeBytes = PixelFormatGpuUtils::getSizeBytes( width, height, depthOrSlices,
                                                                    1u, mPixelFormatFamily,
                                                                    rowAlignment );
        mVboName = reinterpret_cast<uint8*>( OGRE_MALLOC_SIMD( sizeBytes, MEMCATEGORY_RENDERSYS ) );
        memset( mVboName, 0, sizeBytes );
    }
    //-----------------------------------------------------------------------------------
    NULLAsyncTextureTicket::~NULLAsyncTextureTicket()
    {
        if( mStatus == Mapped )
            unmap();

        if( mVboName )
        {
            OGRE_FREE_SIMD( mVboName, MEMCATEGORY_RENDERSYS );
            mVboName = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    TextureBox NULLAsyncTextureTicket::mapImpl( uint32 slice )
    {
        mStatus = Ready;

        TextureBox retVal;

        size_t sizeBytes = 0;
        const uint32 rowAlignment = 4u;
        retVal = TextureBox( mWidth, mHeight, getDepth(), getNumSlices(),
                             PixelFormatGpuUtils::getBytesPerPixel( mPixelFormatFamily ),
                             getBytesPerRow(), getBytesPerImage() );
        sizeBytes = PixelFormatGpuUtils::getSizeBytes( mWidth, mHeight, mDepthOrSlices,
                                                       1u, mPixelFormatFamily,
                                                       rowAlignment );

        if( PixelFormatGpuUtils::isCompressed( mPixelFormatFamily ) )
            retVal.setCompressedPixelFormat( mPixelFormatFamily );

        retVal.data = retVal.at( 0, 0, slice );
        retVal.numSlices -= slice;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void NULLAsyncTextureTicket::unmapImpl(void)
    {
    }
    //-----------------------------------------------------------------------------------
    bool NULLAsyncTextureTicket::queryIsTransferDone(void)
    {
        if( !AsyncTextureTicket::queryIsTransferDone() )
        {
            //Early out. The texture is not even finished being ready.
            //We didn't even start the actual download.
            return false;
        }

        mStatus = Ready;
        return true;
    }
}
