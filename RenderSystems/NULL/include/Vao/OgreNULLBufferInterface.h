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

#ifndef _Ogre_NULLBufferInterface_H_
#define _Ogre_NULLBufferInterface_H_

#include "OgreNULLPrerequisites.h"

#include "Vao/OgreBufferInterface.h"

namespace Ogre
{
    /** For GL3+, most (if not all) buffers, can be treated with the same code.
        Hence most equivalent functionality is encapsulated here.
    */
    class _OgreNULLExport NULLBufferInterface : public BufferInterface
    {
    protected:
        size_t mVboPoolIdx;
        void  *mMappedPtr;

        uint8 *mNullDataPtr;

        size_t advanceFrame( bool bAdvanceFrame );

    public:
        NULLBufferInterface( size_t vboPoolIdx );
        ~NULLBufferInterface() override;

        size_t getVboPoolIndex() { return mVboPoolIdx; }

        uint8 *getNullDataPtr() { return mNullDataPtr; }

        /// will null the data ptr so it wont be freed on destruction
        void nullDataPtr() { mNullDataPtr = 0; }

        /// Only use this function for the first upload
        void _firstUpload( const void *data, size_t elementStart, size_t elementCount );

        void *RESTRICT_ALIAS_RETURN map( size_t elementStart, size_t elementCount,
                                         MappingState prevMappingState,
                                         bool         advanceFrame = true ) override;

        void unmap( UnmapOptions unmapOption, size_t flushStartElem = 0,
                    size_t flushSizeElem = 0 ) override;
        void advanceFrame() override;
        void regressFrame() override;

        void _notifyBuffer( BufferPacked *buffer ) override;

        void copyTo( BufferInterface *dstBuffer, size_t dstOffsetBytes, size_t srcOffsetBytes,
                     size_t sizeBytes ) override;
    };
}  // namespace Ogre

#endif
