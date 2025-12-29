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

#include "OgreD3D11RenderSystem.h"

#include "CommandBuffer/OgreCbDrawCall.h"
#include "Compositor/OgreCompositorManager2.h"
#include "OgreD3D11Driver.h"
#include "OgreD3D11DriverList.h"
#include "OgreD3D11GpuProgramManager.h"
#include "OgreD3D11HLSLProgram.h"
#include "OgreD3D11HLSLProgramFactory.h"
#include "OgreD3D11HardwareBufferManager.h"
#include "OgreD3D11HardwareIndexBuffer.h"
#include "OgreD3D11HardwareOcclusionQuery.h"
#include "OgreD3D11HardwareVertexBuffer.h"
#include "OgreD3D11HlmsPso.h"
#include "OgreD3D11Mappings.h"
#include "OgreD3D11RenderPassDescriptor.h"
#include "OgreD3D11TextureGpu.h"
#include "OgreD3D11TextureGpuManager.h"
#include "OgreD3D11VideoMode.h"
#include "OgreD3D11VideoModeList.h"
#include "OgreD3D11Window.h"
#include "OgreDepthBuffer.h"
#include "OgreException.h"
#include "OgreFrustum.h"
#include "OgreHlmsDatablock.h"
#include "OgreHlmsSamplerblock.h"
#include "OgreLogManager.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreOSVersionHelpers.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreProfiler.h"
#include "OgreSceneManagerEnumerator.h"
#include "OgreTimer.h"
#include "OgreViewport.h"
#include "Vao/OgreD3D11BufferInterface.h"
#include "Vao/OgreD3D11ReadOnlyBufferPacked.h"
#include "Vao/OgreD3D11TexBufferPacked.h"
#include "Vao/OgreD3D11UavBufferPacked.h"
#include "Vao/OgreD3D11VaoManager.h"
#include "Vao/OgreD3D11VertexArrayObject.h"
#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreIndirectBufferPacked.h"
#include "VendorExtensions/OgreD3D11VendorExtension.h"

#include <sstream>

#ifdef _WIN32_WINNT_WIN10
#    include <d3d11_3.h>
#endif

#if OGRE_NO_QUAD_BUFFER_STEREO == 0
#    include "OgreD3D11StereoDriverBridge.h"
#endif

#ifndef D3D_FL9_3_SIMULTANEOUS_RENDER_TARGET_COUNT
#    define D3D_FL9_3_SIMULTANEOUS_RENDER_TARGET_COUNT 4
#endif

#ifndef D3D_FL9_1_SIMULTANEOUS_RENDER_TARGET_COUNT
#    define D3D_FL9_1_SIMULTANEOUS_RENDER_TARGET_COUNT 1
#endif
//---------------------------------------------------------------------
#include <OgreNsightChecker.h>
#include <d3d10.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT && defined( _WIN32_WINNT_WINBLUE ) && \
    _WIN32_WINNT >= _WIN32_WINNT_WINBLUE
#    include <dxgi1_3.h>  // for IDXGIDevice3::Trim
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT && !defined( __cplusplus_winrt )
#    include <winrt/Windows.ApplicationModel.Core.h>
#    include <winrt/Windows.Graphics.Display.h>
#endif

namespace Ogre
{
    //---------------------------------------------------------------------
    D3D11RenderSystem::D3D11RenderSystem() :
        mDevice(),
        mVendorExtension( 0 ),
        mBoundIndirectBuffer( 0 ),
        mSwIndirectBufferPtr( 0 ),
        mPso( 0 ),
        mBoundComputeProgram( 0 ),
        mMaxBoundUavCS( 0 ),
        mCurrentVertexBuffer( 0 ),
        mCurrentIndexBuffer( 0 ),
        mNumberOfViews( 0 ),
        mDepthStencilView( 0 ),
        mMaxComputeShaderSrvCount( 0 )
#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        ,
        mStereoDriver( NULL )
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT
        ,
        suspendingToken(),
        surfaceContentLostToken()
#endif
    {
        LogManager::getSingleton().logMessage( "D3D11: " + getName() + " created." );

        memset( mRenderTargetViews, 0, sizeof( mRenderTargetViews ) );
        memset( mNullViews, 0, sizeof( mNullViews ) );
        memset( mMaxSrvCount, 0, sizeof( mMaxSrvCount ) );

        mRenderSystemWasInited = false;
        mSwitchingFullscreenCounter = 0;
        mDriverType = D3D_DRIVER_TYPE_HARDWARE;

        initRenderSystem();

        // set config options defaults
        initConfigOptions();

        // Clear class instance storage
        memset( mClassInstances, 0, sizeof( mClassInstances ) );
        memset( mNumClassInstances, 0, sizeof( mNumClassInstances ) );

        mEventNames.push_back( "DeviceLost" );
        mEventNames.push_back( "DeviceRestored" );

#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT && defined( __cplusplus_winrt )
#    if defined( _WIN32_WINNT_WINBLUE ) && _WIN32_WINNT >= _WIN32_WINNT_WINBLUE
        suspendingToken =
            ( Windows::ApplicationModel::Core::CoreApplication::Suspending +=
              ref new Windows::Foundation::EventHandler<
                  Windows::ApplicationModel::SuspendingEventArgs ^>(
                  [this]( Platform::Object ^ sender,
                          Windows::ApplicationModel::SuspendingEventArgs ^ e ) {
                      // Hints to the driver that the app is entering an idle state and that its memory
                      // can be used temporarily for other apps.
                      ComPtr<IDXGIDevice3> pDXGIDevice;
                      if( mDevice.get() &&
                          SUCCEEDED( mDevice->QueryInterface( pDXGIDevice.GetAddressOf() ) ) )
                          pDXGIDevice->Trim();
                  } ) );

        surfaceContentLostToken =
            ( Windows::Graphics::Display::DisplayInformation::DisplayContentsInvalidated +=
              ref new Windows::Foundation::TypedEventHandler<
                  Windows::Graphics::Display::DisplayInformation ^, Platform::Object ^>(
                  [this]( Windows::Graphics::Display::DisplayInformation ^ sender,
                          Platform::Object ^ arg ) {
                      LogManager::getSingleton().logMessage( "D3D11: DisplayContentsInvalidated." );
                      validateDevice( true );
                  } ) );
#    else  // Win 8.0
        surfaceContentLostToken =
            ( Windows::Graphics::Display::DisplayProperties::DisplayContentsInvalidated +=
              ref new Windows::Graphics::Display::DisplayPropertiesEventHandler(
                  [this]( Platform::Object ^ sender ) {
                      LogManager::getSingleton().logMessage( "D3D11: DisplayContentsInvalidated." );
                      validateDevice( true );
                  } ) );
#    endif
#elif OGRE_PLATFORM == OGRE_PLATFORM_WINRT && !defined( __cplusplus_winrt )
#    if defined( _WIN32_WINNT_WINBLUE ) && _WIN32_WINNT >= _WIN32_WINNT_WINBLUE
        suspendingToken = winrt::Windows::ApplicationModel::Core::CoreApplication::Suspending(
            [this]( auto const &, auto const & ) {
                // Hints to the driver that the app is entering an idle state and that its memory
                // can be used temporarily for other apps.
                ComPtr<IDXGIDevice3> pDXGIDevice;
                if( mDevice.get() && SUCCEEDED( mDevice->QueryInterface( pDXGIDevice.GetAddressOf() ) ) )
                    pDXGIDevice->Trim();
            } );

        surfaceContentLostToken =
            winrt::Windows::Graphics::Display::DisplayInformation::DisplayContentsInvalidated(
                [this]( auto const &, auto const & ) {
                    LogManager::getSingleton().logMessage( "D3D11: DisplayContentsInvalidated." );
                    validateDevice( true );
                } );
#    else  // Win 8.0
        surfaceContentLostToken =
            winrt::Windows::Graphics::Display::DisplayProperties::DisplayContentsInvalidated(
                [this]( auto const & ) {
                    LogManager::getSingleton().logMessage( "D3D11: DisplayContentsInvalidated." );
                    validateDevice( true );
                } );
#    endif
#endif
    }
    //---------------------------------------------------------------------
    D3D11RenderSystem::~D3D11RenderSystem()
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT && defined( __cplusplus_winrt )
#    if defined( _WIN32_WINNT_WINBLUE ) && _WIN32_WINNT >= _WIN32_WINNT_WINBLUE
        Windows::ApplicationModel::Core::CoreApplication::Suspending -= suspendingToken;
        Windows::Graphics::Display::DisplayInformation::DisplayContentsInvalidated -=
            surfaceContentLostToken;
#    else  // Win 8.0
        Windows::Graphics::Display::DisplayProperties::DisplayContentsInvalidated -=
            surfaceContentLostToken;
#    endif
#elif OGRE_PLATFORM == OGRE_PLATFORM_WINRT && !defined( __cplusplus_winrt )
#    if defined( _WIN32_WINNT_WINBLUE ) && _WIN32_WINNT >= _WIN32_WINNT_WINBLUE
        winrt::Windows::ApplicationModel::Core::CoreApplication::Suspending( suspendingToken );
        winrt::Windows::Graphics::Display::DisplayInformation::DisplayContentsInvalidated(
            surfaceContentLostToken );
#    else  // Win 8.0
        winrt::Windows::Graphics::Display::DisplayProperties::DisplayContentsInvalidated(
            surfaceContentLostToken );
#    endif
#endif

        shutdown();

        // Deleting the HLSL program factory
        if( mHLSLProgramFactory )
        {
            // Remove from manager safely
            if( HighLevelGpuProgramManager::getSingletonPtr() )
                HighLevelGpuProgramManager::getSingleton().removeFactory( mHLSLProgramFactory );
            delete mHLSLProgramFactory;
            mHLSLProgramFactory = 0;
        }

#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        // Stereo driver must be freed after device is created
        D3D11StereoDriverBridge *stereoBridge = D3D11StereoDriverBridge::getSingletonPtr();
        OGRE_DELETE stereoBridge;
#endif

