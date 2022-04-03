
#ifndef _OgreStlMap_H_
#define _OgreStlMap_H_

#include <map>

#include "OgreMemorySTLAllocator.h"

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

    template <typename K, typename V, typename P = std::less<K> OGRE_STL_ALIGNMENT_DEF_ARG>
    struct map
    {
        typedef typename std::map<K, V, P OGRE_STL_ALIGNMENT_A>                 type;
        typedef typename std::map<K, V, P OGRE_STL_ALIGNMENT_A>::iterator       iterator;
        typedef typename std::map<K, V, P OGRE_STL_ALIGNMENT_A>::const_iterator const_iterator;
    };

    template <typename K, typename V, typename P = std::less<K> OGRE_STL_ALIGNMENT_DEF_ARG>
    struct multimap
    {
        typedef typename std::multimap<K, V, P OGRE_STL_ALIGNMENT_A>                 type;
        typedef typename std::multimap<K, V, P OGRE_STL_ALIGNMENT_A>::iterator       iterator;
        typedef typename std::multimap<K, V, P OGRE_STL_ALIGNMENT_A>::const_iterator const_iterator;
    };

    template <typename K, typename V, typename P OGRE_STL_ALIGNMENT_ARG>
    class StdMap : public std::map<K, V, P OGRE_STL_ALIGNMENT_A>
    {
    public:
        StdMap() : std::map<K, V, P OGRE_STL_ALIGNMENT_A>() {}

        StdMap( std::initializer_list<typename std::map<K, V, P OGRE_STL_ALIGNMENT_A>::value_type> __l,
                const P &__comp = P() ) :
            std::map<K, V, P OGRE_STL_ALIGNMENT_A>( __l, __comp )
        {
        }
    };

    template <typename K, typename V, typename P OGRE_STL_ALIGNMENT_ARG>
    class StdMultiMap : public std::multimap<K, V, P OGRE_STL_ALIGNMENT_A>
    {
    };

#undef OGRE_STL_ALIGNMENT_A
#undef OGRE_STL_ALIGNMENT_ARG
#undef OGRE_STL_ALIGNMENT_DEF_ARG

}  // namespace Ogre

#endif
