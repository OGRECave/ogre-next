/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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
#ifndef __D3D11PREREQUISITES_H__
#define __D3D11PREREQUISITES_H__

#include "OgrePrerequisites.h"
#if OGRE_PLATFORM != OGRE_PLATFORM_WINRT
#    include "WIN32/OgreMinGWSupport.h"  // extra defines for MinGW to deal with DX SDK
#endif
#include "WIN32/OgreComPtr.h"  // too much resource leaks were caused without it by throwing constructors

// some D3D commonly used macros
#define SAFE_DELETE( p ) \
    { \
        if( p ) \
        { \
            delete( p ); \
            ( p ) = NULL; \
        } \
    }
#define SAFE_DELETE_ARRAY( p ) \
    { \
        if( p ) \
        { \
            delete[]( p ); \
            ( p ) = NULL; \
        } \
    }
#define SAFE_RELEASE( p ) \
    { \
        if( p ) \
        { \
            ( p )->Release(); \
            ( p ) = NULL; \
        } \
    }

#if defined( _WIN32_WINNT_WIN8 )  // Win8 SDK required to compile, will work on Windows 8 and Platform
                                  // Update for Windows 7
#    define OGRE_D3D11_PROFILING OGRE_PROFILING
#endif

#undef NOMINMAX
#define NOMINMAX  // required to stop windows.h screwing up std::min definition
#if defined( _WIN32_WINNT_WIN8 ) || OGRE_COMPILER != OGRE_COMPILER_MSVC
#    include <d3d11_1.h>
#    if !defined( _WIN32_WINNT_WIN10 )
#        define DXGI_SWAP_EFFECT_FLIP_DISCARD \
            ( (DXGI_SWAP_EFFECT)( 4 ) )  // we want to use it on Win10 even if building with Win8 SDK
#        define D3D11_RLDO_IGNORE_INTERNAL ( (D3D11_RLDO_FLAGS)( 4 ) )
#    endif
#else
#    include <d3d11.h>
#    include "OgreD3D11LegacySDKEmulation.h"
#endif

#if __OGRE_WINRT_PHONE_80
#    include <C:\Program Files (x86)\Windows Kits\8.0\Include\um\d3d11shader.h>
#else
#    include <d3d11shader.h>
#    include <d3dcompiler.h>
#endif

namespace Ogre
{
    // typedefs to work with Direct3D 11 or 11.1 as appropriate
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    typedef ID3D11Device          ID3D11DeviceN;
    typedef ID3D11DeviceContext   ID3D11DeviceContextN;
    typedef ID3D11RasterizerState ID3D11RasterizerStateN;
    typedef IDXGIFactory1         IDXGIFactoryN;
    typedef IDXGIAdapter1         IDXGIAdapterN;
    typedef IDXGIDevice1          IDXGIDeviceN;
    typedef IDXGISwapChain        IDXGISwapChainN;
    typedef DXGI_SWAP_CHAIN_DESC  DXGI_SWAP_CHAIN_DESC_N;
#elif OGRE_PLATFORM == OGRE_PLATFORM_WINRT
    typedef ID3D11Device1          ID3D11DeviceN;
    typedef ID3D11DeviceContext1   ID3D11DeviceContextN;
    typedef ID3D11RasterizerState1 ID3D11RasterizerStateN;
    typedef IDXGIFactory2          IDXGIFactoryN;
    typedef IDXGIAdapter1          IDXGIAdapterN;  // we don`t need IDXGIAdapter2 functionality
    typedef IDXGIDevice2           IDXGIDeviceN;
    typedef IDXGISwapChain1        IDXGISwapChainN;
    typedef DXGI_SWAP_CHAIN_DESC1  DXGI_SWAP_CHAIN_DESC_N;
#endif

    // Predefine classes
    class D3D11AsyncTextureTicket;
    class D3D11RenderSystem;
    class D3D11CompatBufferInterface;
    class D3D11Driver;
    class D3D11DriverList;
    class D3D11DynamicBuffer;
    class D3D11VideoMode;
    class D3D11VideoModeList;
    class D3D11GpuProgramManager;
    struct D3D11HlmsPso;
    class D3D11HLSLProgramFactory;
    class D3D11HLSLProgram;
    class D3D11Device;
    class D3D11StagingTexture;
    class D3D11TextureGpu;
    class D3D11VaoManager;
    class D3D11VendorExtension;
    struct D3D11VertexArrayObjectShared;
    class D3D11Window;

    namespace v1
    {
        class D3D11HardwareBuffer;
        class D3D11HardwareBufferManager;
        class D3D11HardwareIndexBuffer;
        class D3D11HardwarePixelBuffer;
    }  // namespace v1

    typedef SharedPtr<D3D11HLSLProgram> D3D11HLSLProgramPtr;

    //-------------------------------------------
    // Windows settings
    //-------------------------------------------
#if( OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT ) && \
    !defined( OGRE_STATIC_LIB )
#    ifdef OGRED3DENGINEDLL_EXPORTS
#        define _OgreD3D11Export __declspec( dllexport )
#    else
#        if defined( __MINGW32__ )
#            define _OgreD3D11Export
#        else
#            define _OgreD3D11Export __declspec( dllimport )
#        endif
#    endif
#else
#    define _OgreD3D11Export
#endif  // OGRE_WIN32
}  // namespace Ogre
#endif
