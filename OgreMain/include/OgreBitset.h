/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2018 Torus Knot Software Ltd

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

#ifndef _OgreBitset_
#define _OgreBitset_

#include "OgrePrerequisites.h"

namespace Ogre
{
    /** @ingroup Core
    @class cbitsetN
    */
    template <size_t _N, typename _internalDataType, size_t _bits, size_t _mask>
    class cbitsetN
    {
        _internalDataType mValues[( _N + ( 1u << _bits ) - 1u ) >> _bits];

    public:
        cbitsetN() { clear(); }

        /// Sets all bits to 0
        void clear() { memset( mValues, 0, sizeof( mValues ) ); }

        /// Sets all bits to 1
        void setAll() { memset( mValues, 0xFF, sizeof( mValues ) ); }

        /// Return maximum number of bits this bitset can hold
        size_t capacity() const { return _N; }

        /** Sets bit at 'position'
        @param position
            Value in range [0; _N)
        @param bValue
        */
        void setValue( size_t position, bool bValue )
        {
            OGRE_ASSERT_MEDIUM( position < _N );
            const size_t idx = position >> _bits;
            const size_t mask = 1u << ( position & _mask );
            if( bValue )
                mValues[idx] |= mask;
            else
                mValues[idx] &= ~mask;
        }

        /** Sets bit at 'position' to 1
        @param position
            Value in range [0; _N)
        */
        void set( size_t position )
        {
            OGRE_ASSERT_MEDIUM( position < _N );
            const size_t idx = position >> _bits;
            const size_t mask = 1u << ( position & _mask );
            mValues[idx] |= mask;
        }
        /** Sets bit at 'position' to 0
        @param position
            Value in range [0; _N)
        */
        void unset( size_t position )
        {
            OGRE_ASSERT_MEDIUM( position < _N );
            const size_t idx = position >> _bits;
            const size_t mask = 1u << ( position & _mask );
            mValues[idx] &= ~mask;
        }
        /** Returns true if bit at 'position' is 1
        @param position
            Value in range [0; _N)
        */
        bool test( size_t position ) const
        {
            OGRE_ASSERT_MEDIUM( position < _N );
            const size_t idx = position >> _bits;
            const size_t mask = 1u << ( position & _mask );
            return ( mValues[idx] & mask ) != 0u;
        }

        /// Returns the number of bits that are set between range [0; positionEnd).
        uint32 numBitsSet( size_t positionEnd ) const
        {
            OGRE_ASSERT_MEDIUM( positionEnd < _N );
            uint32 retVal = 0u;
            for( size_t i = 0u; i < positionEnd; )
            {
                if( ( positionEnd - i ) >= _mask )
                {
                    const _internalDataType value = mValues[i >> _bits];
                    if( value == (_internalDataType)-1 )
                    {
                        retVal += _mask + 1u;
                    }
                    else if( value != 0u )
                    {
                        for( size_t j = 0u; j <= _mask; ++j )
                            retVal += ( value & ( 1u << j ) ) != 0u ? 1u : 0u;
                    }

                    i += _mask + 1u;
                }
                else
                {
                    retVal += test( i ) ? 1u : 0u;
                    ++i;
                }
            }
            return retVal;
        }
    };

    /** @ingroup Core
    @class cbitset32
        This is similar to std::bitset, except waaay less bloat.
        cbitset32 stands for constant/compile-time bitset with an internal representation of 32-bits
    */
    template <size_t _N>
    class cbitset32 : public cbitsetN<_N, uint32, 5u, 0x1Fu>
    {
    };
}  // namespace Ogre

#endif
