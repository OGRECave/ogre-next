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

#include "Vao/OgreD3D11DynamicBuffer.h"

#include "OgreD3D11Device.h"
#include "OgreException.h"
#include "Vao/OgreD3D11StagingBuffer.h"

namespace Ogre
{
    D3D11DynamicBuffer::D3D11DynamicBuffer( ID3D11Buffer *vboName, size_t vboSize,
                                            D3D11Device &device ) :
        mVboName( vboName ),
        mVboSize( vboSize ),
        mMappedPtr( 0 ),
        mDevice( device )
    {
    }
    //-----------------------------------------------------------------------------------
    D3D11DynamicBuffer::~D3D11DynamicBuffer() {}
    //-----------------------------------------------------------------------------------
    void D3D11DynamicBuffer::notifyDeviceLost( D3D11Device *device ) { mVboName.Reset(); }
    //-----------------------------------------------------------------------------------
    void D3D11DynamicBuffer::notifyDeviceRestored( D3D11Device *device, unsigned pass ) {}
    //-----------------------------------------------------------------------------------
    size_t D3D11DynamicBuffer::addMappedRange( size_t start, size_t count )
    {
        size_t ticket;

        if( !mFreeRanges.empty() )
        {
            ticket = mFreeRanges.back();
            mMappedRanges[ticket] = MappedRange( start, count );
            mFreeRanges.pop_back();
        }
        else
        {
            ticket = mMappedRanges.size();
            mMappedRanges.push_back( MappedRange( start, count ) );
        }

        return ticket;
    }
    //-----------------------------------------------------------------------------------
    void *RESTRICT_ALIAS_RETURN D3D11DynamicBuffer::map( size_t start, size_t count, size_t &outTicket )
    {
        assert( start <= mVboSize && start + count <= mVboSize );

        if( mMappedRanges.size() == mFreeRanges.size() )
        {
            D3D11_MAPPED_SUBRESOURCE mappedSubres;
            HRESULT hr = mDevice.GetImmediateContext()->Map(
                mVboName.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedSubres );
            if( FAILED( hr ) || mDevice.isError() )
            {
                String msg = mDevice.getErrorDescription( hr );
                OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr, "Error calling Map: " + msg,
                                "D3D11DynamicBuffer::map" );
            }
            mMappedPtr = mappedSubres.pData;
        }

        outTicket = addMappedRange( start, count );

        return static_cast<uint8 *>( mMappedPtr ) + start;
    }
    //-----------------------------------------------------------------------------------
    void D3D11DynamicBuffer::unmap( size_t ticket )
    {
        assert( ticket < mMappedRanges.size() && "Invalid unmap ticket!" );
        assert( mMappedRanges.size() != mFreeRanges.size() &&
                "Unmapping an already unmapped buffer! Did you call unmap with the same ticket twice?" );

        mFreeRanges.push_back( ticket );

        if( mMappedRanges.size() == mFreeRanges.size() )
        {
            mDevice.GetImmediateContext()->Unmap( mVboName.Get(), 0 );
            mMappedPtr = 0;
        }
    }
}  // namespace Ogre
