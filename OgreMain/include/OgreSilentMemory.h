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

#ifndef OgreSilentMemory_H_
#define OgreSilentMemory_H_

// Define silent_memset, a memset that doesn't warn on GCC when we're overriding non-POD structs
// Same with memcpy

#include <string.h>

#if defined( __GNUC__ ) && !defined( __clang__ ) && defined( __nonnull ) && defined( __fortify_function )
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wclass-memaccess"

inline void *silent_memset( void *a, int b, size_t c ) noexcept __nonnull( ( 1 ) );
inline void *silent_memset( void *a, int b, size_t c ) noexcept
{
    return memset( a, b, c );
}

inline void *silent_memcpy( void *__restrict __dest, const void *__restrict __src, size_t __n ) noexcept
    __nonnull( ( 1, 2 ) );
inline void *silent_memcpy( void *__restrict a, const void *__restrict b, size_t c ) noexcept
{
    return memcpy( a, b, c );
}

__fortify_function void *__NTH( silent_memmove( void *a, const void *b, size_t c ) )
{
    return memmove( a, b, c );
}

#    pragma GCC diagnostic pop
#else
#    define silent_memset( a, b, c ) memset( a, b, c )
#    define silent_memcpy( a, b, c ) memcpy( a, b, c )
#    define silent_memmove( a, b, c ) memmove( a, b, c )
#endif

#endif