        LogManager::getSingleton().logMessage( "D3D11: " + getName() + " destroyed." );
    }
    //---------------------------------------------------------------------
    const String &D3D11RenderSystem::getName() const
    {
        static String strName( "Direct3D11 Rendering Subsystem" );
        return strName;
    }
    //---------------------------------------------------------------------
    const String &D3D11RenderSystem::getFriendlyName() const
    {
        static String strName( "Direct3D 11" );
        return strName;
    }
    //---------------------------------------------------------------------
    D3D11DriverList *D3D11RenderSystem::getDirect3DDrivers( bool refreshList /* = false*/ )
    {
        if( !mDriverList )
            mDriverList = new D3D11DriverList();

        if( refreshList || mDriverList->count() == 0 )
            mDriverList->refresh();

        return mDriverList;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::createD3D11Device( D3D11VendorExtension *vendorExtension,
                                               const String &appName, D3D11Driver *d3dDriver,
                                               D3D_DRIVER_TYPE driverType, D3D_FEATURE_LEVEL minFL,
                                               D3D_FEATURE_LEVEL maxFL, D3D_FEATURE_LEVEL *pFeatureLevel,
                                               ID3D11Device **outDevice )
    {
        IDXGIAdapterN *pAdapter = ( d3dDriver && driverType == D3D_DRIVER_TYPE_HARDWARE )
                                      ? d3dDriver->getDeviceAdapter()
                                      : NULL;

        assert( driverType == D3D_DRIVER_TYPE_HARDWARE || driverType == D3D_DRIVER_TYPE_SOFTWARE ||
                driverType == D3D_DRIVER_TYPE_REFERENCE || driverType == D3D_DRIVER_TYPE_WARP );
        if( driverType == D3D_DRIVER_TYPE_REFERENCE || driverType == D3D_DRIVER_TYPE_WARP )
            d3dDriver = NULL;
        else if( d3dDriver != NULL )
        {
            if( 0 == wcscmp( d3dDriver->getAdapterIdentifier().Description, L"NVIDIA PerfHUD" ) )
                driverType = D3D_DRIVER_TYPE_REFERENCE;
            else
                driverType = D3D_DRIVER_TYPE_UNKNOWN;
        }

        // determine deviceFlags
        UINT deviceFlags = 0;
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT
        // This flag is required in order to enable compatibility with Direct2D.
        deviceFlags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#endif
        if( OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH && !IsWorkingUnderNsight() &&
            D3D11Device::D3D_NO_EXCEPTION != D3D11Device::getExceptionsErrorLevel() )
        {
            deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
        }
        if( !OGRE_THREAD_SUPPORT )
            deviceFlags |= D3D11_CREATE_DEVICE_SINGLETHREADED;

        // determine feature levels
        D3D_FEATURE_LEVEL requestedLevels[] = {
        // Windows Phone support only FL 9.3, but simulator can create
        // much more capable device, so restrict it artificially here
#if !__OGRE_WINRT_PHONE
#    if defined( _WIN32_WINNT_WIN8 )
            D3D_FEATURE_LEVEL_11_1,
#    endif
            D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
#endif  // !__OGRE_WINRT_PHONE
            D3D_FEATURE_LEVEL_9_3,  D3D_FEATURE_LEVEL_9_2,  D3D_FEATURE_LEVEL_9_1
        };

        D3D_FEATURE_LEVEL *pFirstFL = requestedLevels;
        D3D_FEATURE_LEVEL *pLastFL = pFirstFL + ARRAYSIZE( requestedLevels ) - 1;
        for( size_t i = 0; i < ARRAYSIZE( requestedLevels ); ++i )
        {
            if( minFL == requestedLevels[i] )
                pLastFL = &requestedLevels[i];
            if( maxFL == requestedLevels[i] )
                pFirstFL = &requestedLevels[i];
        }
        if( pLastFL < pFirstFL )
        {
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR,
                         "Requested min level feature is bigger the requested max level feature.",
                         "D3D11RenderSystem::initialise" );
        }

        vendorExtension->createDevice( appName, pAdapter, driverType, deviceFlags, pFirstFL,
                                       static_cast<UINT>( pLastFL - pFirstFL + 1u ), pFeatureLevel,
                                       outDevice );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::initConfigOptions()
    {
        ConfigOption optDevice;
        ConfigOption optVideoMode;
        ConfigOption optFullScreen;
        ConfigOption optVSync;
        ConfigOption optVSyncInterval;
        ConfigOption optBackBufferCount;
        ConfigOption optAA;
        ConfigOption optFPUMode;
        ConfigOption optNVPerfHUD;
        ConfigOption optSRGB;
        ConfigOption optMinFeatureLevels;
        ConfigOption optMaxFeatureLevels;
        ConfigOption optExceptionsErrorLevel;
        ConfigOption optDriverType;
        ConfigOption optVendorExt;
        ConfigOption optFastShaderBuildHack;
#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        ConfigOption optStereoMode;
#endif

        optDevice.name = "Rendering Device";
        optDevice.currentValue = "(default)";
        optDevice.possibleValues.push_back( "(default)" );
        D3D11DriverList *driverList = getDirect3DDrivers();
        for( unsigned j = 0; j < driverList->count(); j++ )
        {
            D3D11Driver *driver = driverList->item( j );
            optDevice.possibleValues.emplace_back( driver->DriverDescription() );
        }
        optDevice.immutable = false;

        optVideoMode.name = "Video Mode";
        // optVideoMode.currentValue = "800 x 600 @ 32-bit colour";
        optVideoMode.immutable = false;

        optFullScreen.name = "Full Screen";
        optFullScreen.possibleValues.push_back( "Yes" );
        optFullScreen.possibleValues.push_back( "No" );
        optFullScreen.currentValue = "Yes";
        optFullScreen.immutable = false;

        optVSync.name = "VSync";
        optVSync.immutable = false;
        optVSync.possibleValues.push_back( "Yes" );
        optVSync.possibleValues.push_back( "No" );
        optVSync.currentValue = "No";

        optVSyncInterval.name = "VSync Interval";
        optVSyncInterval.immutable = false;
        optVSyncInterval.possibleValues.push_back( "1" );
        optVSyncInterval.possibleValues.push_back( "2" );
        optVSyncInterval.possibleValues.push_back( "3" );
        optVSyncInterval.possibleValues.push_back( "4" );
        optVSyncInterval.currentValue = "1";

        optBackBufferCount.name = "Backbuffer Count";
        optBackBufferCount.immutable = false;
        optBackBufferCount.possibleValues.push_back( "Auto" );
        optBackBufferCount.possibleValues.push_back( "1" );
        optBackBufferCount.possibleValues.push_back( "2" );
        optBackBufferCount.currentValue = "Auto";

        optAA.name = "FSAA";
        optAA.immutable = false;
        optAA.possibleValues.push_back( "None" );
        optAA.currentValue = "None";

        optFPUMode.name = "Floating-point mode";
#if OGRE_DOUBLE_PRECISION
        optFPUMode.currentValue = "Consistent";
#else
        optFPUMode.currentValue = "Fastest";
#endif
        optFPUMode.possibleValues.clear();
        optFPUMode.possibleValues.push_back( "Fastest" );
        optFPUMode.possibleValues.push_back( "Consistent" );
        optFPUMode.immutable = false;

        optNVPerfHUD.currentValue = "No";
        optNVPerfHUD.immutable = false;
        optNVPerfHUD.name = "Allow NVPerfHUD";
        optNVPerfHUD.possibleValues.push_back( "Yes" );
        optNVPerfHUD.possibleValues.push_back( "No" );

        // SRGB on auto window
        optSRGB.name = "sRGB Gamma Conversion";
        optSRGB.possibleValues.push_back( "Yes" );
        optSRGB.possibleValues.push_back( "No" );
        optSRGB.currentValue = "Yes";
        optSRGB.immutable = false;

        // min feature level
        optMinFeatureLevels;
        optMinFeatureLevels.name = "Min Requested Feature Levels";
        optMinFeatureLevels.possibleValues.push_back( "9.1" );
        optMinFeatureLevels.possibleValues.push_back( "9.3" );
        optMinFeatureLevels.possibleValues.push_back( "10.0" );
        optMinFeatureLevels.possibleValues.push_back( "10.1" );
        optMinFeatureLevels.possibleValues.push_back( "11.0" );

        optMinFeatureLevels.currentValue = "9.1";
        optMinFeatureLevels.immutable = false;

        // max feature level
        optMaxFeatureLevels;
        optMaxFeatureLevels.name = "Max Requested Feature Levels";
        optMaxFeatureLevels.possibleValues.push_back( "9.1" );

#if __OGRE_WINRT_PHONE_80
        optMaxFeatureLevels.possibleValues.push_back( "9.2" );
        optMaxFeatureLevels.possibleValues.push_back( "9.3" );
        optMaxFeatureLevels.currentValue = "9.3";
#else
        optMaxFeatureLevels.possibleValues.push_back( "9.3" );
        optMaxFeatureLevels.possibleValues.push_back( "10.0" );
        optMaxFeatureLevels.possibleValues.push_back( "10.1" );
        optMaxFeatureLevels.possibleValues.push_back( "11.0" );
#    if defined( _WIN32_WINNT_WIN8 )
        if( IsWindows8OrGreater() )
        {
            optMaxFeatureLevels.possibleValues.push_back( "11.1" );
            optMaxFeatureLevels.currentValue = "11.1";
        }
        else
        {
            optMaxFeatureLevels.currentValue = "11.0";
        }
#    else
        optMaxFeatureLevels.currentValue = "11.0";
#    endif
#endif
        optMaxFeatureLevels.immutable = false;

        // Exceptions Error Level
        optExceptionsErrorLevel.name = "Information Queue Exceptions Bottom Level";
        optExceptionsErrorLevel.possibleValues.push_back( "No information queue exceptions" );
        optExceptionsErrorLevel.possibleValues.push_back( "Corruption" );
        optExceptionsErrorLevel.possibleValues.push_back( "Error" );
        optExceptionsErrorLevel.possibleValues.push_back( "Warning" );
        optExceptionsErrorLevel.possibleValues.push_back( "Info (exception on any message)" );
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
        optExceptionsErrorLevel.currentValue = "Info (exception on any message)";
#else
        optExceptionsErrorLevel.currentValue = "No information queue exceptions";
#endif
        optExceptionsErrorLevel.immutable = false;

        // Driver type
        optDriverType.name = "Driver type";
        optDriverType.possibleValues.push_back( "Hardware" );
        optDriverType.possibleValues.push_back( "Software" );
        optDriverType.possibleValues.push_back( "Reference" );
        optDriverType.possibleValues.push_back( "Warp" );
        optDriverType.currentValue = "Hardware";
        optDriverType.immutable = false;

        optVendorExt.name = "Vendor Extensions";
        optVendorExt.possibleValues.push_back( "Auto" );
        optVendorExt.possibleValues.push_back( "NVIDIA" );
        optVendorExt.possibleValues.push_back( "AMD" );
        optVendorExt.possibleValues.push_back( "Disabled" );
        optVendorExt.currentValue = "Auto";
        optVendorExt.immutable = false;

        // This option improves shader compilation times by massive amounts
        //(requires Hlms to be aware of it), making shader compile times comparable
        // to GL (which is measured in milliseconds per shader, instead of seconds).
        // There's two possible reasons to disable this hack:
        //  1. Easier debugging. Shader structs like "Material m[256];" get declared
        //     as "Material m[2];" which cause debuggers to show only 2 entries,
        //     instead of all of them. Some debuggers (like RenderDoc) allow changing
        //     the amount of elements displayed and workaround it; nonetheless
        //     disabling it makes your life easier.
        //  2. Troubleshooting an obscure GPU/driver combination. I tested this hack
        //     with a lot of hardware and it seems to work. However the possibility
        //     that it breaks with a specific GPU/driver combo always exists. In
        //     such case, the end user should be able to turn this off.
        optFastShaderBuildHack.name = "Fast Shader Build Hack";
        optFastShaderBuildHack.possibleValues.push_back( "Yes" );
        optFastShaderBuildHack.possibleValues.push_back( "No" );
        optFastShaderBuildHack.currentValue = "Yes";
        optFastShaderBuildHack.immutable = false;

#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        optStereoMode.name = "Stereo Mode";
        optStereoMode.possibleValues.push_back( StringConverter::toString( SMT_NONE ) );
        optStereoMode.possibleValues.push_back( StringConverter::toString( SMT_FRAME_SEQUENTIAL ) );
        optStereoMode.currentValue = optStereoMode.possibleValues[0];
        optStereoMode.immutable = false;

        mOptions[optStereoMode.name] = optStereoMode;
#endif

        mOptions[optDevice.name] = optDevice;
        mOptions[optVideoMode.name] = optVideoMode;
        mOptions[optFullScreen.name] = optFullScreen;
        mOptions[optVSync.name] = optVSync;
        mOptions[optVSyncInterval.name] = optVSyncInterval;
        mOptions[optAA.name] = optAA;
        mOptions[optFPUMode.name] = optFPUMode;
        mOptions[optNVPerfHUD.name] = optNVPerfHUD;
        mOptions[optSRGB.name] = optSRGB;
        mOptions[optMinFeatureLevels.name] = optMinFeatureLevels;
        mOptions[optMaxFeatureLevels.name] = optMaxFeatureLevels;
        mOptions[optExceptionsErrorLevel.name] = optExceptionsErrorLevel;
        mOptions[optDriverType.name] = optDriverType;
        mOptions[optVendorExt.name] = optVendorExt;
        mOptions[optFastShaderBuildHack.name] = optFastShaderBuildHack;

        mOptions[optBackBufferCount.name] = optBackBufferCount;

        refreshD3DSettings();
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::refreshD3DSettings()
    {
        ConfigOption *optVideoMode;
        D3D11VideoMode *videoMode;

        ConfigOptionMap::iterator opt = mOptions.find( "Rendering Device" );
        if( opt != mOptions.end() )
        {
            D3D11Driver *driver = getDirect3DDrivers()->findByName( opt->second.currentValue );
            if( driver )
            {
                opt = mOptions.find( "Video Mode" );
                optVideoMode = &opt->second;
                optVideoMode->possibleValues.clear();
                // get vide modes for this device
                for( unsigned k = 0; k < driver->getVideoModeList()->count(); k++ )
                {
                    videoMode = driver->getVideoModeList()->item( k );
                    optVideoMode->possibleValues.emplace_back( videoMode->getDescription() );
                }

                // Reset video mode to default if previous doesn't avail in new possible values
                StringVector::const_iterator itValue =
                    std::find( optVideoMode->possibleValues.begin(), optVideoMode->possibleValues.end(),
                               optVideoMode->currentValue );
                if( itValue == optVideoMode->possibleValues.end() )
                {
                    if( optVideoMode->possibleValues.empty() )
                        optVideoMode->currentValue = "800 x 600 @ 32-bit colour";
                    else
                        optVideoMode->currentValue = optVideoMode->possibleValues.back();
                }

                // Also refresh FSAA options
                refreshFSAAOptions();
            }
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::setConfigOption( const String &name, const String &value )
    {
        initRenderSystem();

        LogManager::getSingleton().stream() << "D3D11: RenderSystem Option: " << name << " = " << value;

        bool viewModeChanged = false;

        // Find option
        ConfigOptionMap::iterator it = mOptions.find( name );

        // Update
        if( it != mOptions.end() )
            it->second.currentValue = value;
        else
        {
            StringStream str;
            str << "Option named '" << name << "' does not exist.";
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, str.str(), "D3D11RenderSystem::setConfigOption" );
        }

        // Refresh other options if D3DDriver changed
        if( name == "Rendering Device" )
            refreshD3DSettings();

        if( name == "Full Screen" )
        {
            // Video mode is applicable
            it = mOptions.find( "Video Mode" );
            if( it->second.currentValue.empty() )
            {
                it->second.currentValue = "800 x 600 @ 32-bit colour";
                viewModeChanged = true;
            }
        }

        if( name == "Min Requested Feature Levels" )
        {
            mMinRequestedFeatureLevel = D3D11Device::parseFeatureLevel( value, D3D_FEATURE_LEVEL_9_1 );
        }

        if( name == "Max Requested Feature Levels" )
        {
#if defined( _WIN32_WINNT_WIN8 ) && _WIN32_WINNT >= _WIN32_WINNT_WIN8
            if( IsWindows8OrGreater() )
                mMaxRequestedFeatureLevel =
                    D3D11Device::parseFeatureLevel( value, D3D_FEATURE_LEVEL_11_1 );
            else
                mMaxRequestedFeatureLevel =
                    D3D11Device::parseFeatureLevel( value, D3D_FEATURE_LEVEL_11_0 );
#else
            mMaxRequestedFeatureLevel = D3D11Device::parseFeatureLevel( value, D3D_FEATURE_LEVEL_11_0 );
#endif
        }

        if( name == "Allow NVPerfHUD" )
        {
            if( value == "Yes" )
                mUseNVPerfHUD = true;
            else
                mUseNVPerfHUD = false;
        }

        if( viewModeChanged || name == "Video Mode" )
        {
            refreshFSAAOptions();
        }
    }
    //---------------------------------------------------------------------
    const char *D3D11RenderSystem::getPriorityConfigOption( size_t ) const { return "Rendering Device"; }
    //---------------------------------------------------------------------
    size_t D3D11RenderSystem::getNumPriorityConfigOptions() const { return 1u; }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::refreshFSAAOptions()
    {
        ConfigOptionMap::iterator it = mOptions.find( "FSAA" );
        ConfigOption *optFSAA = &it->second;
        optFSAA->possibleValues.clear();

        it = mOptions.find( "Rendering Device" );
        D3D11Driver *driver = getDirect3DDrivers()->findByName( it->second.currentValue );
        if( driver )
        {
            it = mOptions.find( "Video Mode" );
            ComPtr<ID3D11Device> device;
            createD3D11Device( mVendorExtension, Root::getSingleton().getAppName(), driver, mDriverType,
                               mMinRequestedFeatureLevel, mMaxRequestedFeatureLevel, NULL,
                               device.GetAddressOf() );
            // 'videoMode' could be NULL if working over RDP/Simulator
            D3D11VideoMode *videoMode = driver->getVideoModeList()->item( it->second.currentValue );
            DXGI_FORMAT format = videoMode ? videoMode->getFormat() : DXGI_FORMAT_R8G8B8A8_UNORM;
            UINT numLevels = 0;
            // set maskable levels supported
            for( UINT n = 1; n <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; n++ )
            {
                // new style enumeration, with "8x CSAA", "8x MSAA" values
                if( n == 4 &&
                    SUCCEEDED( device->CheckMultisampleQualityLevels( format, 2, &numLevels ) ) &&
                    numLevels > 4 )  // 2f4x EQAA
                {
                    optFSAA->possibleValues.push_back( "2f4x EQAA" );
                }
                if( n == 8 &&
                    SUCCEEDED( device->CheckMultisampleQualityLevels( format, 4, &numLevels ) ) &&
                    numLevels > 8 )  // 8x CSAA
                {
                    optFSAA->possibleValues.push_back( "8x CSAA" );
                }
                if( n == 16 &&
                    SUCCEEDED( device->CheckMultisampleQualityLevels( format, 4, &numLevels ) ) &&
                    numLevels > 16 )  // 16x CSAA
                {
                    optFSAA->possibleValues.push_back( "16x CSAA" );
                }
                if( n == 16 &&
                    SUCCEEDED( device->CheckMultisampleQualityLevels( format, 8, &numLevels ) ) &&
                    numLevels > 16 )  // 16xQ CSAA
                {
                    optFSAA->possibleValues.push_back( "16xQ CSAA" );
                }
                if( SUCCEEDED( device->CheckMultisampleQualityLevels( format, n, &numLevels ) ) &&
                    numLevels > 0 )  // Nx MSAA
                {
                    optFSAA->possibleValues.push_back( StringConverter::toString( n ) + "x MSAA" );
                }
            }
        }

        if( optFSAA->possibleValues.empty() )
        {
            optFSAA->possibleValues.push_back(
                "1" );  // D3D11 does not distinguish between noMSAA and 1xMSAA
        }

        // Reset FSAA to none if previous doesn't avail in new possible values
        StringVector::const_iterator itValue = std::find(
            optFSAA->possibleValues.begin(), optFSAA->possibleValues.end(), optFSAA->currentValue );
        if( itValue == optFSAA->possibleValues.end() )
        {
            optFSAA->currentValue = optFSAA->possibleValues[0];
        }
    }
    //---------------------------------------------------------------------
    String D3D11RenderSystem::validateConfigOptions()
    {
        ConfigOptionMap::iterator it;

        // check if video mode is selected
        it = mOptions.find( "Video Mode" );
        if( it->second.currentValue.empty() )
            return "A video mode must be selected.";

        it = mOptions.find( "Rendering Device" );
        String driverName = it->second.currentValue;
        if( driverName != "(default)" &&
            getDirect3DDrivers()->findByName( driverName )->DriverDescription() != driverName )
        {
            // Just pick default driver
            setConfigOption( "Rendering Device", "(default)" );
            return "Requested rendering device could not be found, default would be used instead.";
        }

        return BLANKSTRING;
    }
    //---------------------------------------------------------------------
    ConfigOptionMap &D3D11RenderSystem::getConfigOptions()
    {
        // return a COPY of the current config options
        return mOptions;
    }
    //---------------------------------------------------------------------
    Window *D3D11RenderSystem::_initialise( bool autoCreateWindow, const String &windowTitle )
    {
        Window *autoWindow = NULL;
        LogManager::getSingleton().logMessage( "D3D11: Subsystem Initialising" );

        if( IsWorkingUnderNsight() )
            LogManager::getSingleton().logMessage( "D3D11: Nvidia Nsight found" );

        // Init using current settings
        ConfigOptionMap::iterator opt = mOptions.find( "Rendering Device" );
        if( opt == mOptions.end() )
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Can`t find requested Direct3D driver name!",
                         "D3D11RenderSystem::initialise" );
        mDriverName = opt->second.currentValue;

        // Driver type
        opt = mOptions.find( "Driver type" );
        if( opt == mOptions.end() )
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "Can't find driver type!",
                         "D3D11RenderSystem::initialise" );
        mDriverType = D3D11Device::parseDriverType( opt->second.currentValue );

        opt = mOptions.find( "Information Queue Exceptions Bottom Level" );
        if( opt == mOptions.end() )
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR,
                         "Can't find Information Queue Exceptions Bottom Level option!",
                         "D3D11RenderSystem::initialise" );
        D3D11Device::setExceptionsErrorLevel( opt->second.currentValue );

#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        // Stereo driver must be created before device is created
        StereoModeType stereoMode =
            StringConverter::parseStereoMode( mOptions["Stereo Mode"].currentValue );
        D3D11StereoDriverBridge *stereoBridge = OGRE_NEW D3D11StereoDriverBridge( stereoMode );
#endif

        // create the device for the selected adapter
        createDevice();

        if( autoCreateWindow )
        {
            bool fullScreen;
            opt = mOptions.find( "Full Screen" );
            if( opt == mOptions.end() )
                OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "Can't find full screen option!",
                             "D3D11RenderSystem::initialise" );
            fullScreen = opt->second.currentValue == "Yes";

            D3D11VideoMode *videoMode = NULL;
            unsigned int width, height;
            String temp;

            opt = mOptions.find( "Video Mode" );
            if( opt == mOptions.end() )
                OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "Can't find Video Mode option!",
                             "D3D11RenderSystem::initialise" );

            // The string we are manipulating looks like this :width x height @ colourDepth
            // Pull out the colour depth by getting what comes after the @ and a space
            String colourDepth =
                opt->second.currentValue.substr( opt->second.currentValue.rfind( '@' ) + 1 );
            // Now we know that the width starts a 0, so if we can find the end we can parse that out
            String::size_type widthEnd = opt->second.currentValue.find( ' ' );
            // we know that the height starts 3 characters after the width and goes until the next space
            String::size_type heightEnd = opt->second.currentValue.find( ' ', widthEnd + 3 );
            // Now we can parse out the values
            width = StringConverter::parseInt( opt->second.currentValue.substr( 0, widthEnd ) );
            height =
                StringConverter::parseInt( opt->second.currentValue.substr( widthEnd + 3, heightEnd ) );

            D3D11VideoModeList *videoModeList = mActiveD3DDriver.getVideoModeList();
            for( unsigned j = 0; j < videoModeList->count(); j++ )
            {
                temp = videoModeList->item( j )->getDescription();

                // In full screen we only want to allow supported resolutions, so temp and
                // opt->second.currentValue need to match exactly, but in windowed mode we
                // can allow for arbitrary window sized, so we only need to match the
                // colour values
                if( ( fullScreen && ( temp == opt->second.currentValue ) ) ||
                    ( !fullScreen && ( temp.substr( temp.rfind( '@' ) + 1 ) == colourDepth ) ) )
                {
                    videoMode = videoModeList->item( j );
                    break;
                }
            }

            // sRGB window option
            bool hwGamma = false;
            opt = mOptions.find( "sRGB Gamma Conversion" );
            if( opt == mOptions.end() )
                OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "Can't find sRGB option!",
                             "D3D11RenderSystem::initialise" );
            hwGamma = opt->second.currentValue == "Yes";
            String fsaa;
            if( ( opt = mOptions.find( "FSAA" ) ) != mOptions.end() )
                fsaa = opt->second.currentValue;

            if( !videoMode )
            {
                LogManager::getSingleton().logMessage(
                    "WARNING D3D11: Couldn't find requested video mode. Forcing 32bpp. "
                    "If you have two GPUs and you're rendering to the GPU that is not "
                    "plugged to the monitor you can then ignore this message.",
                    LML_CRITICAL );
            }

            NameValuePairList miscParams;
            miscParams["colourDepth"] =
                StringConverter::toString( videoMode ? videoMode->getColourDepth() : 32 );
            miscParams["FSAA"] = fsaa;
            miscParams["useNVPerfHUD"] = StringConverter::toString( mUseNVPerfHUD );
            miscParams["gamma"] = StringConverter::toString( hwGamma );
            // miscParams["useFlipMode"] = StringConverter::toString(true);

            opt = mOptions.find( "VSync" );
            if( opt == mOptions.end() )
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Can't find VSync options!",
                             "D3D11RenderSystem::initialise" );
            bool vsync = ( opt->second.currentValue == "Yes" );
            miscParams["vsync"] = StringConverter::toString( vsync );

            opt = mOptions.find( "VSync Interval" );
            if( opt == mOptions.end() )
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Can't find VSync Interval options!",
                             "D3D11RenderSystem::initialise" );
            miscParams["vsyncInterval"] = opt->second.currentValue;

            autoWindow =
                this->_createRenderWindow( windowTitle, width, height, fullScreen, &miscParams );

            // If we have 16bit depth buffer enable w-buffering.
            assert( autoWindow );
            if( PixelFormatGpuUtils::getBytesPerPixel( autoWindow->getPixelFormat() ) * 8u == 16u )
            {
                mWBuffer = true;
            }
            else
            {
                mWBuffer = false;
            }
        }

        LogManager::getSingleton().logMessage( "***************************************" );
        LogManager::getSingleton().logMessage( "*** D3D11: Subsystem Initialized OK ***" );
        LogManager::getSingleton().logMessage( "***************************************" );

        // call superclass method
        RenderSystem::_initialise( autoCreateWindow );
        this->fireDeviceEvent( &mDevice, "DeviceCreated" );
        return autoWindow;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::reinitialise()
    {
        LogManager::getSingleton().logMessage( "D3D11: Reinitializing" );
        this->shutdown();
        //  this->initialise( true );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::shutdown()
    {
        RenderSystem::shutdown();

        mRenderSystemWasInited = false;

        mPrimaryWindow = NULL;  // primary window deleted by base class.
        freeDevice();
        SAFE_DELETE( mDriverList );
        mActiveD3DDriver = D3D11Driver();
        mDevice.ReleaseAll();
        SAFE_DELETE( mVendorExtension );
        LogManager::getSingleton().logMessage( "D3D11: Shutting down cleanly." );
        SAFE_DELETE( mHardwareBufferManager );
        SAFE_DELETE( mGpuProgramManager );
    }
    //---------------------------------------------------------------------
    Window *D3D11RenderSystem::_createRenderWindow( const String &name, uint32 width, uint32 height,
                                                    bool fullScreen,
                                                    const NameValuePairList *miscParams )
    {
        // Check we're not creating a secondary window when the primary
        // was fullscreen
        if( mPrimaryWindow && mPrimaryWindow->isFullscreen() && fullScreen == false )
        {
            OGRE_EXCEPT(
                Exception::ERR_INVALID_STATE,
                "Cannot create secondary windows not in full screen when the primary is full screen",
                "D3D11RenderSystem::_createRenderWindow" );
        }

        // Log a message
        StringStream ss;
        ss << "D3D11RenderSystem::_createRenderWindow \"" << name << "\", " << width << "x" << height
           << " ";
        if( fullScreen )
            ss << "fullscreen ";
        else
            ss << "windowed ";
        if( miscParams )
        {
            ss << " miscParams: ";
            NameValuePairList::const_iterator it;
            for( it = miscParams->begin(); it != miscParams->end(); ++it )
            {
                ss << it->first << "=" << it->second << " ";
            }
            LogManager::getSingleton().logMessage( ss.str() );
        }

        String msg;

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        D3D11Window *win =
            new D3D11WindowHwnd( name, width, height, fullScreen, DepthBuffer::DefaultDepthBufferFormat,
                                 miscParams, mDevice, this );
#elif OGRE_PLATFORM == OGRE_PLATFORM_WINRT
        String windowType;
        if( miscParams )
        {
            // Get variable-length params
            NameValuePairList::const_iterator opt = miscParams->find( "windowType" );
            if( opt != miscParams->end() )
                windowType = opt->second;
        }

        D3D11Window *win = NULL;
#    if defined( _WIN32_WINNT_WINBLUE ) && _WIN32_WINNT >= _WIN32_WINNT_WINBLUE
        if( win == NULL && windowType == "SwapChainPanel" )
            win = new D3D11WindowSwapChainPanel( name, width, height, fullScreen,
                                                 DepthBuffer::DefaultDepthBufferFormat, miscParams,
                                                 mDevice, this );
#    endif  // defined(_WIN32_WINNT_WINBLUE) && _WIN32_WINNT >= _WIN32_WINNT_WINBLUE
        if( win == NULL )
            win = new D3D11WindowCoreWindow( name, width, height, fullScreen,
                                             DepthBuffer::DefaultDepthBufferFormat, miscParams, mDevice,
                                             this );
#endif

        mWindows.insert( win );

#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        // Must be called after device has been linked to window
        D3D11StereoDriverBridge::getSingleton().addRenderWindow( win );
        win->_validateStereo();
#endif

        // If this is the first window, get the D3D device and create the texture manager
        if( !mPrimaryWindow )
        {
            mPrimaryWindow = win;
            // win->getCustomAttribute("D3DDEVICE", &mDevice);

            if( miscParams )
            {
                NameValuePairList::const_iterator itOption = miscParams->find( "reverse_depth" );
                if( itOption != miscParams->end() )
                    mReverseDepth = StringConverter::parseBool( itOption->second, true );
            }

            // Also create hardware buffer manager
            mHardwareBufferManager = new v1::D3D11HardwareBufferManager( mDevice );

            // Create the GPU program manager
            mGpuProgramManager = new D3D11GpuProgramManager();
            // create & register HLSL factory
            if( mHLSLProgramFactory == NULL )
                mHLSLProgramFactory = new D3D11HLSLProgramFactory( mDevice );
            mRealCapabilities = createRenderSystemCapabilities();

            // if we are using custom capabilities, then
            // mCurrentCapabilities has already been loaded
            if( !mUseCustomCapabilities )
                mCurrentCapabilities = mRealCapabilities;

            fireEvent( "RenderSystemCapabilitiesCreated" );

            initialiseFromRenderSystemCapabilities( mCurrentCapabilities, mPrimaryWindow );

            assert( !mVaoManager );
            mVaoManager = OGRE_NEW D3D11VaoManager( false, mDevice, this, miscParams );

            mTextureGpuManager = OGRE_NEW D3D11TextureGpuManager( mVaoManager, this, mDevice );

            mTextureGpuManager->_update( true );
        }
        else
        {
            mSecondaryWindows.push_back( win );
        }

        win->_initialize( mTextureGpuManager, miscParams );

        return win;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::fireDeviceEvent( D3D11Device *device, const String &name,
                                             D3D11Window *sendingWindow /* = NULL */ )
    {
        NameValuePairList params;
        params["D3DDEVICE"] = StringConverter::toString( (size_t)device->get() );
        if( sendingWindow )
            params["Window"] = StringConverter::toString( (size_t)sendingWindow );
        fireEvent( name, &params );
    }
    //---------------------------------------------------------------------
    RenderSystemCapabilities *D3D11RenderSystem::createRenderSystemCapabilities() const
    {
        RenderSystemCapabilities *rsc = new RenderSystemCapabilities();
        rsc->setDriverVersion( mDriverVersion );
        rsc->setDeviceName( mActiveD3DDriver.DriverDescription() );
        rsc->setRenderSystemName( getName() );

        rsc->setCapability( RSC_HWSTENCIL );
        rsc->setStencilBufferBitDepth( 8 );

        rsc->setCapability( RSC_HW_GAMMA );
        rsc->setCapability( RSC_TEXTURE_SIGNED_INT );

#ifdef _WIN32_WINNT_WIN10
        // Check if D3D11.3 is installed. If so, typed UAV loads are supported
        ComPtr<ID3D11Device3> d3dDeviceVersion113;
        HRESULT hr = mDevice->QueryInterface( d3dDeviceVersion113.GetAddressOf() );
        if( SUCCEEDED( hr ) && d3dDeviceVersion113 )
        {
            rsc->setCapability( RSC_TYPED_UAV_LOADS );
        }
#    ifdef NTDDI_WIN10_TH2
        D3D11_FEATURE_DATA_D3D11_OPTIONS3 fOpt3 = {};
        hr = mDevice->CheckFeatureSupport( D3D11_FEATURE_D3D11_OPTIONS3, &fOpt3, sizeof( fOpt3 ) );
        if( SUCCEEDED( hr ) && fOpt3.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer )
        {
            rsc->setCapability( RSC_VP_AND_RT_ARRAY_INDEX_FROM_ANY_SHADER );
        }
#    endif
#endif

        rsc->setCapability( RSC_VBO );
        UINT formatSupport;
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_9_2 ||
            ( SUCCEEDED( mDevice->CheckFormatSupport( DXGI_FORMAT_R32_UINT, &formatSupport ) ) &&
              0 != ( formatSupport & D3D11_FORMAT_SUPPORT_IA_INDEX_BUFFER ) ) )
        {
            rsc->setCapability( RSC_32BIT_INDEX );
        }

        // Set number of texture units, always 16
        rsc->setNumTextureUnits( 16 );
        rsc->setMaxSupportedAnisotropy( Real(
            mFeatureLevel >= D3D_FEATURE_LEVEL_9_2
                ? 16
                : 2 ) );  // From http://msdn.microsoft.com/en-us/library/windows/desktop/ff476876.aspx
        rsc->setCapability( RSC_ANISOTROPY );
        rsc->setCapability( RSC_AUTOMIPMAP );
        rsc->setCapability( RSC_BLENDING );
        rsc->setCapability( RSC_DOT3 );
        // Cube map
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_10_0 )
        {
            rsc->setCapability( RSC_CUBEMAPPING );
            rsc->setCapability( RSC_READ_BACK_AS_TEXTURE );
        }

        rsc->setCapability( RSC_EXPLICIT_FSAA_RESOLVE );

        // We always support compression, D3DX will decompress if device does not support
        rsc->setCapability( RSC_TEXTURE_COMPRESSION );
        rsc->setCapability( RSC_TEXTURE_COMPRESSION_DXT );

        if( mFeatureLevel >= D3D_FEATURE_LEVEL_10_0 )
            rsc->setCapability( RSC_TWO_SIDED_STENCIL );

        rsc->setCapability( RSC_STENCIL_WRAP );
        rsc->setCapability( RSC_HWOCCLUSION );
        rsc->setCapability( RSC_HWOCCLUSION_ASYNCHRONOUS );

        rsc->setCapability( RSC_TEXTURE_2D_ARRAY );

        if( mFeatureLevel >= D3D_FEATURE_LEVEL_11_0 )
            rsc->setCapability( RSC_MSAA_2D_ARRAY );

        if( mFeatureLevel >= D3D_FEATURE_LEVEL_10_1 )
        {
            rsc->setCapability( RSC_TEXTURE_CUBE_MAP_ARRAY );
            rsc->setCapability( RSC_TEXTURE_GATHER );
        }

        if( mFeatureLevel >= D3D_FEATURE_LEVEL_11_0 )
        {
            rsc->setMaximumResolutions( static_cast<ushort>( D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION ),
                                        static_cast<ushort>( D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION ),
                                        static_cast<ushort>( D3D11_REQ_TEXTURECUBE_DIMENSION ) );
        }
        else if( mFeatureLevel >= D3D_FEATURE_LEVEL_10_0 )
        {
            rsc->setMaximumResolutions( static_cast<ushort>( D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION ),
                                        static_cast<ushort>( D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION ),
                                        static_cast<ushort>( D3D10_REQ_TEXTURECUBE_DIMENSION ) );
        }
        /*TODO
        else if( mFeatureLevel >= D3D_FEATURE_LEVEL_9_3 )
        {
            rsc->setMaximumResolutions( static_cast<ushort>(D3D_FL9_3_REQ_TEXTURE2D_U_OR_V_DIMENSION),
                                        static_cast<ushort>(D3D_FL9_3_REQ_TEXTURE3D_U_V_OR_W_DIMENSION),
                                        static_cast<ushort>(D3D_FL9_3_REQ_TEXTURECUBE_DIMENSION) );
        }
        else
        {
            rsc->setMaximumResolutions( static_cast<ushort>(D3D_FL9_1_REQ_TEXTURE2D_U_OR_V_DIMENSION),
                                        static_cast<ushort>(D3D_FL9_1_REQ_TEXTURE3D_U_V_OR_W_DIMENSION),
                                        static_cast<ushort>(D3D_FL9_1_REQ_TEXTURECUBE_DIMENSION) );
        }*/

        convertVertexShaderCaps( rsc );
        convertPixelShaderCaps( rsc );
        convertGeometryShaderCaps( rsc );
        convertHullShaderCaps( rsc );
        convertDomainShaderCaps( rsc );
        convertComputeShaderCaps( rsc );
        rsc->addShaderProfile( "hlsl" );

        // Check support for dynamic linkage
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_11_0 )
        {
            rsc->setCapability( RSC_SHADER_SUBROUTINE );
        }

        rsc->setCapability( RSC_USER_CLIP_PLANES );
        rsc->setCapability( RSC_VERTEX_FORMAT_UBYTE4 );

        rsc->setCapability( RSC_RTT_SEPARATE_DEPTHBUFFER );
        rsc->setCapability( RSC_RTT_MAIN_DEPTHBUFFER_ATTACHABLE );

        // Adapter details
        const DXGI_ADAPTER_DESC1 &adapterID = mActiveD3DDriver.getAdapterIdentifier();

        switch( mDriverType )
        {
        case D3D_DRIVER_TYPE_HARDWARE:
            // determine vendor
            // Full list of vendors here: http://www.pcidatabase.com/vendors.php?sort=id
            switch( adapterID.VendorId )
            {
            case 0x10DE:
                rsc->setVendor( GPU_NVIDIA );
                break;
            case 0x1002:
                rsc->setVendor( GPU_AMD );
                break;
            case 0x163C:
            case 0x8086:
                rsc->setVendor( GPU_INTEL );
                break;
            case 0x5333:
                rsc->setVendor( GPU_S3 );
                break;
            case 0x3D3D:
                rsc->setVendor( GPU_3DLABS );
                break;
            case 0x102B:
                rsc->setVendor( GPU_MATROX );
                break;
            default:
                rsc->setVendor( GPU_UNKNOWN );
                break;
            };
            break;
        case D3D_DRIVER_TYPE_SOFTWARE:
            rsc->setVendor( GPU_MS_SOFTWARE );
            break;
        case D3D_DRIVER_TYPE_WARP:
            rsc->setVendor( GPU_MS_WARP );
            break;
        default:
            rsc->setVendor( GPU_UNKNOWN );
            break;
        }

        {
            uint32 numTexturesInTextureDescriptor[NumShaderTypes + 1];
            for( size_t i = 0u; i < NumShaderTypes + 1; ++i )
                numTexturesInTextureDescriptor[i] = D3D11_COMMONSHADER_INPUT_RESOURCE_REGISTER_COUNT;
            if( mFeatureLevel < D3D_FEATURE_LEVEL_11_0 )
            {
                for( size_t i = 0u; i < NumShaderTypes + 1; ++i )
                    numTexturesInTextureDescriptor[i] = D3D10_COMMONSHADER_INPUT_RESOURCE_REGISTER_COUNT;
                numTexturesInTextureDescriptor[HullShader] = 0u;
                numTexturesInTextureDescriptor[DomainShader] = 0u;
                numTexturesInTextureDescriptor[NumShaderTypes] = 0u;
            }
            if( mFeatureLevel < D3D_FEATURE_LEVEL_10_0 )
            {
                numTexturesInTextureDescriptor[GeometryShader] = 0u;
            }
            rsc->setNumTexturesInTextureDescriptor( numTexturesInTextureDescriptor );
        }

        rsc->setCapability( RSC_INFINITE_FAR_PLANE );

        rsc->setCapability( RSC_TEXTURE_3D );
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_10_0 )
        {
            rsc->setCapability( RSC_NON_POWER_OF_2_TEXTURES );
            rsc->setCapability( RSC_HWRENDER_TO_TEXTURE_3D );
            rsc->setCapability( RSC_TEXTURE_1D );
            rsc->setCapability( RSC_TEXTURE_COMPRESSION_BC4_BC5 );
            rsc->setCapability( RSC_COMPLETE_TEXTURE_BINDING );
        }

        if( mFeatureLevel >= D3D_FEATURE_LEVEL_11_0 )
        {
            rsc->setCapability( RSC_TEXTURE_COMPRESSION_BC6H_BC7 );
            rsc->setCapability( RSC_UAV );
        }

        rsc->setCapability( RSC_DEPTH_CLAMP );

        rsc->setCapability( RSC_HWRENDER_TO_TEXTURE );
        rsc->setCapability( RSC_TEXTURE_FLOAT );

#ifdef D3D_FEATURE_LEVEL_9_3
        int numMultiRenderTargets =
            ( mFeatureLevel > D3D_FEATURE_LEVEL_9_3 ) ? D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT :  // 8
                ( mFeatureLevel == D3D_FEATURE_LEVEL_9_3 )
                ? 4 /*D3D_FL9_3_SIMULTANEOUS_RENDER_TARGET_COUNT*/
                :   // 4
                1 /*D3D_FL9_1_SIMULTANEOUS_RENDER_TARGET_COUNT*/;  // 1
#else
        int numMultiRenderTargets = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;  // 8
#endif

        rsc->setNumMultiRenderTargets(
            std::min( numMultiRenderTargets, (int)OGRE_MAX_MULTIPLE_RENDER_TARGETS ) );
        rsc->setCapability( RSC_MRT_DIFFERENT_BIT_DEPTHS );

        rsc->setCapability( RSC_POINT_SPRITES );
        rsc->setCapability( RSC_POINT_EXTENDED_PARAMETERS );
        rsc->setMaxPointSize( 256 );  // TODO: guess!

        rsc->setCapability( RSC_VERTEX_TEXTURE_FETCH );
        rsc->setNumVertexTextureUnits( 4 );
        rsc->setVertexTextureUnitsShared( false );

        rsc->setCapability( RSC_MIPMAP_LOD_BIAS );

        // actually irrelevant, but set
        rsc->setCapability( RSC_PERSTAGECONSTANT );

        rsc->setCapability( RSC_VERTEX_BUFFER_INSTANCE_DATA );
        rsc->setCapability( RSC_CAN_GET_COMPILED_SHADER_BUFFER );
        rsc->setCapability( RSC_SEPARATE_SAMPLERS_FROM_TEXTURES );

        rsc->setCapability( RSC_CONST_BUFFER_SLOTS_IN_SHADER );

        return rsc;
    }
    //-----------------------------------------------------------------------
    void D3D11RenderSystem::initialiseFromRenderSystemCapabilities( RenderSystemCapabilities *caps,
                                                                    Window *primary )
    {
        if( caps->getRenderSystemName() != getName() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Trying to initialize D3D11RenderSystem from RenderSystemCapabilities that do "
                         "not support Direct3D11",
                         "D3D11RenderSystem::initialiseFromRenderSystemCapabilities" );
        }

        // add hlsl
        HighLevelGpuProgramManager::getSingleton().addFactory( mHLSLProgramFactory );

        Log *defaultLog = LogManager::getSingleton().getDefaultLog();
        if( defaultLog )
        {
            caps->log( defaultLog );
            defaultLog->logMessage( " * Using Reverse Z: " +
                                    StringConverter::toString( mReverseDepth, true ) );
        }

        mGpuProgramManager->setSaveMicrocodesToCache( true );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::convertVertexShaderCaps( RenderSystemCapabilities *rsc ) const
    {
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_9_1 )
        {
            rsc->addShaderProfile( "vs_4_0_level_9_1" );
#if SUPPORT_SM2_0_HLSL_SHADERS == 1
            rsc->addShaderProfile( "vs_2_0" );
#endif
        }
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_9_3 )
        {
            rsc->addShaderProfile( "vs_4_0_level_9_3" );
#if SUPPORT_SM2_0_HLSL_SHADERS == 1
            rsc->addShaderProfile( "vs_2_a" );
            rsc->addShaderProfile( "vs_2_x" );
#endif
        }
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_10_0 )
        {
            rsc->addShaderProfile( "vs_4_0" );
#if SUPPORT_SM2_0_HLSL_SHADERS == 1
            rsc->addShaderProfile( "vs_3_0" );
#endif
        }
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_10_1 )
        {
            rsc->addShaderProfile( "vs_4_1" );
        }
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_11_0 )
        {
            rsc->addShaderProfile( "vs_5_0" );
        }

        rsc->setCapability( RSC_VERTEX_PROGRAM );

        // TODO: constant buffers have no limits but lower models do
        // 16 boolean params allowed
        rsc->setVertexProgramConstantBoolCount( 16 );
        // 16 integer params allowed, 4D
        rsc->setVertexProgramConstantIntCount( 16 );
        // float params, always 4D
        rsc->setVertexProgramConstantFloatCount( 512 );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::convertPixelShaderCaps( RenderSystemCapabilities *rsc ) const
    {
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_9_1 )
        {
            rsc->addShaderProfile( "ps_4_0_level_9_1" );
#if SUPPORT_SM2_0_HLSL_SHADERS == 1
            rsc->addShaderProfile( "ps_2_0" );
#endif
        }
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_9_3 )
        {
            rsc->addShaderProfile( "ps_4_0_level_9_3" );
#if SUPPORT_SM2_0_HLSL_SHADERS == 1
            rsc->addShaderProfile( "ps_2_a" );
            rsc->addShaderProfile( "ps_2_b" );
            rsc->addShaderProfile( "ps_2_x" );
#endif
        }
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_10_0 )
        {
            rsc->addShaderProfile( "ps_4_0" );
#if SUPPORT_SM2_0_HLSL_SHADERS == 1
            rsc->addShaderProfile( "ps_3_0" );
            rsc->addShaderProfile( "ps_3_x" );
#endif
        }
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_10_1 )
        {
            rsc->addShaderProfile( "ps_4_1" );
        }
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_11_0 )
        {
            rsc->addShaderProfile( "ps_5_0" );
        }

        rsc->setCapability( RSC_FRAGMENT_PROGRAM );

        // TODO: constant buffers have no limits but lower models do
        // 16 boolean params allowed
        rsc->setFragmentProgramConstantBoolCount( 16 );
        // 16 integer params allowed, 4D
        rsc->setFragmentProgramConstantIntCount( 16 );
        // float params, always 4D
        rsc->setFragmentProgramConstantFloatCount( 512 );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::convertHullShaderCaps( RenderSystemCapabilities *rsc ) const
    {
        // Only for shader model 5.0
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_11_0 )
        {
            rsc->addShaderProfile( "hs_5_0" );

            rsc->setCapability( RSC_TESSELLATION_HULL_PROGRAM );

            // TODO: constant buffers have no limits but lower models do
            // 16 boolean params allowed
            rsc->setTessellationHullProgramConstantBoolCount( 16 );
            // 16 integer params allowed, 4D
            rsc->setTessellationHullProgramConstantIntCount( 16 );
            // float params, always 4D
            rsc->setTessellationHullProgramConstantFloatCount( 512 );
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::convertDomainShaderCaps( RenderSystemCapabilities *rsc ) const
    {
        // Only for shader model 5.0
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_11_0 )
        {
            rsc->addShaderProfile( "ds_5_0" );

            rsc->setCapability( RSC_TESSELLATION_DOMAIN_PROGRAM );

            // TODO: constant buffers have no limits but lower models do
            // 16 boolean params allowed
            rsc->setTessellationDomainProgramConstantBoolCount( 16 );
            // 16 integer params allowed, 4D
            rsc->setTessellationDomainProgramConstantIntCount( 16 );
            // float params, always 4D
            rsc->setTessellationDomainProgramConstantFloatCount( 512 );
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::convertComputeShaderCaps( RenderSystemCapabilities *rsc ) const
    {
        //        if (mFeatureLevel >= D3D_FEATURE_LEVEL_10_0)
        //        {
        //            rsc->addShaderProfile("cs_4_0");
        //        }
        //        if (mFeatureLevel >= D3D_FEATURE_LEVEL_10_1)
        //        {
        //            rsc->addShaderProfile("cs_4_1");
        //        }
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_11_0 )
        {
            rsc->addShaderProfile( "cs_5_0" );
            rsc->setCapability( RSC_COMPUTE_PROGRAM );
        }

        // TODO: constant buffers have no limits but lower models do
        // 16 boolean params allowed
        rsc->setComputeProgramConstantBoolCount( 16 );
        // 16 integer params allowed, 4D
        rsc->setComputeProgramConstantIntCount( 16 );
        // float params, always 4D
        rsc->setComputeProgramConstantFloatCount( 512 );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::convertGeometryShaderCaps( RenderSystemCapabilities *rsc ) const
    {
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_10_0 )
        {
            rsc->addShaderProfile( "gs_4_0" );
            rsc->setCapability( RSC_GEOMETRY_PROGRAM );
            rsc->setCapability( RSC_HWRENDER_TO_VERTEX_BUFFER );
        }
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_10_1 )
        {
            rsc->addShaderProfile( "gs_4_1" );
        }
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_11_0 )
        {
            rsc->addShaderProfile( "gs_5_0" );
        }

        rsc->setGeometryProgramConstantFloatCount( 512 );
        rsc->setGeometryProgramConstantIntCount( 16 );
        rsc->setGeometryProgramConstantBoolCount( 16 );
        rsc->setGeometryProgramNumOutputVertices( 1024 );
    }
    //-----------------------------------------------------------------------
    bool D3D11RenderSystem::checkVertexTextureFormats() { return true; }
    //-----------------------------------------------------------------------------------
    RenderPassDescriptor *D3D11RenderSystem::createRenderPassDescriptor()
    {
        RenderPassDescriptor *retVal = OGRE_NEW D3D11RenderPassDescriptor( mDevice, this );
        mRenderPassDescs.insert( retVal );
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void D3D11RenderSystem::beginRenderPassDescriptor( RenderPassDescriptor *desc, TextureGpu *anyTarget,
                                                       uint8 mipLevel, const Vector4 *viewportSizes,
                                                       const Vector4 *scissors, uint32 numViewports,
                                                       bool overlaysEnabled, bool warnIfRtvWasFlushed )
    {
        if( desc->mInformationOnly && desc->hasSameAttachments( mCurrentRenderPassDescriptor ) )
            return;

        const int oldWidth = mCurrentRenderViewport[0].getActualWidth();
        const int oldHeight = mCurrentRenderViewport[0].getActualHeight();
        const int oldX = mCurrentRenderViewport[0].getActualLeft();
        const int oldY = mCurrentRenderViewport[0].getActualTop();

        D3D11RenderPassDescriptor *currPassDesc =
            static_cast<D3D11RenderPassDescriptor *>( mCurrentRenderPassDescriptor );

        RenderSystem::beginRenderPassDescriptor( desc, anyTarget, mipLevel, viewportSizes, scissors,
                                                 numViewports, overlaysEnabled, warnIfRtvWasFlushed );

        int x, y, w, h;

        // Calculate the new "lower-left" corner of the viewport to compare with the old one
        w = mCurrentRenderViewport[0].getActualWidth();
        h = mCurrentRenderViewport[0].getActualHeight();
        x = mCurrentRenderViewport[0].getActualLeft();
        y = mCurrentRenderViewport[0].getActualTop();

        ID3D11DeviceContextN *context = mDevice.GetImmediateContext();

        const bool vpChanged = oldX != x || oldY != y || oldWidth != w || oldHeight != h;

        D3D11RenderPassDescriptor *newPassDesc = static_cast<D3D11RenderPassDescriptor *>( desc );

        // Determine whether:
        //  1. We need to store current active RenderPassDescriptor
        //  2. We need to perform clears when loading the new RenderPassDescriptor
        uint32 entriesToFlush = 0;
        if( currPassDesc )
        {
            entriesToFlush = currPassDesc->willSwitchTo( newPassDesc, warnIfRtvWasFlushed );

            if( entriesToFlush != 0 )
            {
                currPassDesc->performStoreActions( entriesToFlush );

                // Set all textures to 0 to prevent the runtime from thinkin we might
                // be sampling from the render target (common when doing shadow map
                // rendering)
                context->VSSetShaderResources( 0, mMaxSrvCount[VertexShader], mNullViews );
                context->PSSetShaderResources( 0, mMaxSrvCount[PixelShader], mNullViews );
                context->HSSetShaderResources( 0, mMaxSrvCount[HullShader], mNullViews );
                context->DSSetShaderResources( 0, mMaxSrvCount[DomainShader], mNullViews );
                context->GSSetShaderResources( 0, mMaxSrvCount[GeometryShader], mNullViews );
                context->CSSetShaderResources( 0, mMaxComputeShaderSrvCount, mNullViews );
                memset( mMaxSrvCount, 0, sizeof( mMaxSrvCount ) );
                mMaxComputeShaderSrvCount = 0;
            }
        }
        else
        {
            entriesToFlush = RenderPassDescriptor::All;
        }

        if( vpChanged || numViewports > 1u )
        {
            D3D11_VIEWPORT d3dVp[16];
            for( size_t i = 0; i < numViewports; ++i )
            {
                d3dVp[i].TopLeftX = static_cast<FLOAT>( mCurrentRenderViewport[i].getActualLeft() );
                d3dVp[i].TopLeftY = static_cast<FLOAT>( mCurrentRenderViewport[i].getActualTop() );
                d3dVp[i].Width = static_cast<FLOAT>( mCurrentRenderViewport[i].getActualWidth() );
                d3dVp[i].Height = static_cast<FLOAT>( mCurrentRenderViewport[i].getActualHeight() );
                d3dVp[i].MinDepth = 0.0f;
                d3dVp[i].MaxDepth = 1.0f;
            }
            context->RSSetViewports( numViewports, d3dVp );
        }

        D3D11_RECT scRc[16];
        for( size_t i = 0; i < numViewports; ++i )
        {
            scRc[i].left = mCurrentRenderViewport[i].getScissorActualLeft();
            scRc[i].top = mCurrentRenderViewport[i].getScissorActualTop();
            scRc[i].right = scRc[i].left + mCurrentRenderViewport[i].getScissorActualWidth();
            scRc[i].bottom = scRc[i].top + mCurrentRenderViewport[i].getScissorActualHeight();
        }
        context->RSSetScissorRects( numViewports, scRc );

        newPassDesc->performLoadActions( &mCurrentRenderViewport[0], entriesToFlush, mUavStartingSlot,
                                         mUavRenderingDescSet );
    }
    //-----------------------------------------------------------------------------------
    void D3D11RenderSystem::endRenderPassDescriptor()
    {
        if( mCurrentRenderPassDescriptor )
        {
            D3D11RenderPassDescriptor *passDesc =
                static_cast<D3D11RenderPassDescriptor *>( mCurrentRenderPassDescriptor );
            passDesc->performStoreActions( RenderPassDescriptor::All );
        }

        ID3D11DeviceContextN *context = mDevice.GetImmediateContext();
        // Set all textures to 0 to prevent the runtime from thinkin we might
        // be sampling from the render target (common when doing shadow map
        // rendering)
        context->VSSetShaderResources( 0, mMaxSrvCount[VertexShader], mNullViews );
        context->PSSetShaderResources( 0, mMaxSrvCount[PixelShader], mNullViews );
        context->HSSetShaderResources( 0, mMaxSrvCount[HullShader], mNullViews );
        context->DSSetShaderResources( 0, mMaxSrvCount[DomainShader], mNullViews );
        context->GSSetShaderResources( 0, mMaxSrvCount[GeometryShader], mNullViews );
        context->CSSetShaderResources( 0, mMaxComputeShaderSrvCount, mNullViews );
        memset( mMaxSrvCount, 0, sizeof( mMaxSrvCount ) );
        mMaxComputeShaderSrvCount = 0;

        RenderSystem::endRenderPassDescriptor();
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *D3D11RenderSystem::createDepthBufferFor( TextureGpu *colourTexture,
                                                         bool preferDepthTexture,
                                                         PixelFormatGpu depthBufferFormat,
                                                         uint16 poolId )
    {
        if( depthBufferFormat == PFG_UNKNOWN )
        {
            // GeForce 8 & 9 series are faster using 24-bit depth buffers. Likely
            // other HW from that era has the same issue. Assume D3D10.1 is old
            // HW that prefers 24-bit.
            depthBufferFormat = DepthBuffer::DefaultDepthBufferFormat;
        }

        return RenderSystem::createDepthBufferFor( colourTexture, preferDepthTexture, depthBufferFormat,
                                                   poolId );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_notifyWindowDestroyed( Window *window )
    {
        // Check in specialized lists
        if( mPrimaryWindow == window )
        {
            // We're destroying the primary window, so reset device and window
            mPrimaryWindow = NULL;
        }
    }
    //-----------------------------------------------------------------------
    void D3D11RenderSystem::freeDevice()
    {
        if( !mDevice.isNull() && mCurrentCapabilities )
        {
            // Clean up depth stencil surfaces
            mDevice.ReleaseAll();
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::createDevice()
    {
        mDevice.ReleaseAll();

        D3D11Driver *d3dDriver = getDirect3DDrivers( true )->findByName( mDriverName );
        mActiveD3DDriver = *d3dDriver;  // store copy of selected driver, so that it is not
                                        // lost when drivers would be re-enumerated
        LogManager::getSingleton().stream() << "D3D11: Requested \"" << mDriverName << "\", selected \""
                                            << d3dDriver->DriverDescription() << "\"";

        if( D3D11Driver *nvPerfHudDriver = ( mDriverType == D3D_DRIVER_TYPE_HARDWARE && mUseNVPerfHUD )
                                               ? getDirect3DDrivers()->item( "NVIDIA PerfHUD" )
                                               : NULL )
        {
            d3dDriver = nvPerfHudDriver;
            LogManager::getSingleton().logMessage( "D3D11: Actually \"NVIDIA PerfHUD\" is used" );
        }

        ComPtr<ID3D11Device> device;
        createD3D11Device( mVendorExtension, Root::getSingleton().getAppName(), d3dDriver, mDriverType,
                           mMinRequestedFeatureLevel, mMaxRequestedFeatureLevel, &mFeatureLevel,
                           device.GetAddressOf() );
        mDevice.TransferOwnership( device );

        LogManager::getSingleton().stream() << "D3D11: Device Feature Level " << ( mFeatureLevel >> 12 )
                                            << "." << ( ( mFeatureLevel >> 8 ) & 0xF );

        LARGE_INTEGER driverVersion = mDevice.GetDriverVersion();
        mDriverVersion.major = HIWORD( driverVersion.HighPart );
        mDriverVersion.minor = LOWORD( driverVersion.HighPart );
        mDriverVersion.release = HIWORD( driverVersion.LowPart );
        mDriverVersion.build = LOWORD( driverVersion.LowPart );

        // On AMD's GCN cards, there is no performance or memory difference between
        // PF_D24_UNORM_S8_UINT & PF_D32_FLOAT_X24_S8_UINT, so prefer the latter
        // on modern cards (GL >= 4.3) and that also claim to support this format.
        // NVIDIA's preference? Dunno, they don't tell. But at least the quality
        // will be consistent.
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_11_0 )
        {
            selectDepthBufferFormat( DepthBuffer::DFM_D32 | DepthBuffer::DFM_D24 | DepthBuffer::DFM_D16 |
                                     DepthBuffer::DFM_S8 );
        }
        else
        {
            selectDepthBufferFormat( DepthBuffer::DFM_D24 | DepthBuffer::DFM_D16 | DepthBuffer::DFM_S8 );
        }
    }
    //-----------------------------------------------------------------------
    void D3D11RenderSystem::handleDeviceLost()
    {
        LogManager::getSingleton().logMessage( "D3D11: Device was lost, recreating." );

        Timer timer;
        uint64 startTime = timer.getMicroseconds();

        // release device depended resources
        fireDeviceEvent( &mDevice, "DeviceLost" );

        Root::getSingleton().getCompositorManager2()->_releaseManualHardwareResources();
        SceneManagerEnumerator::SceneManagerIterator scnIt =
            SceneManagerEnumerator::getSingleton().getSceneManagerIterator();
        while( scnIt.hasMoreElements() )
            scnIt.getNext()->_releaseManualHardwareResources();

        Root::getSingleton().getHlmsManager()->_changeRenderSystem( (RenderSystem *)0 );

        MeshManager::getSingleton().unloadAll( Resource::LF_MARKED_FOR_RELOAD );

        notifyDeviceLost( &mDevice );

        static_cast<D3D11TextureGpuManager *>( mTextureGpuManager )->_destroyD3DResources();
        static_cast<D3D11VaoManager *>( mVaoManager )->_destroyD3DResources();

        // Release all automatic temporary buffers and free unused
        // temporary buffers, so we doesn't need to recreate them,
        // and they will reallocate on demand.
        v1::HardwareBufferManager::getSingleton()._releaseBufferCopies( true );

        // recreate device
        createDevice();

        static_cast<D3D11VaoManager *>( mVaoManager )->_createD3DResources();
        static_cast<D3D11TextureGpuManager *>( mTextureGpuManager )->_createD3DResources();

        // recreate device depended resources
        notifyDeviceRestored( &mDevice );

        Root::getSingleton().getHlmsManager()->_changeRenderSystem( this );

        v1::MeshManager::getSingleton().reloadAll( Resource::LF_PRESERVE_STATE );
        MeshManager::getSingleton().reloadAll( Resource::LF_MARKED_FOR_RELOAD );

        Root::getSingleton().getCompositorManager2()->_restoreManualHardwareResources();
        scnIt = SceneManagerEnumerator::getSingleton().getSceneManagerIterator();
        while( scnIt.hasMoreElements() )
            scnIt.getNext()->_restoreManualHardwareResources();

        fireDeviceEvent( &mDevice, "DeviceRestored" );

        uint64 passedTime = ( timer.getMicroseconds() - startTime ) / 1000;
        LogManager::getSingleton().logMessage( "D3D11: Device was restored in " +
                                               StringConverter::toString( passedTime ) + "ms" );
    }
    //---------------------------------------------------------------------
    bool D3D11RenderSystem::isDeviceLost() { return !mDevice.isNull() && mDevice.IsDeviceLost(); }
    //---------------------------------------------------------------------
    bool D3D11RenderSystem::validateDevice( bool forceDeviceElection )
    {
        if( mDevice.isNull() )
            return false;

        // The D3D Device is no longer valid if the elected adapter changes or if
        // the device has been removed.

        bool anotherIsElected = false;
        if( forceDeviceElection )
        {
            // elect new device
            D3D11Driver *newDriver = getDirect3DDrivers( true )->findByName( mDriverName );

            // check by LUID
            LUID newLUID = newDriver->getAdapterIdentifier().AdapterLuid;
            LUID prevLUID = mActiveD3DDriver.getAdapterIdentifier().AdapterLuid;
            anotherIsElected =
                ( newLUID.LowPart != prevLUID.LowPart ) || ( newLUID.HighPart != prevLUID.HighPart );
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
            anotherIsElected = true;  // simulate switching device
#endif
        }

        if( anotherIsElected || mDevice.IsDeviceLost() )
        {
            handleDeviceLost();

            return !mDevice.IsDeviceLost();
        }

        return true;
    }
    //---------------------------------------------------------------------
    VertexElementType D3D11RenderSystem::getColourVertexElementType() const { return VET_COLOUR_ABGR; }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_useLights( const LightList &lights, unsigned short limit ) {}
    //---------------------------------------------------------------------
    void D3D11RenderSystem::setShadingType( ShadeOptions so ) {}
    //---------------------------------------------------------------------
    void D3D11RenderSystem::setLightingEnabled( bool enabled ) {}
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setViewMatrix( const Matrix4 &m ) {}
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setProjectionMatrix( const Matrix4 &m ) {}
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setWorldMatrix( const Matrix4 &m ) {}
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setSurfaceParams( const ColourValue &ambient, const ColourValue &diffuse,
                                               const ColourValue &specular, const ColourValue &emissive,
                                               Real shininess, TrackVertexColourType tracking )
    {
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setPointParameters( Real size, bool attenuationEnabled, Real constant,
                                                 Real linear, Real quadratic, Real minSize,
                                                 Real maxSize )
    {
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setPointSpritesEnabled( bool enabled ) {}
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setTexture( size_t stage, TextureGpu *texPtr, bool bDepthReadOnly )
    {
        if( texPtr )
        {
            D3D11TextureGpu *tex = static_cast<D3D11TextureGpu *>( texPtr );
            ID3D11ShaderResourceView *view = tex->getDefaultDisplaySrv();
            mDevice.GetImmediateContext()->VSSetShaderResources( static_cast<UINT>( stage ), 1u, &view );
            mDevice.GetImmediateContext()->PSSetShaderResources( static_cast<UINT>( stage ), 1u, &view );
            mMaxSrvCount[VertexShader] = std::max( mMaxSrvCount[VertexShader], uint32( stage + 1u ) );
            mMaxSrvCount[PixelShader] = std::max( mMaxSrvCount[PixelShader], uint32( stage + 1u ) );
        }
        else
        {
            ID3D11ShaderResourceView *nullView = 0;
            mDevice.GetImmediateContext()->VSSetShaderResources( static_cast<UINT>( stage ), 1u,
                                                                 &nullView );
            mDevice.GetImmediateContext()->PSSetShaderResources( static_cast<UINT>( stage ), 1u,
                                                                 &nullView );
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setTextures( uint32 slotStart, const DescriptorSetTexture *set,
                                          uint32 hazardousTexIdx )
    {
        ID3D11DeviceContextN *context = mDevice.GetImmediateContext();
        ComPtr<ID3D11ShaderResourceView> *srvList =
            reinterpret_cast<ComPtr<ID3D11ShaderResourceView> *>( set->mRsData );

        ComPtr<ID3D11ShaderResourceView> hazardousSrv;
        if( hazardousTexIdx < set->mTextures.size() )
        {
            // Is the texture currently bound as RTT?
            if( mCurrentRenderPassDescriptor->hasAttachment( set->mTextures[hazardousTexIdx] ) )
            {
                // Then do not set it!
                srvList[hazardousTexIdx].Swap( hazardousSrv );
            }
        }

        UINT texIdx = 0;
        for( size_t i = 0u; i < NumShaderTypes; ++i )
        {
            const UINT numTexturesUsed = set->mShaderTypeTexCount[i];
            if( !numTexturesUsed )
                continue;

            switch( i )
            {
            case VertexShader:
                context->VSSetShaderResources( slotStart + texIdx, numTexturesUsed,
                                               srvList[texIdx].GetAddressOf() );
                break;
            case PixelShader:
                context->PSSetShaderResources( slotStart + texIdx, numTexturesUsed,
                                               srvList[texIdx].GetAddressOf() );
                break;
            case GeometryShader:
                context->GSSetShaderResources( slotStart + texIdx, numTexturesUsed,
                                               srvList[texIdx].GetAddressOf() );
                break;
            case HullShader:
                context->HSSetShaderResources( slotStart + texIdx, numTexturesUsed,
                                               srvList[texIdx].GetAddressOf() );
                break;
            case DomainShader:
                context->DSSetShaderResources( slotStart + texIdx, numTexturesUsed,
                                               srvList[texIdx].GetAddressOf() );
                break;
            }

            mMaxSrvCount[i] = std::max( mMaxSrvCount[i], slotStart + texIdx + numTexturesUsed );

            texIdx += numTexturesUsed;
        }

        // Restore the SRV with the hazardous texture.
        if( hazardousSrv )
            srvList[hazardousTexIdx].Swap( hazardousSrv );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setTextures( uint32 slotStart, const DescriptorSetTexture2 *set )
    {
        ID3D11DeviceContextN *context = mDevice.GetImmediateContext();
        ComPtr<ID3D11ShaderResourceView> *srvList =
            reinterpret_cast<ComPtr<ID3D11ShaderResourceView> *>( set->mRsData );
        UINT texIdx = 0;
        for( size_t i = 0u; i < NumShaderTypes; ++i )
        {
            const UINT numTexturesUsed = set->mShaderTypeTexCount[i];
            if( !numTexturesUsed )
                continue;

            switch( i )
            {
            case VertexShader:
                context->VSSetShaderResources( slotStart + texIdx, numTexturesUsed,
                                               srvList[texIdx].GetAddressOf() );
                break;
            case PixelShader:
                context->PSSetShaderResources( slotStart + texIdx, numTexturesUsed,
                                               srvList[texIdx].GetAddressOf() );
                break;
            case GeometryShader:
                context->GSSetShaderResources( slotStart + texIdx, numTexturesUsed,
                                               srvList[texIdx].GetAddressOf() );
                break;
            case HullShader:
                context->HSSetShaderResources( slotStart + texIdx, numTexturesUsed,
                                               srvList[texIdx].GetAddressOf() );
                break;
            case DomainShader:
                context->DSSetShaderResources( slotStart + texIdx, numTexturesUsed,
                                               srvList[texIdx].GetAddressOf() );
                break;
            }

            mMaxSrvCount[i] = std::max( mMaxSrvCount[i], slotStart + texIdx + numTexturesUsed );

            texIdx += numTexturesUsed;
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setSamplers( uint32 slotStart, const DescriptorSetSampler *set )
    {
        ID3D11SamplerState *samplers[32];

        FastArray<const HlmsSamplerblock *>::const_iterator itor = set->mSamplers.begin();

        ID3D11DeviceContextN *context = mDevice.GetImmediateContext();
        UINT samplerIdx = slotStart;
        for( size_t i = 0u; i < NumShaderTypes; ++i )
        {
            const UINT numSamplersUsed = set->mShaderTypeSamplerCount[i];

            if( !numSamplersUsed )
                continue;

            for( size_t j = 0; j < numSamplersUsed; ++j )
            {
                if( *itor )
                {
                    ID3D11SamplerState *samplerState =
                        reinterpret_cast<ID3D11SamplerState *>( ( *itor )->mRsData );
                    samplers[j] = samplerState;
                }
                else
                {
                    samplers[j] = 0;
                }
                ++itor;
            }

            switch( i )
            {
            case VertexShader:
                context->VSSetSamplers( samplerIdx, numSamplersUsed, samplers );
                break;
            case PixelShader:
                context->PSSetSamplers( samplerIdx, numSamplersUsed, samplers );
                break;
            case GeometryShader:
                context->GSSetSamplers( samplerIdx, numSamplersUsed, samplers );
                break;
            case HullShader:
                context->HSSetSamplers( samplerIdx, numSamplersUsed, samplers );
                break;
            case DomainShader:
                context->DSSetSamplers( samplerIdx, numSamplersUsed, samplers );
                break;
            }

            samplerIdx += numSamplersUsed;
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setTexturesCS( uint32 slotStart, const DescriptorSetTexture *set )
    {
        const uint32 oldSrvCount = mMaxComputeShaderSrvCount;
        uint32 newSrvCount = 0;

        ID3D11DeviceContextN *context = mDevice.GetImmediateContext();
        ComPtr<ID3D11ShaderResourceView> *srvList =
            reinterpret_cast<ComPtr<ID3D11ShaderResourceView> *>( set->mRsData );
        UINT texIdx = 0;
        for( size_t i = 0u; i < NumShaderTypes; ++i )
        {
            const UINT numTexturesUsed = set->mShaderTypeTexCount[i];
            if( !numTexturesUsed )
                continue;

            context->CSSetShaderResources( slotStart + texIdx, numTexturesUsed,
                                           srvList[texIdx].GetAddressOf() );

            mMaxComputeShaderSrvCount =
                std::max( mMaxComputeShaderSrvCount, slotStart + texIdx + numTexturesUsed );
            texIdx += numTexturesUsed;
        }

        // We must unbound old textures otherwise they could clash with the next _setUavCS call
        if( newSrvCount < oldSrvCount )
        {
            const uint32 excessSlots = oldSrvCount - newSrvCount;
            context->CSSetShaderResources( newSrvCount, excessSlots, mNullViews );
        }

        mMaxComputeShaderSrvCount = newSrvCount;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setTexturesCS( uint32 slotStart, const DescriptorSetTexture2 *set )
    {
        const uint32 oldSrvCount = mMaxComputeShaderSrvCount;
        uint32 newSrvCount = 0;

        ID3D11DeviceContextN *context = mDevice.GetImmediateContext();
        ComPtr<ID3D11ShaderResourceView> *srvList =
            reinterpret_cast<ComPtr<ID3D11ShaderResourceView> *>( set->mRsData );
        UINT texIdx = 0;
        for( size_t i = 0u; i < NumShaderTypes; ++i )
        {
            const UINT numTexturesUsed = set->mShaderTypeTexCount[i];
            if( !numTexturesUsed )
                continue;

            context->CSSetShaderResources( slotStart + texIdx, numTexturesUsed,
                                           srvList[texIdx].GetAddressOf() );

            newSrvCount = std::max( newSrvCount, slotStart + texIdx + numTexturesUsed );
            texIdx += numTexturesUsed;
        }

        // We must unbound old textures otherwise they could clash with the next _setUavCS call
        if( newSrvCount < oldSrvCount )
        {
            const uint32 excessSlots = oldSrvCount - newSrvCount;
            context->CSSetShaderResources( newSrvCount, excessSlots, mNullViews );
        }

        mMaxComputeShaderSrvCount = newSrvCount;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setSamplersCS( uint32 slotStart, const DescriptorSetSampler *set )
    {
        ID3D11SamplerState *samplers[32];

        FastArray<const HlmsSamplerblock *>::const_iterator itor = set->mSamplers.begin();

        ID3D11DeviceContextN *context = mDevice.GetImmediateContext();
        UINT samplerIdx = slotStart;
        for( size_t i = 0u; i < NumShaderTypes; ++i )
        {
            const UINT numSamplersUsed = set->mShaderTypeSamplerCount[i];

            if( !numSamplersUsed )
                continue;

            for( size_t j = 0; j < numSamplersUsed; ++j )
            {
                if( *itor )
                {
                    ID3D11SamplerState *samplerState =
                        reinterpret_cast<ID3D11SamplerState *>( ( *itor )->mRsData );
                    samplers[j] = samplerState;
                }
                else
                {
                    samplers[j] = 0;
                }
                ++itor;
            }

            context->CSSetSamplers( samplerIdx, numSamplersUsed, samplers );
            samplerIdx += numSamplersUsed;
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setUavCS( uint32 slotStart, const DescriptorSetUav *set )
    {
        ComPtr<ID3D11UnorderedAccessView> *uavList =
            reinterpret_cast<ComPtr<ID3D11UnorderedAccessView> *>( set->mRsData );
        ID3D11DeviceContextN *context = mDevice.GetImmediateContext();
        context->CSSetUnorderedAccessViews( slotStart, static_cast<UINT>( set->mUavs.size() ),
                                            uavList[0].GetAddressOf(), 0 );

        mMaxBoundUavCS = std::max( mMaxBoundUavCS, uint32( slotStart + set->mUavs.size() ) );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setBindingType( TextureUnitState::BindingType bindingType )
    {
        mBindingType = bindingType;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setVertexTexture( size_t stage, TextureGpu *tex )
    {
        _setTexture( stage, tex, false );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setGeometryTexture( size_t stage, TextureGpu *tex )
    {
        _setTexture( stage, tex, false );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setTessellationHullTexture( size_t stage, TextureGpu *tex )
    {
        _setTexture( stage, tex, false );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setTessellationDomainTexture( size_t stage, TextureGpu *tex )
    {
        _setTexture( stage, tex, false );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setTextureCoordCalculation( size_t stage, TexCoordCalcMethod m,
                                                         const Frustum *frustum )
    {
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setTextureMatrix( size_t stage, const Matrix4 &xForm ) {}
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setTextureBlendMode( size_t stage, const LayerBlendModeEx &bm ) {}
    //---------------------------------------------------------------------
    void D3D11RenderSystem::setStencilBufferParams( uint32 refValue, const StencilParams &stencilParams )
    {
        RenderSystem::setStencilBufferParams( refValue, stencilParams );

        mStencilRef = refValue;
    }
    //---------------------------------------------------------------------
    bool D3D11RenderSystem::_hlmsPipelineStateObjectCreated( HlmsPso *block, uint64 deadline )
    {
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        debugLogPso( block );
#endif

        D3D11HlmsPso *pso = new D3D11HlmsPso();
        memset( pso, 0, sizeof( D3D11HlmsPso ) );

        D3D11_DEPTH_STENCIL_DESC depthStencilDesc;

        ZeroMemory( &depthStencilDesc, sizeof( D3D11_DEPTH_STENCIL_DESC ) );
        depthStencilDesc.DepthEnable = block->macroblock->mDepthCheck;
        depthStencilDesc.DepthWriteMask =
            block->macroblock->mDepthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
        CompareFunction depthFunc = block->macroblock->mDepthFunc;
        if( mReverseDepth )
            depthFunc = reverseCompareFunction( depthFunc );
        depthStencilDesc.DepthFunc = D3D11Mappings::get( depthFunc );
        depthStencilDesc.StencilEnable = block->pass.stencilParams.enabled;
        depthStencilDesc.StencilReadMask = block->pass.stencilParams.readMask;
        depthStencilDesc.StencilWriteMask = block->pass.stencilParams.writeMask;
        const StencilStateOp &stateFront = block->pass.stencilParams.stencilFront;
        depthStencilDesc.FrontFace.StencilFunc = D3D11Mappings::get( stateFront.compareOp );
        depthStencilDesc.FrontFace.StencilDepthFailOp =
            D3D11Mappings::get( stateFront.stencilDepthFailOp );
        depthStencilDesc.FrontFace.StencilPassOp = D3D11Mappings::get( stateFront.stencilPassOp );
        depthStencilDesc.FrontFace.StencilFailOp = D3D11Mappings::get( stateFront.stencilFailOp );
        const StencilStateOp &stateBack = block->pass.stencilParams.stencilBack;
        depthStencilDesc.BackFace.StencilFunc = D3D11Mappings::get( stateBack.compareOp );
        depthStencilDesc.BackFace.StencilDepthFailOp =
            D3D11Mappings::get( stateBack.stencilDepthFailOp );
        depthStencilDesc.BackFace.StencilPassOp = D3D11Mappings::get( stateBack.stencilPassOp );
        depthStencilDesc.BackFace.StencilFailOp = D3D11Mappings::get( stateBack.stencilFailOp );

        HRESULT hr =
            mDevice->CreateDepthStencilState( &depthStencilDesc, pso->depthStencilState.GetAddressOf() );
        if( FAILED( hr ) )
        {
            delete pso;
            pso = 0;

            String errorDescription = mDevice.getErrorDescription( hr );
            OGRE_EXCEPT_EX(
                Exception::ERR_RENDERINGAPI_ERROR, hr,
                "Failed to create depth stencil state\nError Description: " + errorDescription,
                "D3D11RenderSystem::_hlmsPipelineStateObjectCreated" );
        }

        const bool useTesselation = (bool)block->tesselationDomainShader;

        int operationType = block->operationType;
        if( block->geometryShader && block->geometryShader->isAdjacencyInfoRequired() )
            operationType |= OT_DETAIL_ADJACENCY_BIT;

        switch( operationType )
        {
        case OT_POINT_LIST:
            pso->topology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
            break;
        case OT_LINE_LIST:
            if( useTesselation )
                pso->topology = D3D11_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST;
            else
                pso->topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
            break;
        case OT_LINE_LIST_ADJ:
            if( useTesselation )
                pso->topology = D3D11_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST;
            else
                pso->topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
            break;
        case OT_LINE_STRIP:
            if( useTesselation )
                pso->topology = D3D11_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST;
            else
                pso->topology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
            break;
        case OT_LINE_STRIP_ADJ:
            if( useTesselation )
                pso->topology = D3D11_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST;
            else
                pso->topology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
            break;
        default:
        case OT_TRIANGLE_LIST:
            if( useTesselation )
                pso->topology = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
            else
                pso->topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            break;
        case OT_TRIANGLE_LIST_ADJ:
            if( useTesselation )
                pso->topology = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
            else
                pso->topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
            break;
        case OT_TRIANGLE_STRIP:
            if( useTesselation )
                pso->topology = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
            else
                pso->topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            break;
        case OT_TRIANGLE_STRIP_ADJ:
            if( useTesselation )
                pso->topology = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
            else
                pso->topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
            break;
        case OT_TRIANGLE_FAN:
            pso->topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

            delete pso;
            pso = 0;
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Error - DX11 render - no support for triangle fan (OT_TRIANGLE_FAN)",
                         "D3D11RenderSystem::_hlmsPipelineStateObjectCreated" );
            break;
        }

        // No subroutines for now
        if( block->vertexShader )
        {
            pso->vertexShader =
                static_cast<D3D11HLSLProgram *>( block->vertexShader->_getBindingDelegate() );
        }
        if( block->geometryShader )
        {
            pso->geometryShader =
                static_cast<D3D11HLSLProgram *>( block->geometryShader->_getBindingDelegate() );
        }
        if( mFeatureLevel >= D3D_FEATURE_LEVEL_11_0 )
        {
            if( block->tesselationHullShader )
            {
                pso->hullShader = static_cast<D3D11HLSLProgram *>(
                    block->tesselationHullShader->_getBindingDelegate() );
            }
            if( block->tesselationDomainShader )
            {
                pso->domainShader = static_cast<D3D11HLSLProgram *>(
                    block->tesselationDomainShader->_getBindingDelegate() );
            }

            // Check consistency of tessellation shaders
            if( (bool)block->tesselationHullShader != (bool)block->tesselationDomainShader )
            {
                delete pso;
                pso = 0;
                if( !block->tesselationHullShader )
                {
                    OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                                 "Attempted to use tessellation, but domain shader is missing",
                                 "D3D11RenderSystem::_hlmsPipelineStateObjectCreated" );
                }
                else
                {
                    OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                                 "Attempted to use tessellation, but hull shader is missing",
                                 "D3D11RenderSystem::_hlmsPipelineStateObjectCreated" );
                }
            }
        }
        if( block->pixelShader &&
            block->blendblock->mBlendChannelMask != HlmsBlendblock::BlendChannelForceDisabled )
        {
            pso->pixelShader =
                static_cast<D3D11HLSLProgram *>( block->pixelShader->_getBindingDelegate() );
        }

        if( pso->vertexShader )
        {
            try
            {
                pso->inputLayout = pso->vertexShader->getLayoutForPso( block->vertexElements );
            }
            catch( Exception & )
            {
                delete pso;
                pso = 0;
                throw;
            }
        }

        try
        {
            createBlendState( block->blendblock, block->pass.sampleDescription.isMultisample(),
                              pso->blendState );
        }
        catch( Exception & )
        {
            delete pso;
            pso = 0;
            throw;
        }

        block->rsData = pso;
        return true;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_hlmsPipelineStateObjectDestroyed( HlmsPso *pso )
    {
        D3D11HlmsPso *d3dPso = reinterpret_cast<D3D11HlmsPso *>( pso->rsData );
        delete d3dPso;
        pso->rsData = 0;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_hlmsMacroblockCreated( HlmsMacroblock *newBlock )
    {
        D3D11_RASTERIZER_DESC rasterDesc;
        switch( newBlock->mCullMode )
        {
        case CULL_NONE:
            rasterDesc.CullMode = D3D11_CULL_NONE;
            break;
        default:
        case CULL_CLOCKWISE:
            rasterDesc.CullMode = D3D11_CULL_BACK;
            break;
        case CULL_ANTICLOCKWISE:
            rasterDesc.CullMode = D3D11_CULL_FRONT;
            break;
        }

        // This should/will be done in a geometry shader like in the FixedFuncEMU sample and the shader
        // needs solid
        rasterDesc.FillMode =
            newBlock->mPolygonMode == PM_WIREFRAME ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;

        rasterDesc.FrontCounterClockwise = true;

        const float nearFarFactor = 10.0;
        const float biasSign = mReverseDepth ? 1.0f : -1.0f;
        rasterDesc.DepthBias =
            static_cast<int>( nearFarFactor * biasSign * newBlock->mDepthBiasConstant );
        rasterDesc.SlopeScaledDepthBias = newBlock->mDepthBiasSlopeScale * biasSign;
        rasterDesc.DepthBiasClamp = 0;

        rasterDesc.DepthClipEnable = !newBlock->mDepthClamp;
        rasterDesc.ScissorEnable = newBlock->mScissorTestEnabled;

        rasterDesc.MultisampleEnable = true;
        rasterDesc.AntialiasedLineEnable = false;

        ID3D11RasterizerState *rasterizerState = 0;

        HRESULT hr = mDevice->CreateRasterizerState( &rasterDesc, &rasterizerState );
        if( FAILED( hr ) )
        {
            String errorDescription = mDevice.getErrorDescription( hr );
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Failed to create rasterizer state\nError Description: " + errorDescription,
                            "D3D11RenderSystem::_hlmsMacroblockCreated" );
        }

        newBlock->mRsData = rasterizerState;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_hlmsMacroblockDestroyed( HlmsMacroblock *block )
    {
        ID3D11RasterizerState *rasterizerState =
            reinterpret_cast<ID3D11RasterizerState *>( block->mRsData );
        rasterizerState->Release();
        block->mRsData = 0;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::createBlendState( const HlmsBlendblock *newBlock, const bool bIsMultisample,
                                              ComPtr<ID3D11BlendState> &outBlendblock )
    {
        D3D11_BLEND_DESC blendDesc;
        ZeroMemory( &blendDesc, sizeof( D3D11_BLEND_DESC ) );
        blendDesc.IndependentBlendEnable = false;
        blendDesc.RenderTarget[0].BlendEnable = newBlock->mBlendOperation;

        blendDesc.RenderTarget[0].RenderTargetWriteMask =
            newBlock->mBlendChannelMask & HlmsBlendblock::BlendChannelAll;

        if( newBlock->mSeparateBlend )
        {
            if( newBlock->mSourceBlendFactor == SBF_ONE && newBlock->mDestBlendFactor == SBF_ZERO &&
                newBlock->mSourceBlendFactorAlpha == SBF_ONE &&
                newBlock->mDestBlendFactorAlpha == SBF_ZERO )
            {
                blendDesc.RenderTarget[0].BlendEnable = FALSE;
            }
            else
            {
                blendDesc.RenderTarget[0].BlendEnable = TRUE;
                blendDesc.RenderTarget[0].SrcBlend =
                    D3D11Mappings::get( newBlock->mSourceBlendFactor, false );
                blendDesc.RenderTarget[0].DestBlend =
                    D3D11Mappings::get( newBlock->mDestBlendFactor, false );
                blendDesc.RenderTarget[0].SrcBlendAlpha =
                    D3D11Mappings::get( newBlock->mSourceBlendFactorAlpha, true );
                blendDesc.RenderTarget[0].DestBlendAlpha =
                    D3D11Mappings::get( newBlock->mDestBlendFactorAlpha, true );
                blendDesc.RenderTarget[0].BlendOp = blendDesc.RenderTarget[0].BlendOpAlpha =
                    D3D11Mappings::get( newBlock->mBlendOperation );

                blendDesc.RenderTarget[0].RenderTargetWriteMask =
                    newBlock->mBlendChannelMask & HlmsBlendblock::BlendChannelAll;
            }
        }
        else
        {
            if( newBlock->mSourceBlendFactor == SBF_ONE && newBlock->mDestBlendFactor == SBF_ZERO )
            {
                blendDesc.RenderTarget[0].BlendEnable = FALSE;
            }
            else
            {
                blendDesc.RenderTarget[0].BlendEnable = TRUE;
                blendDesc.RenderTarget[0].SrcBlend =
                    D3D11Mappings::get( newBlock->mSourceBlendFactor, false );
                blendDesc.RenderTarget[0].DestBlend =
                    D3D11Mappings::get( newBlock->mDestBlendFactor, false );
                blendDesc.RenderTarget[0].SrcBlendAlpha =
                    D3D11Mappings::get( newBlock->mSourceBlendFactor, true );
                blendDesc.RenderTarget[0].DestBlendAlpha =
                    D3D11Mappings::get( newBlock->mDestBlendFactor, true );
                blendDesc.RenderTarget[0].BlendOp = D3D11Mappings::get( newBlock->mBlendOperation );
                blendDesc.RenderTarget[0].BlendOpAlpha =
                    D3D11Mappings::get( newBlock->mBlendOperationAlpha );

                blendDesc.RenderTarget[0].RenderTargetWriteMask =
                    newBlock->mBlendChannelMask & HlmsBlendblock::BlendChannelAll;
            }
        }

        // feature level 9 and below does not support alpha to coverage.
        if( mFeatureLevel < D3D_FEATURE_LEVEL_10_0 )
            blendDesc.AlphaToCoverageEnable = false;
        else
        {
            blendDesc.AlphaToCoverageEnable =
                newBlock->mAlphaToCoverage == HlmsBlendblock::A2cEnabled ||
                ( newBlock->mAlphaToCoverage == HlmsBlendblock::A2cEnabledMsaaOnly && bIsMultisample );
        }

        ID3D11BlendState *blendState = 0;

        HRESULT hr = mDevice->CreateBlendState( &blendDesc, outBlendblock.GetAddressOf() );
        if( FAILED( hr ) )
        {
            String errorDescription = mDevice.getErrorDescription( hr );
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Failed to create blend state\nError Description: " + errorDescription,
                            "D3D11RenderSystem::createBlendState" );
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_hlmsSamplerblockCreated( HlmsSamplerblock *newBlock )
    {
        D3D11_SAMPLER_DESC samplerDesc;
        ZeroMemory( &samplerDesc, sizeof( D3D11_SAMPLER_DESC ) );
        samplerDesc.Filter =
            D3D11Mappings::get( newBlock->mMinFilter, newBlock->mMagFilter, newBlock->mMipFilter,
                                newBlock->mCompareFunction != NUM_COMPARE_FUNCTIONS );
        if( newBlock->mCompareFunction == NUM_COMPARE_FUNCTIONS )
            samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        else
            samplerDesc.ComparisonFunc = D3D11Mappings::get( newBlock->mCompareFunction );

        samplerDesc.AddressU = D3D11Mappings::get( newBlock->mU );
        samplerDesc.AddressV = D3D11Mappings::get( newBlock->mV );
        samplerDesc.AddressW = D3D11Mappings::get( newBlock->mW );

        samplerDesc.MipLODBias = newBlock->mMipLodBias;
        samplerDesc.MaxAnisotropy = std::min( (UINT)newBlock->mMaxAnisotropy,
                                              (UINT)getCapabilities()->getMaxSupportedAnisotropy() );
        samplerDesc.BorderColor[0] = newBlock->mBorderColour.r;
        samplerDesc.BorderColor[1] = newBlock->mBorderColour.g;
        samplerDesc.BorderColor[2] = newBlock->mBorderColour.b;
        samplerDesc.BorderColor[3] = newBlock->mBorderColour.a;
        samplerDesc.MinLOD = newBlock->mMinLod;
        samplerDesc.MaxLOD = newBlock->mMaxLod;

        ID3D11SamplerState *samplerState = 0;

        HRESULT hr = mDevice->CreateSamplerState( &samplerDesc, &samplerState );
        if( FAILED( hr ) )
        {
            String errorDescription = mDevice.getErrorDescription( hr );
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Failed to create sampler state\nError Description: " + errorDescription,
                            "D3D11RenderSystem::_hlmsSamplerblockCreated" );
        }

        newBlock->mRsData = samplerState;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_hlmsSamplerblockDestroyed( HlmsSamplerblock *block )
    {
        ID3D11SamplerState *samplerState = reinterpret_cast<ID3D11SamplerState *>( block->mRsData );
        samplerState->Release();
        block->mRsData = 0;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_descriptorSetTextureCreated( DescriptorSetTexture *newSet )
    {
        const size_t numElements = newSet->mTextures.size();
        ComPtr<ID3D11ShaderResourceView> *srvList = new ComPtr<ID3D11ShaderResourceView>[numElements];
        newSet->mRsData = srvList;

        size_t texIdx = 0;
        FastArray<const TextureGpu *>::const_iterator itor = newSet->mTextures.begin();

        for( size_t i = 0u; i < NumShaderTypes; ++i )
        {
            const size_t numTexturesUsed = newSet->mShaderTypeTexCount[i];
            for( size_t j = 0u; j < numTexturesUsed; ++j )
            {
                if( *itor )
                {
                    const D3D11TextureGpu *texture = static_cast<const D3D11TextureGpu *>( *itor );
                    srvList[texIdx] = texture->createSrv();
                }

                ++texIdx;
                ++itor;
            }
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_descriptorSetTextureDestroyed( DescriptorSetTexture *set )
    {
        const size_t numElements = set->mTextures.size();
        ComPtr<ID3D11ShaderResourceView> *srvList =
            reinterpret_cast<ComPtr<ID3D11ShaderResourceView> *>( set->mRsData );

        delete[] srvList;
        set->mRsData = 0;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_descriptorSetTexture2Created( DescriptorSetTexture2 *newSet )
    {
        const size_t numElements = newSet->mTextures.size();
        ComPtr<ID3D11ShaderResourceView> *srvList = new ComPtr<ID3D11ShaderResourceView>[numElements];
        newSet->mRsData = srvList;

        FastArray<DescriptorSetTexture2::Slot>::const_iterator itor = newSet->mTextures.begin();

        for( size_t i = 0u; i < numElements; ++i )
        {
            if( itor->isTexture() )
            {
                const DescriptorSetTexture2::TextureSlot &texSlot = itor->getTexture();
                const D3D11TextureGpu *texture = static_cast<const D3D11TextureGpu *>( texSlot.texture );
                srvList[i] = texture->createSrv( texSlot );
            }
            else
            {
                const DescriptorSetTexture2::BufferSlot &bufferSlot = itor->getBuffer();
                if( bufferSlot.buffer->getBufferPackedType() == BP_TYPE_TEX )
                {
                    const D3D11TexBufferPacked *texBuffer =
                        static_cast<const D3D11TexBufferPacked *>( bufferSlot.buffer );
                    srvList[i] = texBuffer->createSrv( bufferSlot );
                }
                else
                {
                    const D3D11ReadOnlyBufferPacked *roBuffer =
                        static_cast<const D3D11ReadOnlyBufferPacked *>( bufferSlot.buffer );
                    srvList[i] = roBuffer->createSrv( bufferSlot );
                }
            }

            ++itor;
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_descriptorSetTexture2Destroyed( DescriptorSetTexture2 *set )
    {
        const size_t numElements = set->mTextures.size();
        ComPtr<ID3D11ShaderResourceView> *srvList =
            reinterpret_cast<ComPtr<ID3D11ShaderResourceView> *>( set->mRsData );

        delete[] srvList;
        set->mRsData = 0;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_descriptorSetUavCreated( DescriptorSetUav *newSet )
    {
        const size_t numElements = newSet->mUavs.size();
        ComPtr<ID3D11UnorderedAccessView> *uavList = new ComPtr<ID3D11UnorderedAccessView>[numElements];
        newSet->mRsData = uavList;

        FastArray<DescriptorSetUav::Slot>::const_iterator itor = newSet->mUavs.begin();

        for( size_t i = 0u; i < numElements; ++i )
        {
            if( itor->empty() )
                ;
            else if( itor->isTexture() )
            {
                const DescriptorSetUav::TextureSlot &texSlot = itor->getTexture();
                const D3D11TextureGpu *texture = static_cast<const D3D11TextureGpu *>( texSlot.texture );
                uavList[i] = texture->createUav( texSlot );
            }
            else
            {
                const DescriptorSetUav::BufferSlot &bufferSlot = itor->getBuffer();
                const D3D11UavBufferPacked *uavBuffer =
                    static_cast<const D3D11UavBufferPacked *>( bufferSlot.buffer );
                uavList[i] = uavBuffer->createUav( bufferSlot );
            }

            ++itor;
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_descriptorSetUavDestroyed( DescriptorSetUav *set )
    {
        const size_t numElements = set->mUavs.size();
        ComPtr<ID3D11UnorderedAccessView> *uavList =
            reinterpret_cast<ComPtr<ID3D11UnorderedAccessView> *>( set->mRsData );

        delete[] uavList;
        set->mRsData = 0;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setHlmsMacroblock( const HlmsMacroblock *macroblock )
    {
        assert( macroblock->mRsData &&
                "The block must have been created via HlmsManager::getMacroblock!" );

        ID3D11RasterizerState *rasterizerState =
            reinterpret_cast<ID3D11RasterizerState *>( macroblock->mRsData );

        mDevice.GetImmediateContext()->RSSetState( rasterizerState );
        if( mDevice.isError() )
        {
            String errorDescription = mDevice.getErrorDescription();
            OGRE_EXCEPT(
                Exception::ERR_RENDERINGAPI_ERROR,
                "D3D11 device cannot set rasterizer state\nError Description: " + errorDescription,
                "D3D11RenderSystem::_setHlmsMacroblock" );
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setHlmsBlendblock( ComPtr<ID3D11BlendState> blendState )
    {
        // TODO - Add this functionality to Ogre (what's the GL equivalent?)
        mDevice.GetImmediateContext()->OMSetBlendState( blendState.Get(), 0, 0xffffffff );
        if( mDevice.isError() )
        {
            String errorDescription = mDevice.getErrorDescription();
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "D3D11 device cannot set blend state\nError Description: " + errorDescription,
                         "D3D11RenderSystem::_setHlmsBlendblock" );
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setHlmsSamplerblock( uint8 texUnit, const HlmsSamplerblock *samplerblock )
    {
        assert( samplerblock->mRsData &&
                "The block must have been created via HlmsManager::getSamplerblock!" );

        ID3D11SamplerState *samplerState =
            reinterpret_cast<ID3D11SamplerState *>( samplerblock->mRsData );

        // TODO: Refactor Ogre to:
        //  a. Separate samplerblocks from textures (GL can emulate the merge).
        //  b. Set all of them at once.
        mDevice.GetImmediateContext()->VSSetSamplers( static_cast<UINT>( texUnit ),
                                                      static_cast<UINT>( 1 ), &samplerState );
        mDevice.GetImmediateContext()->PSSetSamplers( static_cast<UINT>( texUnit ),
                                                      static_cast<UINT>( 1 ), &samplerState );
        if( mDevice.isError() )
        {
            String errorDescription = mDevice.getErrorDescription();
            OGRE_EXCEPT(
                Exception::ERR_RENDERINGAPI_ERROR,
                "D3D11 device cannot set pixel shader samplers\nError Description:" + errorDescription,
                "D3D11RenderSystem::_render" );
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setPipelineStateObject( const HlmsPso *pso )
    {
        RenderSystem::_setPipelineStateObject( pso );

        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();

        // deviceContext->IASetInputLayout( 0 );
        deviceContext->VSSetShader( 0, 0, 0 );
        deviceContext->GSSetShader( 0, 0, 0 );
        deviceContext->HSSetShader( 0, 0, 0 );
        deviceContext->DSSetShader( 0, 0, 0 );
        deviceContext->PSSetShader( 0, 0, 0 );
        deviceContext->CSSetShader( 0, 0, 0 );

        mBoundComputeProgram = 0;

        if( !pso )
            return;

        D3D11HlmsPso *d3dPso = reinterpret_cast<D3D11HlmsPso *>( pso->rsData );

        _setHlmsMacroblock( pso->macroblock );
        _setHlmsBlendblock( d3dPso->blendState );

        mPso = d3dPso;

        deviceContext->OMSetDepthStencilState( d3dPso->depthStencilState.Get(), mStencilRef );
        deviceContext->IASetPrimitiveTopology( d3dPso->topology );
        deviceContext->IASetInputLayout( d3dPso->inputLayout.Get() );

        if( d3dPso->vertexShader )
        {
            deviceContext->VSSetShader( d3dPso->vertexShader->getVertexShader(), 0, 0 );
            mVertexProgramBound = true;
        }

        if( d3dPso->geometryShader )
        {
            deviceContext->GSSetShader( d3dPso->geometryShader->getGeometryShader(), 0, 0 );
            mGeometryProgramBound = true;
        }

        if( mFeatureLevel >= D3D_FEATURE_LEVEL_11_0 )
        {
            if( d3dPso->hullShader )
            {
                deviceContext->HSSetShader( d3dPso->hullShader->getHullShader(), 0, 0 );
                mTessellationHullProgramBound = true;
            }

            if( d3dPso->domainShader )
            {
                deviceContext->DSSetShader( d3dPso->domainShader->getDomainShader(), 0, 0 );
                mTessellationDomainProgramBound = true;
            }
        }

        if( d3dPso->pixelShader )
        {
            deviceContext->PSSetShader( d3dPso->pixelShader->getPixelShader(), 0, 0 );
            mFragmentProgramBound = true;
        }

        if( mDevice.isError() )
        {
            String errorDescription = mDevice.getErrorDescription();
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "D3D11 device cannot set shaders\nError Description: " + errorDescription,
                         "D3D11RenderSystem::_setPipelineStateObject" );
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setIndirectBuffer( IndirectBufferPacked *indirectBuffer )
    {
        if( mVaoManager->supportsIndirectBuffers() )
        {
            if( mBoundIndirectBuffer )
            {
                D3D11BufferInterfaceBase *bufferInterface =
                    static_cast<D3D11BufferInterfaceBase *>( indirectBuffer->getBufferInterface() );
                mBoundIndirectBuffer = bufferInterface->getVboName();
            }
            else
            {
                mBoundIndirectBuffer = 0;
            }
        }
        else
        {
            if( indirectBuffer )
                mSwIndirectBufferPtr = indirectBuffer->getSwBufferPtr();
            else
                mSwIndirectBufferPtr = 0;
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_hlmsComputePipelineStateObjectCreated( HlmsComputePso *newPso )
    {
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        debugLogPso( newPso );
#endif
        newPso->rsData = reinterpret_cast<void *>(
            static_cast<D3D11HLSLProgram *>( newPso->computeShader->_getBindingDelegate() ) );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_hlmsComputePipelineStateObjectDestroyed( HlmsComputePso *newPso )
    {
        newPso->rsData = 0;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setComputePso( const HlmsComputePso *pso )
    {
        D3D11HLSLProgram *newComputeShader = 0;

        if( pso )
        {
            newComputeShader = reinterpret_cast<D3D11HLSLProgram *>( pso->rsData );
            if( mBoundComputeProgram == newComputeShader )
                return;
        }

        RenderSystem::_setPipelineStateObject( (HlmsPso *)0 );

        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();

        // deviceContext->IASetInputLayout( 0 );
        deviceContext->VSSetShader( 0, 0, 0 );
        deviceContext->GSSetShader( 0, 0, 0 );
        deviceContext->HSSetShader( 0, 0, 0 );
        deviceContext->DSSetShader( 0, 0, 0 );
        deviceContext->PSSetShader( 0, 0, 0 );
        deviceContext->CSSetShader( 0, 0, 0 );

        mBoundComputeProgram = newComputeShader;

        if( !pso )
            return;

        deviceContext->CSSetShader( mBoundComputeProgram->getComputeShader(), 0, 0 );
        mActiveComputeGpuProgramParameters = pso->computeParams;
        mComputeProgramBound = true;

        if( mDevice.isError() )
        {
            String errorDescription = mDevice.getErrorDescription();
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "D3D11 device cannot set shaders\nError Description: " + errorDescription,
                         "D3D11RenderSystem::_setComputePso" );
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_beginFrame() {}
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_endFrame()
    {
        mBoundComputeProgram = 0;
        mActiveComputeGpuProgramParameters.reset();
        mComputeProgramBound = false;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_render( const v1::RenderOperation &op )
    {
        // Exit immediately if there is nothing to render
        if( op.vertexData == 0 || op.vertexData->vertexCount == 0 )
        {
            return;
        }

        v1::HardwareVertexBufferSharedPtr globalInstanceVertexBuffer = getGlobalInstanceVertexBuffer();
        v1::VertexDeclaration *globalVertexDeclaration =
            getGlobalInstanceVertexBufferVertexDeclaration();

        size_t numberOfInstances = op.numberOfInstances;

        if( op.useGlobalInstancingVertexBufferIsAvailable )
        {
            numberOfInstances *= getGlobalNumberOfInstances();
        }

        // Call super class
        RenderSystem::_render( op );

        ComPtr<ID3D11Buffer> pSOTarget;
        // Mustn't bind a emulated vertex, pixel shader (see below), if we are rendering to a stream out
        // buffer
        mDevice.GetImmediateContext()->SOGetTargets( 1, pSOTarget.GetAddressOf() );

        // check consistency of vertex-fragment shaders
        if( !mPso->vertexShader ||
            ( !mPso->pixelShader && op.operationType != OT_POINT_LIST && !pSOTarget ) )
        {
            OGRE_EXCEPT(
                Exception::ERR_RENDERINGAPI_ERROR,
                "Attempted to render to a D3D11 device without both vertex and fragment shaders there "
                "is no fixed pipeline in d3d11 - use the RTSS or write custom shaders.",
                "D3D11RenderSystem::_render" );
        }

        // Check consistency of tessellation shaders
        if( ( mPso->hullShader && !mPso->domainShader ) || ( !mPso->hullShader && mPso->domainShader ) )
        {
            if( mPso->hullShader && !mPso->domainShader )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "Attempted to use tessellation, but domain shader is missing",
                             "D3D11RenderSystem::_render" );
            }
            else
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "Attempted to use tessellation, but hull shader is missing",
                             "D3D11RenderSystem::_render" );
            }
        }

        if( mDevice.isError() )
        {
            // this will never happen but we want to be consistent with the error checks...
            String errorDescription = mDevice.getErrorDescription();
            OGRE_EXCEPT(
                Exception::ERR_RENDERINGAPI_ERROR,
                "D3D11 device cannot set geometry shader to null\nError Description:" + errorDescription,
                "D3D11RenderSystem::_render" );
        }

        // Determine rendering operation
        D3D11_PRIMITIVE_TOPOLOGY primType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        DWORD primCount = 0;

        // Handle computing
        if( mPso->hullShader && mPso->domainShader )
        {
            // useful primitives for tessellation
            switch( op.operationType )
            {
            case OT_LINE_LIST:
                primType = D3D11_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST;
                primCount =
                    (DWORD)( op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount ) / 2;
                break;

            case OT_LINE_STRIP:
                primType = D3D11_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST;
                primCount =
                    (DWORD)( op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount ) - 1;
                break;

            case OT_TRIANGLE_LIST:
                primType = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
                primCount =
                    (DWORD)( op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount ) / 3;
                break;

            case OT_TRIANGLE_STRIP:
                primType = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
                primCount =
                    (DWORD)( op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount ) - 2;
                break;
            }
        }
        else
        {
            // rendering without tessellation.
            int operationType = op.operationType;
            if( mGeometryProgramBound && mPso->geometryShader &&
                mPso->geometryShader->isAdjacencyInfoRequired() )
            {
                operationType |= OT_DETAIL_ADJACENCY_BIT;
            }

            switch( operationType )
            {
            case OT_POINT_LIST:
                primType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
                primCount =
                    (DWORD)( op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount );
                break;

            case OT_LINE_LIST:
                primType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
                primCount =
                    (DWORD)( op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount ) / 2;
                break;

            case OT_LINE_LIST_ADJ:
                primType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
                primCount =
                    (DWORD)( op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount ) / 4;
                break;

            case OT_LINE_STRIP:
                primType = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
                primCount =
                    (DWORD)( op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount ) - 1;
                break;

            case OT_LINE_STRIP_ADJ:
                primType = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
                primCount =
                    (DWORD)( op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount ) - 2;
                break;

            case OT_TRIANGLE_LIST:
                primType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
                primCount =
                    (DWORD)( op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount ) / 3;
                break;

            case OT_TRIANGLE_LIST_ADJ:
                primType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
                primCount =
                    (DWORD)( op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount ) / 6;
                break;

            case OT_TRIANGLE_STRIP:
                primType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
                primCount =
                    (DWORD)( op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount ) - 2;
                break;

            case OT_TRIANGLE_STRIP_ADJ:
                primType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
                primCount =
                    (DWORD)( op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount ) /
                        2 -
                    2;
                break;

            case OT_TRIANGLE_FAN:
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "Error - DX11 render - no support for triangle fan (OT_TRIANGLE_FAN)",
                             "D3D11RenderSystem::_render" );
                primType = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;  // todo - no TRIANGLE_FAN in DX 11
                primCount =
                    (DWORD)( op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount ) - 2;
                break;
            }
        }

        if( primCount )
        {
            // Issue the op
            // HRESULT hr;
            if( op.useIndexes )
            {
                v1::D3D11HardwareIndexBuffer *d3dIdxBuf =
                    static_cast<v1::D3D11HardwareIndexBuffer *>( op.indexData->indexBuffer.get() );
                mDevice.GetImmediateContext()->IASetIndexBuffer(
                    d3dIdxBuf->getD3DIndexBuffer(), D3D11Mappings::getFormat( d3dIdxBuf->getType() ),
                    0 );
                if( mDevice.isError() )
                {
                    String errorDescription = mDevice.getErrorDescription();
                    OGRE_EXCEPT(
                        Exception::ERR_RENDERINGAPI_ERROR,
                        "D3D11 device cannot set index buffer\nError Description:" + errorDescription,
                        "D3D11RenderSystem::_render" );
                }
            }

            mDevice.GetImmediateContext()->IASetPrimitiveTopology( primType );
            if( mDevice.isError() )
            {
                String errorDescription = mDevice.getErrorDescription();
                OGRE_EXCEPT(
                    Exception::ERR_RENDERINGAPI_ERROR,
                    "D3D11 device cannot set primitive topology\nError Description:" + errorDescription,
                    "D3D11RenderSystem::_render" );
            }

            do
            {
                if( op.useIndexes )
                {
                    if( numberOfInstances > 1 )
                    {
                        mDevice.GetImmediateContext()->DrawIndexedInstanced(
                            static_cast<UINT>( op.indexData->indexCount ),
                            static_cast<UINT>( numberOfInstances ),
                            static_cast<UINT>( op.indexData->indexStart ),
                            static_cast<INT>( op.vertexData->vertexStart ), 0 );
                    }
                    else
                    {
                        mDevice.GetImmediateContext()->DrawIndexed(
                            static_cast<UINT>( op.indexData->indexCount ),
                            static_cast<UINT>( op.indexData->indexStart ),
                            static_cast<INT>( op.vertexData->vertexStart ) );
                    }
                }
                else  // non indexed
                {
                    if( op.vertexData->vertexCount == -1 )  // -1 is a sign to use DrawAuto
                    {
                        mDevice.GetImmediateContext()->DrawAuto();
                    }
                    else if( numberOfInstances > 1 )
                    {
                        mDevice.GetImmediateContext()->DrawInstanced(
                            static_cast<UINT>( op.vertexData->vertexCount ),
                            static_cast<UINT>( numberOfInstances ),
                            static_cast<UINT>( op.vertexData->vertexStart ), 0 );
                    }
                    else
                    {
                        mDevice.GetImmediateContext()->Draw(
                            static_cast<UINT>( op.vertexData->vertexCount ),
                            static_cast<UINT>( op.vertexData->vertexStart ) );
                    }
                }

                if( mDevice.isError() )
                {
                    String errorDescription = "D3D11 device cannot draw";
                    if( !op.useIndexes &&
                        op.vertexData->vertexCount == -1 )  // -1 is a sign to use DrawAuto
                        errorDescription.append( " auto" );
                    else
                        errorDescription.append( op.useIndexes ? " indexed" : "" )
                            .append( numberOfInstances > 1 ? " instanced" : "" );
                    errorDescription.append( "\nError Description:" )
                        .append( mDevice.getErrorDescription() );
                    errorDescription.append( "\nActive OGRE shaders:" )
                        .append( mPso->vertexShader
                                     ? ( "\nVS = " + mPso->vertexShader->getName() ).c_str()
                                     : "" )
                        .append( mPso->hullShader ? ( "\nHS = " + mPso->hullShader->getName() ).c_str()
                                                  : "" )
                        .append( mPso->domainShader
                                     ? ( "\nDS = " + mPso->domainShader->getName() ).c_str()
                                     : "" )
                        .append( mPso->geometryShader
                                     ? ( "\nGS = " + mPso->geometryShader->getName() ).c_str()
                                     : "" )
                        .append( mPso->pixelShader ? ( "\nFS = " + mPso->pixelShader->getName() ).c_str()
                                                   : "" )
                        .append( mBoundComputeProgram
                                     ? ( "\nCS = " + mBoundComputeProgram->getName() ).c_str()
                                     : "" );

                    OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, errorDescription,
                                 "D3D11RenderSystem::_render" );
                }

            } while( updatePassIterationRenderState() );
        }

        // Crashy : commented this, 99% sure it's useless but really time consuming
        /*if (true) // for now - clear the render state
        {
            mDevice.GetImmediateContext()->OMSetBlendState(0, 0, 0xffffffff);
            mDevice.GetImmediateContext()->RSSetState(0);
            mDevice.GetImmediateContext()->OMSetDepthStencilState(0, 0);
//          mDevice->PSSetSamplers(static_cast<UINT>(0), static_cast<UINT>(0), 0);

            // Clear class instance storage
            memset(mClassInstances, 0, sizeof(mClassInstances));
            memset(mNumClassInstances, 0, sizeof(mNumClassInstances));
        }*/
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_dispatch( const HlmsComputePso &pso )
    {
        mDevice.GetImmediateContext()->Dispatch( pso.mNumThreadGroups[0], pso.mNumThreadGroups[1],
                                                 pso.mNumThreadGroups[2] );

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        if( mDevice.isError() )
        {
            String msg = mDevice.getErrorDescription();
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Error after _dispatch: CS '" + pso.computeShader->getName() + "' " + msg,
                         __FUNCTION__ );
        }
#endif

        assert( mMaxBoundUavCS < 8u );
        ID3D11UnorderedAccessView *nullUavViews[8];
        memset( nullUavViews, 0, sizeof( nullUavViews ) );
        mDevice.GetImmediateContext()->CSSetUnorderedAccessViews( 0, mMaxBoundUavCS + 1u, nullUavViews,
                                                                  NULL );
        mMaxBoundUavCS = 0u;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setVertexArrayObject( const VertexArrayObject *_vao )
    {
        const D3D11VertexArrayObject *vao = static_cast<const D3D11VertexArrayObject *>( _vao );
        D3D11VertexArrayObjectShared *sharedData = vao->mSharedData;

        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();

        deviceContext->IASetVertexBuffers( 0,
                                           UINT( vao->mVertexBuffers.size() + 1u ),  //+1 due to DrawId
                                           sharedData->mVertexBuffers[0].GetAddressOf(),  //
                                           sharedData->mStrides,                          //
                                           sharedData->mOffsets );
        deviceContext->IASetIndexBuffer( sharedData->mIndexBuffer.Get(), sharedData->mIndexFormat, 0 );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_render( const CbDrawCallIndexed *cmd )
    {
        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();

        UINT indirectBufferOffset =
            static_cast<UINT>( reinterpret_cast<uintptr_t>( cmd->indirectBufferOffset ) );
        for( uint32 i = cmd->numDraws; i--; )
        {
            deviceContext->DrawIndexedInstancedIndirect( mBoundIndirectBuffer, indirectBufferOffset );

            indirectBufferOffset += sizeof( CbDrawIndexed );
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_render( const CbDrawCallStrip *cmd )
    {
        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();

        UINT indirectBufferOffset =
            static_cast<UINT>( reinterpret_cast<uintptr_t>( cmd->indirectBufferOffset ) );
        for( uint32 i = cmd->numDraws; i--; )
        {
            deviceContext->DrawInstancedIndirect( mBoundIndirectBuffer, indirectBufferOffset );

            indirectBufferOffset += sizeof( CbDrawStrip );
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_renderEmulated( const CbDrawCallIndexed *cmd )
    {
        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();

        CbDrawIndexed *drawCmd = reinterpret_cast<CbDrawIndexed *>( mSwIndirectBufferPtr +
                                                                    (size_t)cmd->indirectBufferOffset );

        for( uint32 i = cmd->numDraws; i--; )
        {
            deviceContext->DrawIndexedInstanced( drawCmd->primCount, drawCmd->instanceCount,
                                                 drawCmd->firstVertexIndex, drawCmd->baseVertex,
                                                 drawCmd->baseInstance );
            ++drawCmd;
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_renderEmulated( const CbDrawCallStrip *cmd )
    {
        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();

        CbDrawStrip *drawCmd =
            reinterpret_cast<CbDrawStrip *>( mSwIndirectBufferPtr + (size_t)cmd->indirectBufferOffset );

        for( uint32 i = cmd->numDraws; i--; )
        {
            deviceContext->DrawInstanced( drawCmd->primCount, drawCmd->instanceCount,
                                          drawCmd->firstVertexIndex, drawCmd->baseInstance );
            ++drawCmd;
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_setRenderOperation( const v1::CbRenderOp *cmd )
    {
        mCurrentVertexBuffer = cmd->vertexData;
        mCurrentIndexBuffer = cmd->indexData;

        ID3D11DeviceContextN *deviceContext = mDevice.GetImmediateContext();

        if( cmd->indexData )
        {
            v1::D3D11HardwareIndexBuffer *indexBuffer =
                static_cast<v1::D3D11HardwareIndexBuffer *>( cmd->indexData->indexBuffer.get() );
            deviceContext->IASetIndexBuffer( indexBuffer->getD3DIndexBuffer(),
                                             D3D11Mappings::getFormat( indexBuffer->getType() ), 0 );
        }
        else
        {
            deviceContext->IASetIndexBuffer( 0, DXGI_FORMAT_UNKNOWN, 0 );
        }

        uint32 usedSlots = 0;
        const v1::VertexBufferBinding::VertexBufferBindingMap &binds =
            cmd->vertexData->vertexBufferBinding->getBindings();
        v1::VertexBufferBinding::VertexBufferBindingMap::const_iterator i, iend;
        iend = binds.end();
        for( i = binds.begin(); i != iend; ++i )
        {
            const v1::D3D11HardwareVertexBuffer *d3d11buf =
                static_cast<const v1::D3D11HardwareVertexBuffer *>( i->second.get() );

            UINT stride = static_cast<UINT>( d3d11buf->getVertexSize() );
            UINT offset = 0;  // no stream offset, this is handled in _render instead
            UINT slot = static_cast<UINT>( i->first );
            ID3D11Buffer *pVertexBuffers = d3d11buf->getD3DVertexBuffer();
            mDevice.GetImmediateContext()->IASetVertexBuffers(
                slot,  // The first input slot for binding.
                1,     // The number of vertex buffers in the array.
                &pVertexBuffers, &stride, &offset );

            if( mDevice.isError() )
            {
                String errorDescription = mDevice.getErrorDescription();
                OGRE_EXCEPT(
                    Exception::ERR_RENDERINGAPI_ERROR,
                    "D3D11 device cannot set vertex buffers\nError Description:" + errorDescription,
                    "D3D11RenderSystem::setVertexBufferBinding" );
            }

            ++usedSlots;
        }

        static_cast<D3D11VaoManager *>( mVaoManager )->bindDrawId( usedSlots );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_render( const v1::CbDrawCallIndexed *cmd )
    {
        mDevice.GetImmediateContext()->DrawIndexedInstanced(
            cmd->primCount, cmd->instanceCount, cmd->firstVertexIndex,
            static_cast<INT>( mCurrentVertexBuffer->vertexStart ), cmd->baseInstance );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_render( const v1::CbDrawCallStrip *cmd )
    {
        mDevice.GetImmediateContext()->DrawInstanced( cmd->primCount, cmd->instanceCount,
                                                      cmd->firstVertexIndex, cmd->baseInstance );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::setNormaliseNormals( bool normalise ) {}
    //---------------------------------------------------------------------
    void D3D11RenderSystem::bindGpuProgramParameters( GpuProgramType gptype,
                                                      GpuProgramParametersSharedPtr params, uint16 mask )
    {
        if( mask & (uint16)GPV_GLOBAL )
        {
            // TODO: Dx11 supports shared constant buffers, so use them
            // check the match to constant buffers & use rendersystem data hooks to store
            // for now, just copy
            params->_copySharedParams();
        }

        // Do everything here in Dx11, since deal with via buffers anyway so number of calls
        // is actually the same whether we categorise the updates or not
        ID3D11Buffer *pBuffers[2];
        UINT slotStart, numBuffers;
        switch( gptype )
        {
        case GPT_VERTEX_PROGRAM:
            if( mPso->vertexShader )
            {
                mPso->vertexShader->getConstantBuffers( pBuffers, slotStart, numBuffers, params, mask );
                if( numBuffers > 0 )
                {
                    mDevice.GetImmediateContext()->VSSetConstantBuffers( slotStart, numBuffers,
                                                                         pBuffers );
                    if( mDevice.isError() )
                    {
                        String errorDescription = mDevice.getErrorDescription();
                        OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                                     "D3D11 device cannot set vertex shader constant buffers\n"
                                     "Error Description:" +
                                         errorDescription,
                                     "D3D11RenderSystem::bindGpuProgramParameters" );
                    }
                }
            }
            break;
        case GPT_FRAGMENT_PROGRAM:
            if( mPso->pixelShader )
            {
                mPso->pixelShader->getConstantBuffers( pBuffers, slotStart, numBuffers, params, mask );
                if( numBuffers > 0 )
                {
                    mDevice.GetImmediateContext()->PSSetConstantBuffers( slotStart, numBuffers,
                                                                         pBuffers );
                    if( mDevice.isError() )
                    {
                        String errorDescription = mDevice.getErrorDescription();
                        OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                                     "D3D11 device cannot set pixel shader constant buffers\n"
                                     "Error Description:" +
                                         errorDescription,
                                     "D3D11RenderSystem::bindGpuProgramParameters" );
                    }
                }
            }
            break;
        case GPT_GEOMETRY_PROGRAM:
            if( mPso->geometryShader )
            {
                mPso->geometryShader->getConstantBuffers( pBuffers, slotStart, numBuffers, params,
                                                          mask );
                if( numBuffers > 0 )
                {
                    mDevice.GetImmediateContext()->GSSetConstantBuffers( slotStart, numBuffers,
                                                                         pBuffers );
                    if( mDevice.isError() )
                    {
                        String errorDescription = mDevice.getErrorDescription();
                        OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                                     "D3D11 device cannot set geometry shader constant buffers\n"
                                     "Error Description:" +
                                         errorDescription,
                                     "D3D11RenderSystem::bindGpuProgramParameters" );
                    }
                }
            }
            break;
        case GPT_HULL_PROGRAM:
            if( mPso->hullShader )
            {
                mPso->hullShader->getConstantBuffers( pBuffers, slotStart, numBuffers, params, mask );
                if( numBuffers > 0 )
                {
                    mDevice.GetImmediateContext()->HSSetConstantBuffers( slotStart, numBuffers,
                                                                         pBuffers );
                    if( mDevice.isError() )
                    {
                        String errorDescription = mDevice.getErrorDescription();
                        OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                                     "D3D11 device cannot set hull shader constant buffers\n"
                                     "Error Description:" +
                                         errorDescription,
                                     "D3D11RenderSystem::bindGpuProgramParameters" );
                    }
                }
            }
            break;
        case GPT_DOMAIN_PROGRAM:
            if( mPso->domainShader )
            {
                mPso->domainShader->getConstantBuffers( pBuffers, slotStart, numBuffers, params, mask );
                if( numBuffers > 0 )
                {
                    mDevice.GetImmediateContext()->DSSetConstantBuffers( slotStart, numBuffers,
                                                                         pBuffers );
                    if( mDevice.isError() )
                    {
                        String errorDescription = mDevice.getErrorDescription();
                        OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                                     "D3D11 device cannot set domain shader constant buffers\n"
                                     "Error Description:" +
                                         errorDescription,
                                     "D3D11RenderSystem::bindGpuProgramParameters" );
                    }
                }
            }
            break;
        case GPT_COMPUTE_PROGRAM:
            if( mBoundComputeProgram )
            {
                mBoundComputeProgram->getConstantBuffers( pBuffers, slotStart, numBuffers, params,
                                                          mask );
                if( numBuffers > 0 )
                {
                    mDevice.GetImmediateContext()->CSSetConstantBuffers( slotStart, numBuffers,
                                                                         pBuffers );
                    if( mDevice.isError() )
                    {
                        String errorDescription = mDevice.getErrorDescription();
                        OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                                     "D3D11 device cannot set compute shader constant buffers\n"
                                     "Error Description:" +
                                         errorDescription,
                                     "D3D11RenderSystem::bindGpuProgramParameters" );
                    }
                }
            }
            break;
        };

        // Now, set class instances
        const GpuProgramParameters::SubroutineMap &subroutineMap = params->getSubroutineMap();
        if( subroutineMap.empty() )
            return;

        GpuProgramParameters::SubroutineIterator it;
        GpuProgramParameters::SubroutineIterator end = subroutineMap.end();
        for( it = subroutineMap.begin(); it != end; ++it )
        {
            setSubroutine( gptype, it->first, it->second );
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::bindGpuProgramPassIterationParameters( GpuProgramType gptype )
    {
        switch( gptype )
        {
        case GPT_VERTEX_PROGRAM:
            bindGpuProgramParameters( gptype, mActiveVertexGpuProgramParameters,
                                      (uint16)GPV_PASS_ITERATION_NUMBER );
            break;

        case GPT_FRAGMENT_PROGRAM:
            bindGpuProgramParameters( gptype, mActiveFragmentGpuProgramParameters,
                                      (uint16)GPV_PASS_ITERATION_NUMBER );
            break;
        case GPT_GEOMETRY_PROGRAM:
            bindGpuProgramParameters( gptype, mActiveGeometryGpuProgramParameters,
                                      (uint16)GPV_PASS_ITERATION_NUMBER );
            break;
        case GPT_HULL_PROGRAM:
            bindGpuProgramParameters( gptype, mActiveTessellationHullGpuProgramParameters,
                                      (uint16)GPV_PASS_ITERATION_NUMBER );
            break;
        case GPT_DOMAIN_PROGRAM:
            bindGpuProgramParameters( gptype, mActiveTessellationDomainGpuProgramParameters,
                                      (uint16)GPV_PASS_ITERATION_NUMBER );
            break;
        case GPT_COMPUTE_PROGRAM:
            bindGpuProgramParameters( gptype, mActiveComputeGpuProgramParameters,
                                      (uint16)GPV_PASS_ITERATION_NUMBER );
            break;
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::setSubroutine( GpuProgramType gptype, size_t slotIndex,
                                           const String &subroutineName )
    {
        ID3D11ClassInstance *instance = 0;

        ClassInstanceIterator it = mInstanceMap.find( subroutineName );
        if( it == mInstanceMap.end() )
        {
            // try to get instance already created (must have at least one field)
            HRESULT hr =
                mDevice.GetClassLinkage()->GetClassInstance( subroutineName.c_str(), 0, &instance );
            if( FAILED( hr ) || instance == 0 )
            {
                // probably class don't have a field, try create a new
                hr = mDevice.GetClassLinkage()->CreateClassInstance( subroutineName.c_str(), 0, 0, 0, 0,
                                                                     &instance );
                if( FAILED( hr ) || instance == 0 )
                {
                    OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                                    "Shader subroutine with name " + subroutineName + " doesn't exist.",
                                    "D3D11RenderSystem::setSubroutineName" );
                }
            }

            // Store class instance
            mInstanceMap.emplace( subroutineName, instance );
        }
        else
        {
            instance = it->second;
        }

        // If already created, store class instance
        mClassInstances[gptype][slotIndex] = instance;
        mNumClassInstances[gptype] = mNumClassInstances[gptype] + 1;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::setSubroutine( GpuProgramType gptype, const String &slotName,
                                           const String &subroutineName )
    {
        unsigned int slotIdx = 0;
        switch( gptype )
        {
        case GPT_VERTEX_PROGRAM:
        {
            if( mPso->vertexShader )
            {
                slotIdx = mPso->vertexShader->getSubroutineSlot( slotName );
            }
        }
        break;
        case GPT_FRAGMENT_PROGRAM:
        {
            if( mPso->pixelShader )
            {
                slotIdx = mPso->pixelShader->getSubroutineSlot( slotName );
            }
        }
        break;
        case GPT_GEOMETRY_PROGRAM:
        {
            if( mPso->geometryShader )
            {
                slotIdx = mPso->geometryShader->getSubroutineSlot( slotName );
            }
        }
        break;
        case GPT_HULL_PROGRAM:
        {
            if( mPso->hullShader )
            {
                slotIdx = mPso->hullShader->getSubroutineSlot( slotName );
            }
        }
        break;
        case GPT_DOMAIN_PROGRAM:
        {
            if( mPso->domainShader )
            {
                slotIdx = mPso->domainShader->getSubroutineSlot( slotName );
            }
        }
        break;
        case GPT_COMPUTE_PROGRAM:
        {
            if( mBoundComputeProgram )
            {
                slotIdx = mBoundComputeProgram->getSubroutineSlot( slotName );
            }
        }
        break;
        };

        // Set subroutine for slot
        setSubroutine( gptype, slotIdx, subroutineName );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::setClipPlanesImpl( const PlaneList &clipPlanes ) {}
    //---------------------------------------------------------------------
    void D3D11RenderSystem::clearFrameBuffer( RenderPassDescriptor *renderPassDesc,
                                              TextureGpu *anyTarget, uint8 mipLevel )
    {
        D3D11RenderPassDescriptor *renderPassDescD3d =
            static_cast<D3D11RenderPassDescriptor *>( renderPassDesc );
        renderPassDescD3d->clearFrameBuffer();
    }
    // ------------------------------------------------------------------
    void D3D11RenderSystem::setClipPlane( ushort index, Real A, Real B, Real C, Real D ) {}

    // ------------------------------------------------------------------
    void D3D11RenderSystem::enableClipPlane( ushort index, bool enable ) {}
    //---------------------------------------------------------------------
    HardwareOcclusionQuery *D3D11RenderSystem::createHardwareOcclusionQuery()
    {
        D3D11HardwareOcclusionQuery *ret = new D3D11HardwareOcclusionQuery( mDevice );
        mHwOcclusionQueries.push_back( ret );
        return ret;
    }
    //---------------------------------------------------------------------
    Real D3D11RenderSystem::getHorizontalTexelOffset()
    {
        // D3D11 is now like GL
        return 0.0f;
    }
    //---------------------------------------------------------------------
    Real D3D11RenderSystem::getVerticalTexelOffset()
    {
        // D3D11 is now like GL
        return 0.0f;
    }
    //---------------------------------------------------------------------
    Real D3D11RenderSystem::getMinimumDepthInputValue()
    {
        // Range [0.0f, 1.0f]
        return 0.0f;
    }
    //---------------------------------------------------------------------
    Real D3D11RenderSystem::getMaximumDepthInputValue()
    {
        // Range [0.0f, 1.0f]
        // D3D inverts even identity view matrices, so maximum INPUT is -1.0
        return -1.0f;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::registerThread()
    {
        // nothing to do - D3D11 shares rendering context already
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::unregisterThread()
    {
        // nothing to do - D3D11 shares rendering context already
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::preExtraThreadsStarted()
    {
        // nothing to do - D3D11 shares rendering context already
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::postExtraThreadsStarted()
    {
        // nothing to do - D3D11 shares rendering context already
    }
    //---------------------------------------------------------------------
    String D3D11RenderSystem::getErrorDescription( long errorNumber ) const
    {
        return mDevice.getErrorDescription( errorNumber );
    }
    //---------------------------------------------------------------------
    SampleDescription D3D11RenderSystem::validateSampleDescription( const SampleDescription &sampleDesc,
                                                                    PixelFormatGpu format,
                                                                    uint32 textureFlags )
    {
        OGRE_UNUSED_VAR( textureFlags );

        SampleDescription res;
        DXGI_FORMAT dxgiFormat = D3D11Mappings::get( format );
        const uint8 samples = sampleDesc.getMaxSamples();
        const bool msaaOnly = sampleDesc.isMsaa();

        // NVIDIA, AMD - prefer CSAA aka EQAA if available. See
        // http://developer.download.nvidia.com/assets/gamedev/docs/CSAA_Tutorial.pdf
        // http://developer.amd.com/wordpress/media/2012/10/EQAA%20Modes%20for%20AMD%20HD%206900%20Series%20Cards.pdf
        // https://www.khronos.org/registry/OpenGL/extensions/NV/NV_framebuffer_multisample_coverage.txt

        // Modes are sorted from high quality to low quality, CSAA aka EQAA are listed first
        // Note, that max(Count, Quality) == MSAA level and (Count >= 8 && Quality != 0) == quality hint
        DXGI_SAMPLE_DESC presets[] = {
            { sampleDesc.getColourSamples(), sampleDesc.getCoverageSamples() },  // exact match

            { 16, 0 },  // MSAA 16x
            { 8, 16 },  // CSAA 16xQ, EQAA 8f16x
            { 4, 16 },  // CSAA 16x,  EQAA 4f16x

            { 12, 0 },  // MSAA 12x

            { 8, 0 },  // MSAA 8x
            { 4, 8 },  // CSAA 8x,  EQAA 4f8x

            { 6, 0 },  // MSAA 6x
            { 4, 0 },  // MSAA 4x
            { 2, 4 },  // EQAA 2f4x
            { 2, 0 },  // MSAA 2x
            { 1, 0 },  // MSAA 1x
            { NULL, NULL },
        };

        // Use first supported mode
        for( DXGI_SAMPLE_DESC *mode = presets; mode->Count != 0; ++mode )
        {
            // Skip too HQ modes
            unsigned modeSamples = std::max( mode->Count, mode->Quality );
            if( modeSamples > samples )
                continue;

            // Skip CSAA modes if specifically MSAA were requested, but not vice versa
            if( msaaOnly && mode->Quality > 0 )
                continue;

            // Skip unsupported modes
            UINT numQualityLevels;
            HRESULT hr =
                mDevice->CheckMultisampleQualityLevels( dxgiFormat, mode->Count, &numQualityLevels );
            if( FAILED( hr ) || mode->Quality >= numQualityLevels )
                continue;

            // All checks passed
            MsaaPatterns::MsaaPatterns pattern;
            if( mode->Quality != 0 )
                pattern = MsaaPatterns::Undefined;  // CSAA / EQAA
            else
                pattern = sampleDesc.getMsaaPattern();
            res._set( static_cast<uint8>( mode->Count ), static_cast<uint8>( mode->Quality ), pattern );
            break;
        }

        return res;
    }
    //---------------------------------------------------------------------
    unsigned int D3D11RenderSystem::getDisplayMonitorCount() const
    {
        unsigned int monitorCount = 0;
        HRESULT hr;
        ComPtr<IDXGIOutput> pOutput;

        if( !mDriverList )
        {
            return 0;
        }

        for( size_t i = 0; i < mDriverList->count(); ++i )
        {
            for( UINT m = 0u;; ++m )
            {
                hr = mDriverList->item( i )->getDeviceAdapter()->EnumOutputs(
                    m, pOutput.ReleaseAndGetAddressOf() );
                if( DXGI_ERROR_NOT_FOUND == hr )
                {
                    break;
                }
                else if( FAILED( hr ) )
                {
                    break;  // Something bad happened.
                }
                else
                {
                    ++monitorCount;
                }
            }
        }
        return monitorCount;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::initRenderSystem()
    {
        if( mRenderSystemWasInited )
        {
            return;
        }

        mRenderSystemWasInited = true;
        // set pointers to NULL
        mDriverList = NULL;
        mHardwareBufferManager = NULL;
        mGpuProgramManager = NULL;
        mPrimaryWindow = NULL;
        mMinRequestedFeatureLevel = D3D_FEATURE_LEVEL_9_1;
#if __OGRE_WINRT_PHONE  // Windows Phone support only FL 9.3, but simulator can create much more capable
                        // device, so restrict it artificially here
        mMaxRequestedFeatureLevel = D3D_FEATURE_LEVEL_9_3;
#elif defined( _WIN32_WINNT_WIN8 )
        if( IsWindows8OrGreater() )
            mMaxRequestedFeatureLevel = D3D_FEATURE_LEVEL_11_1;
        else
            mMaxRequestedFeatureLevel = D3D_FEATURE_LEVEL_11_0;
#else
        mMaxRequestedFeatureLevel = D3D_FEATURE_LEVEL_11_0;
#endif
        mUseNVPerfHUD = false;
        mHLSLProgramFactory = NULL;

#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        OGRE_DELETE mStereoDriver;
        mStereoDriver = NULL;
#endif

        mPso = NULL;
        mBoundComputeProgram = NULL;

        mBindingType = TextureUnitState::BT_FRAGMENT;

        mVendorExtension = D3D11VendorExtension::initializeExtension( GPU_VENDOR_COUNT, 0 );

        ComPtr<ID3D11Device> device;
        createD3D11Device( mVendorExtension, Root::getSingleton().getAppName(), NULL,
                           D3D_DRIVER_TYPE_HARDWARE, mMinRequestedFeatureLevel,
                           mMaxRequestedFeatureLevel, 0, device.GetAddressOf() );
        mDevice.TransferOwnership( device );
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::getCustomAttribute( const String &name, void *pData )
    {
        if( name == "D3DDEVICE" )
        {
            ID3D11DeviceN **device = (ID3D11DeviceN **)pData;
            *device = mDevice.get();
            return;
        }
        else if( name == "MapNoOverwriteOnDynamicConstantBuffer" )
        {
            *reinterpret_cast<bool *>( pData ) = false;  // TODO
            return;
        }
        else if( name == "MapNoOverwriteOnDynamicBufferSRV" )
        {
            *reinterpret_cast<bool *>( pData ) = false;  // TODO
            return;
        }

        OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Attribute not found: " + name,
                     "RenderSystem::getCustomAttribute" );
    }
    //---------------------------------------------------------------------
    D3D11HLSLProgram *D3D11RenderSystem::_getBoundComputeProgram() const { return mBoundComputeProgram; }
    //---------------------------------------------------------------------
    bool D3D11RenderSystem::setDrawBuffer( ColourBufferType colourBuffer )
    {
#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        return D3D11StereoDriverBridge::getSingleton().setDrawBuffer( colourBuffer );
#else
        return false;
#endif
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::beginProfileEvent( const String &eventName )
    {
#if OGRE_D3D11_PROFILING
        if( mDevice.GetProfiler() )
        {
            wchar_t wideName[256];  // Let avoid heap memory allocation if we are in profiling code.
            bool wideNameOk =
                !eventName.empty() &&
                0 != MultiByteToWideChar( CP_ACP, 0, eventName.data(), eventName.length() + 1, wideName,
                                          ARRAYSIZE( wideName ) );
            mDevice.GetProfiler()->BeginEvent( wideNameOk ? wideName
                                                          : L"<too long or empty event name>" );
        }
#endif
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::endProfileEvent()
    {
#if OGRE_D3D11_PROFILING
        if( mDevice.GetProfiler() )
            mDevice.GetProfiler()->EndEvent();
#endif
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::markProfileEvent( const String &eventName )
    {
#if OGRE_D3D11_PROFILING
        if( mDevice.GetProfiler() )
        {
            wchar_t wideName[256];  // Let avoid heap memory allocation if we are in profiling code.
            bool wideNameOk =
                !eventName.empty() &&
                0 != MultiByteToWideChar( CP_ACP, 0, eventName.data(), eventName.length() + 1, wideName,
                                          ARRAYSIZE( wideName ) );
            mDevice.GetProfiler()->SetMarker( wideNameOk ? wideName
                                                         : L"<too long or empty event name>" );
        }
#endif
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::debugAnnotationPush( const String &eventName )
    {
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        if( mDevice.GetProfiler() )
        {
            wchar_t wideName[256];  // Let avoid heap memory allocation if we are in profiling code.
            bool wideNameOk =
                !eventName.empty() &&
                0 != MultiByteToWideChar( CP_ACP, 0, eventName.data(), int( eventName.length() + 1u ),
                                          wideName, ARRAYSIZE( wideName ) );
            mDevice.GetProfiler()->BeginEvent( wideNameOk ? wideName
                                                          : L"<too long or empty event name>" );
        }
#endif
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::debugAnnotationPop()
    {
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        if( mDevice.GetProfiler() )
        {
            mDevice.GetProfiler()->EndEvent();
        }
#endif
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::initGPUProfiling()
    {
#if OGRE_PROFILING == OGRE_PROFILING_REMOTERY
        _rmt_BindD3D11( (void *)mDevice.get(), (void *)mDevice.GetImmediateContext() );
#endif
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::deinitGPUProfiling()
    {
#if OGRE_PROFILING == OGRE_PROFILING_REMOTERY
        _rmt_UnbindD3D11();
#endif
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::beginGPUSampleProfile( const String &name, uint32 *hashCache )
    {
#if OGRE_PROFILING == OGRE_PROFILING_REMOTERY
        _rmt_BeginD3D11Sample( name.c_str(), hashCache );
#endif
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::endGPUSampleProfile( const String &name )
    {
#if OGRE_PROFILING == OGRE_PROFILING_REMOTERY
        _rmt_EndD3D11Sample();
#endif
    }
    //---------------------------------------------------------------------
    const PixelFormatToShaderType *D3D11RenderSystem::getPixelFormatToShaderType() const
    {
        return &mD3D11PixelFormatToShaderType;
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::_clearStateAndFlushCommandBuffer()
    {
        OgreProfileExhaustive( "D3D11RenderSystem::_clearStateAndFlushCommandBuffer" );

        mDevice.GetImmediateContext()->ClearState();
        mDevice.GetImmediateContext()->Flush();

        endRenderPassDescriptor();
    }
    //---------------------------------------------------------------------
    void D3D11RenderSystem::flushCommands()
    {
        OgreProfileExhaustive( "D3D11RenderSystem::flushCommands" );
        mDevice.GetImmediateContext()->Flush();
    }
}  // namespace Ogre
