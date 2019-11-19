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
#include "OgreD3D11Mappings.h"
#include "OgreD3D11RenderSystem.h"
#include "OgreD3D11TextureGpuManager.h"
#include "OgreD3D11TextureGpuWindow.h"

#include "OgreDepthBuffer.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreStringConverter.h"

#define TODO_convert_to_MSAA_pattern
#define TODO_notify_listeners

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
    //-----------------------------------------------------------------------------------
    uint8 D3D11WindowSwapChainBased::_getSwapChainBufferCount() const
    {
        return mUseFlipSequentialMode ? 2 : mRenderSystem->getVaoManager()->getDynamicBufferMultiplier() - 1u;
    }
    //-----------------------------------------------------------------------------------
    void D3D11WindowSwapChainBased::_initialize( TextureGpuManager *textureGpuManager )
    {
        _createSwapChain();

        D3D11TextureGpuManager *textureManager =
                static_cast<D3D11TextureGpuManager*>( textureGpuManager );

        mpBackBuffer.Reset();
        mpBackBufferNoMSAA.Reset();
        // Obtain back buffer from swapchain
        HRESULT hr = mSwapChain->GetBuffer( 0, __uuidof(ID3D11Texture2D), (void**)mpBackBuffer.ReleaseAndGetAddressOf() );

        if( FAILED(hr) )
        {
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Unable to Get Back Buffer for swap chain",
                            "D3D11WindowHwnd::_initialize" );
        }

        mTexture        = textureManager->createTextureGpuWindow( mpBackBuffer.Get(), this );
        mDepthBuffer    = textureManager->createWindowDepthBuffer();

        mTexture->setPixelFormat( _getRenderFormat() );
        mDepthBuffer->setPixelFormat( DepthBuffer::DefaultDepthBufferFormat );
        if( PixelFormatGpuUtils::isStencil( mDepthBuffer->getPixelFormat() ) )
            mStencilBuffer = mDepthBuffer;

        if( mDepthBuffer )
        {
            mTexture->_setDepthBufferDefaults( DepthBuffer::POOL_NON_SHAREABLE,
                                               false, mDepthBuffer->getPixelFormat() );
        }
        else
        {
            mTexture->_setDepthBufferDefaults( DepthBuffer::POOL_NO_DEPTH, false, PFG_NULL );
        }

        mTexture->setMsaa( mMsaaDesc.Count );
        mDepthBuffer->setMsaa( mMsaaDesc.Count );

#if TODO_OGRE_2_2
        mTexture->setMsaaPattern( );
        mDepthBuffer->setMsaaPattern( );
