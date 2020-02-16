
#ifndef _OgreStlVector_H_
#define _OgreStlVector_H_

#include <vector>

#include "OgreMemorySTLAllocator.h"

namespace Ogre
{
    template <typename T, typename A = STLAllocator<T, GeneralAllocPolicy> >
    struct vector
    {
#if OGRE_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename std::vector<T, A> type;
        typedef typename std::vector<T, A>::iterator iterator;
        typedef typename std::vector<T, A>::const_iterator const_iterator;
#else
        typedef typename std::vector<T> type;
        typedef typename std::vector<T>::iterator iterator;
        typedef typename std::vector<T>::const_iterator const_iterator;
#endif
    };

    template <typename T, typename A>
    class StdVector : public std::vector<T, A>
    {
    };
}  // namespace Ogre

#endif
