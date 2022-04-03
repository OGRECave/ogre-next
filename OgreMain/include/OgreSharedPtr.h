/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#ifndef __SharedPtr_H__
#define __SharedPtr_H__

#include "OgrePrerequisites.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup General
     *  @{
     */
    struct SPFMDeleteT
    {
        template <class T>
        void operator()( T *p )
        {
            OGRE_DELETE_T( p, T, MEMCATEGORY_GENERAL );
        }
    };
    const SPFMDeleteT SPFM_DELETE_T;

    using std::dynamic_pointer_cast;
    using std::static_pointer_cast;

    /// @deprecated for backwards compatibility only, rather use std::shared_ptr directly
    template <class T>
    class SharedPtr : public std::shared_ptr<T>
    {
    public:
        SharedPtr( std::nullptr_t ) {}
        SharedPtr() {}
        template <class Y>
        explicit SharedPtr( Y *ptr ) : std::shared_ptr<T>( ptr )
        {
        }
        template <class Y, class Deleter>
        SharedPtr( Y *ptr, Deleter d ) : std::shared_ptr<T>( ptr, d )
        {
        }
        SharedPtr( const SharedPtr &r ) : std::shared_ptr<T>( r ) {}
        template <class Y>
        SharedPtr( const SharedPtr<Y> &r ) : std::shared_ptr<T>( r )
        {
        }

        // implicit conversion from and to std::shared_ptr
        template <class Y>
        SharedPtr( const std::shared_ptr<Y> &r ) : std::shared_ptr<T>( r )
        {
        }

        operator const std::shared_ptr<T> &() { return static_cast<std::shared_ptr<T> &>( *this ); }

        SharedPtr<T> &operator=( const Ogre::SharedPtr<T> &rhs )
        {
            std::shared_ptr<T>::operator=( rhs );
            return *this;
        }
        // so swig recognizes it should forward the operators
        T *operator->() const { return std::shared_ptr<T>::operator->(); }

        /// @deprecated use Ogre::static_pointer_cast instead
        template <typename Y>
        OGRE_DEPRECATED SharedPtr<Y> staticCast() const
        {
            return static_pointer_cast<Y>( *this );
        }
        /// @deprecated use Ogre::dynamic_pointer_cast instead
        template <typename Y>
        OGRE_DEPRECATED SharedPtr<Y> dynamicCast() const
        {
            return dynamic_pointer_cast<Y>( *this );
        }
        /// @deprecated this api will be dropped. use reset(T*) instead
        OGRE_DEPRECATED void bind( T *rep ) { std::shared_ptr<T>::reset( rep ); }
        /// @deprecated use use_count() instead
        OGRE_DEPRECATED unsigned int useCount() const { return std::shared_ptr<T>::use_count(); }
        /// @deprecated use get() instead
        OGRE_DEPRECATED T *getPointer() const { return std::shared_ptr<T>::get(); }
        /// @deprecated use SharedPtr::operator bool instead
        OGRE_DEPRECATED bool isNull() const { return !std::shared_ptr<T>::operator bool(); }
        /// @deprecated use reset() instead
        OGRE_DEPRECATED void setNull() { std::shared_ptr<T>::reset(); }
    };
    /** @} */
    /** @} */
}  // namespace Ogre

namespace std
{
    template <typename T>
    struct hash<Ogre::SharedPtr<T> >
    {
        size_t operator()( const Ogre::SharedPtr<T> &k ) const noexcept
        {
            return hash<T *>()( k.get() );
        }
    };
}  // namespace std

#endif
