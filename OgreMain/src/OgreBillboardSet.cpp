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

#include "OgreBillboardSet.h"

#include "OgreBillboard.h"
#include "OgreCamera.h"
#include "OgreException.h"
#include "OgreHardwareBufferManager.h"
#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgreLogManager.h"
#include "OgreMaterialManager.h"
#include "OgreMath.h"
#include "OgreRenderOperation.h"
#include "OgreRenderSystem.h"
#include "OgreRoot.h"
#include "OgreSceneNode.h"
#include "OgreSphere.h"
#include "Vao/OgreVaoManager.h"

#include <algorithm>

namespace Ogre
{
    namespace v1
    {
        // Init statics
        RadixSort<BillboardSet::ActiveBillboardList, Billboard *, float> BillboardSet::mRadixSorter;

        //-----------------------------------------------------------------------
        BillboardSet::BillboardSet( IdType id, ObjectMemoryManager *objectMemoryManager,
                                    SceneManager *manager, unsigned int poolSize, bool externalData,
                                    uint8 renderQueueId ) :
            MovableObject( id, objectMemoryManager, manager, renderQueueId ),
            mOriginType( BBO_CENTER ),
            mRotationType( BBR_TEXCOORD ),
            mAllDefaultSize( true ),
            mAutoExtendPool( true ),
            mSortingEnabled( false ),
            mAccurateFacing( false ),
            mAllDefaultRotation( true ),
            mWorldSpace( false ),
            mVertexData( 0 ),
            mLastLockedFrame( (uint32)-1 ),
            mIndexData( 0 ),
            mCullIndividual( false ),
            mBillboardType( BBT_POINT ),
            mCommonDirection( Ogre::Vector3::UNIT_Z ),
            mCommonUpVector( Vector3::UNIT_Y ),
            mVaoManager( 0 ),
            mPointRendering( false ),
            mBuffersCreated( false ),
            mPoolSize( poolSize ),
            mExternalData( externalData ),
            mAutoUpdate( true ),
            mBillboardDataChanged( true )
        {
            setDefaultDimensions( 100, 100 );

            mRenderables.push_back( this );

            setPoolSize( poolSize );
            setCastShadows( false );
            setTextureStacksAndSlices( 1, 1 );

            mObjectData.mQueryFlags[mObjectData.mIndex] = SceneManager::QUERY_FX_DEFAULT_MASK;

            mObjectData.mLocalAabb->setFromAabb( Aabb::BOX_NULL, mObjectData.mIndex );
        }
        //-----------------------------------------------------------------------
        BillboardSet::~BillboardSet()
        {
            // Free pool items
            BillboardPool::iterator i;
            for( i = mBillboardPool.begin(); i != mBillboardPool.end(); ++i )
            {
                OGRE_DELETE *i;
            }

            // Delete shared buffers
            _destroyBuffers();
        }
        //-----------------------------------------------------------------------
        Billboard *BillboardSet::createBillboard( const Vector3 &position, const ColourValue &colour )
        {
            if( mFreeBillboards.empty() )
            {
                if( mAutoExtendPool )
                {
                    setPoolSize( getPoolSize() * 2 );
                }
                else
                {
                    return 0;
                }
            }

            // Get a new billboard
            Billboard *newBill = mFreeBillboards.front();
            mActiveBillboards.splice( mActiveBillboards.end(), mFreeBillboards,
                                      mFreeBillboards.begin() );
            newBill->setPosition( position );
            newBill->setColour( colour );
            newBill->mDirection = Vector3::ZERO;
            newBill->setRotation( Radian( 0 ) );
            newBill->setTexcoordIndex( 0 );
            newBill->resetDimensions();
            newBill->_notifyOwner( this );

            // Merge into bounds
            Real adjust = std::max( mDefaultWidth, mDefaultHeight );
            Vector3 vecAdjust( adjust, adjust, adjust );
            Vector3 newMin = position - vecAdjust;
            Vector3 newMax = position + vecAdjust;

            Aabb aabb;
            mObjectData.mLocalAabb->getAsAabb( aabb, mObjectData.mIndex );
            aabb.merge( newMin );
            aabb.merge( newMax );
            mObjectData.mLocalAabb->setFromAabb( aabb, mObjectData.mIndex );
            mObjectData.mLocalRadius[mObjectData.mIndex] = aabb.getRadius();

            return newBill;
        }

        //-----------------------------------------------------------------------
        Billboard *BillboardSet::createBillboard( Real x, Real y, Real z, const ColourValue &colour )
        {
            return createBillboard( Vector3( x, y, z ), colour );
        }

        //-----------------------------------------------------------------------
        int BillboardSet::getNumBillboards() const
        {
            return static_cast<int>( mActiveBillboards.size() );
        }

        //-----------------------------------------------------------------------
        void BillboardSet::clear()
        {
            // Move actives to free list
            mFreeBillboards.splice( mFreeBillboards.end(), mActiveBillboards );
        }

        //-----------------------------------------------------------------------
        Billboard *BillboardSet::getBillboard( unsigned int index ) const
        {
            assert( index < mActiveBillboards.size() && "Billboard index out of bounds." );

            /* We can't access it directly, so we check whether it's in the first
               or the second half, then we start either from the beginning or the
               end of the list
            */
            ActiveBillboardList::const_iterator it;
            if( index >= ( mActiveBillboards.size() >> 1 ) )
            {
                index = static_cast<unsigned int>( mActiveBillboards.size() ) - index;
                for( it = mActiveBillboards.end(); index; --index, --it )
                    ;
            }
            else
            {
                for( it = mActiveBillboards.begin(); index; --index, ++it )
                    ;
            }

            return *it;
        }

        //-----------------------------------------------------------------------
        void BillboardSet::removeBillboard( unsigned int index )
        {
            assert( index < mActiveBillboards.size() && "Billboard index out of bounds." );

            /* We can't access it directly, so we check whether it's in the first
               or the second half, then we start either from the beginning or the
               end of the list.
               We then remove the billboard form the 'used' list and add it to
               the 'free' list.
            */
            ActiveBillboardList::iterator it;
            if( index >= ( mActiveBillboards.size() >> 1 ) )
            {
                index = static_cast<unsigned int>( mActiveBillboards.size() ) - index;
                for( it = mActiveBillboards.end(); index; --index, --it )
                    ;
            }
            else
            {
                for( it = mActiveBillboards.begin(); index; --index, ++it )
                    ;
            }

            mFreeBillboards.splice( mFreeBillboards.end(), mActiveBillboards, it );
        }

