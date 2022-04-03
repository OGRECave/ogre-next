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

#ifndef _Ogre_D3D11BufferInterfaceBase_H_
#define _Ogre_D3D11BufferInterfaceBase_H_

#include "OgreD3D11Prerequisites.h"

#include "OgreD3D11DeviceResource.h"
#include "Vao/OgreBufferInterface.h"

namespace Ogre
{
    /** For D3D11, most (if not all) buffers, can be treated with the same code.
        Hence most equivalent functionality is encapsulated here.
    */
    class _OgreD3D11Export D3D11BufferInterfaceBase : public BufferInterface,
                                                      protected D3D11DeviceResource
    {
    protected:
        size_t               mVboPoolIdx;
        ComPtr<ID3D11Buffer> mVboName;
        void                *mMappedPtr;

        void splicedCopy( size_t dstOffsetBytes, size_t srcOffsetBytes, size_t sizeBytes,
                          size_t alignment, ID3D11Buffer *dstBuffer, ID3D11Buffer *srcBuffer,
                          ID3D11DeviceContextN *context );

        void notifyDeviceLost( D3D11Device *device ) override;
        void notifyDeviceRestored( D3D11Device *device, unsigned pass ) override;

    public:
        D3D11BufferInterfaceBase( size_t vboPoolIdx, ID3D11Buffer *d3dBuffer );
        ~D3D11BufferInterfaceBase() override;

        size_t        getVboPoolIndex() { return mVboPoolIdx; }
        ID3D11Buffer *getVboName() const { return mVboName.Get(); }

        void copyTo( BufferInterface *dstBuffer, size_t dstOffsetBytes, size_t srcOffsetBytes,
                     size_t sizeBytes ) override;

        void _ensureDelayedImmutableBuffersAreReady() override;
    };
}  // namespace Ogre

#endif
