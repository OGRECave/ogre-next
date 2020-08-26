#ifndef __StdHeaders_H__
#define __StdHeaders_H__

#ifdef __BORLANDC__
    #define __STD_ALGORITHM
#endif

#if defined ( OGRE_GCC_VISIBILITY ) && ((OGRE_PLATFORM == OGRE_PLATFORM_APPLE && !__LP64__) && OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS)
/* Until libstdc++ for gcc 4.2 is released, we have to declare all
 * symbols in libstdc++.so externally visible, otherwise we end up
 * with them marked as hidden by -fvisible=hidden.
 *
 * See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=20218
 *
 * Due to a more strict linker included with Xcode 4, this is disabled on Mac OS X and iOS.
 * The reason? It changes the visibility of Boost functions.  The mismatch between visibility Boost when used in Ogre (default)
 * and Boost when compiled (hidden) results in mysterious link errors such as "Bad codegen, pointer diff".
 */
#   pragma GCC visibility push(default)
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>

// STL containers
#include <vector>
#include <string>

#include "OgreFastArray.h"

// STL algorithms & functions
#include <algorithm>
#include <limits>
#include <functional>

// C++ Stream stuff
#include <iosfwd>

#ifdef __BORLANDC__
namespace Ogre
{
    using namespace std;
}
#endif

extern "C" {

#   include <sys/types.h>
#   include <sys/stat.h>

}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
#  undef min
#  undef max
#  if defined( __MINGW32__ )
#    include <unistd.h>
#  endif
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX || \
    OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || \
    OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN || \
    OGRE_PLATFORM == OGRE_PLATFORM_FREEBSD
extern "C" {

#   include <unistd.h>
#   include <dlfcn.h>

}
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
extern "C" {
#   include <unistd.h>
#   include <sys/param.h>
#   include <CoreFoundation/CoreFoundation.h>
}
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
#   include <emscripten/emscripten.h>
#endif

#if defined ( OGRE_GCC_VISIBILITY ) && ((OGRE_PLATFORM == OGRE_PLATFORM_APPLE && !__LP64__) && OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS)
#   pragma GCC visibility pop
#endif
#endif