        //-----------------------------------------------------------------------
        void BillboardSet::removeBillboard( Billboard *pBill )
        {
            ActiveBillboardList::iterator it =
                std::find( mActiveBillboards.begin(), mActiveBillboards.end(), pBill );
            assert( it != mActiveBillboards.end() && "Billboard isn't in the active list." );

            mFreeBillboards.splice( mFreeBillboards.end(), mActiveBillboards, it );
        }

        //-----------------------------------------------------------------------
        void BillboardSet::setBillboardOrigin( BillboardOrigin origin ) { mOriginType = origin; }

        //-----------------------------------------------------------------------
        BillboardOrigin BillboardSet::getBillboardOrigin() const { return mOriginType; }

        //-----------------------------------------------------------------------
        void BillboardSet::setBillboardRotationType( BillboardRotationType rotationType )
        {
            mRotationType = rotationType;
        }
        //-----------------------------------------------------------------------
        BillboardRotationType BillboardSet::getBillboardRotationType() const { return mRotationType; }
        //-----------------------------------------------------------------------
        void BillboardSet::setDefaultDimensions( Real width, Real height )
        {
            mDefaultWidth = width;
            mDefaultHeight = height;
        }
        //-----------------------------------------------------------------------
        void BillboardSet::setDefaultWidth( Real width ) { mDefaultWidth = width; }
        //-----------------------------------------------------------------------
        Real BillboardSet::getDefaultWidth() const { return mDefaultWidth; }
        //-----------------------------------------------------------------------
        void BillboardSet::setDefaultHeight( Real height ) { mDefaultHeight = height; }
        //-----------------------------------------------------------------------
        Real BillboardSet::getDefaultHeight() const { return mDefaultHeight; }
        //-----------------------------------------------------------------------
        void BillboardSet::_sortBillboards()
        {
            switch( _getSortMode() )
            {
            case SM_DIRECTION:
                mRadixSorter.sort( mActiveBillboards, SortByDirectionFunctor( -mCamDir ) );
                break;
            case SM_DISTANCE:
                mRadixSorter.sort( mActiveBillboards, SortByDistanceFunctor( mCamPos ) );
                break;
            }
        }
        BillboardSet::SortByDirectionFunctor::SortByDirectionFunctor( const Vector3 &dir ) :
            sortDir( dir )
        {
        }
        float BillboardSet::SortByDirectionFunctor::operator()( Billboard *bill ) const
        {
            return sortDir.dotProduct( bill->getPosition() );
        }
        BillboardSet::SortByDistanceFunctor::SortByDistanceFunctor( const Vector3 &pos ) : sortPos( pos )
        {
        }
        float BillboardSet::SortByDistanceFunctor::operator()( Billboard *bill ) const
        {
            // Sort descending by squared distance
            return -( sortPos - bill->getPosition() ).squaredLength();
        }
        //-----------------------------------------------------------------------
        SortMode BillboardSet::_getSortMode() const
        {
            // Need to sort by distance if we're using accurate facing, or perpendicular billboard type.
            if( mAccurateFacing || mBillboardType == BBT_PERPENDICULAR_SELF ||
                mBillboardType == BBT_PERPENDICULAR_COMMON )
            {
                return SM_DISTANCE;
            }
            else
            {
                return SM_DIRECTION;
            }
        }
        //-----------------------------------------------------------------------
        bool BillboardSet::getUseIdentityWorldMatrix() const { return mWorldSpace; }
        //-----------------------------------------------------------------------
        void BillboardSet::_notifyCurrentCamera( const Camera *camera, const Camera *lodCamera )
        {
            // Calculate camera orientation and position
            mCamQ = camera->getDerivedOrientation();
            mCamPos = camera->getDerivedPosition();
            if( !mWorldSpace )
            {
                // Default behaviour is that billboards are in local node space
                // so orientation of camera (in world space) must be reverse-transformed
                // into node space
                mCamQ = mParentNode->convertWorldToLocalOrientation( mCamQ );
                mCamPos = mParentNode->convertWorldToLocalPosition( mCamPos );
            }

            // Camera direction points down -Z
            mCamDir = mCamQ * Vector3::NEGATIVE_UNIT_Z;
        }
        //-----------------------------------------------------------------------
        void BillboardSet::beginBillboards( size_t numBillboards )
        {
            /* Generate the vertices for all the billboards relative to the camera
               Also take the opportunity to update the vertex colours
               May as well do it here to save on loops elsewhere
            */

            /* NOTE: most engines generate world coordinates for the billboards
               directly, taking the world axes of the camera as offsets to the
               center points. I take a different approach, reverse-transforming
               the camera world axes into local billboard space.
               Why?
               Well, it's actually more efficient this way, because I only have to
               reverse-transform using the billboardset world matrix (inverse)
               once, from then on it's simple additions (assuming identically
               sized billboards). If I transformed every billboard center by it's
               world transform, that's a matrix multiplication per billboard
               instead.
               I leave the final transform to the render pipeline since that can
               use hardware TnL if it is available.
            */

            // create vertex and index buffers if they haven't already been
            if( !mBuffersCreated )
                _createBuffers();

            if( mLastLockedFrame == mVaoManager->getFrameCount() )
            {
                ++mLastLockedBuffer;
                if( mLastLockedBuffer >= mMainBuffers[0].size() )
                    createExtraVertexBuffer( mMainBuffers[0][0]->getVertexSize() );
            }
            else
            {
                mLastLockedBuffer = 0;
                mLastLockedFrame = mVaoManager->getFrameCount();
            }

            mMainBuf = mMainBuffers[mVaoManager->waitForTailFrameToFinish()][mLastLockedBuffer];
            mVertexData->vertexBufferBinding->setBinding( 0, mMainBuf );

            // Only calculate vertex offets et al if we're not point rendering
            if( !mPointRendering )
            {
                // Get offsets for origin type
                getParametricOffsets( mLeftOff, mRightOff, mTopOff, mBottomOff );

                // Generate axes etc up-front if not oriented per-billboard
                if( mBillboardType != BBT_ORIENTED_SELF && mBillboardType != BBT_PERPENDICULAR_SELF &&
                    !( mAccurateFacing && mBillboardType != BBT_PERPENDICULAR_COMMON ) )
                {
                    genBillboardAxes( &mCamX, &mCamY );

                    /* If all billboards are the same size we can precalculate the
                       offsets and just use '+' instead of '*' for each billboard,
                       and it should be faster.
                    */
                    genVertOffsets( mLeftOff, mRightOff, mTopOff, mBottomOff, mDefaultWidth,
                                    mDefaultHeight, mCamX, mCamY, mVOffset );
                }
            }

            // Init num visible
            mNumVisibleBillboards = 0;

            // Lock the buffer
            if( numBillboards )  // optimal lock
            {
                // clamp to max
                numBillboards = std::min( mPoolSize, numBillboards );

                size_t billboardSize;
                if( mPointRendering )
                {
                    // just one vertex per billboard (this also excludes texcoords)
                    billboardSize = mMainBuf->getVertexSize();
                }
                else
                {
                    // 4 corners
                    billboardSize = mMainBuf->getVertexSize() * 4;
                }
                assert( numBillboards * billboardSize <= mMainBuf->getSizeInBytes() );

                mLockPtr = static_cast<float *>( mMainBuf->lock(
                    0, numBillboards * billboardSize,
                    mMainBuf->getUsage() & HardwareBuffer::HBU_DYNAMIC ? HardwareBuffer::HBL_NO_OVERWRITE
                                                                       : HardwareBuffer::HBL_NORMAL ) );
            }
            else  // lock the entire thing
            {
                mLockPtr = static_cast<float *>( mMainBuf->lock(
                    mMainBuf->getUsage() & HardwareBuffer::HBU_DYNAMIC ? HardwareBuffer::HBL_NO_OVERWRITE
                                                                       : HardwareBuffer::HBL_NORMAL ) );
            }
        }
        //-----------------------------------------------------------------------
        void BillboardSet::injectBillboard( const Billboard &bb, const Camera *camera )
        {
            // Don't accept injections beyond pool size
            if( mNumVisibleBillboards == mPoolSize )
                return;

            // Skip if not visible (NB always true if not bounds checking individual billboards)
            if( !billboardVisible( camera, bb ) )
                return;

            if( !mPointRendering &&
                ( mBillboardType == BBT_ORIENTED_SELF || mBillboardType == BBT_PERPENDICULAR_SELF ||
                  ( mAccurateFacing && mBillboardType != BBT_PERPENDICULAR_COMMON ) ) )
            {
                // Have to generate axes & offsets per billboard
                genBillboardAxes( &mCamX, &mCamY, &bb );
            }

            // If they're all the same size or we're point rendering
            if( mAllDefaultSize || mPointRendering )
            {
                /* No per-billboard checking, just blast through.
                Saves us an if clause every billboard which may
                make a difference.
                */

                if( !mPointRendering &&
                    ( mBillboardType == BBT_ORIENTED_SELF || mBillboardType == BBT_PERPENDICULAR_SELF ||
                      ( mAccurateFacing && mBillboardType != BBT_PERPENDICULAR_COMMON ) ) )
                {
                    genVertOffsets( mLeftOff, mRightOff, mTopOff, mBottomOff, mDefaultWidth,
                                    mDefaultHeight, mCamX, mCamY, mVOffset );
                }
                genVertices( mVOffset, bb );
            }
            else  // not all default size and not point rendering
            {
                Vector3 vOwnOffset[4];
                // If it has own dimensions, or self-oriented, gen offsets
                if( mBillboardType == BBT_ORIENTED_SELF || mBillboardType == BBT_PERPENDICULAR_SELF ||
                    bb.mOwnDimensions ||
                    ( mAccurateFacing && mBillboardType != BBT_PERPENDICULAR_COMMON ) )
                {
                    // Generate using own dimensions
                    genVertOffsets( mLeftOff, mRightOff, mTopOff, mBottomOff, bb.mWidth, bb.mHeight,
                                    mCamX, mCamY, vOwnOffset );
                    // Create vertex data
                    genVertices( vOwnOffset, bb );
                }
                else  // Use default dimension, already computed before the loop, for faster creation
                {
                    genVertices( mVOffset, bb );
                }
            }
            // Increment visibles
            mNumVisibleBillboards++;
        }
        //-----------------------------------------------------------------------
        void BillboardSet::endBillboards() { mMainBuf->unlock(); }
        //-----------------------------------------------------------------------
        void BillboardSet::setBounds( const Aabb &aabb, Real radius )
        {
            mObjectData.mLocalAabb->setFromAabb( aabb, mObjectData.mIndex );
            mObjectData.mLocalRadius[mObjectData.mIndex] = radius;
        }
        //-----------------------------------------------------------------------
        void BillboardSet::_updateBounds()
        {
            if( mActiveBillboards.empty() )
            {
                // No billboards, null bbox
                mObjectData.mLocalAabb->setFromAabb( Aabb::BOX_NULL, mObjectData.mIndex );
                mObjectData.mLocalRadius[mObjectData.mIndex] = 0.0f;
            }
            else
            {
                Real maxSqLen = -1.0f;

                Vector3 min( Math::POS_INFINITY, Math::POS_INFINITY, Math::POS_INFINITY );
                Vector3 max( Math::NEG_INFINITY, Math::NEG_INFINITY, Math::NEG_INFINITY );
                ActiveBillboardList::iterator i, iend;

                iend = mActiveBillboards.end();
                Matrix4 invWorld;
                if( mWorldSpace && getParentNode() )
                    invWorld = getParentNode()->_getFullTransform().inverse();

                for( i = mActiveBillboards.begin(); i != iend; ++i )
                {
                    Vector3 pos = ( *i )->getPosition();
                    // transform from world space to local space
                    if( mWorldSpace && getParentNode() )
                        pos = invWorld * pos;
                    min.makeFloor( pos );
                    max.makeCeil( pos );

                    maxSqLen = std::max( maxSqLen, pos.squaredLength() );
                }
                // Adjust for billboard size
                Real adjust = std::max( mDefaultWidth, mDefaultHeight );
                Vector3 vecAdjust( adjust, adjust, adjust );
                min -= vecAdjust;
                max += vecAdjust;

                mObjectData.mLocalAabb->setFromAabb( Aabb::newFromExtents( min, max ),
                                                     mObjectData.mIndex );
                mObjectData.mLocalRadius[mObjectData.mIndex] = Math::Sqrt( maxSqLen );
            }
        }
        //-----------------------------------------------------------------------
        void BillboardSet::_updateRenderQueue( RenderQueue *queue, Camera *camera,
                                               const Camera *lodCamera )
        {
            _notifyCurrentCamera( camera, lodCamera );
            _updateRenderQueueImpl( queue, camera, lodCamera );
        }
        //-----------------------------------------------------------------------
        void BillboardSet::_updateRenderQueueImpl( RenderQueue *queue, Camera *camera,
                                                   const Camera *lodCamera )
        {
            // If we're driving this from our own data, update geometry if need to.
            if( !mExternalData && ( mAutoUpdate || mBillboardDataChanged || !mBuffersCreated ) )
            {
                if( mSortingEnabled )
                {
                    _sortBillboards();
                }

                beginBillboards( mActiveBillboards.size() );
                ActiveBillboardList::iterator it;
                for( it = mActiveBillboards.begin(); it != mActiveBillboards.end(); ++it )
                {
                    injectBillboard( *( *it ), lodCamera );
                }
                endBillboards();

                mRenderables.clear();
                if( !mActiveBillboards.empty() )
                    mRenderables.push_back( this );

                mBillboardDataChanged = false;
            }
        }
        //-----------------------------------------------------------------------
        void BillboardSet::getRenderOperation( RenderOperation &op, bool casterPass )
        {
            op.vertexData = mVertexData;
            op.vertexData->vertexStart = 0;

            if( mPointRendering )
            {
                op.operationType = OT_POINT_LIST;
                op.useIndexes = false;
                op.useGlobalInstancingVertexBufferIsAvailable = false;
                op.indexData = 0;
                op.vertexData->vertexCount = mNumVisibleBillboards;
            }
            else
            {
                op.operationType = OT_TRIANGLE_LIST;
                op.useIndexes = true;

                op.vertexData->vertexCount = mNumVisibleBillboards * 4;

                op.indexData = mIndexData;
                op.indexData->indexCount = mNumVisibleBillboards * 6;
                op.indexData->indexStart = 0;
            }
        }

