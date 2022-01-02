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

#include "Vao/OgreD3D11ReadOnlyBufferPacked.h"

#include "OgreD3D11Mappings.h"
#include "OgreD3D11RenderSystem.h"
#include "OgrePixelFormatGpuUtils.h"
#include "Vao/OgreD3D11BufferInterface.h"
#include "Vao/OgreD3D11CompatBufferInterface.h"
#include "Vao/OgreD3D11VaoManager.h"

namespace Ogre
{
    D3D11ReadOnlyBufferPacked::D3D11ReadOnlyBufferPacked(
        size_t internalBufStartBytes, size_t numElements, uint32 bytesPerElement,
        uint32 numElementsPadding, BufferType bufferType, void *initialData, bool keepAsShadow,
        VaoManager *vaoManager, BufferInterface *bufferInterface, PixelFormatGpu pf, bool bIsStructured,
        D3D11Device &device ) :
        ReadOnlyBufferPacked( internalBufStartBytes, numElements, bytesPerElement, numElementsPadding,
                              bufferType, initialData, keepAsShadow, vaoManager, bufferInterface, pf ),
        mInternalFormat( DXGI_FORMAT_UNKNOWN ),
        mDevice( device ),
        mCurrentCacheCursor( 0 )
    {
        memset( mCachedResourceViews, 0, sizeof( mCachedResourceViews ) );

        if( !bIsStructured )
            mInternalFormat = D3D11Mappings::get( pf );
    }
    //-----------------------------------------------------------------------------------
    D3D11ReadOnlyBufferPacked::~D3D11ReadOnlyBufferPacked() {}
    //-----------------------------------------------------------------------------------
    void D3D11ReadOnlyBufferPacked::notifyDeviceLost( D3D11Device *device )
    {
        for( unsigned cacheIdx = 0; cacheIdx < 16; ++cacheIdx )
        {
            mCachedResourceViews[cacheIdx].mResourceView.Reset();
            mCachedResourceViews[cacheIdx].mOffset = 0;
            mCachedResourceViews[cacheIdx].mSize = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11ReadOnlyBufferPacked::notifyDeviceRestored( D3D11Device *device, unsigned pass ) {}
    //-----------------------------------------------------------------------------------
    bool D3D11ReadOnlyBufferPacked::isD3D11Structured() const
    {
        return mInternalFormat == DXGI_FORMAT_UNKNOWN;
    }
    //-----------------------------------------------------------------------------------
    ID3D11ShaderResourceView *D3D11ReadOnlyBufferPacked::createResourceView( int cacheIdx, uint32 offset,
                                                                             uint32 sizeBytes )
    {
        assert( cacheIdx < 16 );

        const uint32 formatSize = mBytesPerElement != 1u
                                      ? mBytesPerElement
                                      : PixelFormatGpuUtils::getBytesPerPixel( mPixelFormat );

        mCachedResourceViews[cacheIdx].mResourceView.Reset();

        mCachedResourceViews[cacheIdx].mOffset = static_cast<uint32>( mFinalBufferStart + offset );
        mCachedResourceViews[cacheIdx].mSize = sizeBytes;

        D3D11_SHADER_RESOURCE_VIEW_DESC srDesc;

        srDesc.Format = mInternalFormat;
        srDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srDesc.Buffer.FirstElement = static_cast<UINT>( ( mFinalBufferStart + offset ) / formatSize );
        srDesc.Buffer.NumElements = sizeBytes / formatSize;

        // D3D11RenderSystem *rs = static_cast<D3D11VaoManager*>(mVaoManager)->getD3D11RenderSystem();
        ID3D11Buffer *vboName = 0;

        D3D11BufferInterfaceBase *bufferInterface =
            static_cast<D3D11BufferInterfaceBase *>( mBufferInterface );
        vboName = bufferInterface->getVboName();

        mDevice.get()->CreateShaderResourceView(
            vboName, &srDesc, mCachedResourceViews[cacheIdx].mResourceView.ReleaseAndGetAddressOf() );

        mCurrentCacheCursor = ( cacheIdx + 1 ) % 16;

        return mCachedResourceViews[cacheIdx].mResourceView.Get();
    }
    //-----------------------------------------------------------------------------------
    ID3D11ShaderResourceView *D3D11ReadOnlyBufferPacked::bindBufferCommon( size_t offset,
                                                                           size_t sizeBytes )
    {
        assert( offset <= getTotalSizeBytes() );
        assert( sizeBytes <= getTotalSizeBytes() );
        assert( ( offset + sizeBytes ) <= getTotalSizeBytes() );

        sizeBytes = !sizeBytes ? ( mNumElements * mBytesPerElement - offset ) : sizeBytes;

        ID3D11ShaderResourceView *resourceView = 0;
        for( int i = 0; i < 16; ++i )
        {
            // Reuse resource views. Reuse res. views that are bigger than what's requested too.
            if( mFinalBufferStart + offset == mCachedResourceViews[i].mOffset &&
                sizeBytes <= mCachedResourceViews[i].mSize )
            {
                resourceView = mCachedResourceViews[i].mResourceView.Get();
                break;
            }
            else if( !mCachedResourceViews[i].mResourceView )
            {
                // We create in-order. If we hit here, the next ones are also null pointers.
                resourceView = createResourceView( i, (uint32)offset, (uint32)sizeBytes );
                break;
            }
        }

        if( !resourceView )
        {
            // If we hit here, the cache is full and couldn't find a match.
            resourceView = createResourceView( mCurrentCacheCursor, (uint32)offset, (uint32)sizeBytes );
        }

        return resourceView;
    }
    //-----------------------------------------------------------------------------------
    ComPtr<ID3D11ShaderResourceView> D3D11ReadOnlyBufferPacked::createSrv(
        const DescriptorSetTexture2::BufferSlot &bufferSlot ) const
    {
        assert( bufferSlot.offset <= getTotalSizeBytes() );
        assert( bufferSlot.sizeBytes <= getTotalSizeBytes() );
        assert( ( bufferSlot.offset + bufferSlot.sizeBytes ) <= getTotalSizeBytes() );

        const size_t formatSize = mBytesPerElement != 1u
                                      ? mBytesPerElement
                                      : PixelFormatGpuUtils::getBytesPerPixel( mPixelFormat );

        const size_t sizeBytes = !bufferSlot.sizeBytes
                                     ? ( mNumElements * mBytesPerElement - bufferSlot.offset )
                                     : bufferSlot.sizeBytes;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        memset( &srvDesc, 0, sizeof( srvDesc ) );

        srvDesc.Format = mInternalFormat;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.FirstElement =
            static_cast<UINT>( ( mFinalBufferStart + bufferSlot.offset ) / formatSize );
        srvDesc.Buffer.NumElements = static_cast<UINT>( sizeBytes / formatSize );

        D3D11BufferInterfaceBase *bufferInterface =
            static_cast<D3D11BufferInterfaceBase *>( mBufferInterface );
        ID3D11Buffer *vboName = bufferInterface->getVboName();

        ComPtr<ID3D11ShaderResourceView> retVal;
        HRESULT hr = mDevice->CreateShaderResourceView( vboName, &srvDesc, retVal.GetAddressOf() );

        if( FAILED( hr ) )
        {
            String errorDescription = mDevice.getErrorDescription( hr );
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Failed to create SRV view on buffer."
                            "\nError Description: " +
                                errorDescription,
                            "D3D11ReadOnlyBufferPacked::createSrv" );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void D3D11ReadOnlyBufferPacked::bindBufferVS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        ID3D11ShaderResourceView *resourceView = bindBufferCommon( offset, sizeBytes );
        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();
        deviceContext->VSSetShaderResources( slot, 1, &resourceView );
    }
    //-----------------------------------------------------------------------------------
    void D3D11ReadOnlyBufferPacked::bindBufferPS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        ID3D11ShaderResourceView *resourceView = bindBufferCommon( offset, sizeBytes );
        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();
        deviceContext->PSSetShaderResources( slot, 1, &resourceView );
    }
    //-----------------------------------------------------------------------------------
    void D3D11ReadOnlyBufferPacked::bindBufferGS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        ID3D11ShaderResourceView *resourceView = bindBufferCommon( offset, sizeBytes );
        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();
        deviceContext->GSSetShaderResources( slot, 1, &resourceView );
    }
    //-----------------------------------------------------------------------------------
    void D3D11ReadOnlyBufferPacked::bindBufferHS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        ID3D11ShaderResourceView *resourceView = bindBufferCommon( offset, sizeBytes );
        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();
        deviceContext->HSSetShaderResources( slot, 1, &resourceView );
    }
    //-----------------------------------------------------------------------------------
    void D3D11ReadOnlyBufferPacked::bindBufferDS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        ID3D11ShaderResourceView *resourceView = bindBufferCommon( offset, sizeBytes );
        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();
        deviceContext->DSSetShaderResources( slot, 1, &resourceView );
    }
    //-----------------------------------------------------------------------------------
    void D3D11ReadOnlyBufferPacked::bindBufferCS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        ID3D11ShaderResourceView *resourceView = bindBufferCommon( offset, sizeBytes );
        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();
        deviceContext->CSSetShaderResources( slot, 1, &resourceView );
    }
    //-----------------------------------------------------------------------------------
}  // namespace Ogre
