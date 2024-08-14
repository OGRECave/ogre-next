
#ifndef Demo_Hlms05MyHlmsPbs_H
#define Demo_Hlms05MyHlmsPbs_H

#include "OgreHlmsListener.h"
#include "OgreHlmsPbs.h"

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    class SharedHelperForHlms
    {
    protected:
        typedef vector<Ogre::ConstBufferPacked *>::type ConstBufferPackedVec;

        /// Vector of buffers holding per-object data.
        /// When one runs out, we push a new one. On the next frame
        /// we reuse them all from 0.
        ConstBufferPackedVec mPerObjectDataBuffers;

        /// The buffer currently use. Can be nullptr. It is contained in mPerObjectDataBuffers.
        ConstBufferPacked *ogre_nullable mCurrPerObjectDataBuffer;

        /// The last content of mCurrentConstBuffer. If it changes
        /// we need a new mCurrPerObjectDataBuffer too (because drawId will
        /// be reset from 0).
        ConstBufferPacked *ogre_nullable mLastMainConstBuffer;

        /// The mapped contents of currPerObjectDataBuffer.
        float *ogre_nullable mCurrPerObjectDataPtr;

        /// Pointer to Ogre's VAO manager. Used here for destroying const buffers.
        VaoManager *ogre_nullable mOwnVaoManager;

        /** Binds currPerObjectDataBuffer to the right slot. Does nothing if it's nullptr.
        @param commandBuffer
            Cmd buffer to bind to.
        @param perObjectDataBufferSlot
            Slot to bind the buffer to.
        */
        void bindObjectDataBuffer( CommandBuffer *commandBuffer, uint16_t perObjectDataBufferSlot );

        /**
        @param instanceIdx
            The index of the instance to write to.
        @param commandBuffer
            Command buffer to bind our new buffer if we create one.
        @param vaoManager
            VaoManager to create new ConstBufferPacked.
        @param constBuffers
            Reference to mConstBuffers so we can tell if we need to bind a new const buffer.
        @param currConstBufferIdx
            Value of mCurrentConstBuffer so we can tell if we need to bind a new const buffer.
        @param startMappedConstBuffer
            Value of mStartMappedConstBuffer for validation (to ensure our implementation isn't out
            of sync with OgreNext's).
        @param perObjectDataBufferSlot
            See mapConstBuffer().
        @return
        */
        float *mapObjectDataBufferFor( uint32_t instanceIdx, Ogre::CommandBuffer *commandBuffer,
                                       Ogre::VaoManager *vaoManager,
                                       const ConstBufferPackedVec &constBuffers,
                                       uint32_t currConstBufferIdx, uint32_t *startMappedConstBuffer,
                                       uint16_t perObjectDataBufferSlot );

        /// Unmaps the current buffer holding per-object data from memory.
        void unmapObjectDataBuffer();

        SharedHelperForHlms();
        ~SharedHelperForHlms();
    };

    class MyHlmsPbs final : public HlmsPbs, public HlmsListener, SharedHelperForHlms
    {
    public:
        /// This value is completely arbitrary. It just doesn't have to clash with anything else.
        /// We use it to tag which SubItems/SubEntities should have wind animation.
        static constexpr uint32 kColourId = 7895211u;

        MyHlmsPbs( Archive *dataFolder, ArchiveVec *libraryFolders ) :
            HlmsPbs( dataFolder, libraryFolders )
        {
            // Set ourselves as our own listener.
            setListener( this );
        }

        void hlmsTypeChanged( bool casterPass, CommandBuffer *commandBuffer,
                              const HlmsDatablock *datablock, size_t texUnit ) override;

        void calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces ) override;

        using HlmsPbs::setupRootLayout;
        void setupRootLayout( RootLayout &rootLayout, const HlmsPropertyVec &properties,
                              size_t tid ) const override;

        uint32 fillBuffersForV1( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                 bool casterPass, uint32 lastCacheHash,
                                 CommandBuffer *commandBuffer ) override;

        uint32 fillBuffersForV2( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                 bool casterPass, uint32 lastCacheHash,
                                 CommandBuffer *commandBuffer ) override;

        void preCommandBufferExecution( CommandBuffer *commandBuffer ) override;
        void frameEnded() override;
    };

    OGRE_ASSUME_NONNULL_END
}  // namespace Ogre

#endif
