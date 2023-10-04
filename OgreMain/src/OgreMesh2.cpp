/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

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

#include "OgreMesh2.h"

#include "Animation/OgreSkeletonDef.h"
#include "Animation/OgreSkeletonManager.h"
#include "OgreException.h"
#include "OgreHardwareBufferManager.h"
#include "OgreIteratorWrappers.h"
#include "OgreLodStrategyManager.h"
#include "OgreLogManager.h"
#include "OgreMesh.h"
#include "OgreMesh2Serializer.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreMovableObject.h"
#include "OgreOldSkeletonManager.h"
#include "OgreOptimisedUtil.h"
#include "OgrePixelCountLodStrategy.h"
#include "OgreProfiler.h"
#include "OgreSkeleton.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreVertexArrayObject.h"

namespace Ogre
{
    bool Mesh::msOptimizeForShadowMapping = false;
    bool Mesh::msUseTimestampAsHash = false;

    //-----------------------------------------------------------------------
    Mesh::Mesh( ResourceManager *creator, const String &name, ResourceHandle handle, const String &group,
                VaoManager *vaoManager, bool isManual, ManualResourceLoader *loader ) :
        Resource( creator, name, handle, group, isManual, loader ),
        mBoundRadius( 0.0f ),
        mLodStrategyName( LodStrategyManager::getSingleton().getDefaultStrategy()->getName() ),
        mVaoManager( vaoManager ),
        mVertexBufferDefaultType( BT_IMMUTABLE ),
        mIndexBufferDefaultType( BT_IMMUTABLE ),
        mVertexBufferShadowBuffer( true ),
        mIndexBufferShadowBuffer( true )
    {
        memset( mHashForCaches, 0, sizeof( mHashForCaches ) );
        mLodValues.push_back( LodStrategyManager::getSingleton().getDefaultStrategy()->getBaseValue() );
    }
    //-----------------------------------------------------------------------
    Mesh::~Mesh()
    {
        if( !isLoaded() )
        {
            // Even while unloaded we still may have stuff to free
            unloadImpl();
        }
        else
        {
            // have to call this here rather than in Resource destructor
            // since calling virtual methods in base destructors causes crash
            unload();
        }
    }
    //-----------------------------------------------------------------------
    SubMesh *Mesh::createSubMesh( size_t index )
    {
        SubMesh *sub = OGRE_NEW SubMesh();
        sub->mParent = this;

        index = std::min( index, mSubMeshes.size() );
        mSubMeshes.insert( mSubMeshes.begin() + static_cast<ptrdiff_t>( index ), sub );

        if( isLoaded() )
            _dirtyState();

        return sub;
    }
    //-----------------------------------------------------------------------
    void Mesh::destroySubMesh( unsigned index )
    {
        if( index >= mSubMeshes.size() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Index out of bounds.", "Mesh::removeSubMesh" );
        }

        SubMeshVec::iterator itor = mSubMeshes.begin() + index;
        OGRE_DELETE *itor;

        mSubMeshes.erase( itor );

        if( isLoaded() )
            _dirtyState();
    }
    //-----------------------------------------------------------------------
    unsigned Mesh::getNumSubMeshes() const { return static_cast<unsigned>( mSubMeshes.size() ); }
    //-----------------------------------------------------------------------
    SubMesh *Mesh::getSubMesh( unsigned index ) const
    {
        if( index >= mSubMeshes.size() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Index out of bounds.", "Mesh::getSubMesh" );
        }

        return mSubMeshes[index];
    }
    //-----------------------------------------------------------------------
    void Mesh::postLoadImpl()
    {
#if !OGRE_NO_MESHLOD
        // The loading process accesses LOD usages directly, so
        // transformation of user values must occur after loading is complete.

        // LodStrategy *lodStrategy = LodStrategyManager::getSingleton().getDefaultStrategy();

        /*assert( mLodValues.size() == mMeshLodUsageList.size()  );
        LodValueArray::iterator lodValueIt = mLodValues.begin();
        // Transform user LOD values (starting at index 1, no need to transform base value)
        for (MeshLodUsageList::iterator i = mMeshLodUsageList.begin(); i != mMeshLodUsageList.end(); ++i)
        {
            i->value = lodStrategy->transformUserValue(i->userValue);
            *lodValueIt++ = i->value;
        }*/
#endif
    }
    //-----------------------------------------------------------------------
    void Mesh::prepareImpl()
    {
        OgreProfileExhaustive( "Mesh2::prepareImpl" );

        // Load from specified 'name'
        if( getCreator()->getVerbose() )
            LogManager::getSingleton().logMessage( "Mesh: Loading " + mName + "." );

        mFreshFromDisk = ResourceGroupManager::getSingleton().openResource( mName, mGroup, true, this );

        // fully prebuffer into host RAM
        mFreshFromDisk = DataStreamPtr( OGRE_NEW MemoryDataStream( mName, mFreshFromDisk ) );
    }
    //-----------------------------------------------------------------------
    void Mesh::unprepareImpl() { mFreshFromDisk.reset(); }
    //-----------------------------------------------------------------------
    void Mesh::loadImpl()
    {
        OgreProfileExhaustive( "Mesh2::loadImpl" );

        MeshSerializer serializer( mVaoManager );
        // serializer.setListener(MeshManager::getSingleton().getListener());

        // If the only copy is local on the stack, it will be cleaned
        // up reliably in case of exceptions, etc
        DataStreamPtr data( mFreshFromDisk );
        mFreshFromDisk.reset();

        if( !data )
        {
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         "Data doesn't appear to have been prepared in " + mName, "Mesh::loadImpl()" );
        }

