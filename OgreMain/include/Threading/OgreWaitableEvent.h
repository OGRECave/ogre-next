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

#ifndef _OgreWaitableEvent_H_
#define _OgreWaitableEvent_H_

#include "OgrePlatform.h"
#include "OgrePlatformInformation.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    //No need to include the heavy windows.h header for something like this!
    typedef void* HANDLE;
#else
    #include <pthread.h>
#endif

namespace Ogre
{
    /** A WaitableEvent is useful in the scenario of a singler consumer, multiple producers
        The consumer will wait until any of the producers wake it up to consume the work.
    @par
        It is not recommended for a multiple consumers scenario; as you would need one
        WaitableEvent per consumer, which would be expensive and won't scale.
    @par
        On Windows it uses an auto-reset event.
        On other OSes it uses a condition variable.
    */
    class _OgreExport WaitableEvent
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        HANDLE  mEvent;
#else
        bool mWait;
        pthread_mutex_t mMutex;
        pthread_cond_t  mCondition;
#endif

    public:
        WaitableEvent();
        ~WaitableEvent();

        /// Waits until another thread calls wake(). If wake has already been called,
        /// we'll return immediately, and we'll go to sleep if you call wait again
        /// (unless wake was called again in the middle, of course).
        /// If wake has never been called yet, we go to sleep.
        /// On error returns false, true if everything's ok.
        bool wait();

        /// Tells the other thread to wake up from wait().
        /// Calling wake multiple times without any wait in the middle is the same
        /// as calling it just once.
        void wake();
    };
}

#endif
