/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

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
#include "OgreD3D11HardwareBufferManager.h"

#include "OgreD3D11Device.h"
#include "OgreD3D11HardwareIndexBuffer.h"
#include "OgreD3D11HardwareVertexBuffer.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"

namespace Ogre
{
    namespace v1
    {
        //-----------------------------------------------------------------------
        D3D11HardwareBufferManagerBase::D3D11HardwareBufferManagerBase( D3D11Device &device ) :
            mlpD3DDevice( device )
        {
        }
        //-----------------------------------------------------------------------
        D3D11HardwareBufferManagerBase::~D3D11HardwareBufferManagerBase()
        {
            destroyAllDeclarations();
            destroyAllBindings();
        }
        //-----------------------------------------------------------------------
        HardwareVertexBufferSharedPtr D3D11HardwareBufferManagerBase::createVertexBuffer(
            size_t vertexSize, size_t numVerts, HardwareBuffer::Usage usage, bool useShadowBuffer )
        {
            assert( numVerts > 0 );
            D3D11HardwareVertexBuffer *vbuf = new D3D11HardwareVertexBuffer(
                this, vertexSize, numVerts, usage, mlpD3DDevice, false, useShadowBuffer, false );
            {
                OGRE_LOCK_MUTEX( mVertexBuffersMutex );
                mVertexBuffers.insert( vbuf );
            }
            return HardwareVertexBufferSharedPtr( vbuf );
        }
        //-----------------------------------------------------------------------
        HardwareVertexBufferSharedPtr D3D11HardwareBufferManagerBase::createStreamOutputVertexBuffer(
            size_t vertexSize, size_t numVerts, HardwareBuffer::Usage usage, bool useShadowBuffer )
        {
            assert( numVerts > 0 );
            D3D11HardwareVertexBuffer *vbuf = new D3D11HardwareVertexBuffer(
                this, vertexSize, numVerts, usage, mlpD3DDevice, false, useShadowBuffer, true );
            {
                OGRE_LOCK_MUTEX( mVertexBuffersMutex );
                mVertexBuffers.insert( vbuf );
            }
            return HardwareVertexBufferSharedPtr( vbuf );
        }
        //-----------------------------------------------------------------------
        HardwareIndexBufferSharedPtr D3D11HardwareBufferManagerBase::createIndexBuffer(
            HardwareIndexBuffer::IndexType itype, size_t numIndexes, HardwareBuffer::Usage usage,
            bool useShadowBuffer )
        {
            assert( numIndexes > 0 );
            D3D11HardwareIndexBuffer *idx = new D3D11HardwareIndexBuffer(
                this, itype, numIndexes, usage, mlpD3DDevice, false, useShadowBuffer );
            {
                OGRE_LOCK_MUTEX( mIndexBuffersMutex );
                mIndexBuffers.insert( idx );
            }
            return HardwareIndexBufferSharedPtr( idx );
        }
    }  // namespace v1
}  // namespace Ogre
