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

#ifndef _OgreMetalTextureGpuManager_H_
#define _OgreMetalTextureGpuManager_H_

#include "OgreMetalPrerequisites.h"

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
    class _OgreMetalExport MetalTextureGpuManager final : public TextureGpuManager
    {
    protected:
        /// 4x4 texture for when we have nothing to display.
        id<MTLTexture> mBlankTexture[TextureTypes::Type3D + 1u];

        MetalDevice *mDevice;

        TextureGpu     *createTextureImpl( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                           IdString name, uint32 textureFlags,
                                           TextureTypes::TextureTypes initialType ) override;
        StagingTexture *createStagingTextureImpl( uint32 width, uint32 height, uint32 depth,
                                                  uint32 slices, PixelFormatGpu pixelFormat ) override;
        void            destroyStagingTextureImpl( StagingTexture *stagingTexture ) override;

        AsyncTextureTicket *createAsyncTextureTicketImpl( uint32 width, uint32 height,
                                                          uint32                     depthOrSlices,
                                                          TextureTypes::TextureTypes textureType,
                                                          PixelFormatGpu pixelFormatFamily ) override;

    public:
        MetalTextureGpuManager( VaoManager *vaoManager, RenderSystem *renderSystem,
                                MetalDevice *device );
        ~MetalTextureGpuManager() override;

        /** Creates a special MetalTextureGpuWindow pointer, to be used by Ogre::Window.
            The pointer can be freed by a regular OGRE_DELETE. We do not track this pointer.
            If caller doesn't delete it, it will leak.
        */
        TextureGpu *createTextureGpuWindow( MetalWindow *window );
        TextureGpu *createWindowDepthBuffer( const bool bMemoryLess );

        id<MTLTexture> getBlankTextureMetalName( TextureTypes::TextureTypes textureType ) const;

        bool checkSupport( PixelFormatGpu format, TextureTypes::TextureTypes textureType,
                           uint32 textureFlags ) const override;

        MetalDevice *getDevice() const { return mDevice; }
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
