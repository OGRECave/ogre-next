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

#include "OgreVulkanTextureGpuManager.h"

#include "OgreVulkanAsyncTextureTicket.h"
#include "OgreVulkanStagingTexture.h"
#include "OgreVulkanTextureGpu.h"
#include "OgreVulkanTextureGpuWindow.h"

#include "Vao/OgreVulkanVaoManager.h"

#include "OgrePixelFormatGpuUtils.h"
#include "OgreVector2.h"

#include "OgreException.h"

namespace Ogre
{
    VulkanTextureGpuManager::VulkanTextureGpuManager( VaoManager *vaoManager, RenderSystem *renderSystem,
                                                      VulkanDevice *device ) :
        TextureGpuManager( vaoManager, renderSystem ),
        mDevice( device )
    {
    }
    //-----------------------------------------------------------------------------------
    VulkanTextureGpuManager::~VulkanTextureGpuManager() { destroyAll(); }
    //-----------------------------------------------------------------------------------
    TextureGpu *VulkanTextureGpuManager::createTextureGpuWindow( VulkanWindow *window )
    {
        return OGRE_NEW VulkanTextureGpuWindow( GpuPageOutStrategy::Discard, mVaoManager,
                                                "RenderWindow",                      //
                                                TextureFlags::NotTexture |           //
                                                    TextureFlags::RenderToTexture |  //
                                                    TextureFlags::RenderWindowSpecific,
                                                TextureTypes::Type2D, this, window );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *VulkanTextureGpuManager::createWindowDepthBuffer( void )
    {
        return OGRE_NEW VulkanTextureGpuRenderTarget( GpuPageOutStrategy::Discard, mVaoManager,
                                                      "RenderWindow DepthBuffer",          //
                                                      TextureFlags::NotTexture |           //
                                                          TextureFlags::RenderToTexture |  //
                                                          TextureFlags::RenderWindowSpecific,
                                                      TextureTypes::Type2D, this );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *VulkanTextureGpuManager::createTextureImpl(
        GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy, IdString name, uint32 textureFlags,
        TextureTypes::TextureTypes initialType )
    {
        VulkanTextureGpu *retVal = 0;
        if( textureFlags & TextureFlags::RenderToTexture )
        {
            retVal = OGRE_NEW VulkanTextureGpuRenderTarget( pageOutStrategy, mVaoManager, name,
                                                            textureFlags, initialType, this );
        }
        else
        {
            retVal = OGRE_NEW VulkanTextureGpu( pageOutStrategy, mVaoManager, name, textureFlags,
                                                initialType, this );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    StagingTexture *VulkanTextureGpuManager::createStagingTextureImpl( uint32 width, uint32 height,
                                                                       uint32 depth, uint32 slices,
                                                                       PixelFormatGpu pixelFormat )
    {
        const uint32 rowAlignment = 4u;
        const size_t sizeBytes =
            PixelFormatGpuUtils::getSizeBytes( width, height, depth, slices, pixelFormat, rowAlignment );

        VulkanVaoManager *vaoManager = static_cast<VulkanVaoManager *>( mVaoManager );
        VulkanStagingTexture *retVal =
            vaoManager->createStagingTexture( PixelFormatGpuUtils::getFamily( pixelFormat ), sizeBytes );
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanTextureGpuManager::destroyStagingTextureImpl( StagingTexture *stagingTexture )
    {
        // Do nothing, caller will delete stagingTexture.
    }
    //-----------------------------------------------------------------------------------
    AsyncTextureTicket *VulkanTextureGpuManager::createAsyncTextureTicketImpl(
        uint32 width, uint32 height, uint32 depthOrSlices, TextureTypes::TextureTypes textureType,
        PixelFormatGpu pixelFormatFamily )
    {
        return OGRE_NEW VulkanAsyncTextureTicket( width, height, depthOrSlices, textureType,
                                                  pixelFormatFamily );
    }
    //-----------------------------------------------------------------------------------
    VkImage VulkanTextureGpuManager::getBlankTextureVulkanName(
        TextureTypes::TextureTypes textureType ) const
    {
        return mBlankTexture[textureType];
    }
    //-----------------------------------------------------------------------------------
    VkImageView VulkanTextureGpuManager::getBlankTextureViewVulkanName(
        TextureTypes::TextureTypes textureType ) const
    {
        return mBlankTextureView[textureType];
    }
}  // namespace Ogre
