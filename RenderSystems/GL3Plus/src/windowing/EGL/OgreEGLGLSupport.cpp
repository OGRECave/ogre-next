/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE
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

#include "OgreEGLGLSupport.h"

#include "OgreException.h"
#include "OgreGL3PlusRenderSystem.h"
#include "OgreLogManager.h"
#include "OgreRoot.h"
#include "OgreStringConverter.h"

#include "OgreEGLWindow.h"

#include <sstream>

typedef EGLBoolean ( *EGLQueryDevicesType )( EGLint, EGLDeviceEXT *, EGLint * );

static EGLQueryDevicesType eglQueryDevices;
static PFNEGLQUERYDEVICESTRINGEXTPROC eglQueryDeviceStringEXT;

namespace Ogre
{
    //-------------------------------------------------------------------------
    EGLGLSupport::EGLGLSupport() : mEglDisplay( EGL_NO_DISPLAY )
    {
        eglQueryDevices = (EGLQueryDevicesType)eglGetProcAddress( "eglQueryDevicesEXT" );
        eglQueryDeviceStringEXT =
            (PFNEGLQUERYDEVICESTRINGEXTPROC)eglGetProcAddress( "eglQueryDeviceStringEXT" );

        if( eglQueryDevices == 0 )
        {
            LogManager::getSingleton().logMessage(
                "[ERROR] eglQueryDevicesEXT not found. EGL driver too old. "
                "Update your GPU drivers and try again" );

            // Do not throw. It will cause all plugins to fail to load.
            // This is may be a recoverable error
            return;
        }

        if( eglQueryDeviceStringEXT == 0 )
        {
            LogManager::getSingleton().logMessage(
                "[ERROR] eglQueryDeviceStringEXT not found. EGL driver too old. "
                "Update your GPU drivers and try again" );

            // Do not throw. It will cause all plugins to fail to load.
            // This is may be a recoverable error
            return;
        }

        EGLint numDevices = 0;
        eglQueryDevices( 0, 0, &numDevices );

        if( numDevices > 0 )
        {
            mDevices.resize( static_cast<size_t>( numDevices ) );
            mDeviceNames.reserve( static_cast<size_t>( numDevices ) );

            eglQueryDevices( numDevices, mDevices.begin(), &numDevices );
        }

        LogManager::getSingleton().logMessage( "Found Num EGL Devices: " +
                                               StringConverter::toString( numDevices ) );

        for( int i = 0u; i < numDevices; ++i )
        {
            const char *name =
                eglQueryDeviceStringEXT( mDevices[i], EGL_EXTENSIONS /*EGL_DRM_DEVICE_FILE_EXT*/ );

            String deviceName;
            deviceName = name + ( " #" + StringConverter::toString( i ) );

            LogManager::getSingleton().logMessage( "EGL Device: " + deviceName );

            mDeviceNames.push_back( deviceName );
        }
    }
    //-------------------------------------------------------------------------
    EGLGLSupport::~EGLGLSupport() {}
    //-------------------------------------------------------------------------
    void EGLGLSupport::addConfig( void )
    {
        ConfigOption optDevices;
        ConfigOption optSRGB;

        optDevices.name = "Device";

        FastArray<String>::const_iterator itor = mDeviceNames.begin();
        FastArray<String>::const_iterator endt = mDeviceNames.end();

        while( itor != endt )
            optDevices.possibleValues.push_back( *itor++ );

        optDevices.currentValue = mDeviceNames.front();
        optDevices.immutable = false;

        optSRGB.name = "sRGB Gamma Conversion";
        optSRGB.immutable = false;

        optSRGB.possibleValues.push_back( "No" );
        optSRGB.possibleValues.push_back( "Yes" );

        optSRGB.currentValue = optSRGB.possibleValues[1];

        mOptions[optDevices.name] = optDevices;
        mOptions[optSRGB.name] = optSRGB;

        refreshConfig();
    }
    //-------------------------------------------------------------------------
    void EGLGLSupport::refreshConfig( void ) {}
    //-------------------------------------------------------------------------
    void EGLGLSupport::setConfigOption( const String &name, const String &value )
    {
        ConfigOptionMap::iterator option = mOptions.find( name );

        if( option == mOptions.end() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Option named " + name + " does not exist.",
                         "EGLGLSupport::setConfigOption" );
        }
        else
        {
            option->second.currentValue = value;
        }

