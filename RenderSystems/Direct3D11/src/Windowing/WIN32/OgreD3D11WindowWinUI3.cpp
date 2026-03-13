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

#include "OgreD3D11Device.h"
#include "OgreD3D11RenderSystem.h"
#include "OgreD3D11Window.h"

#include "OgreLogManager.h"
#include "OgreOSVersionHelpers.h"
#include "OgreRoot.h"

#include <iomanip>

// We support both OGRE_PLATFORM_WIN32 and Desktop Compatible OGRE_PLATFORM_WINRT for WinUI3.

#ifdef OGRE_WINUI3_SWAPCHAINPANEL_SUPPORTED
#    include <dxgi1_3.h>  // for IDXGISwapChain2::SetMatrixTransform used in D3D11RenderWindowSwapChainPanel
#    include <microsoft.ui.xaml.media.dxinterop.h>  // for ISwapChainPanelNative

namespace Ogre
{
    D3D11WindowSwapChainPanelWinUI3::D3D11WindowSwapChainPanelWinUI3(
        const String &title, uint32 width, uint32 height, bool fullscreenMode,
        PixelFormatGpu depthStencilFormat, const NameValuePairList *miscParams, D3D11Device &device,
        D3D11RenderSystem *renderSystem ) :
        D3D11WindowSwapChainBased( title, width, height, fullscreenMode, depthStencilFormat, miscParams,
                                   device, renderSystem ),
        mCompositionScale( 1.0f, 1.0f )

