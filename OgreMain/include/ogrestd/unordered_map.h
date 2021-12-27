
#ifndef _OgreStlUnorderedMap_H_
#define _OgreStlUnorderedMap_H_

#include "OgreMemorySTLAllocator.h"

#include <unordered_map>

namespace Ogre
{
    template <typename K, typename V, typename H = ::std::hash<K>,
              typename E = std::equal_to<K>,
              typename A = STLAllocator<std::pair<const K, V>, GeneralAllocPolicy> >
    struct unordered_map
    {
#if OGRE_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename ::std::unordered_map<K, V, H, E, A> type;
#else
        typedef typename ::std::unordered_map<K, V, H, E> type;
#endif
        typedef typename type::iterator iterator;
        typedef typename type::const_iterator const_iterator;
    };

    template <typename K, typename V, typename H = ::std::hash<K>,
              typename E = std::equal_to<K>,
              typename A = STLAllocator<std::pair<const K, V>, GeneralAllocPolicy> >
    struct unordered_multimap
    {
#if OGRE_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename ::std::unordered_multimap<K, V, H, E, A> type;
#else
        typedef typename ::std::unordered_multimap<K, V, H, E> type;
#endif
        typedef typename type::iterator iterator;
        typedef typename type::const_iterator const_iterator;
    };
}  // namespace Ogre

#endif
