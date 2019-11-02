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

#include "OgreVulkanTextureGpu.h"

#include "OgrePixelFormatGpuUtils.h"

#include "OgreException.h"
#include "OgreVector2.h"

namespace Ogre
{
    VulkanTextureGpu::VulkanTextureGpu( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                        VaoManager *vaoManager, IdString name, uint32 textureFlags,
                                        TextureTypes::TextureTypes initialType,
                                        TextureGpuManager *textureManager ) :
        TextureGpu( pageOutStrategy, vaoManager, name, textureFlags, initialType, textureManager ),
        mFinalTextureName( 0 ),
        mMsaaFramebufferName( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    VulkanTextureGpu::~VulkanTextureGpu() {}
    //-----------------------------------------------------------------------------------
    void VulkanTextureGpu::createInternalResourcesImpl( void ) {}
    //-----------------------------------------------------------------------------------
    void VulkanTextureGpu::destroyInternalResourcesImpl( void ) {}
    //-----------------------------------------------------------------------------------
    VkImageSubresourceRange VulkanTextureGpu::getFullSubresourceRange( void ) const
    {
        VkImageSubresourceRange retVal;
        const uint32 flags = PixelFormatGpuUtils::getFlags( mPixelFormat );
        retVal.aspectMask = 0u;
        if( flags & PixelFormatGpuUtils::PFF_DEPTH )
            retVal.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
        if( flags & PixelFormatGpuUtils::PFF_STENCIL )
            retVal.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        if( !( flags & ( PixelFormatGpuUtils::PFF_DEPTH | PixelFormatGpuUtils::PFF_STENCIL ) ) )
            retVal.aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
        retVal.baseMipLevel = 0u;
        retVal.levelCount = VK_REMAINING_MIP_LEVELS;
        retVal.baseArrayLayer = 0u;
        retVal.layerCount = VK_REMAINING_ARRAY_LAYERS;
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanTextureGpu::getSubsampleLocations( vector<Vector2>::type locations )
    {
        uint8 msaaCount = mMsaa;
        locations.reserve( msaaCount );
        for( size_t i = 0; i < msaaCount; ++i )
            locations.push_back( Vector2( 0, 0 ) );
    }
    //-----------------------------------------------------------------------------------
    void VulkanTextureGpu::notifyDataIsReady( void ) {}
    //-----------------------------------------------------------------------------------
    void VulkanTextureGpu::_autogenerateMipmaps( void ) {}
    //-----------------------------------------------------------------------------------
    void VulkanTextureGpu::_setToDisplayDummyTexture( void ) {}
    //-----------------------------------------------------------------------------------
    bool VulkanTextureGpu::_isDataReadyImpl( void ) const { return true; }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    VulkanTextureGpuRenderTarget::VulkanTextureGpuRenderTarget(
        GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy, VaoManager *vaoManager, IdString name,
        uint32 textureFlags, TextureTypes::TextureTypes initialType,
        TextureGpuManager *textureManager ) :
        VulkanTextureGpu( pageOutStrategy, vaoManager, name, textureFlags, initialType, textureManager ),
        mDepthBufferPoolId( 1u ),
        mPreferDepthTexture( false ),
        mDesiredDepthBufferFormat( PFG_UNKNOWN )
    {
    }
    //-----------------------------------------------------------------------------------
    void VulkanTextureGpuRenderTarget::_setDepthBufferDefaults( uint16 depthBufferPoolId,
                                                                bool preferDepthTexture,
                                                                PixelFormatGpu desiredDepthBufferFormat )
    {
        assert( isRenderToTexture() );
        mDepthBufferPoolId = depthBufferPoolId;
        mPreferDepthTexture = preferDepthTexture;
        mDesiredDepthBufferFormat = desiredDepthBufferFormat;
    }
    //-----------------------------------------------------------------------------------
    uint16 VulkanTextureGpuRenderTarget::getDepthBufferPoolId( void ) const
    {
        return mDepthBufferPoolId;
    }
    //-----------------------------------------------------------------------------------
    bool VulkanTextureGpuRenderTarget::getPreferDepthTexture( void ) const
    {
        return mPreferDepthTexture;
    }
    //-----------------------------------------------------------------------------------
    PixelFormatGpu VulkanTextureGpuRenderTarget::getDesiredDepthBufferFormat( void ) const
    {
        return mDesiredDepthBufferFormat;
    }
}  // namespace Ogre
