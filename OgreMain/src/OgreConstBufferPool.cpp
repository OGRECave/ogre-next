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

#include "OgreConstBufferPool.h"

#include "OgreProfiler.h"
#include "OgreRenderSystem.h"
#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreReadOnlyBufferPacked.h"
#include "Vao/OgreStagingBuffer.h"
#include "Vao/OgreVaoManager.h"

namespace Ogre
{
    ConstBufferPool::ConstBufferPool( uint32 bytesPerSlot, const ExtraBufferParams &extraBufferParams ) :
        mBytesPerSlot( bytesPerSlot ),
        mSlotsPerPool( 0 ),
        mBufferSize( 0 ),
        mExtraBufferParams( extraBufferParams ),
        _mVaoManager( 0 ),
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS && OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
        mOptimizationStrategy( LowerCpuOverhead )
#else
        mOptimizationStrategy( LowerGpuOverhead )
#endif
    {
    }
    //-----------------------------------------------------------------------------------
    ConstBufferPool::~ConstBufferPool() { destroyAllPools(); }
    //-----------------------------------------------------------------------------------
    void ConstBufferPool::destroyAllPools()
    {
        {
            BufferPoolVecMap::const_iterator itor = mPools.begin();
            BufferPoolVecMap::const_iterator endt = mPools.end();

            while( itor != endt )
            {
                BufferPoolVec::const_iterator it = itor->second.begin();
                BufferPoolVec::const_iterator en = itor->second.end();

                while( it != en )
                {
                    _mVaoManager->destroyConstBuffer( ( *it )->materialBuffer );

                    if( ( *it )->extraBuffer )
                    {
                        if( mExtraBufferParams.useReadOnlyBuffers )
                        {
                            assert( dynamic_cast<ReadOnlyBufferPacked *>( ( *it )->extraBuffer ) );
                            _mVaoManager->destroyReadOnlyBuffer(
                                static_cast<ReadOnlyBufferPacked *>( ( *it )->extraBuffer ) );
                        }
                        else
                        {
                            assert( dynamic_cast<ConstBufferPacked *>( ( *it )->extraBuffer ) );
                            _mVaoManager->destroyConstBuffer(
                                static_cast<ConstBufferPacked *>( ( *it )->extraBuffer ) );
                        }
                    }

                    delete *it;

                    ++it;
                }

                ++itor;
            }

            mPools.clear();
        }

        {
            ConstBufferPoolUserVec::const_iterator itor = mUsers.begin();
            ConstBufferPoolUserVec::const_iterator endt = mUsers.end();

            while( itor != endt )
            {
                ( *itor )->mAssignedSlot = 0;
                ( *itor )->mAssignedPool = 0;
                ( *itor )->mGlobalIndex = -1;
                ( *itor )->mDirtyFlags = DirtyNone;
                ++itor;
            }

            mUsers.clear();
        }
    }
    //-----------------------------------------------------------------------------------
    void ConstBufferPool::uploadDirtyDatablocks()
    {
        while( !mDirtyUsers.empty() )
        {
            // While inside ConstBufferPool::uploadToConstBuffer, the pool user may tag
            // itself dirty again, in which case we need to loop again. Move users
            // to a temporary array to avoid iterator invalidation from screwing us.
            mDirtyUsersTmp.swap( mDirtyUsers );
            uploadDirtyDatablocksImpl();
        }
    }
    //-----------------------------------------------------------------------------------
    void ConstBufferPool::uploadDirtyDatablocksImpl()
    {
        assert( !mDirtyUsersTmp.empty() );

        OgreProfileExhaustive( "ConstBufferPool::uploadDirtyDatablocks" );

        const size_t materialSizeInGpu = mBytesPerSlot;
        const size_t extraBufferSizeInGpu = mExtraBufferParams.bytesPerSlot;

        std::sort( mDirtyUsersTmp.begin(), mDirtyUsersTmp.end(),
                   OrderConstBufferPoolUserByPoolThenSlot );

        const size_t uploadSize = ( materialSizeInGpu + extraBufferSizeInGpu ) * mDirtyUsersTmp.size();
        StagingBuffer *stagingBuffer = _mVaoManager->getStagingBuffer( uploadSize, true );

        StagingBuffer::DestinationVec destinations;
        StagingBuffer::DestinationVec extraDestinations;

        destinations.reserve( mDirtyUsersTmp.size() );
        extraDestinations.reserve( mDirtyUsersTmp.size() );

        ConstBufferPoolUserVec::const_iterator itor = mDirtyUsersTmp.begin();
        ConstBufferPoolUserVec::const_iterator endt = mDirtyUsersTmp.end();

        char *bufferStart = reinterpret_cast<char *>( stagingBuffer->map( uploadSize ) );
        char *data = bufferStart;
        char *extraData = bufferStart + materialSizeInGpu * mDirtyUsersTmp.size();

        while( itor != endt )
        {
            const size_t srcOffset = static_cast<size_t>( data - bufferStart );
            const size_t dstOffset = ( *itor )->getAssignedSlot() * materialSizeInGpu;

            uint8 dirtyFlags = ( *itor )->mDirtyFlags;
            ( *itor )->mDirtyFlags = DirtyNone;
            ( *itor )->uploadToConstBuffer( data, dirtyFlags );
            data += materialSizeInGpu;

            const BufferPool *usersPool = ( *itor )->getAssignedPool();

            StagingBuffer::Destination dst( usersPool->materialBuffer, dstOffset, srcOffset,
                                            materialSizeInGpu );

            if( !destinations.empty() )
            {
                StagingBuffer::Destination &lastElement = destinations.back();

                if( lastElement.destination == dst.destination &&
                    ( lastElement.dstOffset + lastElement.length == dst.dstOffset ) )
                {
                    lastElement.length += dst.length;
                }
                else
                {
                    destinations.push_back( dst );
                }
            }
            else
            {
                destinations.push_back( dst );
            }

            if( usersPool->extraBuffer )
            {
                ( *itor )->uploadToExtraBuffer( extraData );

                const size_t extraSrcOffset = static_cast<size_t>( extraData - bufferStart );
                const size_t extraDstOffset = ( *itor )->getAssignedSlot() * extraBufferSizeInGpu;

                extraData += extraBufferSizeInGpu;

                StagingBuffer::Destination extraDst( usersPool->extraBuffer, extraDstOffset,
                                                     extraSrcOffset, extraBufferSizeInGpu );

                if( !extraDestinations.empty() )
                {
                    StagingBuffer::Destination &lastElement = extraDestinations.back();

                    if( lastElement.destination == dst.destination &&
                        ( lastElement.dstOffset + lastElement.length == dst.dstOffset ) )
                    {
                        lastElement.length += dst.length;
                    }
                    else
                    {
                        extraDestinations.push_back( extraDst );
                    }
                }
                else
                {
                    extraDestinations.push_back( extraDst );
                }
            }

            ++itor;
        }

        destinations.insert( destinations.end(), extraDestinations.begin(), extraDestinations.end() );

        stagingBuffer->unmap( destinations );
        stagingBuffer->removeReferenceCount();

        mDirtyUsersTmp.clear();
    }
    //-----------------------------------------------------------------------------------
    void ConstBufferPool::requestSlot( uint32 hash, ConstBufferPoolUser *user, bool wantsExtraBuffer )
    {
        uint8 oldDirtyFlags = 0;
        if( user->mAssignedPool )
        {
            oldDirtyFlags = user->mDirtyFlags;
            releaseSlot( user );
        }

        BufferPoolVecMap::iterator it = mPools.find( hash );

        if( it == mPools.end() )
        {
            mPools[hash] = BufferPoolVec();
            it = mPools.find( hash );
        }

        BufferPoolVec &bufferPool = it->second;

        BufferPoolVec::iterator itor = bufferPool.begin();
        BufferPoolVec::iterator endt = bufferPool.end();

        while( itor != endt && ( *itor )->freeSlots.empty() )
            ++itor;

        if( itor == endt )
        {
            ConstBufferPacked *materialBuffer =
                _mVaoManager->createConstBuffer( mBufferSize, BT_DEFAULT, 0, false );
            BufferPacked *extraBuffer = 0;

            if( mExtraBufferParams.bytesPerSlot && wantsExtraBuffer )
            {
                if( mExtraBufferParams.useReadOnlyBuffers )
                {
                    extraBuffer = _mVaoManager->createReadOnlyBuffer(
                        PFG_RGBA32_FLOAT, mExtraBufferParams.bytesPerSlot * mSlotsPerPool,
                        mExtraBufferParams.bufferType, 0, false );
                }
                else
                {
                    extraBuffer =
                        _mVaoManager->createConstBuffer( mExtraBufferParams.bytesPerSlot * mSlotsPerPool,
                                                         mExtraBufferParams.bufferType, 0, false );
                }
            }

            BufferPool *newPool = new BufferPool( hash, mSlotsPerPool, materialBuffer, extraBuffer );
            bufferPool.push_back( newPool );
            itor = bufferPool.end() - 1;
        }

        BufferPool *pool = *itor;
        user->mAssignedSlot = pool->freeSlots.back();
        user->mAssignedPool = pool;
        user->mGlobalIndex = static_cast<ptrdiff_t>( mUsers.size() );
        // user->mPoolOwner    = this;

        // If this assert triggers, you need to be consistent between your hashes
        // and the wantsExtraBuffer flag so that you land in the right pool.
        assert( ( pool->extraBuffer != 0 ) == wantsExtraBuffer &&
                "The pool was first requested with/without an extra buffer "
                "but now the opposite is being requested" );

        mUsers.push_back( user );

        pool->freeSlots.pop_back();

        scheduleForUpdate( user, DirtyConstBuffer | oldDirtyFlags );
    }
    //-----------------------------------------------------------------------------------
    void ConstBufferPool::releaseSlot( ConstBufferPoolUser *user )
    {
        BufferPool *pool = user->mAssignedPool;

        if( user->mDirtyFlags != DirtyNone )
        {
            ConstBufferPoolUserVec::iterator it =
                std::find( mDirtyUsers.begin(), mDirtyUsers.end(), user );

            if( it != mDirtyUsers.end() )
                efficientVectorRemove( mDirtyUsers, it );
        }

        assert( user->mAssignedSlot < mSlotsPerPool );
        assert( std::find( pool->freeSlots.begin(), pool->freeSlots.end(), user->mAssignedSlot ) ==
                pool->freeSlots.end() );

        pool->freeSlots.push_back( user->mAssignedSlot );
        user->mAssignedSlot = 0;
        user->mAssignedPool = 0;
        // user->mPoolOwner    = 0;
        user->mDirtyFlags = DirtyNone;

        assert( user->mGlobalIndex < static_cast<ptrdiff_t>( mUsers.size() ) &&
                user == *( mUsers.begin() + user->mGlobalIndex ) &&
                "mGlobalIndex out of date or argument doesn't belong to this pool manager" );
        ConstBufferPoolUserVec::iterator itor = mUsers.begin() + user->mGlobalIndex;
        itor = efficientVectorRemove( mUsers, itor );

        // The node that was at the end got swapped and has now a different index
        if( itor != mUsers.end() )
            ( *itor )->mGlobalIndex = itor - mUsers.begin();
    }
    //-----------------------------------------------------------------------------------
    void ConstBufferPool::scheduleForUpdate( ConstBufferPoolUser *dirtyUser, uint8 dirtyFlags )
    {
        assert( dirtyFlags != DirtyNone );

        if( dirtyUser->mDirtyFlags == DirtyNone )
            mDirtyUsers.push_back( dirtyUser );
        dirtyUser->mDirtyFlags |= dirtyFlags;
    }
    //-----------------------------------------------------------------------------------
    size_t ConstBufferPool::getPoolIndex( ConstBufferPoolUser *user ) const
    {
        BufferPool *pool = user->mAssignedPool;

        assert( user->mAssignedSlot < mSlotsPerPool );
        assert( std::find( pool->freeSlots.begin(), pool->freeSlots.end(), user->mAssignedSlot ) ==
                pool->freeSlots.end() );

        BufferPoolVecMap::const_iterator itor = mPools.find( pool->hash );
        assert( itor != mPools.end() && "Error or argument doesn't belong to this pool manager." );

        const BufferPoolVec &poolVec = itor->second;
        BufferPoolVec::const_iterator it = std::find( poolVec.begin(), poolVec.end(), pool );

        return static_cast<size_t>( it - poolVec.begin() );
    }
    //-----------------------------------------------------------------------------------
    struct OldUserRecord
    {
        ConstBufferPoolUser *user;
        uint32 hash;
        bool wantsExtraBuffer;
        OldUserRecord( ConstBufferPoolUser *_user, uint32 _hash, bool _wantsExtraBuffer ) :
            user( _user ),
            hash( _hash ),
            wantsExtraBuffer( _wantsExtraBuffer )
        {
        }
    };
    typedef std::vector<OldUserRecord> OldUserRecordVec;
    void ConstBufferPool::setOptimizationStrategy( OptimizationStrategy optimizationStrategy )
    {
        if( mOptimizationStrategy != optimizationStrategy )
        {
            mDirtyUsers.clear();

            OldUserRecordVec oldUserRecords;
            {
                // Save all the data we need before we destroy the pools.
                ConstBufferPoolUserVec::const_iterator itor = mUsers.begin();
                ConstBufferPoolUserVec::const_iterator endt = mUsers.end();

                while( itor != endt )
                {
                    OldUserRecord record( *itor, ( *itor )->mAssignedPool->hash,
                                          ( *itor )->mAssignedPool->extraBuffer != 0 );
                    oldUserRecords.push_back( record );
                    ++itor;
                }
            }

            destroyAllPools();
            mOptimizationStrategy = optimizationStrategy;
            if( _mVaoManager )
            {
                if( mOptimizationStrategy == LowerCpuOverhead )
                    mBufferSize = std::min<size_t>( _mVaoManager->getConstBufferMaxSize(), 64 * 1024 );
                else
                    mBufferSize = mBytesPerSlot;

                mSlotsPerPool = static_cast<uint32>( mBufferSize / mBytesPerSlot );
            }

            {
                // Recreate the pools.
                OldUserRecordVec::const_iterator itor = oldUserRecords.begin();
                OldUserRecordVec::const_iterator endt = oldUserRecords.end();

                while( itor != endt )
                {
                    this->requestSlot( itor->hash, itor->user, itor->wantsExtraBuffer );
                    itor->user->notifyOptimizationStrategyChanged();
                    ++itor;
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    ConstBufferPool::OptimizationStrategy ConstBufferPool::getOptimizationStrategy() const
    {
        return mOptimizationStrategy;
    }
    //-----------------------------------------------------------------------------------
    void ConstBufferPool::_changeRenderSystem( RenderSystem *newRs )
    {
        if( _mVaoManager )
        {
            destroyAllPools();
            mDirtyUsers.clear();
            _mVaoManager = 0;
        }

        if( newRs )
        {
            _mVaoManager = newRs->getVaoManager();

            if( mOptimizationStrategy == LowerCpuOverhead )
                mBufferSize = std::min<size_t>( _mVaoManager->getConstBufferMaxSize(), 64 * 1024 );
            else
                mBufferSize = mBytesPerSlot;

            mSlotsPerPool = static_cast<uint32>( mBufferSize / mBytesPerSlot );
        }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    ConstBufferPool::BufferPool::BufferPool( uint32 _hash, uint32 slotsPerPool,
                                             ConstBufferPacked *_materialBuffer,
                                             BufferPacked *_extraBuffer ) :
        hash( _hash ),
        materialBuffer( _materialBuffer ),
        extraBuffer( _extraBuffer )
    {
        freeSlots.reserve( slotsPerPool );
        for( uint32 i = 0; i < slotsPerPool; ++i )
            freeSlots.push_back( ( slotsPerPool - i ) - 1 );
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    ConstBufferPool::ExtraBufferParams::ExtraBufferParams( size_t _bytesPerSlot, BufferType _bufferType,
                                                           bool _useReadOnlyBuffers ) :
        bytesPerSlot( _bytesPerSlot ),
        bufferType( _bufferType ),
        useReadOnlyBuffers( _useReadOnlyBuffers )
    {
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    ConstBufferPoolUser::ConstBufferPoolUser() :
        mAssignedSlot( 0 ),
        mAssignedPool( 0 ),
        // mPoolOwner( 0 ),
        mGlobalIndex( -1 ),
        mDirtyFlags( ConstBufferPool::DirtyNone )
    {
    }
}  // namespace Ogre
