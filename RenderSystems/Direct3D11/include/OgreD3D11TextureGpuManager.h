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

#ifndef _OgreD3D11TextureGpuManager_H_
#define _OgreD3D11TextureGpuManager_H_

#include "OgreD3D11Prerequisites.h"
#include "OgreTextureGpuManager.h"

#include "OgreTextureGpu.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */
    class _OgreD3D11Export D3D11TextureGpuManager : public TextureGpuManager
    {
    protected:
        /// 4x4 texture for when we have nothing to display.
        ID3D11Resource              *mBlankTexture[TextureTypes::Type3D + 1u];
        ID3D11ShaderResourceView    *mDefaultSrv[TextureTypes::Type3D + 1u];

        D3D11Device &mDevice;

        virtual TextureGpu* createTextureImpl( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                               IdString name, uint32 textureFlags,
                                               TextureTypes::TextureTypes initialType );
        virtual StagingTexture* createStagingTextureImpl( uint32 width, uint32 height, uint32 depth,
                                                          uint32 slices, PixelFormatGpu pixelFormat );
        virtual void destroyStagingTextureImpl( StagingTexture *stagingTexture );

        virtual AsyncTextureTicket* createAsyncTextureTicketImpl (uint32 width, uint32 height,
                                                                  uint32 depthOrSlices,
                                                                  TextureTypes::TextureTypes textureType,
                                                                  PixelFormatGpu pixelFormatFamily );

    public:
        D3D11TextureGpuManager( VaoManager *vaoManager, RenderSystem *renderSystem,
                                D3D11Device &device );
        virtual ~D3D11TextureGpuManager();

        /** Creates a special D3D11TextureGpuWindow pointer, to be used by Ogre::Window.
            The pointer can be freed by a regular OGRE_DELETE. We do not track this pointer.
            If caller doesnt' delete it, it will leak.
        */
        TextureGpu* createTextureGpuWindow( ID3D11Texture2D *backbuffer, Window *window );
        TextureGpu* createWindowDepthBuffer(void);

        ID3D11Resource* getBlankTextureD3dName( TextureTypes::TextureTypes textureType ) const;
        ID3D11ShaderResourceView* getBlankTextureSrv( TextureTypes::TextureTypes textureType ) const;

        D3D11Device& getDevice(void)        { return mDevice; }
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
