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

#include "Vao/OgreD3D11UavBufferPacked.h"

#include "OgreD3D11Mappings.h"
#include "OgreD3D11RenderSystem.h"
#include "Vao/OgreD3D11BufferInterface.h"
#include "Vao/OgreD3D11CompatBufferInterface.h"
#include "Vao/OgreD3D11ReadOnlyBufferPacked.h"
#include "Vao/OgreD3D11TexBufferPacked.h"
#include "Vao/OgreD3D11VaoManager.h"

namespace Ogre
{
    D3D11UavBufferPacked::D3D11UavBufferPacked( size_t internalBufStartBytes, size_t numElements,
                                                uint32 bytesPerElement, uint32 bindFlags,
                                                void *initialData, bool keepAsShadow,
                                                VaoManager *vaoManager, BufferInterface *bufferInterface,
                                                D3D11Device &device ) :
        UavBufferPacked( internalBufStartBytes, numElements, bytesPerElement, bindFlags, initialData,
                         keepAsShadow, vaoManager, bufferInterface ),
        mDevice( device ),
        mCurrentCacheCursor( 0 )
    {
        memset( mCachedResourceViews, 0, sizeof( mCachedResourceViews ) );
    }
    //-----------------------------------------------------------------------------------
    D3D11UavBufferPacked::~D3D11UavBufferPacked() {}
    //-----------------------------------------------------------------------------------
    void D3D11UavBufferPacked::notifyDeviceLost( D3D11Device *device )
    {
        for( unsigned cacheIdx = 0; cacheIdx < 16; ++cacheIdx )
        {
            mCachedResourceViews[cacheIdx].mResourceView.Reset();
            mCachedResourceViews[cacheIdx].mOffset = 0;
            mCachedResourceViews[cacheIdx].mSize = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11UavBufferPacked::notifyDeviceRestored( D3D11Device *device, unsigned pass ) {}
    //-----------------------------------------------------------------------------------
    TexBufferPacked *D3D11UavBufferPacked::getAsTexBufferImpl( PixelFormatGpu pixelFormat )
    {
        OGRE_ASSERT_HIGH( dynamic_cast<D3D11CompatBufferInterface *>( mBufferInterface ) );

        D3D11CompatBufferInterface *bufferInterface =
            static_cast<D3D11CompatBufferInterface *>( mBufferInterface );

        TexBufferPacked *retVal = OGRE_NEW D3D11TexBufferPacked(
            mInternalBufferStart * mBytesPerElement, mNumElements, mBytesPerElement, 0, mBufferType,
            (void *)0, false, (VaoManager *)0, bufferInterface, pixelFormat, true, mDevice );
        // We were overridden by the BufferPacked we just created. Restore this back!
        bufferInterface->_notifyBuffer( this );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    ReadOnlyBufferPacked *D3D11UavBufferPacked::getAsReadOnlyBufferImpl()
    {
        OGRE_ASSERT_HIGH( dynamic_cast<D3D11CompatBufferInterface *>( mBufferInterface ) );

        D3D11CompatBufferInterface *bufferInterface =
            static_cast<D3D11CompatBufferInterface *>( mBufferInterface );

        ReadOnlyBufferPacked *retVal = OGRE_NEW D3D11ReadOnlyBufferPacked(
            mInternalBufferStart * mBytesPerElement, mNumElements, mBytesPerElement, 0, mBufferType,
            (void *)0, false, (VaoManager *)0, bufferInterface, PFG_NULL, true, mDevice );
        // We were overridden by the BufferPacked we just created. Restore this back!
        bufferInterface->_notifyBuffer( this );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    ID3D11UnorderedAccessView *D3D11UavBufferPacked::createResourceView( int cacheIdx, uint32 offset,
                                                                         uint32 sizeBytes )
    {
        assert( cacheIdx < 16 );

        mCachedResourceViews[cacheIdx].mResourceView.Reset();

        mCachedResourceViews[cacheIdx].mOffset = uint32( mFinalBufferStart + offset );
        mCachedResourceViews[cacheIdx].mSize = sizeBytes;

        D3D11_UNORDERED_ACCESS_VIEW_DESC srDesc;

        // Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffer
        // Format must be DXGI_FORMAT_R32_TYPELESS, when creating Raw Unordered Access View
        srDesc.Format = DXGI_FORMAT_UNKNOWN;
        srDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        srDesc.Buffer.FirstElement = UINT( ( mFinalBufferStart + offset ) / mBytesPerElement );
        srDesc.Buffer.NumElements = sizeBytes / mBytesPerElement;
        srDesc.Buffer.Flags = 0;

        assert( dynamic_cast<D3D11CompatBufferInterface *>( mBufferInterface ) );
        D3D11CompatBufferInterface *bufferInterface =
            static_cast<D3D11CompatBufferInterface *>( mBufferInterface );
        ID3D11Buffer *vboName = bufferInterface->getVboName();

        mDevice.get()->CreateUnorderedAccessView(
            vboName, &srDesc, mCachedResourceViews[cacheIdx].mResourceView.ReleaseAndGetAddressOf() );
        mCurrentCacheCursor = ( cacheIdx + 1 ) % 16;

        return mCachedResourceViews[cacheIdx].mResourceView.Get();
    }
    //-----------------------------------------------------------------------------------
    ID3D11UnorderedAccessView *D3D11UavBufferPacked::_bindBufferCommon( size_t offset, size_t sizeBytes )
    {
        assert( offset <= getTotalSizeBytes() );
        assert( sizeBytes <= getTotalSizeBytes() );
        assert( ( offset + sizeBytes ) <= getTotalSizeBytes() );

        sizeBytes = !sizeBytes ? ( mNumElements * mBytesPerElement - offset ) : sizeBytes;

        ID3D11UnorderedAccessView *resourceView = 0;
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
    ComPtr<ID3D11UnorderedAccessView> D3D11UavBufferPacked::createUav(
        const DescriptorSetUav::BufferSlot &bufferSlot ) const
    {
        assert( bufferSlot.offset <= getTotalSizeBytes() );
        assert( bufferSlot.sizeBytes <= getTotalSizeBytes() );
        assert( ( bufferSlot.offset + bufferSlot.sizeBytes ) <= getTotalSizeBytes() );

        const size_t sizeBytes = !bufferSlot.sizeBytes
                                     ? ( mNumElements * mBytesPerElement - bufferSlot.offset )
                                     : bufferSlot.sizeBytes;

        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        memset( &uavDesc, 0, sizeof( uavDesc ) );

        // Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffer
        // Format must be DXGI_FORMAT_R32_TYPELESS, when creating Raw Unordered Access View
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement =
            UINT( ( mFinalBufferStart + bufferSlot.offset ) / mBytesPerElement );
        uavDesc.Buffer.NumElements = UINT( sizeBytes / mBytesPerElement );
        uavDesc.Buffer.Flags = 0;

        assert( dynamic_cast<D3D11CompatBufferInterface *>( mBufferInterface ) );
        D3D11CompatBufferInterface *bufferInterface =
            static_cast<D3D11CompatBufferInterface *>( mBufferInterface );
        ID3D11Buffer *vboName = bufferInterface->getVboName();

        ComPtr<ID3D11UnorderedAccessView> retVal;
        HRESULT hr = mDevice->CreateUnorderedAccessView( vboName, &uavDesc, retVal.GetAddressOf() );
        if( FAILED( hr ) )
        {
            String errorDescription = mDevice.getErrorDescription( hr );
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Failed to create UAV view on buffer."
                            "\nError Description: " +
                                errorDescription,
                            "D3D11UavBufferPacked::createUav" );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    //    void D3D11UavBufferPacked::bindBufferVS( uint16 slot, size_t offset, size_t sizeBytes )
    //    {
    //        ID3D11UnorderedAccessView *resourceView = _bindBufferCommon( offset, sizeBytes );
    //        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();
    //        deviceContext->VSSetShaderResources( slot, 1, &resourceView );
    //    }
    //    //-----------------------------------------------------------------------------------
    //    void D3D11UavBufferPacked::bindBufferPS( uint16 slot, size_t offset, size_t sizeBytes )
    //    {
    //        ID3D11UnorderedAccessView *resourceView = _bindBufferCommon( offset, sizeBytes );
    //        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();
    //        deviceContext->PSSetShaderResources( slot, 1, &resourceView );
    //    }
    //    //-----------------------------------------------------------------------------------
    //    void D3D11UavBufferPacked::bindBufferGS( uint16 slot, size_t offset, size_t sizeBytes )
    //    {
    //        ID3D11UnorderedAccessView *resourceView = _bindBufferCommon( offset, sizeBytes );
    //        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();
    //        deviceContext->GSSetShaderResources( slot, 1, &resourceView );
    //    }
    //    //-----------------------------------------------------------------------------------
    //    void D3D11UavBufferPacked::bindBufferHS( uint16 slot, size_t offset, size_t sizeBytes )
    //    {
    //        ID3D11UnorderedAccessView *resourceView = _bindBufferCommon( offset, sizeBytes );
    //        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();
    //        deviceContext->HSSetShaderResources( slot, 1, &resourceView );
    //    }
    //    //-----------------------------------------------------------------------------------
    //    void D3D11UavBufferPacked::bindBufferDS( uint16 slot, size_t offset, size_t sizeBytes )
    //    {
    //        ID3D11UnorderedAccessView *resourceView = _bindBufferCommon( offset, sizeBytes );
    //        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();
    //        deviceContext->DSSetShaderResources( slot, 1, &resourceView );
    //    }
    //-----------------------------------------------------------------------------------
    void D3D11UavBufferPacked::bindBufferCS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        ID3D11UnorderedAccessView *resourceView = _bindBufferCommon( offset, sizeBytes );
        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();
        deviceContext->CSSetUnorderedAccessViews( slot, 1, &resourceView, 0 );
    }
    //-----------------------------------------------------------------------------------
}  // namespace Ogre
