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

#ifndef _OgreTextureGpuManager_H_
#define _OgreTextureGpuManager_H_

#include "OgrePrerequisites.h"
#include "OgreTextureGpu.h"
#include "OgreImage2.h"
#include "Threading/OgreLightweightMutex.h"
#include "Threading/OgreWaitableEvent.h"
#include "Threading/OgreThreads.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    typedef vector<TextureGpu*>::type TextureGpuVec;
    class ObjCmdBuffer;
    class ResourceLoadingListener;
    class TextureGpuManagerListener;

    namespace TextureFilter
    {
        class FilterBase;
    }

    typedef FastArray<TextureFilter::FilterBase*> FilterBaseArray;

    struct _OgreExport TexturePool
    {
        TextureGpu  *masterTexture;
        uint16                  usedMemory;
        vector<uint16>::type    availableSlots;
        TextureGpuVec           usedSlots;

        bool hasFreeSlot(void) const;
        bool empty(void) const;
    };

    typedef list<TexturePool>::type TexturePoolList;

    typedef vector<StagingTexture*>::type StagingTextureVec;

    namespace DefaultMipmapGen
    {
        enum DefaultMipmapGen
        {
            /// Do not generate mipmaps when TextureFilter::TypeGenerateDefaultMipmaps is used
            NoMipmaps,
            /// Generate mipmaps via HW when TextureFilter::TypeGenerateDefaultMipmaps is used
            HwMode,
            /// Generate mipmaps via SW when TextureFilter::TypeGenerateDefaultMipmaps is used
            SwMode
        };
    }

    namespace CommonTextureTypes
    {
        enum CommonTextureTypes
        {
            Diffuse,
            NormalMap,
            Monochrome,
            EnvMap,
            NonColourData
        };
    }

    class _OgreExport TextureGpuManager : public ResourceAlloc
    {
    public:
        /// Specifies the minimum squared resolution & number of slices to keep around
        /// all the for time StagingTextures. So:
        /// PixelFormatGpu format = PixelFormatGpuUtils::getFamily(PFG_RGBA8_UNORM);
        /// BudgetEntry( format, 4096u, 2u );
        /// Will keep around 128MB of staging texture, with a resolution of 4096x4096 x 2 slices.
        struct BudgetEntry
        {
            PixelFormatGpu formatFamily;
            uint32 minResolution;
            uint32 minNumSlices;
            BudgetEntry() : formatFamily( PFG_UNKNOWN ), minResolution( 0 ), minNumSlices( 0 ) {}
            BudgetEntry( PixelFormatGpu _formatFamily, uint32 _minResolution, uint32 _minNumSlices ) :
                formatFamily( _formatFamily ), minResolution( _minResolution ),
                minNumSlices( _minNumSlices ) {}

            bool operator () ( const BudgetEntry &_l, const BudgetEntry &_r ) const;
        };

        typedef vector<BudgetEntry>::type BudgetEntryVec;

    protected:
        struct ResourceEntry
        {
            String      name;
            String      alias;
            String      resourceGroup;
            TextureGpu  *texture;
            uint32      filters;

            ResourceEntry() : texture( 0 ) {}
            ResourceEntry( const String &_name, const String &_alias, const String &_resourceGroup,
                           TextureGpu *_texture, uint32 _filters ) :
                name( _name ), alias( _alias ), resourceGroup( _resourceGroup ),
                texture( _texture ), filters( _filters )
            {
            }
        };
        typedef map<IdString, ResourceEntry>::type ResourceEntryMap;

        struct LoadRequest
        {
            String                      name;
            Archive                     *archive;
            ResourceLoadingListener     *loadingListener;
            TextureGpu                  *texture;
            GpuResidency::GpuResidency  nextResidency;
            /// Slice to upload this image to (in case we're uploading a 2D image into an cubemap)
            /// std::numeric_limits<uint32>::max() to disable it.
            uint32                      sliceOrDepth;
            uint32                      filters;

            LoadRequest( const String &_name, Archive *_archive,
                         ResourceLoadingListener *_loadingListener,
                         TextureGpu *_texture, GpuResidency::GpuResidency _nextResidency,
                         uint32 _sliceOrDepth, uint32 _filters ) :
                name( _name ), archive( _archive ), loadingListener( _loadingListener ),
                texture( _texture ), nextResidency( _nextResidency ),
                sliceOrDepth( _sliceOrDepth ), filters( _filters ) {}
        };

        typedef vector<LoadRequest>::type LoadRequestVec;

        struct UsageStats
        {
            uint32 width;
            uint32 height;
            PixelFormatGpu formatFamily;
            size_t accumSizeBytes;
            /// This value gets reset in worker thread every time UsageStats have
            /// been updated for this formatFamily. Main thread will decrease it
            /// with every fulfillUsageStats call. When it reaches 0, the
            /// stat will be removed.
            uint32 loopCount;
            UsageStats( uint32 _width, uint32 _height, uint32 _depthOrSlices,
                        PixelFormatGpu _formatFamily );
        };
        typedef vector<UsageStats>::type UsageStatsVec;

        struct QueuedImage
        {
            Image2      image;
            uint64      mipLevelBitSet[4];
            TextureGpu  *dstTexture;
            uint8       numSlices;
            /// See LoadRequest::sliceOrDepth
            uint32      dstSliceOrDepth;
            FilterBaseArray filters;

            QueuedImage( Image2 &srcImage, uint8 numMips, uint8 _numSlices, TextureGpu *_dstTexture,
                         uint32 _dstSliceOrDepth, FilterBaseArray &inOutFilters );
            void destroy(void);
            bool empty(void) const;
            bool isMipSliceQueued( uint8 mipLevel, uint8 slice ) const;
            void unqueueMipSlice( uint8 mipLevel, uint8 slice );
            uint8 getMinMipLevel(void) const;
            uint8 getMaxMipLevelPlusOne(void) const;
        };

        typedef vector<QueuedImage>::type QueuedImageVec;

        struct ThreadData
        {
            LoadRequestVec  loadRequests;
            ObjCmdBuffer    *objCmdBuffer;
            StagingTextureVec usedStagingTex;
        };
        struct StreamingData
        {
            StagingTextureVec   availableStagingTex;    /// Used by both threads. Needs mutex protection.
            QueuedImageVec      queuedImages;           /// Used by mostly by worker thread. Needs mutex.
            UsageStatsVec       usageStats; /// Exclusively used by worker thread. No protection needed.
            UsageStatsVec       prevStats;              /// Used by both threads.
            /// Number of bytes preloaded by worker thread. Main thread resets this counter.
            size_t              bytesPreloaded;
        };

        DefaultMipmapGen::DefaultMipmapGen mDefaultMipmapGen;
        DefaultMipmapGen::DefaultMipmapGen mDefaultMipmapGenCubemaps;
        bool                mShuttingDown;
        ThreadHandlePtr     mWorkerThread;
        /// Main thread wakes, worker waits.
        WaitableEvent       mWorkerWaitableEvent;
        /// Worker wakes, main thread waits. Used by waitForStreamingCompletion();
        WaitableEvent       mRequestToMainThreadEvent;
        LightweightMutex    mLoadRequestsMutex;
        LightweightMutex    mMutex;
        ThreadData          mThreadData[2];
        StreamingData       mStreamingData;

        TexturePoolList     mTexturePool;
        ResourceEntryMap    mEntries;

        size_t              mEntriesToProcessPerIteration;
        size_t              mMaxPreloadBytes;
        /// See BudgetEntry. Must be sorted by size in bytes (biggest entries first).
        BudgetEntryVec              mBudget;
        TextureGpuManagerListener   *mTextureGpuManagerListener;
        size_t                      mStagingTextureMaxBudgetBytes;

        StagingTextureVec   mUsedStagingTextures;
        StagingTextureVec   mAvailableStagingTextures;

        StagingTextureVec   mTmpAvailableStagingTex;

        typedef vector<AsyncTextureTicket*>::type AsyncTextureTicketVec;
        AsyncTextureTicketVec   mAsyncTextureTickets;

        VaoManager          *mVaoManager;
        RenderSystem        *mRenderSystem;

        void destroyAll(void);
        void destroyAllStagingBuffers(void);
        void destroyAllTextures(void);
        void destroyAllPools(void);

        virtual TextureGpu* createTextureImpl( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                               IdString name, uint32 textureFlags,
                                               TextureTypes::TextureTypes initialType ) = 0;
        virtual StagingTexture* createStagingTextureImpl( uint32 width, uint32 height, uint32 depth,
                                                          uint32 slices, PixelFormatGpu pixelFormat )=0;
        virtual void destroyStagingTextureImpl( StagingTexture *stagingTexture ) = 0;

        virtual AsyncTextureTicket* createAsyncTextureTicketImpl( uint32 width, uint32 height,
                                                                  uint32 depthOrSlices,
                                                                  TextureTypes::TextureTypes textureType,
                                                                  PixelFormatGpu pixelFormatFamily ) = 0;

        /// Fills mTmpAvailableStagingTex with new StagingTextures that support formats &
        /// resolutions the worker thread couldn't upload because it lacked a compatible one.
        /// Assumes we're protected by mMutex! Called from main thread.
        void fulfillUsageStats(void);
        /// Fills mTmpAvailableStagingTex with new StagingTextures if there's not enough
        /// in there to meet our minimum budget as set in mBudget.
        /// Assumes we're protected by mMutex! Called from main thread.
        void fulfillMinimumBudget(void);

        /// See fulfillMinimumBudget and fulfillUsageStats
        /// Assumes we're protected by mMutex! Called from main thread.
        void fullfillBudget(void);

        /// Must be called from worker thread.
        void mergeUsageStatsIntoPrevStats(void);

        /** Finds a StagingTexture that can map the given region defined by the box & pixelFormat.
            Searches in both used & available textures.
            If no staging texture supports this request, it will fill a RareRequest entry.
        @remarks
            Assumes workerData is protected by a mutex.
        @param workerData
            Worker thread data.
        @param box
        @param pixelFormat
        @param outStagingTexture
            StagingTexture that mapped the return value. Unmodified if we couldn't map.
        @return
            The mapped region. If TextureBox::data is null, it couldn't be mapped.
        */
        static TextureBox getStreaming( ThreadData &workerData, StreamingData &streamingData,
                                        const TextureBox &box, PixelFormatGpu pixelFormat,
                                        StagingTexture **outStagingTexture );
        static void processQueuedImage( QueuedImage &queuedImage, ThreadData &workerData,
                                        StreamingData &streamingData );

        /// Retrieves, in bytes, the memory consumed by StagingTextures in a container like
        /// mAvailableStagingTextures, which are textures waiting either to be reused, or to be destroyed.
        size_t getConsumedMemoryByStagingTextures( const StagingTextureVec &stagingTextures ) const;
        /** Checks if we've exceeded our memory budget for available staging textures.
                * If we haven't, it early outs and returns nullptr.
                * If we have, we'll start stalling the GPU until we find a StagingTexture that
                  can fit the requirements, and return that pointer.
                * If we couldn't find a fit, we start removing StagingTextures until we've
                  freed enough to stay below the budget; and return a nullptr.
        */
        StagingTexture* checkStagingTextureLimits( uint32 width, uint32 height,
                                                   uint32 depth, uint32 slices,
                                                   PixelFormatGpu pixelFormat,
                                                   size_t minConsumptionRatioThreshold );

    public:
        TextureGpuManager( VaoManager *vaoManager, RenderSystem *renderSystem );
        virtual ~TextureGpuManager();

        /** Whether to use HW or SW mipmap generation when specifying
            TextureFilter::TypeGenerateDefaultMipmaps for loading files from textures.
            This setting has no effect for filters explicitly asking for HW mipmap generation.
        @param hwMipmapGen
            Whether to enable HW mipmap generation for textures. Default is true.
        @param hwMipmapGenCubemaps
            Whether to enable HW mipmap generation for cubemap textures. Default is false.
        */
        void setDefaultMipmapGeneration( DefaultMipmapGen::DefaultMipmapGen defaultMipmapGen,
                                         DefaultMipmapGen::DefaultMipmapGen defaultMipmapGenCubemaps );
        DefaultMipmapGen::DefaultMipmapGen getDefaultMipmapGeneration(void) const;
        DefaultMipmapGen::DefaultMipmapGen getDefaultMipmapGenerationCubemaps(void) const;

        /// Must be called from main thread.
        void _reserveSlotForTexture( TextureGpu *texture );
        /// Must be called from main thread.
        void _releaseSlotFromTexture( TextureGpu *texture );

        unsigned long _updateStreamingWorkerThread( ThreadHandle *threadHandle );
        void _updateStreaming(void);

        /** Returns true if there is no more streaming work to be done yet
            (if false, calls to _update could be needed once again)
            See waitForStreamingCompletion.
        @param syncWithWorkerThread
            When true, we will wait for the worker thread to release the main mutex
            instead of just continuing and trying again next time we get called.
            This is important for waitForStreamingCompletion & _waitFor because
            otherwise main thread may not see worker thread has finished because
            it's also grabbing the main mutex; and waitForStreamingCompletion
            will go to sleep thinking worker thread has yet to finish, and
            worker thread won't wake up the main thread because it has
            already notified it.
        */
        bool _update( bool syncWithWorkerThread );

        /// Blocks main thread until are pending textures are fully loaded.
        void waitForStreamingCompletion(void);

        /// Do not use directly. See TextureGpu::waitForMetadata & TextureGpu::waitForDataReady
        void _waitFor( TextureGpu *texture, bool metadataOnly );

        /// Reserves and preallocates a pool with the given parameters
        void reservePoolId( uint32 poolId, uint32 width, uint32 height,
                            uint32 numSlices, uint8 numMipmaps, PixelFormatGpu pixelFormat );

        bool hasPoolId( uint32 poolId, uint32 width, uint32 height,
                        uint8 numMipmaps, PixelFormatGpu pixelFormat ) const;

        /**
        @param name
            Name of the resource. For example TreeWood.png
        @param aliasName
            Usually aliasName = name. An alias name allows you to load the same texture
            (e.g. TreeWood.png) with different settings.
            For example:
                Alias 0 - "Tree Wood With Mipmaps"
                Alias 1 - "Tree Wood Without Mipmaps"
                Alias 2 - "Tree Wood Without TextureFlags::AutomaticBatching"
            This lets you have 3 copies of the same file in memory.
        @param pageOutStrategy
        @param textureFlags
            See TextureFlags::TextureFlags
        @param initialType
            Strictly not required (i.e. can be left TextureTypes::Unknown) however it
            can be needed if set to a material before it is fully loaded; and the
            shader expects a particular type (e.g. it expects a cubemap).
            While it's not yet loaded, a dummy texture will that matches the type will
            be used; and it's important that the right dummy texture is selected.
            So if you know in advance a particular type is needed, this parameter
            tells Ogre what dummy to use.
        @param resourceGroup
            Optional, but required if you want to load files from disk
            (or anything provided by the ResourceGroupManager)
        @param poolId
            Optional. See TextureGpu::setTexturePoolId
            This parameter informs which pool ID you wish the texture to be assigned for.
            Note however, if you're using createOrRetrieveTexture and the texture has already been
            created (i.e. it's being retrieved) then the pool ID parameter will be ignored,
            as the texture was already created with a pool ID.
        @return
        */
        TextureGpu* createTexture( const String &name,
                                   const String &aliasName,
                                   GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                   uint32 textureFlags, TextureTypes::TextureTypes initialType,
                                   const String &resourceGroup=BLANKSTRING,
                                   uint32 filters=0,
                                   uint32 poolId=0 );
        TextureGpu* createTexture( const String &name,
                                   GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                   uint32 textureFlags, TextureTypes::TextureTypes initialType,
                                   const String &resourceGroup=BLANKSTRING,
                                   uint32 filters=0, uint32 poolId=0 );
        TextureGpu* createOrRetrieveTexture( const String &name,
                                             const String &aliasName,
                                             GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                             uint32 textureFlags,
                                             TextureTypes::TextureTypes initialType,
                                             const String &resourceGroup=BLANKSTRING,
                                             uint32 filters=0,
                                             uint32 poolId=0 );
        TextureGpu* createOrRetrieveTexture( const String &name,
                                             GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                             uint32 textureFlags,
                                             TextureTypes::TextureTypes initialType,
                                             const String &resourceGroup=BLANKSTRING,
                                             uint32 filters=0, uint32 poolId=0 );
        /// Helper function to call createOrRetrieveTexture with common
        /// parameters used for 2D diffuse textures loaded from file.
        TextureGpu* createOrRetrieveTexture( const String &name,
                                             GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                             CommonTextureTypes::CommonTextureTypes type,
                                             const String &resourceGroup=BLANKSTRING,
                                             uint32 poolId=0 );
        TextureGpu* findTextureNoThrow( IdString name ) const;
        void destroyTexture( TextureGpu *texture );

        /** Creates a StagingTexture which is required to upload data CPU -> GPU into
            a TextureGpu.
            To download data GPU -> CPU see readRequest
        @remarks
            We try to find the smallest available texture (won't stall) that can fit the request.
        @param minConsumptionRatioThreshold
            Value in range [0; 100].
            The smallest available texture we find may still be too big (e.g. you need to upload
            64x64 texture RGBA8 and we return a 8192x8192x4 staging texture which is overkill).
            For these cases, here you can specify how much "is too big". For example by
            specifying a consumptionRatio of 50; it means that the data you asked for
            must occupy at least 50% of the space; otherwise we'll create a new StagingTexture.

            A value of 100 means the StagingTexture must fit exactly (fully used).
            A value of 0 means any StagingTexture will do, no matter how large.

            StagingTextures that haven't been using in a while will be destroyed. However
            if for some reason we end up returning a huge texture every frame for small
            workloads, we'll be keeping that waste potentially forever.
        @return
            StagingTexture that meets the criteria. When you're done, remove it by calling
            removeStagingTexture.
        */
        StagingTexture* getStagingTexture( uint32 width, uint32 height, uint32 depth,
                                           uint32 slices, PixelFormatGpu pixelFormat,
                                           size_t minConsumptionRatioThreshold=25u );
        void removeStagingTexture( StagingTexture *stagingTexture );

        /** Creates an AsyncTextureTicket that can be used to download data GPU -> CPU
            from a TextureGpu.
            To upload data CPU -> GPU see getStagingTexture
        @param width
        @param height
        @param depthOrSlices
        @param pixelFormatFamily
            If the value is not a family value, it will automatically be converted to one.
        @return
        */
        AsyncTextureTicket* createAsyncTextureTicket( uint32 width, uint32 height, uint32 depthOrSlices,
                                                      TextureTypes::TextureTypes textureType,
                                                      PixelFormatGpu pixelFormatFamily );
        void destroyAsyncTextureTicket( AsyncTextureTicket *ticket );
        void destroyAllAsyncTextureTicket(void);

        void saveTexture( TextureGpu *texture,
                          const String &folderPath, set<String>::type &savedTextures,
                          bool saveOitd, bool saveOriginal,
                          HlmsTextureExportListener *listener );

        void dumpStats(void) const;
        void dumpMemoryUsage( Log* log ) const;

        /// Sets a new listener. The old one will be destroyed with OGRE_DELETE
        /// See TextureGpuManagerListener. Pointer cannot be null.
        void setTextureGpuManagerListener( TextureGpuManagerListener *listener );

        /** Background streaming works by having a bunch of preallocated StagingTextures so
            we're ready to start uploading as soon as we see a request to load a texture
            from file.
        @par
            If there is no minimum budget or it is too small for the texture you're trying
            to load, background threads can't start as soon as possible and has to wait
            until the next call to _update (or to waitForStreamingCompletion).
            This controls how much memory we reserve.
        @remarks
            Be careful on reserving too much memory, or else Out of Memory situations
            could arise. The amount of memory you can reserved is limited by the
            GTT (Graphics Translation Table) and the limit may be much lower than the
            total System RAM. For example my 16GB RAM system with a 2GB GPU, the GTT
            limit on Linux is of 3GB (use radeontop to find this information).
            See https://en.wikipedia.org/wiki/Graphics_address_remapping_table
        @param budget
            Array of parameters for the staging textures we'll reserve.
            The budget can be empty.
        */
        void setWorkerThreadMinimumBudget( const BudgetEntryVec &budget );

        /** At a high level, texture loading works like this:
            1. Grab a free StagingTexture from "available" pool in main thread
            2. Load image from file in secondary thread and fill the StagingTexture
            3. Copy from StagingTexture to final TextureGpu in main thread
            4. Release the StagingTexture so it goes back to "available" pool.
            5. Repeat from step 1 for next batch of images to load.
        @par
            All is well except for one little detail in steps 1 and 4:
            The StagingTexture released at step 4 can't become immediately available
            as the GPU could still be performing the copy from step 3, so we must wait
            a few frames until it's safe to map it again.
        @par
            That means at step 1, there may be StagingTextures in the "available" pool,
            yet however none of them are actually ready to grab; so we create a new
            one instead.
        @par
            In other words, if the CPU produces textures faster than the GPU can consume
            them, we may keep creating more and more StagingTextures until we run out of memory.
        @par
            That's where this function comes in. This function limits how much we let
            the "available" pool grow. If the threshold is exceeded, instead of creating
            a new StagingTexture at step 1; we'll begin to stall and wait for the GPU
            to catch up; so we can reuse these StagingTextures again. If no StagingTexture
            is capable of performing the upload (e.g. they're of incompatible format)
            we'll start deleting StagingTextures to make room for the one we need.
            The details are explained in checkStagingTextureLimits.
        @par
            This limit is tightly respected by Ogre but not a hard one. For example
            if you set the limit on 256MB and we require a StagingTexture of 326MB
            to load a very, very big texture, then Ogre has no other choice but to
            delete all textures in mAvailableStagingTextures and create one of 326MB
            that can perform the operation; but Ogre won't error out because 326 > 256MB.
            (though in such scenario the process may run out of memory and crash)
        @remarks
            This limit only counts for textures that are in zero-referenced in
            mAvailableStagingTextures. For example if you've set the limit in 256MB and
            you've created 1GB worth of StagingTextures (i.e. via getStagingTexture) and
            never released them via removeStagingTexture; those textures don't count.
            We only check the limit against the released textures in mAvailableStagingTextures.
        @param stagingTextureMaxBudgetBytes
            Limit in bytes, on how much memory we let in mAvailableStagingTextures before
            we start stalling the GPU and/or aggressively destroying them.
        */
        void setStagingTextureMaxBudgetBytes( size_t stagingTextureMaxBudgetBytes );

        /** The worker thread first loads the texture from disk to RAM (aka "preload",
            and then copies from RAM to StagingTexture.
            Later the main thread will copy from StagingTexture to actual texture.
        @par
            This value controls how many bytes are preloaded (i.e. from disk to RAM)
            by the worker thread until the next _update call from the main thread is issued.
        @par
            Higher values allows worker thread to keep loading textures while your main
            thread loads the rest of the scene. Lower values prevent Out of Memory conditions.
        @remarks
            Due to how the code works, this value will also affect how much StagingTexture
            we ask to the main thread (because preloading becomes a bottleneck).
        @par
            Testing shows that very high values (i.e. >256MB) have the potential of uncovering
            driver bugs (even in 64-bit builds) and thus are not recommended.
        @par
            The value is an approximation and not a hard limit. e.g. if loading a 128MB
            cubemap and the limit is 1 byte; then we'll preload 128MBs. But we won't
            be loading anything else.
            Also due to how the code works, there is some broad granularity issues that
            can cause us to consume a bit more.
        @param maxPreloadBytes
        */
        void setWorkerThreadMaxPreloadBytes( size_t maxPreloadBytes );

        const String* findAliasNameStr( IdString idName ) const;
        const String* findResourceNameStr( IdString idName ) const;
        const String* findResourceGroupStr( IdString idName ) const;

        RenderSystem* getRenderSystem(void) const;

    protected:
        void scheduleLoadRequest( TextureGpu *texture, GpuResidency::GpuResidency nextResidency,
                                  const String &name, const String &resourceGroup,
                                  uint32 filters,
                                  uint32 sliceOrDepth=std::numeric_limits<uint32>::max() );
    public:
        void _scheduleTransitionTo( TextureGpu *texture, GpuResidency::GpuResidency nextResidency );
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
