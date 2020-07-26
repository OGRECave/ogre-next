
#ifndef _OgreStlSet_H_
#define _OgreStlSet_H_

#include <set>

#include "OgreMemorySTLAllocator.h"

namespace Ogre
{
    template <typename T, typename P = std::less<T>, typename A = STLAllocator<T, GeneralAllocPolicy> >
    struct set
    {
#if OGRE_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename std::set<T, P, A> type;
        typedef typename std::set<T, P, A>::iterator iterator;
        typedef typename std::set<T, P, A>::const_iterator const_iterator;
#else
        typedef typename std::set<T, P> type;
        typedef typename std::set<T, P>::iterator iterator;
        typedef typename std::set<T, P>::const_iterator const_iterator;
#endif
    };
}  // namespace Ogre

#endif