        //-----------------------------------------------------------------------
        void BillboardSet::getWorldTransforms( Matrix4 *xform ) const
        {
            if( mWorldSpace )
            {
                *xform = Matrix4::IDENTITY;
            }
            else
            {
                *xform = _getParentNodeFullTransform();
            }
        }

        //-----------------------------------------------------------------------
        void BillboardSet::setAutoextend( bool autoextend ) { mAutoExtendPool = autoextend; }

        //-----------------------------------------------------------------------
        bool BillboardSet::getAutoextend() const { return mAutoExtendPool; }

        //-----------------------------------------------------------------------
        void BillboardSet::setSortingEnabled( bool sortenable ) { mSortingEnabled = sortenable; }

        //-----------------------------------------------------------------------
        bool BillboardSet::getSortingEnabled() const { return mSortingEnabled; }

        //-----------------------------------------------------------------------
        void BillboardSet::setPoolSize( size_t size )
        {
            // If we're driving this from our own data, allocate billboards
            if( !mExternalData )
            {
                // Never shrink below size()
                size_t currSize = mBillboardPool.size();
                if( currSize >= size )
                    return;

                this->increasePool( size );

                for( size_t i = currSize; i < size; ++i )
                {
                    // Add new items to the queue
                    mFreeBillboards.push_back( mBillboardPool[i] );
                }
            }

            mPoolSize = size;

            _destroyBuffers();
        }
        //-----------------------------------------------------------------------
        void BillboardSet::createExtraVertexBuffer( size_t vertexSize )
        {
            size_t vertexCount = mPoolSize;
            if( !mPointRendering )
                vertexCount = mPoolSize * 4;

            if( mAutoUpdate )
            {
                const size_t dynamicBufferMultiplier = mVaoManager->getDynamicBufferMultiplier();

                if( mMainBuffers.size() != dynamicBufferMultiplier )
                    mMainBuffers.resize( dynamicBufferMultiplier );

                for( size_t i = 0; i < dynamicBufferMultiplier; ++i )
                {
                    mMainBuffers[i].push_back(
                        mVertexData->_getHardwareBufferManager()->createVertexBuffer(
                            vertexSize, vertexCount * dynamicBufferMultiplier,
                            HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE ) );
                }
            }
            else
            {
                if( mMainBuffers.empty() )
                    mMainBuffers.resize( 1 );

                assert( mMainBuffers[0].empty() );

                mMainBuffers[0].push_back( mVertexData->_getHardwareBufferManager()->createVertexBuffer(
                    vertexSize, vertexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY ) );
            }
        }
        //-----------------------------------------------------------------------
        void BillboardSet::_createBuffers()
        {
            mVaoManager = mManager->getDestinationRenderSystem()->getVaoManager();

            /* Allocate / reallocate vertex data
               Note that we allocate enough space for ALL the billboards in the pool, but only issue
               rendering operations for the sections relating to the active billboards
            */

            /* Alloc positions   ( 1 or 4 verts per billboard, 3 components )
                     colours     ( 1 x RGBA per vertex )
                     indices     ( 6 per billboard ( 2 tris ) if not point rendering )
                     tex. coords ( 2D coords, 1 or 4 per billboard )
            */

            // Warn if user requested an invalid setup
            // Do it here so it only appears once
            if( mPointRendering && mBillboardType != BBT_POINT )
            {
                LogManager::getSingleton().logMessage(
                    "Warning: BillboardSet " + mName +
                        " has point rendering enabled but is using a type "
                        "other than BBT_POINT, this may not give you the results you "
                        "expect.",
                    LML_CRITICAL );
            }

            mVertexData = OGRE_NEW VertexData( NULL );
            if( mPointRendering )
                mVertexData->vertexCount = mPoolSize;
            else
                mVertexData->vertexCount = mPoolSize * 4;

            mVertexData->vertexStart = 0;

            // Vertex declaration
            VertexDeclaration *decl = mVertexData->vertexDeclaration;
            VertexBufferBinding *binding = mVertexData->vertexBufferBinding;

            size_t offset = 0;
            decl->addElement( 0, offset, VET_FLOAT3, VES_POSITION );
            offset += VertexElement::getTypeSize( VET_FLOAT3 );
            decl->addElement( 0, offset, VET_COLOUR, VES_DIFFUSE );
            offset += VertexElement::getTypeSize( VET_COLOUR );
            // Texture coords irrelevant when enabled point rendering (generated
            // in point sprite mode, and unused in standard point mode)
            if( !mPointRendering )
            {
                decl->addElement( 0, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0 );
            }

            createExtraVertexBuffer( decl->getVertexSize( 0 ) );

            mLastLockedBuffer = 0;
            mLastLockedFrame = mVaoManager->getFrameCount() - 1;
            mMainBuf = mMainBuffers[0][0];

            // bind position and diffuses
            binding->setBinding( 0, mMainBuffers[0][0] );

            if( !mPointRendering )
            {
                mIndexData = OGRE_NEW IndexData();
                mIndexData->indexStart = 0;
                mIndexData->indexCount = mPoolSize * 6;

                mIndexData->indexBuffer = mVertexData->_getHardwareBufferManager()->createIndexBuffer(
                    HardwareIndexBuffer::IT_16BIT, mIndexData->indexCount,
                    HardwareBuffer::HBU_STATIC_WRITE_ONLY );

                /* Create indexes (will be the same every frame)
                   Using indexes because it means 1/3 less vertex transforms (4 instead of 6)

                   Billboard layout relative to camera:

                    0-----1
                    |    /|
                    |  /  |
                    |/    |
                    2-----3
                */

                HardwareBufferLockGuard indexLock( mIndexData->indexBuffer,
                                                   HardwareBuffer::HBL_DISCARD );
                ushort *pIdx = static_cast<ushort *>( indexLock.pData );

                for( size_t idx, idxOff, bboard = 0; bboard < mPoolSize; ++bboard )
                {
                    // Do indexes
                    idx = bboard * 6;
                    idxOff = bboard * 4;

                    pIdx[idx] = static_cast<unsigned short>( idxOff );  // + 0;, for clarity
                    pIdx[idx + 1] = static_cast<unsigned short>( idxOff + 2 );
                    pIdx[idx + 2] = static_cast<unsigned short>( idxOff + 1 );
                    pIdx[idx + 3] = static_cast<unsigned short>( idxOff + 1 );
                    pIdx[idx + 4] = static_cast<unsigned short>( idxOff + 2 );
                    pIdx[idx + 5] = static_cast<unsigned short>( idxOff + 3 );
                }
            }

            if( mHlmsDatablock && !getMaterial() )
            {
                // Needed to force Hlms to recalculate the hash (vertex attributes may have changed)
                mHlmsDatablock->_unlinkRenderable( this );
                mHlmsDatablock = 0;
            }

            mBuffersCreated = true;

            if( mMaterialName.empty() )
            {
                Hlms *hlms = Root::getSingleton().getHlmsManager()->getHlms( HLMS_PBS );
                setDatablock( hlms->getDefaultDatablock() );
            }
            else
            {
                if( mMaterialGroup.empty() )
                {
                    Hlms *hlms = Root::getSingleton().getHlmsManager()->getHlms( HLMS_PBS );
                    HlmsManager *hlmsManager = Root::getSingleton().getHlmsManager();
                    HlmsDatablock *datablock = hlmsManager->getDatablockNoDefault( mMaterialName );

                    if( datablock )
                        setDatablock( datablock );
                    else
                        setDatablock( hlms->getDefaultDatablock() );
                }
                else
                {
                    setMaterialName( mMaterialName, mMaterialGroup );
                }
            }
        }
        //-----------------------------------------------------------------------
        void BillboardSet::_destroyBuffers()
        {
            if( mVertexData )
            {
                OGRE_DELETE mVertexData;
                mVertexData = 0;
            }
            if( mIndexData )
            {
                OGRE_DELETE mIndexData;
                mIndexData = 0;
            }

            mMainBuf.reset();
            mMainBuffers.clear();

            if( mHlmsDatablock && !getMaterial() )
            {
                _setHlmsHashes( 0u, 0u );
                mHlmsDatablock->_unlinkRenderable( this );
                mHlmsDatablock = 0;
            }

            mBuffersCreated = false;
        }
        //-----------------------------------------------------------------------
        void BillboardSet::setMaterial( const MaterialPtr &material )
        {
            mMaterialName = material->getName();
            mMaterialGroup = material->getGroup();

            if( mBuffersCreated )
                Renderable::setMaterial( material );
        }
        //-----------------------------------------------------------------------
        void BillboardSet::setDatablock( HlmsDatablock *datablock )
        {
            if( datablock->getCreator()->getType() != HLMS_LOW_LEVEL )
            {
                mMaterialName.clear();
                mMaterialGroup.clear();

                const String *fullDatablockName = datablock->getNameStr();
                if( fullDatablockName )
                {
                    mMaterialName = *fullDatablockName;
                }
                else
                {
                    LogManager::getSingleton().logMessage(
                        "Couldn't retrieve full material name of datablock '" +
                        datablock->getName().getFriendlyText() +
                        "' Billboard may not render as expected. "
                        "May be the datablock is scheduled for deletion?" );
                }
            }

            if( mBuffersCreated )
            {
                // HACK
                // We should always call setDatablock, but we can't due to how Billboards
                // currently work.
                Renderable::setDatablock( datablock );
            }
            else
            {
                // HACK
                // If we're here datablock will later be assigned too late (i.e. inside render)
                // and if the textures haven't changed to Resident yet, calculateHashFor
                // will delay hash assignment but it's too late to delay, causing bugs.
                // Thus we force the datablock to preload now (calculateHashFor would normally
                // do this too when it calls loadAllTextures anyway).
                datablock->preload();
            }
        }
        //-----------------------------------------------------------------------
        void BillboardSet::_setNullDatablock()
        {
            mMaterialName.clear();
            mMaterialGroup.clear();
            Renderable::_setNullDatablock();
        }
        //-----------------------------------------------------------------------
        unsigned int BillboardSet::getPoolSize() const
        {
            return static_cast<unsigned int>( mBillboardPool.size() );
        }

