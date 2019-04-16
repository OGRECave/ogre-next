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

#include "VendorExtensions/OgreD3D11AmdExtension.h"

#if !OGRE_NO_AMD_AGS

#include "OgreLwString.h"
#include "OgreLogManager.h"

#include "OgreUTFString.h"

namespace Ogre
{
    D3D11AmdExtension::D3D11AmdExtension() :
        mAgsContext( 0 )
    {
        memset( &mGpuInfo, 0, sizeof(mGpuInfo) );
        AGSReturnCode result = agsInit( &mAgsContext, NULL, &mGpuInfo );

        if( result != AGS_SUCCESS )
        {
            char tmpBuffer[256];
            LwString errorText( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );
            errorText.a( "Could not initialize AMD AGS extension error: ", result,
                         "\nAMD extensions won't be available." );
            LogManager::getSingleton().logMessage( errorText.c_str(), LML_CRITICAL );
        }
        else
        {
            dumpAgsInfo( mGpuInfo );
        }
    }
    //-------------------------------------------------------------------------
    D3D11AmdExtension::~D3D11AmdExtension()
    {
        if( mAgsContext )
        {
            LogManager::getSingleton().logMessage( "Deinitializing AMD AGS extensions" );
            agsDeInit( mAgsContext );
            mAgsContext = 0;
        }
    }
    //-------------------------------------------------------------------------
    void D3D11AmdExtension::dumpAgsInfo( const AGSGPUInfo &gpuInfo )
    {
        LogManager &logManager = LogManager::getSingleton();

        logManager.logMessage( "AMD AGS extensions initialized" );

        char tmpBuffer[1024];
        LwString infoText( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

        infoText.a( "AGS Version: ", gpuInfo.agsVersionMajor, ".",
                    gpuInfo.agsVersionMinor, ".", gpuInfo.agsVersionPatch );
        infoText.a( "\nWACK compliant (UWP): ", gpuInfo.isWACKCompliant ? "Yes" : "No" );
        infoText.a( "\nNum devices: ", gpuInfo.numDevices );
        logManager.logMessage( infoText.c_str() );

        infoText.clear();
        infoText.a( "Driver version: ", gpuInfo.driverVersion );
        logManager.logMessage( infoText.c_str() );

        infoText.clear();
        infoText.a( "Radeon SW version: ", gpuInfo.radeonSoftwareVersion );
        logManager.logMessage( infoText.c_str() );
    }
    //-------------------------------------------------------------------------
    bool D3D11AmdExtension::recommendsAgs( IDXGIAdapter *adapter )
    {
        LogManager &logManager = LogManager::getSingleton();
        logManager.logMessage( "Querying AMD AGS extensions" );

        if( adapter )
        {
            DXGI_ADAPTER_DESC adapterDesc;
            HRESULT hr = adapter->GetDesc( &adapterDesc );
            if( SUCCEEDED( hr ) )
            {
                if( adapterDesc.VendorId != 0x1002 )
                {
                    logManager.logMessage( "Requested adapter is not from AMD. Not recommending AGS" );
                    return false;
                }
            }
        }

        AGSContext *agsContext;
        AGSGPUInfo gpuInfo;
        AGSReturnCode result = agsInit( &agsContext, NULL, &gpuInfo );

        if( result != AGS_SUCCESS )
            return false;

        dumpAgsInfo( gpuInfo );

        bool recommended = false;
        for( int deviceIdx=0; deviceIdx<gpuInfo.numDevices && !recommended; ++deviceIdx )
        {
            if( gpuInfo.devices[deviceIdx].vendorId == 0x1002 )
                recommended |= gpuInfo.devices[deviceIdx].vendorId == 0x1002;
        }

        if( recommended )
        {
            unsigned int minAgsVersion = AGS_MAKE_VERSION( 18, 8, 2 );
            AGSDriverVersionResult verResult = agsCheckDriverVersion( gpuInfo.radeonSoftwareVersion,
                                                                      minAgsVersion );
            if( verResult == AGS_SOFTWAREVERSIONCHECK_OLDER )
            {
                recommended = false;
                int major = (minAgsVersion & 0xFFC00000) >> 22;
                int minor = (minAgsVersion & 0x003FF000) >> 12;
                int patch = (minAgsVersion & 0x00000FFF);

                char tmpBuffer[256];
                LwString errorText( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );
                errorText.a( "Minimum recommended AMD driver version for AGS is ",
                             major, ".", minor, ".", patch );
                logManager.logMessage( errorText.c_str() );
            }
            else if( verResult == AGS_SOFTWAREVERSIONCHECK_UNDEFINED )
            {
                recommended = false;
                logManager.logMessage( "Could not properly determine AMD driver version" );
            }
        }

        if( recommended )
            logManager.logMessage( "AMD AGS extensions: Recommended" );
        else
            logManager.logMessage( "AMD AGS extensions: Not recommended" );

        LogManager::getSingleton().logMessage( "Deinitializing AMD AGS extensions" );
        agsDeInit( agsContext );

        return recommended;
    }
    //-------------------------------------------------------------------------
    HRESULT D3D11AmdExtension::createDeviceImpl( const String &appName,
                                                 IDXGIAdapter *adapter, D3D_DRIVER_TYPE driverType,
                                                 UINT deviceFlags, D3D_FEATURE_LEVEL *pFirstFL,
                                                 UINT numFeatureLevels,
                                                 D3D_FEATURE_LEVEL *outFeatureLevel,
                                                 ID3D11Device **outDevice )
    {
        bool adapterNotAmd = false;
        if( mAgsContext )
        {
            if( adapter )
            {
                DXGI_ADAPTER_DESC adapterDesc;
                HRESULT hr = adapter->GetDesc( &adapterDesc );
                if( SUCCEEDED( hr ) )
                    adapterNotAmd = adapterDesc.VendorId != 0x1002;
            }
            else
            {
                adapterNotAmd = mGpuInfo.devices[0].vendorId != 0x1002;
            }
        }

        if( !mAgsContext || adapterNotAmd )
        {
            if( adapterNotAmd )
            {
                LogManager::getSingleton().logMessage( "AMD AGS extensions enabled but requested "
                                                       "Adapter is not from AMD!", LML_CRITICAL );
            }
            return D3D11VendorExtension::createDeviceImpl( appName, adapter, driverType, deviceFlags,
                                                           pFirstFL, numFeatureLevels, outFeatureLevel,
                                                           outDevice );
        }

        AGSDX11DeviceCreationParams creationParams =
        {
            adapter,
            driverType,
            NULL,
            deviceFlags,
            pFirstFL,
            numFeatureLevels,
            D3D11_SDK_VERSION,
            NULL
        };

        UTFString wText( appName );
        AGSDX11ExtensionParams extensionParams;
        memset( &extensionParams, 0, sizeof(extensionParams) );
        extensionParams.pAppName = wText.asWStr_c_str();
        extensionParams.pEngineName = L"Ogre3D D3D11 Engine";

        AGSDX11ReturnedParams returnedParams;
        memset( &returnedParams, 0, sizeof(returnedParams) );

        AGSReturnCode result = agsDriverExtensionsDX11_CreateDevice( mAgsContext, &creationParams,
                                                                     &extensionParams, &returnedParams );

        HRESULT hr = -1;

        if( result == AGS_DX_FAILURE )
            hr = E_INVALIDARG; //Tell parent class to try D3D11.0 instead of 11.1
        else if( result != AGS_SUCCESS )
        {
            char tmpBuffer[256];
            LwString errorText( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );
            errorText.a( "Could not initialize AMD AGS extension error: ", result,
                         "\nFalling back to no extension." );
            LogManager::getSingleton().logMessage( errorText.c_str(), LML_CRITICAL );
            hr = D3D11VendorExtension::createDeviceImpl( appName, adapter, driverType, deviceFlags,
                                                         pFirstFL, numFeatureLevels, outFeatureLevel,
                                                         outDevice );
        }
        else
        {
            mReturnedParams.push_back( returnedParams );

            *outDevice = returnedParams.pDevice;
            *outFeatureLevel = returnedParams.FeatureLevel;

            char tmpBuffer[256];
            LwString infoText( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );
            infoText.a( "D3D11 AMD AGS device created. Extensions supported: ",
                        returnedParams.extensionsSupported );
            LogManager::getSingleton().logMessage( infoText.c_str() );

            hr = S_OK;
        }

        return hr;
    }
    //-------------------------------------------------------------------------
    void D3D11AmdExtension::destroyDevice( ID3D11Device *device )
    {
        if( !device )
            return;

        FastArray<AGSDX11ReturnedParams>::iterator itor = mReturnedParams.begin();
        FastArray<AGSDX11ReturnedParams>::iterator end  = mReturnedParams.end();

        while( itor != end && itor->pDevice != device )
            ++itor;

        if( itor == end )
        {
            //Device wasn't created by us (or corrupted, or double free)
            D3D11VendorExtension::destroyDevice( device );
        }
        else
        {
            agsDriverExtensionsDX11_DestroyDevice( mAgsContext, itor->pDevice, NULL,
                                                   itor->pImmediateContext, NULL );
            efficientVectorRemove( mReturnedParams, itor );
        }
    }
}

#endif
