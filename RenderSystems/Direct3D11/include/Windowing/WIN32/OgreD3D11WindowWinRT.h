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

#ifndef _OgreD3D11WindowWinRT_H_
#define _OgreD3D11WindowWinRT_H_

#include "OgreD3D11Window.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT && defined( __cplusplus_winrt )
#    include <agile.h>
#elif OGRE_PLATFORM == OGRE_PLATFORM_WINRT && !defined( __cplusplus_winrt )
#    include <winrt/Windows.UI.Core.h>
#    include <winrt/Windows.UI.Xaml.Controls.h>
#endif

namespace Ogre
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT
    class D3D11WindowCoreWindow : public D3D11WindowSwapChainBased
    {
    protected:
#    if defined( __cplusplus_winrt )
        Platform::Agile<Windows::UI::Core::CoreWindow> mCoreWindow;
#    else
        winrt::agile_ref<winrt::Windows::UI::Core::CoreWindow> mCoreWindow;
#    endif

    protected:
        virtual HRESULT _createSwapChainImpl();

    public:
        D3D11WindowCoreWindow( const String &title, uint32 width, uint32 height, bool fullscreenMode,
                               PixelFormatGpu depthStencilFormat, const NameValuePairList *miscParams,
                               D3D11Device &device, D3D11RenderSystem *renderSystem );
        virtual ~D3D11WindowCoreWindow();
        virtual void destroy();

#    if defined( __cplusplus_winrt )
        Windows::UI::Core::CoreWindow ^ getCoreWindow() const { return mCoreWindow.Get(); }
#    else
        winrt::Windows::UI::Core::CoreWindow getCoreWindow() const { return mCoreWindow.get(); }
#    endif

        virtual float getViewPointToPixelScale() const;
        virtual void  windowMovedOrResized();
        virtual bool  isVisible() const;
    };
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT && defined( _WIN32_WINNT_WINBLUE ) && \
    _WIN32_WINNT >= _WIN32_WINNT_WINBLUE
    class D3D11WindowSwapChainPanel : public D3D11WindowSwapChainBased
    {
    protected:
#    if defined( __cplusplus_winrt )
        Windows::UI::Xaml::Controls::SwapChainPanel ^ mSwapChainPanel;
        Windows::Foundation::Size                   mCompositionScale;
        Windows::Foundation::EventRegistrationToken sizeChangedToken, compositionScaleChangedToken;
#    else
        winrt::Windows::UI::Xaml::Controls::SwapChainPanel mSwapChainPanel{ nullptr };
        winrt::Windows::Foundation::Size                   mCompositionScale;
        winrt::event_token sizeChangedToken, compositionScaleChangedToken;
#    endif

    protected:
        virtual HRESULT _createSwapChainImpl();
        virtual void    _destroySwapChain();
        HRESULT         _compensateSwapChainCompositionScale();

    public:
        D3D11WindowSwapChainPanel( const String &title, uint32 width, uint32 height, bool fullscreenMode,
                                   PixelFormatGpu           depthStencilFormat,
                                   const NameValuePairList *miscParams, D3D11Device &device,
                                   D3D11RenderSystem *renderSystem );
        virtual ~D3D11WindowSwapChainPanel();
        virtual void destroy();

#    if defined( __cplusplus_winrt )
        Windows::UI::Xaml::Controls::SwapChainPanel ^ getSwapChainPanel() const
#    else
        winrt::Windows::UI::Xaml::Controls::SwapChainPanel getSwapChainPanel() const
#    endif
        {
            return mSwapChainPanel;
        }

        virtual float getViewPointToPixelScale() const;
        virtual void  windowMovedOrResized();
        virtual bool  isVisible() const;
    };
#endif
}  // namespace Ogre
#endif
