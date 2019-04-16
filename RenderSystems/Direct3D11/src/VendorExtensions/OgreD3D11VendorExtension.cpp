/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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

#include "OgreStableHeaders.h"

#include "VendorExtensions/OgreD3D11VendorExtension.h"
#include "VendorExtensions/OgreD3D11AmdExtension.h"

#include "OgreLwString.h"
#include "OgreLogManager.h"

namespace Ogre
{
    D3D11VendorExtension::D3D11VendorExtension()
    {
    }
    //-------------------------------------------------------------------------
    D3D11VendorExtension::~D3D11VendorExtension()
    {
    }
    //-------------------------------------------------------------------------
    D3D11VendorExtension* D3D11VendorExtension::initializeExtension( GPUVendor preferredVendor,
                                                                     IDXGIAdapter *adapter )
    {
        D3D11VendorExtension *retVal = 0;
        switch( preferredVendor )
        {
        case GPU_UNKNOWN:
            if( D3D11AmdExtension::recommendsAgs( adapter ) )
                retVal = new D3D11AmdExtension();
            else
                retVal = new D3D11VendorExtension();
            break;
        case GPU_AMD:
            retVal = new D3D11AmdExtension();
            break;
        default:
            retVal = new D3D11VendorExtension();
            break;
        }
        return retVal;
    }
    //-------------------------------------------------------------------------
    HRESULT D3D11VendorExtension::createDeviceImpl( const String &appName,
                                                    IDXGIAdapter *adapter, D3D_DRIVER_TYPE driverType,
                                                    UINT deviceFlags, D3D_FEATURE_LEVEL *pFirstFL,
                                                    UINT numFeatureLevels,
                                                    D3D_FEATURE_LEVEL *outFeatureLevel,
                                                    ID3D11Device **outDevice )
    {
        HRESULT hr = D3D11CreateDevice( adapter, driverType, NULL, deviceFlags, pFirstFL,
                                        numFeatureLevels, D3D11_SDK_VERSION, outDevice,
                                        outFeatureLevel, NULL );

        if( FAILED( hr ) )
        {
            char tmpBuffer[256];
            LwString errorText( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );
            errorText.a( "Failed to create Direct3D device HRESULT(", (uint32)hr, ")" );
            LogManager::getSingleton().logMessage( errorText.c_str() );
        }

        return hr;
    }
    //-------------------------------------------------------------------------
    void D3D11VendorExtension::createDevice( const String &appName,
                                             IDXGIAdapter *adapter, D3D_DRIVER_TYPE driverType,
                                             UINT deviceFlags, D3D_FEATURE_LEVEL *pFirstFL,
                                             UINT numFeatureLevels, D3D_FEATURE_LEVEL *outFeatureLevel,
                                             ID3D11Device **outDevice, ID3D11Device1 **outDevice1 )
    {
        ID3D11Device *device = NULL;
        HRESULT hr = createDeviceImpl( appName, adapter, driverType, deviceFlags, pFirstFL,
                                       numFeatureLevels, outFeatureLevel, &device );

        if( FAILED( hr ) && 0 != (deviceFlags & D3D11_CREATE_DEVICE_DEBUG) )
        {
            LogManager::getSingleton().logMessage( "Failed to create Direct3D11 device with debug "
                                                   "layer\nRetrying without debug layer." );

            // create device - second attempt, without debug layer
            const UINT deviceFlagsCopy = deviceFlags & ~D3D11_CREATE_DEVICE_DEBUG;
            hr = createDeviceImpl( appName, adapter, driverType, deviceFlagsCopy, pFirstFL,
                                   numFeatureLevels, outFeatureLevel, &device );
        }

#if defined( _WIN32_WINNT_WIN8 )
        if( FAILED( hr ) && *pFirstFL == D3D_FEATURE_LEVEL_11_1 )
        {
            LogManager::getSingleton().logMessage( "Failed to create Direct3D 11.1 device\n"
                                                   "Retrying asking for 11.0 device" );

            pFirstFL= pFirstFL + 1u;
            --numFeatureLevels;
            if( numFeatureLevels == 0u )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "Direct3D 11.1 not supported. If you're on Windows Vista or Windows 7, "
                             "try installing the KB2670838 update",
                             "D3D11VendorExtension::createDevice" );
            }

            // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1
            // so we need to retry without it
            hr = createDeviceImpl( appName, adapter, driverType, deviceFlags, pFirstFL,
                                   numFeatureLevels, outFeatureLevel, &device );
        }
#endif
        if( FAILED( hr ) )
        {
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Failed to create Direct3D11 device",
                            "D3D11VendorExtension::createDevice" );
        }

        *outDevice = device;
        *outDevice1 = 0;

#if defined(_WIN32_WINNT_WIN8)
        hr = device->QueryInterface( __uuidof(ID3D11Device1),
                                     reinterpret_cast<void**>(outDevice1) );
#endif
    }
    //-------------------------------------------------------------------------
    void D3D11VendorExtension::destroyDevice( ID3D11Device *device )
    {
        if( device )
            device->Release();
    }
}