        //-----------------------------------------------------------------------
        void BillboardSet::_notifyBillboardResized() { mAllDefaultSize = false; }

        //-----------------------------------------------------------------------
        void BillboardSet::_notifyBillboardRotated() { mAllDefaultRotation = false; }

        //-----------------------------------------------------------------------
        void BillboardSet::getParametricOffsets( Real &left, Real &right, Real &top, Real &bottom )
        {
            switch( mOriginType )
            {
            case BBO_TOP_LEFT:
                left = 0.0f;
                right = 1.0f;
                top = 0.0f;
                bottom = -1.0f;
                break;

            case BBO_TOP_CENTER:
                left = -0.5f;
                right = 0.5f;
                top = 0.0f;
                bottom = -1.0f;
                break;

            case BBO_TOP_RIGHT:
                left = -1.0f;
                right = 0.0f;
                top = 0.0f;
                bottom = -1.0f;
                break;

            case BBO_CENTER_LEFT:
                left = 0.0f;
                right = 1.0f;
                top = 0.5f;
                bottom = -0.5f;
                break;

            case BBO_CENTER:
                left = -0.5f;
                right = 0.5f;
                top = 0.5f;
                bottom = -0.5f;
                break;

            case BBO_CENTER_RIGHT:
                left = -1.0f;
                right = 0.0f;
                top = 0.5f;
                bottom = -0.5f;
                break;

            case BBO_BOTTOM_LEFT:
                left = 0.0f;
                right = 1.0f;
                top = 1.0f;
                bottom = 0.0f;
                break;

            case BBO_BOTTOM_CENTER:
                left = -0.5f;
                right = 0.5f;
                top = 1.0f;
                bottom = 0.0f;
                break;

            case BBO_BOTTOM_RIGHT:
                left = -1.0f;
                right = 0.0f;
                top = 1.0f;
                bottom = 0.0f;
                break;
            }
        }
        //-----------------------------------------------------------------------
        bool BillboardSet::getCullIndividually() const { return mCullIndividual; }
        //-----------------------------------------------------------------------
        void BillboardSet::setCullIndividually( bool cullIndividual )
        {
            mCullIndividual = cullIndividual;
        }
        //-----------------------------------------------------------------------
        bool BillboardSet::billboardVisible( const Camera *cam, const Billboard &bill )
        {
            // Return always visible if not culling individually
            if( !mCullIndividual )
                return true;

            // Cull based on sphere (have to transform less)
            Sphere sph;
            Matrix4 xworld;

            getWorldTransforms( &xworld );

            sph.setCenter( xworld.transformAffine( bill.mPosition ) );

            if( bill.mOwnDimensions )
            {
                sph.setRadius( std::max( bill.mWidth, bill.mHeight ) );
            }
            else
            {
                sph.setRadius( std::max( mDefaultWidth, mDefaultHeight ) );
            }

            return cam->isVisible( sph );
        }
        //-----------------------------------------------------------------------
        void BillboardSet::increasePool( size_t size )
        {
            size_t oldSize = mBillboardPool.size();

            // Increase size
            mBillboardPool.reserve( size );
            mBillboardPool.resize( size );

            // Create new billboards
            for( size_t i = oldSize; i < size; ++i )
                mBillboardPool[i] = OGRE_NEW Billboard();
        }
        //-----------------------------------------------------------------------
        void BillboardSet::genBillboardAxes( Vector3 *pX, Vector3 *pY, const Billboard *bb )
        {
            // If we're using accurate facing, recalculate camera direction per BB
            if( mAccurateFacing &&
                ( mBillboardType == BBT_POINT || mBillboardType == BBT_ORIENTED_COMMON ||
                  mBillboardType == BBT_ORIENTED_SELF ) )
            {
                // cam -> bb direction
                mCamDir = bb->mPosition - mCamPos;
                mCamDir.normalise();
            }

            switch( mBillboardType )
            {
            case BBT_POINT:
                if( mAccurateFacing )
                {
                    // Point billboards will have 'up' based on but not equal to cameras
                    // Use pY temporarily to avoid allocation
                    *pY = mCamQ * Vector3::UNIT_Y;
                    *pX = mCamDir.crossProduct( *pY );
                    pX->normalise();
                    *pY = pX->crossProduct( mCamDir );  // both normalised already
                }
                else
                {
                    // Get camera axes for X and Y (depth is irrelevant)
                    *pX = mCamQ * Vector3::UNIT_X;
                    *pY = mCamQ * Vector3::UNIT_Y;
                }
                break;

            case BBT_ORIENTED_COMMON:
                // Y-axis is common direction
                // X-axis is cross with camera direction
                *pY = mCommonDirection;
                *pX = mCamDir.crossProduct( *pY );
                pX->normalise();
                break;

            case BBT_ORIENTED_SELF:
                // Y-axis is direction
                // X-axis is cross with camera direction
                // Scale direction first
                *pY = bb->mDirection;
                *pX = mCamDir.crossProduct( *pY );
                pX->normalise();
                break;

            case BBT_PERPENDICULAR_COMMON:
                // X-axis is up-vector cross common direction
                // Y-axis is common direction cross X-axis
                *pX = mCommonUpVector.crossProduct( mCommonDirection );
                *pY = mCommonDirection.crossProduct( *pX );
                break;

            case BBT_PERPENDICULAR_SELF:
                // X-axis is up-vector cross own direction
                // Y-axis is own direction cross X-axis
                *pX = mCommonUpVector.crossProduct( bb->mDirection );
                pX->normalise();
                *pY = bb->mDirection.crossProduct( *pX );  // both should be normalised
                break;
            }
        }
        //-----------------------------------------------------------------------
        void BillboardSet::setBillboardType( BillboardType bbt ) { mBillboardType = bbt; }
        //-----------------------------------------------------------------------
        BillboardType BillboardSet::getBillboardType() const { return mBillboardType; }
        //-----------------------------------------------------------------------
        void BillboardSet::setCommonDirection( const Vector3 &vec ) { mCommonDirection = vec; }
        //-----------------------------------------------------------------------
        const Vector3 &BillboardSet::getCommonDirection() const { return mCommonDirection; }
        //-----------------------------------------------------------------------
        void BillboardSet::setCommonUpVector( const Vector3 &vec ) { mCommonUpVector = vec; }
        //-----------------------------------------------------------------------
        const Vector3 &BillboardSet::getCommonUpVector() const { return mCommonUpVector; }
        //-----------------------------------------------------------------------
        void BillboardSet::genVertices( const Vector3 *const offsets, const Billboard &bb )
        {
            RGBA colour;
            Root::getSingleton().convertColourValue( bb.mColour, &colour );
            RGBA *pCol;

            // Texcoords
            assert( bb.mUseTexcoordRect || bb.mTexcoordIndex < mTextureCoords.size() );
            const Ogre::FloatRect &r =
                bb.mUseTexcoordRect ? bb.mTexcoordRect : mTextureCoords[bb.mTexcoordIndex];

            if( mPointRendering )
            {
                // Single vertex per billboard, ignore offsets
                // position
                *mLockPtr++ = bb.mPosition.x;
                *mLockPtr++ = bb.mPosition.y;
                *mLockPtr++ = bb.mPosition.z;
                // Colour
                // Convert float* to RGBA*
                pCol = static_cast<RGBA *>( static_cast<void *>( mLockPtr ) );
                *pCol++ = colour;
                // Update lock pointer
                mLockPtr = static_cast<float *>( static_cast<void *>( pCol ) );
                // No texture coords in point rendering
            }
            else if( mAllDefaultRotation || bb.mRotation == Radian( 0 ) )
            {
                // Left-top
                // Positions
                *mLockPtr++ = offsets[0].x + bb.mPosition.x;
                *mLockPtr++ = offsets[0].y + bb.mPosition.y;
                *mLockPtr++ = offsets[0].z + bb.mPosition.z;
                // Colour
                // Convert float* to RGBA*
                pCol = static_cast<RGBA *>( static_cast<void *>( mLockPtr ) );
                *pCol++ = colour;
                // Update lock pointer
                mLockPtr = static_cast<float *>( static_cast<void *>( pCol ) );
                // Texture coords
                *mLockPtr++ = r.left;
                *mLockPtr++ = r.top;

                // Right-top
                // Positions
                *mLockPtr++ = offsets[1].x + bb.mPosition.x;
                *mLockPtr++ = offsets[1].y + bb.mPosition.y;
                *mLockPtr++ = offsets[1].z + bb.mPosition.z;
                // Colour
                // Convert float* to RGBA*
                pCol = static_cast<RGBA *>( static_cast<void *>( mLockPtr ) );
                *pCol++ = colour;
                // Update lock pointer
                mLockPtr = static_cast<float *>( static_cast<void *>( pCol ) );
                // Texture coords
                *mLockPtr++ = r.right;
                *mLockPtr++ = r.top;

                // Left-bottom
                // Positions
                *mLockPtr++ = offsets[2].x + bb.mPosition.x;
                *mLockPtr++ = offsets[2].y + bb.mPosition.y;
                *mLockPtr++ = offsets[2].z + bb.mPosition.z;
                // Colour
                // Convert float* to RGBA*
                pCol = static_cast<RGBA *>( static_cast<void *>( mLockPtr ) );
                *pCol++ = colour;
                // Update lock pointer
                mLockPtr = static_cast<float *>( static_cast<void *>( pCol ) );
                // Texture coords
                *mLockPtr++ = r.left;
                *mLockPtr++ = r.bottom;

                // Right-bottom
                // Positions
                *mLockPtr++ = offsets[3].x + bb.mPosition.x;
                *mLockPtr++ = offsets[3].y + bb.mPosition.y;
                *mLockPtr++ = offsets[3].z + bb.mPosition.z;
                // Colour
                // Convert float* to RGBA*
                pCol = static_cast<RGBA *>( static_cast<void *>( mLockPtr ) );
                *pCol++ = colour;
                // Update lock pointer
                mLockPtr = static_cast<float *>( static_cast<void *>( pCol ) );
                // Texture coords
                *mLockPtr++ = r.right;
                *mLockPtr++ = r.bottom;
            }
            else if( mRotationType == BBR_VERTEX )
            {
                // TODO: Cache axis when billboard type is BBT_POINT or BBT_PERPENDICULAR_COMMON
                Vector3 axis =
                    ( offsets[3] - offsets[0] ).crossProduct( offsets[2] - offsets[1] ).normalisedCopy();

                Matrix3 rotation;
                rotation.FromAngleAxis( axis, bb.mRotation );

                Vector3 pt;

                // Left-top
                // Positions
                pt = rotation * offsets[0];
                *mLockPtr++ = pt.x + bb.mPosition.x;
                *mLockPtr++ = pt.y + bb.mPosition.y;
                *mLockPtr++ = pt.z + bb.mPosition.z;
                // Colour
                // Convert float* to RGBA*
                pCol = static_cast<RGBA *>( static_cast<void *>( mLockPtr ) );
                *pCol++ = colour;
                // Update lock pointer
                mLockPtr = static_cast<float *>( static_cast<void *>( pCol ) );
                // Texture coords
                *mLockPtr++ = r.left;
                *mLockPtr++ = r.top;

                // Right-top
                // Positions
                pt = rotation * offsets[1];
                *mLockPtr++ = pt.x + bb.mPosition.x;
                *mLockPtr++ = pt.y + bb.mPosition.y;
                *mLockPtr++ = pt.z + bb.mPosition.z;
                // Colour
                // Convert float* to RGBA*
                pCol = static_cast<RGBA *>( static_cast<void *>( mLockPtr ) );
                *pCol++ = colour;
                // Update lock pointer
                mLockPtr = static_cast<float *>( static_cast<void *>( pCol ) );
                // Texture coords
                *mLockPtr++ = r.right;
                *mLockPtr++ = r.top;

                // Left-bottom
                // Positions
                pt = rotation * offsets[2];
                *mLockPtr++ = pt.x + bb.mPosition.x;
                *mLockPtr++ = pt.y + bb.mPosition.y;
                *mLockPtr++ = pt.z + bb.mPosition.z;
                // Colour
                // Convert float* to RGBA*
                pCol = static_cast<RGBA *>( static_cast<void *>( mLockPtr ) );
                *pCol++ = colour;
                // Update lock pointer
                mLockPtr = static_cast<float *>( static_cast<void *>( pCol ) );
                // Texture coords
                *mLockPtr++ = r.left;
                *mLockPtr++ = r.bottom;

                // Right-bottom
                // Positions
                pt = rotation * offsets[3];
                *mLockPtr++ = pt.x + bb.mPosition.x;
                *mLockPtr++ = pt.y + bb.mPosition.y;
                *mLockPtr++ = pt.z + bb.mPosition.z;
                // Colour
                // Convert float* to RGBA*
                pCol = static_cast<RGBA *>( static_cast<void *>( mLockPtr ) );
                *pCol++ = colour;
                // Update lock pointer
                mLockPtr = static_cast<float *>( static_cast<void *>( pCol ) );
                // Texture coords
                *mLockPtr++ = r.right;
                *mLockPtr++ = r.bottom;
            }
            else
            {
                const Real cos_rot( Math::Cos( bb.mRotation ) );
                const Real sin_rot( Math::Sin( bb.mRotation ) );

                float width = ( r.right - r.left ) / 2;
                float height = ( r.bottom - r.top ) / 2;
                float mid_u = r.left + width;
                float mid_v = r.top + height;

                float cos_rot_w = cos_rot * width;
                float cos_rot_h = cos_rot * height;
                float sin_rot_w = sin_rot * width;
                float sin_rot_h = sin_rot * height;

                // Left-top
                // Positions
                *mLockPtr++ = offsets[0].x + bb.mPosition.x;
                *mLockPtr++ = offsets[0].y + bb.mPosition.y;
                *mLockPtr++ = offsets[0].z + bb.mPosition.z;
                // Colour
                // Convert float* to RGBA*
                pCol = static_cast<RGBA *>( static_cast<void *>( mLockPtr ) );
                *pCol++ = colour;
                // Update lock pointer
                mLockPtr = static_cast<float *>( static_cast<void *>( pCol ) );
                // Texture coords
                *mLockPtr++ = mid_u - cos_rot_w + sin_rot_h;
                *mLockPtr++ = mid_v - sin_rot_w - cos_rot_h;

                // Right-top
                // Positions
                *mLockPtr++ = offsets[1].x + bb.mPosition.x;
                *mLockPtr++ = offsets[1].y + bb.mPosition.y;
                *mLockPtr++ = offsets[1].z + bb.mPosition.z;
                // Colour
                // Convert float* to RGBA*
                pCol = static_cast<RGBA *>( static_cast<void *>( mLockPtr ) );
                *pCol++ = colour;
                // Update lock pointer
                mLockPtr = static_cast<float *>( static_cast<void *>( pCol ) );
                // Texture coords
                *mLockPtr++ = mid_u + cos_rot_w + sin_rot_h;
                *mLockPtr++ = mid_v + sin_rot_w - cos_rot_h;

                // Left-bottom
                // Positions
                *mLockPtr++ = offsets[2].x + bb.mPosition.x;
                *mLockPtr++ = offsets[2].y + bb.mPosition.y;
                *mLockPtr++ = offsets[2].z + bb.mPosition.z;
                // Colour
                // Convert float* to RGBA*
                pCol = static_cast<RGBA *>( static_cast<void *>( mLockPtr ) );
                *pCol++ = colour;
                // Update lock pointer
                mLockPtr = static_cast<float *>( static_cast<void *>( pCol ) );
                // Texture coords
                *mLockPtr++ = mid_u - cos_rot_w - sin_rot_h;
                *mLockPtr++ = mid_v - sin_rot_w + cos_rot_h;

                // Right-bottom
                // Positions
                *mLockPtr++ = offsets[3].x + bb.mPosition.x;
                *mLockPtr++ = offsets[3].y + bb.mPosition.y;
                *mLockPtr++ = offsets[3].z + bb.mPosition.z;
                // Colour
                // Convert float* to RGBA*
                pCol = static_cast<RGBA *>( static_cast<void *>( mLockPtr ) );
                *pCol++ = colour;
                // Update lock pointer
                mLockPtr = static_cast<float *>( static_cast<void *>( pCol ) );
                // Texture coords
                *mLockPtr++ = mid_u + cos_rot_w - sin_rot_h;
                *mLockPtr++ = mid_v + sin_rot_w + cos_rot_h;
            }
        }
        //-----------------------------------------------------------------------
        void BillboardSet::genVertOffsets( Real inleft, Real inright, Real intop, Real inbottom,
                                           Real width, Real height, const Vector3 &x, const Vector3 &y,
                                           Vector3 *pDestVec )
        {
            Vector3 vLeftOff, vRightOff, vTopOff, vBottomOff;
            /* Calculate default offsets. Scale the axes by
               parametric offset and dimensions, ready to be added to
               positions.
            */

            vLeftOff = x * ( inleft * width );
            vRightOff = x * ( inright * width );
            vTopOff = y * ( intop * height );
            vBottomOff = y * ( inbottom * height );

            // Make final offsets to vertex positions
            pDestVec[0] = vLeftOff + vTopOff;
            pDestVec[1] = vRightOff + vTopOff;
            pDestVec[2] = vLeftOff + vBottomOff;
            pDestVec[3] = vRightOff + vBottomOff;
        }
        //-----------------------------------------------------------------------
        const String &BillboardSet::getMovableType() const
        {
            return BillboardSetFactory::FACTORY_TYPE_NAME;
        }
        //-----------------------------------------------------------------------
        Real BillboardSet::getSquaredViewDepth( const Camera *const cam ) const
        {
            assert( mParentNode );
            return mParentNode->getSquaredViewDepth( cam );
        }
        //-----------------------------------------------------------------------
        const LightList &BillboardSet::getLights() const
        {
            // It's actually quite unlikely that this will be called,
            // because most billboards are unlit, but here we go anyway
            return queryLights();
        }

