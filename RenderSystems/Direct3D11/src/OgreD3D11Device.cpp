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
#include "OgreD3D11Device.h"

#include "OgreException.h"

// #include <dxgi1_3.h> // for DXGIGetDebugInterface1
// #include <dxgidebug.h> // for IDXGIDebug1

#include "ogrestd/vector.h"

namespace Ogre
{
    //---------------------------------------------------------------------
    D3D11Device::eExceptionsErrorLevel D3D11Device::mExceptionsErrorLevel =
        D3D11Device::D3D_NO_EXCEPTION;
    //---------------------------------------------------------------------
    D3D11Device::D3D11Device() { mDriverVersion.QuadPart = 0; }
    //---------------------------------------------------------------------
    D3D11Device::~D3D11Device() { ReleaseAll(); }
    //---------------------------------------------------------------------
    void D3D11Device::ReleaseAll()
    {
        // Clear state
        if( mImmediateContext )
        {
            mImmediateContext->ClearState();
            mImmediateContext->Flush();
        }
#if OGRE_D3D11_PROFILING || OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        mPerf.Reset();
#endif
        mInfoQueue.Reset();
        mClassLinkage.Reset();
        mImmediateContext.Reset();
        mImmediateContext1.Reset();

        ComPtr<ID3D11Debug> d3dDebug;
        if( mD3D11Device )
            mD3D11Device->QueryInterface( d3dDebug.GetAddressOf() );

        mD3D11Device.Reset();
        mD3D11Device1.Reset();
        mDXGIFactory.Reset();
        mDXGIFactory2.Reset();
        mDriverVersion.QuadPart = 0;

        // It is normal to get live ID3D11Device with ref count 2, all are gone after d3dDebug.Reset()
        // The commented out code below is available since Win8.1 and will not report current
        // ID3D11Device as live, but it will notice and report other live devices, for example in our
        // drivers list
        if( d3dDebug )
        {
            d3dDebug->ReportLiveDeviceObjects( D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL |
                                               D3D11_RLDO_IGNORE_INTERNAL );
            d3dDebug.Reset();
        }

        // ComPtr<IDXGIDebug1> dxgiDebug;
        // if (SUCCEEDED(DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1),
        // (void**)dxgiDebug.GetAddressOf())))
        //    dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY |
        //    DXGI_DEBUG_RLO_IGNORE_INTERNAL));
    }
    //---------------------------------------------------------------------
    void D3D11Device::TransferOwnership( ComPtr<ID3D11Device> &d3d11device )
    {
        assert( mD3D11Device.Get() != d3d11device.Get() );
        assert( mD3D11Device1.Get() != d3d11device.Get() );
        ReleaseAll();

        if( d3d11device )
        {
            HRESULT hr = S_OK;

            d3d11device.As( &mD3D11Device );
#if defined( _WIN32_WINNT_WIN8 )
            d3d11device.As( &mD3D11Device1 );
#endif

            // get DXGI factory from device
            ComPtr<IDXGIDeviceN> pDXGIDevice;
            ComPtr<IDXGIAdapterN> pDXGIAdapter;
            if( SUCCEEDED( mD3D11Device.As( &pDXGIDevice ) ) &&
                SUCCEEDED( pDXGIDevice->GetParent( __uuidof( IDXGIAdapterN ),
                                                   (void **)pDXGIAdapter.GetAddressOf() ) ) )
            {
                pDXGIAdapter->GetParent( __uuidof( IDXGIFactoryN ),
                                         (void **)mDXGIFactory.ReleaseAndGetAddressOf() );

                // We intentionally check for ID3D10Device support instead of ID3D11Device as
                // CheckInterfaceSupport() is not supported for later.
                // We hope, that there would be one UMD for both D3D10 and D3D11, or two different
                // but with the same version number, or with different but correlated version numbers,
                // so that blacklisting could be done with high confidence level.
                if( FAILED( pDXGIAdapter->CheckInterfaceSupport(
                        IID_ID3D10Device,  // intentionally D3D10, not D3D11
                        &mDriverVersion ) ) )
                {
                    mDriverVersion.QuadPart = 0;
                }
            }

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            mD3D11Device->GetImmediateContext( mImmediateContext.ReleaseAndGetAddressOf() );
#elif OGRE_PLATFORM == OGRE_PLATFORM_WINRT
            mD3D11Device->GetImmediateContext1( mImmediateContext.ReleaseAndGetAddressOf() );
#endif
            if( mD3D11Device1 )
                mD3D11Device1->GetImmediateContext1( mImmediateContext1.ReleaseAndGetAddressOf() );

#if OGRE_D3D11_PROFILING || OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
            hr = mImmediateContext.As( &mPerf );
            if( FAILED( hr ) || !mPerf->GetStatus() )
                mPerf.Reset();
#endif

            hr = mD3D11Device.As( &mInfoQueue );
            if( SUCCEEDED( hr ) )
            {
                mInfoQueue->ClearStoredMessages();
                mInfoQueue->ClearRetrievalFilter();
                mInfoQueue->ClearStorageFilter();

                D3D11_INFO_QUEUE_FILTER filter;
                ZeroMemory( &filter, sizeof( D3D11_INFO_QUEUE_FILTER ) );
                vector<D3D11_MESSAGE_SEVERITY>::type severityList;

                switch( mExceptionsErrorLevel )
                {
                case D3D_NO_EXCEPTION:
                    severityList.push_back( D3D11_MESSAGE_SEVERITY_CORRUPTION );
                    // fallthrough
                case D3D_CORRUPTION:
                    severityList.push_back( D3D11_MESSAGE_SEVERITY_ERROR );
                    // fallthrough
                case D3D_ERROR:
                    severityList.push_back( D3D11_MESSAGE_SEVERITY_WARNING );
                    // fallthrough
                case D3D_WARNING:
                case D3D_INFO:
                    severityList.push_back( D3D11_MESSAGE_SEVERITY_INFO );
                default:
                    break;
                }

                if( severityList.size() > 0 )
                {
                    filter.DenyList.NumSeverities = static_cast<UINT>( severityList.size() );
                    filter.DenyList.pSeverityList = &severityList[0];
                }

                mInfoQueue->AddStorageFilterEntries( &filter );
                mInfoQueue->AddRetrievalFilterEntries( &filter );
            }

            // If feature level is 11, create class linkage
            if( mD3D11Device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0 )
            {
                hr = mD3D11Device->CreateClassLinkage( mClassLinkage.ReleaseAndGetAddressOf() );
            }
        }
    }
    //---------------------------------------------------------------------
    void D3D11Device::throwIfFailed( HRESULT hr, const char *desc, const char *src )
    {
        if( FAILED( hr ) || isError() )
        {
            String description =
                std::string( desc ).append( "\nError Description:" ).append( getErrorDescription( hr ) );
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr, description, src );
        }
    }
    //---------------------------------------------------------------------
    String D3D11Device::getErrorDescription( const HRESULT lastResult /* = NO_ERROR */ ) const
    {
        if( !mD3D11Device )
        {
            return "NULL device";
        }

        if( D3D_NO_EXCEPTION == mExceptionsErrorLevel )
        {
            return "infoQ exceptions are turned off";
        }

        String res;

        switch( lastResult )
        {
        case NO_ERROR:
            break;
        case E_INVALIDARG:
            res.append( "invalid parameters were passed.\n" );
            break;
        case DXGI_ERROR_DEVICE_REMOVED:
        {
            HRESULT deviceRemovedReason = mD3D11Device->GetDeviceRemovedReason();
            char tmp[64];
            sprintf( tmp, "deviceRemovedReason = 0x%08X\n", (unsigned)deviceRemovedReason );
            res.append( tmp );
            OGRE_FALLTHROUGH;
        }
        default:
        {
            char tmp[64];
            sprintf( tmp, "hr = 0x%08X\n", (unsigned)lastResult );
            res.append( tmp );
        }
        }

        if( mInfoQueue )
        {
            UINT64 numStoredMessages = mInfoQueue->GetNumStoredMessages();
            for( UINT64 i = 0; i < numStoredMessages; i++ )
            {
                // Get the size of the message
                SIZE_T messageLength = 0;
                mInfoQueue->GetMessage( i, NULL, &messageLength );
                if( messageLength > 0u )
                {
                    // Allocate space and get the message
                    D3D11_MESSAGE *pMessage = (D3D11_MESSAGE *)malloc( messageLength );
                    mInfoQueue->GetMessage( i, pMessage, &messageLength );
                    res = res + pMessage->pDescription + "\n";
                    free( pMessage );
                }
            }
        }

        return res;
    }
    //---------------------------------------------------------------------
    bool D3D11Device::_getErrorsFromQueue() const
    {
        if( mInfoQueue )
        {
            UINT64 numStoredMessages = mInfoQueue->GetNumStoredMessages();

            if( D3D_INFO == mExceptionsErrorLevel && numStoredMessages > 0 )
            {
                // if D3D_INFO we don't need to loop if the numStoredMessages > 0
                return true;
            }
            for( UINT64 i = 0; i < numStoredMessages; i++ )
            {
                // Get the size of the message
                SIZE_T messageLength = 0;
                mInfoQueue->GetMessage( i, NULL, &messageLength );
                if( messageLength > 0u )
                {
                    // Allocate space and get the message
                    D3D11_MESSAGE *pMessage = (D3D11_MESSAGE *)malloc( messageLength );
                    mInfoQueue->GetMessage( i, pMessage, &messageLength );

                    bool res = false;
                    switch( pMessage->Severity )
                    {
                    case D3D11_MESSAGE_SEVERITY_CORRUPTION:
                        if( D3D_CORRUPTION == mExceptionsErrorLevel )
                        {
                            res = true;
                        }
                        break;
                    case D3D11_MESSAGE_SEVERITY_ERROR:
                        switch( mExceptionsErrorLevel )
                        {
                        case D3D_INFO:
                        case D3D_WARNING:
                        case D3D_ERROR:
                            res = true;
                        }
                        break;
                    case D3D11_MESSAGE_SEVERITY_WARNING:
                        switch( mExceptionsErrorLevel )
                        {
                        case D3D_INFO:
                        case D3D_WARNING:
                            res = true;
                        }
                        break;
                    }

                    free( pMessage );

                    if( res )
                    {
                        // we don't need to loop anymore...
                        return true;
                    }
                }
            }

            clearStoredErrorMessages();

            return false;
        }
        else
        {
            return false;
        }
    }
    //---------------------------------------------------------------------
    void D3D11Device::clearStoredErrorMessages() const
    {
        if( mD3D11Device && D3D_NO_EXCEPTION != mExceptionsErrorLevel )
        {
            if( mInfoQueue )
            {
                mInfoQueue->ClearStoredMessages();
            }
        }
    }
    //---------------------------------------------------------------------
    D3D11Device::eExceptionsErrorLevel D3D11Device::getExceptionsErrorLevel()
    {
        return mExceptionsErrorLevel;
    }
    //---------------------------------------------------------------------
    void D3D11Device::setExceptionsErrorLevel( const eExceptionsErrorLevel exceptionsErrorLevel )
    {
        mExceptionsErrorLevel = exceptionsErrorLevel;
    }
    //---------------------------------------------------------------------
    void D3D11Device::setExceptionsErrorLevel( const Ogre::String &exceptionsErrorLevel )
    {
        eExceptionsErrorLevel onlyIfDebugMode =
            OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH ? D3D11Device::D3D_ERROR : D3D11Device::D3D_NO_EXCEPTION;
        if( "No information queue exceptions" == exceptionsErrorLevel )
            setExceptionsErrorLevel( onlyIfDebugMode );
        else if( "Corruption" == exceptionsErrorLevel )
            setExceptionsErrorLevel( D3D11Device::D3D_CORRUPTION );
        else if( "Error" == exceptionsErrorLevel )
            setExceptionsErrorLevel( D3D11Device::D3D_ERROR );
        else if( "Warning" == exceptionsErrorLevel )
            setExceptionsErrorLevel( D3D11Device::D3D_WARNING );
        else if( "Info (exception on any message)" == exceptionsErrorLevel )
            setExceptionsErrorLevel( D3D11Device::D3D_INFO );
        else
            setExceptionsErrorLevel( onlyIfDebugMode );
    }
    //---------------------------------------------------------------------
    D3D_FEATURE_LEVEL D3D11Device::parseFeatureLevel( const Ogre::String &value,
                                                      D3D_FEATURE_LEVEL fallback )
    {
        if( value == "9.1" )
            return D3D_FEATURE_LEVEL_9_1;
        if( value == "9.2" )
            return D3D_FEATURE_LEVEL_9_2;
        if( value == "9.3" )
            return D3D_FEATURE_LEVEL_9_3;
        if( value == "10.0" )
            return D3D_FEATURE_LEVEL_10_0;
        if( value == "10.1" )
            return D3D_FEATURE_LEVEL_10_1;
        if( value == "11.0" )
            return D3D_FEATURE_LEVEL_11_0;
#if defined( _WIN32_WINNT_WIN8 )
        if( value == "11.1" )
            return D3D_FEATURE_LEVEL_11_1;
#endif
        return fallback;
    }
    //---------------------------------------------------------------------
    D3D_DRIVER_TYPE D3D11Device::parseDriverType( const Ogre::String &driverTypeName,
                                                  D3D_DRIVER_TYPE fallback )
    {
        if( "Hardware" == driverTypeName )
            return D3D_DRIVER_TYPE_HARDWARE;
        if( "Software" == driverTypeName )
            return D3D_DRIVER_TYPE_SOFTWARE;
        if( "Reference" == driverTypeName )
            return D3D_DRIVER_TYPE_REFERENCE;
        if( "Warp" == driverTypeName )
            return D3D_DRIVER_TYPE_WARP;
        return fallback;
    }
    //---------------------------------------------------------------------
    bool D3D11Device::IsDeviceLost()
    {
        HRESULT hr = mD3D11Device->GetDeviceRemovedReason();
        if( FAILED( hr ) )
            return true;
        return false;
    }
}  // namespace Ogre
