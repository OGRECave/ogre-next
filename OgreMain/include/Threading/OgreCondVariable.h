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

#ifndef OgreCondVariable_H
#define OgreCondVariable_H

#include "OgrePlatform.h"

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    define VC_EXTRALEAN
#    define NOMINMAX
#    include <windows.h>
#else
#    include <pthread.h>
#endif

namespace Ogre
{
    typedef bool ( *CondVariableWaitFunc )( void *userData );

    /**
    @remarks
    @author
        Matias N. Goldberg
    */
    class _OgreExport CondVariable
    {
#ifdef _WIN32
        CRITICAL_SECTION   mMutex;
        CONDITION_VARIABLE mCondVariable;
#else
        pthread_mutex_t mMutex;
        pthread_cond_t  mCondVariable;
#endif

    public:
        CondVariable();
        ~CondVariable();

        void lock();
        void unlock();

        /** Blocks until waitFunc returns false.
            Other threads need to call notifyOne or notifyAll to wake up this thread

            You shouldn't call this inside lock() and unlock()
        @param waitFunc
        @param userData
        */
        void wait( CondVariableWaitFunc waitFunc, void *userData );

        /** Wakes up at least one thread being blocked by CondVariable::wait

            You can modify the condition and call notifyOne while inside lock/unlock pairs:
                condVar.lock();
                bCondition = true;
                condVar.notifyOne();
                condVar.unlock();
            or outside:
                condVar.lock();
                bCondition = true;
                condVar.unlock();
                condVar.notifyOne();
        */
        void notifyOne();

        /// Same as notifyOne, but wakes up all threads
        void notifyAll();
    };

    class _OgreExport ScopedCvLock
    {
        CondVariable &mCv;

    public:
        ScopedCvLock( CondVariable &cv ) : mCv( cv ) { mCv.lock(); }
        ~ScopedCvLock() { mCv.unlock(); }
    };
}  // namespace Ogre

#endif
