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

#ifndef _Ogre_MetalUavBufferPacked_H_
#define _Ogre_MetalUavBufferPacked_H_

#include "OgreMetalPrerequisites.h"
#include "Vao/OgreUavBufferPacked.h"

namespace Ogre
{
    class MetalBufferInterface;

    class _OgreMetalExport MetalUavBufferPacked final : public UavBufferPacked
    {
        MetalDevice *mDevice;

    protected:
        TexBufferPacked      *getAsTexBufferImpl( PixelFormatGpu pixelFormat ) override;
        ReadOnlyBufferPacked *getAsReadOnlyBufferImpl() override;

    public:
        MetalUavBufferPacked( size_t internalBufStartBytes, size_t numElements, uint32 bytesPerElement,
                              uint32 bindFlags, void *initialData, bool keepAsShadow,
                              VaoManager *vaoManager, MetalBufferInterface *bufferInterface,
                              MetalDevice *device );
        ~MetalUavBufferPacked() override;

        void bindBufferAllRenderStages( uint16 slot, size_t offset = 0 );
        void bindBufferVS( uint16 slot, size_t offset = 0, size_t sizeBytes = 0 );
        void bindBufferPS( uint16 slot, size_t offset = 0, size_t sizeBytes = 0 );
        // void bindBufferGS( uint16 slot, size_t offset=0, size_t sizeBytes=0 ) override {}
        // void bindBufferDS( uint16 slot, size_t offset=0, size_t sizeBytes=0 ) override {}
        // void bindBufferHS( uint16 slot, size_t offset=0, size_t sizeBytes=0 ) override {}
        void bindBufferCS( uint16 slot, size_t offset = 0, size_t sizeBytes = 0 ) override;

        void bindBufferForDescriptor( __unsafe_unretained id<MTLBuffer> *buffers, NSUInteger *offsets,
                                      size_t offset );
    };
}

#endif
