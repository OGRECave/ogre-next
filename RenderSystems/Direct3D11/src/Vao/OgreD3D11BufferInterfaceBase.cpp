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

#include "Vao/OgreD3D11BufferInterfaceBase.h"

#include "OgreD3D11Device.h"
#include "OgreException.h"
#include "Vao/OgreD3D11VaoManager.h"

namespace Ogre
{
    D3D11BufferInterfaceBase::D3D11BufferInterfaceBase( size_t vboPoolIdx, ID3D11Buffer *d3dBuffer ) :
        mVboPoolIdx( vboPoolIdx ),
        mVboName( d3dBuffer ),
        mMappedPtr( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    D3D11BufferInterfaceBase::~D3D11BufferInterfaceBase() {}
    //-----------------------------------------------------------------------------------
    void D3D11BufferInterfaceBase::notifyDeviceLost( D3D11Device *device ) { mVboName.Reset(); }
    //-----------------------------------------------------------------------------------
    void D3D11BufferInterfaceBase::notifyDeviceRestored( D3D11Device *device, unsigned pass ) {}
    //-----------------------------------------------------------------------------------
    void D3D11BufferInterfaceBase::splicedCopy( size_t dstOffsetBytes, size_t srcOffsetBytes,
                                                size_t sizeBytes, size_t alignment,
                                                ID3D11Buffer *dstBuffer, ID3D11Buffer *srcBuffer,
                                                ID3D11DeviceContextN *context )
    {
        OGRE_ASSERT_MEDIUM( ( dstOffsetBytes % alignment ) == 0u );

        D3D11VaoManager *vaoManager = static_cast<D3D11VaoManager *>( mBuffer->mVaoManager );
        ID3D11Buffer *helperBuffer = vaoManager->getSplicingHelperBuffer();

        D3D11_BOX srcBox;
        srcBox.top = 0u;
        srcBox.bottom = 1u;
        srcBox.front = 0u;
        srcBox.back = 1u;

        // Only the tail end needs to be spliced
        size_t dstBlockStart = alignToPreviousMult( dstOffsetBytes + sizeBytes, alignment );
        size_t dstBlockEnd = alignToNextMultiple( dstOffsetBytes + sizeBytes, alignment );

        // Copy DST's block current contents
        srcBox.left = static_cast<UINT>( dstBlockStart );
        srcBox.right = static_cast<UINT>( dstBlockEnd );
        context->CopySubresourceRegion( helperBuffer, 0, 0, 0, 0, dstBuffer, 0, &srcBox );

        // Merge in SRC's contents
        srcBox.left = static_cast<UINT>( srcOffsetBytes + alignToPreviousMult( sizeBytes, alignment ) );
        srcBox.right = srcBox.left + static_cast<UINT>( sizeBytes % alignment );
        context->CopySubresourceRegion( helperBuffer, 0, 0, 0, 0, srcBuffer, 0, &srcBox );

        // Copy spliced result into DST block
        srcBox.left = static_cast<UINT>( 0u );
        srcBox.right = static_cast<UINT>( alignment );
        context->CopySubresourceRegion( dstBuffer, 0, static_cast<UINT>( dstBlockStart ), 0, 0,
                                        helperBuffer, 0, &srcBox );
    }
    //-----------------------------------------------------------------------------------
    void D3D11BufferInterfaceBase::copyTo( BufferInterface *dstBuffer, size_t dstOffsetBytes,
                                           size_t srcOffsetBytes, size_t sizeBytes )
    {
        D3D11VaoManager *vaoManager = static_cast<D3D11VaoManager *>( mBuffer->mVaoManager );
        ID3D11DeviceContextN *context = vaoManager->getDevice().GetImmediateContext();

        OGRE_ASSERT_HIGH( dynamic_cast<D3D11BufferInterfaceBase *>( dstBuffer ) );
        D3D11BufferInterfaceBase *dstBufferD3d = static_cast<D3D11BufferInterfaceBase *>( dstBuffer );

        BufferPacked *dstBufferPacked = dstBuffer->getBufferPacked();

        D3D11_BOX srcBox;
        srcBox.top = 0u;
        srcBox.bottom = 1u;
        srcBox.front = 0u;
        srcBox.back = 1u;

        if( ( dstBufferPacked->getBufferPackedType() == BP_TYPE_UAV ||
              dstBufferPacked->getBufferPackedType() == BP_TYPE_TEX ||
              dstBufferPacked->getBufferPackedType() == BP_TYPE_READONLY ) &&
            sizeBytes % dstBufferPacked->getBytesPerElement() )
        {
            // D3D11 wants the length of copies to/from structured buffers to be multiples of the stride.
            // We have to perform splicing with a temporary helper buffer to deal with the issue.
            const size_t alignment = dstBufferPacked->getBytesPerElement();

            if( dstOffsetBytes % alignment )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Copying to a destination offset that is not "
                             "multiple of dst->getBytesPerElement()",
                             "D3D11BufferInterfaceBase::copyTo" );
            }

            if( sizeBytes >= dstBufferPacked->getBytesPerElement() )
            {
                // Copy range [srcOffsetBytes; srcOffsetBytes + sizeBytes - alignment % sizeBytes)
                // which is allowed by D3D11
                srcBox.left = static_cast<UINT>( srcOffsetBytes );
                srcBox.right =
                    static_cast<UINT>( srcOffsetBytes + alignToPreviousMult( sizeBytes, alignment ) );
                context->CopySubresourceRegion( dstBufferD3d->mVboName.Get(), 0,
                                                static_cast<UINT>( dstOffsetBytes ), 0, 0,
                                                this->mVboName.Get(), 0, &srcBox );
            }

            // Now deal with the last few bytes (which is up to 'alignment - 1u' bytes)
            splicedCopy( dstOffsetBytes, srcOffsetBytes, sizeBytes, alignment,
                         dstBufferD3d->mVboName.Get(), this->mVboName.Get(), context );
        }
        else
        {
            srcBox.left = static_cast<UINT>( srcOffsetBytes );
            srcBox.right = static_cast<UINT>( srcOffsetBytes + sizeBytes );
            context->CopySubresourceRegion( dstBufferD3d->mVboName.Get(), 0,
                                            static_cast<UINT>( dstOffsetBytes ), 0, 0,
                                            this->mVboName.Get(), 0, &srcBox );
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11BufferInterfaceBase::_ensureDelayedImmutableBuffersAreReady()
    {
        if( !mVboName )
        {
            D3D11VaoManager *vaoManager = static_cast<D3D11VaoManager *>( mBuffer->mVaoManager );
            vaoManager->_forceCreateDelayedImmutableBuffers();
        }
    }
}  // namespace Ogre
