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

#include "OgreLog.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
#    include <windows.h>
#endif

namespace Ogre
{
    //-----------------------------------------------------------------------
    Log::Log( const String &name, bool debuggerOuput, bool suppressFile ) :
        mLogLevel( LL_NORMAL ),
        mDebugOut( debuggerOuput ),
        mSuppressFile( suppressFile ),
        mTimeStamp( true ),
        mLogName( name )
    {
        if( !mSuppressFile )
        {
            mLog = new std::ofstream;
            mLog->open( name.c_str() );
        }
    }
    //-----------------------------------------------------------------------
    Log::~Log()
    {
        ScopedLock scopedLock( mMutex );
        if( !mSuppressFile )
        {
            mLog->close();
            delete mLog;
        }
    }
    //-----------------------------------------------------------------------
    void Log::logMessage( const String &message, LogMessageLevel lml, bool maskDebug )
    {
        ScopedLock scopedLock( mMutex );
        if( ( mLogLevel + lml ) >= OGRE_LOG_THRESHOLD )
        {
            bool skipThisMessage = false;
            for( mtLogListener::iterator i = mListeners.begin(); i != mListeners.end(); ++i )
                ( *i )->messageLogged( message, lml, maskDebug, mLogName, skipThisMessage );

            if( !skipThisMessage )
            {
                if( mDebugOut && !maskDebug )
                {
#if( OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT ) && OGRE_DEBUG_MODE
#    if OGRE_WCHAR_T_STRINGS
                    OutputDebugStringW( L"Ogre: " );
                    OutputDebugStringW( message.c_str() );
                    OutputDebugStringW( L"\n" );
#    else
                    OutputDebugStringA( "Ogre: " );
                    OutputDebugStringA( message.c_str() );
                    OutputDebugStringA( "\n" );
#    endif
#endif
                    if( lml == LML_CRITICAL )
                        std::cerr << message << std::endl;
                    else
                        std::cout << message << std::endl;
                }

                // Write time into log
                if( !mSuppressFile )
                {
                    if( mTimeStamp )
                    {
                        struct tm *pTime;
                        time_t ctTime;
                        time( &ctTime );
                        pTime = localtime( &ctTime );
                        *mLog << std::setw( 2 ) << std::setfill( '0' ) << pTime->tm_hour << ":"
                              << std::setw( 2 ) << std::setfill( '0' ) << pTime->tm_min << ":"
                              << std::setw( 2 ) << std::setfill( '0' ) << pTime->tm_sec << ": ";
                    }
                    *mLog << message << std::endl;

                    // Flush stcmdream to ensure it is written (incase of a crash, we need log to be up
                    // to date)
                    mLog->flush();
                }
            }
        }
    }

    //-----------------------------------------------------------------------
    void Log::setTimeStampEnabled( bool timeStamp )
    {
        ScopedLock scopedLock( mMutex );
        mTimeStamp = timeStamp;
    }

    //-----------------------------------------------------------------------
    void Log::setDebugOutputEnabled( bool debugOutput )
    {
        ScopedLock scopedLock( mMutex );
        mDebugOut = debugOutput;
    }

    //-----------------------------------------------------------------------
    void Log::setLogDetail( LoggingLevel ll )
    {
        ScopedLock scopedLock( mMutex );
        mLogLevel = ll;
    }

    //-----------------------------------------------------------------------
    void Log::addListener( LogListener *listener )
    {
        ScopedLock scopedLock( mMutex );
        mListeners.push_back( listener );
    }

    //-----------------------------------------------------------------------
    void Log::removeListener( LogListener *listener )
    {
        ScopedLock scopedLock( mMutex );
        mListeners.erase( std::find( mListeners.begin(), mListeners.end(), listener ) );
    }
    //---------------------------------------------------------------------
    Log::Stream Log::stream( LogMessageLevel lml, bool maskDebug )
    {
        return Stream( this, lml, maskDebug );
    }
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    LogListener::~LogListener() {}
    //---------------------------------------------------------------------
    Log::Stream::Stream( Log *target, LogMessageLevel lml, bool maskDebug ) :
        mTarget( target ),
        mLevel( lml ),
        mMaskDebug( maskDebug ),
        mCache( new BaseStream() )
    {
    }
    //---------------------------------------------------------------------
    Log::Stream::Stream( const Stream &rhs ) :
        mTarget( rhs.mTarget ),
        mLevel( rhs.mLevel ),
        mMaskDebug( rhs.mMaskDebug ),
        mCache( new BaseStream() )
    {
        // explicit copy of stream required, gcc doesn't like implicit
        mCache->str( rhs.mCache->str() );
    }
    //---------------------------------------------------------------------
    Log::Stream::~Stream()
    {
        // flush on destroy
        if( mCache->tellp() > 0 )
        {
            mTarget->logMessage( mCache->str(), mLevel, mMaskDebug );
        }

        delete mCache;
        mCache = 0;
    }
}  // namespace Ogre
