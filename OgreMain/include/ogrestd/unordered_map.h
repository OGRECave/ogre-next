
#ifndef _OgreStlUnorderedMap_H_
#define _OgreStlUnorderedMap_H_

#include "OgreMemorySTLAllocator.h"

#include <unordered_map>

namespace Ogre
{
#if OGRE_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
#    define OGRE_STL_ALIGNMENT_DEF_ARG , typename A = STLAllocator<std::pair<const K, V>, AllocPolicy>
#    define OGRE_STL_ALIGNMENT_ARG , typename A
#    define OGRE_STL_ALIGNMENT_A , A
#else
#    define OGRE_STL_ALIGNMENT_DEF_ARG
#    define OGRE_STL_ALIGNMENT_ARG
#    define OGRE_STL_ALIGNMENT_A
#endif

    template <typename K, typename V, typename H = ::std::hash<K>,
              typename E = std::equal_to<K> OGRE_STL_ALIGNMENT_DEF_ARG>
    struct unordered_map
    {
        typedef typename ::std::unordered_map<K, V, H, E OGRE_STL_ALIGNMENT_A> type;
        typedef typename type::iterator                                        iterator;
        typedef typename type::const_iterator                                  const_iterator;
    };

    template <typename K, typename V, typename H = ::std::hash<K>,
              typename E = std::equal_to<K> OGRE_STL_ALIGNMENT_DEF_ARG>
    struct unordered_multimap
    {
        typedef typename ::std::unordered_multimap<K, V, H, E OGRE_STL_ALIGNMENT_A> type;
        typedef typename type::iterator                                             iterator;
        typedef typename type::const_iterator                                       const_iterator;
    };

#undef OGRE_STL_ALIGNMENT_A
#undef OGRE_STL_ALIGNMENT_ARG
#undef OGRE_STL_ALIGNMENT_DEF_ARG

}  // namespace Ogre

#endif
