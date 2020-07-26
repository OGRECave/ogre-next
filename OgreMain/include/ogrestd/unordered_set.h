
#ifndef _OgreStlUnorderedSet_H_
#define _OgreStlUnorderedSet_H_

#include "OgreMemorySTLAllocator.h"

#if( OGRE_COMPILER == OGRE_COMPILER_GNUC ) && !defined( STLPORT )
#    if __cplusplus >= 201103L
#        include <unordered_set>
#    elif OGRE_COMP_VER >= 430
#        include <tr1/unordered_set>
#    else
#        include <ext/hash_set>
#    endif
#elif( OGRE_COMPILER == OGRE_COMPILER_CLANG )
#    if defined( _LIBCPP_VERSION ) || __cplusplus >= 201103L
#        include <unordered_set>
#    else
#        include <tr1/unordered_set>
#    endif
#elif !defined( STLPORT )
#    if( OGRE_COMPILER == OGRE_COMPILER_MSVC ) && _MSC_FULL_VER >= 150030729  // VC++ 9.0 SP1+
#        include <unordered_set>
#    elif OGRE_THREAD_PROVIDER == 1
#        include <boost/unordered_set.hpp>
#    else
#        error "Your compiler doesn't support unordered_set. Try to compile Ogre with Boost or STLPort."
#    endif
#endif

namespace Ogre
{
    template <typename K, typename H = OGRE_HASH_NAMESPACE::hash<K>, typename E = std::equal_to<K>,
              typename A = STLAllocator<K, GeneralAllocPolicy> >
    struct unordered_set
    {
#if OGRE_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename OGRE_HASH_NAMESPACE::OGRE_HASHSET_NAME<K, H, E, A> type;
#else
        typedef typename OGRE_HASH_NAMESPACE::OGRE_HASHSET_NAME<K, H, E> type;
#endif
        typedef typename type::iterator iterator;
        typedef typename type::const_iterator const_iterator;
    };

    template <typename K, typename H = OGRE_HASH_NAMESPACE::hash<K>, typename E = std::equal_to<K>,
              typename A = STLAllocator<K, GeneralAllocPolicy> >
    struct unordered_multiset
    {
#if OGRE_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
        typedef typename OGRE_HASH_NAMESPACE::OGRE_HASHMULTISET_NAME<K, H, E, A> type;
#else
        typedef typename OGRE_HASH_NAMESPACE::OGRE_HASHMULTISET_NAME<K, H, E> type;
#endif
        typedef typename type::iterator iterator;
        typedef typename type::const_iterator const_iterator;
    };

    template <typename K, typename H, typename E, typename A>
    class StdUnorderedSet : public OGRE_HASH_NAMESPACE::OGRE_HASHSET_NAME<K, H, E, A>
    {
    };
}  // namespace Ogre

#endif
