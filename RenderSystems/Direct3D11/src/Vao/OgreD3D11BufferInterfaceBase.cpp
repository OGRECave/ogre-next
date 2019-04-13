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

#include "Vao/OgreD3D11BufferInterfaceBase.h"
#include "Vao/OgreD3D11VaoManager.h"
#include "OgreD3D11Device.h"

namespace Ogre
{
    D3D11BufferInterfaceBase::D3D11BufferInterfaceBase( size_t vboPoolIdx, ID3D11Buffer *d3dBuffer ) :
        mVboPoolIdx( vboPoolIdx ),
        mVboName( d3dBuffer ),
        mMappedPtr( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    D3D11BufferInterfaceBase::~D3D11BufferInterfaceBase()
    {
    }
    //-----------------------------------------------------------------------------------
    void D3D11BufferInterfaceBase::copyTo( BufferInterface *dstBuffer, size_t dstOffsetBytes,
                                           size_t srcOffsetBytes, size_t sizeBytes )
    {
        D3D11VaoManager *vaoManager = static_cast<D3D11VaoManager*>( mBuffer->mVaoManager );
        ID3D11DeviceContextN *context = vaoManager->getDevice().GetImmediateContext();

        OGRE_ASSERT_HIGH( dynamic_cast<D3D11BufferInterfaceBase*>( dstBuffer ) );
        D3D11BufferInterfaceBase *dstBufferD3d = static_cast<D3D11BufferInterfaceBase*>( dstBuffer );

        D3D11_BOX srcBox;
        srcBox.left     = static_cast<UINT>( srcOffsetBytes );
        srcBox.right    = static_cast<UINT>( srcOffsetBytes + sizeBytes );
        srcBox.top      = 0u;
        srcBox.bottom   = 1u;
        srcBox.front    = 0u;
        srcBox.back     = 1u;
        context->CopySubresourceRegion( dstBufferD3d->mVboName, 0, dstOffsetBytes, 0, 0,
                                        this->mVboName, 0, &srcBox );
    }
}
