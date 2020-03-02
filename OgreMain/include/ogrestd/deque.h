
#ifndef _OgreStlDeque_H_
#define _OgreStlDeque_H_

#include <deque>

#include "OgreMemorySTLAllocator.h"

namespace Ogre
{
    template <typename T, typename A = STLAllocator<T, GeneralAllocPolicy> >
    struct deque
    {
#if OGRE_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename std::deque<T, A> type;
        typedef typename std::deque<T, A>::iterator iterator;
        typedef typename std::deque<T, A>::const_iterator const_iterator;
#else
        typedef typename std::deque<T> type;
        typedef typename std::deque<T>::iterator iterator;
        typedef typename std::deque<T>::const_iterator const_iterator;
#endif
    };
}  // namespace Ogre

#endif
