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
#ifndef __OGREOSVERSIONHELPERS_H__
#define __OGREOSVERSIONHELPERS_H__
#include "OgrePrerequisites.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT && !defined( __cplusplus_winrt )
#    include <winrt/Windows.Foundation.Metadata.h>
#endif

namespace Ogre
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    inline bool IsWindowsVersionOrGreater( WORD wMajorVersion, WORD wMinorVersion,
                                           WORD wServicePackMajor, WORD wBuildNumber = 0 )
    {
        OSVERSIONINFOEXW osvi = { sizeof( osvi ), 0, 0, 0, 0, { 0 }, 0, 0 };
        DWORDLONG const  dwlConditionMask =
            VerSetConditionMask( VerSetConditionMask(          //
                                     VerSetConditionMask(      //
                                         VerSetConditionMask(  //
                                             0, VER_MAJORVERSION, VER_GREATER_EQUAL ),
                                         VER_MINORVERSION, VER_GREATER_EQUAL ),
                                     VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL ),
                                 VER_BUILDNUMBER, VER_GREATER_EQUAL );

        osvi.dwMajorVersion = wMajorVersion;
        osvi.dwMinorVersion = wMinorVersion;
        osvi.dwBuildNumber = wBuildNumber;
        osvi.wServicePackMajor = wServicePackMajor;

        // Note that this API will lie without OS compatibility manifest embedded into .exe
        // https://docs.microsoft.com/en-us/windows/win32/sysinfo/targeting-your-application-at-windows-8-1
        return VerifyVersionInfoW(
                   &osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_BUILDNUMBER,
                   dwlConditionMask ) != FALSE;
    }
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT
    inline bool IsApiContractPresent( unsigned short version )
    {
#    if _WIN32_WINNT < 0x0A00  // _WIN32_WINNT_WIN10
        return false;
#    elif defined( __cplusplus_winrt )
        return Windows::Foundation::Metadata::ApiInformation::IsApiContractPresent(
            "Windows.Foundation.UniversalApiContract", version );
#    else
        return winrt::Windows::Foundation::Metadata::ApiInformation::IsApiContractPresent(
            L"Windows.Foundation.UniversalApiContract", version );
#    endif
    }
#endif

    inline bool IsWindows8OrGreater()
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        return IsWindowsVersionOrGreater( 6, 2, 0 );
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT
        return true;
#endif
    }

    inline bool IsWindows8Point1OrGreater()
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        return IsWindowsVersionOrGreater( 6, 3, 0 );
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT
        return _WIN32_WINNT >= 0x0603;  // _WIN32_WINNT_WINBLUE
#endif
    }

    inline bool IsWindows10OrGreater()
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        return IsWindowsVersionOrGreater( 10, 0, 0 );
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT
        return IsApiContractPresent( 1 );
#endif
    }

    inline bool IsWindows10RS3OrGreater()
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        return IsWindowsVersionOrGreater( 10, 0, 0, 16299 );
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT
        return IsApiContractPresent( 5 );
#endif
    }

}  // namespace Ogre
#endif  // __OGREOSVERSIONHELPERS_H__
