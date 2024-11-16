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

#ifndef _OgreD3D11Window_H_
#define _OgreD3D11Window_H_

#include "OgreD3D11Prerequisites.h"

#include "OgreCommon.h"
#include "OgreD3D11DeviceResource.h"
#include "OgreWindow.h"

namespace Ogre
{
    class _OgreD3D11Export D3D11Window : public Window, protected D3D11DeviceResource
    {
    protected:
        D3D11Device &mDevice;
        /// Whether window was not created by Ogre
        bool mIsExternal;
        bool mSizing;
        bool mClosed;
        bool mHidden;
        bool mAlwaysWindowedMode;
        bool mHwGamma;
        bool mVisible;

        // Window size depended resources - must be released
        // before swapchain resize and recreated later
        ComPtr<ID3D11Texture2D> mpBackBuffer;
        /// Optional, always holds up-to-date copy data from mpBackBuffer if not NULL
        ComPtr<ID3D11Texture2D> mpBackBufferInterim;

        D3D11RenderSystem *mRenderSystem;

    protected:
        virtual PixelFormatGpu _getRenderFormat()
        {
            return mHwGamma ? PFG_BGRA8_UNORM_SRGB : PFG_BGRA8_UNORM;
        }  // preferred since Win8

    public:
        D3D11Window( const String &title, uint32 width, uint32 height, bool fullscreenMode,
                     PixelFormatGpu depthStencilFormat, const NameValuePairList *miscParams,
                     D3D11Device &device, D3D11RenderSystem *renderSystem );
        ~D3D11Window() override;
        void destroy() override;

        void reposition( int leftPt, int topPt ) override {}

        bool isClosed() const override { return mClosed; }
        void _setVisible( bool visible ) override { mVisible = visible; }
        void setHidden( bool hidden ) override { mHidden = hidden; }
        bool isHidden() const override { return mHidden; }

        void getCustomAttribute( IdString name, void *pData ) override;
    };

    class _OgreD3D11Export D3D11WindowSwapChainBased : public D3D11Window
    {
    protected:
        ComPtr<IDXGISwapChain>  mSwapChain;
        ComPtr<IDXGISwapChain1> mSwapChain1;
        // DXGI_SWAP_CHAIN_DESC_N  mSwapChainDesc;

        /// Flag to determine if the swapchain flip model is active.
        /// Not supported before Win8.0, required for WinRT.
        bool mUseFlipMode;

        // We save the previous present stats - so we can detect a "vblank miss"
        DXGI_FRAME_STATISTICS mPreviousPresentStats;
        // Does mLastPresentStats data is valid (it isn't if when you start or resize the window)
        bool mPreviousPresentStatsIsValid;
        // Number of times we missed the v sync blank
        uint32 mVBlankMissCount;

    protected:
        DXGI_FORMAT          _getSwapChainFormat();
        DXGI_SWAP_CHAIN_FLAG _getSwapChainFlags();
        uint8                _getSwapChainBufferCount() const;

        void            _createSwapChain();
        virtual HRESULT _createSwapChainImpl() = 0;
        virtual void    _destroySwapChain();

        void _createSizeDependedD3DResources();
        void _destroySizeDependedD3DResources();

        void resizeSwapChainBuffers( uint32 width, uint32 height );
        void notifyResolutionChanged();
        void notifyDeviceLost( D3D11Device *device ) override;
        void notifyDeviceRestored( D3D11Device *device, unsigned pass ) override;

    public:
        D3D11WindowSwapChainBased( const String &title, uint32 width, uint32 height, bool fullscreenMode,
                                   PixelFormatGpu           depthStencilFormat,
                                   const NameValuePairList *miscParams, D3D11Device &device,
                                   D3D11RenderSystem *renderSystem );
        ~D3D11WindowSwapChainBased() override;

        void _initialize( TextureGpuManager                     *textureGpuManager,
                          const NameValuePairList *ogre_nullable miscParams ) override;
        void destroy() override;

        /// @copydoc Window::setFsaa
        void setFsaa( const String &fsaa ) override;

        void swapBuffers() override;
    };
}  // namespace Ogre

#include "Windowing/WIN32/OgreD3D11WindowHwnd.h"
#include "Windowing/WIN32/OgreD3D11WindowWinRT.h"

#endif
