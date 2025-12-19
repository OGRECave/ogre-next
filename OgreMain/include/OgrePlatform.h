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
#ifndef __Platform_H_
#define __Platform_H_

#include "OgreConfig.h"

/* Initial platform/compiler-related stuff to set.
 */
#define OGRE_PLATFORM_WIN32 1
#define OGRE_PLATFORM_LINUX 2
#define OGRE_PLATFORM_APPLE 3
#define OGRE_PLATFORM_APPLE_IOS 4
#define OGRE_PLATFORM_ANDROID 5
#define OGRE_PLATFORM_UNUSED 6  // It's free for whatever. Used to be NaCL
#define OGRE_PLATFORM_WINRT 7
#define OGRE_PLATFORM_EMSCRIPTEN 8
#define OGRE_PLATFORM_FREEBSD 9

#define OGRE_COMPILER_MSVC 1
#define OGRE_COMPILER_GNUC 2
#define OGRE_COMPILER_BORL 3
#define OGRE_COMPILER_WINSCW 4
#define OGRE_COMPILER_GCCE 5
#define OGRE_COMPILER_CLANG 6

#define OGRE_ENDIAN_LITTLE 1
#define OGRE_ENDIAN_BIG 2

#define OGRE_ARCHITECTURE_32 1
#define OGRE_ARCHITECTURE_64 2

#define OGRE_CPU_UNKNOWN 0
#define OGRE_CPU_X86 1
#define OGRE_CPU_PPC 2
#define OGRE_CPU_ARM 3
#define OGRE_CPU_MIPS 4

/* Find CPU type */
#if defined( __i386__ ) || defined( __x86_64__ ) || defined( _M_IX86 ) || defined( _M_X64 ) || \
    defined( _M_AMD64 ) || defined( __e2k__ )
#    define OGRE_CPU OGRE_CPU_X86
#elif defined( __ppc__ ) || defined( __PPC__ ) || defined( __ppc64__ ) || defined( __PPC64__ ) || \
    defined( _M_PPC )
#    define OGRE_CPU OGRE_CPU_PPC
#elif defined( __arm__ ) || defined( __arm64__ ) || defined( __aarch64__ ) || defined( _M_ARM ) || \
    defined( _M_ARM64 )
#    define OGRE_CPU OGRE_CPU_ARM
#elif defined( __mips__ ) || defined( __mips64 ) || defined( __mips64_ ) || defined( _M_MIPS )
#    define OGRE_CPU OGRE_CPU_MIPS
#else
#    define OGRE_CPU OGRE_CPU_UNKNOWN
#endif

/* Find the arch type */
#if defined( __x86_64__ ) || defined( _M_X64 ) || defined( _M_X64 ) || defined( _M_AMD64 ) || \
    defined( __ppc64__ ) || defined( __PPC64__ ) || defined( __arm64__ ) || defined( __aarch64__ ) || \
    defined( _M_ARM64 ) || defined( __mips64 ) || defined( __mips64_ ) || defined( __alpha__ ) || \
    defined( __ia64__ ) || defined( __e2k__ ) || defined( __s390__ ) || defined( __s390x__ ) || \
    ( defined( __riscv ) && __riscv_xlen == 64 )
#    define OGRE_ARCH_TYPE OGRE_ARCHITECTURE_64
#else
#    define OGRE_ARCH_TYPE OGRE_ARCHITECTURE_32
#endif

/* Determine CPU endian.
   We were once in situation when XCode could produce mixed endian fat binary with x86 and ppc archs
   inside, so it's safer to sniff compiler macros too
 */
#if defined( OGRE_CONFIG_BIG_ENDIAN ) || \
    ( defined( __BYTE_ORDER__ ) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ )
#    define OGRE_ENDIAN OGRE_ENDIAN_BIG
#else
#    define OGRE_ENDIAN OGRE_ENDIAN_LITTLE
#endif

/* Finds the compiler type and version.
 */
