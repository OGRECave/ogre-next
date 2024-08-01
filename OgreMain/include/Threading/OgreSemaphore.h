/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2023 Torus Knot Software Ltd

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

#ifndef _OgreSemaphore_H_
#define _OgreSemaphore_H_

#include "OgrePlatform.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
// No need to include the heavy windows.h header for something like this!
typedef void *HANDLE;
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
#    include <dispatch/dispatch.h>
#else
#    include <semaphore.h>
#endif

namespace Ogre
{
    /// Implements a semaphore
    class _OgreExport Semaphore
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
        HANDLE mSemaphore;
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        dispatch_semaphore_t mSemaphore;
#else
        sem_t mSemaphore;
#endif

    public:
        Semaphore( uint32_t intialCount );
        ~Semaphore();

        /// Decrements the semaphore by 1.
        /// If the value would reach -1, it blocks the thread and clamps it to 0
        ///
        /// On error returns false, true if everything's ok.
        bool decrementOrWait();

        /// Increments the semaphore by 1. May wake up a thread stalled on decrement()
        ///
        /// On error returns false, true if everything's ok.
        bool increment();

        /// Increments the semaphore by the given value.
        bool increment( uint32_t value );
    };
}  // namespace Ogre

#endif
