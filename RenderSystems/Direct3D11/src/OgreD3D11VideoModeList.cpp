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
#include "OgreD3D11VideoModeList.h"

#include "OgreD3D11Driver.h"
#include "OgreD3D11VideoMode.h"
#include "OgreException.h"

namespace Ogre
{
    //---------------------------------------------------------------------
    D3D11VideoModeList::D3D11VideoModeList( IDXGIAdapterN *pAdapter )
    {
        if( NULL == pAdapter )
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "pAdapter parameter is NULL",
                         "D3D11VideoModeList::D3D11VideoModeList" );

        for( int iOutput = 0;; ++iOutput )
        {
            // AIZTODO: one output for a single monitor ,to be handled for mulimon
            ComPtr<IDXGIOutput> pOutput;
            HRESULT hr = pAdapter->EnumOutputs( iOutput, pOutput.ReleaseAndGetAddressOf() );
            if( DXGI_ERROR_NOT_FOUND == hr )
                break;
            else if( FAILED( hr ) )
            {
                break;  // Something bad happened.
            }
            else  // Success!
            {
                DXGI_OUTPUT_DESC OutputDesc;
                pOutput->GetDesc( &OutputDesc );

                UINT NumModes = 0;
                hr = pOutput->GetDisplayModeList( DXGI_FORMAT_R8G8B8A8_UNORM, 0, &NumModes, NULL );

                // If working over a terminal session, for example using the Simulator for
                // deployment/development, display modes cannot be obtained.
                if( hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE )
                {
                    DXGI_MODE_DESC fullScreenMode;
                    fullScreenMode.Width =
                        OutputDesc.DesktopCoordinates.right - OutputDesc.DesktopCoordinates.left;
                    fullScreenMode.Height =
                        OutputDesc.DesktopCoordinates.bottom - OutputDesc.DesktopCoordinates.top;
                    fullScreenMode.RefreshRate.Numerator = 60;
                    fullScreenMode.RefreshRate.Denominator = 1;
                    fullScreenMode.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    fullScreenMode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
                    fullScreenMode.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

                    mModeList.push_back( D3D11VideoMode( OutputDesc, fullScreenMode ) );
                }
                else if( hr == S_OK )
                {
                    if( NumModes > 0 )
                    {
                        // Create an array to store Display Mode information
                        vector<DXGI_MODE_DESC>::type modes;
                        modes.resize( NumModes );

                        // Populate our array with information
                        hr = pOutput->GetDisplayModeList( DXGI_FORMAT_R8G8B8A8_UNORM, 0, &NumModes,
                                                          modes.empty() ? 0 : &modes[0] );

                        for( UINT m = 0; m < NumModes; m++ )
                        {
                            DXGI_MODE_DESC displayMode = modes[m];
                            // Filter out low-resolutions
                            if( displayMode.Width < 640 || displayMode.Height < 400 )
                                continue;

                            // Check to see if it is already in the list (to filter out refresh rates)
                            BOOL found = FALSE;
                            vector<D3D11VideoMode>::type::iterator it;
                            for( it = mModeList.begin(); it != mModeList.end(); it++ )
                            {
                                DXGI_OUTPUT_DESC oldOutput = it->getDisplayMode();
                                DXGI_MODE_DESC oldDisp = it->getModeDesc();
                                if(  // oldOutput.Monitor==OutputDesc.Monitor &&
                                    oldDisp.Width == displayMode.Width &&
                                    oldDisp.Height == displayMode.Height  // &&
                                    // oldDisp.Format == displayMode.Format
                                )
                                {
                                    // Check refresh rate and favour higher if poss
                                    // if (oldDisp.RefreshRate < displayMode.RefreshRate)
                                    //  it->increaseRefreshRate(displayMode.RefreshRate);
                                    found = TRUE;
                                    break;
                                }
                            }

                            if( !found )
                                mModeList.push_back( D3D11VideoMode( OutputDesc, displayMode ) );
                        }
                    }
                }
            }
        }
    }
    //---------------------------------------------------------------------
    size_t D3D11VideoModeList::count() { return mModeList.size(); }
    //---------------------------------------------------------------------
    D3D11VideoMode *D3D11VideoModeList::item( size_t index )
    {
        vector<D3D11VideoMode>::type::iterator p = mModeList.begin();

        return &p[index];
    }
    //---------------------------------------------------------------------
    D3D11VideoMode *D3D11VideoModeList::item( const String &name )
    {
        vector<D3D11VideoMode>::type::iterator it = mModeList.begin();
        if( it == mModeList.end() )
            return NULL;

        for( ; it != mModeList.end(); ++it )
        {
            if( it->getDescription() == name )
                return &( *it );
        }

        return NULL;
    }
    //---------------------------------------------------------------------
}  // namespace Ogre
