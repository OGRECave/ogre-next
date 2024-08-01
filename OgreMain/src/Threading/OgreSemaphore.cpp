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

#include "OgreStableHeaders.h"

#include "Threading/OgreSemaphore.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
#    define WIN32_LEAN_AND_MEAN
#    define NOMINMAX
#    include <windows.h>
#endif

namespace Ogre
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
    Semaphore::Semaphore( uint32_t intialCount ) : mSemaphore( 0 )
    {
        mSemaphore = CreateSemaphore( NULL, static_cast<LONG>( intialCount ),
                                      std::numeric_limits<LONG>::max(), NULL );
    }
    //-----------------------------------------------------------------------------------
    Semaphore::~Semaphore()
    {
        CloseHandle( mSemaphore );
        mSemaphore = 0;
    }
    //-----------------------------------------------------------------------------------
    bool Semaphore::decrementOrWait()
    {
        return WaitForSingleObject( mSemaphore, INFINITE ) == WAIT_OBJECT_0;
    }
    //-----------------------------------------------------------------------------------
    bool Semaphore::increment() { return ReleaseSemaphore( mSemaphore, 1, NULL ) == 0; }
    //-----------------------------------------------------------------------------------
    bool Semaphore::increment( uint32_t value )
    {
        return ReleaseSemaphore( mSemaphore, static_cast<LONG>( value ), NULL ) == 0;
    }
#else
    Semaphore::Semaphore( uint32_t intialCount ) { sem_init( &mSemaphore, 0, intialCount ); }
    //-----------------------------------------------------------------------------------
    Semaphore::~Semaphore() { sem_destroy( &mSemaphore ); }
    //-----------------------------------------------------------------------------------
    bool Semaphore::decrementOrWait() { return sem_wait( &mSemaphore ) == 0; }
    //-----------------------------------------------------------------------------------
    bool Semaphore::increment() { return sem_post( &mSemaphore ) == 0; }
    //-----------------------------------------------------------------------------------
    bool Semaphore::increment( uint32_t value )
    {
        bool anyErrors = false;
        while( value-- )
            anyErrors |= sem_post( &mSemaphore ) != 0;
        return !anyErrors;
    }
#endif
}  // namespace Ogre
