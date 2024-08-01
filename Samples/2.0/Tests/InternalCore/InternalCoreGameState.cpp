
#include "InternalCoreGameState.h"

#include "GraphicsSystem.h"

#include "Math/Array/OgreArrayVector3.h"

using namespace Demo;

InternalCoreGameState::InternalCoreGameState( const Ogre::String &helpDescription ) :
    TutorialGameState( helpDescription )
{
}
//-----------------------------------------------------------------------------------
void InternalCoreGameState::createScene01()
{
    TutorialGameState::createScene01();

    using namespace Ogre;

    Vector3 scalarA[ARRAY_PACKED_REALS];
    Vector3 minCollapsed( std::numeric_limits<Real>::max() );
    Vector3 maxCollapsed( -std::numeric_limits<Real>::max() );
    ArrayVector3 packedVec;
    for( int i = 0; i < ARRAY_PACKED_REALS; ++i )
    {
        scalarA[i].x = Real( i * 3 - ARRAY_PACKED_REALS ) * 17.67f;
        scalarA[i].y = Real( i * 3 + 1 - ARRAY_PACKED_REALS ) * 31.11f;
        scalarA[i].z = Real( i * 3 + 2 - ARRAY_PACKED_REALS ) * 5.84f;

        minCollapsed.makeFloor( scalarA[i] );
        maxCollapsed.makeCeil( scalarA[i] );

        packedVec.setFromVector3( scalarA[i], (size_t)i );
    }

    for( int i = 0; i < ARRAY_PACKED_REALS; ++i )
    {
        Vector3 output;
        packedVec.getAsVector3( output, (size_t)i );
        OGRE_ASSERT( output == scalarA[i] );
        if( (size_t)i != ARRAY_PACKED_REALS - (size_t)i - 1u )
        {
            packedVec.getAsVector3( output, ARRAY_PACKED_REALS - (size_t)i - 1u );
            OGRE_ASSERT( output != scalarA[i] );
        }
    }

    {
        const Vector3 collapsedMinToTest = packedVec.collapseMin();
        const Vector3 collapsedMaxToTest = packedVec.collapseMax();
        OGRE_ASSERT( collapsedMinToTest == minCollapsed );
        OGRE_ASSERT( collapsedMaxToTest == maxCollapsed );
    }

    for( int i = -128; i < 128; ++i )
    {
        Real vals[ARRAY_PACKED_REALS];
        ArrayReal arrayVal = ARRAY_REAL_ZERO;
        for( int j = 0; j < ARRAY_PACKED_REALS; ++j )
        {
            vals[j] = Real( i ) + Real( j ) * 3.0f;
            if( vals[j] >= 128.0f )
                vals[j] -= 255.0f;

            Mathlib::Set( arrayVal, vals[j], (size_t)j );
        }

        OGRE_ASSERT( Mathlib::Get0( arrayVal ) == vals[0] );

        arrayVal = arrayVal / 127.0f;

        int8 outValues[ARRAY_PACKED_REALS];
        Mathlib::extractS8( Mathlib::ToSnorm8Unsafe( arrayVal ), outValues );

        for( int j = 0; j < ARRAY_PACKED_REALS; ++j )
        {
            OGRE_ASSERT( outValues[j] == (int8)vals[j] ||  //
                         ( outValues[j] == -127 && (int8)vals[j] == -128 ) ||
                         ( outValues[j] == -128 && (int8)vals[j] == -127 ) );
        }
    }

    for( int i = -32768; i < 32767; ++i )
    {
        Real vals[ARRAY_PACKED_REALS];
        ArrayReal arrayVal = ARRAY_REAL_ZERO;
        for( int j = 0; j < ARRAY_PACKED_REALS; ++j )
        {
            vals[j] = Real( i ) + Real( j ) * 3.0f;
            if( vals[j] >= 32768.0f )
                vals[j] -= 65535.0f;

            Mathlib::Set( arrayVal, vals[j], (size_t)j );
        }

        OGRE_ASSERT( Mathlib::Get0( arrayVal ) == vals[0] );

        arrayVal = arrayVal / 32767.0f;

        int16 outValues[ARRAY_PACKED_REALS];
        Mathlib::extractS16( Mathlib::ToSnorm16( arrayVal ), outValues );

        for( int j = 0; j < ARRAY_PACKED_REALS; ++j )
        {
            OGRE_ASSERT( ( outValues[j] - (int16)vals[j] ) <= 1 ||  //
                         ( outValues[j] == -32767 && (int16)vals[j] == -32768 ) ||
                         ( outValues[j] == -32768 && (int16)vals[j] == -32767 ) );
        }
    }

    mGraphicsSystem->setQuit();
}
