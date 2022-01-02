
#ifndef _OgreStlVector_H_
#define _OgreStlVector_H_

#include <vector>

#include "OgreMemorySTLAllocator.h"

namespace Ogre
{
#if OGRE_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
#    define OGRE_STL_ALIGNMENT_DEF_ARG , typename A = STLAllocator<T, AllocPolicy>
#    define OGRE_STL_ALIGNMENT_ARG , typename A
#    define OGRE_STL_ALIGNMENT_A , A
#else
#    define OGRE_STL_ALIGNMENT_DEF_ARG
#    define OGRE_STL_ALIGNMENT_ARG
#    define OGRE_STL_ALIGNMENT_A
#endif

    template <typename T OGRE_STL_ALIGNMENT_DEF_ARG>
    struct vector
    {
        typedef typename std::vector<T OGRE_STL_ALIGNMENT_A>                 type;
        typedef typename std::vector<T OGRE_STL_ALIGNMENT_A>::iterator       iterator;
        typedef typename std::vector<T OGRE_STL_ALIGNMENT_A>::const_iterator const_iterator;
    };

    template <typename T OGRE_STL_ALIGNMENT_ARG>
    class StdVector : public std::vector<T OGRE_STL_ALIGNMENT_A>
    {
    };

#undef OGRE_STL_ALIGNMENT_A
#undef OGRE_STL_ALIGNMENT_ARG
#undef OGRE_STL_ALIGNMENT_DEF_ARG
}  // namespace Ogre

#endif