#if( defined( __WIN32__ ) || defined( _WIN32 ) ) && defined( __ANDROID__ )  // We are using NVTegra
#    define OGRE_COMPILER OGRE_COMPILER_GNUC
#    define OGRE_COMP_VER 470
#elif defined( __GCCE__ )
#    define OGRE_COMPILER OGRE_COMPILER_GCCE
#    define OGRE_COMP_VER _MSC_VER
// # include <staticlibinit_gcce.h> // This is a GCCE toolchain workaround needed when compiling with
// GCCE
#elif defined( __WINSCW__ )
#    define OGRE_COMPILER OGRE_COMPILER_WINSCW
#    define OGRE_COMP_VER _MSC_VER
#elif defined( _MSC_VER )
#    define OGRE_COMPILER OGRE_COMPILER_MSVC
#    define OGRE_COMP_VER _MSC_VER
#elif defined( __clang__ )
#    define OGRE_COMPILER OGRE_COMPILER_CLANG
#    define OGRE_COMP_VER \
        ( ( ( __clang_major__ ) * 100 ) + ( __clang_minor__ * 10 ) + __clang_patchlevel__ )
#elif defined( __GNUC__ )
#    define OGRE_COMPILER OGRE_COMPILER_GNUC
#    define OGRE_COMP_VER ( ( ( __GNUC__ ) * 100 ) + ( __GNUC_MINOR__ * 10 ) + __GNUC_PATCHLEVEL__ )
#elif defined( __BORLANDC__ )
#    define OGRE_COMPILER OGRE_COMPILER_BORL
#    define OGRE_COMP_VER __BCPLUSPLUS__
#    define __FUNCTION__ __FUNC__
#else
#    pragma error "No known compiler. Abort! Abort!"

#endif

#if OGRE_COMPILER == OGRE_COMPILER_GNUC || OGRE_COMPILER == OGRE_COMPILER_CLANG
#    define OGRE_GCC_VISIBILITY
#endif

/* See if we can use __forceinline or if we need to use __inline instead */
#if OGRE_COMPILER == OGRE_COMPILER_MSVC
#    if OGRE_COMP_VER >= 1200
#        define FORCEINLINE __forceinline
#    endif
#elif defined( __MINGW32__ )
#    if !defined( FORCEINLINE )
#        define FORCEINLINE __inline
#    endif
#elif ( OGRE_COMPILER == OGRE_COMPILER_GNUC || OGRE_COMPILER == OGRE_COMPILER_CLANG )
#    define FORCEINLINE inline __attribute__( ( always_inline ) )
#else
#    define FORCEINLINE __inline
#endif

/* define OGRE_NORETURN macro */
#if OGRE_COMPILER == OGRE_COMPILER_MSVC
#    define OGRE_NORETURN __declspec( noreturn )
#elif OGRE_COMPILER == OGRE_COMPILER_GNUC || OGRE_COMPILER == OGRE_COMPILER_CLANG
#    define OGRE_NORETURN __attribute__( ( noreturn ) )
#else
#    define OGRE_NORETURN
#endif

/* Finds the current platform */
#if( defined( __WIN32__ ) || defined( _WIN32 ) ) && !defined( __ANDROID__ )
#    include <sdkddkver.h>
#    if defined( WINAPI_FAMILY )
#        include <winapifamily.h>
#        if WINAPI_FAMILY == WINAPI_FAMILY_APP || WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
#            define OGRE_PLATFORM OGRE_PLATFORM_WINRT
#        else
#            define OGRE_PLATFORM OGRE_PLATFORM_WIN32
#        endif
#    else
#        define OGRE_PLATFORM OGRE_PLATFORM_WIN32
#    endif
#    define __OGRE_WINRT_STORE \
        ( OGRE_PLATFORM == OGRE_PLATFORM_WINRT && \
          WINAPI_FAMILY == WINAPI_FAMILY_APP )  // WindowsStore 8.0 and 8.1
#    define __OGRE_WINRT_PHONE \
        ( OGRE_PLATFORM == OGRE_PLATFORM_WINRT && \
          WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP )  // WindowsPhone 8.0 and 8.1
#    define __OGRE_WINRT_PHONE_80 \
        ( OGRE_PLATFORM == OGRE_PLATFORM_WINRT && WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP && \
          _WIN32_WINNT <= _WIN32_WINNT_WIN8 )  // Windows Phone 8.0 often need special handling,
                                               // while 8.1 is OK