        serializer.importMesh( data, this );

        if( mHashForCaches[0] == 0u && mHashForCaches[1] == 0u && Mesh::msUseTimestampAsHash )
        {
            try
            {
                LogManager::getSingleton().logMessage( "Using timestamp as hash cache for Mesh " + mName,
                                                       LML_TRIVIAL );
                Archive *archive =
                    ResourceGroupManager::getSingleton()._getArchiveToResource( mName, mGroup, true );
                mHashForCaches[0] = static_cast<uint64>( archive->getModifiedTime( mName ) );
            }
            catch( Exception & )
            {
                LogManager::getSingleton().logMessage( "Using timestamp as hash cache for Mesh " + mName,
                                                       LML_TRIVIAL );
            }
        }
    }
    //-----------------------------------------------------------------------
    void Mesh::unloadImpl()
    {
        OgreProfileExhaustive( "Mesh2::unloadImpl" );

        // Teardown submeshes
        for( SubMesh *submesh : mSubMeshes )
            OGRE_DELETE submesh;

        mSubMeshes.clear();
#if !OGRE_NO_MESHLOD
        // Removes all LOD data
        removeLodLevels();
#endif

        // Removes reference to skeleton
        setSkeletonName( BLANKSTRING );
    }
    //-----------------------------------------------------------------------
    MeshPtr Mesh::clone( const String &newName, const String &newGroup, int vertexBufferType,
                         int indexBufferType )
    {
        // This is a bit like a copy constructor, but with the additional
        // aspect of registering the clone with the MeshManager

        // New Mesh is assumed to be manually defined rather
        // than loaded since you're cloning it for a reason
        String theGroup;
        if( newGroup.empty() )
            theGroup = this->getGroup();
        else
            theGroup = newGroup;
        MeshPtr newMesh = MeshManager::getSingleton().createManual( newName, theGroup );
        newMesh->mVaoManager = mVaoManager;
        copy( newMesh, vertexBufferType, indexBufferType );
        return newMesh;
    }

    //-----------------------------------------------------------------------
    void Mesh::copy( const MeshPtr &destination, int vertexBufferType, int indexBufferType )
    {
        destination->unload();

        // Copy submeshes first
        for( SubMesh *submesh : mSubMeshes )
            submesh->clone( destination.get(), vertexBufferType, indexBufferType );

        // Copy bounds
        destination->mAabb = mAabb;
        destination->mBoundRadius = mBoundRadius;

        destination->mSkeletonName = mSkeletonName;
        destination->mSkeleton = mSkeleton;

        destination->mLodStrategyName = mLodStrategyName;
        destination->mLodValues = mLodValues;

        destination->mVertexBufferDefaultType = mVertexBufferDefaultType;
        destination->mIndexBufferDefaultType = mIndexBufferDefaultType;
        destination->mVertexBufferShadowBuffer = mVertexBufferShadowBuffer;
        destination->mIndexBufferShadowBuffer = mIndexBufferShadowBuffer;

        // Copy submesh names
        destination->mSubMeshNameMap = mSubMeshNameMap;

        destination->load();
        destination->touch();
    }

    //-----------------------------------------------------------------------
    const Aabb &Mesh::getAabb() const { return mAabb; }
    //-----------------------------------------------------------------------
    void Mesh::_setBounds( const Aabb &bounds, bool pad )
    {
        mAabb = bounds;
        mBoundRadius = mAabb.getRadius();

        if( pad )
        {
            // Pad out the AABB a little, helps with most bounds tests
            mAabb.mHalfSize +=
                2.0f * mAabb.mHalfSize * MeshManager::getSingleton().getBoundsPaddingFactor();
            // Pad out the sphere a little too
            mBoundRadius =
                mBoundRadius + ( mBoundRadius * MeshManager::getSingleton().getBoundsPaddingFactor() );
        }
    }
    //-----------------------------------------------------------------------
    void Mesh::_setBoundingSphereRadius( Real radius ) { mBoundRadius = radius; }
    //-----------------------------------------------------------------------
    void Mesh::_updateBoundsFromVertexBuffers( bool pad )
    {
        /*bool extendOnly = false; // First time we need full AABB of the given submesh, but on the
        second call just extend that one.

        for (size_t i = 0; i < mSubMeshes.size(); i++)
        {
            mSubMeshes[i]->_calcBoundsFromVertexBuffer( mAABB, mBoundRadius, extendOnly );
            extendOnly = true;
        }

        if (pad)
        {
            Vector3 max = mAABB.getMaximum();
            Vector3 min = mAABB.getMinimum();
            // Pad out the AABB a little, helps with most bounds tests
            Vector3 scaler = (max - min) * MeshManager::getSingleton().getBoundsPaddingFactor();
            mAABB.setExtents(min - scaler, max + scaler);
            // Pad out the sphere a little too
            mBoundRadius = mBoundRadius + (mBoundRadius *
        MeshManager::getSingleton().getBoundsPaddingFactor());
        }*/
    }
    //-----------------------------------------------------------------------
    void Mesh::setSkeletonName( const String &skelName )
    {
        if( skelName != mSkeletonName )
        {
            mSkeletonName = skelName;

            if( skelName.empty() )
            {
                // No skeleton
                mSkeleton.reset();
            }
            else
            {
                // Load skeleton
                try
                {
                    mSkeleton = SkeletonManager::getSingleton().getSkeletonDef( skelName, mGroup );
                }
                catch( ... )
                {
                    mSkeleton.reset();
                    // Log this error
                    String msg = "Unable to load skeleton ";
                    msg += skelName + " for Mesh " + mName + ". This Mesh will not be animated. " +
                           "You can ignore this message if you are using an offline tool.";
                    LogManager::getSingleton().logMessage( msg );
                }
            }
            if( isLoaded() )
                _dirtyState();
        }
    }
    //---------------------------------------------------------------------
    void Mesh::_notifySkeleton( v1::SkeletonPtr &pSkel )
    {
        mSkeletonName = pSkel->getName();
        mSkeleton = SkeletonManager::getSingleton().getSkeletonDef( pSkel.get() );
    }
    //---------------------------------------------------------------------
    const String &Mesh::getSkeletonName() const { return mSkeletonName; }
    //---------------------------------------------------------------------
    ushort Mesh::getNumLodLevels() const { return static_cast<uint16>( mLodValues.size() ); }
    //---------------------------------------------------------------------
    void Mesh::_setLodInfo( unsigned short numLevels )
    {
        /*assert(!mEdgeListsBuilt && "Can't modify LOD after edge lists built");

        // Basic prerequisites
        assert(numLevels > 0 && "Must be at least one level (full detail level must exist)");

        mNumLods = numLevels;
        mLodValues.resize(numLevels);*/
        // Resize submesh face data lists too
        /*for( SubMesh *submesh : mSubMeshList )
            submesh->mLodFaceList.resize(numLevels - 1);
        */
    }
    //---------------------------------------------------------------------
    /*void Mesh::_setSubMeshLodFaceList(unsigned short subIdx, unsigned short level,
        IndexData* facedata)
    {
        assert(!mEdgeListsBuilt && "Can't modify LOD after edge lists built");

        // Basic prerequisites
        assert(mMeshLodUsageList[level].manualName.empty() && "Not using generated LODs!");
        assert(subIdx < mSubMeshes.size() && "Index out of bounds");
        assert(level != 0 && "Can't modify first LOD level (full detail)");
        assert(level-1 < (unsigned short)mSubMeshes[subIdx]->mLodFaceList.size() && "Index out of
    bounds");

        SubMesh* sm = mSubMeshes[subIdx];
        sm->mLodFaceList[level - 1] = facedata;
    }*/
    //--------------------------------------------------------------------
    void Mesh::removeLodLevels()
    {
#if !OGRE_NO_MESHLOD
        // Remove data from SubMeshes
        /*for( SubMesh *submesh : mSubMeshList )
            submesh->removeLodLevels();

        freeEdgeList();
        mMeshLodUsageList.clear();
        mLodValues.clear();

        LodStrategy *lodStrategy = LodStrategyManager::getSingleton().getDefaultStrategy();

        // Reinitialise
        mNumLods = 1;
        mMeshLodUsageList.resize(1);
        mMeshLodUsageList[0].edgeData = NULL;
        // TODO: Shouldn't we rebuild edge lists after freeing them?
        mLodValues.push_back( lodStrategy->getBaseValue() );*/
#endif
    }
    //---------------------------------------------------------------------
    void Mesh::_setHashForCaches( const uint64 hash[2] )
    {
        mHashForCaches[0] = hash[0];
        mHashForCaches[1] = hash[1];
    }
    //---------------------------------------------------------------------
    Real Mesh::getBoundingSphereRadius() const { return mBoundRadius; }
    //---------------------------------------------------------------------
    void Mesh::setVertexBufferPolicy( BufferType bufferType, bool shadowBuffer )
    {
        mVertexBufferDefaultType = bufferType;
        mVertexBufferShadowBuffer = shadowBuffer;
    }
    //---------------------------------------------------------------------
    void Mesh::setIndexBufferPolicy( BufferType bufferType, bool shadowBuffer )
    {
        mIndexBufferDefaultType = bufferType;
        mIndexBufferShadowBuffer = shadowBuffer;
    }
    //---------------------------------------------------------------------
    void Mesh::nameSubMesh( const String &name, unsigned index )
    {
        if( index >= 65536 )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Only first 65536 submeshes could be named.",
                         "Mesh::nameSubMesh" );
        }
        mSubMeshNameMap[name] = static_cast<uint16>( index );
    }
    //---------------------------------------------------------------------
    void Mesh::unnameSubMesh( const String &name )
    {
        SubMeshNameMap::iterator i = mSubMeshNameMap.find( name );
        if( i != mSubMeshNameMap.end() )
            mSubMeshNameMap.erase( i );
    }
    //---------------------------------------------------------------------
    size_t Mesh::calculateSize() const
    {
        // calculate GPU size
        size_t retVal = 0;

        for( SubMesh *submesh : mSubMeshes )
        {
            size_t numExtraVaos = 2;

            if( !submesh->mVao[VpNormal].empty() && !submesh->mVao[VpShadow].empty() &&
                submesh->mVao[VpNormal][0] == submesh->mVao[VpShadow][0] )
            {
                numExtraVaos = 1;
            }

            for( size_t i = 0; i < numExtraVaos; ++i )
            {
                VertexArrayObjectArray::const_iterator itVao = submesh->mVao[i].begin();
                VertexArrayObjectArray::const_iterator enVao = submesh->mVao[i].end();

                while( itVao != enVao )
                {
                    VertexArrayObject *vao = *itVao;
                    VertexBufferPackedVec::const_iterator itVertexBuf = vao->getVertexBuffers().begin();
                    VertexBufferPackedVec::const_iterator enVertexBuf = vao->getVertexBuffers().end();

                    while( itVertexBuf != enVertexBuf )
                    {
                        retVal += ( *itVertexBuf )->getTotalSizeBytes();
                        ++itVertexBuf;
                    }

                    if( vao->getIndexBuffer() )
                        retVal += vao->getIndexBuffer()->getTotalSizeBytes();
                    ++itVao;
                }
            }
        }

        return retVal;
    }
    //---------------------------------------------------------------------
    void Mesh::importV1( v1::Mesh *mesh, bool halfPos, bool halfTexCoords, bool qTangents,
                         bool halfPose )
    {
        OgreProfileExhaustive( "Mesh2::importV1" );

        mesh->load();

        if( mLoadingState.get() != LOADSTATE_UNLOADED && mLoadingState.get() != LOADSTATE_LOADING )
        {
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         "To import a v1 mesh, the v2 mesh must be in unloaded state!",
                         "Mesh::importV1" );
        }

        if( mesh->sharedVertexData[VpNormal] )
        {
            LogManager::getSingleton().logMessage( "WARNING: Mesh '" + mesh->getName() +
                                                   "' has shared vertices. They're being "
                                                   "'unshared' for importing to v2" );
            v1::MeshManager::unshareVertices( mesh );
        }

        mAabb.setExtents( mesh->getBounds().getMinimum(), mesh->getBounds().getMaximum() );
        mBoundRadius = mesh->getBoundingSphereRadius();

        try
        {
            if( qTangents )
            {
                unsigned short sourceCoordSet;
                unsigned short index;
                bool alreadyHasTangents =
                    mesh->suggestTangentVectorBuildParams( VES_TANGENT, sourceCoordSet, index );
                if( !alreadyHasTangents )
                    mesh->buildTangentVectors( VES_TANGENT, sourceCoordSet, index, false, false, true );
            }
        }
        catch( Exception & )
        {
        }

        for( unsigned i = 0; i < mesh->getNumSubMeshes(); ++i )
        {
            SubMesh *subMesh = createSubMesh();
            subMesh->importFromV1( mesh->getSubMesh( i ), halfPos, halfTexCoords, qTangents, halfPose );
        }

        mSubMeshNameMap = mesh->getSubMeshNameMap();

        mSkeletonName = mesh->getSkeletonName();
        v1::SkeletonPtr v1Skeleton = mesh->getOldSkeleton();
        if( v1Skeleton )
            mSkeleton = SkeletonManager::getSingleton().getSkeletonDef( v1Skeleton.get() );

        // So far we only import manual LOD levels. If the mesh had manual LOD levels,
        // mLodValues will have more entries than Vaos, causing an out of bounds exception.
        // Don't use LOD if the imported mesh had manual levels.
        // Note: Mesh2 supports LOD levels that have their own vertex and index buffers,
        // so it should be possible to import them as well.
        if( !mesh->hasManualLodLevel() )
            mLodValues = *mesh->_getLodValueArray();
        else
            mLodValues = MovableObject::c_DefaultLodMesh;

        const uint64 *hash = mesh->getHashForCaches();
        mHashForCaches[0] = hash[0];
        mHashForCaches[1] = hash[1];

        mIsManual = true;
        setToLoaded();
    }
    //---------------------------------------------------------------------
    void Mesh::arrangeEfficient( bool halfPos, bool halfTexCoords, bool qTangents )
    {
        for( SubMesh *submesh : mSubMeshes )
            submesh->arrangeEfficient( halfPos, halfTexCoords, qTangents );
    }
    //---------------------------------------------------------------------
    void Mesh::dearrangeToInefficient()
    {
        for( SubMesh *submesh : mSubMeshes )
            submesh->dearrangeToInefficient();
    }
    //---------------------------------------------------------------------
    void Mesh::prepareForShadowMapping( bool forceSameBuffers )
    {
        OgreProfileExhaustive( "Mesh2::prepareForShadowMapping" );

        for( SubMesh *submesh : mSubMeshes )
            submesh->_prepareForShadowMapping( forceSameBuffers );
    }
    //---------------------------------------------------------------------
    bool Mesh::hasValidShadowMappingVaos() const
    {
        for( SubMesh *submesh : mSubMeshes )
        {
            if( submesh->mVao[VpNormal].size() != submesh->mVao[VpShadow].size() )
                return false;
        }

        return true;
    }
    //---------------------------------------------------------------------
    bool Mesh::hasIndependentShadowMappingVaos() const
    {
        if( !hasValidShadowMappingVaos() )
            return false;

        bool independent = false;

        for( SubMesh *submesh : mSubMeshes )
        {
            if( !independent && !submesh->mVao[VpNormal].empty() &&
                submesh->mVao[VpNormal][0] != submesh->mVao[VpShadow][0] )
            {
                independent = true;
                break;
            }
        }

        return independent;
    }
    //---------------------------------------------------------------------
}  // namespace Ogre
