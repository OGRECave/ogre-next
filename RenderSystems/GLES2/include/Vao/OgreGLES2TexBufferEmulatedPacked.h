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

#ifndef _Ogre_GLES2TexBufferEmulatedPacked_H_
#define _Ogre_GLES2TexBufferEmulatedPacked_H_

#include "OgreGLES2Prerequisites.h"
#include "Vao/OgreTexBufferPacked.h"

namespace Ogre
{
    class GLES2BufferInterface;

    class _OgreGLES2Export GLES2TexBufferEmulatedPacked : public TexBufferPacked
    {
        GLuint mTexName;
        GLenum mInternalFormat;

        GLenum mOriginFormat;
        GLenum mOriginDataType;
        size_t mMaxTexSize;
        size_t mInternalNumElemBytes;
        size_t mInternalNumElements;

        inline void bindBuffer( uint16 slot, size_t offset, size_t sizeBytes );

    public:
        GLES2TexBufferEmulatedPacked( size_t internalBufStartBytes, size_t numElements,
                                        uint32 bytesPerElement, uint32 numElementsPadding,
                                        BufferType bufferType,
                                        void *initialData, bool keepAsShadow,
                                        VaoManager *vaoManager, GLES2BufferInterface *bufferInterface,
                                        Ogre::PixelFormat pf );
        virtual ~GLES2TexBufferEmulatedPacked();

        virtual void bindBufferVS( uint16 slot, size_t offset=0, size_t sizeBytes=0 );
        virtual void bindBufferPS( uint16 slot, size_t offset=0, size_t sizeBytes=0 );
        virtual void bindBufferGS( uint16 slot, size_t offset=0, size_t sizeBytes=0 );
        virtual void bindBufferDS( uint16 slot, size_t offset=0, size_t sizeBytes=0 );
        virtual void bindBufferHS( uint16 slot, size_t offset=0, size_t sizeBytes=0 );
        virtual void bindBufferCS( uint16 slot, size_t offset=0, size_t sizeBytes=0 );
    };
}

#endif
