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

#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT && defined( _WIN32_WINNT_WINBLUE ) && \
    _WIN32_WINNT >= _WIN32_WINNT_WINBLUE
#    include <dxgi1_3.h>  // for IDXGISwapChain2::SetMatrixTransform used in D3D11RenderWindowSwapChainPanel
#    if !__OGRE_WINRT_PHONE_80
#        include <windows.ui.xaml.media.dxinterop.h>  // for ISwapChainPanelNative
#    endif
#endif

namespace Ogre
{
#pragma region D3D11WindowCoreWindow
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT
    D3D11WindowCoreWindow::D3D11WindowCoreWindow( const String &title, uint32 width, uint32 height,
                                                  bool fullscreenMode, PixelFormatGpu depthStencilFormat,
                                                  const NameValuePairList *miscParams,
                                                  D3D11Device &device,
                                                  D3D11RenderSystem *renderSystem ) :
        D3D11WindowSwapChainBased( title, width, height, fullscreenMode, depthStencilFormat, miscParams,
                                   device, renderSystem )
    {
        mUseFlipMode = true;

        Windows::UI::Core::CoreWindow ^ externalHandle = nullptr;
        if( miscParams )
        {
            NameValuePairList::const_iterator opt = miscParams->find( "externalWindowHandle" );
            if( opt != miscParams->end() )
                externalHandle = reinterpret_cast<Windows::UI::Core::CoreWindow ^>(
                    (void *)StringConverter::parseSizeT( opt->second ) );
            else
                externalHandle = Windows::UI::Core::CoreWindow::GetForCurrentThread();
        }
        else
            externalHandle = Windows::UI::Core::CoreWindow::GetForCurrentThread();

        if( !externalHandle )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "External window handle is not specified.",
                         "D3D11WindowSwapChainPanel::.ctor" );
        }
        else
        {
            mCoreWindow = externalHandle;
            mIsExternal = true;
        }

        float scale = getViewPointToPixelScale();
        Windows::Foundation::Rect rc = mCoreWindow->Bounds;
        mLeft = (int)floorf( rc.X * scale + 0.5f );
        mTop = (int)floorf( rc.Y * scale + 0.5f );
        mRequestedWidth = (int)floorf( rc.Width + 0.5f );
        mRequestedHeight = (int)floorf( rc.Height + 0.5f );
    }
    //-----------------------------------------------------------------------------------
    D3D11WindowCoreWindow::~D3D11WindowCoreWindow() { destroy(); }
    //---------------------------------------------------------------------
    void D3D11WindowCoreWindow::destroy()
    {
        D3D11WindowSwapChainBased::destroy();

        if( mCoreWindow.Get() && !mIsExternal )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Only external window handles are supported.",
                         "D3D11RenderWindow::destroy" );
        }

        mCoreWindow = nullptr;
    }
    //-----------------------------------------------------------------------------------
    float D3D11WindowCoreWindow::getViewPointToPixelScale() const
    {
#    if defined( _WIN32_WINNT_WINBLUE ) && _WIN32_WINNT >= _WIN32_WINNT_WINBLUE
        return Windows::Graphics::Display::DisplayInformation::GetForCurrentView()->LogicalDpi / 96;
#    else
        return Windows::Graphics::Display::DisplayProperties::LogicalDpi / 96;
#    endif
    }
    //---------------------------------------------------------------------
    HRESULT D3D11WindowCoreWindow::_createSwapChainImpl()
    {
#    if !__OGRE_WINRT_PHONE
        mSampleDescription = mRenderSystem->validateSampleDescription(
            mRequestedSampleDescription, _getRenderFormat(),
            TextureFlags::NotTexture | TextureFlags::RenderWindowSpecific );
#    endif
        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Width = 0;  // Use automatic sizing.
        desc.Height = 0;
        desc.Format = _getSwapChainFormat();
        desc.Stereo = false;

        assert( mUseFlipMode );  // i.e. no MSAA for swapchain, but can be enabled in separate backbuffer
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;

        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
#    if __OGRE_WINRT_PHONE_80
        desc.BufferCount = 1;                        // WP8: One buffer.
        desc.Scaling = DXGI_SCALING_STRETCH;         // WP8: Must be stretch scaling mode.
        desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;  // WP8: No swap effect.
#    else
        desc.BufferCount = 2;              // Use two buffers to enable flip effect.
        desc.Scaling = DXGI_SCALING_NONE;  // Otherwise stretch would be used by default.
        desc.SwapEffect =
            IsWindows10OrGreater() ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
#    endif
        desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        desc.Flags = _getSwapChainFlags();

        // Create swap chain
        HRESULT hr = mDevice.GetDXGIFactory()->CreateSwapChainForCoreWindow(
            mDevice.get(), reinterpret_cast<IUnknown *>( mCoreWindow.Get() ), &desc, NULL,
            mSwapChain1.ReleaseAndGetAddressOf() );
        if( FAILED( hr ) )
            return hr;

            // Ensure that DXGI does not queue more than one frame at a time. This both reduces
            // latency and ensures that the application will only render after each VSync, minimizing
            // power consumption.
#    if defined( _WIN32_WINNT_WINBLUE ) && _WIN32_WINNT >= _WIN32_WINNT_WINBLUE
        ComPtr<IDXGISwapChain2> pSwapChain2;
        if( SUCCEEDED( mSwapChain1->QueryInterface( pSwapChain2.GetAddressOf() ) ) )
            pSwapChain2->SetMaximumFrameLatency( 1 );  // Win8.1
        else
#    endif
        {
            ComPtr<IDXGIDevice2> pDXGIDevice2;
            if( SUCCEEDED( mDevice->QueryInterface( pDXGIDevice2.GetAddressOf() ) ) )
                pDXGIDevice2->SetMaximumFrameLatency( 1 );  // Win8.0
        }

        return hr;
    }
    //---------------------------------------------------------------------
    void D3D11WindowCoreWindow::windowMovedOrResized()
    {
        float scale = getViewPointToPixelScale();
        Windows::Foundation::Rect rc = mCoreWindow->Bounds;
        mLeft = (int)floorf( rc.X * scale + 0.5f );
        mTop = (int)floorf( rc.Y * scale + 0.5f );
        mRequestedWidth = (int)floorf( rc.Width + 0.5f );
        mRequestedHeight = (int)floorf( rc.Height + 0.5f );

        resizeSwapChainBuffers( 0, 0 );  // pass zero to autodetect size
    }
    //---------------------------------------------------------------------
    bool D3D11WindowCoreWindow::isVisible() const
    {
        return ( mCoreWindow.Get() &&
                 Windows::UI::Core::CoreWindow::GetForCurrentThread() == mCoreWindow.Get() );
    }

