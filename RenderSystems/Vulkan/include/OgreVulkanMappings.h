/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE-Next
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-present Torus Knot Software Ltd

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

#ifndef _OgreVulkanMappings_H_
#define _OgreVulkanMappings_H_

#include <SPIRV-Reflect/include/spirv/unified1/spirv.h>

#include "OgreVulkanPrerequisites.h"

#include "OgreBlendMode.h"
#include "OgreHlmsPso.h"
#include "OgreHlmsSamplerblock.h"
#include "OgrePixelFormatGpu.h"
#include "OgreTextureGpu.h"

#include "vulkan/vulkan_core.h"

namespace Ogre
{
    class _OgreVulkanExport VulkanMappings
    {
    public:
        static VkPrimitiveTopology get( OperationType opType );
        static VkPolygonMode get( PolygonMode polygonMode );
        static VkCullModeFlags get( CullingMode cullMode );
        static VkCompareOp get( CompareFunction compareFunc );
        static VkStencilOp get( StencilOperation stencilOp );

        static VkBlendFactor get( SceneBlendFactor blendFactor );
        static VkBlendOp get( SceneBlendOperation blendOp );

        static VkFormat get( VertexElementType vertexElemType );

        static VkFilter get( FilterOptions filter );
        static VkSamplerMipmapMode getMipFilter( FilterOptions filter );
        static VkSamplerAddressMode get( TextureAddressingMode mode );

        static VkImageViewType get( TextureTypes::TextureTypes textureType );
        static VkFormat get( PixelFormatGpu pf );
        static VkImageAspectFlags getImageAspect( PixelFormatGpu pf,
                                                  const bool bPreferDepthOverStencil = false );

        static VkAccessFlags get( const TextureGpu *texture );
        static VkAccessFlags get( BufferPackedTypes bufferPackedTypes );

        static VkAccessFlags getAccessFlags( ResourceLayout::Layout layout,
                                             ResourceAccess::ResourceAccess access,
                                             const TextureGpu *texture, bool bIsDst );

        static VkImageLayout get( ResourceLayout::Layout layout, const TextureGpu *texture );

        static uint32_t getFormatSize( VkFormat format );

        static GpuConstantType get( SpvOp op );

        // For HLSL
        static VertexElementSemantic getHlslSemantic( const char *sem );
    };
}  // namespace Ogre

#endif
