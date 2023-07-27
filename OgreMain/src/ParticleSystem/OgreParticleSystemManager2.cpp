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
#include "OgreRenderQueue.h"
#include "OgreSceneManager.h"
#include "ParticleSystem/OgreEmitter2.h"
#include "ParticleSystem/OgreParticle2.h"
#include "ParticleSystem/OgreParticleAffector2.h"
#include "ParticleSystem/OgreParticleSystem2.h"
#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreReadOnlyBufferPacked.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

using namespace Ogre;

static std::map<IdString, ParticleAffectorFactory2 *> sAffectorFactories;
static std::map<IdString, ParticleEmitterDefDataFactory *> sEmitterDefFactories;

ParticleSystemManager2::ParticleSystemManager2( SceneManager *sceneManager ) :
    mSceneManager( sceneManager ),
    mSharedIndexBuffer16( 0 ),
    mSharedIndexBuffer32( 0 ),
    mHighestPossibleQuota16( 0u ),
    mHighestPossibleQuota32( 0u ),
    mTimeSinceLast( 0 )
{
}
//-----------------------------------------------------------------------------
ParticleSystemManager2::~ParticleSystemManager2()
{
    VaoManager *vaoManager = mSceneManager->getDestinationRenderSystem()->getVaoManager();

    for( std::pair<IdString, ParticleSystemDef *> itor : mParticleSystemDefMap )
    {
        itor.second->_destroy( vaoManager );
        delete itor.second;
    }

    mActiveParticleSystemDefs.clear();
    mParticleSystemDefMap.clear();
}
//-----------------------------------------------------------------------------
void ParticleSystemManager2::tickParticles( const size_t threadIdx, const ArrayReal timeSinceLast,
                                            ParticleCpuData cpuData, ParticleGpuData *gpuData,
                                            const size_t numParticles, ParticleSystemDef *systemDef )
{
    const ArrayReal invPi = Mathlib::SetAll( 1.0f / Math::PI );

    // To support HDR, particles are encoded as SNORM16.
    // Although rare, it is possible some particles may have negative colour to subtract instead of add.
    // To support the most common cases, we support sending the range [-4; 120] while still
    // having full 8-bit precision in the range [0; 1].
    //
    // We chose 124 because 32767 is a multiple of it, which causes the value 0 to map exactly to 0.
    // However the value 1.0 maps to sint16 1321, which after decoding maps to 0.999053926206244
    // This should be good enough for SDR (since 254/255 = 0.996078431)
    //
    // If you need a larger range, use material diffuse multipliers.
    const Real kColourRange = 124.0f;
    const Real kMinColourValue = -4.0f;

    const ArrayReal colourRange = Mathlib::SetAll( 1.0f / kColourRange );
    const ArrayReal minColorValue = Mathlib::SetAll( kMinColourValue / kColourRange );
    const ArrayReal alphaScale = Mathlib::SetAll( 2.0f );
    const ArrayReal alphaOffset = Mathlib::SetAll( -1.0f );

    for( size_t i = 0u; i < numParticles; i += ARRAY_PACKED_REALS )
    {
        *cpuData.mPosition += *cpuData.mDirection * timeSinceLast;

        const ArrayMaskR wasDead = Mathlib::CompareLessEqual( *cpuData.mTimeToLive, ARRAY_REAL_ZERO );
        *cpuData.mTimeToLive -= timeSinceLast;
        *cpuData.mTimeToLive = Mathlib::Max( *cpuData.mTimeToLive, ARRAY_REAL_ZERO );
        const ArrayMaskR isDead = Mathlib::CompareLessEqual( *cpuData.mTimeToLive, ARRAY_REAL_ZERO );

        const uint32 scalarWasDead = BooleanMask4::getScalarMask( wasDead );
        const uint32 scalarIsDead = BooleanMask4::getScalarMask( isDead );

        const ArrayVector3 normDir = cpuData.mDirection->normalisedCopy();

        ArrayVector4 vColour = *cpuData.mColour;
        vColour.fma3x1( colourRange, minColorValue, alphaScale, alphaOffset );

        int8 directions[3][ARRAY_PACKED_REALS];
        int16 rotations[ARRAY_PACKED_REALS];
        int16 colour[3][ARRAY_PACKED_REALS];
        int8 alpha[ARRAY_PACKED_REALS];
        Mathlib::extractS8( Mathlib::ToSnorm8Unsafe( normDir.mChunkBase[0] ), directions[0] );
        Mathlib::extractS8( Mathlib::ToSnorm8Unsafe( normDir.mChunkBase[1] ), directions[1] );
        Mathlib::extractS8( Mathlib::ToSnorm8Unsafe( normDir.mChunkBase[2] ), directions[2] );
        Mathlib::extractS16( Mathlib::ToSnorm16( cpuData.mRotation->valueRadians() * invPi ),
                             rotations );
        Mathlib::extractS16( Mathlib::ToSnorm16( vColour.mChunkBase[0] ), colour[0] );
        Mathlib::extractS16( Mathlib::ToSnorm16( vColour.mChunkBase[1] ), colour[1] );
        Mathlib::extractS16( Mathlib::ToSnorm16( vColour.mChunkBase[2] ), colour[2] );
        Mathlib::extractS8( Mathlib::ToSnorm8Unsafe( vColour.mChunkBase[3] ), alpha );

        for( size_t j = 0; j < ARRAY_PACKED_REALS; ++j )
        {
            if( IS_BIT_SET( j, scalarIsDead ) )
            {
                if( !IS_BIT_SET( j, scalarWasDead ) )
                {
                    systemDef->mParticlesToKill[threadIdx].push_back(
                        systemDef->getHandle( cpuData, j ) );
                }
                // Should we use NaN? GPU is supposed to reject them faster.
                gpuData->mWidth = 0.0f;
                gpuData->mHeight = 0.0f;
                gpuData->mPos[0] = gpuData->mPos[1] = gpuData->mPos[2] = 0.0f;
                gpuData->mDirection[0] = gpuData->mDirection[1] = gpuData->mDirection[2] = 0;
                gpuData->mColourAlpha = 0;
                gpuData->mRotation = 0;
                gpuData->mColourRgb[0] = gpuData->mColourRgb[1] = gpuData->mColourRgb[2] = 0;
            }
            else
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
                gpuData->mDirection[0] = directions[0][j];
                gpuData->mDirection[1] = directions[1][j];
                gpuData->mDirection[2] = directions[2][j];
                gpuData->mColourAlpha = alpha[j];
                gpuData->mRotation = rotations[j];
                gpuData->mColourRgb[0] = colour[0][j];
                gpuData->mColourRgb[1] = colour[1][j];
                gpuData->mColourRgb[2] = colour[2][j];
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

            const size_t numSimdActiveParticles = systemDef->getNumSimdActiveParticles();
            if( numSimdActiveParticles > 0u )
            {
                systemDef->mParticleGpuData = reinterpret_cast<ParticleGpuData *>(
                    systemDef->mGpuData->map( 0u, systemDef->getNumSimdActiveParticles() ) );
            }

            systemDef->mVaoPerLod[0].back()->setPrimitiveRange(
                0u, static_cast<uint32>( numSimdActiveParticles * 6u ) );
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

        if( systemDef->mParticleGpuData )
        {
            systemDef->mGpuData->unmap( UO_KEEP_PERSISTENT );
            systemDef->mParticleGpuData = 0;
        }
    }
}
//-----------------------------------------------------------------------------
void ParticleSystemManager2::updateParallel( const size_t threadIdx, const size_t numThreads )
{
    const ArrayReal timeSinceLast = Mathlib::SetAll( mTimeSinceLast );

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

                for( const ParticleAffector2 *affector : systemDef->mInitializableAffectors )
                {
                    affector->initEmittedParticles(
                        cpuData, systemDef->mNewParticles.begin() + currOffset + toAdvance,
                        numParticlesToProcess );
                }

                // We've processed numParticlesToProcess but we need to skip newParticlesPerEmitter
                // because the gap "newParticlesPerEmitter - numParticlesToProcess" is being
                // processed by other threads.
                //
                // We need to move on to the next set of particles requested by the next
                // emitter during updateSerial().
                currOffset += newParticlesPerEmitter;
            }
        }

        // We have the following guarantees:
        //      1. systemDef->mFirstParticleIdx is in range [0; quota)
        //      2. systemDef->mLastParticleIdx is in range [0; quota * 2)
        //      3. mLastParticleIdx - mFirstParticleIdx <= quota
        //
        // We need to handle the following:
        //      1. Quota = 100, numThreads = 4, ARRAY_PACKED_REALS = 4
        //      2. systemDef->getActiveParticlesPackOffset() = int(75 / 4) = 18 => 18 * 4 = 72
        //      3. systemDef->getNumSimdActiveParticles() = 52
        //
        // This means we need to iterate the range [72; 100) and then the range [0; 24)
        //  Threads should split the work and handle:
        //      1. Thread A: [72; 88)
        //      2. Thread B: [88; 100) and [0; 4)
        //      3. Thread C: [4; 20)
        //      4. Thread D: [20; 24)
        //
        // Same example but ARRAY_PACKED_REALS = 1
        //      1. Thread A: [72; 85)
        //      2. Thread B: [85; 98)
        //      3. Thread C: [98; 100) [0; 11)
        //      4. Thread D: [11; 24)
        const size_t numSimdActiveParticles = systemDef->getNumSimdActiveParticles();

        // particlesPerThread must be multiple of ARRAY_PACKED_REALS
        size_t particlesPerThread = ( numSimdActiveParticles + numThreads - 1u ) / numThreads;
        particlesPerThread = ( ( particlesPerThread + ARRAY_PACKED_REALS - 1 ) / ARRAY_PACKED_REALS ) *
                             ARRAY_PACKED_REALS;

        const size_t quota = systemDef->getQuota();
        size_t threadAdvance = std::min( threadIdx * particlesPerThread, numSimdActiveParticles );
        size_t totalThreadNumParticlesToProcess =
            std::min( particlesPerThread, numSimdActiveParticles - threadAdvance );
        size_t gpuAdvance = threadAdvance;

        OGRE_ASSERT_MEDIUM( quota % ARRAY_PACKED_REALS == 0u );
        OGRE_ASSERT_MEDIUM( threadAdvance % ARRAY_PACKED_REALS == 0u );
        OGRE_ASSERT_MEDIUM( totalThreadNumParticlesToProcess % ARRAY_PACKED_REALS == 0u );

        // systemDef->getActiveParticlesPackOffset() * APR must be < quota
        // threadAdvance (so far) & gpuAdvance must be < quota
        OGRE_ASSERT_MEDIUM( threadAdvance <= quota );
        // threadAdvance can now be >= quota
        threadAdvance += systemDef->getActiveParticlesPackOffset() * ARRAY_PACKED_REALS;

        for( int i = 0; i < 2; ++i )
        {
            // maxParticle can be >= quota
            const size_t maxParticle = totalThreadNumParticlesToProcess + threadAdvance;
            // particleExcess is the amount of particles we can't
            // process this iteration and will do on the next one.
            const size_t particleExcess = maxParticle - std::min( maxParticle, quota );

            // If threadAdvance > quota, then particleExcess > totalThreadNumParticlesToProcess
            OGRE_ASSERT_MEDIUM(
                ( threadAdvance <= quota && totalThreadNumParticlesToProcess >= particleExcess ) ||
                ( threadAdvance > quota && totalThreadNumParticlesToProcess <= particleExcess ) );

            const size_t numParticlesToProcess =
                std::max( totalThreadNumParticlesToProcess, particleExcess ) - particleExcess;

            // This can advance out of bounds. But if so, then numParticlesToProcess == 0
            OGRE_ASSERT_MEDIUM( threadAdvance <= quota || numParticlesToProcess == 0u );
            cpuData.advancePack( threadAdvance / ARRAY_PACKED_REALS );

            ParticleGpuData *gpuData = systemDef->mParticleGpuData + gpuAdvance;

            for( const ParticleAffector2 *affector : systemDef->mAffectors )
                affector->run( cpuData, numParticlesToProcess, timeSinceLast );

            tickParticles( threadIdx, timeSinceLast, cpuData, gpuData, numParticlesToProcess,
                           systemDef );

            gpuAdvance += numParticlesToProcess;
            totalThreadNumParticlesToProcess = particleExcess;
            // If threadAdvance < quota, then we are crossing the boundary and
            // must start processing from threadAdvance = 0
            //
            // If threadAdvance >= quota, then just do threadAdvance -= quota.
            threadAdvance = threadAdvance - std::min( quota, threadAdvance );
            cpuData = systemDef->getParticleCpuData();
        }
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
void ParticleSystemManager2::addAffectorFactory( ParticleAffectorFactory2 *factory )
{
    sAffectorFactories[factory->getName()] = factory;
}
//-----------------------------------------------------------------------------
void ParticleSystemManager2::removeAffectorFactory( ParticleAffectorFactory2 *factory )
{
    sAffectorFactories.erase( factory->getName() );
}
//-----------------------------------------------------------------------------
ParticleAffectorFactory2 *ParticleSystemManager2::getAffectorFactory( IdString name )
{
    std::map<IdString, ParticleAffectorFactory2 *>::const_iterator itor =
        sAffectorFactories.find( name );
    if( itor == sAffectorFactories.end() )
    {
        OGRE_EXCEPT(
            Exception::ERR_ITEM_NOT_FOUND,
            "No Affector with name '" + name.getFriendlyText() + "' found. Is the plugin installed?",
            "ParticleSystemManager2::getAffectorFactory" );
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

    itor->second = new ParticleSystemDef( Id::generateNewId<MovableObject>(),
                                          &mSceneManager->_getParticleSysDefMemoryManager(),
                                          mSceneManager, this, name );
    mSceneManager->getRootSceneNode( SCENE_STATIC )->attachObject( itor->second );
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
void ParticleSystemManager2::_addToRenderQueue( size_t threadIdx, size_t numThreads,
                                                RenderQueue *renderQueue, uint8 renderQueueId,
                                                uint32 visibilityMask ) const
{
    const size_t numSystemDefs = mActiveParticleSystemDefs.size();
    const size_t systemDefsPerThread = ( numSystemDefs + numThreads - 1u ) / numThreads;
    const size_t toAdvance = std::min( threadIdx * systemDefsPerThread, numSystemDefs );
    const size_t numParticlesToProcess = std::min( systemDefsPerThread, numSystemDefs - toAdvance );

    FastArray<ParticleSystemDef *>::const_iterator itor = mActiveParticleSystemDefs.begin() + toAdvance;
    FastArray<ParticleSystemDef *>::const_iterator endt =
        mActiveParticleSystemDefs.begin() + toAdvance + numParticlesToProcess;

    while( itor != endt )
    {
        ParticleSystemDef *systemDef = *itor;
        if( systemDef->getNumSimdActiveParticles() > 0u &&  //
            systemDef->mRenderQueueID == renderQueueId &&   //
            systemDef->getVisibilityFlags() & visibilityMask )
        {
            renderQueue->addRenderableV2( threadIdx, systemDef->mRenderQueueID, false, systemDef,
                                          systemDef );
        }

        ++itor;
    }
}
//-----------------------------------------------------------------------------
void ParticleSystemManager2::setHighestPossibleQuota( uint16 highestQuota16, uint32 highestQuota32 )
{
    if( mSharedIndexBuffer16 && mSharedIndexBuffer32 )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                     "It's too late. This function must be called earlier.",
                     "ParticleSystemManager2::setHighestPossibleQuota" );
    }

    if( highestQuota16 * 4u > std::numeric_limits<uint16>::max() )
    {
        OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                     "highestQuota16 must be equal or lower than 65535 / 4",
                     "ParticleSystemManager2::setHighestPossibleQuota" );
    }

    mHighestPossibleQuota16 = highestQuota16 * 4u;
    mHighestPossibleQuota32 = highestQuota32 * 4u;
}
//-----------------------------------------------------------------------------
void ParticleSystemManager2::calculateHighestPossibleQuota( VaoManager *vaoManager )
{
    uint32 highestQuota16 = 0u;
    uint32 highestQuota32 = 0u;

    for( const auto &pair : mParticleSystemDefMap )
    {
        ParticleSystemDef *systemDef = pair.second;
        const uint32 quota = systemDef->getQuota() * 4u;
        if( quota <= std::numeric_limits<uint16>::max() )
            highestQuota16 = std::max( quota, highestQuota16 );
        else
            highestQuota32 = std::max( quota, highestQuota32 );
    }

    mHighestPossibleQuota16 = std::max( mHighestPossibleQuota16, highestQuota16 );
    mHighestPossibleQuota32 = std::max( mHighestPossibleQuota32, highestQuota32 );

    if( ( mSharedIndexBuffer16 &&
          mHighestPossibleQuota16 * 6u > mSharedIndexBuffer16->getNumElements() ) ||
        ( mSharedIndexBuffer32 &&
          mHighestPossibleQuota32 * 6u > mSharedIndexBuffer32->getNumElements() ) )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "Raising highest possible quota after initialization is not yet implemented. "
                     "Call setHighestPossibleQuota() earlier with a bigger number.",
                     "ParticleSystemManager2::calculateHighestPossibleQuota" );
    }

    createSharedIndexBuffers( vaoManager );
}
//-----------------------------------------------------------------------------
void ParticleSystemManager2::createSharedIndexBuffers( VaoManager *vaoManager )
{
    // If these asserts trigger, then mHighestPossibleQuotaXX was
    // increased without destroying the old mSharedIndexBufferXX
    OGRE_ASSERT_LOW( !mSharedIndexBuffer16 ||
                     mHighestPossibleQuota16 * 6u <= mSharedIndexBuffer16->getNumElements() );
    OGRE_ASSERT_LOW( !mSharedIndexBuffer32 ||
                     mHighestPossibleQuota32 * 6u <= mSharedIndexBuffer32->getNumElements() );

    if( mHighestPossibleQuota16 > 0u && !mSharedIndexBuffer16 )
    {
        const size_t numIndices = mHighestPossibleQuota16 * 6u;

        uint16_t *index16 = reinterpret_cast<uint16_t *>(
            OGRE_MALLOC_SIMD( sizeof( uint16_t ) * numIndices, MEMCATEGORY_GEOMETRY ) );
        FreeOnDestructor dataPtrContainer( index16 );

        const size_t maxParticles = numIndices / 6u;
        for( size_t k = 0; k < maxParticles; ++k )
        {
            // Particle vertices:
            //	 A--C
            //   | /|
            //   |/ |
            //   B--D
            // Triangle 1 (ABC)
            index16[k * 6u + 0u] = static_cast<uint16>( k * 4u + 0u );  // A
            index16[k * 6u + 1u] = static_cast<uint16>( k * 4u + 1u );  // B
            index16[k * 6u + 2u] = static_cast<uint16>( k * 4u + 2u );  // C

            // Triangle 2 (ACD)
            index16[k * 6u + 3u] = static_cast<uint16>( k * 4u + 1u );  // B
            index16[k * 6u + 4u] = static_cast<uint16>( k * 4u + 3u );  // D
            index16[k * 6u + 5u] = static_cast<uint16>( k * 4u + 2u );  // C
        }

        mSharedIndexBuffer16 =
            vaoManager->createIndexBuffer( IT_16BIT, numIndices, BT_IMMUTABLE, index16, false );
    }

    if( mHighestPossibleQuota32 > 0u )
    {
        const size_t numIndices = mHighestPossibleQuota32 * 6u;

        uint32_t *index32 = reinterpret_cast<uint32_t *>(
            OGRE_MALLOC_SIMD( sizeof( uint32_t ) * numIndices, MEMCATEGORY_GEOMETRY ) );
        FreeOnDestructor dataPtrContainer( index32 );

        const size_t maxParticles = numIndices / 6u;
        for( size_t k = 0; k < maxParticles; ++k )
        {
            index32[k * 6u + 0u] = static_cast<uint32>( k * 6u + 0u );  // A
            index32[k * 6u + 1u] = static_cast<uint32>( k * 6u + 1u );  // B
            index32[k * 6u + 2u] = static_cast<uint32>( k * 6u + 2u );  // C

            index32[k * 6u + 3u] = static_cast<uint32>( k * 6u + 0u );  // A
            index32[k * 6u + 4u] = static_cast<uint32>( k * 6u + 2u );  // C
            index32[k * 6u + 5u] = static_cast<uint32>( k * 6u + 3u );  // D
        }

        mSharedIndexBuffer32 =
            vaoManager->createIndexBuffer( IT_32BIT, numIndices, BT_IMMUTABLE, index32, false );
    }
}
//-----------------------------------------------------------------------------
IndexBufferPacked *ParticleSystemManager2::_getSharedIndexBuffer( size_t maxQuota,
                                                                  VaoManager *vaoManager )
{
    if( maxQuota <= std::numeric_limits<uint16>::max() )
    {
        if( !mSharedIndexBuffer16 )
            calculateHighestPossibleQuota( vaoManager );
        return mSharedIndexBuffer16;
    }

    if( !mSharedIndexBuffer32 )
        calculateHighestPossibleQuota( vaoManager );
    return mSharedIndexBuffer32;
}
//-----------------------------------------------------------------------------
void ParticleSystemManager2::update( const Real timeSinceLast )
{
    if( mActiveParticleSystemDefs.empty() )
        return;

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