        void BillboardSet::setTextureCoords( Ogre::FloatRect const *coords, uint16 numCoords )
        {
            if( !numCoords || !coords )
            {
                setTextureStacksAndSlices( 1, 1 );
                return;
            }
            //  clear out any previous allocation (as vectors may not shrink)
            TextureCoordSets().swap( mTextureCoords );
            //  make room
            mTextureCoords.resize( numCoords );
            //  copy in data
            std::copy( coords, coords + numCoords, &mTextureCoords.front() );
        }

        void BillboardSet::setTextureStacksAndSlices( uchar stacks, uchar slices )
        {
            if( stacks == 0 )
                stacks = 1;
            if( slices == 0 )
                slices = 1;
            //  clear out any previous allocation (as vectors may not shrink)
            TextureCoordSets().swap( mTextureCoords );
            //  make room
            mTextureCoords.resize( (size_t)stacks * slices );
            unsigned int coordIndex = 0;
            //  spread the U and V coordinates across the rects
            for( uint v = 0; v < stacks; ++v )
            {
                //  (float)X / X is guaranteed to be == 1.0f for X up to 8 million, so
                //  our range of 1..256 is quite enough to guarantee perfect coverage.
                float top = (float)v / (float)stacks;
                float bottom = ( (float)v + 1 ) / (float)stacks;
                for( uint u = 0; u < slices; ++u )
                {
                    Ogre::FloatRect &r = mTextureCoords[coordIndex];
                    r.left = (float)u / (float)slices;
                    r.bottom = bottom;
                    r.right = ( (float)u + 1 ) / (float)slices;
                    r.top = top;
                    ++coordIndex;
                }
            }
            assert( coordIndex == (size_t)stacks * slices );
        }
        //-----------------------------------------------------------------------
        Ogre::FloatRect const *BillboardSet::getTextureCoords( uint16 *oNumCoords )
        {
            *oNumCoords = (uint16)mTextureCoords.size();
            //  std::vector<> is guaranteed to be contiguous
            return &mTextureCoords.front();
        }
        //-----------------------------------------------------------------------
        void BillboardSet::setPointRenderingEnabled( bool enabled )
        {
            // Override point rendering if not supported
            if( enabled && !Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(
                               RSC_POINT_SPRITES ) )
            {
                enabled = false;
            }

            if( enabled != mPointRendering )
            {
                mPointRendering = enabled;
                // Different buffer structure (1 or 4 verts per billboard)
                _destroyBuffers();
            }
        }

