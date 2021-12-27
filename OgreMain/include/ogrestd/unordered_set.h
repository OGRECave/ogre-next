
#ifndef _OgreStlUnorderedSet_H_
#define _OgreStlUnorderedSet_H_

#include "OgreMemorySTLAllocator.h"

#include <unordered_set>

namespace Ogre
{
    template <typename K, typename H = ::std::hash<K>, typename E = std::equal_to<K>,
              typename A = STLAllocator<K, GeneralAllocPolicy> >
    struct unordered_set
    {
#if OGRE_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename ::std::unordered_set<K, H, E, A> type;
#else
        typedef typename ::std::unordered_set<K, H, E> type;
#endif
        typedef typename type::iterator iterator;
        typedef typename type::const_iterator const_iterator;
    };

    template <typename K, typename H = ::std::hash<K>, typename E = std::equal_to<K>,
              typename A = STLAllocator<K, GeneralAllocPolicy> >
    struct unordered_multiset
    {
#if OGRE_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename ::std::unordered_multiset<K, H, E, A> type;
#else
        typedef typename ::std::unordered_multiset<K, H, E> type;
#endif
        typedef typename type::iterator iterator;
        typedef typename type::const_iterator const_iterator;
    };

    template <typename K, typename H, typename E, typename A>
    class StdUnorderedSet : public ::std::unordered_set<K, H, E, A>
    {
    };
}  // namespace Ogre

#endif
