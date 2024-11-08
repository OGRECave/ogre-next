/*
 * Copyright (c) 2008, Power of Two Games LLC
 * Copyright (c) 2018, Matias N. Goldberg (small enhancements, ported to other compilers)
 * All rights reserved.
 *
 * THIS FILE WAS AUTOMATICALLY GENERATED USING AssertLib
 *
 * See https://bitbucket.org/dark_sylinc/AssertLib
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Power of Two Games LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY POWER OF TWO GAMES LLC ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL POWER OF TWO GAMES LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _OgreAssert_H_
#define _OgreAssert_H_

#include "OgrePrerequisites.h"

namespace Ogre
{
    namespace Assert
    {
        enum FailBehavior
        {
            Halt,
            Continue,
        };

        typedef FailBehavior ( *Handler )( const char *condition, const char *msg, const char *file,
                                           int line );

        _OgreExport Handler GetHandler();
        _OgreExport void    SetHandler( Handler newHandler );

        _OgreExport FailBehavior ReportFailure( const char *condition, const char *file, int line,
                                                const char *msg, ... );
    }  // namespace Assert
}  // namespace Ogre

// We want to halt using macro + builtin rather than inline function, to generate debug information
// with assertion line number rather than halting machinery line number. Inspired by
// https://github.com/nemequ/portable-snippets/blob/master/debug-trap/debug-trap.h
// https://github.com/scottt/debugbreak/blob/master/debugbreak.h
#if defined( __has_builtin )
#    if __has_builtin( __builtin_debugtrap )
#        define OGRE_HALT() __builtin_debugtrap()
#    elif __has_builtin( __debugbreak )
#        define OGRE_HALT() __debugbreak()
#    endif
#endif
#if !defined( OGRE_HALT )
#    if defined( _MSC_VER ) || defined( __INTEL_COMPILER )
#        define OGRE_HALT() __debugbreak()
#    elif defined( __DMC__ ) && defined( _M_IX86 )
#        define OGRE_HALT() __asm int 3h
#    elif defined( __i386__ ) || defined( __x86_64__ )
#        define OGRE_HALT() __asm__ __volatile__( "int3" )
#    elif defined( __thumb__ )
#        define OGRE_HALT() __asm__ __volatile__( ".inst 0xde01" )
#    elif defined( __aarch64__ )
#        define OGRE_HALT() __asm__ __volatile__( ".inst 0xd4200000" )
#    elif defined( __arm__ )
#        define OGRE_HALT() __asm__ __volatile__( ".inst 0xe7f001f0" )
#    elif defined( __alpha__ ) && !defined( __osf__ )
#        define OGRE_HALT() __asm__ __volatile__( "bpt" )
#    elif defined( __powerpc__ )
#        define OGRE_HALT() __asm__ __volatile__( ".4byte 0x7d821008" )
#    elif defined( __riscv )
#        define OGRE_HALT() __asm__ __volatile__( ".4byte 0x00100073" )
#    else
#        include <signal.h>
#        if defined( SIGTRAP )
#            define OGRE_HALT() raise( SIGTRAP )
#        else
#            define OGRE_HALT() raise( SIGABRT )
#        endif
#    endif
#endif

#define OGRE_UNUSED( x ) \
    do \
    { \
        (void)sizeof( x ); \
    } while( 0 )

#ifdef OGRE_ASSERTS_ENABLED
#    define OGRE_ASSERT( cond ) \
        do \
        { \
            if( !( cond ) ) \
            { \
                if( Ogre::Assert::ReportFailure( #cond, __FILE__, __LINE__, 0 ) == Ogre::Assert::Halt ) \
                    OGRE_HALT(); \
            } \
        } while( 0 )

#    if _MSC_VER
#        define OGRE_ASSERT_MSG( cond, msg, ... ) \
            do \
            { \
                if( !( cond ) ) \
                { \
                    if( Ogre::Assert::ReportFailure( #cond, __FILE__, __LINE__, ( msg ), \
                                                     __VA_ARGS__ ) == Ogre::Assert::Halt ) \
                        OGRE_HALT(); \
                } \
            } while( 0 )

#        define OGRE_ASSERT_FAIL( msg, ... ) \
            do \
            { \
                if( Ogre::Assert::ReportFailure( 0, __FILE__, __LINE__, ( msg ), __VA_ARGS__ ) == \
                    Ogre::Assert::Halt ) \
                    OGRE_HALT(); \
            } while( 0 )
#        define OGRE_VERIFY_MSG( cond, msg, ... ) OGRE_ASSERT_MSG( cond, msg, __VA_ARGS__ )
#    else
#        define OGRE_ASSERT_MSG( cond, msg, ... ) \
            do \
            { \
                if( !( cond ) ) \
                { \
                    if( Ogre::Assert::ReportFailure( #cond, __FILE__, __LINE__, ( msg ), \
                                                     ##__VA_ARGS__ ) == Ogre::Assert::Halt ) \
                        OGRE_HALT(); \
                } \
            } while( 0 )

#        define OGRE_ASSERT_FAIL( msg, ... ) \
            do \
            { \
                if( Ogre::Assert::ReportFailure( 0, __FILE__, __LINE__, ( msg ), ##__VA_ARGS__ ) == \
                    Ogre::Assert::Halt ) \
                    OGRE_HALT(); \
            } while( 0 )
#        define OGRE_VERIFY_MSG( cond, msg, ... ) OGRE_ASSERT_MSG( cond, msg, ##__VA_ARGS__ )
#    endif

#    define OGRE_VERIFY( cond ) OGRE_ASSERT( cond )
#else
#    define OGRE_ASSERT( condition ) ( (void)0 )
#    define OGRE_ASSERT_MSG( condition, msg, ... ) ( (void)0 )
#    define OGRE_ASSERT_FAIL( msg, ... ) ( (void)0 )
#    define OGRE_VERIFY( cond ) ( (void)0 )
#    define OGRE_VERIFY_MSG( cond, msg, ... ) ( (void)0 )
#endif

#if __cplusplus >= 201703L || defined( _MSVC_LANG ) && _MSVC_LANG >= 201703L
#    define OGRE_STATIC_ASSERT( x ) static_assert( x )
#else  // C++11
#    define OGRE_STATIC_ASSERT( x ) static_assert( x, #    x )
#endif

#endif
