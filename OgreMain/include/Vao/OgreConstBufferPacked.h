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

#ifndef _Ogre_ConstBufferPacked_H_
#define _Ogre_ConstBufferPacked_H_

#include "Vao/OgreBufferPacked.h"

namespace Ogre
{
    /** Represents constant buffers (also known as Uniform Buffers in GL)
     */
    class _OgreExport ConstBufferPacked : public BufferPacked
    {
    public:
        ConstBufferPacked( size_t internalBufferStartBytes, size_t numElements, uint32 bytesPerElement,
                           uint32 numElementsPadding, BufferType bufferType, void *initialData,
                           bool keepAsShadow, VaoManager *vaoManager,
                           BufferInterface *bufferInterface ) :
            BufferPacked( internalBufferStartBytes, numElements, bytesPerElement, numElementsPadding,
                          bufferType, initialData, keepAsShadow, vaoManager, bufferInterface )
        {
        }

        BufferPackedTypes getBufferPackedType() const override { return BP_TYPE_CONST; }

        /** Binds the constant buffer to the given slot in the
            Vertex/Pixel/Geometry/Hull/Domain/Compute Shader
        @remarks
            Not all RS API separate by shader stage. For best compatibility,
            don't assign two different buffers at the same slot for different
            stages (just leave the slot empty on the stages you don't use).
        @param slot
            The slot to asign this constant buffer. In D3D11 it's called 'slot'.
            In GLSL it's called it's called 'binding'
        */
        virtual void bindBufferVS( uint16 slot ) = 0;
        virtual void bindBufferPS( uint16 slot ) = 0;
        virtual void bindBufferGS( uint16 slot ) = 0;
        virtual void bindBufferHS( uint16 slot ) = 0;
        virtual void bindBufferDS( uint16 slot ) = 0;
        virtual void bindBufferCS( uint16 slot ) = 0;
    };
}  // namespace Ogre

#endif
