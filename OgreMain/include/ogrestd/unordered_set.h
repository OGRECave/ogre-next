
#ifndef _OgreStlUnorderedSet_H_
#define _OgreStlUnorderedSet_H_

#include "OgreMemorySTLAllocator.h"

#include <unordered_set>

namespace Ogre
{
#if OGRE_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
#    define OGRE_STL_ALIGNMENT_DEF_ARG , typename A = STLAllocator<K, AllocPolicy>
#    define OGRE_STL_ALIGNMENT_ARG , typename A
#    define OGRE_STL_ALIGNMENT_A , A
#else
#    define OGRE_STL_ALIGNMENT_DEF_ARG
#    define OGRE_STL_ALIGNMENT_ARG
#    define OGRE_STL_ALIGNMENT_A
#endif

    template <typename K, typename H = ::std::hash<K>,
              typename E = std::equal_to<K> OGRE_STL_ALIGNMENT_DEF_ARG>
    struct unordered_set
    {
        typedef typename ::std::unordered_set<K, H, E OGRE_STL_ALIGNMENT_A> type;
        typedef typename type::iterator                                     iterator;
        typedef typename type::const_iterator                               const_iterator;
    };

    template <typename K, typename H = ::std::hash<K>,
              typename E = std::equal_to<K> OGRE_STL_ALIGNMENT_DEF_ARG>
    struct unordered_multiset
    {
        typedef typename ::std::unordered_multiset<K, H, E OGRE_STL_ALIGNMENT_A> type;
        typedef typename type::iterator                                          iterator;
        typedef typename type::const_iterator                                    const_iterator;
    };

    template <typename K, typename H, typename E OGRE_STL_ALIGNMENT_ARG>
    class StdUnorderedSet : public ::std::unordered_set<K, H, E OGRE_STL_ALIGNMENT_A>
    {
    };

#undef OGRE_STL_ALIGNMENT_A
#undef OGRE_STL_ALIGNMENT_ARG
#undef OGRE_STL_ALIGNMENT_DEF_ARG

}  // namespace Ogre

#endif
