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

#include "Vao/OgreNULLUavBufferPacked.h"

#include "Vao/OgreNULLBufferInterface.h"
#include "Vao/OgreNULLTexBufferPacked.h"

namespace Ogre
{
    NULLUavBufferPacked::NULLUavBufferPacked( size_t internalBufStartBytes, size_t numElements,
                                              uint32 bytesPerElement, uint32 bindFlags,
                                              void *initialData, bool keepAsShadow,
                                              VaoManager *vaoManager,
                                              NULLBufferInterface *bufferInterface ) :
        UavBufferPacked( internalBufStartBytes, numElements, bytesPerElement, bindFlags, initialData,
                         keepAsShadow, vaoManager, bufferInterface )
    {
    }
    //-----------------------------------------------------------------------------------
    NULLUavBufferPacked::~NULLUavBufferPacked() {}
    //-----------------------------------------------------------------------------------
    TexBufferPacked *NULLUavBufferPacked::getAsTexBufferImpl( PixelFormatGpu pixelFormat )
    {
        OGRE_ASSERT_HIGH( dynamic_cast<NULLBufferInterface *>( mBufferInterface ) );

        NULLBufferInterface *bufferInterface = static_cast<NULLBufferInterface *>( mBufferInterface );

        TexBufferPacked *retVal = OGRE_NEW NULLTexBufferPacked(
            mInternalBufferStart * mBytesPerElement, mNumElements, mBytesPerElement, 0, mBufferType,
            (void *)0, false, (VaoManager *)0, bufferInterface, pixelFormat );
        // We were overridden by the BufferPacked we just created. Restore this back!
        bufferInterface->_notifyBuffer( this );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    ReadOnlyBufferPacked *NULLUavBufferPacked::getAsReadOnlyBufferImpl()
    {
        OGRE_ASSERT_HIGH( dynamic_cast<NULLBufferInterface *>( mBufferInterface ) );

        NULLBufferInterface *bufferInterface = static_cast<NULLBufferInterface *>( mBufferInterface );

        ReadOnlyBufferPacked *retVal = OGRE_NEW NULLReadOnlyBufferPacked(
            mInternalBufferStart * mBytesPerElement, mNumElements, mBytesPerElement, 0, mBufferType,
            (void *)0, false, (VaoManager *)0, bufferInterface, PFG_NULL );
        // We were overridden by the BufferPacked we just created. Restore this back!
        bufferInterface->_notifyBuffer( this );

        return retVal;
    }
}  // namespace Ogre