#    if defined( _WIN32_WINNT_WIN8 ) && \
        _WIN32_WINNT >= _WIN32_WINNT_WIN8  // i.e. this is modern SDK and we compile for OS with
                                           // guaranteed support for DirectXMath
#        define __OGRE_HAVE_DIRECTXMATH 1
#    endif
#    ifndef _CRT_SECURE_NO_WARNINGS
#        define _CRT_SECURE_NO_WARNINGS
#    endif
#    ifndef _SCL_SECURE_NO_WARNINGS
#        define _SCL_SECURE_NO_WARNINGS
#    endif
#elif defined( __EMSCRIPTEN__ )
#    define OGRE_PLATFORM OGRE_PLATFORM_EMSCRIPTEN
#elif defined( __APPLE_CC__ )
// Device                                                     Simulator
// Both requiring OS version 6.0 or greater
#    if __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 60000 || \
        __IPHONE_OS_VERSION_MIN_REQUIRED >= 60000 || \
        ( defined( __has_builtin ) && __has_builtin( __is_target_os ) && __is_target_os( xros ) )
#        define OGRE_PLATFORM OGRE_PLATFORM_APPLE_IOS
#    else
#        define OGRE_PLATFORM OGRE_PLATFORM_APPLE
#    endif
#elif defined( __ANDROID__ )
#    define OGRE_PLATFORM OGRE_PLATFORM_ANDROID
#elif defined( __FreeBSD__ )
#    define OGRE_PLATFORM OGRE_PLATFORM_FREEBSD
#else
#    define OGRE_PLATFORM OGRE_PLATFORM_LINUX
#endif

// For generating compiler warnings - should work on any compiler
// As a side note, if you start your message with 'Warning: ', the MSVC
// IDE actually does catch a warning :)
#define OGRE_QUOTE_INPLACE( x ) #x
#define OGRE_QUOTE( x ) OGRE_QUOTE_INPLACE( x )
#define OGRE_WARN( x ) message( __FILE__ "(" QUOTE( __LINE__ ) ") : " x "\n" )

// portable analog of __PRETTY_FUNCTION__, see also BOOST_CURRENT_FUNCTION
#if OGRE_COMPILER == OGRE_COMPILER_GNUC || OGRE_COMPILER == OGRE_COMPILER_CLANG
#    define OGRE_CURRENT_FUNCTION __PRETTY_FUNCTION__
#elif OGRE_COMPILER == OGRE_COMPILER_MSVC || defined( __FUNCSIG__ )
#    define OGRE_CURRENT_FUNCTION __FUNCSIG__
#else  // C++11
#    define OGRE_CURRENT_FUNCTION __func__
#endif

// For marking functions as deprecated
#if __cplusplus >= 201402L || OGRE_COMPILER == OGRE_COMPILER_MSVC && OGRE_COMP_VER >= 1900
#    define OGRE_DEPRECATED [[deprecated]]
#    define OGRE_DEPRECATED_VER( x ) [[deprecated]]
#    define OGRE_DEPRECATED_ENUM_VER( x ) [[deprecated]]
#else
#    if OGRE_COMPILER == OGRE_COMPILER_MSVC
#        define OGRE_DEPRECATED __declspec( deprecated )
#        define OGRE_DEPRECATED_VER( x ) __declspec( deprecated )
#        define OGRE_DEPRECATED_ENUM_VER( x )
#    elif OGRE_COMPILER == OGRE_COMPILER_GNUC || OGRE_COMPILER == OGRE_COMPILER_CLANG
#        define OGRE_DEPRECATED __attribute__( ( deprecated ) )
#        define OGRE_DEPRECATED_VER( x ) __attribute__( ( deprecated ) )
#        define OGRE_DEPRECATED_ENUM_VER( x ) __attribute__( ( deprecated ) )
#    else
#        pragma message( "WARNING: You need to implement OGRE_DEPRECATED for this compiler" )
#        define OGRE_DEPRECATED
#        define OGRE_DEPRECATED_VER( x )
#        define OGRE_DEPRECATED_ENUM_VER( x )
#    endif
#endif
// Disable OGRE_WCHAR_T_STRINGS until we figure out what to do about it.
#define OGRE_WCHAR_T_STRINGS 0

