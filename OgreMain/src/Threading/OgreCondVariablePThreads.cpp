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

#ifndef _WIN32

#    include "Threading/OgreCondVariable.h"

namespace Ogre
{
    CondVariable::CondVariable()
    {
        pthread_mutex_init( &mMutex, 0 );
        pthread_cond_init( &mCondVariable, 0 );
    }
    //-----------------------------------------------------------------------------------
    CondVariable::~CondVariable()
    {
        pthread_cond_destroy( &mCondVariable );
        pthread_mutex_destroy( &mMutex );
    }
    //-----------------------------------------------------------------------------------
    void CondVariable::lock( void ) { pthread_mutex_lock( &mMutex ); }
    //-----------------------------------------------------------------------------------
    void CondVariable::unlock( void ) { pthread_mutex_unlock( &mMutex ); }
    //-----------------------------------------------------------------------------------
    void CondVariable::wait( CondVariableWaitFunc waitFunc, void *userData )
    {
        //		WaitForSingleObject(evt);   // 1
        //		EnterCriticalSection(&cs);  // 2
        //		//... fetching data from the queue
        //		LeaveCriticalSection(&cs);  // 3
        pthread_mutex_lock( &mMutex );
        while( ( *waitFunc )( userData ) )
            pthread_cond_wait( &mCondVariable, &mMutex );
        pthread_mutex_unlock( &mMutex );
    }
    //-----------------------------------------------------------------------------------
    void CondVariable::notifyOne( void ) { pthread_cond_signal( &mCondVariable ); }
    //-----------------------------------------------------------------------------------
    void CondVariable::notifyAll( void ) { pthread_cond_broadcast( &mCondVariable ); }
}  // namespace Ogre

#endif
