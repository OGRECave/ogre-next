
#ifndef _OgreStlMap_H_
#define _OgreStlMap_H_

#include <map>

#include "OgreMemorySTLAllocator.h"

namespace Ogre
{
    template <typename K, typename V, typename P = std::less<K>,
              typename A = STLAllocator<std::pair<const K, V>, GeneralAllocPolicy> >
    struct map
    {
#if OGRE_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename std::map<K, V, P, A> type;
        typedef typename std::map<K, V, P, A>::iterator iterator;
        typedef typename std::map<K, V, P, A>::const_iterator const_iterator;
#else
        typedef typename std::map<K, V, P> type;
        typedef typename std::map<K, V, P>::iterator iterator;
        typedef typename std::map<K, V, P>::const_iterator const_iterator;
#endif
    };

    template <typename K, typename V, typename P = std::less<K>,
              typename A = STLAllocator<std::pair<const K, V>, GeneralAllocPolicy> >
    struct multimap
    {
#if OGRE_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename std::multimap<K, V, P, A> type;
        typedef typename std::multimap<K, V, P, A>::iterator iterator;
        typedef typename std::multimap<K, V, P, A>::const_iterator const_iterator;
#else
        typedef typename std::multimap<K, V, P> type;
        typedef typename std::multimap<K, V, P>::iterator iterator;
        typedef typename std::multimap<K, V, P>::const_iterator const_iterator;
#endif
    };

    template <typename K, typename V, typename P, typename A>
    class StdMap : public std::map<K, V, P, A>
    {
    public:
        StdMap() : std::map<K, V, P, A>() {}

#if __cplusplus >= 201103L
        StdMap( std::initializer_list<typename std::map<K, V, P, A>::value_type> __l,
                const P &__comp = P() ) :
            std::map<K, V, P, A>( __l, __comp )
        {
        }
#endif
    };

    template <typename K, typename V, typename P, typename A>
    class StdMultiMap : public std::multimap<K, V, P, A>
    {
    };
}  // namespace Ogre

#endif
