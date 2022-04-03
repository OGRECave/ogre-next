/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2016 Torus Knot Software Ltd

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

#ifndef _Ogre_MetalConstBufferPacked_H_
#define _Ogre_MetalConstBufferPacked_H_

#include "OgreMetalPrerequisites.h"
#include "Vao/OgreConstBufferPacked.h"

namespace Ogre
{
    class _OgreMetalExport MetalConstBufferPacked final : public ConstBufferPacked
    {
        MetalDevice *mDevice;

    public:
        MetalConstBufferPacked( size_t internalBufferStartBytes, size_t numElements,
                                uint32 bytesPerElement, uint32 numElementsPadding, BufferType bufferType,
                                void *initialData, bool keepAsShadow, VaoManager *vaoManager,
                                BufferInterface *bufferInterface, MetalDevice *device );
        ~MetalConstBufferPacked() override;

        void bindBufferVS( uint16 slot ) override;
        void bindBufferPS( uint16 slot ) override;
        void bindBufferGS( uint16 ) override {}
        void bindBufferHS( uint16 ) override {}
        void bindBufferDS( uint16 ) override {}
        void bindBufferCS( uint16 slot ) override;

        void bindBufferVS( uint16 slot, uint32 offsetBytes );
        void bindBufferPS( uint16 slot, uint32 offsetBytes );
        void bindBufferCS( uint16 slot, uint32 offsetBytes );
    };
}  // namespace Ogre

#endif
