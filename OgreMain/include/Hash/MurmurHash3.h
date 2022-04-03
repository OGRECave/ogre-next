//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#ifndef _MURMURHASH3_H_
#define _MURMURHASH3_H_

//-----------------------------------------------------------------------------
// Platform-specific functions and macros

// Microsoft Visual Studio

#if defined( _MSC_VER ) && _MSC_VER < 1600

namespace Ogre
{
    typedef unsigned char    uint8_t;
    typedef unsigned long    uint32_t;
    typedef unsigned __int64 uint64_t;
}  // namespace Ogre
// Other compilers

#else  // defined(_MSC_VER)

#    include <stdint.h>

#endif  // !defined(_MSC_VER)

//-----------------------------------------------------------------------------

namespace Ogre
{
    void _OgreExport MurmurHash3_x86_32( const void *key, int len, uint32_t seed, void *out );

    void _OgreExport MurmurHash3_x86_128( const void *key, int len, uint32_t seed, void *out );

    void _OgreExport MurmurHash3_x64_128( const void *key, int len, uint32_t seed, void *out );
}  // namespace Ogre

//-----------------------------------------------------------------------------

#endif  // _MURMURHASH3_H_
