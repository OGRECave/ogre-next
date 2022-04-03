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

#include "Threading/OgreWaitableEvent.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
#    define WIN32_LEAN_AND_MEAN
#    define NOMINMAX
#    include <windows.h>
#endif

namespace Ogre
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
    WaitableEvent::WaitableEvent() : mEvent( 0 ) { mEvent = CreateEvent( NULL, FALSE, FALSE, NULL ); }
    //-----------------------------------------------------------------------------------
    WaitableEvent::~WaitableEvent() { CloseHandle( mEvent ); }
    //-----------------------------------------------------------------------------------
    bool WaitableEvent::wait()
    {
        DWORD result = WaitForSingleObject( mEvent, INFINITE );
        return result == WAIT_OBJECT_0;
    }
    //-----------------------------------------------------------------------------------
    void WaitableEvent::wake()
    {
        // In Windows' lingo:
        // signaled = go
        // not signaled = must wait.
        SetEvent( mEvent );
    }
#else
    WaitableEvent::WaitableEvent() : mWait( true )
    {
        pthread_mutex_init( &mMutex, 0 );
        pthread_cond_init( &mCondition, 0 );
    }
    //-----------------------------------------------------------------------------------
    WaitableEvent::~WaitableEvent()
    {
        pthread_cond_destroy( &mCondition );
        pthread_mutex_destroy( &mMutex );
    }
    //-----------------------------------------------------------------------------------
    bool WaitableEvent::wait()
    {
        pthread_mutex_lock( &mMutex );
        while( mWait )
            pthread_cond_wait( &mCondition, &mMutex );
        mWait = true;
        pthread_mutex_unlock( &mMutex );
        return true;
    }
    //-----------------------------------------------------------------------------------
    void WaitableEvent::wake()
    {
        pthread_mutex_lock( &mMutex );
        mWait = false;
        pthread_cond_signal( &mCondition );
        pthread_mutex_unlock( &mMutex );
    }
#endif
}  // namespace Ogre
