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

#ifndef OgreBitset_inl_H_
#define OgreBitset_inl_H_

#include "OgreBitset.h"

#include "OgreBitwise.h"

namespace Ogre
{
#define OGRE_TEMPL_DECL template <size_t _N, typename _internalDataType, size_t _bits, size_t _mask>
#define OGRE_TEMPL_USE _N, _internalDataType, _bits, _mask
    //-------------------------------------------------------------------------
    OGRE_TEMPL_DECL
    bool cbitsetN<OGRE_TEMPL_USE>::empty() const
    {
        bool         bIsEmpty = true;
        const size_t internalArraySize = getInternalArraySize();
        for( size_t i = 0u; i < internalArraySize; ++i )
            bIsEmpty &= mValues[i] == _internalDataType( 0 );
        return bIsEmpty;
    }
    //-------------------------------------------------------------------------
    OGRE_TEMPL_DECL
    void cbitsetN<OGRE_TEMPL_USE>::setAll() { this->setAllUntil( _N ); }
    //-------------------------------------------------------------------------
    OGRE_TEMPL_DECL
    void cbitsetN<OGRE_TEMPL_USE>::setAllUntil( size_t position )
    {
        OGRE_ASSERT_MEDIUM( position < _N );

        const size_t numValuesFullSet = position >> _bits;

        for( size_t i = 0u; i < numValuesFullSet; ++i )
        {
            // Set all values to 0xFFFFF...
            mValues[i] = std::numeric_limits<_internalDataType>::max();
            position -= 1u << _bits;
        }

        // Deal with the remainder
        if( position != 0u )
        {
            OGRE_ASSERT_MEDIUM( numValuesFullSet < getInternalArraySize() );
            mValues[numValuesFullSet] |= ( _internalDataType( 1ul ) << position ) - uint64( 1ul );
            position = 0;
        }
    }
    //-------------------------------------------------------------------------
    OGRE_TEMPL_DECL
    void cbitsetN<OGRE_TEMPL_USE>::setValue( const size_t position, const bool bValue )
    {
        OGRE_ASSERT_MEDIUM( position < _N );
        const size_t            idx = position >> _bits;
        const _internalDataType mask = _internalDataType( 1u ) << ( position & _mask );
        if( bValue )
            mValues[idx] |= mask;
        else
            mValues[idx] &= ~mask;
    }
    //-------------------------------------------------------------------------
    OGRE_TEMPL_DECL
    void cbitsetN<OGRE_TEMPL_USE>::set( const size_t position )
    {
        OGRE_ASSERT_MEDIUM( position < _N );
        const size_t            idx = position >> _bits;
        const _internalDataType mask = _internalDataType( 1u ) << ( position & _mask );
        mValues[idx] |= mask;
    }
    //-------------------------------------------------------------------------
    OGRE_TEMPL_DECL
    void cbitsetN<OGRE_TEMPL_USE>::unset( const size_t position )
    {
        OGRE_ASSERT_MEDIUM( position < _N );
        const size_t            idx = position >> _bits;
        const _internalDataType mask = _internalDataType( 1u ) << ( position & _mask );
        mValues[idx] &= ~mask;
    }  //-------------------------------------------------------------------------
    OGRE_TEMPL_DECL
    bool cbitsetN<OGRE_TEMPL_USE>::test( const size_t position ) const
    {
        OGRE_ASSERT_MEDIUM( position < _N );
        const size_t            idx = position >> _bits;
        const _internalDataType mask = _internalDataType( 1u ) << ( position & _mask );
        return ( mValues[idx] & mask ) != 0u;
    }
    //-------------------------------------------------------------------------
    OGRE_TEMPL_DECL
    size_t cbitsetN<OGRE_TEMPL_USE>::numBitsSet( const size_t positionEnd ) const
    {
        OGRE_ASSERT_MEDIUM( positionEnd <= _N );
        size_t retVal = 0u;
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
                        retVal += ( value & ( _internalDataType( 1u ) << j ) ) != 0u ? 1u : 0u;
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
    //-------------------------------------------------------------------------
    template <size_t _N>
    size_t cbitset64<_N>::findFirstBitSet() const
    {
        const size_t internalArraySize = this->getInternalArraySize();
        for( size_t i = 0; i < internalArraySize; ++i )
        {
            if( this->mValues[i] != 0u )
            {
                const size_t firstBitSet = Bitwise::ctz64( this->mValues[i] );
                return firstBitSet + 64u * i;
            }
        }

        return _N;
    }
    //-------------------------------------------------------------------------
    template <size_t _N>
    size_t cbitset64<_N>::findFirstBitSet( const size_t startFrom ) const
    {
        OGRE_ASSERT_HIGH( startFrom < this->capacity() );

        const size_t internalArraySize = this->getInternalArraySize();
        const size_t internalArrayStart = startFrom >> 6u;

        for( size_t i = internalArrayStart; i < internalArraySize; ++i )
        {
            if( this->mValues[i] != 0u )
            {
                if( i == internalArrayStart )
                {
                    const uint64 localIdx = ( startFrom & 0x3F );
                    const uint64 mask = ~( ( uint64( 1ul ) << localIdx ) - 1ul );

                    const uint64 value = this->mValues[i] & mask;

                    if( value != 0u )
                    {
                        const size_t firstBitSet = Bitwise::ctz64( value );
                        return firstBitSet + 64u * i;
                    }
                }
                else
                {
                    const size_t firstBitSet = Bitwise::ctz64( this->mValues[i] );
                    return firstBitSet + 64u * i;
                }
            }
        }

        return _N;
    }
    //-------------------------------------------------------------------------
    template <size_t _N>
    size_t cbitset64<_N>::findLastBitSetPlusOne() const
    {
        const size_t internalArraySize = this->getInternalArraySize();
        for( size_t i = internalArraySize; i--; )
        {
            if( this->mValues[i] != 0u )
            {
                const size_t lastBitSet = 64u - Bitwise::clz64( this->mValues[i] );
                return lastBitSet + 64u * i;
            }
        }

        return 0u;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------

    bool bitset64::empty() const
    {
        bool         bIsEmpty = true;
        const size_t internalArraySize = mValues.size();
        for( size_t i = 0u; i < internalArraySize; ++i )
            bIsEmpty &= mValues[i] == uint64( 0 );
        return bIsEmpty;
    }
    //-------------------------------------------------------------------------
    void bitset64::setAll() { this->setAllUntil( mBitsCapacity ); }
    //-------------------------------------------------------------------------
    void bitset64::setAllUntil( size_t position )
    {
        OGRE_ASSERT_MEDIUM( position < mBitsCapacity );

        const size_t numValuesFullSet = position >> 6u;

        for( size_t i = 0u; i < numValuesFullSet; ++i )
        {
            // Set all values to 0xFFFFF...
            mValues[i] = std::numeric_limits<uint64>::max();
            position -= 1u << 6u;
        }

        // Deal with the remainder
        if( position != 0u )
        {
            OGRE_ASSERT_MEDIUM( numValuesFullSet < mValues.size() );
            mValues[numValuesFullSet] |= ( uint64( 1ul ) << position ) - uint64( 1ul );
            position = 0;
        }
    }
    //-------------------------------------------------------------------------
    void bitset64::setValue( const size_t position, const bool bValue )
    {
        OGRE_ASSERT_MEDIUM( position < mBitsCapacity );
        const size_t idx = position >> 6u;
        const uint64 mask = uint64( 1u ) << ( position & 63u );
        if( bValue )
            mValues[idx] |= mask;
        else
            mValues[idx] &= ~mask;
    }
    //-------------------------------------------------------------------------
    void bitset64::set( const size_t position )
    {
        OGRE_ASSERT_MEDIUM( position < mBitsCapacity );
        const size_t idx = position >> 6u;
        const uint64 mask = uint64( 1u ) << ( position & 63u );
        mValues[idx] |= mask;
    }
    //-------------------------------------------------------------------------
    void bitset64::unset( const size_t position )
    {
        OGRE_ASSERT_MEDIUM( position < mBitsCapacity );
        const size_t idx = position >> 6u;
        const uint64 mask = uint64( 1u ) << ( position & 63u );
        mValues[idx] &= ~mask;
    }  //-------------------------------------------------------------------------
    bool bitset64::test( const size_t position ) const
    {
        OGRE_ASSERT_MEDIUM( position < mBitsCapacity );
        const size_t idx = position >> 6u;
        const uint64 mask = uint64( 1u ) << ( position & 63u );
        return ( mValues[idx] & mask ) != 0u;
    }
    //-------------------------------------------------------------------------
    size_t bitset64::numBitsSet( const size_t positionEnd ) const
    {
        OGRE_ASSERT_MEDIUM( positionEnd < mBitsCapacity );
        size_t retVal = 0u;
        for( size_t i = 0u; i < positionEnd; )
        {
            if( ( positionEnd - i ) >= 63u )
            {
                const uint64 value = mValues[i >> 6u];
                if( value == (uint64)-1 )
                {
                    retVal += 63u + 1u;
                }
                else if( value != 0u )
                {
                    for( size_t j = 0u; j <= 63u; ++j )
                        retVal += ( value & ( uint64( 1u ) << j ) ) != 0u ? 1u : 0u;
                }

                i += 63u + 1u;
            }
            else
            {
                retVal += test( i ) ? 1u : 0u;
                ++i;
            }
        }
        return retVal;
    }
    //-------------------------------------------------------------------------
    size_t bitset64::findFirstBitSet() const
    {
        const size_t internalArraySize = this->mValues.size();
        for( size_t i = 0; i < internalArraySize; ++i )
        {
            if( this->mValues[i] != 0u )
            {
                const size_t firstBitSet = Bitwise::ctz64( this->mValues[i] );
                return firstBitSet + 64u * i;
            }
        }

        return mBitsCapacity;
    }
    //-------------------------------------------------------------------------
    size_t bitset64::findFirstBitSet( const size_t startFrom ) const
    {
        OGRE_ASSERT_HIGH( startFrom < this->capacity() );

        const size_t internalArraySize = this->mValues.size();
        const size_t internalArrayStart = startFrom >> 6u;

        for( size_t i = internalArrayStart; i < internalArraySize; ++i )
        {
            if( this->mValues[i] != 0u )
            {
                if( i == internalArrayStart )
                {
                    const uint64 localIdx = ( startFrom & 0x3F );
                    const uint64 mask = ~( ( uint64( 1ul ) << localIdx ) - 1ul );

                    const uint64 value = this->mValues[i] & mask;

                    if( value != 0u )
                    {
                        const size_t firstBitSet = Bitwise::ctz64( value );
                        return firstBitSet + 64u * i;
                    }
                }
                else
                {
                    const size_t firstBitSet = Bitwise::ctz64( this->mValues[i] );
                    return firstBitSet + 64u * i;
                }
            }
        }

        return mBitsCapacity;
    }
    //-------------------------------------------------------------------------
    size_t bitset64::findLastBitSetPlusOne() const
    {
        const size_t internalArraySize = this->mValues.size();
        for( size_t i = internalArraySize; i--; )
        {
            if( this->mValues[i] != 0u )
            {
                const size_t lastBitSet = 64u - Bitwise::clz64( this->mValues[i] );
                return lastBitSet + 64u * i;
            }
        }

        return 0u;
    }
    //-------------------------------------------------------------------------
    size_t bitset64::findLastBitSetPlusOne( const size_t startFrom ) const
    {
        OGRE_ASSERT_HIGH( startFrom <= this->capacity() );
        OGRE_ASSERT_HIGH( startFrom > 0u );
        const size_t internalArraySize = ( startFrom + 63u ) >> 6u;  // ( startFrom + 63u ) / 64u;
        OGRE_ASSERT_HIGH( internalArraySize <= this->mValues.size() );

        for( size_t i = internalArraySize; i--; )
        {
            if( this->mValues[i] != 0u )
            {
                if( i + 1u == internalArraySize )
                {
                    const uint64 localIdx = ( startFrom & 0x3F );
                    const uint64 mask = 0xFFFFFFFFFFFFFFFF >> ( 64ul - localIdx );

                    const uint64 value = this->mValues[i] & mask;
                    if( value != 0u )
                    {
                        const size_t lastBitSet = 64u - Bitwise::clz64( value );
                        return lastBitSet + 64u * i;
                    }
                }
                else
                {
                    const size_t lastBitSet = 64u - Bitwise::clz64( this->mValues[i] );
                    return lastBitSet + 64u * i;
                }
            }
        }

        return 0u;
    }
    //-------------------------------------------------------------------------
    size_t bitset64::findLastBitUnset() const { return findLastBitUnset( mBitsCapacity ); }
    //-------------------------------------------------------------------------
    size_t bitset64::findLastBitUnset( const size_t startFrom ) const
    {
        OGRE_ASSERT_HIGH( startFrom <= this->capacity() );
        OGRE_ASSERT_HIGH( startFrom > 0u );
        const size_t internalArraySize = ( startFrom + 63u ) >> 6u;  // ( startFrom + 63u ) / 64u;
        OGRE_ASSERT_HIGH( internalArraySize <= this->mValues.size() );
        for( size_t i = internalArraySize; i--; )
        {
            if( this->mValues[i] != 0xFFFFFFFFFFFFFFFF )
            {
                if( i + 1u == internalArraySize )
                {
                    const uint64 localIdx = ( startFrom & 0x3F );
                    const uint64 mask = 0xFFFFFFFFFFFFFFFF >> ( 64ul - localIdx );

                    const uint64 value = ( this->mValues[i] ^ 0xFFFFFFFFFFFFFFFF ) & mask;
                    if( value != 0u )
                    {
                        const size_t lastBitUnset = 63u - Bitwise::clz64( value );
                        return lastBitUnset + 64u * i;
                    }
                }
                else
                {
                    const uint64 value = this->mValues[i] ^ 0xFFFFFFFFFFFFFFFF;
                    const size_t lastBitUnset = 63u - Bitwise::clz64( value );
                    return lastBitUnset + 64u * i;
                }
            }
        }

        return mBitsCapacity;
    }
};  // namespace Ogre

#undef OGRE_TEMPL_DECL
#undef OGRE_TEMPL_USE

#endif
