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

#include "OgreD3D11Window.h"

#include "OgreD3D11Mappings.h"
#include "OgreD3D11RenderSystem.h"
#include "OgreD3D11TextureGpuManager.h"
#include "OgreD3D11TextureGpuWindow.h"
#include "OgreDepthBuffer.h"
#include "OgreOSVersionHelpers.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreStringConverter.h"
#include "Vao/OgreVaoManager.h"

#define TODO_convert_to_MSAA_pattern
#define TODO_notify_listeners

namespace Ogre
{
    D3D11Window::D3D11Window( const String &title, uint32 width, uint32 height, bool fullscreenMode,
                              PixelFormatGpu depthStencilFormat, const NameValuePairList *miscParams,
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
        mRenderSystem( renderSystem )
    {
        if( miscParams )
        {
            NameValuePairList::const_iterator opt;
            // hidden   [parseBool]
            opt = miscParams->find( "hidden" );
            if( opt != miscParams->end() )
                mHidden = StringConverter::parseBool( opt->second );
            // FSAA
            opt = miscParams->find( "FSAA" );
            if( opt != miscParams->end() )
                mRequestedSampleDescription.parseString( opt->second );
            // sRGB?
            opt = miscParams->find( "gamma" );
            if( opt != miscParams->end() )
                mHwGamma = StringConverter::parseBool( opt->second );
        }
    }
    //-----------------------------------------------------------------------------------
    D3D11Window::~D3D11Window() { mRenderSystem->_notifyWindowDestroyed( this ); }
    //---------------------------------------------------------------------
    void D3D11Window::destroy()
    {
        mpBackBuffer.Reset();
        mpBackBufferInterim.Reset();

        OGRE_DELETE mTexture;
        mTexture = 0;

        OGRE_DELETE mDepthBuffer;
        mDepthBuffer = 0;
        mStencilBuffer = 0;

        mClosed = true;
    }
    //-----------------------------------------------------------------------------------
    void D3D11Window::getCustomAttribute( IdString name, void *pData )
    {
        if( name == "D3DDEVICE" || name == "RENDERDOC_DEVICE" )
        {
            ID3D11DeviceN **device = (ID3D11DeviceN **)pData;
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
    D3D11WindowSwapChainBased::D3D11WindowSwapChainBased( const String &title, uint32 width,
                                                          uint32 height, bool fullscreenMode,
                                                          PixelFormatGpu depthStencilFormat,
                                                          const NameValuePairList *miscParams,
                                                          D3D11Device &device,
                                                          D3D11RenderSystem *renderSystem ) :
        D3D11Window( title, width, height, fullscreenMode, depthStencilFormat, miscParams, device,
                     renderSystem ),
        mUseFlipMode( false ),
        mPreviousPresentStatsIsValid( false ),
        mVBlankMissCount( 0 )
    {
        memset( &mPreviousPresentStats, 0, sizeof( mPreviousPresentStats ) );
    }
    //-----------------------------------------------------------------------------------
    D3D11WindowSwapChainBased::~D3D11WindowSwapChainBased() {}
    //-----------------------------------------------------------------------------------
    void D3D11WindowSwapChainBased::notifyDeviceLost( D3D11Device *device )
    {
        _destroySizeDependedD3DResources();
        _destroySwapChain();
    }
    //-----------------------------------------------------------------------------------
    void D3D11WindowSwapChainBased::notifyDeviceRestored( D3D11Device *device, unsigned pass )
    {
        if( pass == 0 )
        {
            _createSwapChain();
            _createSizeDependedD3DResources();
        }
    }
    //-----------------------------------------------------------------------------------
    DXGI_FORMAT D3D11WindowSwapChainBased::_getSwapChainFormat()
    {
        // We prefer to use *_SRGB format for swapchain, so that multisampled swapchain are resolved
        // properly. Unfortunately, swapchains in flip mode are incompatible with multisampling and with
        // *_SRGB formats, and special handling is required.
        PixelFormatGpu pf = _getRenderFormat();
        if( mUseFlipMode )
            pf = PixelFormatGpuUtils::getEquivalentLinear( pf );
        return D3D11Mappings::get( pf );
    }
    //-----------------------------------------------------------------------------------
    DXGI_SWAP_CHAIN_FLAG D3D11WindowSwapChainBased::_getSwapChainFlags()
    {
        unsigned flags = 0;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT && defined( _WIN32_WINNT_WINBLUE ) && \
    _WIN32_WINNT >= _WIN32_WINNT_WINBLUE
        // We use SetMaximumFrameLatency in WinRT mode, and prefer to call it on swapchain rather than on
        // whole device
        if( IsWindows8Point1OrGreater() )
            flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
#endif
        return DXGI_SWAP_CHAIN_FLAG( flags );
    }
    //-----------------------------------------------------------------------------------
    uint8 D3D11WindowSwapChainBased::_getSwapChainBufferCount() const
    {
        return mUseFlipMode ? 2 : mRenderSystem->getVaoManager()->getDynamicBufferMultiplier() - 1u;
    }
    //-----------------------------------------------------------------------------------
    void D3D11WindowSwapChainBased::_initialize( TextureGpuManager *textureGpuManager )
    {
        _createSwapChain();

        D3D11TextureGpuManager *textureManager =
            static_cast<D3D11TextureGpuManager *>( textureGpuManager );

        mTexture = textureManager->createTextureGpuWindow( mUseFlipMode, this );
        if( DepthBuffer::DefaultDepthBufferFormat != PFG_NULL )
            mDepthBuffer = textureManager->createWindowDepthBuffer();

        mTexture->setPixelFormat( _getRenderFormat() );
        if( mDepthBuffer )
        {
            mDepthBuffer->setPixelFormat( DepthBuffer::DefaultDepthBufferFormat );
            if( PixelFormatGpuUtils::isStencil( mDepthBuffer->getPixelFormat() ) )
                mStencilBuffer = mDepthBuffer;
        }

        if( mDepthBuffer )
        {
            mTexture->_setDepthBufferDefaults( DepthBuffer::NO_POOL_EXPLICIT_RTV, false,
                                               mDepthBuffer->getPixelFormat() );
        }
        else
        {
            mTexture->_setDepthBufferDefaults( DepthBuffer::POOL_NO_DEPTH, false, PFG_NULL );
        }

        _createSizeDependedD3DResources();
        mClosed = false;
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
        // You may not release a swap chain in full-screen mode because doing so may create thread
        // contention (which will cause DXGI to raise a non-continuable exception). Before releasing a
        // swap chain, first switch to windowed mode
        if( mFullscreenMode && mSwapChain )
            mSwapChain->SetFullscreenState( false, NULL );

        mSwapChain.Reset();
        mSwapChain1.Reset();
    }
    //---------------------------------------------------------------------
    void D3D11WindowSwapChainBased::_createSwapChain()
    {
        mSwapChain.Reset();
        mSwapChain1.Reset();

        HRESULT hr = _createSwapChainImpl();

        if( SUCCEEDED( hr ) && mSwapChain1 )
            hr = mSwapChain1.As( &mSwapChain );

        if( FAILED( hr ) )
        {
            const String errorDescription = mDevice.getErrorDescription( hr );
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Unable to create swap chain\nError Description:" + errorDescription,
                            "D3D11WindowSwapChainBased::_createSwapChain" );
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11WindowSwapChainBased::_destroySizeDependedD3DResources()
    {
        if( mDepthBuffer && mDepthBuffer->getResidencyStatus() != GpuResidency::OnStorage )
            mDepthBuffer->_transitionTo( GpuResidency::OnStorage, (uint8 *)0 );
        if( mTexture->getResidencyStatus() != GpuResidency::OnStorage )
            mTexture->_transitionTo( GpuResidency::OnStorage, (uint8 *)0 );
        mpBackBuffer.Reset();
        mpBackBufferInterim.Reset();
    }
    //-----------------------------------------------------------------------------------
    void D3D11WindowSwapChainBased::_createSizeDependedD3DResources()
    {
        mpBackBuffer.Reset();
        mpBackBufferInterim.Reset();
        // Obtain back buffer from swapchain
        HRESULT hr = mSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ),
                                            (void **)mpBackBuffer.ReleaseAndGetAddressOf() );

        // check if we need workaround for broken back buffer casting in ResolveSubresource
        // From
        // https://github.com/microsoft/Xbox-ATG-Samples/blob/master/PCSamples/IntroGraphics/SimpleMSAA_PC/Readme.docx
        // Known issues:
        //     Due to a bug in the Windows 10 validation layer prior to the Windows 10 Fall Creators
        //     Update (16299), a DirectX 11 Resolve with an sRGB format using new "flip - style"
        //     swapchain would fail. This has been fixed in the newer versions of Windows 10.
        if( SUCCEEDED( hr ) && mHwGamma && isMultisample() && mUseFlipMode &&
            !IsWindows10RS3OrGreater() )
        {
            D3D11_TEXTURE2D_DESC desc = { 0 };
            mpBackBuffer->GetDesc( &desc );
            desc.Format = D3D11Mappings::get( _getRenderFormat() );
            hr = mDevice->CreateTexture2D( &desc, NULL, mpBackBufferInterim.ReleaseAndGetAddressOf() );
        }

        if( FAILED( hr ) )
        {
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Unable to Get Back Buffer for swap chain",
                            "D3D11WindowSwapChainBased::_createSizeDependedD3DResources" );
        }

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32  // to avoid DXGI ERROR: GetFullscreenDesc can only be called
                                          // for HWND based swapchains.
        if( mSwapChain1 )
        {
            DXGI_SWAP_CHAIN_DESC1 desc;
            mSwapChain1->GetDesc1( &desc );
            setFinalResolution( desc.Width, desc.Height );
            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsDesc;
            mSwapChain1->GetFullscreenDesc( &fsDesc );
            // Alt-Enter together with SetWindowAssociation() can change this state
            mRequestedFullscreenMode = fsDesc.Windowed == 0;
            mFullscreenMode = mRequestedFullscreenMode;
        }
        else
#endif
        {
            DXGI_SWAP_CHAIN_DESC desc;
            mSwapChain->GetDesc( &desc );
            setFinalResolution( desc.BufferDesc.Width, desc.BufferDesc.Height );
            // Alt-Enter together with SetWindowAssociation() can change this state
            mRequestedFullscreenMode = desc.Windowed == 0;
            mFullscreenMode = mRequestedFullscreenMode;
        }

        assert( dynamic_cast<D3D11TextureGpuWindow *>( mTexture ) );
        D3D11TextureGpuWindow *texWindow = static_cast<D3D11TextureGpuWindow *>( mTexture );
        texWindow->_setBackbuffer( mpBackBufferInterim ? mpBackBufferInterim.Get()
                                                       : mpBackBuffer.Get() );

        mTexture->_setSampleDescription( mRequestedSampleDescription, mSampleDescription );
        if( mDepthBuffer )
            mDepthBuffer->_setSampleDescription( mRequestedSampleDescription, mSampleDescription );

        mTexture->_transitionTo( GpuResidency::Resident, (uint8 *)0 );
        if( mDepthBuffer )
            mDepthBuffer->_transitionTo( GpuResidency::Resident, (uint8 *)0 );
    }
    //--------------------------------------------------------------------------
    void D3D11WindowSwapChainBased::setFsaa( const String &fsaa )
    {
        mRequestedSampleDescription.parseString( fsaa );

        mRenderSystem->fireDeviceEvent( &mDevice, "WindowBeforeResize", this );

        _destroySizeDependedD3DResources();

        // Call flush to ensure destruction of resources.
        // not doing so may result in 'Out of memory' exception.
        mDevice.GetImmediateContext()->Flush();

        if( mUseFlipMode )
        {
            // swapchain is not multisampled in flip sequential mode, so we reuse it
            // D3D11 doesn't care about texture flags, so we leave them as 0.
            mSampleDescription = mRenderSystem->validateSampleDescription( mRequestedSampleDescription,
                                                                           _getRenderFormat(), 0u, 0u );
        }
        else
        {
            _destroySwapChain();
            _createSwapChain();
        }

        _createSizeDependedD3DResources();

        // Notify viewports of resize
        notifyResolutionChanged();
        mRenderSystem->fireDeviceEvent( &mDevice, "WindowResized", this );
    }
    //-----------------------------------------------------------------------------------
    void D3D11WindowSwapChainBased::resizeSwapChainBuffers( uint32 width, uint32 height )
    {
        mRenderSystem->validateDevice();
        mRenderSystem->fireDeviceEvent( &mDevice, "WindowBeforeResize", this );

        _destroySizeDependedD3DResources();

        // Call flush before resize buffers to ensure destruction of resources.
        // not doing so may result in 'Out of memory' exception.
        mDevice.GetImmediateContext()->Flush();

        // width and height can be zero to autodetect size, therefore do not rely on them
        HRESULT hr = mSwapChain->ResizeBuffers( _getSwapChainBufferCount(), width, height,
                                                _getSwapChainFormat(), _getSwapChainFlags() );
        if( hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET )
        {
            mRenderSystem->handleDeviceLost();
            // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will
            // reenter this method and correctly set up the new device.
            return;
        }
        else if( FAILED( hr ) )
        {
            const String errorDescription = mDevice.getErrorDescription( hr );
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Unable to resize swap chain\nError Description:" + errorDescription,
                            "D3D11WindowSwapChainBased::resizeSwapChainBuffers" );
        }

        _createSizeDependedD3DResources();

        // Notify viewports of resize
        notifyResolutionChanged();
        mRenderSystem->fireDeviceEvent( &mDevice, "WindowResized", this );
    }
    //-----------------------------------------------------------------------------------
    void D3D11WindowSwapChainBased::notifyResolutionChanged() { TODO_notify_listeners; }
    //-----------------------------------------------------------------------------------
    void D3D11WindowSwapChainBased::swapBuffers()
    {
        mRenderSystem->fireDeviceEvent( &mDevice, "BeforeDevicePresent", this );

        if( !mDevice.isNull() )
        {
            // workaround needed only for mHwGamma && isMultisample() && mUseFlipMode &&
            // !IsWindows10RS3OrGreater()
            if( mpBackBufferInterim && mpBackBuffer )
                mDevice.GetImmediateContext()->CopyResource( mpBackBuffer.Get(),
                                                             mpBackBufferInterim.Get() );

            // flip presentation model swap chains have another semantic for first parameter
            UINT syncInterval =
                mUseFlipMode ? std::max( 1u, mVSyncInterval ) : ( mVSync ? mVSyncInterval : 0 );
            HRESULT hr = mSwapChain->Present( syncInterval, 0 );
            if( hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET )
                return;

            if( FAILED( hr ) )
            {
                OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr, "Error Presenting surfaces",
                                "D3D11WindowHwnd::swapBuffers" );
            }
        }
    }
}  // namespace Ogre
