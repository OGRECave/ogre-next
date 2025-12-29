/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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

#include "OgreStableHeaders.h"

#include "OgreVertexFormatWarmUp.h"

#include "Animation/OgreSkeletonManager.h"
#include "OgreItem.h"
#include "OgreLogManager.h"
#include "OgreSceneManager.h"
#include "OgreSkeleton.h"
#include "OgreSubItem.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

using namespace Ogre;

namespace Ogre
{
    static const uint16 c_hlmsVertexFormatWarmUpVersion = 0u;

    class WarmUpRenderable final : public MovableObject, public RenderableAnimated
    {
    public:
        WarmUpRenderable( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *manager,
                          uint8 renderQueueId, VertexArrayObject *vao, VertexArrayObject *shadowVao );
        ~WarmUpRenderable() override;

        void setupSkeleton( SkeletonInstance *instance,
                            FastArray<unsigned short> *blendIndexToBoneIndexMap );

        // Overrides from MovableObject
        const String &getMovableType() const override;

        // Overrides from Renderable
        const LightList &getLights() const override;
        void getRenderOperation( v1::RenderOperation &op, bool casterPass ) override;
        void getWorldTransforms( Matrix4 *xform ) const override;
        bool getCastsShadows() const override;
    };
}  // namespace Ogre
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
VertexFormatWarmUpStorage::VertexFormatWarmUpStorage() : mNeedsSkeleton( false ), mSkeleton( 0 ) {}
//-----------------------------------------------------------------------------
VertexFormatWarmUpStorage::~VertexFormatWarmUpStorage() { destroyWarmUp(); }
//-----------------------------------------------------------------------------
void VertexFormatWarmUpStorage::createSkeleton( SceneManager *sceneManager )
{
    destroySkeleton( sceneManager );
    if( !mNeedsSkeleton )
        return;

    v1::Skeleton *skeleton =
        OGRE_NEW Ogre::v1::Skeleton( nullptr, "VertexFormatWarmUpStorage Skel", 0,
                                     ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true, nullptr );
    skeleton->setToLoaded();
    skeleton->createBone();  // We just need a root bone. We don't care about setting it up.
    // Because the vertex buffer has all 0s, even vertex formats with multiple bones per vertex
    // will all reference bone 0. Thus we just set [0] = 0.
    mBlendIndexToBoneIndexMap.resizePOD( 1u );
    mBlendIndexToBoneIndexMap[0] = 0;
    // We don't care about animations either. Just create the v2 skeleton now.
    SkeletonDefPtr skeletonDef = SkeletonManager::getSingleton().getSkeletonDef( skeleton );
    OGRE_DELETE skeleton;  // We don't need the v1 skeleton anymore.
    skeleton = 0;

    mSkeleton = sceneManager->createSkeletonInstance( skeletonDef.get() );
}
//-----------------------------------------------------------------------------
void VertexFormatWarmUpStorage::destroySkeleton( SceneManager *sceneManager )
{
    if( !mSkeleton )
        return;

    sceneManager->destroySkeletonInstance( mSkeleton );
    mSkeleton = 0;
    SkeletonDefPtr skeletonDef =
        SkeletonManager::getSingleton().getSkeletonDef( "VertexFormatWarmUpStorage Skel" );
    sceneManager->_removeSkeletonDef( skeletonDef.get() );
    skeletonDef.reset();
    SkeletonManager::getSingleton().remove( "VertexFormatWarmUpStorage Skel" );
}
//-----------------------------------------------------------------------------
void VertexFormatWarmUpStorage::analyze( const uint8 renderQueueId, const Renderable *renderable )
{
    if( !renderable->mRenderableVisible )
        return;

    const VertexArrayObjectArray &vaos = renderable->getVaos( VpNormal );
    if( vaos.empty() )
        return;  // Must be V1 object or malformed.

    if( !renderable->getCustomParameters().empty() )
        return;  // Ignore objects with potential indications for custom Hlms for now.

    if( renderable->getNumPoses() > 0u )
        return;  // Poses not supported yet.

    const VertexArrayObjectArray &shadowVaos = renderable->getVaos( VpShadow );

    const VertexFormatEntry::VFPair pair = { renderable->hasSkeletonAnimation(),
                                             vaos.front()->getOperationType(),
                                             shadowVaos.front()->getOperationType(),
                                             vaos.front()->getVertexDeclaration(),
                                             shadowVaos.front()->getVertexDeclaration() };

    mNeedsSkeleton |= pair.hasSkeleton;

    const uint64 hash = ( uint64( renderQueueId ) << 32u ) | renderable->getHlmsHash();

    VertexFormatEntryVec::iterator itor = std::lower_bound( mEntries.begin(), mEntries.end(), pair );
    if( itor == mEntries.end() || itor->vertexFormats != pair )
    {
        // New entry
        VertexFormatEntry entry;
        entry.vertexFormats = pair;
        entry.seenHlmsHashes.insert( hash );
        entry.materials.push_back( { renderQueueId, renderable->getDatablockOrMaterialName() } );
        mEntries.insert( itor, entry );
    }
    else
    {
        // Old entry. Check if we've seen this Hlms hash combination.
        const auto insertionResult = itor->seenHlmsHashes.insert( hash );
        if( insertionResult.second )
            itor->materials.push_back( { renderQueueId, renderable->getDatablockOrMaterialName() } );
    }
}
//-----------------------------------------------------------------------------
void VertexFormatWarmUpStorage::analyze( Ogre::SceneManager *sceneManager )
{
    for( size_t sceneMgrType = 0u; sceneMgrType < NUM_SCENE_MEMORY_MANAGER_TYPES; ++sceneMgrType )
    {
        ObjectMemoryManager &memoryManager =
            sceneManager->_getEntityMemoryManager( static_cast<SceneMemoryMgrTypes>( sceneMgrType ) );

        const size_t numRenderQueues = memoryManager.getNumRenderQueues();

        for( size_t i = 0u; i < numRenderQueues; ++i )
        {
            ObjectData objData;
            const size_t totalObjs = memoryManager.getFirstObjectData( objData, i );

            for( size_t j = 0u; j < totalObjs; j += ARRAY_PACKED_REALS )
            {
                for( size_t k = 0u; k < ARRAY_PACKED_REALS; ++k )
                {
                    const Item *item = dynamic_cast<const Item *>( objData.mOwner[k] );
                    if( item )
                    {
                        const size_t numSubItems = item->getNumSubItems();
                        for( size_t l = 0u; l < numSubItems; ++l )
                            analyze( item->getRenderQueueGroup(), item->getSubItem( l ) );
                    }
                    else
                    {
                        // Generic fallback.
                        for( const Renderable *renderable : objData.mOwner[k]->mRenderables )
                            analyze( objData.mOwner[k]->getRenderQueueGroup(), renderable );
                    }
                }
                objData.advancePack();
            }
        }
    }
}
//-----------------------------------------------------------------------------
template <typename T>
void write( DataStreamPtr &dataStream, const T &value )
{
    dataStream->write( &value, sizeof( value ) );
}
template <>
void write<bool>( DataStreamPtr &dataStream, const bool &value )
{
    const uint8 valueAsU8 = value ? 1u : 0u;
    dataStream->write( &valueAsU8, sizeof( valueAsU8 ) );
}
inline void writeString16( DataStreamPtr &dataStream, const String &value )
{
    OGRE_ASSERT_LOW( value.size() + 1u < std::numeric_limits<uint16>::max() );
    write<uint16>( dataStream, uint16( value.size() ) );
    dataStream->write( value.c_str(), value.size() );
}
//-----------------------------------------------------------------------------
void VertexFormatWarmUpStorage::save( DataStreamPtr &dataStream,
                                      const VertexElement2VecVec &vertexElements )
{
    const uint16 numVertexBuffers = uint16( vertexElements.size() );
    write<uint16>( dataStream, numVertexBuffers );

    for( size_t i = 0u; i < numVertexBuffers; ++i )
    {
        const uint16 numFormats = uint16( vertexElements[i].size() );
        write<uint16>( dataStream, numFormats );

        for( size_t j = 0u; j < numFormats; ++j )
        {
            write<uint8>( dataStream, vertexElements[i][j].mType );
            write<uint8>( dataStream, vertexElements[i][j].mSemantic );
            write<uint32>( dataStream, vertexElements[i][j].mInstancingStepRate );
        }
    }
}
//-----------------------------------------------------------------------------
void VertexFormatWarmUpStorage::saveTo( DataStreamPtr &dataStream )
{
    LogManager::getSingleton().logMessage( "Saving VertexFormatWarmUpStorage to " +
                                           dataStream->getName() );

    write<uint16>( dataStream, c_hlmsVertexFormatWarmUpVersion );

    const uint32 numEntries = static_cast<uint32>( mEntries.size() );
    write<uint32>( dataStream, numEntries );

    for( const VertexFormatEntry &entry : mEntries )
    {
        write<uint8>( dataStream, entry.vertexFormats.hasSkeleton );
        write<uint8>( dataStream, entry.vertexFormats.opType );
        write<uint8>( dataStream, entry.vertexFormats.opTypeShadow );
        save( dataStream, entry.vertexFormats.normal );
        save( dataStream, entry.vertexFormats.shadow );

        const uint32 numMaterials = static_cast<uint32>( entry.materials.size() );
        write<uint32>( dataStream, numMaterials );
        for( const VertexFormatEntry::MaterialRqId &material : entry.materials )
        {
            write<uint8>( dataStream, material.renderQueueId );
            writeString16( dataStream, material.name );
        }

        // entry.seenHlmsHashes are not saved because they're not deterministic
        // (they can be deterministic, but a lot of care must be taken).
    }
}
//-----------------------------------------------------------------------------
template <typename T>
void read( DataStreamPtr &dataStream, T &value )
{
    dataStream->read( &value, sizeof( value ) );
}
template <typename T>
T read( DataStreamPtr &dataStream )
{
    T value;
    dataStream->read( &value, sizeof( value ) );
    return value;
}
template <>
void read<bool>( DataStreamPtr &dataStream, bool &value )
{
    uint8 valueU8;
    dataStream->read( &valueU8, sizeof( valueU8 ) );
    value = valueU8 != 0u;
}
template <>
bool read<bool>( DataStreamPtr &dataStream )
{
    uint8 value;
    dataStream->read( &value, sizeof( value ) );
    return value != 0u;
}
inline String readString16( DataStreamPtr &dataStream )
{
    const size_t stringLength = read<uint16>( dataStream );
    String retVal;
    retVal.resize( stringLength );
    dataStream->read( &retVal[0], stringLength );
    return retVal;
}
//-----------------------------------------------------------------------------
void VertexFormatWarmUpStorage::load( DataStreamPtr &dataStream,
                                      VertexElement2VecVec &outVertexElements )
{
    outVertexElements.clear();

    const uint16 numVertexBuffers = read<uint16>( dataStream );
    outVertexElements.reserve( numVertexBuffers );

    VertexElement2Vec vertexElements;
    for( size_t i = 0u; i < numVertexBuffers; ++i )
    {
        const uint16 numFormats = read<uint16>( dataStream );
        vertexElements.clear();
        vertexElements.reserve( numFormats );

        VertexElement2 vertexElement( VET_FLOAT1, VES_POSITION );
        for( size_t j = 0u; j < numFormats; ++j )
        {
            vertexElement.mType = static_cast<VertexElementType>( read<uint8>( dataStream ) );
            vertexElement.mSemantic = static_cast<VertexElementSemantic>( read<uint8>( dataStream ) );
            vertexElement.mInstancingStepRate = read<uint32>( dataStream );
            vertexElements.push_back( vertexElement );
        }

        outVertexElements.push_back( vertexElements );
    }
}
//-----------------------------------------------------------------------------
void VertexFormatWarmUpStorage::loadFrom( DataStreamPtr &dataStream )
{
    LogManager::getSingleton().logMessage( "Loading VertexFormatWarmUpStorage from " +
                                           dataStream->getName() );

    mNeedsSkeleton = false;
    mEntries.clear();

    const uint16 version = read<uint16>( dataStream );
    if( version != c_hlmsVertexFormatWarmUpVersion )
    {
        LogManager::getSingleton().logMessage(
            "VertexFormatWarmUpStorage: Version mismatch. Not loading.", LML_CRITICAL );
        OGRE_ASSERT_HIGH( false );
        return;
    }

    const uint32 numEntries = read<uint32>( dataStream );
    mEntries.reserve( numEntries );

    VertexFormatEntry entry;
    for( size_t i = 0u; i < numEntries; ++i )
    {
        entry.vertexFormats.hasSkeleton = read<bool>( dataStream );
        mNeedsSkeleton |= entry.vertexFormats.hasSkeleton;
        entry.vertexFormats.opType = static_cast<OperationType>( read<uint8>( dataStream ) );
        entry.vertexFormats.opTypeShadow = static_cast<OperationType>( read<uint8>( dataStream ) );
        load( dataStream, entry.vertexFormats.normal );
        load( dataStream, entry.vertexFormats.shadow );

        const uint32 numMaterials = read<uint32>( dataStream );
        entry.materials.clear();
        entry.materials.reserve( numMaterials );
        for( size_t j = 0u; j < numMaterials; ++j )
        {
            const uint8 renderQueueId = read<uint8>( dataStream );
            entry.materials.push_back( { renderQueueId, readString16( dataStream ) } );
        }

        mEntries.push_back( entry );
    }
}
//-----------------------------------------------------------------------------
void VertexFormatWarmUpStorage::createWarmUp( SceneManager *sceneManager )
{
    destroyWarmUp();

    if( mNeedsSkeleton && !mSkeleton )
        createSkeleton( sceneManager );

    VaoManager *vaoManager = sceneManager->getDestinationRenderSystem()->getVaoManager();

    FastArray<uint8> zeroData;
    VertexBufferPackedVec vertexBuffers;

    SceneNode *rootNode = sceneManager->getRootSceneNode( SCENE_STATIC );
    for( VertexFormatEntry &entry : mEntries )
    {
        // Generate dummy buffers full of 0. We hold data for 3 vertices
        // to avoid tripping any potential HW bug. Index buffers are not needed.
        vertexBuffers.clear();
        for( size_t i = 0u; i < entry.vertexFormats.normal.size(); ++i )
        {
            const size_t bytesNeeded = VaoManager::calculateVertexSize( entry.vertexFormats.normal[i] );
            zeroData.resizePOD( bytesNeeded * 3u, 0u );
            vertexBuffers.push_back( vaoManager->createVertexBuffer(
                entry.vertexFormats.normal[i], 3u, BT_DEFAULT, zeroData.begin(), false ) );
        }

        VertexArrayObject *vao =
            vaoManager->createVertexArrayObject( vertexBuffers, nullptr, entry.vertexFormats.opType );
        VertexArrayObject *shadowVao = vao;

        if( entry.vertexFormats.normal != entry.vertexFormats.shadow ||
            entry.vertexFormats.opType != entry.vertexFormats.opTypeShadow )
        {
            vertexBuffers.clear();
            for( size_t i = 0u; i < entry.vertexFormats.normal.size(); ++i )
            {
                const size_t bytesNeeded =
                    VaoManager::calculateVertexSize( entry.vertexFormats.shadow[i] );
                zeroData.resizePOD( bytesNeeded * 3u, 0u );
                vertexBuffers.push_back( vaoManager->createVertexBuffer(
                    entry.vertexFormats.shadow[i], 3u, BT_DEFAULT, zeroData.begin(), false ) );
            }
            shadowVao = vaoManager->createVertexArrayObject( vertexBuffers, nullptr,
                                                             entry.vertexFormats.opTypeShadow );
        }

        // This assert should be impossible unless the cached file was corrupted or tampered.
        // If we saw an object in analyze(), then we recorded at least one material.
        //
        // If this assert would trigger and is ignored, we'll be leaking our
        // Vao and vertex buffer handles.
        OGRE_ASSERT_LOW( !entry.materials.empty() );

        const bool bHasSkeleton = entry.vertexFormats.hasSkeleton;

        // Start spawning
        for( const VertexFormatEntry::MaterialRqId &material : entry.materials )
        {
            WarmUpRenderable *renderable =
                new WarmUpRenderable( Ogre::Id::generateNewId<Ogre::MovableObject>(),
                                      &sceneManager->_getEntityMemoryManager( Ogre::SCENE_STATIC ),
                                      sceneManager, material.renderQueueId, vao, shadowVao );
            if( bHasSkeleton )
                renderable->setupSkeleton( mSkeleton, &mBlendIndexToBoneIndexMap );
            entry.renderables.push_back( renderable );
            renderable->setDatablock( material.name );
            rootNode->attachObject( renderable );
        }
    }
}
//-----------------------------------------------------------------------------
void VertexFormatWarmUpStorage::destroyWarmUp()
{
    VaoManager *vaoManager = 0;
    SceneManager *sceneManager = 0;

    if( !mEntries.empty() && !mEntries.back().renderables.empty() )
    {
        sceneManager = mEntries.back().renderables.back()->_getManager();
        vaoManager = sceneManager->getDestinationRenderSystem()->getVaoManager();
    }

    for( VertexFormatEntry &entry : mEntries )
    {
        VertexArrayObject *vaos[NumVertexPass] = { nullptr, nullptr };
        if( !entry.renderables.empty() )
        {
            vaos[VpNormal] = entry.renderables.back()->mRenderables.back()->getVaos( VpNormal ).back();
            vaos[VpShadow] = entry.renderables.back()->mRenderables.back()->getVaos( VpShadow ).back();
        }

        for( WarmUpRenderable *renderable : entry.renderables )
            delete renderable;
        entry.renderables.clear();

        if( vaos[VpNormal] )
        {
            if( vaos[VpNormal] != vaos[VpShadow] )
            {
                const VertexBufferPackedVec &vertexBuffers = vaos[VpShadow]->getVertexBuffers();
                for( VertexBufferPacked *vertexBuffer : vertexBuffers )
                    vaoManager->destroyVertexBuffer( vertexBuffer );
                vaoManager->destroyVertexArrayObject( vaos[VpShadow] );
            }

            {
                const VertexBufferPackedVec &vertexBuffers = vaos[VpNormal]->getVertexBuffers();
                for( VertexBufferPacked *vertexBuffer : vertexBuffers )
                    vaoManager->destroyVertexBuffer( vertexBuffer );
                vaoManager->destroyVertexArrayObject( vaos[VpNormal] );
            }
        }
    }

    destroySkeleton( sceneManager );
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
WarmUpRenderable::WarmUpRenderable( IdType id, ObjectMemoryManager *objectMemoryManager,
                                    SceneManager *manager, uint8 renderQueueId, VertexArrayObject *vao,
                                    VertexArrayObject *shadowVao ) :
    MovableObject( id, objectMemoryManager, manager, renderQueueId ),
    RenderableAnimated()
{
    Aabb aabb( Aabb::BOX_INFINITE );
    mObjectData.mLocalAabb->setFromAabb( aabb, mObjectData.mIndex );
    mObjectData.mWorldAabb->setFromAabb( aabb, mObjectData.mIndex );
    mObjectData.mLocalRadius[mObjectData.mIndex] = std::numeric_limits<Real>::max();
    mObjectData.mWorldRadius[mObjectData.mIndex] = std::numeric_limits<Real>::max();

    mVaoPerLod[VpNormal].push_back( vao );
    mVaoPerLod[VpShadow].push_back( shadowVao );

    mRenderables.push_back( this );
}
//-----------------------------------------------------------------------------------
WarmUpRenderable::~WarmUpRenderable()
{
    // We don't own this ptr, our creator will free it. Set this to nullptr to prevent an assert.
    mSkeletonInstance = 0;
}
//-----------------------------------------------------------------------------------
void WarmUpRenderable::setupSkeleton( SkeletonInstance *instance,
                                      FastArray<unsigned short> *blendIndexToBoneIndexMap )
{
    OGRE_ASSERT_MEDIUM( instance );
    mHasSkeletonAnimation = true;
    mSkeletonInstance = instance;
    mBlendIndexToBoneIndexMap = blendIndexToBoneIndexMap;
}
//-----------------------------------------------------------------------------------
const String &WarmUpRenderable::getMovableType() const { return BLANKSTRING; }
//-----------------------------------------------------------------------------------
const LightList &WarmUpRenderable::getLights() const
{
    return this->queryLights();  // Return the data from our MovableObject base class.
}
//-----------------------------------------------------------------------------------
void WarmUpRenderable::getRenderOperation( v1::RenderOperation &op, bool casterPass )
{
    OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                 "WarmUpRenderable do not implement getRenderOperation."
                 " You've put a v2 object in "
                 "the wrong RenderQueue ID (which is set to be compatible with "
                 "v1::Entity). Do not mix v2 and v1 objects",
                 "WarmUpRenderable::getRenderOperation" );
}
//-----------------------------------------------------------------------------------
void WarmUpRenderable::getWorldTransforms( Matrix4 *xform ) const
{
    OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                 "WarmUpRenderable do not implement getWorldTransforms."
                 " You've put a v2 object in "
                 "the wrong RenderQueue ID (which is set to be compatible with "
                 "v1::Entity). Do not mix v2 and v1 objects",
                 "WarmUpRenderable::getWorldTransforms" );
}
//-----------------------------------------------------------------------------------
bool WarmUpRenderable::getCastsShadows() const
{
    OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                 "WarmUpRenderable do not implement getCastsShadows."
                 " You've put a v2 object in "
                 "the wrong RenderQueue ID (which is set to be compatible with "
                 "v1::Entity). Do not mix v2 and v1 objects",
                 "WarmUpRenderable::getCastsShadows" );
}
