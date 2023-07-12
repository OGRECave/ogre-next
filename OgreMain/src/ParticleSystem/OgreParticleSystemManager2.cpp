/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2023 Torus Knot Software Ltd

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

#include "ParticleSystem/OgreParticleSystemManager2.h"

#include "Math/Array/OgreArrayConfig.h"
#include "Math/Array/OgreBooleanMask.h"
#include "OgreSceneManager.h"
#include "ParticleSystem/OgreEmitter2.h"
#include "ParticleSystem/OgreParticle2.h"
#include "ParticleSystem/OgreParticleAffector2.h"
#include "ParticleSystem/OgreParticleSystem2.h"

using namespace Ogre;

static std::map<IdString, ParticleEmitterDefDataFactory *> sEmitterDefFactories;

ParticleSystemManager2::ParticleSystemManager2( SceneManager *sceneManager ) :
    mSceneManager( sceneManager ),
    mTimeSinceLast( 0 )
{
}
//-----------------------------------------------------------------------------
ParticleSystemManager2::~ParticleSystemManager2()
{
    for( std::pair<IdString, ParticleSystemDef *> itor : mParticleSystemDefMap )
        delete itor.second;

    mActiveParticleSystemDefs.clear();
    mParticleSystemDefMap.clear();
}
//-----------------------------------------------------------------------------
void ParticleSystemManager2::tickParticles( const size_t threadIdx, const Real _timeSinceLast,
                                            ParticleCpuData cpuData, const size_t numParticles,
                                            ParticleSystemDef *systemDef )
{
    const ArrayReal timeSinceLast = Mathlib::SetAll( _timeSinceLast );

    const ArrayReal invPi = Mathlib::SetAll( 1.0f / Math::PI );

    ParticleGpuData *gpuData = 0;

    for( size_t i = 0u; i < numParticles; i += ARRAY_PACKED_REALS )
    {
        *cpuData.mPosition += *cpuData.mDirection * timeSinceLast;
        *cpuData.mTimeToLive -= timeSinceLast;

        const ArrayMaskR wasAlive = Mathlib::CompareGreater( *cpuData.mTimeToLive, ARRAY_REAL_ZERO );

        *cpuData.mTimeToLive = Mathlib::Max( *cpuData.mTimeToLive, ARRAY_REAL_ZERO );

        const ArrayMaskR isDead = Mathlib::CompareLessEqual( *cpuData.mTimeToLive, ARRAY_REAL_ZERO );
        const uint32 scalarMask = BooleanMask4::getScalarMask( Mathlib::And( wasAlive, isDead ) );
        const uint32 scalarIsDead = BooleanMask4::getScalarMask( isDead );

        // ArrayReal normalizedRotation = Mathlib::Saturate(  );
        const ArrayVector3 normDir = cpuData.mDirection->normalisedCopy();

        int16 directions[3][ARRAY_PACKED_REALS];
        int16 rotations[ARRAY_PACKED_REALS];
        Mathlib::extractS16( Mathlib::ToSnorm16( normDir.mChunkBase[0] ), directions[0] );
        Mathlib::extractS16( Mathlib::ToSnorm16( normDir.mChunkBase[1] ), directions[1] );
        Mathlib::extractS16( Mathlib::ToSnorm16( normDir.mChunkBase[2] ), directions[2] );
        Mathlib::extractS16( Mathlib::ToSnorm16( cpuData.mRotation->valueRadians() * invPi ),
                             rotations );

        for( size_t j = 0; j < ARRAY_PACKED_REALS; ++j )
        {
            if( IS_BIT_SET( j, scalarMask ) )
            {
                systemDef->mParticlesToKill[threadIdx].push_back( systemDef->getHandle( cpuData, j ) );
                // Should we use NaN? GPU is supposed to reject them faster.
                gpuData->mWidth = 0.0f;
                gpuData->mHeight = 0.0f;
                gpuData->mPos[0] = gpuData->mPos[1] = gpuData->mPos[2] = 0.0f;
                gpuData->mDirection[0] = gpuData->mDirection[1] = gpuData->mDirection[2] = 0.0f;
                gpuData->mRotation = 0u;
                gpuData->mColour[0] = gpuData->mColour[1] = gpuData->mColour[2] = gpuData->mColour[3] =
                    0u;
            }
            else if( IS_BIT_SET( j, scalarIsDead ) )
            {
                Vector3 pos;
                cpuData.mPosition->getAsVector3( pos, j );
                Vector2 dim;
                cpuData.mDimensions->getAsVector2( dim, j );
                gpuData->mWidth = static_cast<float>( dim.x );
                gpuData->mHeight = static_cast<float>( dim.y );
                gpuData->mPos[0] = static_cast<float>( pos.x );
                gpuData->mPos[1] = static_cast<float>( pos.y );
                gpuData->mPos[2] = static_cast<float>( pos.z );
                gpuData->mColourScale = 1.0f;
                gpuData->mDirection[0] = directions[0][j];
                gpuData->mDirection[1] = directions[1][j];
                gpuData->mDirection[2] = directions[2][j];
                gpuData->mRotation = rotations[j];
                gpuData->mColour[0] = 255u;
                gpuData->mColour[1] = 255u;
                gpuData->mColour[2] = 255u;
                gpuData->mColour[3] = 255u;
            }

            ++gpuData;
        }

        cpuData.advancePack();
    }
}
//-----------------------------------------------------------------------------
void ParticleSystemManager2::updateSerialPre( const Real timeSinceLast )
{
    for( ParticleSystemDef *systemDef : mActiveParticleSystemDefs )
    {
        const size_t numEmitters = systemDef->mEmitters.size();
        systemDef->mNewParticles.clear();

        for( ParticleSystem2 *system : systemDef->mActiveParticleSystems )
        {
            for( size_t i = 0u; i < numEmitters; ++i )
            {
                const uint32 numRequestedParticles = systemDef->mEmitters[i]->genEmissionCount(
                    timeSinceLast, system->mEmitterInstanceData[i] );
                system->mNewParticlesPerEmitter[i] = numRequestedParticles;

                for( uint32 j = 0u; j < numRequestedParticles; ++j )
                {
                    const uint32 handle = systemDef->allocParticle();
                    if( handle != ParticleSystemDef::InvalidHandle )
                    {
                        system->mParticleHandles.push_back( handle );
                        systemDef->mNewParticles.push_back( handle );
                    }
                    else
                    {
                        // The pool run out of particles.
                        // It won't be handling more while in updateSerial()
                        system->mNewParticlesPerEmitter[i] = j;
                        break;
                    }
                }
            }
        }
    }
}
//-----------------------------------------------------------------------------
void ParticleSystemManager2::updateSerialPos()
{
    for( ParticleSystemDef *systemDef : mActiveParticleSystemDefs )
    {
        for( FastArray<uint32> &threadParticlesToKill : systemDef->mParticlesToKill )
        {
            for( const uint32 handle : threadParticlesToKill )
                systemDef->deallocParticle( handle );
            threadParticlesToKill.clear();
        }
    }
}
//-----------------------------------------------------------------------------
void ParticleSystemManager2::updateParallel( const size_t threadIdx, const size_t numThreads )
{
    const Real timeSinceLast = mTimeSinceLast;

    for( ParticleSystemDef *systemDef : mActiveParticleSystemDefs )
    {
        const size_t numEmitters = systemDef->mEmitters.size();

        // We split particle systems
        size_t currOffset = 0u;

        ParticleCpuData cpuData = systemDef->getParticleCpuData();

        for( const ParticleSystem2 *system : systemDef->mActiveParticleSystems )
        {
            for( size_t i = 0u; i < numEmitters; ++i )
            {
                const size_t newParticlesPerEmitter = system->mNewParticlesPerEmitter[i];

                const size_t particlesPerThread =
                    ( newParticlesPerEmitter + numThreads - 1u ) / numThreads;

                const size_t toAdvance =
                    std::min( threadIdx * particlesPerThread, newParticlesPerEmitter );
                const size_t numParticlesToProcess =
                    std::min( particlesPerThread, newParticlesPerEmitter - toAdvance );

                systemDef->mEmitters[i]->initEmittedParticles(
                    cpuData, systemDef->mNewParticles.begin() + currOffset + toAdvance,
                    numParticlesToProcess );

                // We've processed numParticlesToProcess but we need to skip newParticlesPerEmitter
                // because the gap "newParticlesPerEmitter - numParticlesToProcess" is being
                // processed by other threads.
                //
                // We need to move on to the next set of particles requested by the next
                // emitter during updateSerial().
                currOffset += newParticlesPerEmitter;
            }
        }

        cpuData.advancePack( systemDef->getActiveParticlesPackOffset() );
        const size_t numParticles = systemDef->getNumActiveParticles();

        // particlesPerThread must be multiple of ARRAY_PACKED_REALS
        size_t particlesPerThread = ( numParticles + numThreads - 1u ) / numThreads;
        particlesPerThread = ( ( particlesPerThread + ARRAY_PACKED_REALS - 1 ) / ARRAY_PACKED_REALS ) *
                             ARRAY_PACKED_REALS;

        const size_t toAdvance = std::min( threadIdx * particlesPerThread, numParticles );
        const size_t numParticlesToProcess = std::min( particlesPerThread, numParticles - toAdvance );

        cpuData.advancePack( toAdvance );

        for( const Affector *affector : systemDef->mAffectors )
            affector->run( cpuData, numParticlesToProcess );

        tickParticles( threadIdx, timeSinceLast, cpuData, numParticles, systemDef );
    }
}
//-----------------------------------------------------------------------------
void ParticleSystemManager2::addEmitterFactory( ParticleEmitterDefDataFactory *factory )
{
    sEmitterDefFactories[factory->getName()] = factory;
}
//-----------------------------------------------------------------------------
void ParticleSystemManager2::removeEmitterFactory( ParticleEmitterDefDataFactory *factory )
{
    sEmitterDefFactories.erase( factory->getName() );
}
//-----------------------------------------------------------------------------
ParticleEmitterDefDataFactory *ParticleSystemManager2::getFactory( IdString name )
{
    std::map<IdString, ParticleEmitterDefDataFactory *>::const_iterator itor =
        sEmitterDefFactories.find( name );
    if( itor == sEmitterDefFactories.end() )
    {
        OGRE_EXCEPT(
            Exception::ERR_ITEM_NOT_FOUND,
            "No emitter with name '" + name.getFriendlyText() + "' found. Is the plugin installed?",
            "ParticleSystemManager2::getFactory" );
    }
    return itor->second;
}
//-----------------------------------------------------------------------------
ParticleSystemDef *ParticleSystemManager2::createParticleSystemDef( const String &name )
{
    const IdString nameHash = name;

    auto insertResult = mParticleSystemDefMap.insert( { nameHash, nullptr } );
    if( !insertResult.second )
    {
        OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                     "Particle System Definition '" + name + "' already exists.",
                     "ParticleSystemManager2::createParticleSystemDef" );
    }

    std::map<IdString, ParticleSystemDef *>::iterator itor = insertResult.first;

    itor->second = new ParticleSystemDef( mSceneManager, this, name );
    return itor->second;
}
//-----------------------------------------------------------------------------
ParticleSystemDef *ParticleSystemManager2::getParticleSystemDef( const String &name )
{
    std::map<IdString, ParticleSystemDef *>::const_iterator itor = mParticleSystemDefMap.find( name );
    if( itor == mParticleSystemDefMap.end() )
    {
        OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                     "Particle System Definition '" + name + "' not found.",
                     "ParticleSystemManager2::getParticleSystemDef" );
    }
    return itor->second;
}
//-----------------------------------------------------------------------------
void ParticleSystemManager2::destroyAllParticleSystems()
{
    for( std::pair<IdString, ParticleSystemDef *> itor : mParticleSystemDefMap )
        itor.second->_destroyAllParticleSystems();
    mActiveParticleSystemDefs.clear();
}
//-----------------------------------------------------------------------------
void ParticleSystemManager2::update( const Real timeSinceLast )
{
    mTimeSinceLast = timeSinceLast;

    updateSerialPre( timeSinceLast );
    updateParallel( 0u, 1u );
    updateSerialPos();
}
//-----------------------------------------------------------------------------
void ParticleSystemManager2::_addParticleSystemDefAsActive( ParticleSystemDef *def )
{
    def->mGlobalIndex = mActiveParticleSystemDefs.size();
    mActiveParticleSystemDefs.push_back( def );
}
//-----------------------------------------------------------------------------
void ParticleSystemManager2::_removeParticleSystemDefFromActive( ParticleSystemDef *def )
{
    OGRE_ASSERT_MEDIUM(
        def->mGlobalIndex <= mActiveParticleSystemDefs.size() &&
        def == *( mActiveParticleSystemDefs.begin() + static_cast<ptrdiff_t>( def->mGlobalIndex ) ) );

    FastArray<ParticleSystemDef *>::iterator itor =
        mActiveParticleSystemDefs.begin() + static_cast<ptrdiff_t>( def->mGlobalIndex );
    itor = efficientVectorRemove( mActiveParticleSystemDefs, itor );

    // The node that was at the end got swapped and has now a different index
    if( itor != mActiveParticleSystemDefs.end() )
        ( *itor )->mGlobalIndex = static_cast<size_t>( itor - mActiveParticleSystemDefs.begin() );

    def->mGlobalIndex = mActiveParticleSystemDefs.size();
}
