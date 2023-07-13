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

#include "OgreMesh.h"

#include "Animation/OgreSkeletonDef.h"
#include "Animation/OgreSkeletonManager.h"
#include "OgreAnimation.h"
#include "OgreAnimationState.h"
#include "OgreAnimationTrack.h"
#include "OgreEdgeListBuilder.h"
#include "OgreException.h"
#include "OgreHardwareBufferManager.h"
#include "OgreIteratorWrappers.h"
#include "OgreLodStrategyManager.h"
#include "OgreLogManager.h"
#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshSerializer.h"
#include "OgreOldBone.h"
#include "OgreOldSkeletonManager.h"
#include "OgreOptimisedUtil.h"
#include "OgrePixelCountLodStrategy.h"
#include "OgreProfiler.h"
#include "OgreSkeleton.h"
#include "OgreStringConverter.h"
#include "OgreSubMesh.h"
#include "OgreTangentSpaceCalc.h"
#include "OgreVertexShadowMapHelper.h"

namespace Ogre
{
    namespace v1
    {
        bool Mesh::msOptimizeForShadowMapping = false;

        //-----------------------------------------------------------------------
        Mesh::Mesh( ResourceManager *creator, const String &name, ResourceHandle handle,
                    const String &group, bool isManual, ManualResourceLoader *loader ) :
            Resource( creator, name, handle, group, isManual, loader ),
            mBoundRadius( 0.0f ),
            mBoneBoundingRadius( 0.0f ),
            mBoneAssignmentsOutOfDate( false ),
            mLodStrategyName( LodStrategyManager::getSingleton().getDefaultStrategy()->getName() ),
            mHasManualLodLevel( false ),
            mNumLods( 1 ),
            mBufferManager( 0 ),
            mVertexBufferUsage( HardwareBuffer::HBU_STATIC_WRITE_ONLY ),
            mIndexBufferUsage( HardwareBuffer::HBU_STATIC_WRITE_ONLY ),
            mVertexBufferShadowBuffer( true ),
            mIndexBufferShadowBuffer( true ),
            mPreparedForShadowVolumes( false ),
            mEdgeListsBuilt( false ),
            mAutoBuildEdgeLists( true ),  // will be set to false by serializers of 1.30 and above
            mSharedVertexDataAnimationType( VAT_NONE ),
            mSharedVertexDataAnimationIncludesNormals( false ),
            mAnimationTypesDirty( true ),
            mPosesIncludeNormals( false )
        {
            memset( mHashForCaches, 0, sizeof( mHashForCaches ) );
            memset( sharedVertexData, 0, sizeof( sharedVertexData ) );

            // Init first (manual) lod
            MeshLodUsage lod;
            lod.userValue = 0;  // User value not used for base LOD level
            lod.value = LodStrategyManager::getSingleton().getDefaultStrategy()->getBaseValue();
            lod.edgeData = NULL;
            lod.manualMesh.reset();
            mMeshLodUsageList.push_back( lod );
            mLodValues.push_back( lod.value );
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
        HardwareBufferManagerBase *Mesh::getHardwareBufferManager()
        {
            return mBufferManager ? mBufferManager : HardwareBufferManager::getSingletonPtr();
        }
        //-----------------------------------------------------------------------
        SubMesh *Mesh::createSubMesh()
        {
            SubMesh *sub = OGRE_NEW SubMesh();
            sub->parent = this;

            mSubMeshList.push_back( sub );

            if( isLoaded() )
                _dirtyState();

            return sub;
        }
        //-----------------------------------------------------------------------
        SubMesh *Mesh::createSubMesh( const String &name )
        {
            if( mSubMeshList.size() >= 65536 )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Only first 65536 submeshes could be named.",
                             "Mesh::createSubMesh" );
            }
            SubMesh *sub = createSubMesh();
            nameSubMesh( name, (unsigned)( mSubMeshList.size() - 1u ) );
            return sub;
        }
        //-----------------------------------------------------------------------
        void Mesh::destroySubMesh( unsigned index )
        {
            if( index >= mSubMeshList.size() )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Index out of bounds.",
                             "Mesh::removeSubMesh" );
            }
            SubMeshList::iterator i = mSubMeshList.begin();
            std::advance( i, index );
            OGRE_DELETE *i;
            mSubMeshList.erase( i );

            // Fix up any name/index entries
            for( SubMeshNameMap::iterator ni = mSubMeshNameMap.begin(); ni != mSubMeshNameMap.end(); )
            {
                if( ni->second == index )
                {
                    SubMeshNameMap::iterator eraseIt = ni++;
                    mSubMeshNameMap.erase( eraseIt );
                }
                else
                {
                    // reduce indexes following
                    if( ni->second > index )
                        ni->second = ni->second - 1;

                    ++ni;
                }
            }

            // fix edge list data by simply recreating all edge lists
            if( mEdgeListsBuilt )
            {
                this->freeEdgeList();
                this->buildEdgeList();
            }

            if( isLoaded() )
                _dirtyState();
        }
        //-----------------------------------------------------------------------
        void Mesh::destroySubMesh( const String &name )
        {
            unsigned index = _getSubMeshIndex( name );
            destroySubMesh( index );
        }
        //-----------------------------------------------------------------------
        unsigned Mesh::getNumSubMeshes() const { return static_cast<unsigned>( mSubMeshList.size() ); }

        //---------------------------------------------------------------------
        void Mesh::nameSubMesh( const String &name, unsigned index )
        {
            if( index >= 65536 )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Only first 65536 submeshes could be named.",
                             "Mesh::nameSubMesh" );
            }
            mSubMeshNameMap[name] = (uint16)index;
        }

        //---------------------------------------------------------------------
        void Mesh::unnameSubMesh( const String &name )
        {
            SubMeshNameMap::iterator i = mSubMeshNameMap.find( name );
            if( i != mSubMeshNameMap.end() )
                mSubMeshNameMap.erase( i );
        }
        //-----------------------------------------------------------------------
        SubMesh *Mesh::getSubMesh( const String &name ) const
        {
            unsigned index = _getSubMeshIndex( name );
            return getSubMesh( index );
        }
        //-----------------------------------------------------------------------
        SubMesh *Mesh::getSubMesh( unsigned index ) const
        {
            if( index >= mSubMeshList.size() )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Index out of bounds.", "Mesh::getSubMesh" );
            }

            return mSubMeshList[index];
        }
        //-----------------------------------------------------------------------
        void Mesh::postLoadImpl()
        {
            OgreProfileExhaustive( "v1::Mesh::postLoadImpl" );

            // Prepare for shadow volumes?
            if( MeshManager::getSingleton().getPrepareAllMeshesForShadowVolumes() )
            {
                if( mEdgeListsBuilt || mAutoBuildEdgeLists )
                {
                    prepareForShadowVolume();
                }

                if( !mEdgeListsBuilt && mAutoBuildEdgeLists )
                {
                    buildEdgeList();
                }
            }
#if !OGRE_NO_MESHLOD
            // The loading process accesses LOD usages directly, so
            // transformation of user values must occur after loading is complete.

            LodStrategy *lodStrategy = LodStrategyManager::getSingleton().getDefaultStrategy();

            assert( mLodValues.size() == mMeshLodUsageList.size() );
            LodValueArray::iterator lodValueIt = mLodValues.begin();
            // Transform user LOD values (starting at index 1, no need to transform base value)
            for( MeshLodUsage &usage : mMeshLodUsageList )
            {
                usage.value = lodStrategy->transformUserValue( usage.userValue );
                *lodValueIt++ = usage.value;
            }
            if( !mLodValues.empty() )
            {
                // Rewrite first value
                mLodValues[0] = lodStrategy->getBaseValue();
                mMeshLodUsageList[0].value = lodStrategy->getBaseValue();
            }
#endif
        }
        //-----------------------------------------------------------------------
        void Mesh::prepareImpl()
        {
            OgreProfileExhaustive( "v1::Mesh::prepareImpl" );

            // Load from specified 'name'
            if( getCreator()->getVerbose() )
                LogManager::getSingleton().logMessage( "Mesh: Loading " + mName + "." );

            mFreshFromDisk =
                ResourceGroupManager::getSingleton().openResource( mName, mGroup, true, this );

            // fully prebuffer into host RAM
            mFreshFromDisk = DataStreamPtr( OGRE_NEW MemoryDataStream( mName, mFreshFromDisk ) );
        }
        //-----------------------------------------------------------------------
        void Mesh::unprepareImpl() { mFreshFromDisk.reset(); }
        void Mesh::loadImpl()
        {
            OgreProfileExhaustive( "v1::Mesh::loadImpl" );

            MeshSerializer serializer;
            serializer.setListener( MeshManager::getSingleton().getListener() );

            // If the only copy is local on the stack, it will be cleaned
            // up reliably in case of exceptions, etc
            DataStreamPtr data( mFreshFromDisk );
            mFreshFromDisk.reset();

            if( !data )
            {
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                             "Data doesn't appear to have been prepared in " + mName,
                             "Mesh::loadImpl()" );
            }

            serializer.importMesh( data, this );

            if( mHashForCaches[0] == 0u && mHashForCaches[1] == 0u && Ogre::Mesh::msUseTimestampAsHash )
            {
                try
                {
                    LogManager::getSingleton().logMessage(
                        "Using timestamp as hash cache for Mesh " + mName, LML_TRIVIAL );
                    Archive *archive = ResourceGroupManager::getSingleton()._getArchiveToResource(
                        mName, mGroup, true );
                    mHashForCaches[0] = static_cast<uint64>( archive->getModifiedTime( mName ) );
                }
                catch( Exception & )
                {
                    LogManager::getSingleton().logMessage(
                        "Using timestamp as hash cache for Mesh " + mName, LML_TRIVIAL );
                }
            }

            /* check all submeshes to see if their materials should be
               updated.  If the submesh has texture aliases that match those
               found in the current material then a new material is created using
               the textures from the submesh.
            */
            updateMaterialForAllSubMeshes();
        }

        //-----------------------------------------------------------------------
        void Mesh::unloadImpl()
        {
            // Teardown submeshes
            OgreProfileExhaustive( "v1::Mesh::unloadImpl" );

            // mSubMeshList is iterated in submesh destructor, so pop after deleting
            while( !mSubMeshList.empty() )
            {
                OGRE_DELETE mSubMeshList.back();
                mSubMeshList.pop_back();
            }

            if( sharedVertexData[VpNormal] == sharedVertexData[VpShadow] )
                sharedVertexData[VpShadow] = 0;

            for( size_t i = 0; i < NumVertexPass; ++i )
            {
                OGRE_DELETE sharedVertexData[i];
                sharedVertexData[i] = 0;
            }

            // Clear SubMesh lists
            mSubMeshList.clear();
            mSubMeshNameMap.clear();

            freeEdgeList();
#if !OGRE_NO_MESHLOD
            // Removes all LOD data
            removeLodLevels();
#endif
            mPreparedForShadowVolumes = false;

            // remove all poses & animations
            removeAllAnimations();
            removeAllPoses();

            // Clear bone assignments
            mBoneAssignments.clear();
            mBoneAssignmentsOutOfDate = false;

            // Removes reference to skeleton
            setSkeletonName( BLANKSTRING );
        }
        //-----------------------------------------------------------------------
        void Mesh::reload( LoadingFlags flags )
        {
            bool wasPreparedForShadowVolumes = mPreparedForShadowVolumes;
            bool wasEdgeListsBuilt = mEdgeListsBuilt;
            bool wasAutoBuildEdgeLists = mAutoBuildEdgeLists;

            Resource::reload( flags );

            if( flags & LF_PRESERVE_STATE )
            {
                if( wasPreparedForShadowVolumes )
                    prepareForShadowVolume();
                if( wasEdgeListsBuilt )
                    buildEdgeList();
                setAutoBuildEdgeLists( wasAutoBuildEdgeLists );
            }
        }
        //-----------------------------------------------------------------------
        void Mesh::importV2( Ogre::Mesh *mesh )
        {
            if( mLoadingState.get() != LOADSTATE_UNLOADED )
            {
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                             "To import a v2 mesh, the v1 mesh must be in unloaded state!",
                             "v1::Mesh::importV2" );
            }

            mAABB.setMinimum( mesh->getAabb().getMinimum() );
            mAABB.setMaximum( mesh->getAabb().getMaximum() );
            mBoundRadius = mesh->getBoundingSphereRadius();
            mBoneBoundingRadius = mBoundRadius;

            for( unsigned i = 0; i < mesh->getNumSubMeshes(); ++i )
            {
                SubMesh *subMesh = createSubMesh();
                subMesh->importFromV2( mesh->getSubMesh( i ) );
            }

            mSubMeshNameMap = mesh->getSubMeshNameMap();

            mLodValues = *mesh->_getLodValueArray();

            mIsManual = true;

            if( !mesh->hasIndependentShadowMappingVaos() )
                prepareForShadowMapping( true );

            setSkeletonName( mesh->getSkeletonName() );

            setToLoaded();
        }
        //-----------------------------------------------------------------------
        void Mesh::arrangeEfficient( bool halfPos, bool halfTexCoords, bool qTangents )
        {
            if( sharedVertexData[VpNormal] )
            {
                LogManager::getSingleton().logMessage( "WARNING: Mesh '" + this->getName() +
                                                       "' has shared vertices. They're being "
                                                       "'unshared' in Mesh::arrangeEfficient" );
                v1::MeshManager::unshareVertices( this );
            }

            for( SubMesh *submesh : mSubMeshList )
                submesh->arrangeEfficient( halfPos, halfTexCoords, qTangents );
        }
        //-----------------------------------------------------------------------
        void Mesh::dearrangeToInefficient()
        {
            if( sharedVertexData[VpNormal] )
            {
                OGRE_EXCEPT( Exception::ERR_INVALID_CALL,
                             "Meshes with shared vertex buffers can't be dearranged.",
                             "v1::Mesh::dearrangeToInefficient" );
            }

            for( SubMesh *submesh : mSubMeshList )
                submesh->dearrangeToInefficient();
        }
        //-----------------------------------------------------------------------
        MeshPtr Mesh::clone( const String &newName, const String &newGroup )
        {
            // This is a bit like a copy constructor, but with the additional aspect of registering the
            // clone with
            //  the MeshManager

            // New Mesh is assumed to be manually defined rather than loaded since you're cloning it for
            // a reason
            String theGroup;
            if( newGroup == BLANKSTRING )
            {
                theGroup = this->getGroup();
            }
            else
            {
                theGroup = newGroup;
            }
            MeshPtr newMesh = MeshManager::getSingleton().createManual( newName, theGroup );

            newMesh->mBufferManager = mBufferManager;
            newMesh->mVertexBufferUsage = mVertexBufferUsage;
            newMesh->mIndexBufferUsage = mIndexBufferUsage;
            newMesh->mVertexBufferShadowBuffer = mVertexBufferShadowBuffer;
            newMesh->mIndexBufferShadowBuffer = mIndexBufferShadowBuffer;

            // Copy submeshes first
            for( SubMesh *submesh : mSubMeshList )
                submesh->clone( "", newMesh.get() );

            // Copy shared geometry and index map, if any
            if( sharedVertexData[VpNormal] )
            {
                newMesh->sharedVertexData[VpNormal] =
                    sharedVertexData[VpNormal]->clone( true, mBufferManager );
                if( sharedVertexData[VpNormal] == sharedVertexData[VpShadow] )
                    newMesh->sharedVertexData[VpShadow] = newMesh->sharedVertexData[VpNormal];
                else
                    newMesh->sharedVertexData[VpShadow] =
                        sharedVertexData[VpShadow]->clone( true, mBufferManager );

                newMesh->sharedBlendIndexToBoneIndexMap = sharedBlendIndexToBoneIndexMap;
            }

            // Copy submesh names
            newMesh->mSubMeshNameMap = mSubMeshNameMap;
            // Copy any bone assignments
            newMesh->mBoneAssignments = mBoneAssignments;
            newMesh->mBoneAssignmentsOutOfDate = mBoneAssignmentsOutOfDate;
            // Copy bounds
            newMesh->mAABB = mAABB;
            newMesh->mBoundRadius = mBoundRadius;
            newMesh->mBoneBoundingRadius = mBoneBoundingRadius;
            newMesh->mAutoBuildEdgeLists = mAutoBuildEdgeLists;
            newMesh->mEdgeListsBuilt = mEdgeListsBuilt;

            newMesh->mLodStrategyName = mLodStrategyName;
            newMesh->mHasManualLodLevel = mHasManualLodLevel;
            newMesh->mNumLods = mNumLods;
            newMesh->mMeshLodUsageList = mMeshLodUsageList;
            newMesh->mLodValues = mLodValues;

            // Unreference edge lists, otherwise we'll delete the same lot twice, build on demand
            MeshLodUsageList::iterator lodIt = mMeshLodUsageList.begin();
            for( MeshLodUsage &newLod : newMesh->mMeshLodUsageList )
            {
                MeshLodUsage &lod = *lodIt++;
                newLod.manualName = lod.manualName;
                newLod.userValue = lod.userValue;
                newLod.value = lod.value;
                if( lod.edgeData )
                    newLod.edgeData = lod.edgeData->clone();
            }

            newMesh->mSkeletonName = mSkeletonName;
            newMesh->mOldSkeleton = mOldSkeleton;
            newMesh->mSkeleton = mSkeleton;

            // Keep prepared shadow volume info (buffers may already be prepared)
            newMesh->mPreparedForShadowVolumes = mPreparedForShadowVolumes;

            newMesh->mEdgeListsBuilt = mEdgeListsBuilt;

            // Clone vertex animation
            for( AnimationList::iterator i = mAnimationsList.begin(); i != mAnimationsList.end(); ++i )
            {
                Animation *newAnim = i->second->clone( i->second->getName() );
                newMesh->mAnimationsList[i->second->getName()] = newAnim;
            }
            // Clone pose list
            for( Pose *pose : mPoseList )
                newMesh->mPoseList.push_back( pose->clone() );

            newMesh->mSharedVertexDataAnimationType = mSharedVertexDataAnimationType;
            newMesh->mAnimationTypesDirty = true;

            newMesh->load();
            newMesh->touch();

            return newMesh;
        }
        //-----------------------------------------------------------------------
        const AxisAlignedBox &Mesh::getBounds() const { return mAABB; }
        //-----------------------------------------------------------------------
        void Mesh::_setBounds( const AxisAlignedBox &bounds, bool pad )
        {
            mAABB = bounds;
            mBoundRadius = Math::boundingRadiusFromAABB( mAABB );

            if( mAABB.isFinite() )
            {
                Vector3 max = mAABB.getMaximum();
                Vector3 min = mAABB.getMinimum();

                if( pad )
                {
                    // Pad out the AABB a little, helps with most bounds tests
                    Vector3 scaler =
                        ( max - min ) * MeshManager::getSingleton().getBoundsPaddingFactor();
                    mAABB.setExtents( min - scaler, max + scaler );
                    // Pad out the sphere a little too
                    mBoundRadius =
                        mBoundRadius +
                        ( mBoundRadius * MeshManager::getSingleton().getBoundsPaddingFactor() );
                }
            }
        }
        //-----------------------------------------------------------------------
        void Mesh::_setBoundingSphereRadius( Real radius ) { mBoundRadius = radius; }
        //-----------------------------------------------------------------------
        void Mesh::_setBoneBoundingRadius( Real radius ) { mBoneBoundingRadius = radius; }
        //-----------------------------------------------------------------------
        void Mesh::_updateBoundsFromVertexBuffers( bool pad )
        {
            bool extendOnly = false;  // First time we need full AABB of the given submesh, but on the
                                      // second call just extend that one.
            if( sharedVertexData[VpShadow] )
            {
                _calcBoundsFromVertexBuffer( sharedVertexData[VpShadow], mAABB, mBoundRadius,
                                             extendOnly );
                extendOnly = true;
            }
            for( size_t i = 0; i < mSubMeshList.size(); i++ )
            {
                if( mSubMeshList[i]->vertexData[VpShadow] )
                {
                    _calcBoundsFromVertexBuffer( mSubMeshList[i]->vertexData[VpShadow], mAABB,
                                                 mBoundRadius, extendOnly );
                    extendOnly = true;
                }
            }
            if( pad )
            {
                Vector3 max = mAABB.getMaximum();
                Vector3 min = mAABB.getMinimum();
                // Pad out the AABB a little, helps with most bounds tests
                Vector3 scaler = ( max - min ) * MeshManager::getSingleton().getBoundsPaddingFactor();
                mAABB.setExtents( min - scaler, max + scaler );
                // Pad out the sphere a little too
                mBoundRadius = mBoundRadius +
                               ( mBoundRadius * MeshManager::getSingleton().getBoundsPaddingFactor() );
            }
        }
        void Mesh::_calcBoundsFromVertexBuffer( VertexData *vertexData, AxisAlignedBox &outAABB,
                                                Real &outRadius, bool extendOnly /*= false*/ )
        {
            if( vertexData->vertexCount == 0 )
            {
                if( !extendOnly )
                {
                    outAABB = AxisAlignedBox( Vector3::ZERO, Vector3::ZERO );
                    outRadius = 0;
                }
                return;
            }
            const VertexElement *elemPos =
                vertexData->vertexDeclaration->findElementBySemantic( VES_POSITION );
            HardwareVertexBufferSharedPtr vbuf =
                vertexData->vertexBufferBinding->getBuffer( elemPos->getSource() );
            HardwareBufferLockGuard vertexLock( vbuf, HardwareBuffer::HBL_READ_ONLY );
            unsigned char *vertex = static_cast<unsigned char *>( vertexLock.pData );

            if( !extendOnly )
            {
                // init values
                outRadius = 0;
                float *pFloat;
                elemPos->baseVertexPointerToElement( vertex, &pFloat );
                Vector3 basePos( pFloat[0], pFloat[1], pFloat[2] );
                outAABB.setExtents( basePos, basePos );
            }
            size_t vSize = vbuf->getVertexSize();
            unsigned char *vEnd = vertex + vertexData->vertexCount * vSize;
            Real radiusSqr = outRadius * outRadius;
            // Loop through all vertices.
            for( ; vertex < vEnd; vertex += vSize )
            {
                float *pFloat;
                elemPos->baseVertexPointerToElement( vertex, &pFloat );
                Vector3 pos( pFloat[0], pFloat[1], pFloat[2] );
                outAABB.getMinimum().makeFloor( pos );
                outAABB.getMaximum().makeCeil( pos );
                radiusSqr = std::max<Real>( radiusSqr, pos.squaredLength() );
            }
            outRadius = std::sqrt( radiusSqr );
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
                    mOldSkeleton.reset();
                    mSkeleton.reset();
                }
                else
                {
                    // Load skeleton
                    try
                    {
                        mOldSkeleton = std::static_pointer_cast<Skeleton>(
                            OldSkeletonManager::getSingleton().load( skelName, mGroup ) );

                        // TODO: put mOldSkeleton in legacy mode only.
                        mSkeleton = SkeletonManager::getSingleton().getSkeletonDef( mOldSkeleton.get() );
                    }
                    catch( ... )
                    {
                        mOldSkeleton.reset();
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
        //-----------------------------------------------------------------------
        bool Mesh::hasSkeleton() const { return !( mSkeletonName.empty() ); }
        //-----------------------------------------------------------------------
        const SkeletonPtr &Mesh::getOldSkeleton() const { return mOldSkeleton; }
        //-----------------------------------------------------------------------
        void Mesh::addBoneAssignment( const VertexBoneAssignment &vertBoneAssign )
        {
            mBoneAssignments.emplace( vertBoneAssign.vertexIndex, vertBoneAssign );
            mBoneAssignmentsOutOfDate = true;
        }
        //-----------------------------------------------------------------------
        void Mesh::clearBoneAssignments()
        {
            mBoneAssignments.clear();
            mBoneAssignmentsOutOfDate = true;
        }
        //-----------------------------------------------------------------------
        void Mesh::_initAnimationState( AnimationStateSet *animSet )
        {
            // Animation states for skeletal animation
            if( mOldSkeleton )
            {
                // Delegate to Skeleton
                mOldSkeleton->_initAnimationState( animSet );

                // Take the opportunity to update the compiled bone assignments
                _updateCompiledBoneAssignments();
            }

            // Animation states for vertex animation
            for( AnimationList::iterator i = mAnimationsList.begin(); i != mAnimationsList.end(); ++i )
            {
                // Only create a new animation state if it doesn't exist
                // We can have the same named animation in both skeletal and vertex
                // with a shared animation state affecting both, for combined effects
                // The animations should be the same length if this feature is used!
                if( !animSet->hasAnimationState( i->second->getName() ) )
                {
                    animSet->createAnimationState( i->second->getName(), 0.0, i->second->getLength() );
                }
            }
        }
        //---------------------------------------------------------------------
        void Mesh::_refreshAnimationState( AnimationStateSet *animSet )
        {
            if( mOldSkeleton )
            {
                mOldSkeleton->_refreshAnimationState( animSet );
            }

            // Merge in any new vertex animations
            AnimationList::iterator i;
            for( i = mAnimationsList.begin(); i != mAnimationsList.end(); ++i )
            {
                Animation *anim = i->second;
                // Create animation at time index 0, default params mean this has weight 1 and is
                // disabled
                const String &animName = anim->getName();
                if( !animSet->hasAnimationState( animName ) )
                {
                    animSet->createAnimationState( animName, 0.0, anim->getLength() );
                }
                else
                {
                    // Update length incase changed
                    AnimationState *animState = animSet->getAnimationState( animName );
                    animState->setLength( anim->getLength() );
                    animState->setTimePosition(
                        std::min( anim->getLength(), animState->getTimePosition() ) );
                }
            }
        }
        //-----------------------------------------------------------------------
        void Mesh::_updateCompiledBoneAssignments()
        {
            if( mBoneAssignmentsOutOfDate )
                _compileBoneAssignments();

            for( SubMesh *submesh : mSubMeshList )
            {
                if( submesh->mBoneAssignmentsOutOfDate )
                    submesh->_compileBoneAssignments();
            }

            prepareForShadowMapping( false );
        }
        //-----------------------------------------------------------------------
        typedef multimap<Real, Mesh::VertexBoneAssignmentList::iterator>::type WeightIteratorMap;
        unsigned short Mesh::_rationaliseBoneAssignments( size_t vertexCount,
                                                          Mesh::VertexBoneAssignmentList &assignments )
        {
            // Iterate through, finding the largest # bones per vertex
            unsigned short maxBones = 0;
            bool existsNonSkinnedVertices = false;
            VertexBoneAssignmentList::iterator i;

            for( size_t v = 0; v < vertexCount; ++v )
            {
                // Get number of entries for this vertex
                const uint16 currBones = static_cast<uint16>( assignments.count( v ) );
                if( currBones == 0 )
                    existsNonSkinnedVertices = true;

                // Deal with max bones update
                // (note this will record maxBones even if they exceed limit)
                if( maxBones < currBones )
                    maxBones = currBones;
                // does the number of bone assignments exceed limit?
                if( currBones > OGRE_MAX_BLEND_WEIGHTS )
                {
                    // To many bone assignments on this vertex
                    // Find the start & end (end is in iterator terms ie exclusive)
                    std::pair<VertexBoneAssignmentList::iterator, VertexBoneAssignmentList::iterator>
                        range;
                    // map to sort by weight
                    WeightIteratorMap weightToAssignmentMap;
                    range = assignments.equal_range( v );
                    // Add all the assignments to map
                    for( i = range.first; i != range.second; ++i )
                    {
                        // insert value weight->iterator
                        weightToAssignmentMap.insert(
                            WeightIteratorMap::value_type( i->second.weight, i ) );
                    }
                    // Reverse iterate over weight map, remove lowest n
                    unsigned short numToRemove = currBones - OGRE_MAX_BLEND_WEIGHTS;
                    WeightIteratorMap::iterator remIt = weightToAssignmentMap.begin();

                    while( numToRemove-- )
                    {
                        // Erase this one
                        assignments.erase( remIt->second );
                        ++remIt;
                    }
                }  // if (currBones > OGRE_MAX_BLEND_WEIGHTS)

                // Make sure the weights are normalised
                // Do this irrespective of whether we had to remove assignments or not
                //   since it gives us a guarantee that weights are normalised
                //  We assume this, so it's a good idea since some modellers may not
                std::pair<VertexBoneAssignmentList::iterator, VertexBoneAssignmentList::iterator>
                    normalise_range = assignments.equal_range( v );
                Real totalWeight = 0;
                // Find total first
                for( i = normalise_range.first; i != normalise_range.second; ++i )
                {
                    totalWeight += i->second.weight;
                }
                // Now normalise if total weight is outside tolerance
                if( !Math::RealEqual( totalWeight, 1.0f ) )
                {
                    for( i = normalise_range.first; i != normalise_range.second; ++i )
                    {
                        i->second.weight = i->second.weight / totalWeight;
                    }
                }
            }

            if( maxBones > OGRE_MAX_BLEND_WEIGHTS )
            {
                // Warn that we've reduced bone assignments
                LogManager::getSingleton().logMessage(
                    "WARNING: the mesh '" + mName +
                        "' "
                        "includes vertices with more than " +
                        StringConverter::toString( OGRE_MAX_BLEND_WEIGHTS ) +
                        " bone assignments. "
                        "The lowest weighted assignments beyond this limit have been removed, so "
                        "your animation may look slightly different. To eliminate this, reduce "
                        "the number of bone assignments per vertex on your mesh to " +
                        StringConverter::toString( OGRE_MAX_BLEND_WEIGHTS ) + ".",
                    LML_CRITICAL );
                // we've adjusted them down to the max
                maxBones = OGRE_MAX_BLEND_WEIGHTS;
            }

            if( existsNonSkinnedVertices )
            {
                // Warn that we've non-skinned vertices
                LogManager::getSingleton().logMessage(
                    "WARNING: the mesh '" + mName +
                        "' "
                        "includes vertices without bone assignments. Those vertices will "
                        "transform to wrong position when skeletal animation enabled. "
                        "To eliminate this, assign at least one bone assignment per vertex "
                        "on your mesh.",
                    LML_CRITICAL );
            }

            return maxBones;
        }
        //-----------------------------------------------------------------------
        void Mesh::_compileBoneAssignments()
        {
            if( sharedVertexData[VpNormal] )
            {
                unsigned short maxBones = _rationaliseBoneAssignments(
                    sharedVertexData[VpNormal]->vertexCount, mBoneAssignments );

                if( maxBones != 0 )
                {
                    compileBoneAssignments( mBoneAssignments, maxBones, sharedBlendIndexToBoneIndexMap,
                                            sharedVertexData[VpNormal] );
                }
            }
            mBoneAssignmentsOutOfDate = false;
        }
        //---------------------------------------------------------------------
        void Mesh::buildIndexMap( const VertexBoneAssignmentList &boneAssignments,
                                  IndexMap &boneIndexToBlendIndexMap,
                                  IndexMap &blendIndexToBoneIndexMap )
        {
            if( boneAssignments.empty() )
            {
                // Just in case
                boneIndexToBlendIndexMap.clear();
                blendIndexToBoneIndexMap.clear();
                return;
            }

            typedef set<unsigned short>::type BoneIndexSet;
            BoneIndexSet usedBoneIndices;

            // Collect actually used bones
            VertexBoneAssignmentList::const_iterator itVBA, itendVBA;
            itendVBA = boneAssignments.end();
            for( itVBA = boneAssignments.begin(); itVBA != itendVBA; ++itVBA )
            {
                usedBoneIndices.insert( itVBA->second.boneIndex );
            }

            // Allocate space for index map
            blendIndexToBoneIndexMap.resize( usedBoneIndices.size() );
            boneIndexToBlendIndexMap.resize( *usedBoneIndices.rbegin() + 1 );

            // Make index map between bone index and blend index
            BoneIndexSet::const_iterator itBoneIndex, itendBoneIndex;
            unsigned short blendIndex = 0;
            itendBoneIndex = usedBoneIndices.end();
            for( itBoneIndex = usedBoneIndices.begin(); itBoneIndex != itendBoneIndex;
                 ++itBoneIndex, ++blendIndex )
            {
                boneIndexToBlendIndexMap[*itBoneIndex] = blendIndex;
                blendIndexToBoneIndexMap[blendIndex] = *itBoneIndex;
            }
        }
        //---------------------------------------------------------------------
        void Mesh::compileBoneAssignments( const VertexBoneAssignmentList &boneAssignments,
                                           unsigned short numBlendWeightsPerVertex,
                                           IndexMap &blendIndexToBoneIndexMap,
                                           VertexData *targetVertexData )
        {
            // Create or reuse blend weight / indexes buffer
            // Indices are always a UBYTE4 no matter how many weights per vertex
            // Weights are more specific though since they are Reals
            VertexDeclaration *decl = targetVertexData->vertexDeclaration;
            VertexBufferBinding *bind = targetVertexData->vertexBufferBinding;
            unsigned short bindIndex;

            // Build the index map brute-force. It's possible to store the index map
            // in .mesh, but maybe trivial.
            IndexMap boneIndexToBlendIndexMap;
            buildIndexMap( boneAssignments, boneIndexToBlendIndexMap, blendIndexToBoneIndexMap );

            const VertexElement *testElem = decl->findElementBySemantic( VES_BLEND_INDICES );
            if( testElem )
            {
                // Already have a buffer, unset it & delete elements
                bindIndex = testElem->getSource();
                // unset will cause deletion of buffer
                bind->unsetBinding( bindIndex );
                decl->removeElement( VES_BLEND_INDICES );
                decl->removeElement( VES_BLEND_WEIGHTS );
            }
            else
            {
                // Get new binding
                bindIndex = bind->getNextIndex();
            }

            // Determine what usage to use for the new buffer
            HardwareBuffer::Usage currentBufferUsage;
            const VertexBufferBinding::VertexBufferBindingMap &bindingMap = bind->getBindings();
            VertexBufferBinding::VertexBufferBindingMap::const_iterator currentBindingIt =
                bindingMap.find( bindIndex );
            if( currentBindingIt != bindingMap.end() )
            {
                currentBufferUsage = currentBindingIt->second->getUsage();
            }
            else if( bindingMap.size() )
            {
                currentBufferUsage = bindingMap.begin()->second->getUsage();
            }
            else
            {
                currentBufferUsage = HardwareBuffer::HBU_STATIC_WRITE_ONLY;
            }
            // Create a new buffer for bone assignments
            HardwareVertexBufferSharedPtr vbuf = getHardwareBufferManager()->createVertexBuffer(
                sizeof( unsigned char ) * 4 + sizeof( float ) * numBlendWeightsPerVertex,
                targetVertexData->vertexCount, currentBufferUsage,
                true  // use shadow buffer
            );
            // bind new buffer
            bind->setBinding( bindIndex, vbuf );
            const VertexElement *pIdxElem, *pWeightElem;

            // We do no longer care for pre-Dx9 compatible vertex decls., so just tack it on the end
            // We'll sort at the end
            const VertexElement &idxElem =
                decl->addElement( bindIndex, 0, VET_UBYTE4, VES_BLEND_INDICES );
            const VertexElement &wtElem = decl->addElement(
                bindIndex, sizeof( unsigned char ) * 4,
                VertexElement::multiplyTypeCount( VET_FLOAT1, numBlendWeightsPerVertex ),
                VES_BLEND_WEIGHTS );
            pIdxElem = &idxElem;
            pWeightElem = &wtElem;

            // Assign data
            size_t v;
            VertexBoneAssignmentList::const_iterator i, iend;
            i = boneAssignments.begin();
            iend = boneAssignments.end();
            HardwareBufferLockGuard vertexLock( vbuf, HardwareBuffer::HBL_DISCARD );
            unsigned char *pBase = static_cast<unsigned char *>( vertexLock.pData );
            // Iterate by vertex
            float *pWeight;
            unsigned char *pIndex;
            for( v = 0; v < targetVertexData->vertexCount; ++v )
            {
                /// Convert to specific pointers
                pWeightElem->baseVertexPointerToElement( pBase, &pWeight );
                pIdxElem->baseVertexPointerToElement( pBase, &pIndex );
                for( unsigned short bone = 0; bone < numBlendWeightsPerVertex; ++bone )
                {
                    // Do we still have data for this vertex?
                    if( i != iend && i->second.vertexIndex == v )
                    {
                        // If so, write weight
                        *pWeight++ = i->second.weight;
                        *pIndex++ =
                            static_cast<unsigned char>( boneIndexToBlendIndexMap[i->second.boneIndex] );
                        ++i;
                    }
                    else
                    {
                        // Ran out of assignments for this vertex, use weight 0 to indicate empty.
                        // If no bones are defined (an error in itself) set bone 0 as the assigned bone.
                        *pWeight++ = ( bone == 0 ) ? 1.0f : 0.0f;
                        *pIndex++ = 0;
                    }
                }
                pBase += vbuf->getVertexSize();
            }
        }
        //---------------------------------------------------------------------
        Real distLineSegToPoint( const Vector3 &line0, const Vector3 &line1, const Vector3 &pt )
        {
            Vector3 v01 = line1 - line0;
            Real tt = v01.dotProduct( pt - line0 ) /
                      std::max( v01.dotProduct( v01 ), std::numeric_limits<Real>::epsilon() );
            tt = Math::Clamp( tt, Real( 0.0f ), Real( 1.0f ) );
            Vector3 onLine = line0 + tt * v01;
            return pt.distance( onLine );
        }
        //---------------------------------------------------------------------
        Real _computeBoneBoundingRadiusHelper( VertexData *vertexData,
                                               const Mesh::VertexBoneAssignmentList &boneAssignments,
                                               const vector<Vector3>::type &bonePositions,
                                               const vector<vector<ushort>::type>::type &boneChildren )
        {
            vector<Vector3>::type vertexPositions;
            {
                // extract vertex positions
                const VertexElement *posElem =
                    vertexData->vertexDeclaration->findElementBySemantic( VES_POSITION );
                HardwareVertexBufferSharedPtr vbuf =
                    vertexData->vertexBufferBinding->getBuffer( posElem->getSource() );
                // if usage is write only,
                if( !vbuf->hasShadowBuffer() && ( vbuf->getUsage() & HardwareBuffer::HBU_WRITE_ONLY ) )
                {
                    // can't do it
                    return Real( 0.0f );
                }
                vertexPositions.resize( vertexData->vertexCount );
                HardwareBufferLockGuard vertexLock( vbuf, HardwareBuffer::HBL_READ_ONLY );
                unsigned char *vertex = static_cast<unsigned char *>( vertexLock.pData );
                float *pFloat;

                for( size_t i = 0; i < vertexData->vertexCount; ++i )
                {
                    posElem->baseVertexPointerToElement( vertex, &pFloat );
                    vertexPositions[i] = Vector3( pFloat[0], pFloat[1], pFloat[2] );
                    vertex += vbuf->getVertexSize();
                }
            }
            Real maxRadius = Real( 0 );
            Real minWeight = Real( 0.01 );
            // for each vertex-bone assignment,
            for( Mesh::VertexBoneAssignmentList::const_iterator i = boneAssignments.begin();
                 i != boneAssignments.end(); ++i )
            {
                // if weight is close to zero, ignore
                if( i->second.weight > minWeight )
                {
                    // if we have a bounding box around all bone origins, we consider how far outside
                    // this box the current vertex could ever get (assuming it is only attached to the
                    // given bone, and the bones all have unity scale)
                    size_t iBone = i->second.boneIndex;
                    const Vector3 &v = vertexPositions[i->second.vertexIndex];
                    Vector3 diff = v - bonePositions[iBone];
                    Real dist = diff.length();  // max distance of vertex v outside of bounding box
                    // if this bone has children, we can reduce the dist under the assumption that the
                    // children may rotate wrt their parent, but don't translate
                    for( size_t iChild = 0; iChild < boneChildren[iBone].size(); ++iChild )
                    {
                        // given this assumption, we know that the bounding box will enclose both the
                        // bone origin as well as the origin of the child bone, and therefore everything
                        // on a line segment between the bone origin and the child bone will be inside
                        // the bounding box as well
                        size_t iChildBone = boneChildren[iBone][iChild];
                        // compute distance from vertex to line segment between bones
                        Real distChild =
                            distLineSegToPoint( bonePositions[iBone], bonePositions[iChildBone], v );
                        dist = std::min( dist, distChild );
                    }
                    // scale the distance by the weight, this prevents the radius from being
                    // over-inflated because of a vertex that is lightly influenced by a faraway bone
                    dist *= i->second.weight;
                    maxRadius = std::max( maxRadius, dist );
                }
            }
            return maxRadius;
        }
        //---------------------------------------------------------------------
        void Mesh::_computeBoneBoundingRadius()
        {
            if( mBoneBoundingRadius == Real( 0 ) && mSkeleton )
            {
                Real radius = Real( 0 );
                vector<Vector3>::type bonePositions;
                vector<vector<ushort>::type>::type boneChildren;  // for each bone, a list of children
                {
                    // extract binding pose bone positions, and also indices for child bones
                    const size_t numBones = mOldSkeleton->getNumBones();
                    mOldSkeleton->setBindingPose();
                    mOldSkeleton->_updateTransforms();
                    bonePositions.resize( numBones );
                    boneChildren.resize( numBones );
                    // for each bone,
                    for( size_t iBone = 0; iBone < numBones; ++iBone )
                    {
                        OldBone *bone = mOldSkeleton->getBone( (uint16)iBone );
                        bonePositions[iBone] = bone->_getDerivedPosition();
                        boneChildren[iBone].reserve( bone->numChildren() );
                        for( size_t iChild = 0; iChild < bone->numChildren(); ++iChild )
                        {
                            OldBone *child = static_cast<OldBone *>( bone->getChild( (uint16)iChild ) );
                            boneChildren[iBone].push_back( child->getHandle() );
                        }
                    }
                }
                if( sharedVertexData[VpNormal] )
                {
                    // check shared vertices
                    radius = _computeBoneBoundingRadiusHelper(
                        sharedVertexData[VpNormal], mBoneAssignments, bonePositions, boneChildren );
                }

                // check submesh vertices
                for( SubMesh *submesh : mSubMeshList )
                {
                    if( !submesh->useSharedVertices && submesh->vertexData[VpNormal] )
                    {
                        Real r = _computeBoneBoundingRadiusHelper( submesh->vertexData[VpNormal],
                                                                   submesh->mBoneAssignments,
                                                                   bonePositions, boneChildren );
                        radius = std::max( radius, r );
                    }
                }
                if( radius > Real( 0 ) )
                {
                    mBoneBoundingRadius = radius;
                }
                else
                {
                    // fallback if we failed to find the vertices
                    mBoneBoundingRadius = mBoundRadius;
                }
            }
        }
        //---------------------------------------------------------------------
        void Mesh::_notifySkeleton( SkeletonPtr &pSkel )
        {
            mOldSkeleton = pSkel;
            mSkeletonName = pSkel->getName();

            mSkeleton = SkeletonManager::getSingleton().getSkeletonDef( mOldSkeleton.get() );
        }
        //---------------------------------------------------------------------
        Mesh::BoneAssignmentIterator Mesh::getBoneAssignmentIterator()
        {
            return BoneAssignmentIterator( mBoneAssignments.begin(), mBoneAssignments.end() );
        }
        //---------------------------------------------------------------------
        const String &Mesh::getSkeletonName() const { return mSkeletonName; }
        //---------------------------------------------------------------------
        ushort Mesh::getNumLodLevels() const { return mNumLods; }
        //---------------------------------------------------------------------
        const MeshLodUsage &Mesh::getLodLevel( ushort index ) const
        {
#if !OGRE_NO_MESHLOD
            assert( index < mMeshLodUsageList.size() );
            if( this->_isManualLodLevel( index ) && index > 0 && !mMeshLodUsageList[index].manualMesh )
            {
                // Load the mesh now
                try
                {
                    mMeshLodUsageList[index].manualMesh = MeshManager::getSingleton().load(
                        mMeshLodUsageList[index].manualName, getGroup() );
                    // get the edge data, if required
                    if( !mMeshLodUsageList[index].edgeData )
                    {
                        mMeshLodUsageList[index].edgeData =
                            mMeshLodUsageList[index].manualMesh->getEdgeList( 0 );
                    }
                }
                catch( Exception & )
                {
                    LogManager::getSingleton().logMessage(
                        "Error while loading manual LOD level " + mMeshLodUsageList[index].manualName +
                        " - this LOD level will not be rendered. You can "
                        "ignore this error in offline mesh tools." );
                }
            }
            return mMeshLodUsageList[index];
#else
            return mMeshLodUsageList[0];
#endif
        }
        //---------------------------------------------------------------------

        void Mesh::updateManualLodLevel( ushort index, const String &meshName )
        {
            // Basic prerequisites
            assert( index != 0 && "Can't modify first LOD level (full detail)" );
            assert( index < mMeshLodUsageList.size() && "Idndex out of bounds" );
            // get lod
            MeshLodUsage *lod = &( mMeshLodUsageList[index] );

            lod->manualName = meshName;
            lod->manualMesh.reset();
            OGRE_DELETE lod->edgeData;
            lod->edgeData = 0;
        }
        //---------------------------------------------------------------------
        void Mesh::_setLodInfo( unsigned short numLevels )
        {
            assert( !mEdgeListsBuilt && "Can't modify LOD after edge lists built" );

            // Basic prerequisites
            assert( numLevels > 0 && "Must be at least one level (full detail level must exist)" );

            mNumLods = numLevels;
            mMeshLodUsageList.resize( numLevels );
            mLodValues.resize( numLevels );
            // Resize submesh face data lists too
            for( SubMesh *submesh : mSubMeshList )
            {
                submesh->mLodFaceList[VpNormal].resize( numLevels - 1 );
                submesh->mLodFaceList[VpShadow].resize( numLevels - 1 );
            }
        }
        //---------------------------------------------------------------------
        void Mesh::_setLodUsage( unsigned short level, const MeshLodUsage &usage )
        {
            assert( !mEdgeListsBuilt && "Can't modify LOD after edge lists built" );

            // Basic prerequisites
            assert( level != 0 && "Can't modify first LOD level (full detail)" );
            assert( level < mMeshLodUsageList.size() && "Index out of bounds" );
            assert( mLodValues.size() == mMeshLodUsageList.size() );

            mMeshLodUsageList[level] = usage;
            mLodValues[level] = usage.userValue;

            if( !mMeshLodUsageList[level].manualName.empty() )
            {
                mHasManualLodLevel = true;
            }
        }
        //---------------------------------------------------------------------
        void Mesh::_setSubMeshLodFaceList( unsigned subIdx, unsigned short level, IndexData *facedata,
                                           bool casterPass )
        {
            assert( !mEdgeListsBuilt && "Can't modify LOD after edge lists built" );

            // Basic prerequisites
            assert( mMeshLodUsageList[level].manualName.empty() && "Not using generated LODs!" );
            assert( subIdx < mSubMeshList.size() && "Index out of bounds" );
            assert( level != 0 && "Can't modify first LOD level (full detail)" );
            assert( level - 1 < (unsigned short)mSubMeshList[subIdx]->mLodFaceList[casterPass].size() &&
                    "Index out of bounds" );

            SubMesh *sm = mSubMeshList[subIdx];
            sm->mLodFaceList[casterPass][level - 1] = facedata;
        }
        //---------------------------------------------------------------------
        bool Mesh::_isManualLodLevel( unsigned short level ) const
        {
#if !OGRE_NO_MESHLOD
            return !mMeshLodUsageList[level].manualName.empty();
#else
            return false;
#endif
        }
        //---------------------------------------------------------------------
        unsigned Mesh::_getSubMeshIndex( const String &name ) const
        {
            SubMeshNameMap::const_iterator i = mSubMeshNameMap.find( name );
            if( i == mSubMeshNameMap.end() )
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "No SubMesh named " + name + " found.",
                             "Mesh::_getSubMeshIndex" );

            return i->second;
        }
        //--------------------------------------------------------------------
        void Mesh::removeLodLevels()
        {
#if !OGRE_NO_MESHLOD
            // Remove data from SubMeshes
            for( SubMesh *submesh : mSubMeshList )
                submesh->removeLodLevels();

            freeEdgeList();
            mMeshLodUsageList.clear();
            mLodValues.clear();

            LodStrategy *lodStrategy = LodStrategyManager::getSingleton().getDefaultStrategy();

            // Reinitialise
            mNumLods = 1;
            mMeshLodUsageList.resize( 1 );
            mMeshLodUsageList[0].edgeData = NULL;
            // TODO: Shouldn't we rebuild edge lists after freeing them?
            mLodValues.push_back( lodStrategy->getBaseValue() );
#endif
        }

        //---------------------------------------------------------------------
        Real Mesh::getBoundingSphereRadius() const { return mBoundRadius; }
        //---------------------------------------------------------------------
        Real Mesh::getBoneBoundingRadius() const { return mBoneBoundingRadius; }
        //---------------------------------------------------------------------
        void Mesh::setVertexBufferPolicy( HardwareBuffer::Usage vbUsage, bool shadowBuffer )
        {
            mVertexBufferUsage = vbUsage;
            mVertexBufferShadowBuffer = shadowBuffer;
        }
        //---------------------------------------------------------------------
        void Mesh::setIndexBufferPolicy( HardwareBuffer::Usage vbUsage, bool shadowBuffer )
        {
            mIndexBufferUsage = vbUsage;
            mIndexBufferShadowBuffer = shadowBuffer;
        }
        //---------------------------------------------------------------------
        void Mesh::mergeAdjacentTexcoords( unsigned short finalTexCoordSet,
                                           unsigned short texCoordSetToDestroy )
        {
            if( sharedVertexData[VpNormal] )
                mergeAdjacentTexcoords( finalTexCoordSet, texCoordSetToDestroy,
                                        sharedVertexData[VpNormal] );

            for( SubMesh *submesh : mSubMeshList )
            {
                if( !submesh->useSharedVertices )
                {
                    mergeAdjacentTexcoords( finalTexCoordSet, texCoordSetToDestroy,
                                            submesh->vertexData[VpNormal] );
                }
            }
        }
        //---------------------------------------------------------------------
        void Mesh::mergeAdjacentTexcoords( unsigned short finalTexCoordSet,
                                           unsigned short texCoordSetToDestroy, VertexData *vertexData )
        {
            VertexDeclaration *vDecl = vertexData->vertexDeclaration;

            const VertexElement *uv0 =
                vDecl->findElementBySemantic( VES_TEXTURE_COORDINATES, finalTexCoordSet );
            const VertexElement *uv1 =
                vDecl->findElementBySemantic( VES_TEXTURE_COORDINATES, texCoordSetToDestroy );

            if( uv0 && uv1 )
            {
                // Check that both base types are compatible (mix floats w/ shorts) and there's enough
                // space
                VertexElementType baseType0 = VertexElement::getBaseType( uv0->getType() );
                VertexElementType baseType1 = VertexElement::getBaseType( uv1->getType() );

                unsigned short totalTypeCount = VertexElement::getTypeCount( uv0->getType() ) +
                                                VertexElement::getTypeCount( uv1->getType() );
                if( baseType0 == baseType1 && totalTypeCount <= 4 )
                {
                    const VertexDeclaration::VertexElementList &veList = vDecl->getElements();
                    VertexDeclaration::VertexElementList::const_iterator uv0Itor =
                        std::find( veList.begin(), veList.end(), *uv0 );
                    unsigned short elem_idx = (uint16)std::distance( veList.begin(), uv0Itor );
                    VertexElementType newType =
                        VertexElement::multiplyTypeCount( baseType0, totalTypeCount );

                    if( ( uv0->getOffset() + uv0->getSize() == uv1->getOffset() ||
                          uv1->getOffset() + uv1->getSize() == uv0->getOffset() ) &&
                        uv0->getSource() == uv1->getSource() )
                    {
                        // Special case where they adjacent, just change the declaration & we're done.
                        size_t newOffset = std::min( uv0->getOffset(), uv1->getOffset() );
                        unsigned short newIdx = std::min( uv0->getIndex(), uv1->getIndex() );

                        vDecl->modifyElement( elem_idx, uv0->getSource(), newOffset, newType,
                                              VES_TEXTURE_COORDINATES, newIdx );
                        vDecl->removeElement( VES_TEXTURE_COORDINATES, texCoordSetToDestroy );
                        uv1 = 0;
                    }

                    vDecl->closeGapsInSource();
                }
            }
        }
        //---------------------------------------------------------------------
        void Mesh::destroyShadowMappingGeom()
        {
            if( sharedVertexData[VpShadow] != sharedVertexData[VpNormal] )
                OGRE_DELETE sharedVertexData[VpShadow];
            sharedVertexData[VpShadow] = 0;

            for( SubMesh *submesh : mSubMeshList )
            {
                if( submesh->vertexData[VpShadow] != submesh->vertexData[VpNormal] )
                    OGRE_DELETE( submesh->vertexData[VpShadow] );
                submesh->vertexData[VpShadow] = 0;

                if( submesh->indexData[VpShadow] != submesh->indexData[VpNormal] )
                    OGRE_DELETE( submesh->indexData[VpShadow] );
                submesh->indexData[VpShadow] = 0;

                if( submesh->mLodFaceList[VpNormal].empty() || submesh->mLodFaceList[VpShadow].empty() ||
                    submesh->mLodFaceList[VpNormal][0] != submesh->mLodFaceList[VpShadow][0] )
                {
                    for( IndexData *lodFace : submesh->mLodFaceList[VpShadow] )
                        OGRE_DELETE lodFace;
                }

                submesh->mLodFaceList[VpShadow].clear();
            }
        }
        //---------------------------------------------------------------------
        void Mesh::prepareForShadowMapping( bool forceSameBuffers )
        {
            destroyShadowMappingGeom();

            if( !msOptimizeForShadowMapping || forceSameBuffers )
            {
                sharedVertexData[VpShadow] = sharedVertexData[VpNormal];

                for( SubMesh *submesh : mSubMeshList )
                {
                    submesh->vertexData[VpShadow] = submesh->vertexData[VpNormal];
                    submesh->indexData[VpShadow] = submesh->indexData[VpNormal];

                    for( IndexData *lodFace : submesh->mLodFaceList[VpNormal] )
                        submesh->mLodFaceList[VpShadow].push_back( lodFace );
                }
            }
            else
            {
                VertexShadowMapHelper::GeometryVec inGeom;
                VertexShadowMapHelper::GeometryVec outGeom;

                for( SubMesh *submesh : mSubMeshList )
                {
                    VertexShadowMapHelper::Geometry geom;

                    if( submesh->vertexData[VpNormal] )
                        geom.vertexData = submesh->vertexData[VpNormal];
                    else
                        geom.vertexData = sharedVertexData[VpNormal];

                    geom.indexData = submesh->indexData[VpNormal];
                    inGeom.push_back( geom );

                    for( IndexData *lodFace : submesh->mLodFaceList[VpNormal] )
                    {
                        geom.indexData = lodFace;
                        inGeom.push_back( geom );
                    }
                }

                VertexShadowMapHelper::optimizeForShadowMapping( inGeom, outGeom );

                VertexShadowMapHelper::GeometryVec::const_iterator itGeom = outGeom.begin();

                for( SubMesh *submesh : mSubMeshList )
                {
                    if( submesh->vertexData[VpNormal] )
                        submesh->vertexData[VpShadow] = itGeom->vertexData;
                    else
                        sharedVertexData[VpShadow] = itGeom->vertexData;

                    submesh->indexData[VpShadow] = itGeom->indexData;
                    ++itGeom;

                    const size_t numLods = submesh->mLodFaceList[VpNormal].size();
                    for( size_t j = 0; j < numLods; ++j )
                    {
                        submesh->mLodFaceList[VpShadow].push_back( itGeom->indexData );
                        ++itGeom;
                    }
                }
            }
        }
        //---------------------------------------------------------------------
        bool Mesh::hasValidShadowMappingBuffers() const
        {
            bool retVal = true;

            retVal &= ( sharedVertexData[VpNormal] == 0 ) ||
                      ( sharedVertexData[VpNormal] != 0 && sharedVertexData[VpShadow] != 0 );

            for( SubMesh *submesh : mSubMeshList )
            {
                if( !retVal )
                    break;

                retVal &= ( submesh->vertexData[VpNormal] == 0 ) ||
                          ( submesh->vertexData[VpNormal] != 0 && submesh->vertexData[VpShadow] != 0 );
                retVal &= ( submesh->indexData[VpNormal] == 0 ||
                            !submesh->indexData[VpNormal]->indexBuffer ) ||
                          ( submesh->indexData[VpNormal] != 0 && submesh->indexData[VpShadow] != 0 );

                retVal &=
                    submesh->mLodFaceList[VpNormal].size() == submesh->mLodFaceList[VpShadow].size();
            }

            return retVal;
        }
        //---------------------------------------------------------------------
        bool Mesh::hasIndependentShadowMappingBuffers() const
        {
            if( !hasValidShadowMappingBuffers() )
                return false;

            bool independent = sharedVertexData[VpNormal] != sharedVertexData[VpShadow];

            for( SubMesh *submesh : mSubMeshList )
                if( !independent && submesh->vertexData[VpNormal] != submesh->vertexData[VpShadow] )
                {
                    independent = true;
                    break;
                }

            return independent;
        }
        //---------------------------------------------------------------------
        void Mesh::organiseTangentsBuffer( VertexData *vertexData, VertexElementSemantic targetSemantic,
                                           unsigned short index, unsigned short sourceTexCoordSet )
        {
            VertexDeclaration *vDecl = vertexData->vertexDeclaration;
            VertexBufferBinding *vBind = vertexData->vertexBufferBinding;

            const VertexElement *tangentsElem = vDecl->findElementBySemantic( targetSemantic, index );
            bool needsToBeCreated = false;

            if( !tangentsElem )
            {  // no tex coords with index 1
                needsToBeCreated = true;
            }
            else if( tangentsElem->getType() != VET_FLOAT3 )
            {
                //  buffer exists, but not 3D
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Target semantic set already exists but is not 3D, therefore "
                             "cannot contain tangents. Pick an alternative destination semantic. ",
                             "Mesh::organiseTangentsBuffer" );
            }

            HardwareVertexBufferSharedPtr newBuffer;
            if( needsToBeCreated )
            {
                // To be most efficient with our vertex streams,
                // tack the new tangents onto the same buffer as the
                // source texture coord set
                const VertexElement *prevTexCoordElem =
                    vertexData->vertexDeclaration->findElementBySemantic( VES_TEXTURE_COORDINATES,
                                                                          sourceTexCoordSet );
                if( !prevTexCoordElem )
                {
                    OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                 "Cannot locate the first texture coordinate element to "
                                 "which to append the new tangents.",
                                 "Mesh::orgagniseTangentsBuffer" );
                }
                // Find the buffer associated with  this element
                HardwareVertexBufferSharedPtr origBuffer =
                    vertexData->vertexBufferBinding->getBuffer( prevTexCoordElem->getSource() );
                // Now create a new buffer, which includes the previous contents
                // plus extra space for the 3D coords
                newBuffer = getHardwareBufferManager()->createVertexBuffer(
                    origBuffer->getVertexSize() + 3 * sizeof( float ), vertexData->vertexCount,
                    origBuffer->getUsage(), origBuffer->hasShadowBuffer() );
                // Add the new element
                vDecl->addElement( prevTexCoordElem->getSource(), origBuffer->getVertexSize(),
                                   VET_FLOAT3, targetSemantic, index );
                // Now copy the original data across
                HardwareBufferLockGuard srcLock( origBuffer, HardwareBuffer::HBL_READ_ONLY );
                HardwareBufferLockGuard dstLock( newBuffer, HardwareBuffer::HBL_DISCARD );
                unsigned char *pSrc = static_cast<unsigned char *>( srcLock.pData );
                unsigned char *pDest = static_cast<unsigned char *>( dstLock.pData );
                size_t vertSize = origBuffer->getVertexSize();
                for( size_t v = 0; v < vertexData->vertexCount; ++v )
                {
                    // Copy original vertex data
                    memcpy( pDest, pSrc, vertSize );
                    pSrc += vertSize;
                    pDest += vertSize;
                    // Set the new part to 0 since we'll accumulate in this
                    memset( pDest, 0, sizeof( float ) * 3 );
                    pDest += sizeof( float ) * 3;
                }

                // Rebind the new buffer
                vBind->setBinding( prevTexCoordElem->getSource(), newBuffer );
            }
        }
        //---------------------------------------------------------------------
        void Mesh::buildTangentVectors( VertexElementSemantic targetSemantic,
                                        unsigned short sourceTexCoordSet, unsigned short index,
                                        bool splitMirrored, bool splitRotated, bool storeParityInW )
        {
            if( !sharedVertexData[VpNormal] )
                dearrangeToInefficient();

            TangentSpaceCalc tangentsCalc;
            tangentsCalc.setSplitMirrored( splitMirrored );
            tangentsCalc.setSplitRotated( splitRotated );
            tangentsCalc.setStoreParityInW( storeParityInW );

            // shared geometry first
            if( sharedVertexData[VpNormal] )
            {
                tangentsCalc.setVertexData( sharedVertexData[VpNormal] );
                bool found = false;
                for( SubMesh *sm : mSubMeshList )
                {
                    if( sm->useSharedVertices && ( sm->operationType == OT_TRIANGLE_FAN ||
                                                   sm->operationType == OT_TRIANGLE_LIST ||
                                                   sm->operationType == OT_TRIANGLE_STRIP ) )
                    {
                        tangentsCalc.addIndexData( sm->indexData[VpNormal] );
                        found = true;
                    }
                }
                if( found )
                {
                    TangentSpaceCalc::Result res =
                        tangentsCalc.build( targetSemantic, sourceTexCoordSet, index );

                    // If any vertex splitting happened, we have to give them bone assignments
                    if( getSkeletonName() != BLANKSTRING )
                    {
                        for( TangentSpaceCalc::IndexRemapList::iterator r = res.indexesRemapped.begin();
                             r != res.indexesRemapped.end(); ++r )
                        {
                            TangentSpaceCalc::IndexRemap &remap = *r;
                            // Copy all bone assignments from the split vertex
                            VertexBoneAssignmentList::iterator vbstart =
                                mBoneAssignments.lower_bound( remap.splitVertex.first );
                            VertexBoneAssignmentList::iterator vbend =
                                mBoneAssignments.upper_bound( remap.splitVertex.first );
                            for( VertexBoneAssignmentList::iterator vba = vbstart; vba != vbend; ++vba )
                            {
                                VertexBoneAssignment newAsgn = vba->second;
                                newAsgn.vertexIndex =
                                    static_cast<unsigned int>( remap.splitVertex.second );
                                // multimap insert doesn't invalidate iterators
                                addBoneAssignment( newAsgn );
                            }
                        }
                    }

                    // Update poses (some vertices might have been duplicated)
                    // we will just check which vertices have been split and copy
                    // the offset for the original vertex to the corresponding new vertex
                    PoseIterator pose_it = getPoseIterator();

                    while( pose_it.hasMoreElements() )
                    {
                        Pose *current_pose = pose_it.getNext();
                        const Pose::VertexOffsetMap &offset_map = current_pose->getVertexOffsets();

                        for( TangentSpaceCalc::VertexSplits::iterator it = res.vertexSplits.begin();
                             it != res.vertexSplits.end(); ++it )
                        {
                            TangentSpaceCalc::VertexSplit &split = *it;

                            Pose::VertexOffsetMap::const_iterator found_offset =
                                offset_map.find( split.first );

                            // copy the offset
                            if( found_offset != offset_map.end() )
                            {
                                current_pose->addVertex( split.second, found_offset->second );
                            }
                        }
                    }
                }
            }

            // Dedicated geometry
            for( SubMesh *sm : mSubMeshList )
            {
                if( !sm->useSharedVertices &&
                    ( sm->operationType == OT_TRIANGLE_FAN || sm->operationType == OT_TRIANGLE_LIST ||
                      sm->operationType == OT_TRIANGLE_STRIP ) )
                {
                    tangentsCalc.clear();
                    tangentsCalc.setVertexData( sm->vertexData[VpNormal] );
                    tangentsCalc.addIndexData( sm->indexData[VpNormal], sm->operationType );
                    TangentSpaceCalc::Result res =
                        tangentsCalc.build( targetSemantic, sourceTexCoordSet, index );

                    // If any vertex splitting happened, we have to give them bone assignments
                    if( getSkeletonName() != BLANKSTRING )
                    {
                        for( TangentSpaceCalc::IndexRemapList::iterator r = res.indexesRemapped.begin();
                             r != res.indexesRemapped.end(); ++r )
                        {
                            TangentSpaceCalc::IndexRemap &remap = *r;
                            // Copy all bone assignments from the split vertex
                            VertexBoneAssignmentList::const_iterator vbstart =
                                sm->getBoneAssignments().lower_bound( remap.splitVertex.first );
                            VertexBoneAssignmentList::const_iterator vbend =
                                sm->getBoneAssignments().upper_bound( remap.splitVertex.first );
                            for( VertexBoneAssignmentList::const_iterator vba = vbstart; vba != vbend;
                                 ++vba )
                            {
                                VertexBoneAssignment newAsgn = vba->second;
                                newAsgn.vertexIndex =
                                    static_cast<unsigned int>( remap.splitVertex.second );
                                // multimap insert doesn't invalidate iterators
                                sm->addBoneAssignment( newAsgn );
                            }
                        }
                    }
                }
            }

            prepareForShadowMapping( false );
        }
        //---------------------------------------------------------------------
        bool Mesh::suggestTangentVectorBuildParams( VertexElementSemantic targetSemantic,
                                                    unsigned short &outSourceCoordSet,
                                                    unsigned short &outIndex )
        {
            // Go through all the vertex data and locate source and dest (must agree)
            bool sharedGeometryDone = false;
            bool foundExisting = false;
            bool firstOne = true;
            for( SubMesh *sm : mSubMeshList )
            {
                VertexData *vertexData;

                if( sm->useSharedVertices )
                {
                    if( sharedGeometryDone )
                        continue;
                    vertexData = sharedVertexData[VpNormal];
                    sharedGeometryDone = true;
                }
                else
                {
                    vertexData = sm->vertexData[VpNormal];
                }

                const VertexElement *sourceElem = 0;
                unsigned short targetIndex = 0;
                for( targetIndex = 0; targetIndex < OGRE_MAX_TEXTURE_COORD_SETS; ++targetIndex )
                {
                    const VertexElement *testElem = vertexData->vertexDeclaration->findElementBySemantic(
                        VES_TEXTURE_COORDINATES, targetIndex );
                    if( !testElem )
                        break;  // finish if we've run out, t will be the target

                    if( !sourceElem )
                    {
                        // We're still looking for the source texture coords
                        if( testElem->getType() == VET_FLOAT2 )
                        {
                            // Ok, we found it
                            sourceElem = testElem;
                        }
                    }

                    if( !foundExisting && targetSemantic == VES_TEXTURE_COORDINATES )
                    {
                        // We're looking for the destination
                        // Check to see if we've found a possible
                        if( testElem->getType() == VET_FLOAT3 )
                        {
                            // This is a 3D set, might be tangents
                            foundExisting = true;
                        }
                    }
                }

                if( !foundExisting && targetSemantic != VES_TEXTURE_COORDINATES )
                {
                    targetIndex = 0;
                    // Look for existing semantic
                    const VertexElement *testElem = vertexData->vertexDeclaration->findElementBySemantic(
                        targetSemantic, targetIndex );
                    if( testElem )
                    {
                        foundExisting = true;
                    }
                }

                if( !foundExisting )
                {
                    const VertexElement *testElem =
                        vertexData->vertexDeclaration->findElementBySemantic( VES_NORMAL, 0 );

                    if( testElem && testElem->getType() == VET_SHORT4_SNORM )
                        foundExisting = true;  // Tangents were stored as QTangents
                }

                // After iterating, we should have a source and a possible destination (t)
                if( !sourceElem )
                {
                    OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                 "Cannot locate an appropriate 2D texture coordinate set for "
                                 "all the vertex data in this mesh to create tangents from. ",
                                 "Mesh::suggestTangentVectorBuildParams" );
                }
                // Check that we agree with previous decisions, if this is not the
                // first one, and if we're not just using the existing one
                if( !firstOne && !foundExisting )
                {
                    if( sourceElem->getIndex() != outSourceCoordSet )
                    {
                        OGRE_EXCEPT(
                            Exception::ERR_INVALIDPARAMS,
                            "Multiple sets of vertex data in this mesh disagree on "
                            "the appropriate index to use for the source texture coordinates. "
                            "This ambiguity must be rectified before tangents can be generated.",
                            "Mesh::suggestTangentVectorBuildParams" );
                    }
                    if( targetIndex != outIndex )
                    {
                        OGRE_EXCEPT(
                            Exception::ERR_INVALIDPARAMS,
                            "Multiple sets of vertex data in this mesh disagree on "
                            "the appropriate index to use for the target texture coordinates. "
                            "This ambiguity must be rectified before tangents can be generated.",
                            "Mesh::suggestTangentVectorBuildParams" );
                    }
                }

                // Otherwise, save this result
                outSourceCoordSet = sourceElem->getIndex();
                outIndex = targetIndex;

                firstOne = false;
            }

            return foundExisting;
        }
        //---------------------------------------------------------------------
        void Mesh::buildEdgeList()
        {
            if( mEdgeListsBuilt )
                return;
#if !OGRE_NO_MESHLOD
            // Loop over LODs
            for( unsigned short lodIndex = 0; lodIndex < (unsigned short)mMeshLodUsageList.size();
                 ++lodIndex )
            {
                // use getLodLevel to enforce loading of manual mesh lods
                MeshLodUsage &usage = const_cast<MeshLodUsage &>( getLodLevel( lodIndex ) );

                if( !usage.manualName.empty() && lodIndex != 0 )
                {
                    // Delegate edge building to manual mesh
                    // It should have already built it's own edge list while loading
                    if( usage.manualMesh )
                    {
                        usage.edgeData = usage.manualMesh->getEdgeList( 0 );
                    }
                }
                else
                {
                    // Build
                    EdgeListBuilder eb;
                    size_t vertexSetCount = 0;
                    bool atLeastOneIndexSet = false;

                    if( sharedVertexData[VpNormal] )
                    {
                        eb.addVertexData( sharedVertexData[VpNormal] );
                        vertexSetCount++;
                    }

                    // Prepare the builder using the submesh information
                    for( SubMesh *s : mSubMeshList )
                    {
                        if( s->operationType != OT_TRIANGLE_FAN &&
                            s->operationType != OT_TRIANGLE_LIST &&
                            s->operationType != OT_TRIANGLE_STRIP )
                        {
                            continue;
                        }
                        if( s->useSharedVertices )
                        {
                            // Use shared vertex data, index as set 0
                            if( lodIndex == 0 )
                            {
                                eb.addIndexData( s->indexData[VpNormal], 0, s->operationType );
                            }
                            else
                            {
                                eb.addIndexData( s->mLodFaceList[VpNormal][lodIndex - 1], 0,
                                                 s->operationType );
                            }
                        }
                        else if( s->isBuildEdgesEnabled() )
                        {
                            // own vertex data, add it and reference it directly
                            eb.addVertexData( s->vertexData[VpNormal] );
                            if( lodIndex == 0 )
                            {
                                // Base index data
                                eb.addIndexData( s->indexData[VpNormal], vertexSetCount++,
                                                 s->operationType );
                            }
                            else
                            {
                                // LOD index data
                                eb.addIndexData( s->mLodFaceList[VpNormal][lodIndex - 1],
                                                 vertexSetCount++, s->operationType );
                            }
                        }
                        atLeastOneIndexSet = true;
                    }

                    if( atLeastOneIndexSet )
                    {
                        usage.edgeData = eb.build();

#    if OGRE_DEBUG_MODE
                        // Override default log
                        Log *log = LogManager::getSingleton().createLog(
                            mName + "_lod" + StringConverter::toString( lodIndex ) + "_prepshadow.log",
                            false, false );
                        usage.edgeData->log( log );
                        // clean up log & close file handle
                        LogManager::getSingleton().destroyLog( log );
#    endif
                    }
                    else
                    {
                        // create empty edge data
                        usage.edgeData = OGRE_NEW EdgeData();
                    }
                }
            }
#else
            // Build
            EdgeListBuilder eb;
            size_t vertexSetCount = 0;
            if( sharedVertexData[VpNormal] )
            {
                eb.addVertexData( sharedVertexData[VpNormal] );
                vertexSetCount++;
            }

            // Prepare the builder using the submesh information
            for( SubMesh *s : mSubMeshList )
            {
                if( s->operationType != OT_TRIANGLE_FAN && s->operationType != OT_TRIANGLE_LIST &&
                    s->operationType != OT_TRIANGLE_STRIP )
                {
                    continue;
                }
                if( s->useSharedVertices )
                {
                    eb.addIndexData( s->indexData[VpNormal], 0, s->operationType );
                }
                else if( s->isBuildEdgesEnabled() )
                {
                    // own vertex data, add it and reference it directly
                    eb.addVertexData( s->vertexData[VpNormal] );
                    // Base index data
                    eb.addIndexData( s->indexData[VpNormal], vertexSetCount++, s->operationType );
                }
            }

            mMeshLodUsageList[0].edgeData = eb.build();

#    if OGRE_DEBUG_MODE
            // Override default log
            Log *log = LogManager::getSingleton().createLog( mName + "_lod0" + "_prepshadow.log", false,
                                                             false );
            mMeshLodUsageList[0].edgeData->log( log );
            // clean up log & close file handle
            LogManager::getSingleton().destroyLog( log );
#    endif
#endif
            mEdgeListsBuilt = true;
        }
        //---------------------------------------------------------------------
        void Mesh::freeEdgeList()
        {
            if( !mEdgeListsBuilt )
                return;
#if !OGRE_NO_MESHLOD
            // Loop over LODs
            unsigned short index = 0;
            for( MeshLodUsage &usage : mMeshLodUsageList )
            {
                if( usage.manualName.empty() || index == 0 )
                {
                    // Only delete if we own this data
                    // Manual LODs > 0 own their own
                    OGRE_DELETE usage.edgeData;
                }
                usage.edgeData = NULL;
                ++index;
            }
#else
            OGRE_DELETE mMeshLodUsageList[0].edgeData;
            mMeshLodUsageList[0].edgeData = NULL;
#endif
            mEdgeListsBuilt = false;
        }
        //---------------------------------------------------------------------
        void Mesh::prepareForShadowVolume()
        {
            if( mPreparedForShadowVolumes )
                return;

            if( sharedVertexData[VpNormal] )
            {
                sharedVertexData[VpNormal]->prepareForShadowVolume();
            }
            for( SubMesh *s : mSubMeshList )
            {
                if( !s->useSharedVertices &&
                    ( s->operationType == OT_TRIANGLE_FAN || s->operationType == OT_TRIANGLE_LIST ||
                      s->operationType == OT_TRIANGLE_STRIP ) )
                {
                    s->vertexData[VpNormal]->prepareForShadowVolume();
                }
            }
            mPreparedForShadowVolumes = true;
        }
        //---------------------------------------------------------------------
        EdgeData *Mesh::getEdgeList( unsigned short lodIndex )
        {
            // Build edge list on demand
            if( !mEdgeListsBuilt && mAutoBuildEdgeLists )
            {
                buildEdgeList();
            }
#if !OGRE_NO_MESHLOD
            return getLodLevel( lodIndex ).edgeData;
#else
            assert( lodIndex == 0 );
            return mMeshLodUsageList[0].edgeData;
#endif
        }
        //---------------------------------------------------------------------
        const EdgeData *Mesh::getEdgeList( unsigned short lodIndex ) const
        {
#if !OGRE_NO_MESHLOD
            return getLodLevel( lodIndex ).edgeData;
#else
            assert( lodIndex == 0 );
            return mMeshLodUsageList[0].edgeData;
#endif
        }
        //---------------------------------------------------------------------
        void Mesh::prepareMatricesForVertexBlend( const Matrix4 **blendMatrices,
                                                  const Matrix4 *boneMatrices, const IndexMap &indexMap )
        {
            assert( indexMap.size() <= 256 );
            IndexMap::const_iterator it, itend;
            itend = indexMap.end();
            for( it = indexMap.begin(); it != itend; ++it )
            {
                *blendMatrices++ = boneMatrices + *it;
            }
        }
        //---------------------------------------------------------------------
        void Mesh::softwareVertexBlend( const VertexData *sourceVertexData,
                                        const VertexData *targetVertexData,
                                        const Matrix4 *const *blendMatrices, size_t numMatrices,
                                        bool blendNormals )
        {
            float *pSrcPos = 0;
            float *pSrcNorm = 0;
            float *pDestPos = 0;
            float *pDestNorm = 0;
            float *pBlendWeight = 0;
            unsigned char *pBlendIdx = 0;
            size_t srcPosStride = 0;
            size_t srcNormStride = 0;
            size_t destPosStride = 0;
            size_t destNormStride = 0;
            size_t blendWeightStride = 0;
            size_t blendIdxStride = 0;

            // Get elements for source
            const VertexElement *srcElemPos =
                sourceVertexData->vertexDeclaration->findElementBySemantic( VES_POSITION );
            const VertexElement *srcElemNorm =
                sourceVertexData->vertexDeclaration->findElementBySemantic( VES_NORMAL );
            const VertexElement *srcElemBlendIndices =
                sourceVertexData->vertexDeclaration->findElementBySemantic( VES_BLEND_INDICES );
            const VertexElement *srcElemBlendWeights =
                sourceVertexData->vertexDeclaration->findElementBySemantic( VES_BLEND_WEIGHTS );
            assert( srcElemPos && srcElemBlendIndices && srcElemBlendWeights &&
                    "You must supply at least positions, blend indices and blend weights" );
            // Get elements for target
            const VertexElement *destElemPos =
                targetVertexData->vertexDeclaration->findElementBySemantic( VES_POSITION );
            const VertexElement *destElemNorm =
                targetVertexData->vertexDeclaration->findElementBySemantic( VES_NORMAL );

            // Do we have normals and want to blend them?
            bool includeNormals = blendNormals && ( srcElemNorm != NULL ) && ( destElemNorm != NULL );

            // Get buffers for source
            HardwareVertexBufferSharedPtr srcPosBuf =
                sourceVertexData->vertexBufferBinding->getBuffer( srcElemPos->getSource() );
            HardwareVertexBufferSharedPtr srcIdxBuf =
                sourceVertexData->vertexBufferBinding->getBuffer( srcElemBlendIndices->getSource() );
            HardwareVertexBufferSharedPtr srcWeightBuf =
                sourceVertexData->vertexBufferBinding->getBuffer( srcElemBlendWeights->getSource() );
            HardwareVertexBufferSharedPtr srcNormBuf;

            srcPosStride = srcPosBuf->getVertexSize();

            blendIdxStride = srcIdxBuf->getVertexSize();

            blendWeightStride = srcWeightBuf->getVertexSize();
            if( includeNormals )
            {
                srcNormBuf =
                    sourceVertexData->vertexBufferBinding->getBuffer( srcElemNorm->getSource() );
                srcNormStride = srcNormBuf->getVertexSize();
            }
            // Get buffers for target
            HardwareVertexBufferSharedPtr destPosBuf =
                targetVertexData->vertexBufferBinding->getBuffer( destElemPos->getSource() );
            HardwareVertexBufferSharedPtr destNormBuf;
            destPosStride = destPosBuf->getVertexSize();
            if( includeNormals )
            {
                destNormBuf =
                    targetVertexData->vertexBufferBinding->getBuffer( destElemNorm->getSource() );
                destNormStride = destNormBuf->getVertexSize();
            }

            // Lock source buffers for reading
            HardwareBufferLockGuard srcPosLock( srcPosBuf, HardwareBuffer::HBL_READ_ONLY );
            srcElemPos->baseVertexPointerToElement( srcPosLock.pData, &pSrcPos );
            HardwareBufferLockGuard srcNormLock;
            if( includeNormals )
            {
                if( srcNormBuf != srcPosBuf )
                {
                    // Different buffer
                    srcNormLock.lock( srcNormBuf, HardwareBuffer::HBL_READ_ONLY );
                }
                srcElemNorm->baseVertexPointerToElement(
                    srcNormBuf != srcPosBuf ? srcNormLock.pData : srcPosLock.pData, &pSrcNorm );
            }

            // Indices must be 4 bytes
            assert( srcElemBlendIndices->getType() == VET_UBYTE4 && "Blend indices must be VET_UBYTE4" );
            HardwareBufferLockGuard srcIdxLock( srcIdxBuf, HardwareBuffer::HBL_READ_ONLY );
            srcElemBlendIndices->baseVertexPointerToElement( srcIdxLock.pData, &pBlendIdx );
            HardwareBufferLockGuard srcWeightLock;
            if( srcWeightBuf != srcIdxBuf )
            {
                // Lock buffer
                srcWeightLock.lock( srcWeightBuf, HardwareBuffer::HBL_READ_ONLY );
            }
            srcElemBlendWeights->baseVertexPointerToElement(
                srcWeightBuf != srcIdxBuf ? srcWeightLock.pData : srcIdxLock.pData, &pBlendWeight );
            unsigned short numWeightsPerVertex =
                VertexElement::getTypeCount( srcElemBlendWeights->getType() );

            // Lock destination buffers for writing
            HardwareBufferLockGuard destPosLock(
                destPosBuf,
                ( destNormBuf != destPosBuf && destPosBuf->getVertexSize() == destElemPos->getSize() ) ||
                        ( destNormBuf == destPosBuf &&
                          destPosBuf->getVertexSize() ==
                              destElemPos->getSize() + destElemNorm->getSize() )
                    ? HardwareBuffer::HBL_DISCARD
                    : HardwareBuffer::HBL_NORMAL );
            destElemPos->baseVertexPointerToElement( destPosLock.pData, &pDestPos );
            HardwareBufferLockGuard destNormLock;
            if( includeNormals )
            {
                if( destNormBuf != destPosBuf )
                {
                    destNormLock.lock( destNormBuf,
                                       destNormBuf->getVertexSize() == destElemNorm->getSize()
                                           ? HardwareBuffer::HBL_DISCARD
                                           : HardwareBuffer::HBL_NORMAL );
                }
                destElemNorm->baseVertexPointerToElement(
                    destNormBuf != destPosBuf ? destNormLock.pData : destPosLock.pData, &pDestNorm );
            }

            OptimisedUtil::getImplementation()->softwareVertexSkinning(
                pSrcPos, pDestPos, pSrcNorm, pDestNorm, pBlendWeight, pBlendIdx, blendMatrices,
                srcPosStride, destPosStride, srcNormStride, destNormStride, blendWeightStride,
                blendIdxStride, numWeightsPerVertex, targetVertexData->vertexCount );
        }
        //---------------------------------------------------------------------
        void Mesh::softwareVertexMorph( Real t, const HardwareVertexBufferSharedPtr &b1,
                                        const HardwareVertexBufferSharedPtr &b2,
                                        VertexData *targetVertexData )
        {
            HardwareBufferLockGuard b1Lock( b1, HardwareBuffer::HBL_READ_ONLY );
            float *pb1 = static_cast<float *>( b1Lock.pData );
            HardwareBufferLockGuard b2Lock;
            float *pb2;
            if( b1.get() != b2.get() )
            {
                b2Lock.lock( b2, HardwareBuffer::HBL_READ_ONLY );
                pb2 = static_cast<float *>( b2Lock.pData );
            }
            else
            {
                // Same buffer - track with only one entry or time index exactly matching
                // one keyframe
                // For simplicity of main code, interpolate still but with same val
                pb2 = pb1;
            }

            const VertexElement *posElem =
                targetVertexData->vertexDeclaration->findElementBySemantic( VES_POSITION );
            assert( posElem );
            const VertexElement *normElem =
                targetVertexData->vertexDeclaration->findElementBySemantic( VES_NORMAL );

            bool morphNormals = false;
            if( normElem && normElem->getSource() == posElem->getSource() && b1->getVertexSize() == 24 &&
                b2->getVertexSize() == 24 )
                morphNormals = true;

            HardwareVertexBufferSharedPtr destBuf =
                targetVertexData->vertexBufferBinding->getBuffer( posElem->getSource() );
            assert( ( posElem->getSize() == destBuf->getVertexSize() ||
                      ( morphNormals &&
                        posElem->getSize() + normElem->getSize() == destBuf->getVertexSize() ) ) &&
                    "Positions (or positions & normals) must be in a buffer on their own for morphing" );
            HardwareBufferLockGuard destLock( destBuf, HardwareBuffer::HBL_DISCARD );
            float *pdst = static_cast<float *>( destLock.pData );

            OptimisedUtil::getImplementation()->softwareVertexMorph(
                t, pb1, pb2, pdst, b1->getVertexSize(), b2->getVertexSize(), destBuf->getVertexSize(),
                targetVertexData->vertexCount, morphNormals );
        }
        //---------------------------------------------------------------------
        void Mesh::softwareVertexPoseBlend( Real weight,
                                            const map<size_t, Vector3>::type &vertexOffsetMap,
                                            const map<size_t, Vector3>::type &normalsMap,
                                            VertexData *targetVertexData )
        {
            // Do nothing if no weight
            if( weight == 0.0f )
                return;

            const VertexElement *posElem =
                targetVertexData->vertexDeclaration->findElementBySemantic( VES_POSITION );
            const VertexElement *normElem =
                targetVertexData->vertexDeclaration->findElementBySemantic( VES_NORMAL );
            assert( posElem );
            // Support normals if they're in the same buffer as positions and pose includes them
            bool normals =
                normElem && !normalsMap.empty() && posElem->getSource() == normElem->getSource();
            HardwareVertexBufferSharedPtr destBuf =
                targetVertexData->vertexBufferBinding->getBuffer( posElem->getSource() );

            size_t elemsPerVertex = destBuf->getVertexSize() / sizeof( float );

            // Have to lock in normal mode since this is incremental
            HardwareBufferLockGuard destLock( destBuf, HardwareBuffer::HBL_NORMAL );
            float *pBase = static_cast<float *>( destLock.pData );

            // Iterate over affected vertices
            for( map<size_t, Vector3>::type::const_iterator i = vertexOffsetMap.begin();
                 i != vertexOffsetMap.end(); ++i )
            {
                // Adjust pointer
                float *pdst = pBase + i->first * elemsPerVertex;

                *pdst = *pdst + ( i->second.x * weight );
                ++pdst;
                *pdst = *pdst + ( i->second.y * weight );
                ++pdst;
                *pdst = *pdst + ( i->second.z * weight );
                ++pdst;
            }

            if( normals )
            {
                float *pNormBase;
                normElem->baseVertexPointerToElement( (void *)pBase, &pNormBase );
                for( map<size_t, Vector3>::type::const_iterator i = normalsMap.begin();
                     i != normalsMap.end(); ++i )
                {
                    // Adjust pointer
                    float *pdst = pNormBase + i->first * elemsPerVertex;

                    *pdst = *pdst + ( i->second.x * weight );
                    ++pdst;
                    *pdst = *pdst + ( i->second.y * weight );
                    ++pdst;
                    *pdst = *pdst + ( i->second.z * weight );
                    ++pdst;
                }
            }
        }
        //---------------------------------------------------------------------
        size_t Mesh::calculateSize() const
        {
            // calculate GPU size
            size_t ret = 0;
            unsigned short i;
            // Shared vertices
            if( sharedVertexData[VpNormal] )
            {
                for( i = 0; i < sharedVertexData[VpNormal]->vertexBufferBinding->getBufferCount(); ++i )
                {
                    ret += sharedVertexData[VpNormal]
                               ->vertexBufferBinding->getBuffer( i )
                               ->getSizeInBytes();
                }

                if( sharedVertexData[VpNormal] != sharedVertexData[VpShadow] )
                {
                    for( i = 0; i < sharedVertexData[VpShadow]->vertexBufferBinding->getBufferCount();
                         ++i )
                    {
                        ret += sharedVertexData[VpShadow]
                                   ->vertexBufferBinding->getBuffer( i )
                                   ->getSizeInBytes();
                    }
                }
            }

            for( SubMesh *submesh : mSubMeshList )
            {
                // Dedicated vertices
                if( !submesh->useSharedVertices )
                {
                    for( i = 0; i < submesh->vertexData[VpNormal]->vertexBufferBinding->getBufferCount();
                         ++i )
                    {
                        ret += submesh->vertexData[VpNormal]
                                   ->vertexBufferBinding->getBuffer( i )
                                   ->getSizeInBytes();
                    }

                    if( submesh->vertexData[VpNormal] != submesh->vertexData[VpShadow] )
                    {
                        for( i = 0;
                             i < submesh->vertexData[VpShadow]->vertexBufferBinding->getBufferCount();
                             ++i )
                        {
                            ret += submesh->vertexData[VpShadow]
                                       ->vertexBufferBinding->getBuffer( i )
                                       ->getSizeInBytes();
                        }
                    }
                }
                if( submesh->indexData[VpNormal]->indexBuffer )
                {
                    // Index data
                    ret += submesh->indexData[VpNormal]->indexBuffer->getSizeInBytes();

                    if( submesh->indexData[VpNormal] != submesh->indexData[VpShadow] )
                        ret += submesh->indexData[VpShadow]->indexBuffer->getSizeInBytes();
                }
            }
            return ret;
        }
        //-----------------------------------------------------------------------------
        bool Mesh::hasVertexAnimation() const { return !mAnimationsList.empty(); }
        //---------------------------------------------------------------------
        VertexAnimationType Mesh::getSharedVertexDataAnimationType() const
        {
            if( mAnimationTypesDirty )
            {
                _determineAnimationTypes();
            }

            return mSharedVertexDataAnimationType;
        }
        //---------------------------------------------------------------------
        void Mesh::_determineAnimationTypes() const
        {
            // Don't check flag here; since detail checks on track changes are not
            // done, allow caller to force if they need to

            // Initialise all types to nothing
            mSharedVertexDataAnimationType = VAT_NONE;
            mSharedVertexDataAnimationIncludesNormals = false;
            for( SubMesh *submesh : mSubMeshList )
            {
                submesh->mVertexAnimationType = VAT_NONE;
                submesh->mVertexAnimationIncludesNormals = false;
            }

            mPosesIncludeNormals = false;
            for( PoseList::const_iterator i = mPoseList.begin(); i != mPoseList.end(); ++i )
            {
                if( i == mPoseList.begin() )
                    mPosesIncludeNormals = ( *i )->getIncludesNormals();
                else if( mPosesIncludeNormals != ( *i )->getIncludesNormals() )
                    // only support normals if consistently included
                    mPosesIncludeNormals = mPosesIncludeNormals && ( *i )->getIncludesNormals();
            }

            // Scan all animations and determine the type of animation tracks
            // relating to each vertex data
            for( AnimationList::const_iterator ai = mAnimationsList.begin(); ai != mAnimationsList.end();
                 ++ai )
            {
                Animation *anim = ai->second;
                Animation::VertexTrackIterator vit = anim->getVertexTrackIterator();
                while( vit.hasMoreElements() )
                {
                    VertexAnimationTrack *track = vit.getNext();
                    ushort handle = track->getHandle();
                    if( handle == 0 )
                    {
                        // shared data
                        if( mSharedVertexDataAnimationType != VAT_NONE &&
                            mSharedVertexDataAnimationType != track->getAnimationType() )
                        {
                            // Mixing of morph and pose animation on same data is not allowed
                            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                         "Animation tracks for shared vertex data on mesh " + mName +
                                             " try to mix vertex animation types, which is "
                                             "not allowed.",
                                         "Mesh::_determineAnimationTypes" );
                        }
                        mSharedVertexDataAnimationType = track->getAnimationType();
                        if( track->getAnimationType() == VAT_MORPH )
                            mSharedVertexDataAnimationIncludesNormals =
                                track->getVertexAnimationIncludesNormals();
                        else
                            mSharedVertexDataAnimationIncludesNormals = mPosesIncludeNormals;
                    }
                    else
                    {
                        // submesh index (-1)
                        SubMesh *sm = getSubMesh( handle - 1 );
                        if( sm->mVertexAnimationType != VAT_NONE &&
                            sm->mVertexAnimationType != track->getAnimationType() )
                        {
                            // Mixing of morph and pose animation on same data is not allowed
                            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                         "Animation tracks for dedicated vertex data " +
                                             StringConverter::toString( handle - 1 ) + " on mesh " +
                                             mName +
                                             " try to mix vertex animation types, which is "
                                             "not allowed.",
                                         "Mesh::_determineAnimationTypes" );
                        }
                        sm->mVertexAnimationType = track->getAnimationType();
                        if( track->getAnimationType() == VAT_MORPH )
                            sm->mVertexAnimationIncludesNormals =
                                track->getVertexAnimationIncludesNormals();
                        else
                            sm->mVertexAnimationIncludesNormals = mPosesIncludeNormals;
                    }
                }
            }

            mAnimationTypesDirty = false;
        }
        //---------------------------------------------------------------------
        Animation *Mesh::createAnimation( const String &name, Real length )
        {
            // Check name not used
            if( mAnimationsList.find( name ) != mAnimationsList.end() )
            {
                OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                             "An animation with the name " + name + " already exists",
                             "Mesh::createAnimation" );
            }

            Animation *ret = OGRE_NEW Animation( name, length );
            ret->_notifyContainer( this );

            // Add to list
            mAnimationsList[name] = ret;

            // Mark animation types dirty
            mAnimationTypesDirty = true;

            return ret;
        }
        //---------------------------------------------------------------------
        Animation *Mesh::getAnimation( const String &name ) const
        {
            Animation *ret = _getAnimationImpl( name );
            if( !ret )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "No animation entry found named " + name,
                             "Mesh::getAnimation" );
            }

            return ret;
        }
        //---------------------------------------------------------------------
        Animation *Mesh::getAnimation( unsigned short index ) const
        {
            // If you hit this assert, then the index is out of bounds.
            assert( index < mAnimationsList.size() );

            AnimationList::const_iterator i = mAnimationsList.begin();

            std::advance( i, index );

            return i->second;
        }
        //---------------------------------------------------------------------
        unsigned short Mesh::getNumAnimations() const
        {
            return static_cast<unsigned short>( mAnimationsList.size() );
        }
        //---------------------------------------------------------------------
        bool Mesh::hasAnimation( const String &name ) const { return _getAnimationImpl( name ) != 0; }
        //---------------------------------------------------------------------
        Animation *Mesh::_getAnimationImpl( const String &name ) const
        {
            Animation *ret = 0;
            AnimationList::const_iterator i = mAnimationsList.find( name );

            if( i != mAnimationsList.end() )
            {
                ret = i->second;
            }

            return ret;
        }
        //---------------------------------------------------------------------
        void Mesh::removeAnimation( const String &name )
        {
            AnimationList::iterator i = mAnimationsList.find( name );

            if( i == mAnimationsList.end() )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "No animation entry found named " + name,
                             "Mesh::getAnimation" );
            }

            OGRE_DELETE i->second;

            mAnimationsList.erase( i );

            mAnimationTypesDirty = true;
        }
        //---------------------------------------------------------------------
        void Mesh::removeAllAnimations()
        {
            AnimationList::iterator i = mAnimationsList.begin();
            for( ; i != mAnimationsList.end(); ++i )
            {
                OGRE_DELETE i->second;
            }
            mAnimationsList.clear();
            mAnimationTypesDirty = true;
        }
        //---------------------------------------------------------------------
        VertexData *Mesh::getVertexDataByTrackHandle( unsigned short handle )
        {
            if( handle == 0 )
            {
                return sharedVertexData[VpNormal];
            }
            else
            {
                return getSubMesh( handle - 1 )->vertexData[VpNormal];
            }
        }
        //---------------------------------------------------------------------
        Pose *Mesh::createPose( ushort target, const String &name )
        {
            Pose *retPose = OGRE_NEW Pose( target, name );
            mPoseList.push_back( retPose );
            return retPose;
        }
        //---------------------------------------------------------------------
        Pose *Mesh::getPose( ushort index )
        {
            if( index >= getPoseCount() )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Index out of bounds", "Mesh::getPose" );
            }

            return mPoseList[index];
        }
        //---------------------------------------------------------------------
        Pose *Mesh::getPose( const String &name )
        {
            for( Pose *pose : mPoseList )
                if( pose->getName() == name )
                    return pose;

            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "No pose called " + name + " found in Mesh " + mName, "Mesh::getPose" );
        }
        //---------------------------------------------------------------------
        void Mesh::removePose( ushort index )
        {
            if( index >= getPoseCount() )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Index out of bounds", "Mesh::removePose" );
            }
            PoseList::iterator i = mPoseList.begin();
            std::advance( i, index );
            OGRE_DELETE *i;
            mPoseList.erase( i );
        }
        //---------------------------------------------------------------------
        void Mesh::removePose( const String &name )
        {
            for( PoseList::iterator i = mPoseList.begin(); i != mPoseList.end(); ++i )
            {
                if( ( *i )->getName() == name )
                {
                    OGRE_DELETE *i;
                    mPoseList.erase( i );
                    return;
                }
            }
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "No pose called " + name + " found in Mesh " + mName, "Mesh::removePose" );
        }
        //---------------------------------------------------------------------
        void Mesh::removeAllPoses()
        {
            for( Pose *pose : mPoseList )
                OGRE_DELETE pose;
            mPoseList.clear();
        }
        //---------------------------------------------------------------------
        Mesh::PoseIterator Mesh::getPoseIterator()
        {
            return PoseIterator( mPoseList.begin(), mPoseList.end() );
        }
        //---------------------------------------------------------------------
        Mesh::ConstPoseIterator Mesh::getPoseIterator() const
        {
            return ConstPoseIterator( mPoseList.begin(), mPoseList.end() );
        }
        //-----------------------------------------------------------------------------
        const PoseList &Mesh::getPoseList() const { return mPoseList; }
        //---------------------------------------------------------------------
        void Mesh::updateMaterialForAllSubMeshes()
        {
            // iterate through each sub mesh and request the submesh to update its material
            for( SubMesh *submesh : mSubMeshList )
                submesh->updateMaterialUsingTextureAliases();
        }
        //---------------------------------------------------------------------
        void Mesh::createAzdoBuffers()
        {
            // for( SubMesh *submesh : mSubMeshList )
        }

        //---------------------------------------------------------------------
    }  // namespace v1
}  // namespace Ogre
