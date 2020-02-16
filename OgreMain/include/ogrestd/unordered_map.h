
#ifndef _OgreStlUnorderedMap_H_
#define _OgreStlUnorderedMap_H_

#include "OgreMemorySTLAllocator.h"

#if( OGRE_COMPILER == OGRE_COMPILER_GNUC ) && !defined( STLPORT )
#    if __cplusplus >= 201103L
#        include <unordered_map>
#    elif OGRE_COMP_VER >= 430
#        include <tr1/unordered_map>
#    else
#        include <ext/hash_map>
#    endif
#elif( OGRE_COMPILER == OGRE_COMPILER_CLANG )
#    if defined( _LIBCPP_VERSION ) || __cplusplus >= 201103L
#        include <unordered_map>
#    else
#        include <tr1/unordered_map>
#    endif
#elif !defined( STLPORT )
#    if( OGRE_COMPILER == OGRE_COMPILER_MSVC ) && _MSC_FULL_VER >= 150030729  // VC++ 9.0 SP1+
#        include <unordered_map>
#    elif OGRE_THREAD_PROVIDER == 1
#        include <boost/unordered_map.hpp>
#    else
#        error "Your compiler doesn't support unordered_map. Try to compile Ogre with Boost or STLPort."
#    endif
#endif

namespace Ogre
{
    template <typename K, typename V, typename H = OGRE_HASH_NAMESPACE::hash<K>,
              typename E = std::equal_to<K>,
              typename A = STLAllocator<std::pair<const K, V>, GeneralAllocPolicy> >
    struct unordered_map
    {
#if OGRE_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename OGRE_HASH_NAMESPACE::OGRE_HASHMAP_NAME<K, V, H, E, A> type;
#else
        typedef typename OGRE_HASH_NAMESPACE::OGRE_HASHMAP_NAME<K, V, H, E> type;
#endif
        typedef typename type::iterator iterator;
        typedef typename type::const_iterator const_iterator;
    };

    template <typename K, typename V, typename H = OGRE_HASH_NAMESPACE::hash<K>,
              typename E = std::equal_to<K>,
              typename A = STLAllocator<std::pair<const K, V>, GeneralAllocPolicy> >
    struct unordered_multimap
    {
#if OGRE_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename OGRE_HASH_NAMESPACE::OGRE_HASHMULTIMAP_NAME<K, V, H, E, A> type;
#else
        typedef typename OGRE_HASH_NAMESPACE::OGRE_HASHMULTIMAP_NAME<K, V, H, E> type;
#endif
        typedef typename type::iterator iterator;
        typedef typename type::const_iterator const_iterator;
    };
}  // namespace Ogre

#endif