        //-----------------------------------------------------------------------
        void BillboardSet::setAutoUpdate( bool autoUpdate )
        {
            // Case auto update buffers changed we have to destroy the current buffers
            // since their usage will be different.
            if( autoUpdate != mAutoUpdate )
            {
                mAutoUpdate = autoUpdate;
                _destroyBuffers();
            }
        }

        //-----------------------------------------------------------------------
        //-----------------------------------------------------------------------
        String BillboardSetFactory::FACTORY_TYPE_NAME = "BillboardSet";
        //-----------------------------------------------------------------------
        const String &BillboardSetFactory::getType() const { return FACTORY_TYPE_NAME; }
        //-----------------------------------------------------------------------
        MovableObject *BillboardSetFactory::createInstanceImpl( IdType id,
                                                                ObjectMemoryManager *objectMemoryManager,
                                                                SceneManager *manager,
                                                                const NameValuePairList *params )
        {
            // may have parameters
            bool externalData = false;
            unsigned int poolSize = 0;

            if( params != 0 )
            {
                NameValuePairList::const_iterator ni = params->find( "poolSize" );
                if( ni != params->end() )
                {
                    poolSize = StringConverter::parseUnsignedInt( ni->second );
                }
                ni = params->find( "externalData" );
                if( ni != params->end() )
                {
                    externalData = StringConverter::parseBool( ni->second );
                }
            }

            if( poolSize > 0 )
            {
                return OGRE_NEW BillboardSet( id, objectMemoryManager, manager, poolSize, externalData );
            }
            else
            {
                return OGRE_NEW BillboardSet( id, objectMemoryManager, manager );
            }
        }
        //-----------------------------------------------------------------------
        void BillboardSetFactory::destroyInstance( MovableObject *obj ) { OGRE_DELETE obj; }

    }  // namespace v1
}  // namespace Ogre
