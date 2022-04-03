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

#include "OgreAtomicScalar.h"

#ifdef NEED_TO_INIT_INTERLOCKEDCOMPAREEXCHANGE64WRAPPER

namespace Ogre
{
    InterlockedCompareExchange64Wrapper::func_InterlockedCompareExchange64
        InterlockedCompareExchange64Wrapper::Ogre_InterlockedCompareExchange64;

    InterlockedCompareExchange64Wrapper::InterlockedCompareExchange64Wrapper()
    {
        Ogre_InterlockedCompareExchange64 = NULL;

// In win32 we will get InterlockedCompareExchange64 function address, in XP we will get NULL - as the
// function doesn't exist there.
#    if( OGRE_PLATFORM == OGRE_PLATFORM_WIN32 )
        HINSTANCE kernel32Dll = LoadLibrary( "KERNEL32.DLL" );
        Ogre_InterlockedCompareExchange64 = (func_InterlockedCompareExchange64)GetProcAddress(
            kernel32Dll, "InterlockedCompareExchange64" );
#    endif

// In WinRT we can't LoadLibrary("KERNEL32.DLL") - but on the other hand we don't have the issue - as
// InterlockedCompareExchange64 exist on all WinRT platforms (if not - add to the #if below)
#    if( OGRE_PLATFORM == OGRE_PLATFORM_WINRT )
        Ogre_InterlockedCompareExchange64 = _InterlockedCompareExchange64;
#    endif
    }

    // Here we call the InterlockedCompareExchange64Wrapper constructor so
    // Ogre_InterlockedCompareExchange64 will be init once when the program loads.
    InterlockedCompareExchange64Wrapper interlockedCompareExchange64Wrapper;

}  // namespace Ogre

#endif
