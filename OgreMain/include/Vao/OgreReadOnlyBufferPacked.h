/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-present Torus Knot Software Ltd

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

#ifndef _Ogre_ReadOnlyBufferPacked_H_
#define _Ogre_ReadOnlyBufferPacked_H_

#include "Vao/OgreTexBufferPacked.h"

#include "OgrePixelFormatGpu.h"

namespace Ogre
{
    /** Represents the best way to access read-only data.
        But how it is implemented depends largely on which HW/API it is running on:
            - Buffer<> aka texture buffer in D3D11 on D3D10+ HW
            - samplerBuffer aka texture buffer in GL3 on GL3/D3D10 HW
            - SSBO aka UAV buffer in GL4 on GL4/D3D11 HW
            - SSBO aka UAV buffer in Vulkan
            - Metal doesn't matter, there are no differences between TexBuffer & UavBuffer,
              however we consume the slots reserved for tex buffers.

        In short, it either behaves as a TexBufferPacked or as an UavBufferPacked (but read only)
        depending on HW and API being used.
     */
    class _OgreExport ReadOnlyBufferPacked : public TexBufferPacked
    {
    public:
        ReadOnlyBufferPacked( size_t internalBufferStartBytes, size_t numElements,
                              uint32 bytesPerElement, uint32 numElementsPadding, BufferType bufferType,
                              void *initialData, bool keepAsShadow, VaoManager *vaoManager,
                              BufferInterface *bufferInterface, PixelFormatGpu pf ) :
            TexBufferPacked( internalBufferStartBytes, numElements, bytesPerElement, numElementsPadding,
                             bufferType, initialData, keepAsShadow, vaoManager, bufferInterface, pf )
        {
        }

        BufferPackedTypes getBufferPackedType() const override { return BP_TYPE_READONLY; }
    };
}  // namespace Ogre

#endif
