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

#include "Threading/OgreThreads.h"

#include "Threading/OgreLightweightMutex.h"

#include "ogrestd/vector.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

// The SetThreadDescription API appeared in version 1607 of Windows 10.
typedef HRESULT( WINAPI *SetThreadDescriptionFuncDecl )( HANDLE hThread, PCWSTR lpThreadDescription );

namespace Ogre
{
    extern std::wstring to_wpath( const String &text, unsigned codepage = CP_UTF8 );

    static LightweightMutex mOsHandleMutex;

    ThreadHandle::ThreadHandle( size_t threadIdx, void *userParam ) :
        mThread( 0 ),
        mThreadIdx( threadIdx ),
        mUserParam( userParam )
    {
    }
    //-----------------------------------------------------------------------------------
    ThreadHandle::~ThreadHandle()
    {
        if( mThread )
            CloseHandle( mThread );
        mThread = 0;
    }
    //-----------------------------------------------------------------------------------
    void ThreadHandle::_setOsHandleToSelf()
    {
        ScopedLock lock( mOsHandleMutex );
        HANDLE handle = ::GetCurrentThread();
        _setOsHandle( handle );
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------

    ThreadHandlePtr Threads::CreateThread( THREAD_ENTRY_POINT entryPoint, size_t threadIdx, void *param )
    {
        ThreadHandle *threadArg( new ThreadHandle( threadIdx, param ) );
        ThreadHandlePtr retVal( new ThreadHandle( threadIdx, param ) );
        HANDLE handle = ::CreateThread( 0, 0, entryPoint, threadArg, 0, 0 );
        {
            ScopedLock lock( mOsHandleMutex );
            retVal->_setOsHandle( handle );
        }
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void Threads::WaitForThreads( size_t numThreadHandles, const ThreadHandlePtr *threadHandles )
    {
        assert( numThreadHandles < 128 );
        HANDLE hThreads[128];
        for( size_t i = 0; i < numThreadHandles; ++i )
            hThreads[i] = threadHandles[i]->_getOsHandle();

        WaitForMultipleObjects( (DWORD)numThreadHandles, hThreads, true, INFINITE );
    }
    //-----------------------------------------------------------------------------------
    void Threads::WaitForThreads( const ThreadHandleVec &threadHandles )
    {
        if( !threadHandles.empty() )
            Threads::WaitForThreads( threadHandles.size(), &threadHandles[0] );
    }
    //-----------------------------------------------------------------------------------
    void Threads::Sleep( uint32 milliseconds ) { ::Sleep( milliseconds ); }
    //-----------------------------------------------------------------------------------
    bool Threads::SetThreadName( ThreadHandle *thread, const String &name )
    {
        // The SetThreadDescription API works even if no debugger is attached.
        SetThreadDescriptionFuncDecl SetThreadDescriptionFunc =
            reinterpret_cast<SetThreadDescriptionFuncDecl>(
                ::GetProcAddress( ::GetModuleHandleW( L"Kernel32.dll" ), "SetThreadDescription" ) );

        bool bSuccess = false;
        if( SetThreadDescriptionFunc )
        {
            HANDLE threadHandle;
            if( thread )
                threadHandle = thread->_getOsHandle();
            else
                threadHandle = ::GetCurrentThread();

            bSuccess = SUCCEEDED( SetThreadDescriptionFunc( threadHandle, to_wpath( name ).c_str() ) );
        }

        return bSuccess;
    }
    //-----------------------------------------------------------------------------------
    bool Threads::CreateTls( TlsHandle *outTls )
    {
        *outTls = TlsAlloc();
        if( *outTls == TLS_OUT_OF_INDEXES )
            *outTls = OGRE_TLS_INVALID_HANDLE;

        return *outTls == OGRE_TLS_INVALID_HANDLE;
    }
    //-----------------------------------------------------------------------------------
    void Threads::DestroyTls( TlsHandle tlsHandle ) { TlsFree( tlsHandle ); }
    //-----------------------------------------------------------------------------------
    void Threads::SetTls( TlsHandle tlsHandle, void *value ) { TlsSetValue( tlsHandle, value ); }
    //-----------------------------------------------------------------------------------
    void *Threads::GetTls( TlsHandle tlsHandle ) { return TlsGetValue( tlsHandle ); }
}  // namespace Ogre
