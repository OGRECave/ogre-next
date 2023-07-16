/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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
    @remarks
        IMPORTANT:
        To use this class, you MUST include OgreBitset.inl in your cpp file
        This split into *.h and *.inl was to avoid bloating build times
        of headers that include OgreBitset.h

    @param _N
        Number of bits this bitset will hold
    @param _bits
        The exponent of the number of bits to hold per mValues.
        E.g. 32 bits per mValues needs _bits = 5, because 2^5 = 32
        Note we never tested other combinations where 2^_bits * 8 != sizeof( _internalDataType )
    @param _mask
        The maximum number of bits - 1 to avoid overflow
        e.g. 32 bits per mValues needs a mask of 31 (0x1F) to wrap around
        so when we call
        @code
            this->set( 32 );
        @endcode

        It actually performs:
        @code
            idx = 32 / 32;  // 32 >> 5
            mask = 32 % 32; // 32 & 0x1F
            this->mValues[idx] |= mask;
        @endcode
    */
    template <size_t _N, typename _internalDataType, size_t _bits, size_t _mask>
    class cbitsetN
    {
    protected:
        _internalDataType mValues[( _N + ( (size_t)1u << _bits ) - 1u ) >> _bits];

        inline size_t getInternalArraySize() const { return sizeof( mValues ) / sizeof( mValues[0] ); }

    public:
        cbitsetN() { clear(); }

        /// Sets all bits to 0
        void clear() { memset( mValues, 0, sizeof( mValues ) ); }

        /// Returns true if all bits are unset
        bool empty() const;

        /// Sets all bits to 1
        void setAll();

        /** Sets all bits in range [0; position)
            It's the same as calling:

            for( size_t i = 0u; i < position; ++i )
                this->set( i );

            Values in range [position; _N) are left untouched
        @param position
        */
        void setAllUntil( size_t position );

        /// Return maximum number of bits this bitset can hold
        size_t capacity() const { return _N; }

        /** Sets bit at 'position'
        @param position
            Value in range [0; _N)
        @param bValue
        */
        void setValue( const size_t position, const bool bValue );

        /** Sets bit at 'position' to 1
        @param position
            Value in range [0; _N)
        */
        void set( const size_t position );

        /** Sets bit at 'position' to 0
        @param position
            Value in range [0; _N)
        */
        void unset( const size_t position );

        /** Returns true if bit at 'position' is 1
        @param position
            Value in range [0; _N)
        */
        bool test( const size_t position ) const;

        /// Returns the number of bits that are set between range [0; positionEnd).
        size_t numBitsSet( const size_t positionEnd ) const;
    };

    /** @ingroup Core
    @class cbitset32
        This is similar to std::bitset, except waaay less bloat.
        cbitset32 stands for constant/compile-time bitset with an internal representation of 32-bits

        2^5 = 32
        0x1F = 32 - 1 = 5 bits set
    @remarks
        IMPORTANT:
        To use this class, you MUST include OgreBitset.inl in your cpp file
        This split into *.h and *.inl was to avoid bloating build times
        of headers that include OgreBitset.h
    @param _N
        Number of bits this bitset will hold
    */
    template <size_t _N>
    class cbitset32 : public cbitsetN<_N, uint32, 5u, 0x1Fu>
    {
    };

    /** @ingroup Core
    @class cbitset64
        This is similar to std::bitset, except waaay less bloat.
        cbitset64 stands for constant/compile-time bitset with an internal representation of 64-bits

        2^6 = 64
        0x3F = 64 - 1 = 6 bits set
    @remarks
        IMPORTANT:
        To use this class, you MUST include OgreBitset.inl in your cpp file
        This split into *.h and *.inl was to avoid bloating build times
        of headers that include OgreBitset.h
    @param _N
        Number of bits this bitset will hold
    */
    template <size_t _N>
    class cbitset64 : public cbitsetN<_N, uint64, 6u, 0x3Fu>
    {
    public:
        /** Finds the first bit set.
        @return
            The index to the first set bit
            returns this->capacity() if all bits are unset (i.e. all 0s)
        */
        size_t findFirstBitSet() const;

        /** Same as findFirstBitSet(); starting from startFrom (inclusive).
        @param startFrom
            In range [0; capacity)

            e.g. if capacity == 5 and we've set 01001b (0 is the 4th bit, 1 is the 0th bit) then:

                findFirstBitSet( 5 ) = INVALID
                findFirstBitSet( 4 ) = 5u (capacity, not found)
                findFirstBitSet( 3 ) = 3u
                findFirstBitSet( 2 ) = 3u
                findFirstBitSet( 1 ) = 3u
                findFirstBitSet( 0 ) = 0u
        @return
            The index to the first unset bit
            returns this->capacity() if all bits are unset (i.e. all 0s)
        */
        size_t findFirstBitSet( const size_t startFrom ) const;

        /** Finds the first bit unset after the last bit set.
        @return
            If all bits are unset (i.e. all 0s):
                Returns 0
            If at least one bit is set:
                The index to the last set bit, plus one.
                Return range is [1; capacity]
        */
        size_t findLastBitSetPlusOne() const;
    };

    /** @ingroup Core
    @class bitset64
        See cbitset64

        This is the same, but the maximum number of bits is not known
        at build time
    */
    class bitset64
    {
    protected:
        FastArray<uint64> mValues;
        size_t            mBitsCapacity;

    public:
        bitset64() : mBitsCapacity( 0u ) {}

        void reset( size_t bitsCapacity )
        {
            mBitsCapacity = bitsCapacity;
            mValues.clear();
            mValues.resizePOD( ( bitsCapacity + 63u ) >> 6u, 0 );
        }

        /// @copydoc cbitset64::clear
        void clear() { memset( mValues.begin(), 0, mValues.size() * sizeof( uint64 ) ); }

        /// @copydoc cbitset64::empty
        inline bool empty() const;

        /// @copydoc cbitset64::setAll
        inline void setAll();

        /// @copydoc cbitset64::setAllUntil
        inline void setAllUntil( size_t position );

        /// @copydoc cbitset64::capacity
        inline size_t capacity() const { return mBitsCapacity; }

        /// @copydoc cbitset64::setValue
        inline void setValue( const size_t position, const bool bValue );

        /// @copydoc cbitset64::set
        inline void set( const size_t position );

        /// @copydoc cbitset64::unset
        inline void unset( const size_t position );

        /// @copydoc cbitset64::test
        inline bool test( const size_t position ) const;

        /// @copydoc cbitset64::numBitsSet
        inline size_t numBitsSet( const size_t positionEnd ) const;

        /// @copydoc cbitset64::findFirstBitSet
        inline size_t findFirstBitSet() const;

        /// @copydoc cbitset64::findFirstBitSet
        inline size_t findFirstBitSet( size_t startFrom ) const;

        /// @copydoc cbitset64::findLastBitSetPlusOne
        inline size_t findLastBitSetPlusOne() const;

        /** Same as findLastBitSetPlusOne(); starting from startFrom (exclusive).
        @param startFrom
            Must be in range (0; capacity]
            e.g. if capacity == 5 and we've set 01001b (0 is the 4th bit, 1 is the 0th bit) then:

                findLastBitSetPlusOne( 6 ) = INVALID
                findLastBitSetPlusOne( 5 ) = 4u
                findLastBitSetPlusOne( 4 ) = 4u
                findLastBitSetPlusOne( 3 ) = 1u
                findLastBitSetPlusOne( 2 ) = 1u
                findLastBitSetPlusOne( 1 ) = 1u
                findLastBitSetPlusOne( 0 ) = INVALID

            e.g. if capacity == 5 and we've set 10110b (1 is the 4th bit, 0 is the 0th bit) then:

                findLastBitSetPlusOne( 6 ) = INVALID
                findLastBitSetPlusOne( 5 ) = 5u
                findLastBitSetPlusOne( 4 ) = 3u
                findLastBitSetPlusOne( 3 ) = 3u
                findLastBitSetPlusOne( 2 ) = 2u
                findLastBitSetPlusOne( 1 ) = 0u (not found)
                findLastBitSetPlusOne( 0 ) = INVALID

        @return
            If all bits are unset (i.e. all 0s):
                Returns 0
            If at least one bit is set:
                The index to the last set bit, plus one.
                Return range is [1; startFrom]
        */
        inline size_t findLastBitSetPlusOne( size_t startFrom ) const;

        /** Finds the last bit unset.
        @return
            If all bits are set (i.e. all 1s):
                Returns this->capacity()
            If at least one bit is unset:
                The index to the last set unbit.
                Return range is [0; capacity)
        */
        inline size_t findLastBitUnset() const;

        /** Finds the last bit unset; starting from startFrom (exclusive).
        @param startFrom
            Must be in range (0; capacity]
            e.g. if capacity == 5 and we've set 01001b (0 is the 4th bit, 1 is the 0th bit) then:

                findLastBitUnsetPlusOne( 6 ) = INVALID
                findLastBitUnsetPlusOne( 5 ) = 4u
                findLastBitUnsetPlusOne( 4 ) = 2u
                findLastBitUnsetPlusOne( 3 ) = 2u
                findLastBitUnsetPlusOne( 2 ) = 1u
                findLastBitUnsetPlusOne( 1 ) = 5u (capacity, not found)
                findLastBitUnsetPlusOne( 0 ) = INVALID

        @return
            If all bits are set (i.e. all 1s):
                Returns this->capacity()
            If at least one bit is unset:
                The index to the last set unbit.
                Return range is [0; capacity)
        */
        inline size_t findLastBitUnset( size_t startFrom ) const;
    };
}  // namespace Ogre

#endif