//----------------------------------------------------------------------------
// Windows Settings
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT

// If we're not including this from a client build, specify that the stuff
// should get exported. Otherwise, import it.
#    if defined( OGRE_STATIC_LIB )
// Linux compilers don't have symbol import/export directives.
#        define _OgreExport
#        define _OgrePrivate
#    else
#        if defined( OGRE_NONCLIENT_BUILD )
#            define _OgreExport __declspec( dllexport )
#        else
#            if defined( __MINGW32__ )
#                define _OgreExport
#            else
#                define _OgreExport __declspec( dllimport )
#            endif
#        endif
#        define _OgrePrivate
#    endif

// Disable unicode support on MingW for GCC 3, poorly supported in stdlibc++
// STLPORT fixes this though so allow if found
// MinGW C++ Toolkit supports unicode and sets the define __MINGW32_TOOLBOX_UNICODE__ in _mingw.h
// GCC 4 is also fine
#    if defined( __MINGW32__ )
#        if OGRE_COMP_VER < 400
#            if !defined( _STLPORT_VERSION )
#                include <_mingw.h>
#                if defined( __MINGW32_TOOLBOX_UNICODE__ ) || OGRE_COMP_VER > 345
#                    define OGRE_UNICODE_SUPPORT 1
#                else
#                    define OGRE_UNICODE_SUPPORT 0
#                endif
#            else
#                define OGRE_UNICODE_SUPPORT 1
#            endif
#        else
#            define OGRE_UNICODE_SUPPORT 1
#        endif
#    else
#        define OGRE_UNICODE_SUPPORT 1
#    endif

#endif  // OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT

//----------------------------------------------------------------------------
// Linux/Apple/iOS/Android/Emscripten/FreeBSD Settings
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX || OGRE_PLATFORM == OGRE_PLATFORM_APPLE || \
    OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS || OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || \
    OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN || OGRE_PLATFORM == OGRE_PLATFORM_FREEBSD

// Enable GCC symbol visibility
#    if defined( OGRE_GCC_VISIBILITY )
#        define _OgrePrivate __attribute__( ( visibility( "hidden" ) ) )
#        if !defined( OGRE_STATIC_LIB )
#            define _OgreExport __attribute__( ( visibility( "default" ) ) )
#        else
#            define _OgreExport __attribute__( ( visibility( "hidden" ) ) )
#        endif
#    else
#        define _OgreExport
#        define _OgrePrivate
#    endif

// A quick define to overcome different names for the same function
#    define stricmp strcasecmp

#    if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#        define OGRE_PLATFORM_LIB "OgrePlatform.bundle"
#    elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
#        define OGRE_PLATFORM_LIB "OgrePlatform.a"
#    else  // OGRE_PLATFORM_LINUX || OGRE_PLATFORM_FREEBSD
#        define OGRE_PLATFORM_LIB "libOgrePlatform.so"
#    endif

#    if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS || \
        OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || defined _LIBCPP_VERSION && _LIBCPP_VERSION >= 190000
// XCode 16.3 fails to compile our UTFString due undefined std::char_traits<ushort>
// Same problem with Android NDK 28, actually problem caused by changes in Clang libc++ 19
// TODO: migrate from Ogre::UTFString to C++11 std::u16string
#        define OGRE_UNICODE_SUPPORT 0
#    else
// Always enable unicode support for the moment
// Perhaps disable in old versions of gcc if necessary
#        define OGRE_UNICODE_SUPPORT 1
#    endif

#endif

//----------------------------------------------------------------------------
// Debug mode and asserts

// No checks done at all
#define OGRE_DEBUG_NONE 0
// We perform basic assert checks and other non-performance intensive checks
// This setting must NOT break the ABI. i.e. a binary compiled with OGRE_DEBUG_LOW
// must be ABI compatible with a binary compiled without it.
#define OGRE_DEBUG_LOW 1
// We alter classes to add some debug variables for a bit more thorough checking
// and also perform checks that may cause some performance hit
// This setting or higher is allowed to break the ABI.
#define OGRE_DEBUG_MEDIUM 2
// We perform intensive validation without concerns for performance
#define OGRE_DEBUG_HIGH 3