        // if( name == "Video Mode" || name == "Full Screen" )
        // refreshConfig();
    }
    //-------------------------------------------------------------------------
    String EGLGLSupport::validateConfig( void )
    {
        // TODO
        return BLANKSTRING;
    }
    //-------------------------------------------------------------------------
    Window *EGLGLSupport::createWindow( bool autoCreateWindow, GL3PlusRenderSystem *renderSystem,
                                        const String &windowTitle )
    {
        Window *window = 0;

        if( autoCreateWindow )
        {
            ConfigOptionMap::iterator opt;
            ConfigOptionMap::iterator end = mOptions.end();
            NameValuePairList miscParams;

            bool fullscreen = false;
            uint w = 800, h = 600;

            /*if( ( opt = mOptions.find( "FSAA" ) ) != end )
                miscParams["FSAA"] = opt->second.currentValue;

            if( ( opt = mOptions.find( "VSync" ) ) != end )
                miscParams["vsync"] = opt->second.currentValue;*/

            if( ( opt = mOptions.find( "sRGB Gamma Conversion" ) ) != end )
                miscParams["gamma"] = opt->second.currentValue;

            window = renderSystem->_createRenderWindow( windowTitle, w, h, fullscreen, &miscParams );
        }

        return window;
    }

    //-------------------------------------------------------------------------
    Window *EGLGLSupport::newWindow( const String &name, uint32 width, uint32 height, bool fullscreen,
                                     const NameValuePairList *miscParams )
    {
        EGLWindow *window =
            new EGLWindow( name, width, height, fullscreen, PFG_UNKNOWN, miscParams, this );
        return window;
    }
    //-------------------------------------------------------------------------
    void EGLGLSupport::start()
    {
        LogManager::getSingleton().logMessage(
            "******************************\n"
            "*** Starting EGL Subsystem ***\n"
            "******************************" );
    }
    //-------------------------------------------------------------------------
    void EGLGLSupport::stop()
    {
        LogManager::getSingleton().logMessage(
            "******************************\n"
            "*** Stopping EGL Subsystem ***\n"
            "******************************" );

        if( mEglDisplay != EGL_NO_DISPLAY )
        {
            eglTerminate( mEglDisplay );
            EGL_CHECK_ERROR
            mEglDisplay = 0;
        }
    }
    //-------------------------------------------------------------------------
    void *EGLGLSupport::getProcAddress( const char *procname ) const
    {
        return (void *)eglGetProcAddress( procname );
    }
    //-------------------------------------------------------------------------
    ::EGLContext EGLGLSupport::createNewContext( EGLDisplay eglDisplay, EGLConfig eglCfg,
                                                 ::EGLContext shareList ) const
    {
        ::EGLContext eglContext = NULL;
        EGLint contextattrs[] = {
            EGL_CONTEXT_MAJOR_VERSION,
            4,
            EGL_CONTEXT_MINOR_VERSION,
            5,
            EGL_CONTEXT_OPENGL_PROFILE_MASK,
            EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
#if OGRE_DEBUG_MODE
            EGL_CONTEXT_FLAGS_KHR,
            EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,
#endif
            EGL_NONE
        };

        if( !eglDisplay )
            eglDisplay = mEglDisplay;

        while( !eglContext && ( contextattrs[1] >= 3 ) )
        {
            eglContext = eglCreateContext( eglDisplay, eglCfg, shareList, contextattrs );

            if( eglContext )
            {
                LogManager::getSingleton().logMessage(
                    "Created GL " + StringConverter::toString( contextattrs[1] ) + "." +
                    StringConverter::toString( contextattrs[3] ) + " context" );
            }
            else
            {
                if( contextattrs[3] == 0 )
                {
                    contextattrs[1] -= 1;
                    contextattrs[3] = 5;
                }
                else
                {
                    contextattrs[3] -= 1;
                }
            }
        }

        if( !eglContext )
        {
            LogManager::getSingleton().logMessage( "Failed to create an OpenGL 3+ context",
                                                   LML_CRITICAL );
        }

        return eglContext;
    }
    //-------------------------------------------------------------------------
    EGLDisplay EGLGLSupport::getGLDisplay( void )
    {
        if( mEglDisplay )
            return mEglDisplay;

        // mEglDisplay = eglGetDisplay( EGL_DEFAULT_DISPLAY );
        const uint32 deviceIdx = getSelectedDeviceIdx();

        EGLAttrib attribs[] = { EGL_NONE };
        mEglDisplay =
            eglGetPlatformDisplay( EGL_PLATFORM_DEVICE_EXT, mDevices[getSelectedDeviceIdx()], attribs );

        EGL_CHECK_ERROR

        if( mEglDisplay == EGL_NO_DISPLAY )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Couldn't open EGLDisplay for device " + mDeviceNames[deviceIdx],
                         "EGLSupport::getGLDisplay" );
        }

        EGLint major = 0, minor = 0;
        if( eglInitialize( mEglDisplay, &major, &minor ) == EGL_FALSE )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Couldn't initialize EGLDisplay for device " + mDeviceNames[deviceIdx],
                         "EGLSupport::getGLDisplay" );
        }

        EGL_CHECK_ERROR

        return mEglDisplay;
    }
    //-------------------------------------------------------------------------
    uint32 EGLGLSupport::getSelectedDeviceIdx( void ) const
    {
        uint32 deviceIdx = 0u;

        ConfigOptionMap::const_iterator it = mOptions.find( "Device" );
        if( it != mOptions.end() )
        {
            const String deviceName = it->second.currentValue;
            FastArray<String>::const_iterator itDevice =
                std::find( mDeviceNames.begin(), mDeviceNames.end(), deviceName );
            if( itDevice != mDeviceNames.end() )
                deviceIdx = static_cast<uint32>( itDevice - mDeviceNames.begin() );
        }

        return deviceIdx;
    }
}  // namespace Ogre
