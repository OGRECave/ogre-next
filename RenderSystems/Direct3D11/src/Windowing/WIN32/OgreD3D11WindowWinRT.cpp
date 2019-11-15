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

namespace Ogre
{
#pragma region D3D11WindowCoreWindow
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT
    D3D11WindowCoreWindow::D3D11WindowCoreWindow( const String &title, uint32 width, uint32 height,
                                      bool fullscreenMode, PixelFormatGpu depthStencilFormat,
                                      const NameValuePairList *miscParams,
                                      D3D11Device &device, D3D11RenderSystem *renderSystem ) :
        D3D11WindowSwapChainBased( title, width, height, fullscreenMode, depthStencilFormat,
                                   miscParams, device, renderSystem )
    {
        mUseFlipSequentialMode = true;
    }
    //-----------------------------------------------------------------------------------
    D3D11WindowCoreWindow::~D3D11WindowCoreWindow()
    {
        destroy();
    }
    //-----------------------------------------------------------------------------------
    float D3D11WindowCoreWindow::getViewPointToPixelScale() const
    {
#if _WIN32_WINNT > _WIN32_WINNT_WIN8
        return Windows::Graphics::Display::DisplayInformation::GetForCurrentView()->LogicalDpi / 96;
#else
        return Windows::Graphics::Display::DisplayProperties::LogicalDpi / 96;
#endif
    }

#endif
#pragma endregion

#pragma region D3D11WindowSwapChainPanel
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT && defined(_WIN32_WINNT_WINBLUE) && _WIN32_WINNT >= _WIN32_WINNT_WINBLUE
    D3D11WindowSwapChainPanel::D3D11WindowSwapChainPanel( const String &title, uint32 width, uint32 height,
                                      bool fullscreenMode, PixelFormatGpu depthStencilFormat,
                                      const NameValuePairList *miscParams,
                                      D3D11Device &device, D3D11RenderSystem *renderSystem ) :
        D3D11WindowSwapChainBased( title, width, height, fullscreenMode, depthStencilFormat,
                                   miscParams, device, renderSystem ),
        mCompositionScale(1.0f, 1.0f)

    {
        mUseFlipSequentialMode = true;
    }
    //-----------------------------------------------------------------------------------
    D3D11WindowSwapChainPanel::~D3D11WindowSwapChainPanel()
    {
        destroy();
    }
    //-----------------------------------------------------------------------------------
    float D3D11WindowSwapChainPanel::getViewPointToPixelScale() const
    {
        return std::max(mCompositionScale.Width, mCompositionScale.Height);
    }
#endif
#pragma endregion
}