// We cannot tell whether something is a debug build or not simply by checking NDEBUG because
// it's perfectly valid for a user to #undef NDEBUG to get assertions in a release build.
// DEBUG is set automatically on MSVC and _DEBUG is set automatically on MINGW.
// Some environments don't provide any debug flags besides NDEBUG by default, so we issue a
// warning here.
#ifndef OGRE_DEBUG_MODE
#    if !defined( NDEBUG ) && !defined( _DEBUG ) && !defined( DEBUG ) && \
        !defined( OGRE_IGNORE_UNKNOWN_DEBUG )
#        pragma message( \
            "Ogre can't tell whether this is a debug build. If it is, please add _DEBUG to the preprocessor " \
            "definitions. Otherwise, you can set OGRE_IGNORE_UNKNOWN_DEBUG to suppress this warning. Ogre will " \
            "assume this is not a debug build by default. To add _DEBUG with g++, invoke g++ with the argument " \
            "-D_DEBUG. To add it in CMake, include " \
            "set( CMAKE_CXX_FLAGS_DEBUG \"${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -D_DEBUG=1\" ) at the top of your " \
            "CMakeLists.txt file. IDEs usually provide the possibility to add preprocessor definitions in the build " \
            "settings. You can also manually set OGRE_DEBUG_MODE to either 1 or 0 instead of adding _DEBUG." )
#    endif
#    if defined( NDEBUG ) && defined( _DEBUG ) && !defined( OGRE_IGNORE_DEBUG_FLAG_CONTRADICTION )
#        pragma message( \
            "You have both NDEBUG and _DEBUG defined. Ogre will assume you're running a debug build. To suppress this " \
            "warning, set OGRE_IGNORE_DEBUG_FLAG_CONTRADICTION." )
#    endif

#    if defined( _DEBUG ) || defined( DEBUG )
#        define OGRE_DEBUG_MODE OGRE_DEBUG_LEVEL_DEBUG
#    else
#        define OGRE_DEBUG_MODE OGRE_DEBUG_LEVEL_RELEASE
#    endif
#endif

#define OGRE_ASSERTS_ENABLED

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_LOW
#    define OGRE_ASSERT_LOW OGRE_ASSERT
#else
#    define OGRE_ASSERT_LOW( condition ) ( (void)0 )
#endif

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
#    define OGRE_ASSERT_MEDIUM OGRE_ASSERT
#else
#    define OGRE_ASSERT_MEDIUM( condition ) ( (void)0 )
#endif

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
#    define OGRE_ASSERT_HIGH OGRE_ASSERT
#else
#    define OGRE_ASSERT_HIGH( condition ) ( (void)0 )
#endif

//----------------------------------------------------------------------------
// Android Settings
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
#    ifndef CLOCKS_PER_SEC
#        define CLOCKS_PER_SEC 1000
#    endif
#endif

#ifndef __OGRE_HAVE_DIRECTXMATH
#    define __OGRE_HAVE_DIRECTXMATH 0
#endif

//----------------------------------------------------------------------------
// Set the default locale for strings
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
//  Locales are not supported by the C lib you have to go through JNI.
#    define OGRE_DEFAULT_LOCALE ""
#else
#    define OGRE_DEFAULT_LOCALE "C"
#endif

//----------------------------------------------------------------------------
// Library suffixes
// "_d" for debug builds, nothing otherwise
#if OGRE_DEBUG_MODE
#    define OGRE_BUILD_SUFFIX "_d"
#else
#    define OGRE_BUILD_SUFFIX ""
#endif

#ifndef OGRE_FLEXIBILITY_LEVEL
#    define OGRE_FLEXIBILITY_LEVEL 0
#endif

