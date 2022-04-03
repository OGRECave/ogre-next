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
#include "OgreStableHeaders.h"

#include "OgreDynLib.h"

#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
#    define WIN32_LEAN_AND_MEAN
#    if !defined( NOMINMAX ) && defined( _MSC_VER )
#        define NOMINMAX  // required to stop windows.h messing up std::min
#    endif
#    include <windows.h>
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT
#    include "OgreUTFString.h"
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
#    include "macUtils.h"
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
#    include <dlfcn.h>
#endif

namespace Ogre
{
    //-----------------------------------------------------------------------
    DynLib::DynLib( const String &name )
    {
        mName = name;
        mInst = NULL;
    }

    //-----------------------------------------------------------------------
    DynLib::~DynLib() {}

    //-----------------------------------------------------------------------
    void DynLib::load( const bool bOptional )
    {
        // Log library load
        LogManager::getSingleton().logMessage( "Loading library " + mName );

        String name = mName;
#if OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
        if( name.find( ".js" ) == String::npos )
            name += ".js";
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX || OGRE_PLATFORM == OGRE_PLATFORM_FREEBSD
        // dlopen() does not add .so to the filename, like windows does for .dll
        if( name.find( ".so" ) == String::npos )
        {
            name += ".so.";
            name += StringConverter::toString( OGRE_VERSION_MAJOR ) + ".";
            name += StringConverter::toString( OGRE_VERSION_MINOR ) + ".";
            name += StringConverter::toString( OGRE_VERSION_PATCH );
        }
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        // dlopen() does not add .dylib to the filename, like windows does for .dll
        if( name.substr( name.find_last_of( "." ) + 1 ) != "dylib" )
            name += ".dylib";
#elif OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
        // Although LoadLibraryEx will add .dll itself when you only specify the library name,
        // if you include a relative path then it does not. So, add it to be sure.
        if( name.substr( name.find_last_of( "." ) + 1 ) != "dll" )
            name += ".dll";
#endif
        mInst = (DYNLIB_HANDLE)DYNLIB_LOAD( name.c_str() );
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        if( !mInst )
        {
            // Try again as a framework
            mInst = (DYNLIB_HANDLE)FRAMEWORK_LOAD( mName );
        }
#endif
        if( !mInst )
        {
            if( !bOptional )
            {
                OGRE_EXCEPT(
                    Exception::ERR_INTERNAL_ERROR,
                    "Could not load dynamic library " + mName + ".  System Error: " + dynlibError(),
                    "DynLib::load" );
            }
            else
            {
                LogManager::getSingleton().logMessage( "Could not load optional dynamic library " +
                                                           mName + ".  System Error: " + dynlibError(),
                                                       LML_CRITICAL );
            }
        }
    }

    //-----------------------------------------------------------------------
    void DynLib::unload()
    {
        // Log library unload
        LogManager::getSingleton().logMessage( "Unloading library " + mName );

        if( mInst )
        {
            if( DYNLIB_UNLOAD( mInst ) )
            {
                OGRE_EXCEPT(
                    Exception::ERR_INTERNAL_ERROR,
                    "Could not unload dynamic library " + mName + ".  System Error: " + dynlibError(),
                    "DynLib::unload" );
            }
        }
    }
    //-----------------------------------------------------------------------
    bool DynLib::isLoaded() const { return mInst != NULL; }
    //-----------------------------------------------------------------------
    void *DynLib::getSymbol( const String &strName ) const noexcept
    {
        return (void *)DYNLIB_GETSYM( mInst, strName.c_str() );
    }
    //-----------------------------------------------------------------------
    String DynLib::dynlibError()
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        LPTSTR lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,                                         //
            GetLastError(),                               //
            MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  //
            (LPTSTR)&lpMsgBuf, 0, NULL );
        String ret = lpMsgBuf;
        // Free the buffer.
        LocalFree( lpMsgBuf );
        return ret;
#elif OGRE_PLATFORM == OGRE_PLATFORM_WINRT
        WCHAR wideMsgBuf[1024];
        if( 0 == FormatMessageW( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,  //
                                 NULL,                                                        //
                                 GetLastError(),                                              //
                                 MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),                 //
                                 wideMsgBuf,                                                  //
                                 sizeof( wideMsgBuf ) / sizeof( wideMsgBuf[0] ),              //
                                 NULL ) )
        {
            wideMsgBuf[0] = 0;
        }
#    if OGRE_WCHAR_T_STRINGS
        String ret = wideMsgBuf;
#    else
        char narrowMsgBuf[2048] = "";
        if( 0 == WideCharToMultiByte( CP_ACP, 0, wideMsgBuf, -1, narrowMsgBuf,
                                      sizeof( narrowMsgBuf ) / sizeof( narrowMsgBuf[0] ), NULL, NULL ) )
        {
            narrowMsgBuf[0] = 0;
        }
        String ret = narrowMsgBuf;
#    endif
        return ret;
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX || OGRE_PLATFORM == OGRE_PLATFORM_APPLE || \
    OGRE_PLATFORM == OGRE_PLATFORM_FREEBSD
        const char *errorStr = dlerror();
        if( errorStr )
            return String( errorStr );
        else
            return String( "" );
#else
        return String( "" );
#endif
    }

}  // namespace Ogre
