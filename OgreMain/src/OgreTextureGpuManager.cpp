/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2017 Torus Knot Software Ltd

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

#include "OgreTextureGpuManager.h"
#include "OgreTextureGpu.h"
#include "OgreStagingTexture.h"

#include "OgreId.h"
#include "OgreLwString.h"
#include "OgreCommon.h"

#include "Vao/OgreVaoManager.h"

#include "OgreException.h"

#define TODO_grow_pool 1

namespace Ogre
{
    static const int c_mainThread = 0;
    static const int c_workerThread = 1;

    TextureGpuManager::TextureGpuManager( VaoManager *vaoManager ) :
        mVaoManager( vaoManager )
    {
        //64MB default
        mDefaultPoolParameters.maxBytesPerPool = 64 * 1024 * 1024;
        mDefaultPoolParameters.minSlicesPerPool[0] = 16;
        mDefaultPoolParameters.minSlicesPerPool[1] = 8;
        mDefaultPoolParameters.minSlicesPerPool[2] = 6;
        mDefaultPoolParameters.minSlicesPerPool[3] = 2;
        mDefaultPoolParameters.maxResolutionToApplyMinSlices[0] = 256;
        mDefaultPoolParameters.maxResolutionToApplyMinSlices[1] = 512;
        mDefaultPoolParameters.maxResolutionToApplyMinSlices[2] = 1024;
        mDefaultPoolParameters.maxResolutionToApplyMinSlices[3] = 4096;
    }
    //-----------------------------------------------------------------------------------
    TextureGpuManager::~TextureGpuManager()
    {
        assert( mAvailableStagingTextures.empty() && "Derived class didn't call destroyAll!" );
        assert( mUsedStagingTextures.empty() && "Derived class didn't call destroyAll!" );
        assert( mEntries.empty() && "Derived class didn't call destroyAll!" );
        assert( mTexturePool.empty() && "Derived class didn't call destroyAll!" );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyAll(void)
    {
        destroyAllStagingBuffers();
        destroyAllTextures();
        destroyAllPools();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyAllStagingBuffers(void)
    {
        StagingTextureVec::iterator itor = mAvailableStagingTextures.begin();
        StagingTextureVec::iterator end  = mAvailableStagingTextures.end();

        while( itor != end )
        {
            destroyStagingTextureImpl( *itor );
            delete *itor;
            ++itor;
        }

        mAvailableStagingTextures.clear();

        itor = mUsedStagingTextures.begin();
        end  = mUsedStagingTextures.end();

        while( itor != end )
        {
            destroyStagingTextureImpl( *itor );
            delete *itor;
            ++itor;
        }

        mUsedStagingTextures.clear();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyAllTextures(void)
    {
        ResourceEntryMap::const_iterator itor = mEntries.begin();
        ResourceEntryMap::const_iterator end  = mEntries.end();

        while( itor != end )
        {
            const ResourceEntry &entry = itor->second;
            delete entry.texture;
            ++itor;
        }

        mEntries.clear();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyAllPools(void)
    {
        TexturePoolList::const_iterator itor = mTexturePool.begin();
        TexturePoolList::const_iterator end  = mTexturePool.end();

        while( itor != end )
        {
            delete itor->masterTexture;
            ++itor;
        }

        mTexturePool.clear();
    }
    //-----------------------------------------------------------------------------------
    uint16 TextureGpuManager::getNumSlicesFor( TextureGpu *texture ) const
    {
        const PoolParameters &poolParams = mDefaultPoolParameters;

        uint32 maxResolution = std::max( texture->getWidth(), texture->getHeight() );
        uint16 minSlicesPerPool = 1u;

        for( int i=0; i<4; ++i )
        {
            if( maxResolution <= poolParams.maxResolutionToApplyMinSlices[i] )
            {
                minSlicesPerPool = poolParams.minSlicesPerPool[i];
                break;
            }
        }

        return minSlicesPerPool;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* TextureGpuManager::createTexture( const String &name,
                                                  GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                                  uint32 textureFlags )
    {
        IdString idName( name );

        if( mEntries.find( idName ) != mEntries.end() )
        {
            OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                         "A texture with name '" + name + "' already exists.",
                         "TextureGpuManager::createTexture" );
        }

        TextureGpu *retVal = createTextureImpl( pageOutStrategy, idName, textureFlags );

        mEntries[idName] = ResourceEntry( name, retVal );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyTexture( TextureGpu *texture )
    {
        ResourceEntryMap::iterator itor = mEntries.find( texture->getName() );

        if( itor == mEntries.end() )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Texture with name '" + texture->getName().getFriendlyText() +
                         "' not found. Perhaps already destroyed?",
                         "TextureGpuManager::destroyTexture" );
        }

        delete texture;
        mEntries.erase( itor );
    }
    //-----------------------------------------------------------------------------------
    StagingTexture* TextureGpuManager::getStagingTexture( uint32 width, uint32 height,
                                                          uint32 depth, uint32 slices,
                                                          PixelFormatGpu pixelFormat )
    {
        StagingTexture *retVal = 0;
        StagingTextureVec::iterator itor = mAvailableStagingTextures.begin();
        StagingTextureVec::iterator end  = mAvailableStagingTextures.end();

        while( itor != end && !retVal )
        {
            StagingTexture *stagingTexture = *itor;

            if( stagingTexture->supportsFormat( width, height, depth, slices, pixelFormat ) )
            {
                if( !stagingTexture->uploadWillStall() )
                {
                    retVal = stagingTexture;
                    mUsedStagingTextures.push_back( stagingTexture );
                    mAvailableStagingTextures.erase( itor );
                }
            }

            ++itor;
        }

        if( !retVal )
        {
            //Couldn't find an existing StagingTexture that could handle our request. Create one.
            retVal = createStagingTextureImpl( width, height, depth, slices, pixelFormat );
            mUsedStagingTextures.push_back( retVal );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::removeStagingTexture( StagingTexture *stagingTexture )
    {
        //Reverse search to speed up since most removals are
        //likely to remove what has just been requested.
        StagingTextureVec::reverse_iterator ritor = std::find( mUsedStagingTextures.rbegin(),
                                                               mUsedStagingTextures.rend(),
                                                               stagingTexture );
        assert( ritor != mUsedStagingTextures.rend() &&
                "StagingTexture does not belong to this TextureGpuManager or already removed" );

        StagingTextureVec::iterator itor = ritor.base() - 1u;
        efficientVectorRemove( mUsedStagingTextures, itor );

        mAvailableStagingTextures.push_back( stagingTexture );
    }
    //-----------------------------------------------------------------------------------
    const String* TextureGpuManager::findNameStr( IdString idName ) const
    {
        const String *retVal = 0;

        ResourceEntryMap::const_iterator itor = mEntries.find( idName );

        if( itor != mEntries.end() )
            retVal = &itor->second.name;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::_reserveSlotForTexture( TextureGpu *texture )
    {
        bool matchFound = false;

        TexturePoolList::iterator itor = mTexturePool.begin();
        TexturePoolList::iterator end  = mTexturePool.end();

        while( itor != end && !matchFound )
        {
            const TexturePool &pool = *itor;

            matchFound =
                    pool.hasFreeSlot() &&
                    pool.masterTexture->getWidth() == texture->getWidth() &&
                    pool.masterTexture->getHeight() == texture->getHeight() &&
                    pool.masterTexture->getDepthOrSlices() == texture->getDepthOrSlices() &&
                    pool.masterTexture->getPixelFormat() == texture->getPixelFormat() &&
                    pool.masterTexture->getNumMipmaps() == texture->getNumMipmaps();

            TODO_grow_pool;

            ++itor;
        }

        bool queueToMainThread = false;

        if( itor == end )
        {
            IdType newId = Id::generateNewId<TextureGpuManager>();
            char tmpBuffer[64];
            LwString texName( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );
            texName.a( "_InternalTex", newId );

            TexturePool newPool;
            newPool.masterTexture = createTextureImpl( GpuPageOutStrategy::Discard,
                                                       texName.c_str(), TextureFlags::Texture );
            const uint16 numSlices = getNumSlicesFor( texture );

            newPool.usedMemory = 0;
            newPool.usedSlots.reserve( numSlices );

            newPool.masterTexture->setTextureType( TextureTypes::Type2DArray );
            newPool.masterTexture->setResolution( texture->getWidth(), texture->getHeight(), numSlices );
            newPool.masterTexture->setPixelFormat( texture->getPixelFormat() );
            newPool.masterTexture->setNumMipmaps( texture->getNumMipmaps() );

            mTexturePool.push_back( newPool );
            itor = --mTexturePool.end();

            //itor->masterTexture->transitionTo( GpuResidency::Resident, 0 );
            queueToMainThread = true;
        }

        uint16 sliceIdx = 0;
        //See if we can reuse a slot that was previously acquired and released
        if( !itor->availableSlots.empty() )
        {
            sliceIdx = itor->availableSlots.back();
            itor->availableSlots.pop_back();
        }
        else
        {
            sliceIdx = itor->usedMemory++;
        }
        itor->usedSlots.push_back( texture );
        texture->_notifyTextureSlotChanged( &(*itor), sliceIdx );

        //Must happen after _notifyTextureSlotChanged to avoid race condition.
        if( queueToMainThread )
        {
            mMutex.lock();
                mPoolsPending[c_workerThread].push_back( &(*itor) );
            mMutex.unlock();
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::_releaseSlotFromTexture( TextureGpu *texture )
    {
        //const_cast? Yes. We own it. We could do a linear search to mTexturePool;
        //but it's O(N) vs O(1); and O(N) can quickly turn into O(N!).
        TexturePool *texturePool = const_cast<TexturePool*>( texture->getTexturePool() );
        TextureGpuVec::iterator itor = std::find( texturePool->usedSlots.begin(),
                                                  texturePool->usedSlots.end(), texture );
        assert( itor != texturePool->usedSlots.end() );
        efficientVectorRemove( texturePool->usedSlots, itor );

        const uint16 internalSliceStart = texture->getInternalSliceStart();
        if( texturePool->usedMemory == internalSliceStart + 1u )
            --texturePool->usedMemory;
        else
            texturePool->availableSlots.push_back( internalSliceStart );

        texture->_notifyTextureSlotChanged( 0, 0 );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::update(void)
    {
        {
            mMutex.lock();
                mPoolsPending[c_mainThread].swap( mPoolsPending[c_workerThread] );
            mMutex.unlock();
        }

        {
            TexturePoolVec::const_iterator itor = mPoolsPending[c_mainThread].begin();
            TexturePoolVec::const_iterator end  = mPoolsPending[c_mainThread].end();

            while( itor != end )
            {
                TexturePool *pool = *itor;
                pool->masterTexture->transitionTo( GpuResidency::Resident, 0 );
                TextureGpuVec::const_iterator itTex = pool->usedSlots.begin();
                TextureGpuVec::const_iterator enTex = pool->usedSlots.end();

                while( itTex != enTex )
                {
                    (*itTex)->_notifyTextureSlotChanged( pool, (*itTex)->getInternalSliceStart() );
                    ++itTex;
                }

                ++itor;
            }

            mPoolsPending[c_mainThread].clear();
        }

        {
            StagingTextureVec::iterator itor = mAvailableStagingTextures.begin();
            StagingTextureVec::iterator end  = mAvailableStagingTextures.end();

            const uint32 numFramesThreshold = mVaoManager->getDynamicBufferMultiplier() + 2u;

            //They're kept in order.
            while( itor != end &&
                   (*itor)->getLastFrameUsed() - mVaoManager->getFrameCount() > numFramesThreshold )
            {
                destroyStagingTextureImpl( *itor );
                delete *itor;
                ++itor;
            }

            mAvailableStagingTextures.erase( mAvailableStagingTextures.begin(), itor );
        }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    bool TexturePool::hasFreeSlot(void) const
    {
        return !availableSlots.empty() || usedMemory < masterTexture->getNumSlices();
    }
}