#if OGRE_FLEXIBILITY_LEVEL >= 0
#    define virtual_l0 virtual
#else
#    define virtual_l0
#endif
#if OGRE_FLEXIBILITY_LEVEL > 1
#    define virtual_l1 virtual
#else
#    define virtual_l1
#endif
#if OGRE_FLEXIBILITY_LEVEL > 2
#    define virtual_l2 virtual
#else
#    define virtual_l2
#endif

#if OGRE_COMPILER == OGRE_COMPILER_MSVC
#    define DECL_MALLOC __declspec( restrict ) __declspec( noalias )
#else
#    define DECL_MALLOC __attribute__( ( malloc ) )
#endif

// Stack-alignment hackery.
//
// If macro __OGRE_SIMD_ALIGN_STACK defined, means there requests
// special code to ensure stack align to a 16-bytes boundary.
//
// Note:
//   This macro can only guarantee callee stack pointer (esp) align
// to a 16-bytes boundary, but not that for frame pointer (ebp).
// Because most compiler might use frame pointer to access to stack
// variables, so you need to wrap those alignment required functions
// with extra function call.
//
#if defined( __INTEL_COMPILER )
// For intel's compiler, simply calling alloca seems to do the right
// thing. The size of the allocated block seems to be irrelevant.
#    define _OGRE_SIMD_ALIGN_STACK() _alloca( 16 )
#    define _OGRE_SIMD_ALIGN_ATTRIBUTE

#elif OGRE_CPU == OGRE_CPU_X86 && \
    ( OGRE_COMPILER == OGRE_COMPILER_GNUC || OGRE_COMPILER == OGRE_COMPILER_CLANG ) && \
    ( OGRE_ARCH_TYPE != OGRE_ARCHITECTURE_64 )
// mark functions with GCC attribute to force stack alignment to 16 bytes
#    define _OGRE_SIMD_ALIGN_ATTRIBUTE __attribute__( ( force_align_arg_pointer ) )

#elif defined( _MSC_VER )
// Fortunately, MSVC will align the stack automatically
#    define _OGRE_SIMD_ALIGN_ATTRIBUTE

#else
#    define _OGRE_SIMD_ALIGN_ATTRIBUTE

#endif

// Find how to declare aligned variable.
#if OGRE_COMPILER == OGRE_COMPILER_MSVC
#    define OGRE_ALIGNED_DECL( type, var, alignment ) __declspec( align( alignment ) ) type var
#elif( OGRE_COMPILER == OGRE_COMPILER_GNUC ) || ( OGRE_COMPILER == OGRE_COMPILER_CLANG )
#    define OGRE_ALIGNED_DECL( type, var, alignment ) \
        type var __attribute__( ( __aligned__( alignment ) ) )
#else
#    define OGRE_ALIGNED_DECL( type, var, alignment ) type var
#endif

// Find perfect alignment (should supports SIMD alignment if SIMD available)
#if OGRE_CPU == OGRE_CPU_X86
#    define OGRE_SIMD_ALIGNMENT 16
#else
#    define OGRE_SIMD_ALIGNMENT 16
#endif

// Declare variable aligned to SIMD alignment.
#define OGRE_SIMD_ALIGNED_DECL( type, var ) OGRE_ALIGNED_DECL( type, var, OGRE_SIMD_ALIGNMENT )

#if OGRE_USE_SIMD == 1
// Define whether or not Ogre compiled with SSE support.
#    if OGRE_DOUBLE_PRECISION == 0 && OGRE_CPU == OGRE_CPU_X86
#        define __OGRE_HAVE_SSE 1
#    endif

// Define whether or not Ogre compiled with NEON support.
#    if OGRE_DOUBLE_PRECISION == 0 && OGRE_CPU == OGRE_CPU_ARM && \
        ( defined( __aarch64__ ) || defined( __arm64__ ) || defined( _M_ARM64 ) || \
          defined( __ARM_NEON__ ) || \
          defined( _WIN32_WINNT_WIN8 ) && _WIN32_WINNT >= _WIN32_WINNT_WIN8 )
#        define __OGRE_HAVE_NEON 1
#    endif
#endif

#ifndef __OGRE_HAVE_SSE
#    define __OGRE_HAVE_SSE 0
#endif