#endif
        setResolutionFromSwapChain();
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
    //-----------------------------------------------------------------------------------
    void D3D11WindowSwapChainBased::resizeSwapChainBuffers( uint32 width, uint32 height )
    {
        mRenderSystem->validateDevice();
        mRenderSystem->fireDeviceEvent( &mDevice, "WindowBeforeResize", this );

        mDepthBuffer->_transitionTo( GpuResidency::OnStorage, (uint8*)0 );
        mTexture->_transitionTo( GpuResidency::OnStorage, (uint8*)0 );
        mpBackBuffer.Reset();
        mpBackBufferNoMSAA.Reset();

        // Call flush before resize buffers to ensure destruction of resources.
        // not doing so may result in 'Out of memory' exception.
        mDevice.GetImmediateContext()->Flush();

        // width and height can be zero to autodetect size, therefore do not rely on them
        HRESULT hr = mSwapChain->ResizeBuffers( _getSwapChainBufferCount(), width, height,
                                                _getSwapChainFormat(), 0 );
        if(hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            mRenderSystem->handleDeviceLost();
            // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method 
            // and correctly set up the new device.
            return;
        }
        else if( FAILED(hr) )
        {
            const String errorDescription = mDevice.getErrorDescription(hr);
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Unable to resize swap chain\nError Description:" + errorDescription,
                            "D3D11WindowSwapChainBased::resizeSwapChainBuffers" );
        }

        // Obtain back buffer from swapchain
        hr = mSwapChain->GetBuffer( 0, __uuidof(ID3D11Texture2D), (void**)mpBackBuffer.ReleaseAndGetAddressOf() );

        if( FAILED(hr) )
        {
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Unable to Get Back Buffer for swap chain",
                            "D3D11WindowSwapChainBased::resizeSwapChainBuffers" );
        }

        setResolutionFromSwapChain();

        // Notify viewports of resize
        notifyResolutionChanged();
        mRenderSystem->fireDeviceEvent( &mDevice, "WindowResized", this );
    }
    //-----------------------------------------------------------------------------------
    void D3D11WindowSwapChainBased::setResolutionFromSwapChain()
    {
        if( mSwapChain1 )
        {
            DXGI_SWAP_CHAIN_DESC1 desc;
            mSwapChain1->GetDesc1( &desc );
            setFinalResolution( desc.Width, desc.Height );
            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsDesc;
            mSwapChain1->GetFullscreenDesc( &fsDesc );
            //Alt-Enter together with SetWindowAssociation() can change this state
            mRequestedFullscreenMode    = fsDesc.Windowed == 0;
            mFullscreenMode             = mRequestedFullscreenMode;
        }
        else
        {
            DXGI_SWAP_CHAIN_DESC desc;
            mSwapChain->GetDesc( &desc );
            setFinalResolution( desc.BufferDesc.Width, desc.BufferDesc.Height );
            //Alt-Enter together with SetWindowAssociation() can change this state
            mRequestedFullscreenMode    = desc.Windowed == 0;
            mFullscreenMode             = mRequestedFullscreenMode;
        }

        assert( mpBackBuffer && "Back buffer should've been obtained by now!" );

        assert( dynamic_cast<D3D11TextureGpuWindow*>( mTexture ) );
        D3D11TextureGpuWindow *texWindow = static_cast<D3D11TextureGpuWindow*>( mTexture );
        texWindow->_setBackbuffer( mpBackBuffer.Get() );

        mTexture->_transitionTo( GpuResidency::Resident, (uint8*)0 );
        mDepthBuffer->_transitionTo( GpuResidency::Resident, (uint8*)0 );
    }
    //-----------------------------------------------------------------------------------
    void D3D11WindowSwapChainBased::notifyResolutionChanged(void)
    {
        TODO_notify_listeners;
    }
    //-----------------------------------------------------------------------------------
    void D3D11WindowSwapChainBased::swapBuffers(void)
    {
        mRenderSystem->fireDeviceEvent( &mDevice,"BeforeDevicePresent",this );

        if( !mDevice.isNull() )
        {
#if TODO_OGRE_2_2
            //Step of resolving MSAA resource for swap chains in FlipSequentialMode
            //should be done by application rather than by OS.
            if( mUseFlipSequentialMode && getMsaa() > 1u )
            {
                //We can't resolve MSAA sRGB -> MSAA non-sRGB, so we need to have 2 textures:
                // 1. Render to MSAA sRGB
                // 2. Resolve MSAA sRGB -> sRGB regular tex.
                // 3. Copy sRGB regular texture to swap chain.
                ID3D11Texture2D* swapChainBackBuffer = NULL;
                HRESULT hr = mpSwapChain->GetBuffer( 0, __uuidof(ID3D11Texture2D),
                                                     (LPVOID*)&swapChainBackBuffer );
                if( FAILED(hr) )
                {
                    OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                                    "Error obtaining backbuffer",
                                    "D3D11WindowHwnd::swapBuffers" );
                }

                ID3D11DeviceContextN *context = mDevice.GetImmediateContext();

                if( !isHardwareGammaEnabled() )
                {
                    assert(_getRenderFormat() == _getSwapChainFormat());
                    context->ResolveSubresource( swapChainBackBuffer, 0, mpBackBuffer,
                                                 0, _getRenderFormat() );
                }
                else
                {
                    assert( mpBackBufferNoMSAA );
                    context->ResolveSubresource( mpBackBufferNoMSAA, 0, mpBackBuffer,
                                                 0, _getRenderFormat() );
                    context->CopyResource( swapChainBackBuffer, mpBackBufferNoMSAA );
                }

                SAFE_RELEASE(swapChainBackBuffer);
            }
#endif

            // flip presentation model swap chains have another semantic for first parameter
            UINT syncInterval = mUseFlipSequentialMode ? std::max( 1u, mVSyncInterval ) :
                                                         (mVSync ? mVSyncInterval : 0);
            HRESULT hr = mSwapChain->Present( syncInterval, 0 );
            if( FAILED(hr) )
            {
                OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                                "Error Presenting surfaces",
                                "D3D11WindowHwnd::swapBuffers");
            }
        }
    }
}
