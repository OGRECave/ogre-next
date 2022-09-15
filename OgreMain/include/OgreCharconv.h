/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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
#ifndef _OgreCharconv_H_
#define _OgreCharconv_H_

#if __cplusplus >= 201703L || defined (_MSVC_LANG) && _MSVC_LANG >= 201703L // C++17, we have __has_include
#if __has_include(<charconv>)
#include <charconv>

#if __cpp_lib_to_chars >= 201611L // defined for MSVC, but not yet for Clang and GCC due to the missing floating point support
#define OGRE_HAS_CHARCONV
#define OGRE_HAS_CHARCONV_FLOAT

// Clang STL integer only implementation, not available on old Apple platforms
#elif defined(_LIBCPP_STD_VER) && _LIBCPP_STD_VER > 14 && \
    (!defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__) || __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 101500) && \
    (!defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) || __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 130000) && \
    (!defined(__ENVIRONMENT_TV_OS_VERSION_MIN_REQUIRED__) || __ENVIRONMENT_TV_OS_VERSION_MIN_REQUIRED__ >= 130000) && \
    (!defined(__ENVIRONMENT_WATCH_OS_VERSION_MIN_REQUIRED__) || __ENVIRONMENT_WATCH_OS_VERSION_MIN_REQUIRED__ >= 60000)
#define OGRE_HAS_CHARCONV

// GCC STL implementation, integer only since GCC 8.1, full since GCC 11.1, but __cpp_lib_to_chars still not yet defined
#elif defined(_GLIBCXX_RELEASE) && _GLIBCXX_HOSTED && \
    (_GLIBCXX_RELEASE > 8 || _GLIBCXX_RELEASE == __GNUC__ && (__GNUC__ * 100 + __GNUC_MINOR__) >= 801)
#define OGRE_HAS_CHARCONV
#if ((_GLIBCXX_RELEASE > 11 || _GLIBCXX_RELEASE == __GNUC__ && (__GNUC__ * 100 + __GNUC_MINOR__) >= 1101) && !__MINGW32__ && !__MINGW64__)
#define OGRE_HAS_CHARCONV_FLOAT
#endif
#endif

#endif // __has_include(<charconv>)
#endif // C++17


#ifdef OGRE_HAS_CHARCONV

namespace Ogre
{
	using std::from_chars_result;
	using std::from_chars;
	using std::to_chars_result;
	using std::to_chars;
	using std::chars_format;
	using std::errc;
}

// some utilities
namespace Ogre
{
	namespace
	{
		constexpr bool is_space(char c) noexcept { return c == ' ' || ((unsigned)(c - '\t') <= (unsigned)('\r' - '\t')); } // 0x20' ', 0x9'\t', 0xA'\n', 0xB'\v', 0xC'\f', 0xD'\r'
		constexpr void skip_space(const char*& ptr, const char* last, char SEP) { while (ptr < last && *ptr != SEP && is_space(*ptr)) ++ptr; }
		constexpr bool skip_sep(const char*& ptr, const char* last, char SEP) { skip_space(ptr, last, SEP); return ptr < last && *ptr == SEP ? ++ptr, true : false; }
	}

    /** Scans SEP separated arguments list from the buffer, skipping spaces. */
    template <char SEP = 0, typename... Args>
    from_chars_result from_chars_all( const char *first, const char *last, Args &...args )
    {
        from_chars_result res = { first, errc() };
        return ( ... && ( ( SEP == 0 || res.ptr == first || skip_sep( res.ptr, last, SEP ) ) &&
                          ( skip_space( res.ptr, last, SEP ), res = from_chars( res.ptr, last, args ),
                            res.ec == errc() ) ) )
               ? skip_space( res.ptr, last, SEP ),
               res : from_chars_result{ first, errc::invalid_argument };
    }

    /** Prints SEP separated arguments list into the buffer, with smallest possible precision necessary
     * to restore values exactly. Buffer is NOT zero-terminated. */
    template <char SEP = 0, typename... Args>
    to_chars_result to_chars_all( char *first, char *last, Args... args )
    {
        to_chars_result res = { first, errc() };
        return ( ... &&
                 ( ( res.ptr == first || ( res.ptr < last && ( *res.ptr++ = ( SEP ? SEP : ' ' ) ) ) ) &&
                   ( res = to_chars( res.ptr, last, args ), res.ec == errc() ) ) )
                   ? res
                   : to_chars_result{ last, errc::value_too_large };
    }
}  // namespace Ogre

#endif

#endif
