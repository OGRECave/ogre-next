/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

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
#ifndef __D3D11MAPPINGS_H__
#define __D3D11MAPPINGS_H__

#include "OgreD3D11Prerequisites.h"
#include "OgreTextureUnitState.h"
#include "OgreHlmsSamplerblock.h"
#include "OgreRenderSystem.h"
#include "OgreHardwareIndexBuffer.h"
#include "OgreRoot.h"
#include "OgrePixelFormatGpu.h"
#include "OgreTextureGpu.h"

namespace Ogre 
{
    class _OgreD3D11Export D3D11Mappings
	{
	public:
        /// return a D3D11 equivalent for a Ogre TextureAddressingMode value
        static D3D11_TEXTURE_ADDRESS_MODE get(TextureAddressingMode tam);
		/// return a D3D11 equivalent for a Ogre SceneBlendFactor value
		static D3D11_BLEND get(SceneBlendFactor sbf, bool forAlpha);
		/// return a D3D11 equivalent for a Ogre SceneBlendOperation value
		static D3D11_BLEND_OP get(SceneBlendOperation sbo);
		/// return a D3D11 equivalent for a Ogre CompareFunction value
		static D3D11_COMPARISON_FUNC get(CompareFunction cf);
		/// return a D3D11 equivalent for a Ogre CillingMode value
		static D3D11_CULL_MODE get(CullingMode cm, bool flip = false);
		/// return a D3D11 equivalent for a Ogre PolygonMode value
		static D3D11_FILL_MODE get(PolygonMode level);
		/// return a D3D11 equivalent for a Ogre StencilOperation value
		static D3D11_STENCIL_OP get(StencilOperation op, bool invert = false);
		/// return a D3D11 state type for Ogre FilterOption min/mag/mip values
		static D3D11_FILTER get(const FilterOptions minification, const FilterOptions magnification, const FilterOptions mips, const bool comparison = false);
		/// Get lock options
        static D3D11_MAP get(v1::HardwareBuffer::LockOptions options, v1::HardwareBuffer::Usage usage);
		/// Get index type
        static DXGI_FORMAT getFormat(v1::HardwareIndexBuffer::IndexType itype);
		/// Get vertex data type
		static DXGI_FORMAT get(VertexElementType vType);
		/// Get vertex semantic
		static LPCSTR get(VertexElementSemantic sem);
		static VertexElementSemantic get(LPCSTR sem);
		/// Get dx11 color
		static void get(const ColourValue& inColour, float * outColour );

        static D3D11_USAGE _getUsage(v1::HardwareBuffer::Usage usage);
        static UINT _getAccessFlags(v1::HardwareBuffer::Usage usage);
        static bool _isDynamic(v1::HardwareBuffer::Usage usage);

        static UINT get( MsaaPatterns::MsaaPatterns msaaPatterns );
        static D3D11_SRV_DIMENSION get( TextureTypes::TextureTypes type,
                                        bool cubemapsAs2DArrays, bool forMsaa );
        static DXGI_FORMAT get( PixelFormatGpu pf );
        static DXGI_FORMAT getForSrv( PixelFormatGpu pf );
        static DXGI_FORMAT getFamily( PixelFormatGpu pf );
	};
}
#endif
