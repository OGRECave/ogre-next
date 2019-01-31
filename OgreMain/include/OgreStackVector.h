/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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

#ifndef _OgreStackVector_H_
#define _OgreStackVector_H_

#include "OgreAssert.h"

namespace Ogre
{
    /** Compact implementation similar to std::array
    @remarks
        This container is a simple one, for simple cases

        It will assert on buffer overflows, but it asserts are disabled,
        it will let them happen.
    @author
        Matias N. Goldberg
    @version
        1.0
    */
    template <typename T, size_t Capacity> class StackVector
    {
        T           mData[Capacity];
        size_t      mSize;

    public:
        typedef T value_type;

        typedef T* iterator;
        typedef const T* const_iterator;

        StackVector() :
            mSize( 0 )
        {
        }

        template <size_t C>
        StackVector( const StackVector<T, C> &copy ) :
            mSize( copy.mSize )
        {
            OGRE_ASSERT_LOW( copy.mSize <= Capacity );
            for( size_t i=0; i<mSize; ++i )
            {
                new (&mData[i]) T( copy.mData[i] );
            }
        }

        template <size_t C>
        void operator = ( const StackVector<T, C> &copy )
        {
            if( copy.data() != this->data() )
            {
                for( size_t i=0; i<mSize; ++i )
                    mData[i] = T();

                OGRE_ASSERT_LOW( copy.size() <= Capacity );
                mSize = copy.size();

                const T * RESTRICT_ALIAS srcData = copy.data();

                for( size_t i=0; i<mSize; ++i )
                {
                    new (&mData[i]) T( srcData[i] );
                }
            }
        }

        /// Creates an array pushing the value N times
        StackVector( size_t count, const T &value ) :
            mSize( count )
        {
            for( size_t i=0; i<count; ++i )
                new (&mData[i]) T( value );
        }

        ~StackVector()
        {
            destroy();
        }

        void destroy()
        {
            mSize = 0;
        }

        size_t size() const                     { return mSize; }
        size_t capacity() const                 { return Capacity; }
        T* data()                               { return mData; }
        const T* data() const                   { return mData; }

        void push_back( const T& val )
        {
            OGRE_ASSERT_LOW( mSize < Capacity );
            new (&mData[mSize]) T( val );
            ++mSize;
        }

        void pop_back()
        {
            OGRE_ASSERT_LOW( mSize > 0 && "Can't pop a zero-sized array" );
            --mSize;
            mData[mSize] = T();
        }

        iterator insert( iterator where, const T& val )
        {
            size_t idx = (where - mData);

            OGRE_ASSERT_LOW( mSize < Capacity );

            memmove( mData + idx + 1, mData + idx, (mSize - idx) *  sizeof(T) );
            new (&mData[idx]) T( val );
            ++mSize;

            return mData + idx;
        }

        /// otherBegin & otherEnd must not overlap with this->begin() and this->end()
        iterator insertPOD( iterator where, const_iterator otherBegin, const_iterator otherEnd )
        {
            size_t idx = (where - mData);

            const size_t otherSize = otherEnd - otherBegin;

            OGRE_ASSERT_LOW( mSize + otherSize <= Capacity );

            memmove( mData + idx + otherSize, mData + idx, (mSize - idx) *  sizeof(T) );

            while( otherBegin != otherEnd )
                *where++ = *otherBegin++;
            mSize += otherSize;

            return mData + idx;
        }

        void appendPOD( const_iterator otherBegin, const_iterator otherEnd )
        {
            OGRE_ASSERT_LOW( mSize + (otherEnd - otherBegin) <= Capacity );

            memcpy( mData + mSize, otherBegin, (otherEnd - otherBegin) *  sizeof(T) );
            mSize += otherEnd - otherBegin;
        }

        iterator erase( iterator toErase )
        {
            size_t idx = (toErase - mData);
            toErase = T();
            memmove( mData + idx, mData + idx + 1, (mSize - idx - 1) * sizeof(T) );
            --mSize;

            return mData + idx;
        }

        iterator erase( iterator first, iterator last )
        {
            assert( first <= last && last <= end() );

            size_t idx      = (first - mData);
            size_t idxNext  = (last - mData);
            while( first != last )
            {
                first = T();
                ++first;
            }
            memmove( mData + idx, mData + idxNext, (mSize - idxNext) * sizeof(T) );
            mSize -= idxNext - idx;

            return mData + idx;
        }

        iterator erasePOD( iterator first, iterator last )
        {
            assert( first <= last && last <= end() );

            size_t idx      = (first - mData);
            size_t idxNext  = (last - mData);
            memmove( mData + idx, mData + idxNext, (mSize - idxNext) * sizeof(T) );
            mSize -= idxNext - idx;

            return mData + idx;
        }

        void clear()
        {
            for( size_t i=0; i<mSize; ++i )
                mData[i] = T();
            mSize = 0;
        }

        bool empty() const                      { return mSize == 0; }

        void resize( size_t newSize, const T &value=T() )
        {
            if( newSize > mSize )
            {
                OGRE_ASSERT_LOW( newSize <= Capacity );
                for( size_t i=mSize; i<newSize; ++i )
                {
                    new (&mData[i]) T( value );
                }
            }
            else
            {
                for( size_t i=newSize; i<mSize; ++i )
                {
                    mData[i] = T();
                }
            }

            mSize = newSize;
        }

        void resizePOD( size_t newSize, const T &value=T() )
        {
            if( newSize > mSize )
            {
                OGRE_ASSERT_LOW( newSize <= Capacity );
                for( size_t i=mSize; i<newSize; ++i )
                {
                    new (&mData[i]) T( value );
                }
            }

            mSize = newSize;
        }

        T& operator [] ( size_t idx )
        {
            assert( idx < mSize && "Index out of bounds" );
            return mData[idx];
        }

        const T& operator [] ( size_t idx ) const
        {
            assert( idx < mSize && "Index out of bounds" );
            return mData[idx];
        }

        T& back()
        {
            assert( mSize > 0 && "Can't call back with no elements" );
            return mData[mSize-1];
        }

        const T& back() const
        {
            assert( mSize > 0 && "Can't call back with no elements" );
            return mData[mSize-1];
        }

        T& front()
        {
            assert( mSize > 0 && "Can't call front with no elements" );
            return mData[0];
        }

        const T& front() const
        {
            assert( mSize > 0 && "Can't call front with no elements" );
            return mData[0];
        }

        iterator begin()                        { return mData; }
        const_iterator begin() const            { return mData; }
        iterator end()                          { return mData + mSize; }
        const_iterator end() const              { return mData + mSize; }
    };
}

#endif
