/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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

#ifndef _OgreMetalTextureGpuWindow_H_
#define _OgreMetalTextureGpuWindow_H_

#include "OgreMetalTextureGpu.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class _OgreMetalExport MetalTextureGpuWindow : public MetalTextureGpuRenderTarget
    {
        MetalWindow *mWindow;

        virtual void createInternalResourcesImpl(void);
        virtual void destroyInternalResourcesImpl(void);

    public:
        MetalTextureGpuWindow( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                               VaoManager *vaoManager, IdString name, uint32 textureFlags,
                               TextureTypes::TextureTypes initialType,
                               TextureGpuManager *textureManager,
                               MetalWindow *window );
        virtual ~MetalTextureGpuWindow();

        void nextDrawable(void);

        void _setBackbuffer( id<MTLTexture> backbuffer );
        void _setMsaaBackbuffer( id<MTLTexture> msaaTex );

        virtual void setTextureType( TextureTypes::TextureTypes textureType );

        virtual void notifyDataIsReady(void);
        virtual bool _isDataReadyImpl(void) const;

        virtual void swapBuffers(void);

        virtual void getCustomAttribute( IdString name, void *pData );

        virtual void _setToDisplayDummyTexture(void);
        virtual void _notifyTextureSlotChanged( const TexturePool *newPool, uint16 slice );
    };
}

#include "OgreHeaderSuffix.h"

#endif
