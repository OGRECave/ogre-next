
#ifndef _OgreStlList_H_
#define _OgreStlList_H_

#include <list>

#include "OgreMemorySTLAllocator.h"

namespace Ogre
{
    template <typename T, typename A = STLAllocator<T, GeneralAllocPolicy> >
    struct list
    {
#if OGRE_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename std::list<T, A> type;
        typedef typename std::list<T, A>::iterator iterator;
        typedef typename std::list<T, A>::const_iterator const_iterator;
#else
        typedef typename std::list<T> type;
        typedef typename std::list<T>::iterator iterator;
        typedef typename std::list<T>::const_iterator const_iterator;
#endif
    };

    template <typename T, typename A>
    class StdList : public std::list<T, A>
    {
    };
}  // namespace Ogre

#endif
