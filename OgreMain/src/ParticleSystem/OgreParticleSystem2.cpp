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

#include "ParticleSystem/OgreParticleSystem2.h"

#include "OgreBitset.inl"
#include "OgreException.h"
#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "ParticleSystem/OgreEmitter2.h"
#include "ParticleSystem/OgreParticleAffector2.h"
#include "ParticleSystem/OgreParticleSystemManager2.h"
#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreReadOnlyBufferPacked.h"
#include "Vao/OgreVaoManager.h"

using namespace Ogre;

struct GpuParticleCommon
{
    float commonDir[3];
    float padding0;
    float commonUp[3];
    float padding1;

    GpuParticleCommon( const Vector3 &commonDirection, const Vector3 &commonUpVector ) :
        commonDir{ (float)commonDirection.x, (float)commonDirection.y, (float)commonDirection.z },
        padding0( 0 ),
        commonUp{ (float)commonUpVector.x, (float)commonUpVector.y, (float)commonUpVector.z },
        padding1( 0 )
    {
    }
};

ParticleSystemDef::ParticleSystemDef( IdType id, ObjectMemoryManager *objectMemoryManager,
                                      SceneManager *manager,
                                      ParticleSystemManager2 *particleSystemManager,
                                      const String &name ) :
    ParticleSystem( id, objectMemoryManager, manager, "", kParticleSystemDefaultRenderQueueId ),
    mName( name ),
    mParticleSystemManager( particleSystemManager ),
    mGpuCommonData( 0 ),
    mGpuData( 0 ),
    mCommonDirection( Ogre::Vector3::UNIT_Z ),
    mCommonUpVector( Vector3::UNIT_Y ),
    mParticleGpuData( 0 ),
    mFirstParticleIdx( 0u ),
    mLastParticleIdx( 0u ),
    mParticleQuotaFull( false ),
    mRotationType( ParticleRotationType::None ),
    mParticleType( ParticleType::Point )
{
    memset( &mParticleCpuData, 0, sizeof( mParticleCpuData ) );
    mParticlesToKill.resizePOD( manager->getNumWorkerThreads() );

    mRenderQueueID = kParticleSystemDefaultRenderQueueId;
}
//-----------------------------------------------------------------------------
ParticleSystemDef::~ParticleSystemDef()
{
    OGRE_ASSERT_LOW( !mParticleCpuData.mPosition && !mGpuData && !mGpuCommonData &&
                     "destroy() not called!" );
}
//-----------------------------------------------------------------------------
uint32 ParticleSystemDef::getQuota() const
{
    return static_cast<uint32>( mActiveParticles.capacity() );
}
//-----------------------------------------------------------------------------
void ParticleSystemDef::init( VaoManager *vaoManager )
{
    OGRE_ASSERT_LOW( mParticleCpuData.mPosition == nullptr );
    OGRE_ASSERT_MEDIUM( mActiveParticles.empty() );

    mFirstParticleIdx = 0u;
    mLastParticleIdx = 0u;

    const uint32 numParticles = getQuota();

    mParticleCpuData.mPosition = reinterpret_cast<ArrayVector3 *>(
        OGRE_MALLOC_SIMD( numParticles * sizeof( Vector3 ), MEMCATEGORY_GEOMETRY ) );
    mParticleCpuData.mDirection = reinterpret_cast<ArrayVector3 *>(
        OGRE_MALLOC_SIMD( numParticles * sizeof( Vector3 ), MEMCATEGORY_GEOMETRY ) );
    mParticleCpuData.mDimensions = reinterpret_cast<ArrayVector2 *>(
        OGRE_MALLOC_SIMD( numParticles * sizeof( Vector2 ), MEMCATEGORY_GEOMETRY ) );
    mParticleCpuData.mRotation = reinterpret_cast<ArrayRadian *>(
        OGRE_MALLOC_SIMD( numParticles * sizeof( Radian ), MEMCATEGORY_GEOMETRY ) );
    mParticleCpuData.mRotationSpeed = reinterpret_cast<ArrayRadian *>(
        OGRE_MALLOC_SIMD( numParticles * sizeof( Radian ), MEMCATEGORY_GEOMETRY ) );
    mParticleCpuData.mTimeToLive = reinterpret_cast<ArrayReal *>(
        OGRE_MALLOC_SIMD( numParticles * sizeof( Real ), MEMCATEGORY_GEOMETRY ) );
    mParticleCpuData.mTotalTimeToLive = reinterpret_cast<ArrayReal *>(
        OGRE_MALLOC_SIMD( numParticles * sizeof( Real ), MEMCATEGORY_GEOMETRY ) );
    mParticleCpuData.mColour = reinterpret_cast<ArrayVector4 *>(
        OGRE_MALLOC_SIMD( numParticles * sizeof( Vector4 ), MEMCATEGORY_GEOMETRY ) );

    memset( mParticleCpuData.mPosition, 0, numParticles * sizeof( Vector3 ) );
    memset( mParticleCpuData.mDirection, 0, numParticles * sizeof( Vector3 ) );
    memset( mParticleCpuData.mDimensions, 0, numParticles * sizeof( Vector2 ) );
    memset( mParticleCpuData.mRotation, 0, numParticles * sizeof( Radian ) );
    memset( mParticleCpuData.mRotationSpeed, 0, numParticles * sizeof( Radian ) );
    memset( mParticleCpuData.mTimeToLive, 0, numParticles * sizeof( Real ) );
    memset( mParticleCpuData.mTotalTimeToLive, 0, numParticles * sizeof( Real ) );
    memset( mParticleCpuData.mColour, 0, numParticles * sizeof( Vector4 ) );

    GpuParticleCommon particleCommon( mCommonDirection,
                                      mParticleType != ParticleType::PerpendicularCommon
                                          ? mCommonUpVector
                                          : mCommonUpVector.crossProduct( mCommonDirection ) );
    mGpuCommonData =
        vaoManager->createConstBuffer( sizeof( GpuParticleCommon ), BT_DEFAULT, &particleCommon, false );

    mGpuData = vaoManager->createReadOnlyBuffer(
        PFG_RGBA8_UINT, sizeof( ParticleGpuData ) * numParticles, BT_DYNAMIC_PERSISTENT, 0, false );

    mVaoPerLod[VpNormal].push_back( vaoManager->createVertexArrayObject(
        {}, mParticleSystemManager->_getSharedIndexBuffer( numParticles, vaoManager ),
        OT_TRIANGLE_LIST ) );
    mVaoPerLod[VpShadow] = mVaoPerLod[VpNormal];

    HlmsManager *hlmsManager = Root::getSingleton().getHlmsManager();
    this->setDatablock( hlmsManager->getHlms( HLMS_UNLIT )->getDefaultDatablock() );
}
//-----------------------------------------------------------------------------
void ParticleSystemDef::_destroy( VaoManager *vaoManager )
{
    for( ParticleSystem2 *system : mParticleSystems )
        delete system;
    mActiveParticleSystems.clear();
    mParticleSystems.clear();

    if( mParticleCpuData.mPosition )
    {
        OGRE_FREE_SIMD( mParticleCpuData.mColour, MEMCATEGORY_GEOMETRY );
        OGRE_FREE_SIMD( mParticleCpuData.mTotalTimeToLive, MEMCATEGORY_GEOMETRY );
        OGRE_FREE_SIMD( mParticleCpuData.mTimeToLive, MEMCATEGORY_GEOMETRY );
        OGRE_FREE_SIMD( mParticleCpuData.mRotationSpeed, MEMCATEGORY_GEOMETRY );
        OGRE_FREE_SIMD( mParticleCpuData.mRotation, MEMCATEGORY_GEOMETRY );
        OGRE_FREE_SIMD( mParticleCpuData.mDimensions, MEMCATEGORY_GEOMETRY );
        OGRE_FREE_SIMD( mParticleCpuData.mDirection, MEMCATEGORY_GEOMETRY );
        OGRE_FREE_SIMD( mParticleCpuData.mPosition, MEMCATEGORY_GEOMETRY );

        mParticleCpuData.mPosition = 0;

        if( mGpuData->getMappingState() != MS_UNMAPPED )
        {
            mGpuData->unmap( UO_UNMAP_ALL );
            mParticleGpuData = 0;
        }

        vaoManager->destroyReadOnlyBuffer( mGpuData );
        mGpuData = 0;

        vaoManager->destroyConstBuffer( mGpuCommonData );
        mGpuCommonData = 0;
    }

    for( EmitterDefData *emitter : mEmitters )
        delete emitter;
    mEmitters.clear();
    for( ParticleAffector2 *affector : mAffectors )
        delete affector;
    mAffectors.clear();
}
//-----------------------------------------------------------------------------
bool ParticleSystemDef::isInitialized() const
{
    return mParticleCpuData.mPosition != nullptr;
}
//-----------------------------------------------------------------------------
void ParticleSystemDef::setParticleQuota( size_t quota )
{
    OGRE_ASSERT_LOW( !isInitialized() );
    mActiveParticles.reset( alignToNextMultiple<size_t>( quota, ARRAY_PACKED_REALS ) );
}
//-----------------------------------------------------------------------------
void ParticleSystemDef::setParticleType( ParticleType::ParticleType particleType )
{
    if( particleType == ParticleType::NotParticle )
    {
        OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Invalid value: ParticleType::NotParticle",
                     "ParticleSystemDef::setParticleType" );
    }
    mParticleType = particleType;
}
//-----------------------------------------------------------------------------
void ParticleSystemDef::setRotationType( ParticleRotationType::ParticleRotationType rotationType )
{
    mRotationType = rotationType;
}
//-----------------------------------------------------------------------------
ParticleRotationType::ParticleRotationType ParticleSystemDef::getRotationType() const
{
    return mRotationType;
}
//-----------------------------------------------------------------------------
void ParticleSystemDef::setCommonVectors( const Vector3 &commonDir, const Vector3 &commonUp )
{
    mCommonDirection = commonDir;
    mCommonUpVector = commonUp;

    if( mGpuCommonData )
    {
        GpuParticleCommon particleCommon( mCommonDirection,
                                          mParticleType != ParticleType::PerpendicularCommon
                                              ? mCommonUpVector
                                              : mCommonUpVector.crossProduct( mCommonDirection ) );
        mGpuCommonData->upload( &particleCommon, 0u, mGpuCommonData->getTotalSizeBytes() );
    }
}
//-----------------------------------------------------------------------------
void ParticleSystemDef::setCommonDirection( const Vector3 &vec )
{
    setCommonVectors( vec, mCommonUpVector );
}
//-----------------------------------------------------------------------------
const Vector3 &ParticleSystemDef::getCommonDirection() const
{
    return mCommonDirection;
}
//-----------------------------------------------------------------------------
void ParticleSystemDef::setCommonUpVector( const Vector3 &vec )
{
    setCommonVectors( mCommonDirection, vec );
}
//-----------------------------------------------------------------------------
const Vector3 &ParticleSystemDef::getCommonUpVector() const
{
    return mCommonUpVector;
}
//-----------------------------------------------------------------------------
void ParticleSystemDef::reserveNumEmitters( size_t numEmitters )
{
    mEmitters.reserve( numEmitters );
}
//-----------------------------------------------------------------------------
EmitterDefData *ParticleSystemDef::addEmitter( IdString name )
{
    mEmitters.push_back( ParticleSystemManager2::getFactory( name )->createEmitter() );
    return mEmitters.back();
}
//-----------------------------------------------------------------------------
size_t ParticleSystemDef::getNumEmitters() const
{
    return mEmitters.size();
}
//-----------------------------------------------------------------------------
void ParticleSystemDef::reserveNumAffectors( size_t numAffectors )
{
    mAffectors.reserve( numAffectors );
}
//-----------------------------------------------------------------------------
ParticleAffector2 *ParticleSystemDef::addAffector( IdString name )
{
    ParticleAffector2 *newAffector =
        ParticleSystemManager2::getAffectorFactory( name )->createAffector();
    mAffectors.push_back( newAffector );
    if( newAffector->needsInitialization() )
        mInitializableAffectors.push_back( newAffector );
    return newAffector;
}
//-----------------------------------------------------------------------------
size_t ParticleSystemDef::getNumAffectors() const
{
    return mAffectors.size();
}
//-----------------------------------------------------------------------------
ParticleSystem2 *ParticleSystemDef::_createParticleSystem( ObjectMemoryManager *objectMemoryManager )
{
    // We must track mParticleSystems and not mActiveParticleSystems because even if a ParticleSystems
    // gets disabled, its live particles must still be simulated until they all die.
    if( mParticleSystems.empty() )
        mParticleSystemManager->_addParticleSystemDefAsActive( this );

    ParticleSystem2 *retVal =
        new ParticleSystem2( Id::generateNewId<MovableObject>(), objectMemoryManager, mManager, this );
    retVal->mParentDefGlobalIdx = mParticleSystems.size();
    mParticleSystems.push_back( retVal );
    mActiveParticleSystems.push_back( retVal );
    return retVal;
}
//-----------------------------------------------------------------------------
void ParticleSystemDef::_destroyParticleSystem( ParticleSystem2 *system )
{
    if( system->mParentDefGlobalIdx >= mParticleSystems.size() ||
        system != *( mParticleSystems.begin() + static_cast<ptrdiff_t>( system->mParentDefGlobalIdx ) ) )
    {
        OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR,
                     "ParticleSystem '" + mName +
                         "' had it's mParentDefGlobalIdx out of date!!! (or the ParticleSystem wasn't "
                         "created by this ParticleSystemDef)",
                     "ParticleSystemDef::destroyParticleSystem" );
    }

    FastArray<ParticleSystem2 *>::iterator itor =
        mParticleSystems.begin() + static_cast<ptrdiff_t>( system->mParentDefGlobalIdx );
    itor = efficientVectorRemove( mParticleSystems, itor );
    delete system;
    system = 0;

    // The node that was at the end got swapped and has now a different index
    if( itor != mParticleSystems.end() )
        ( *itor )->mParentDefGlobalIdx = static_cast<size_t>( itor - mParticleSystems.begin() );

    if( mParticleSystems.empty() )
        mParticleSystemManager->_removeParticleSystemDefFromActive( this );
}
//-----------------------------------------------------------------------------
void ParticleSystemDef::_destroyAllParticleSystems()
{
    for( ParticleSystem2 *system : mParticleSystems )
        delete system;
    mParticleSystems.clear();

    const size_t numActiveParticles = getNumSimdActiveParticles();
    ArrayReal *RESTRICT_ALIAS timeToLive =
        mParticleCpuData.mTimeToLive + this->getActiveParticlesPackOffset();
    for( size_t i = 0; i < numActiveParticles; i += ARRAY_PACKED_REALS )
        *timeToLive++ = ARRAY_REAL_ZERO;

    mFirstParticleIdx = 0u;
    mLastParticleIdx = 0u;
    mParticleQuotaFull = false;
    mActiveParticles.clear();
}
//-----------------------------------------------------------------------------
uint32 ParticleSystemDef::allocParticle()
{
    OGRE_ASSERT_MEDIUM( mLastParticleIdx >= mFirstParticleIdx );

    const uint32 quota = getQuota();
    const uint32 distance = mLastParticleIdx - mFirstParticleIdx;

    if( distance == quota )
    {
        // Hard case. We need to see if there's anything available between range
        // [mFirstParticleIdx; mLastParticleIdx)
        if( mParticleQuotaFull )
        {
            // We already cached the search from a previous call.
            return InvalidHandle;
        }

        // We use findLastBitUnset() because we assume this new particle will be the youngest (and thus
        // remain the longest); thus place it as higher as possible in the bitset to maintain
        // FIFO removals.
        if( mLastParticleIdx == quota )
        {
            // We search in range [0; quota)
            const size_t newIdx = mActiveParticles.findLastBitUnset();

            if( newIdx == mActiveParticles.capacity() )
            {
                mParticleQuotaFull = true;
                return InvalidHandle;
            }

            return static_cast<uint32>( newIdx );
        }
        else
        {
            const size_t lastParticleIdxWrapped = mLastParticleIdx % quota;

            // This should be impossible since we already checked mLastParticleIdx != quota
            // and mLastParticleIdx must be < quota * 2
            OGRE_ASSERT_MEDIUM( lastParticleIdxWrapped != 0u );

            // We search in range [0; lastParticleIdxWrapped)
            size_t newIdx = mActiveParticles.findLastBitUnset( lastParticleIdxWrapped );

            if( newIdx == mActiveParticles.capacity() )
            {
                // That failed. We have to search in range [mFirstParticleIdx; quota) but because
                // we can't do that, we just do a full [0; quota) search.
                newIdx = mActiveParticles.findLastBitUnset( mFirstParticleIdx + 1u );

                if( newIdx == mActiveParticles.capacity() )
                {
                    mParticleQuotaFull = true;
                    return InvalidHandle;
                }
            }

            return static_cast<uint32>( newIdx );
        }
    }
    else
    {
        // Easy case.
        OGRE_ASSERT_MEDIUM( distance < quota );

        const uint32 newIdx = mLastParticleIdx % quota;

        OGRE_ASSERT_MEDIUM( !mActiveParticles.test( newIdx ) );
        mActiveParticles.set( newIdx );
        ++mLastParticleIdx;

        OGRE_ASSERT_MEDIUM( mLastParticleIdx < quota * 2u );

        return newIdx;
    }
}
//-----------------------------------------------------------------------------
void ParticleSystemDef::deallocParticle( uint32 handle )
{
    const uint32 quota = getQuota();

    const size_t lastParticleIdxWrapped = mLastParticleIdx % quota;

    OGRE_ASSERT_MEDIUM( handle < quota && "Invalid handle!" );
    OGRE_ASSERT_MEDIUM( mActiveParticles.test( handle ) && "Particle double freed!" );
    OGRE_ASSERT_MEDIUM(
        ( mFirstParticleIdx >= handle && handle < mLastParticleIdx && mLastParticleIdx <= quota ) ||
        ( ( mFirstParticleIdx >= handle || handle < lastParticleIdxWrapped ) &&
          mLastParticleIdx > quota ) &&
            "Invalid state detected. The handle is outside the tracking zone." );

    reinterpret_cast<Real * RESTRICT_ALIAS>( mParticleCpuData.mTimeToLive )[handle] = -1.0f;

    mParticleQuotaFull = false;
    mActiveParticles.unset( handle );
    if( handle + 1u == lastParticleIdxWrapped )
        --mLastParticleIdx;  // Non-FIFO order. Unexpected, but easy case
    else if( handle == mFirstParticleIdx )
    {
        ++mFirstParticleIdx;  // FIFO order. As expected. Easy case.
        if( mFirstParticleIdx == quota )
        {
            OGRE_ASSERT_MEDIUM( mLastParticleIdx >= quota &&
                                "Did mFirstParticleIdx overtake mLastParticleIdx???" );
            mFirstParticleIdx = 0u;
            mLastParticleIdx -= quota;
        }
    }
    else
    {
        // Non-FIFO order. If mFirstParticleIdx has already been freed
        // then advance mFirstParticleIdx as much as possible.
        if( !mActiveParticles.test( mFirstParticleIdx ) )
        {
            const size_t firstSet = mActiveParticles.findFirstBitSet( mFirstParticleIdx );
            if( firstSet == mActiveParticles.capacity() )
            {
                // No active particles in [mFirstParticleIdx; capacity)
                // We need to shrink mLastParticleIdx by searching [0; mLastParticleIdx)
                OGRE_ASSERT_HIGH( mActiveParticles.empty() );
                mFirstParticleIdx = 0u;
                mLastParticleIdx =
                    static_cast<uint32>( mActiveParticles.findLastBitSetPlusOne( mLastParticleIdx ) );
                OGRE_ASSERT_MEDIUM( mLastParticleIdx != 0u || mActiveParticles.empty() );
            }
            else
            {
                mFirstParticleIdx = static_cast<uint32>( firstSet );
                OGRE_ASSERT_MEDIUM( firstSet < mLastParticleIdx );
            }
        }
    }
}
//-----------------------------------------------------------------------------
uint32 ParticleSystemDef::getHandle( const ParticleCpuData &cpuData, size_t idx ) const
{
    const uint32 handle =
        uint32( cpuData.mPosition - mParticleCpuData.mPosition ) * ARRAY_PACKED_REALS + uint32( idx );
    OGRE_ASSERT_MEDIUM( handle < getQuota() && "Invalid handle!" );
    return handle;
}
//-----------------------------------------------------------------------------
void ParticleSystemDef::cloneImpl( ParticleSystemDef *toClone )
{
    toClone->mCommonDirection = this->mCommonDirection;
    toClone->mCommonUpVector = this->mCommonUpVector;
    toClone->mRotationType = this->mRotationType;
    toClone->mParticleType = this->mParticleType;

    toClone->mEmitters.reserve( this->mEmitters.size() );
    for( const EmitterDefData *emitter : mEmitters )
    {
        EmitterDefData *newEmitter = toClone->addEmitter( emitter->asParticleEmitter()->getType() );
        newEmitter->_cloneFrom( emitter );
    }
}
//-----------------------------------------------------------------------------
ParticleType::ParticleType ParticleSystemDef::getParticleType() const
{
    return mParticleType;
}
//-----------------------------------------------------------------------------
void ParticleSystemDef::_notifyAttached( Node *parent )
{
    // We are a fake MovableObject, bypass ParticleSystem's functionality
    // (which we only derive from to get backwards compatibility)
    MovableObject::_notifyAttached( parent );
}
//-----------------------------------------------------------------------------
void ParticleSystemDef::getRenderOperation( v1::RenderOperation &, bool )
{
    OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                 "ParticleSystemDef does not implement this function."
                 " ParticleSystemDef must be put in a RenderQueue ID operating in a special mode. See "
                 "RenderQueue::Modes",
                 "ParticleSystemDef::getRenderOperation" );
}
//-----------------------------------------------------------------------------
void ParticleSystemDef::getWorldTransforms( Matrix4 * ) const
{
    OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                 "ParticleSystemDef does not implement this function."
                 " ParticleSystemDef must be put in a RenderQueue ID operating in a special mode. See "
                 "RenderQueue::Modes",
                 "ParticleSystemDef::getWorldTransforms" );
}
//-----------------------------------------------------------------------------
const LightList &ParticleSystemDef::getLights() const
{
    OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                 "ParticleSystemDef does not implement this function."
                 " ParticleSystemDef must be put in a RenderQueue ID operating in a special mode. See "
                 "RenderQueue::Modes",
                 "ParticleSystemDef::getLights" );
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
ParticleSystem2::ParticleSystem2( IdType id, ObjectMemoryManager *objectMemoryManager,
                                  SceneManager *manager, const ParticleSystemDef *creator ) :
    MovableObject( id, objectMemoryManager, manager, 110u ),
    mCreator( creator ),
    mParentDefGlobalIdx( std::numeric_limits<size_t>::max() )
{
    const size_t numEmitters = creator->getNumEmitters();
    mNewParticlesPerEmitter.resizePOD( numEmitters, 0u );
    mEmitterInstanceData.resize( numEmitters );

    const FastArray<EmitterDefData *> &emitterDefs = creator->getEmitters();

    for( size_t i = 0u; i < numEmitters; ++i )
        mEmitterInstanceData[i].setEnabled( true, *emitterDefs[i] );
}
//-----------------------------------------------------------------------------
const String &ParticleSystem2::getMovableType() const
{
    return ParticleSystem2Factory::FACTORY_TYPE_NAME;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const String ParticleSystem2Factory::FACTORY_TYPE_NAME = "ParticleSystem2";
//-----------------------------------------------------------------------------
const String &ParticleSystem2Factory::getType() const
{
    return FACTORY_TYPE_NAME;
}
//-----------------------------------------------------------------------------
MovableObject *ParticleSystem2Factory::createInstanceImpl( IdType id,
                                                           ObjectMemoryManager *objectMemoryManager,
                                                           SceneManager *,
                                                           const NameValuePairList *params )
{
    ParticleSystemDef *def =
        reinterpret_cast<ParticleSystemDef *>( StringConverter::parseSizeT( params->begin()->second ) );
    return def->_createParticleSystem( objectMemoryManager );
}
//-----------------------------------------------------------------------------
void ParticleSystem2Factory::destroyInstance( MovableObject *obj )
{
    OGRE_ASSERT_HIGH( dynamic_cast<ParticleSystem2 *>( obj ) );
    ParticleSystem2 *system = static_cast<ParticleSystem2 *>( obj );
    ParticleSystemDef *systemDef = const_cast<ParticleSystemDef *>( system->getParticleSystemDef() );
    systemDef->_destroyParticleSystem( system );
}
