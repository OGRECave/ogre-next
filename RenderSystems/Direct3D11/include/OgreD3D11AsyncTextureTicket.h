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

#ifndef _OgreD3D11AsyncTextureTicket_H_
#define _OgreD3D11AsyncTextureTicket_H_

#include "OgreD3D11Prerequisites.h"

#include "OgreAsyncTextureTicket.h"
#include "OgreTextureBox.h"

namespace Ogre
{
    /** See AsyncTextureTicket
     */
    class _OgreD3D11Export D3D11AsyncTextureTicket final : public AsyncTextureTicket
    {
    protected:
        ComPtr<ID3D11Resource> mStagingTexture;

        uint32              mDownloadFrame;
        ComPtr<ID3D11Query> mAccurateFence;
        D3D11VaoManager    *mVaoManager;
        uint32              mMappedSlice;
        bool                mIsArray2DTexture;

        TextureBox mapImpl( uint32 slice ) override;
        void       unmapImpl() override;

        bool canMapMoreThanOneSlice() const override;

        void waitForDownloadToFinish();

        void downloadFromGpu( TextureGpu *textureSrc, uint8 mipLevel, bool accurateTracking,
                              TextureBox *srcBox = 0 ) override;

    public:
        D3D11AsyncTextureTicket( uint32 width, uint32 height, uint32 depthOrSlices,
                                 TextureTypes::TextureTypes textureType,
                                 PixelFormatGpu pixelFormatFamily, D3D11VaoManager *vaoManager );
        ~D3D11AsyncTextureTicket() override;

        bool queryIsTransferDone() override;
    };
}  // namespace Ogre

#endif
