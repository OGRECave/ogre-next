
#include "Tutorial_Hlms05_MyHlmsPbs.h"

#include "CommandBuffer/OgreCbShaderBuffer.h"
#include "CommandBuffer/OgreCommandBuffer.h"
#include "OgreRenderQueue.h"
#include "OgreRootLayout.h"
#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreVaoManager.h"

using namespace Ogre;

/// The slot where to bind mCurrPerObjectDataBuffer
/// HlmsPbs might consume slot 3, so we always use slot 4 for simplicity.
static const uint16 kPerObjectDataBufferSlot = 4u;

SharedHelperForHlms::SharedHelperForHlms() :
    mCurrPerObjectDataBuffer( 0 ),
    mLastMainConstBuffer( 0 ),
    mCurrPerObjectDataPtr( 0 ),
    mOwnVaoManager( 0 )
{
}
//-----------------------------------------------------------------------------
SharedHelperForHlms::~SharedHelperForHlms()
{
    if( !mOwnVaoManager )
        return;

    for( ConstBufferPacked *buffer : mPerObjectDataBuffers )
        mOwnVaoManager->destroyConstBuffer( buffer );

    mPerObjectDataBuffers.clear();
}
//-----------------------------------------------------------------------------
void SharedHelperForHlms::bindObjectDataBuffer( CommandBuffer *commandBuffer,
                                                uint16_t perObjectDataBufferSlot )
{
    if( mCurrPerObjectDataBuffer )
    {
        *commandBuffer->addCommand<Ogre::CbShaderBuffer>() = Ogre::CbShaderBuffer(
            Ogre::VertexShader, perObjectDataBufferSlot, mCurrPerObjectDataBuffer, 0u,
            static_cast<uint32_t>( mCurrPerObjectDataBuffer->getTotalSizeBytes() ) );
    }
}
//-----------------------------------------------------------------------------
float *SharedHelperForHlms::mapObjectDataBufferFor( uint32_t instanceIdx, CommandBuffer *commandBuffer,
                                                    VaoManager *vaoManager,
                                                    const ConstBufferPackedVec &constBuffers,
                                                    uint32_t currConstBufferIdx,
                                                    uint32_t *startMappedConstBuffer,
                                                    uint16_t perObjectDataBufferSlot )
{
    const uint32_t numFloatsPerObject = 4u;

    if( !mCurrPerObjectDataBuffer || mLastMainConstBuffer != constBuffers[currConstBufferIdx] )
    {
        // mConstBuffers[mCurrentConstBuffer] changed, which means
        // gl_InstanceId / drawId will be reset to 0. We must create a new
        // buffer and bind that one

        unmapObjectDataBuffer();

        ConstBufferPacked *constBuffer = nullptr;
        if( currConstBufferIdx >= mPerObjectDataBuffers.size() )
        {
            mOwnVaoManager = vaoManager;
            const size_t bufferSize = std::min<size_t>( 65536, vaoManager->getConstBufferMaxSize() );
            constBuffer =
                vaoManager->createConstBuffer( bufferSize, Ogre::BT_DYNAMIC_PERSISTENT, nullptr, false );
            mPerObjectDataBuffers.push_back( constBuffer );
        }
        else
        {
            constBuffer = mPerObjectDataBuffers[currConstBufferIdx];
        }

        mCurrPerObjectDataBuffer = constBuffer;
        mCurrPerObjectDataPtr =
            reinterpret_cast<float *>( constBuffer->map( 0u, constBuffer->getNumElements() ) );

        OGRE_ASSERT_LOW( currConstBufferIdx <= constBuffers.size() &&
                         startMappedConstBuffer != nullptr &&
                         "This should not happen. Base class must've bound something" );

        mLastMainConstBuffer = constBuffers[currConstBufferIdx];

        bindObjectDataBuffer( commandBuffer, perObjectDataBufferSlot );
    }

    const size_t offset = instanceIdx * numFloatsPerObject;

    // This assert triggering either means:
    //  - This class got modified and we're packing more data into
    //    currPerObjectDataBuffer, so it must be bigger.
    //    (use a TexBufferPacked if we're past limits).
    //  - There is a bug and currPerObjectDataBuffer got out of sync
    //    with mCurrentConstBuffer.
    OGRE_ASSERT_LOW( ( offset + numFloatsPerObject ) * sizeof( float ) <=
                         mCurrPerObjectDataBuffer->getTotalSizeBytes() &&
                     "Out of bounds!" );

    return mCurrPerObjectDataPtr + offset;
}
//-----------------------------------------------------------------------------
void SharedHelperForHlms::unmapObjectDataBuffer()
{
    if( mCurrPerObjectDataBuffer )
    {
        mCurrPerObjectDataBuffer->unmap( UO_KEEP_PERSISTENT, 0u,
                                         mCurrPerObjectDataBuffer->getNumElements() );
        mCurrPerObjectDataPtr = 0;
        mCurrPerObjectDataBuffer = 0;
        mLastMainConstBuffer = 0;
    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void MyHlmsPbs::calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces )
{
    HlmsPbs::calculateHashForPreCreate( renderable, inOutPieces );

    // Check if the object has been tagged to have custom colour. If it does, then set
    // the respective property so the shader gets generated with the respective code.
    if( renderable->hasCustomParameter( kColourId ) )
    {
        setProperty( kNoTid, "use_arbitrary_colour", 1 );
        setProperty( kNoTid, "MyPerObjectDataSlot", kPerObjectDataBufferSlot );
    }
}
//-----------------------------------------------------------------------------
void MyHlmsPbs::hlmsTypeChanged( bool casterPass, CommandBuffer *commandBuffer,
                                 const HlmsDatablock *datablock, size_t texUnit )
{
    if( !casterPass )
        bindObjectDataBuffer( commandBuffer, kPerObjectDataBufferSlot );
}
//-----------------------------------------------------------------------------
void MyHlmsPbs::setupRootLayout( RootLayout &rootLayout, const HlmsPropertyVec &properties,
                                 size_t tid ) const
{
    if( this->getProperty( properties, "use_arbitrary_colour" ) != 0 )
    {
        // Account for the extra buffer bound at kPerObjectDataBufferSlot
        // It should be the last buffer to be set, so kPerObjectDataBufferSlot + 1
        rootLayout.mDescBindingRanges[0][DescBindingTypes::ConstBuffer].end =
            kPerObjectDataBufferSlot + 1u;
    }
}
//-----------------------------------------------------------------------------
uint32 MyHlmsPbs::fillBuffersForV1( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                    bool casterPass, uint32 lastCacheHash, CommandBuffer *commandBuffer )
{
    const uint32 instanceIdx =
        HlmsPbs::fillBuffersForV1( cache, queuedRenderable, casterPass, lastCacheHash, commandBuffer );

    if( !casterPass )
    {
        const Renderable::CustomParameterMap &customParams =
            queuedRenderable.renderable->getCustomParameters();

        // We always have to fill this buffer even for renderables that don't have it at all.
        // The shader will not even know that this buffer or data exists, thus GPU performance
        // won't be impacted for objects that don't use it. But the CPU impact is unavoidable.
        //
        // This is why OgreNext generally avoids per-object arbitrary data: It can be expensive.
        Renderable::CustomParameterMap::const_iterator itor = customParams.find( kColourId );
        Vector4 customParam;
        if( itor != customParams.end() )
            customParam = itor->second;
        else
            customParam = Vector4( 1, 1, 1, 1 );

        float *dataPtr = mapObjectDataBufferFor( instanceIdx, commandBuffer, mVaoManager, mConstBuffers,
                                                 this->mCurrentConstBuffer, mStartMappedConstBuffer,
                                                 kPerObjectDataBufferSlot );
        dataPtr[0] = customParam.x;
        dataPtr[1] = customParam.y;
        dataPtr[2] = customParam.z;
        dataPtr[3] = customParam.w;
    }
    return instanceIdx;
}

//-----------------------------------------------------------------------------
uint32 MyHlmsPbs::fillBuffersForV2( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                    bool casterPass, uint32 lastCacheHash, CommandBuffer *commandBuffer )
{
    const uint32 instanceIdx =
        HlmsPbs::fillBuffersForV2( cache, queuedRenderable, casterPass, lastCacheHash, commandBuffer );

    if( !casterPass )
    {
        const Renderable::CustomParameterMap &customParams =
            queuedRenderable.renderable->getCustomParameters();

        // We always have to fill this buffer even for renderables that don't have it at all.
        // The shader will not even know that this buffer or data exists, thus GPU performance
        // won't be impacted for objects that don't use it. But the CPU impact is unavoidable.
        //
        // This is why OgreNext generally avoids per-object arbitrary data: It can be expensive.
        Renderable::CustomParameterMap::const_iterator itor = customParams.find( kColourId );
        Vector4 customParam;
        if( itor != customParams.end() )
            customParam = itor->second;
        else
            customParam = Vector4( 1, 1, 1, 1 );

        float *dataPtr = mapObjectDataBufferFor( instanceIdx, commandBuffer, mVaoManager, mConstBuffers,
                                                 this->mCurrentConstBuffer, mStartMappedConstBuffer,
                                                 kPerObjectDataBufferSlot );
        dataPtr[0] = customParam.x;
        dataPtr[1] = customParam.y;
        dataPtr[2] = customParam.z;
        dataPtr[3] = customParam.w;
    }

    return instanceIdx;
}
//-----------------------------------------------------------------------------
void MyHlmsPbs::preCommandBufferExecution( CommandBuffer *commandBuffer )
{
    unmapObjectDataBuffer();
    HlmsPbs::preCommandBufferExecution( commandBuffer );
}
//-----------------------------------------------------------------------------
void MyHlmsPbs::frameEnded()
{
    HlmsPbs::frameEnded();

    mCurrPerObjectDataBuffer = nullptr;
    mLastMainConstBuffer = nullptr;
    mCurrPerObjectDataPtr = nullptr;
}
