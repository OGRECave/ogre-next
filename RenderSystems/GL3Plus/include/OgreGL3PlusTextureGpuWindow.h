/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2017 Torus Knot Software Ltd

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

#ifndef _OgreGL3PlusTextureGpuWindow_H_
#define _OgreGL3PlusTextureGpuWindow_H_

#include "OgreGL3PlusTextureGpu.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class _OgreGL3PlusExport GL3PlusTextureGpuWindow final : public GL3PlusTextureGpuRenderTarget
    {
        GL3PlusContext *mContext;
        Window         *mWindow;

        void createInternalResourcesImpl() override;
        void destroyInternalResourcesImpl() override;

    public:
        GL3PlusTextureGpuWindow( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                 VaoManager *vaoManager, IdString name, uint32 textureFlags,
                                 TextureTypes::TextureTypes initialType,
                                 TextureGpuManager *textureManager, GL3PlusContext *context,
                                 Window *window );
        ~GL3PlusTextureGpuWindow() override;

        void setTextureType( TextureTypes::TextureTypes textureType ) override;

        void getSubsampleLocations( vector<Vector2>::type locations ) override;

        void notifyDataIsReady() override;
        bool _isDataReadyImpl() const override;

        void swapBuffers() override;

        void getCustomAttribute( IdString name, void *pData ) override;

        bool isOpenGLRenderWindow() const override;

        void _setToDisplayDummyTexture() override;
        void _notifyTextureSlotChanged( const TexturePool *newPool, uint16 slice ) override;
    };

    class _OgreGL3PlusExport GL3PlusTextureGpuHeadlessWindow final : public GL3PlusTextureGpuRenderTarget
    {
        GL3PlusContext *mContext;
        Window         *mWindow;

    public:
        GL3PlusTextureGpuHeadlessWindow( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                         VaoManager *vaoManager, IdString name, uint32 textureFlags,
                                         TextureTypes::TextureTypes initialType,
                                         TextureGpuManager *textureManager, GL3PlusContext *context,
                                         Window *window );
        ~GL3PlusTextureGpuHeadlessWindow() override;

        void setTextureType( TextureTypes::TextureTypes textureType ) override;

        void swapBuffers() override;

        void getCustomAttribute( IdString name, void *pData ) override;

        bool isOpenGLRenderWindow() const override;

        void _notifyTextureSlotChanged( const TexturePool *newPool, uint16 slice ) override;
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
