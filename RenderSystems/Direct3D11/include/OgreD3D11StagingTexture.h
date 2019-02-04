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

#ifndef _OgreD3D11StagingTexture_H_
#define _OgreD3D11StagingTexture_H_

#include "OgreD3D11Prerequisites.h"
#include "OgreStagingTextureBufferImpl.h"

#include "Vao/OgreD3D11DynamicBuffer.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class _OgreD3D11Export D3D11StagingTexture : public StagingTexture
    {
        typedef vector<D3D11_MAPPED_SUBRESOURCE>::type D3D11_MAPPED_SUBRESOURCEVec;

        /// mStagingTexture is either a 3D texture (if depthOrSlices > 1 and
        /// resolution is below 2048x2048) or else it's a 2D array texture.
        /// Fortunately D3D11 allows us to copy slices of 3D textures into
        /// 2D textures; so we use 3D Staging Textures whenever possible
        /// because they can be mapped like in other APIs.
        ID3D11Resource *mStagingTexture;
        D3D11_MAPPED_SUBRESOURCEVec mSubresourceData;
        D3D11_MAPPED_SUBRESOURCEVec mLastSubresourceData;
        uint32 mWidth;
        uint32 mHeight;
        uint32 mDepthOrSlices;
        bool mIsArray2DTexture;
        D3D11Device &mDevice;

        struct StagingBox
        {
            uint32 x;
            uint32 y;
            uint32 width;
            uint32 height;
            StagingBox() {}
            StagingBox( uint32 _x, uint32 _y, uint32 _width, uint32 _height ) :
                x( _x ), y( _y ), width( _width ), height( _height ) {}

            bool contains( uint32 _x, uint32 _y, uint32 _width, uint32 _height ) const
            {
                return !( _x >= this->x + this->width ||
                          _y >= this->y + this->height ||
                          this->x >= _x + _width ||
                          this->y >= _y + _height );
            }
        };
        typedef vector<StagingBox>::type StagingBoxVec;
        typedef vector<StagingBoxVec>::type StagingBoxVecVec;

        StagingBoxVecVec mFreeBoxes;

        uint32 findRealSlice( void *data ) const;

        virtual bool belongsToUs( const TextureBox &box );

        void shrinkRecords( size_t slice, StagingBoxVec::iterator record,
                            TextureBox consumedBox );
        void shrinkMultisliceRecords( size_t slice, StagingBoxVec::iterator record,
                                      const TextureBox &consumedBox );
        TextureBox mapMultipleSlices( uint32 width, uint32 height, uint32 depth, uint32 slices,
                                      PixelFormatGpu pixelFormat );
        virtual TextureBox mapRegionImpl( uint32 width, uint32 height, uint32 depth, uint32 slices,
                                          PixelFormatGpu pixelFormat );

    public:
        D3D11StagingTexture( VaoManager *vaoManager, PixelFormatGpu formatFamily,
                             uint32 width, uint32 height, uint32 depthOrSlices,
                             D3D11Device &device );
        virtual ~D3D11StagingTexture();

        virtual bool supportsFormat( uint32 width, uint32 height, uint32 depth, uint32 slices,
                                     PixelFormatGpu pixelFormat ) const;
        virtual bool isSmallerThan( const StagingTexture *other ) const;
        virtual size_t _getSizeBytes(void);

        virtual void startMapRegion(void);
        virtual void stopMapRegion(void);

        virtual void upload( const TextureBox &srcBox, TextureGpu *dstTexture,
                             uint8 mipLevel, const TextureBox *cpuSrcBox=0,
                             const TextureBox *dstBox=0, bool skipSysRamCopy=false );
    };
}

#include "OgreHeaderSuffix.h"

#endif