    {
        mUseFlipMode = true;

        winrt::Windows::Foundation::IInspectable externalHandle = nullptr;
        if( miscParams )
        {
            NameValuePairList::const_iterator opt = miscParams->find( "externalWindowHandle" );
            if( opt != miscParams->end() )
            {
                winrt::copy_from_abi( externalHandle,
                                      (void *)StringConverter::parseSizeT( opt->second ) );
            }
        }

        if( !externalHandle )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "External window handle is not specified.",
                         "D3D11RenderWindow::create" );
        }
        else
        {
            mSwapChainPanel = externalHandle.as<winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel>();
            mIsExternal = true;

            // subscribe to important notifications
            compositionScaleChangedToken = mSwapChainPanel.CompositionScaleChanged(
                [this]( auto const &, auto const & ) { windowMovedOrResized(); } );
            sizeChangedToken = mSwapChainPanel.SizeChanged(
                [this]( auto const &, auto const & ) { windowMovedOrResized(); } );
        }

        winrt::Windows::Foundation::Size sz{ static_cast<float>( mSwapChainPanel.ActualWidth() ),
                                             static_cast<float>( mSwapChainPanel.ActualHeight() ) };
        mCompositionScale = winrt::Windows::Foundation::Size( mSwapChainPanel.CompositionScaleX(),
                                                              mSwapChainPanel.CompositionScaleY() );
        mRequestedWidth = (int)floorf( sz.Width + 0.5f );
        mRequestedHeight = (int)floorf( sz.Height + 0.5f );
    }
    //-----------------------------------------------------------------------------------
    D3D11WindowSwapChainPanelWinUI3::~D3D11WindowSwapChainPanelWinUI3() { destroy(); }
    //---------------------------------------------------------------------
    void D3D11WindowSwapChainPanelWinUI3::destroy()
    {
        D3D11WindowSwapChainBased::destroy();

        if( mSwapChainPanel && !mIsExternal )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Only external window handles are supported.",
                         "D3D11RenderWindow::destroy" );
        }
        mSwapChainPanel.CompositionScaleChanged( compositionScaleChangedToken );
        compositionScaleChangedToken.value = 0;
        mSwapChainPanel.SizeChanged( sizeChangedToken );
        sizeChangedToken.value = 0;
        mSwapChainPanel = nullptr;
    }
    //-----------------------------------------------------------------------------------
    float D3D11WindowSwapChainPanelWinUI3::getViewPointToPixelScale() const
    {
        return std::max( mCompositionScale.Width, mCompositionScale.Height );
    }
    //---------------------------------------------------------------------
    HRESULT D3D11WindowSwapChainPanelWinUI3::_createSwapChainImpl()
    {
        mSampleDescription = mRenderSystem->validateSampleDescription(
            mRequestedSampleDescription, _getRenderFormat(),
            TextureFlags::NotTexture | TextureFlags::RenderWindowSpecific );

        int widthPx = std::max( 1, (int)floorf( mRequestedWidth * mCompositionScale.Width + 0.5f ) );
        int heightPx = std::max( 1, (int)floorf( mRequestedHeight * mCompositionScale.Height + 0.5f ) );

        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Width = widthPx;
        desc.Height = heightPx;
        desc.Format = _getSwapChainFormat();
        desc.Stereo = false;

        assert( mUseFlipMode );  // i.e. no FSAA for swapchain, but can be enabled in separate backbuffer
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;

        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;                 // Use two buffers to enable flip effect.
        desc.Scaling = DXGI_SCALING_STRETCH;  // Required for CreateSwapChainForComposition.
        desc.SwapEffect =
            IsWindows10OrGreater() ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        desc.Flags = _getSwapChainFlags();

        // Create swap chain
        HRESULT hr = mDevice.GetDXGIFactory()->CreateSwapChainForComposition(
            mDevice.get(), &desc, NULL, mSwapChain1.ReleaseAndGetAddressOf() );
        if( FAILED( hr ) )
            return hr;

        // Associate swap chain with SwapChainPanel
        // Get backing native interface for SwapChainPanel
        ComPtr<ISwapChainPanelNative> panelNative;
        hr = reinterpret_cast<IUnknown *>( winrt::get_abi( mSwapChainPanel ) )
                 ->QueryInterface( IID_PPV_ARGS( panelNative.ReleaseAndGetAddressOf() ) );
        if( FAILED( hr ) )
            return hr;
        hr = panelNative->SetSwapChain( mSwapChain1.Get() );
        if( FAILED( hr ) )
            return hr;

        // Ensure that DXGI does not queue more than one frame at a time. This both reduces
        // latency and ensures that the application will only render after each VSync, minimizing
        // power consumption.
        ComPtr<IDXGISwapChain2> pSwapChain2;
        if( SUCCEEDED( mSwapChain1->QueryInterface( pSwapChain2.GetAddressOf() ) ) )
            pSwapChain2->SetMaximumFrameLatency( 1 );

        hr = _compensateSwapChainCompositionScale();
        return hr;
    }
    //---------------------------------------------------------------------
    void D3D11WindowSwapChainPanelWinUI3::_destroySwapChain()
    {
        // Broke association between SwapChainPanel and swap chain to avoid reporting it as leaked on
        // device lost event
        ComPtr<ISwapChainPanelNative> panelNative;
        if( SUCCEEDED( reinterpret_cast<IUnknown *>( winrt::get_abi( mSwapChainPanel ) )
                           ->QueryInterface( IID_PPV_ARGS( panelNative.ReleaseAndGetAddressOf() ) ) ) )
            panelNative->SetSwapChain( 0 );

        D3D11WindowSwapChainBased::_destroySwapChain();
    }
    //---------------------------------------------------------------------
    HRESULT D3D11WindowSwapChainPanelWinUI3::_compensateSwapChainCompositionScale()
    {
        // Setup inverse scale on the swap chain
        DXGI_MATRIX_3X2_F inverseScale = { 0 };
        inverseScale._11 = 1.0f / mCompositionScale.Width;
        inverseScale._22 = 1.0f / mCompositionScale.Height;
        ComPtr<IDXGISwapChain2> pSwapChain2;
        HRESULT hr = mSwapChain1.As<IDXGISwapChain2>( &pSwapChain2 );
        if( FAILED( hr ) )
            return hr;

        hr = pSwapChain2->SetMatrixTransform( &inverseScale );
        return hr;
    }
    //---------------------------------------------------------------------
    void D3D11WindowSwapChainPanelWinUI3::windowMovedOrResized()
    {
        winrt::Windows::Foundation::Size sz{ static_cast<float>( mSwapChainPanel.ActualWidth() ),
                                             static_cast<float>( mSwapChainPanel.ActualHeight() ) };
        mCompositionScale = winrt::Windows::Foundation::Size( mSwapChainPanel.CompositionScaleX(),
                                                              mSwapChainPanel.CompositionScaleY() );
        mRequestedWidth = (int)floorf( sz.Width + 0.5f );
        mRequestedHeight = (int)floorf( sz.Height + 0.5f );

        int widthPx = std::max( 1, (int)floorf( mRequestedWidth * mCompositionScale.Width + 0.5f ) );
        int heightPx = std::max( 1, (int)floorf( mRequestedHeight * mCompositionScale.Height + 0.5f ) );

        resizeSwapChainBuffers( widthPx, heightPx );

        _compensateSwapChainCompositionScale();
    }
    //---------------------------------------------------------------------
    bool D3D11WindowSwapChainPanelWinUI3::isVisible() const
    {
        return ( mSwapChainPanel &&
                 mSwapChainPanel.Visibility() == winrt::Microsoft::UI::Xaml::Visibility::Visible );
    }
}  // namespace Ogre

#endif
