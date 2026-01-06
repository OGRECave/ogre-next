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

#include "PBuffer/OgreEglPBufferSupport.h"

#include "OgreException.h"
#include "OgreGL3PlusRenderSystem.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"

#include "PBuffer/OgreEglPBufferWindow.h"

typedef EGLBoolean ( *EGLQueryDevicesType )( EGLint, EGLDeviceEXT *, EGLint * );

static EGLQueryDevicesType eglQueryDevices;
static PFNEGLQUERYDEVICESTRINGEXTPROC eglQueryDeviceStringEXT;

namespace Ogre
{
    //-------------------------------------------------------------------------
    EglPBufferSupport::EglPBufferSupport()
    {
        eglQueryDevices = (EGLQueryDevicesType)eglGetProcAddress( "eglQueryDevicesEXT" );
        eglQueryDeviceStringEXT =
            (PFNEGLQUERYDEVICESTRINGEXTPROC)eglGetProcAddress( "eglQueryDeviceStringEXT" );

        if( eglQueryDevices == 0 )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "eglQueryDevicesEXT not found. EGL driver too old. "
                         "Update your GPU drivers and try again",
                         "EglPBufferSupport::EglPBufferSupport" );
        }

        if( eglQueryDeviceStringEXT == 0 )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "eglQueryDeviceStringEXT not found. EGL driver too old. "
                         "Update your GPU drivers and try again",
                         "EglPBufferSupport::EglPBufferSupport" );
        }

        EGLint numDevices = 0;
        eglQueryDevices( 0, 0, &numDevices );

        if( numDevices > 0 )
        {
            mDevices.resize( static_cast<size_t>( numDevices ) );
            mDeviceData.reserve( static_cast<size_t>( numDevices ) );

            eglQueryDevices( numDevices, mDevices.begin(), &numDevices );
        }

        LogManager::getSingleton().logMessage( "Found Num EGL Devices: " +
                                               StringConverter::toString( numDevices ) );

        if( numDevices == 0 )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "No EGL devices found! Update your GPU drivers and try again",
                         "EglPBufferSupport::EglPBufferSupport" );
        }

        for( int i = 0u; i < numDevices; ++i )
        {
            EGLDeviceEXT device = mDevices[size_t( i )];
            const char *name = eglQueryDeviceStringEXT( device, EGL_EXTENSIONS );

            DeviceData deviceData;
            deviceData.name = name + ( " #" + StringConverter::toString( i ) );

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
            const char *gpuCard = eglQueryDeviceStringEXT( device, EGL_DRM_DEVICE_FILE_EXT );
            if( gpuCard )
            {
                deviceData.name += " ";
                deviceData.name += gpuCard;
            }
#endif

            LogManager::getSingleton().logMessage( "EGL Device: " + deviceData.name );

            // Always support no MSAA (in case maxSamples returned invalid value of 0)
            deviceData.fsaa.push_back( SampleDescription( 1u ) );

            mDeviceData.push_back( deviceData );

            try
            {
                // Initialize to get FSAA counts
                initDevice( size_t( i ) );

                // We don't know which sample counts are supported (and some colour
                // formats may have further restrictions)
                //
                // However it's usually a safe guess that all powers of 2 between 1 and maxSamples
                // ought to be supported
                PFNGLGETINTEGERVPROC myGlGetIntegerv =
                    (PFNGLGETINTEGERVPROC)eglGetProcAddress( "glGetIntegerv" );
                GLint maxSamples;
                myGlGetIntegerv( GL_MAX_COLOR_TEXTURE_SAMPLES, &maxSamples );

                for( GLint sample = 2u; sample <= maxSamples; sample <<= 1 )
                {
                    mDeviceData.back().fsaa.push_back(
                        SampleDescription( static_cast<uint8>( sample ) ) );
                }

                destroyDevice( size_t( i ) );
            }
            catch( Exception &e )
            {
                LogManager::getSingleton().logMessage( e.getFullDescription(), LML_CRITICAL );
                destroyDevice( size_t( i ) );
            }
        }
    }
    //-------------------------------------------------------------------------
    EglPBufferSupport::~EglPBufferSupport() {}
    //-------------------------------------------------------------------------
    void EglPBufferSupport::addConfig()
    {
        ConfigOption optDevices;
        ConfigOption optFSAA;
        ConfigOption optSRGB;

        optDevices.name = "Device";

        FastArray<DeviceData>::const_iterator itor = mDeviceData.begin();
        FastArray<DeviceData>::const_iterator endt = mDeviceData.end();

        while( itor != endt )
        {
            optDevices.possibleValues.push_back( itor->name );
            ++itor;
        }

        optDevices.currentValue = mDeviceData.front().name;
        optDevices.immutable = false;

        optFSAA.name = "FSAA";
        optFSAA.immutable = false;
        optFSAA.currentValue = StringConverter::toString(
            mDeviceData[getSelectedDeviceIdx()].fsaa.front().getColourSamples() );

        optSRGB.name = "sRGB Gamma Conversion";
        optSRGB.immutable = false;

        optSRGB.possibleValues.push_back( "No" );
        optSRGB.possibleValues.push_back( "Yes" );

        optSRGB.currentValue = optSRGB.possibleValues[1];

        mOptions[optDevices.name] = optDevices;
        mOptions[optFSAA.name] = optFSAA;
        mOptions[optSRGB.name] = optSRGB;

        refreshConfig();
    }
    //-------------------------------------------------------------------------
    void EglPBufferSupport::refreshConfig()
    {
        ConfigOptionMap::iterator optFSAA = mOptions.find( "FSAA" );

        if( optFSAA != mOptions.end() )
        {
            optFSAA->second.possibleValues.clear();

            const size_t deviceIdx = getSelectedDeviceIdx();

            FastArray<SampleDescription>::const_iterator itor = mDeviceData[deviceIdx].fsaa.begin();
            FastArray<SampleDescription>::const_iterator endt = mDeviceData[deviceIdx].fsaa.end();

            while( itor != endt )
            {
                optFSAA->second.possibleValues.push_back(
                    StringConverter::toString( itor->getColourSamples() ) );
                ++itor;
            }

            optFSAA->second.currentValue = optFSAA->second.possibleValues.front();
        }
    }
    //-------------------------------------------------------------------------
    void EglPBufferSupport::setConfigOption( const String &name, const String &value )
    {
        ConfigOptionMap::iterator option = mOptions.find( name );

        if( option == mOptions.end() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Option named " + name + " does not exist.",
                         "EglPBufferSupport::setConfigOption" );
        }
        else
        {
            option->second.currentValue = value;
        }

        if( name == "Device" )
            refreshConfig();
    }
    //-------------------------------------------------------------------------
    String EglPBufferSupport::validateConfig()
    {
        // TODO
        return BLANKSTRING;
    }
    //-------------------------------------------------------------------------
    const char *EglPBufferSupport::getPriorityConfigOption( size_t ) const { return "Device"; }
    //-------------------------------------------------------------------------
    size_t EglPBufferSupport::getNumPriorityConfigOptions() const { return 1u; }
    //-------------------------------------------------------------------------
    Window *EglPBufferSupport::createWindow( bool autoCreateWindow, GL3PlusRenderSystem *renderSystem,
                                             const String &windowTitle )
    {
        Window *window = 0;

        if( autoCreateWindow )
        {
            ConfigOptionMap::iterator opt;
            ConfigOptionMap::iterator end = mOptions.end();
            NameValuePairList miscParams;

            bool fullscreen = false;
            uint w = 1280, h = 720;

            if( ( opt = mOptions.find( "FSAA" ) ) != end )
                miscParams["FSAA"] = opt->second.currentValue;

            if( ( opt = mOptions.find( "sRGB Gamma Conversion" ) ) != end )
                miscParams["gamma"] = opt->second.currentValue;

            window = renderSystem->_createRenderWindow( windowTitle, w, h, fullscreen, &miscParams );
        }

        return window;
    }

    //-------------------------------------------------------------------------
    Window *EglPBufferSupport::newWindow( const String &name, uint32 width, uint32 height,
                                          bool fullscreen, const NameValuePairList *miscParams )
    {
        EglPBufferWindow *window =
            new EglPBufferWindow( name, width, height, fullscreen, miscParams, this );
        return window;
    }
    //-------------------------------------------------------------------------
    void EglPBufferSupport::start()
    {
        LogManager::getSingleton().logMessage(
            "******************************\n"
            "*** Starting EGL Subsystem ***\n"
            "******************************" );
    }
    //-------------------------------------------------------------------------
    void EglPBufferSupport::stop()
    {
        LogManager::getSingleton().logMessage(
            "******************************\n"
            "*** Stopping EGL Subsystem ***\n"
            "******************************" );

        const size_t numDevices = mDevices.size();
        for( size_t i = 0u; i < numDevices; ++i )
            destroyDevice( i );
    }
    //-------------------------------------------------------------------------
    void *EglPBufferSupport::getProcAddress( const char *procname ) const
    {
        return (void *)eglGetProcAddress( procname );
    }
    //-------------------------------------------------------------------------
    void EglPBufferSupport::initDevice( const size_t deviceIdx )
    {
        DeviceData &deviceData = mDeviceData[deviceIdx];

        LogManager::getSingleton().logMessage( "Trying to init device: " + deviceData.name + "..." );

        EGLAttrib attribs[] = { EGL_NONE };
        deviceData.eglDisplay =
            eglGetPlatformDisplay( EGL_PLATFORM_DEVICE_EXT, mDevices[deviceIdx], attribs );

        EGL_CHECK_ERROR

        if( deviceData.eglDisplay == EGL_NO_DISPLAY )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "eglGetPlatformDisplay failed for device " + deviceData.name,
                         "EGLSupport::getGLDisplay" );
        }

        EGLint major = 0, minor = 0;
        if( eglInitialize( deviceData.eglDisplay, &major, &minor ) == EGL_FALSE )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "eglInitialize failed for device " + deviceData.name,
                         "EGLSupport::getGLDisplay" );
        }

        EGL_CHECK_ERROR

        const EGLint configAttribs[] = {
            EGL_SURFACE_TYPE,    EGL_PBUFFER_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,  EGL_NONE
        };

        EGLint numConfigs;
        if( eglChooseConfig( deviceData.eglDisplay, configAttribs, &deviceData.eglCfg, 1,
                             &numConfigs ) == EGL_FALSE )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "eglChooseConfig for device " + deviceData.name, "EGLSupport::getGLDisplay" );
        }

        const EGLint pbufferAttribs[] = {
            EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE,
        };
        deviceData.eglSurf =
            eglCreatePbufferSurface( deviceData.eglDisplay, deviceData.eglCfg, pbufferAttribs );

        // Beware ES must not ask EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR
        // eglBindAPI( EGL_OPENGL_ES_API );
        eglBindAPI( EGL_OPENGL_API );

        EGLint contextAttrs[] = { EGL_CONTEXT_MAJOR_VERSION,
                                  4,
                                  EGL_CONTEXT_MINOR_VERSION,
                                  5,
                                  EGL_CONTEXT_OPENGL_PROFILE_MASK,
                                  EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
#if OGRE_DEBUG_MODE
                                  EGL_CONTEXT_FLAGS_KHR,
                                  EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,
#endif
                                  EGL_NONE };

        while( !deviceData.eglCtx && ( contextAttrs[1] >= 3 ) )
        {
            deviceData.eglCtx =
                eglCreateContext( deviceData.eglDisplay, deviceData.eglCfg, 0, contextAttrs );

            if( deviceData.eglCtx )
            {
                LogManager::getSingleton().logMessage(
                    "Created GL " + StringConverter::toString( contextAttrs[1] ) + "." +
                    StringConverter::toString( contextAttrs[3] ) + " context for device " +
                    deviceData.name );
            }
            else
            {
                if( contextAttrs[3] == 0 )
                {
                    contextAttrs[1] -= 1;
                    contextAttrs[3] = 5;
                }
                else
                {
                    contextAttrs[3] -= 1;
                }
            }
        }

        if( !deviceData.eglCtx )
        {
            LogManager::getSingleton().logMessage(
                "Failed to create an OpenGL 3+ context for device " + deviceData.name, LML_CRITICAL );
        }
        else
        {
            eglMakeCurrent( deviceData.eglDisplay, deviceData.eglSurf, deviceData.eglSurf,
                            deviceData.eglCtx );
        }
    }
    //-------------------------------------------------------------------------
    void EglPBufferSupport::destroyDevice( const size_t deviceIdx )
    {
        DeviceData &deviceData = mDeviceData[deviceIdx];

        LogManager::getSingleton().logMessage( "Destroying device: " + deviceData.name + "..." );

        if( deviceData.eglCtx )
        {
            eglMakeCurrent( deviceData.eglDisplay, 0, 0, 0 );
            eglDestroyContext( deviceData.eglDisplay, deviceData.eglCtx );
            deviceData.eglCtx = 0;
        }

        if( deviceData.eglSurf )
        {
            eglDestroySurface( deviceData.eglDisplay, deviceData.eglSurf );
            deviceData.eglSurf = 0;
        }

        if( deviceData.eglDisplay )
        {
            eglTerminate( deviceData.eglDisplay );
            EGL_CHECK_ERROR
        }

        deviceData.eglCfg = 0;
        deviceData.eglDisplay = 0;
    }
    //-------------------------------------------------------------------------
    const EglPBufferSupport::DeviceData *EglPBufferSupport::getCurrentDevice()
    {
        const size_t selectedDeviceIdx = getSelectedDeviceIdx();
        if( !mDeviceData[selectedDeviceIdx].eglDisplay )
            initDevice( selectedDeviceIdx );
        return &mDeviceData[selectedDeviceIdx];
    }
    //-------------------------------------------------------------------------
    EGLDisplay EglPBufferSupport::getGLDisplay()
    {
        const size_t selectedDeviceIdx = getSelectedDeviceIdx();
        if( mDeviceData[selectedDeviceIdx].eglDisplay )
            return mDeviceData[selectedDeviceIdx].eglDisplay;
        initDevice( selectedDeviceIdx );
        return mDeviceData[selectedDeviceIdx].eglDisplay;
    }
    //-------------------------------------------------------------------------
    uint32 EglPBufferSupport::getSelectedDeviceIdx() const
    {
        uint32 deviceIdx = 0u;

        ConfigOptionMap::const_iterator it = mOptions.find( "Device" );
        if( it != mOptions.end() )
        {
            const String deviceName = it->second.currentValue;
            FastArray<DeviceData>::const_iterator itDevice =
                std::find( mDeviceData.begin(), mDeviceData.end(), deviceName );
            if( itDevice != mDeviceData.end() )
                deviceIdx = static_cast<uint32>( itDevice - mDeviceData.begin() );
        }

        return deviceIdx;
    }
}  // namespace Ogre