#if OGRE_USE_SIMD == 0 || !defined( __OGRE_HAVE_NEON )
#    define __OGRE_HAVE_NEON 0
#endif

#if !defined( __OGRE_HAVE_DIRECTXMATH )
#    define __OGRE_HAVE_DIRECTXMATH 0
#endif

// Integer formats of fixed bit width
#if OGRE_COMPILER == OGRE_COMPILER_MSVC && OGRE_COMP_VER < 1600  // no <stdint.h>
namespace Ogre
{
    typedef unsigned char    uint8;
    typedef unsigned short   uint16;
    typedef unsigned int     uint32;
    typedef unsigned __int64 uint64;
    typedef signed char      int8;
    typedef short            int16;
    typedef int              int32;
    typedef __int64          int64;
}  // namespace Ogre
#else
#    include <stdint.h>
namespace Ogre
{
    typedef ::uint8_t  uint8;
    typedef ::uint16_t uint16;
    typedef ::uint32_t uint32;
    typedef ::uint64_t uint64;
    typedef ::int8_t   int8;
    typedef ::int16_t  int16;
    typedef ::int32_t  int32;
    typedef ::int64_t  int64;
}  // namespace Ogre
#endif

#ifndef OGRE_RESTRICT_ALIASING
#    define OGRE_RESTRICT_ALIASING 0
#endif

#if OGRE_RESTRICT_ALIASING != 0
#    if OGRE_COMPILER == OGRE_COMPILER_MSVC
#        define RESTRICT_ALIAS __restrict  // MSVC
#        define RESTRICT_ALIAS_RETURN __restrict
#    else
#        define RESTRICT_ALIAS __restrict__  // GCC... and others?
#        define RESTRICT_ALIAS_RETURN
#    endif
#else
#    define RESTRICT_ALIAS
#    define RESTRICT_ALIAS_RETURN
#endif

// Disable these warnings (too much noise)
#if OGRE_COMPILER == OGRE_COMPILER_MSVC
#    ifndef _CRT_SECURE_NO_WARNINGS
#        define _CRT_SECURE_NO_WARNINGS
#    endif
#    ifndef _SCL_SECURE_NO_WARNINGS
#        define _SCL_SECURE_NO_WARNINGS
#    endif
// Turn off warnings generated by long std templates
// This warns about truncation to 255 characters in debug/browse info
#    pragma warning( disable : 4786 )
// Turn off warnings generated by long std templates
// This warns about truncation to 255 characters in debug/browse info
#    pragma warning( disable : 4503 )
// disable: "<type> needs to have dll-interface to be used by clients'
// Happens on STL member variables which are not public therefore is ok
#    pragma warning( disable : 4251 )
// disable: "non dll-interface class used as base for dll-interface class"
// Happens when deriving from Singleton because bug in compiler ignores
// template export
#    pragma warning( disable : 4275 )
// disable: "C++ Exception Specification ignored"
// This is because MSVC 6 did not implement all the C++ exception
// specifications in the ANSI C++ draft.
#    pragma warning( disable : 4290 )
// disable: "no suitable definition provided for explicit template
// instantiation request" Occurs in VC7 for no justifiable reason on all
// #includes of Singleton
#    pragma warning( disable : 4661 )
// disable: deprecation warnings when using CRT calls in VC8
// These show up on all C runtime lib code in VC8, disable since they clutter
// the warnings with things we may not be able to do anything about (e.g.
// generated code from nvparse etc). I doubt very much that these calls
// will ever be actually removed from VC anyway, it would break too much code.
#    pragma warning( disable : 4996 )
// disable: "conditional expression constant", always occurs on
// OGRE_MUTEX_CONDITIONAL when no threading enabled
#    pragma warning( disable : 201 )
// disable: "unreferenced formal parameter"
// Many versions of VC have bugs which generate this error in cases where they shouldn't
#    pragma warning( disable : 4100 )
// disable: "behavior change: an object of POD type constructed with an initializer of the form () will
// be default-initialized" We have this issue in OgreMemorySTLAlloc.h - so we see it over and over
#    pragma warning( disable : 4345 )
#endif

#endif
