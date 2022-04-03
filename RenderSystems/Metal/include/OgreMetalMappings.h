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

#ifndef _OgreMetalMappings_H_
#define _OgreMetalMappings_H_

#include "OgreMetalPrerequisites.h"

#include "OgreBlendMode.h"
#include "OgreGpuProgramParams.h"
#include "OgreHlmsSamplerblock.h"
#include "OgrePixelFormatGpu.h"
#include "Vao/OgreVertexElements.h"

#import <Metal/MTLDepthStencil.h>
#import <Metal/MTLPixelFormat.h>
#import <Metal/MTLRenderPipeline.h>
#import <Metal/MTLSampler.h>
#import <Metal/MTLVertexDescriptor.h>

namespace Ogre
{
    class _OgreMetalExport MetalMappings
    {
    public:
        /// Can throw if PixelFormat is not depth.
        static void              getDepthStencilFormat( PixelFormatGpu pf, MetalDevice *device,
                                                        MTLPixelFormat &outDepth, MTLPixelFormat &outStencil );
        static MTLBlendFactor    get( SceneBlendFactor op );
        static MTLBlendOperation get( SceneBlendOperation op );
        /// @see HlmsBlendblock::BlendChannelMasks
        static MTLColorWriteMask get( uint8 mask );

        static MTLStencilOperation get( StencilOperation op );

        static MTLCompareFunction get( CompareFunction cmp );

        static MTLVertexFormat get( VertexElementType vertexElemType );

        static MTLSamplerMinMagFilter get( FilterOptions filter );
        static MTLSamplerMipFilter    getMipFilter( FilterOptions filter );
        static MTLSamplerAddressMode  get( TextureAddressingMode mode );

        static MTLVertexFormat dataTypeToVertexFormat( MTLDataType dataType );

        static MTLPixelFormat get( PixelFormatGpu pf, MetalDevice *device );

        static GpuConstantType get( MTLDataType dataType );
    };
}  // namespace Ogre

#endif