#endif
#pragma endregion

#pragma region D3D11WindowSwapChainPanel
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT && defined( _WIN32_WINNT_WINBLUE ) && \
    _WIN32_WINNT >= _WIN32_WINNT_WINBLUE
    D3D11WindowSwapChainPanel::D3D11WindowSwapChainPanel( const String &title, uint32 width,
                                                          uint32 height, bool fullscreenMode,
                                                          PixelFormatGpu depthStencilFormat,
                                                          const NameValuePairList *miscParams,
                                                          D3D11Device &device,
                                                          D3D11RenderSystem *renderSystem ) :
        D3D11WindowSwapChainBased( title, width, height, fullscreenMode, depthStencilFormat, miscParams,
                                   device, renderSystem ),
        mCompositionScale( 1.0f, 1.0f )

    {
        mUseFlipMode = true;

        Windows::UI::Xaml::Controls::SwapChainPanel ^ externalHandle = nullptr;
        if( miscParams )
        {
            NameValuePairList::const_iterator opt = miscParams->find( "externalWindowHandle" );
            if( opt != miscParams->end() )
                externalHandle = reinterpret_cast<Windows::UI::Xaml::Controls::SwapChainPanel ^>(
                    (void *)StringConverter::parseSizeT( opt->second ) );
        }

        if( !externalHandle )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "External window handle is not specified.",
                         "D3D11RenderWindow::create" );
        }
        else
        {
            mSwapChainPanel = externalHandle;
            mIsExternal = true;

            // subscribe to important notifications
            compositionScaleChangedToken =
                ( mSwapChainPanel->CompositionScaleChanged +=
                  ref new Windows::Foundation::TypedEventHandler<
                      Windows::UI::Xaml::Controls::SwapChainPanel ^, Platform::Object ^>(
                      [this]( Windows::UI::Xaml::Controls::SwapChainPanel ^ sender,
                              Platform::Object ^ e ) { windowMovedOrResized(); } ) );
            sizeChangedToken =
                ( mSwapChainPanel->SizeChanged += ref new Windows::UI::Xaml::SizeChangedEventHandler(
                      [this]( Platform::Object ^ sender, Windows::UI::Xaml::SizeChangedEventArgs ^ e ) {
                          windowMovedOrResized();
                      } ) );
        }

        Windows::Foundation::Size sz =
            Windows::Foundation::Size( static_cast<float>( mSwapChainPanel->ActualWidth ),
                                       static_cast<float>( mSwapChainPanel->ActualHeight ) );
        mCompositionScale = Windows::Foundation::Size( mSwapChainPanel->CompositionScaleX,
                                                       mSwapChainPanel->CompositionScaleY );
        mRequestedWidth = (int)floorf( sz.Width + 0.5f );
        mRequestedHeight = (int)floorf( sz.Height + 0.5f );
    }
    //-----------------------------------------------------------------------------------
    D3D11WindowSwapChainPanel::~D3D11WindowSwapChainPanel() { destroy(); }
    //---------------------------------------------------------------------
    void D3D11WindowSwapChainPanel::destroy()
    {
        D3D11WindowSwapChainBased::destroy();

        if( mSwapChainPanel && !mIsExternal )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Only external window handles are supported.",
                         "D3D11RenderWindow::destroy" );
        }
        mSwapChainPanel->CompositionScaleChanged -= compositionScaleChangedToken;
        compositionScaleChangedToken.Value = 0;
        mSwapChainPanel->SizeChanged -= sizeChangedToken;
        sizeChangedToken.Value = 0;
        mSwapChainPanel = nullptr;
    }
    //-----------------------------------------------------------------------------------
    float D3D11WindowSwapChainPanel::getViewPointToPixelScale() const
    {
        return std::max( mCompositionScale.Width, mCompositionScale.Height );
    }
    //---------------------------------------------------------------------
    HRESULT D3D11WindowSwapChainPanel::_createSwapChainImpl()
    {
#    if !__OGRE_WINRT_PHONE
        mSampleDescription = mRenderSystem->validateSampleDescription(
            mRequestedSampleDescription, _getRenderFormat(),
            TextureFlags::NotTexture | TextureFlags::RenderWindowSpecific );
#    endif

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
        hr = reinterpret_cast<IUnknown *>( mSwapChainPanel )
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
    void D3D11WindowSwapChainPanel::_destroySwapChain()
    {
        // Broke association between SwapChainPanel and swap chain to avoid reporting it as leaked on
        // device lost event
        ComPtr<ISwapChainPanelNative> panelNative;
        if( SUCCEEDED( reinterpret_cast<IUnknown *>( mSwapChainPanel )
                           ->QueryInterface( IID_PPV_ARGS( panelNative.ReleaseAndGetAddressOf() ) ) ) )
            panelNative->SetSwapChain( 0 );

        D3D11WindowSwapChainBased::_destroySwapChain();
    }
    //---------------------------------------------------------------------
    HRESULT D3D11WindowSwapChainPanel::_compensateSwapChainCompositionScale()
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
    void D3D11WindowSwapChainPanel::windowMovedOrResized()
    {
        Windows::Foundation::Size sz =
            Windows::Foundation::Size( static_cast<float>( mSwapChainPanel->ActualWidth ),
                                       static_cast<float>( mSwapChainPanel->ActualHeight ) );
        mCompositionScale = Windows::Foundation::Size( mSwapChainPanel->CompositionScaleX,
                                                       mSwapChainPanel->CompositionScaleY );
        mRequestedWidth = (int)floorf( sz.Width + 0.5f );
        mRequestedHeight = (int)floorf( sz.Height + 0.5f );

        int widthPx = std::max( 1, (int)floorf( mRequestedWidth * mCompositionScale.Width + 0.5f ) );
        int heightPx = std::max( 1, (int)floorf( mRequestedHeight * mCompositionScale.Height + 0.5f ) );

        resizeSwapChainBuffers( widthPx, heightPx );

        _compensateSwapChainCompositionScale();
    }
    //---------------------------------------------------------------------
    bool D3D11WindowSwapChainPanel::isVisible() const
    {
        return ( mSwapChainPanel &&
                 mSwapChainPanel->Visibility == Windows::UI::Xaml::Visibility::Visible );
    }
#endif
#pragma endregion
}  // namespace Ogre
