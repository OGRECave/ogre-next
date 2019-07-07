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
        mpBackBuffer( 0 ),
        mpBackBufferNoMSAA( 0 ),
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
            {
                mMsaaDesc.Count = StringConverter::parseUnsignedInt( opt->second, 1u );
//                const bool useCSAA = opt->second.find( "[Quality]" ) != String::npos;
//                if( useCSAA )
//                    mMsaaDesc.Quality = mMsaaDesc.Count;
            }
            opt = miscParams->find("MSAA_quality");
            if( opt != miscParams->end() )
            {
                mMsaaDesc.Quality = StringConverter::parseUnsignedInt(opt->second);
                TODO_convert_to_MSAA_pattern;
            }
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
        mSwapChain( 0 ),
        mSwapChain1( 0 ),
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
}
