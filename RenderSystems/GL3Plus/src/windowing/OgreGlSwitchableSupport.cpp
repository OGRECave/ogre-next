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

#include "windowing/OgreGlSwitchableSupport.h"

#include "OgreException.h"
#include "OgreGL3PlusRenderSystem.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"

#ifdef OGRE_GLSUPPORT_USE_GLX
#    include "windowing/GLX/OgreGLXGLSupport.h"
#endif
#ifdef OGRE_GLSUPPORT_USE_WGL
#    include "windowing/win32/OgreWin32GLSupport.h"
#endif
#ifdef OGRE_GLSUPPORT_USE_EGL_HEADLESS
#    include "windowing/EGL/PBuffer/OgreEglPBufferSupport.h"
#endif

namespace Ogre
{
    //-------------------------------------------------------------------------
    GlSwitchableSupport::GlSwitchableSupport() : mSelectedInterface( 0u ), mInterfaceSelected( false )
    {
#ifdef OGRE_GLSUPPORT_USE_GLX
        try
        {
            mAvailableInterfaces.push_back( Interface( WindowNative, new GLXGLSupport() ) );
        }
        catch( Exception &e )
        {
            LogManager::getSingleton().logMessage(
                "GLX raised an exception. Won't be available. Is X11 running?" );
            LogManager::getSingleton().logMessage( e.getFullDescription() );
        }
#endif
#ifdef OGRE_GLSUPPORT_USE_WGL
        mAvailableInterfaces.push_back( Interface( WindowNative, new Win32GLSupport() ) );
#endif
#ifdef OGRE_GLSUPPORT_USE_EGL_HEADLESS
        try
        {
            mAvailableInterfaces.push_back( Interface( HeadlessEgl, new EglPBufferSupport() ) );
        }
        catch( Exception &e )
        {
            LogManager::getSingleton().logMessage(
                "EGL Headless raised an exception. Won't be available. Are drivers too old?" );
            LogManager::getSingleton().logMessage( e.getFullDescription() );
        }
#endif

        if( mAvailableInterfaces.empty() )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "No Interface could be loaded. Check previous error messages."
                         "Try disabling OpenGL plugin from plugins.cfg.",
                         "GlSwitchableSupport::GlSwitchableSupport" );
        }
    }  // namespace Ogre
    //-------------------------------------------------------------------------
    GlSwitchableSupport::~GlSwitchableSupport()
    {
        FastArray<Interface>::const_iterator itor = mAvailableInterfaces.begin();
        FastArray<Interface>::const_iterator endt = mAvailableInterfaces.end();

        while( itor != endt )
        {
            delete itor->support;
            ++itor;
        }
        mAvailableInterfaces.clear();
    }
    //-------------------------------------------------------------------------
    const char *GlSwitchableSupport::getInterfaceName( InterfaceType interface )
    {
        switch( interface )
        {
        case WindowNative:
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX || OGRE_PLATFORM == OGRE_PLATFORM_FREEBSD
            return "GLX Window (Default)";
#elif OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            return "WGL Window (Default)";
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
            return "Cocoa Window (Default)";
#else
#    error Unsupported platform. Build without GL Switchable support
#endif
        case HeadlessEgl:
            return "Headless EGL / PBuffer";
        }

        return "ERROR";
    }
    //-------------------------------------------------------------------------
    void GlSwitchableSupport::addConfig()
    {
        mInterfaceSelected = false;

        ConfigOption optInterfaces;

        optInterfaces.name = "Interface";

        FastArray<Interface>::const_iterator itor = mAvailableInterfaces.begin();
        FastArray<Interface>::const_iterator endt = mAvailableInterfaces.end();

        while( itor != endt )
        {
            optInterfaces.possibleValues.push_back( getInterfaceName( itor->type ) );
            ++itor;
        }

        optInterfaces.currentValue = optInterfaces.possibleValues[mSelectedInterface];
        optInterfaces.immutable = false;

        mAvailableInterfaces[mSelectedInterface].support->addConfig();

        mOptions[optInterfaces.name] = optInterfaces;
        mOptions.insert( mAvailableInterfaces[mSelectedInterface].support->getConfigOptions().begin(),
                         mAvailableInterfaces[mSelectedInterface].support->getConfigOptions().end() );

        refreshConfig();
    }
    //-------------------------------------------------------------------------
    void GlSwitchableSupport::refreshConfig()
    {
        ConfigOptionMap::iterator optInterfaces = mOptions.find( "Interface" );

        if( optInterfaces != mOptions.end() )
        {
            const uint8 newInterfaceIdx = findSelectedInterfaceIdx();
            if( newInterfaceIdx != mSelectedInterface )
            {
                ConfigOptionMap::iterator itNext = optInterfaces;
                ++itNext;

                mOptions.erase( mOptions.begin(), optInterfaces );
                mOptions.erase( itNext, mOptions.end() );

                stop();
                mSelectedInterface = newInterfaceIdx;
                start();
                mAvailableInterfaces[mSelectedInterface].support->addConfig();
                mOptions.insert(
                    mAvailableInterfaces[mSelectedInterface].support->getConfigOptions().begin(),
                    mAvailableInterfaces[mSelectedInterface].support->getConfigOptions().end() );
            }
        }
    }
    //-------------------------------------------------------------------------
    void GlSwitchableSupport::setConfigOption( const String &name, const String &value )
    {
        ConfigOptionMap::iterator option = mOptions.find( name );

        if( name == "Interface" )
        {
            option->second.currentValue = value;
            refreshConfig();
        }
        else
        {
            mAvailableInterfaces[mSelectedInterface].support->setConfigOption( name, value );

            // Update our copy of mOptions
            const ConfigOptionMap &interfOpts =
                mAvailableInterfaces[mSelectedInterface].support->getConfigOptions();
            ConfigOptionMap::const_iterator itOpt = interfOpts.find( name );
            if( interfOpts.find( name ) != interfOpts.end() )
                mOptions[name] = itOpt->second;
        }
    }
    //-------------------------------------------------------------------------
    String GlSwitchableSupport::validateConfig()
    {
        // TODO
        return BLANKSTRING;
    }
    //-------------------------------------------------------------------------
    const char *GlSwitchableSupport::getPriorityConfigOption( size_t idx ) const
    {
        if( idx > 0u )
            return mAvailableInterfaces[mSelectedInterface].support->getPriorityConfigOption( idx );
        return "Interface";
    }
    //-------------------------------------------------------------------------
    size_t GlSwitchableSupport::getNumPriorityConfigOptions() const
    {
        return 1u + mAvailableInterfaces[mSelectedInterface].support->getNumPriorityConfigOptions();
    }
    //-------------------------------------------------------------------------
    Window *GlSwitchableSupport::createWindow( bool autoCreateWindow, GL3PlusRenderSystem *renderSystem,
                                               const String &windowTitle )
    {
        return mAvailableInterfaces[mSelectedInterface].support->createWindow(
            autoCreateWindow, renderSystem, windowTitle );
    }
    //-------------------------------------------------------------------------
    Window *GlSwitchableSupport::newWindow( const String &name, uint32 width, uint32 height,
                                            bool fullscreen, const NameValuePairList *miscParams )
    {
        return mAvailableInterfaces[mSelectedInterface].support->newWindow( name, width, height,
                                                                            fullscreen, miscParams );
    }
    //-------------------------------------------------------------------------
    void GlSwitchableSupport::start() { mAvailableInterfaces[mSelectedInterface].support->start(); }
    //-------------------------------------------------------------------------
    void GlSwitchableSupport::stop() { mAvailableInterfaces[mSelectedInterface].support->stop(); }
    //-------------------------------------------------------------------------
    void *GlSwitchableSupport::getProcAddress( const char *procname ) const
    {
        return mAvailableInterfaces[mSelectedInterface].support->getProcAddress( procname );
    }
    //-------------------------------------------------------------------------
    uint8 GlSwitchableSupport::findSelectedInterfaceIdx() const
    {
        ConfigOptionMap::const_iterator it = mOptions.find( "Interface" );
        if( it != mOptions.end() )
        {
            uint8 interfaceIdx = 0u;

            FastArray<Interface>::const_iterator itor = mAvailableInterfaces.begin();
            FastArray<Interface>::const_iterator endt = mAvailableInterfaces.end();

            while( itor != endt )
            {
                if( it->second.currentValue == getInterfaceName( itor->type ) )
                    return interfaceIdx;
                ++interfaceIdx;
                ++itor;
            }
        }

        return 0u;
    }
}  // namespace Ogre
