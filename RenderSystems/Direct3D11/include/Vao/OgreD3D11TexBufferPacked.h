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

#ifndef _Ogre_D3D11TexBufferPacked_H_
#define _Ogre_D3D11TexBufferPacked_H_

#include "OgreD3D11Prerequisites.h"

#include "OgreD3D11DeviceResource.h"
#include "OgreDescriptorSetTexture.h"
#include "Vao/OgreTexBufferPacked.h"

namespace Ogre
{
    class D3D11BufferInterface;

    class _OgreD3D11Export D3D11TexBufferPacked final : public TexBufferPacked,
                                                        protected D3D11DeviceResource
    {
        DXGI_FORMAT  mInternalFormat;
        D3D11Device &mDevice;

        struct CachedResourceView
        {
            ComPtr<ID3D11ShaderResourceView> mResourceView;
            uint32                           mOffset;
            uint32                           mSize;
        };

        CachedResourceView mCachedResourceViews[16];
        uint8              mCurrentCacheCursor;

        bool isD3D11Structured() const;

        ID3D11ShaderResourceView *createResourceView( int cacheIdx, uint32 offset, uint32 sizeBytes );
        ID3D11ShaderResourceView *bindBufferCommon( size_t offset, size_t sizeBytes );

        void notifyDeviceLost( D3D11Device *device ) override;
        void notifyDeviceRestored( D3D11Device *device, unsigned pass ) override;

    public:
        D3D11TexBufferPacked( size_t internalBufStartBytes, size_t numElements, uint32 bytesPerElement,
                              uint32 numElementsPadding, BufferType bufferType, void *initialData,
                              bool keepAsShadow, VaoManager *vaoManager,
                              BufferInterface *bufferInterface, PixelFormatGpu pf, bool bIsStructured,
                              D3D11Device &device );
        ~D3D11TexBufferPacked() override;

        ComPtr<ID3D11ShaderResourceView> createSrv(
            const DescriptorSetTexture2::BufferSlot &bufferSlot ) const;

        void bindBufferVS( uint16 slot, size_t offset = 0, size_t sizeBytes = 0 ) override;
        void bindBufferPS( uint16 slot, size_t offset = 0, size_t sizeBytes = 0 ) override;
        void bindBufferGS( uint16 slot, size_t offset = 0, size_t sizeBytes = 0 ) override;
        void bindBufferDS( uint16 slot, size_t offset = 0, size_t sizeBytes = 0 ) override;
        void bindBufferHS( uint16 slot, size_t offset = 0, size_t sizeBytes = 0 ) override;
        void bindBufferCS( uint16 slot, size_t offset = 0, size_t sizeBytes = 0 ) override;
    };
}  // namespace Ogre

#endif
