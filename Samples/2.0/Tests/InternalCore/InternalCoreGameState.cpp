
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

    mGraphicsSystem->setQuit();
}
