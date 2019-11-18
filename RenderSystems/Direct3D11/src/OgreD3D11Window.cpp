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

#include "OgreD3D11Window.h"
#include "OgreD3D11RenderSystem.h"
#include "OgreD3D11Mappings.h"

#include "OgrePixelFormatGpuUtils.h"
#include "OgreStringConverter.h"

#define TODO_convert_to_MSAA_pattern

namespace Ogre
{
    D3D11Window::D3D11Window( const String &title, uint32 width, uint32 height,
                              bool fullscreenMode, PixelFormatGpu depthStencilFormat,
                              const NameValuePairList *miscParams,
                              D3D11Device &device, D3D11RenderSystem *renderSystem ) :
        Window( title, width, height, fullscreenMode ),
        mDevice( device ),
        mIsExternal( false ),
        mSizing( false ),
        mClosed( false ),
        mHidden( false ),
        mAlwaysWindowedMode( false ),
        mHwGamma( false ),
        mVisible( true ),
        mMsaa( 0 ),
        mMsaaHint(),
        mRenderSystem( renderSystem )
    {
        mMsaaDesc.Count     = 1u;
        mMsaaDesc.Quality   = 0;

        if( miscParams )
        {
            NameValuePairList::const_iterator opt;
            // hidden   [parseBool]
            opt = miscParams->find("hidden");
            if( opt != miscParams->end() )
                mHidden = StringConverter::parseBool(opt->second);
            // MSAA
            opt = miscParams->find("MSAA");
            if( opt != miscParams->end() )
                mMsaa = StringConverter::parseUnsignedInt( opt->second);
            // MSAA Quality
            opt = miscParams->find("MSAAHint");
            if (opt != miscParams->end())
                mMsaaHint = opt->second;
            // sRGB?
            opt = miscParams->find("gamma");
            if( opt != miscParams->end() )
                mHwGamma = StringConverter::parseBool(opt->second);
        }
    }
    //-----------------------------------------------------------------------------------
    D3D11Window::~D3D11Window()
    {
        mRenderSystem->_notifyWindowDestroyed( this );
    }
    //---------------------------------------------------------------------
    void D3D11Window::destroy()
    {
        mpBackBuffer.Reset();
        mpBackBufferNoMSAA.Reset();

        OGRE_DELETE mTexture;
        mTexture = 0;

        OGRE_DELETE mDepthBuffer;
        mDepthBuffer = 0;
        mStencilBuffer = 0;

        mClosed = true;
    }
    //-----------------------------------------------------------------------------------
    void D3D11Window::getCustomAttribute( IdString name, void* pData )
    {
        if( name == "D3DDEVICE" )
        {
            ID3D11DeviceN  **device = (ID3D11DeviceN**)pData;
            *device = mDevice.get();
        }
        else
        {
            Window::getCustomAttribute( name, pData );
        }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    D3D11WindowSwapChainBased::D3D11WindowSwapChainBased(
            const String &title, uint32 width, uint32 height, bool fullscreenMode,
            PixelFormatGpu depthStencilFormat, const NameValuePairList *miscParams,
            D3D11Device &device, D3D11RenderSystem *renderSystem ) :
        D3D11Window( title, width, height, fullscreenMode,
                     depthStencilFormat, miscParams,
                     device, renderSystem ),
        mUseFlipSequentialMode( false ),
        mPreviousPresentStatsIsValid( false ),
        mVBlankMissCount( 0 )
    {
        memset( &mPreviousPresentStats, 0, sizeof(mPreviousPresentStats) );
    }
    //-----------------------------------------------------------------------------------
    D3D11WindowSwapChainBased::~D3D11WindowSwapChainBased()
    {
    }
    //---------------------------------------------------------------------
    DXGI_FORMAT D3D11WindowSwapChainBased::_getSwapChainFormat()
    {
        // We prefer to use *_SRGB format for swapchain, so that multisampled swapchain are resolved properly.
        // Unfortunately, swapchains in flip mode are incompatible with multisampling and with *_SRGB formats,
        // and special handling is required.
        PixelFormatGpu pf = _getRenderFormat();
        if(mUseFlipSequentialMode)
            pf = PixelFormatGpuUtils::getEquivalentLinear(pf);
        return D3D11Mappings::get(pf);
    }
    //---------------------------------------------------------------------
    void D3D11WindowSwapChainBased::destroy()
    {
        _destroySwapChain();
        D3D11Window::destroy();
    }
    //---------------------------------------------------------------------
    void D3D11WindowSwapChainBased::_destroySwapChain()
    {
        // https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi#destroying-a-swap-chain
        // You may not release a swap chain in full-screen mode because doing so may create thread contention
        // (which will cause DXGI to raise a non-continuable exception). Before releasing a swap chain, first switch to windowed mode
        if(mFullscreenMode && mSwapChain)
            mSwapChain->SetFullscreenState(false, NULL);

        mSwapChain.Reset();
        mSwapChain1.Reset();
    }
    //---------------------------------------------------------------------
    void D3D11WindowSwapChainBased::_createSwapChain()
    {
        mSwapChain.Reset();
        mSwapChain1.Reset();

        HRESULT hr = _createSwapChainImpl();

        if (SUCCEEDED(hr) && mSwapChain1)
            hr = mSwapChain1.As(&mSwapChain);

        if (FAILED(hr))
        {
            const String errorDescription = mDevice.getErrorDescription(hr);
            OGRE_EXCEPT_EX(Exception::ERR_RENDERINGAPI_ERROR, hr,
                "Unable to create swap chain\nError Description:" + errorDescription,
                "D3D11WindowSwapChainBased::_createSwapChain");
        }
    }
}
