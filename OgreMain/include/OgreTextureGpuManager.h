/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#include "OgreBitset.h"
#include "OgreImage2.h"
#include "OgreTextureGpu.h"
#include "OgreTextureGpuListener.h"
#include "Threading/OgreLightweightMutex.h"
#include "Threading/OgreSemaphore.h"
#include "Threading/OgreThreads.h"
#include "Threading/OgreWaitableEvent.h"

#include "ogrestd/list.h"
#include "ogrestd/map.h"
#include "ogrestd/set.h"

#include <atomic>

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */

    typedef vector<TextureGpu *>::type TextureGpuVec;
    class ObjCmdBuffer;
    class ResourceLoadingListener;
    class TextureGpuManagerListener;

    namespace TextureFilter
    {
        class FilterBase;
    }

    typedef FastArray<TextureFilter::FilterBase *> FilterBaseArray;

    struct _OgreExport TexturePool
    {
        TextureGpu          *masterTexture;
        bool                 manuallyReserved;
        uint16               usedMemory;
        vector<uint16>::type availableSlots;
        TextureGpuVec        usedSlots;

        bool hasFreeSlot() const;
        bool empty() const;
    };

    typedef list<TexturePool>::type TexturePoolList;

    typedef vector<StagingTexture *>::type StagingTextureVec;

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
            NonColourData,
            NumCommonTextureTypes
        };
    }

    namespace ResidencyMask
    {
        enum ResidencyMask
        {
            OnStorage = 1 << GpuResidency::OnStorage,
            OnSystemRam = 1 << GpuResidency::OnSystemRam,
            Resident = 1 << GpuResidency::Resident,
            All = OnStorage | OnSystemRam | Resident
        };
    }

    /**
    @class TextureGpuManager
        This class manages all textures (i.e. TextureGpu) since Ogre 2.2

        Explanation of the streaming model:

        TextureGpuManager uses a worker thread to load textures in the background.
        There are several restrictions the implementation needs to account for:
            + D3D11 does not support persistent mapping. This means we must call unmap
              on a StagingTexture before we can copy it to the final texture.
            + Calling map/unmap from multiple threads is nearly impossible in OpenGL.
              This means map/unmap calls must happen in the main thread.
            + ResourceGroupManager is not thread-friendly (building with thread
              support fills ResourceGroupManager with huge fat mutexes)
            + Most APIs allow using a simple buffer to store all sorts of staging
              data (regardless of format and resolution), but D3D11 is very inflexible
              about this, requiring StagingTextures to have a 2D resolution (rather
              than being a 1D buffer with just bytes), and must match the same
              format.

        Because of these restrictions, TextureGpuManager::scheduleLoadRequest will
        grab the Archive to load from file from ResourceGroupManager and then
        create a request for the worker thread.

        The worker thread (_updateStreamingWorkerThread) runs an infinite loop
        waiting for new requests.

        The worker thread will process incoming LoadRequest: it will open the file
        and retrieve the important information first aka the metadata (such as
        resolution, pixel format, number of mipmaps, etc). It is most likely
        the worker thread will load the whole image from disk, and not just
        the metadata (at least the first slice + first mip).

        The image is at the moment in RAM, but two things need to happen:
            1. The image must be copied to a StagingTexture so that it can
               be later be copied to the final texture.
            2. The texture must become Resident so that our API commands
               can actually work (like copying from StagingTexture to the
               final texture object)

        The worker thread will now push an entry to mStreamingData.queuedImages
        for all the slices & mips that are pending to copy from RAM to a
        StagingTexture. That includes slice 0 mip 0.

        It will also emit a command the main thread will eventually see to
        make the texture Resident (via ObjCmdBuffer::TransitionToResident)

        The worker thread will call TextureGpuManager::getStreaming to grab an
        available StagingTexture that has been pre-mapped in the main thread that
        can hold the data we want to upload.

        If no such Texture is available, we cannot upload the texture yet and this
        failure is recorded; and we do not remove the entry from mStreamingData.queuedImages
        Until mStreamingData.queuedImages[i].empty() returns true, the worker thread,
        with each new iteration, will try again to grab a StagingTexture to finish the jobs.

        From the main thread, with each TextureGpuManager::_update; fullfillBudget will
        check the recorded failures in getStreaming from the worker thread and create & map
        more StagingTextures to satisfy the worker thread.

        The main thread will also execute the transition to resident commands sent from
        the worker thread.

        This means that in worst case scenario, uploading a texture may require multiple
        ping pongs between the worker & main thread, which is why
        TextureGpuManager::waitForStreamingCompletion keeps calling _update in a loop
        until the texture is fully loaded.

        To put things into perspective: if getStreaming always failed to grab a
        StagingTexture (i.e. due to a bug), then waitForStreamingCompletion would
        be stuck in an infinite loop.

        Note that this behavior applies to both multithreaded and singlethreaded versions
        (i.e. when OGRE_FORCE_TEXTURE_STREAMING_ON_MAIN_THREAD is defined).

        When everything's done, queuedImage.empty() returns true, a
        ObjCmdBuffer::NotifyDataIsReady command to the main thread is issued,
        and the entry is removed from mStreamingData.queuedImages

        Of course, the main thread tries to predict how much StagingTexture will
        be needed by tracking past usage and by trying to fullfill mBudget
        (see TextureGpuManager::setWorkerThreadMinimumBudget), so under best case
        scenario the ping pong between worker & main thread is kept to a minimum.

        TextureGpuManager::mMaxPreloadBytes puts an upper limit to this prediction
        to prevent memory from sky-rocketing (or reaching out of memory conditions)
        during spikes. There's only so much GART/GTT memory available in a system.

        The variable mEntriesToProcessPerIteration controls how many entries in
        LoadRequests are processed per iteration in the worker thread.
        High values cause the worker thread to appear "stuck" loading lots of
        images without flushing our internal command buffer, which means
        the main thread won't see what's happening and thus unable to
        fullfull budget failure requests, predict accurately, and unable
        to perform transition to resident requests. I.e. the main thread
        will sit idle, until it suddenly sees a lot of work coming from
        the worker thread.
        Very low values can cause more threading contention due to excessive
        flushing.

        The metadata cache helps with performance by being able to know in the
        main thread before creating the LoadRequest what texture pool to reserve.
        But performance will be degraded if the metadata cache lied, as we must
        then perform multiple ping pongs between the threads to correct the error.

        @see    TextureGpu

    @remarks
        Even if you use waitForStreamingCompletion to have all of your frames
        rendering "perfectly" (i.e. prefer stalling rendering instead of showing
        a replacement texture until the real one is loaded), multithreaded
        streaming can still reduce your loading times because Ogre will
        immediately start loading the texture from disk while the main
        thread can keep executing your code (like moving on to the next Item
        or Datablock you're instantiating)
    */
    class _OgreExport TextureGpuManager : public OgreAllocatedObj, public TextureGpuListener
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
            uint32         minResolution;
            uint32         minNumSlices;
            BudgetEntry() : formatFamily( PFG_UNKNOWN ), minResolution( 0 ), minNumSlices( 0 ) {}
            BudgetEntry( PixelFormatGpu _formatFamily, uint32 _minResolution, uint32 _minNumSlices ) :
                formatFamily( _formatFamily ),
                minResolution( _minResolution ),
                minNumSlices( _minNumSlices )
            {
            }

            bool operator()( const BudgetEntry &_l, const BudgetEntry &_r ) const;
        };

        typedef vector<BudgetEntry>::type BudgetEntryVec;

        struct MetadataCacheEntry
        {
            String                     aliasName;
            uint32                     width;
            uint32                     height;
            uint32                     depthOrSlices;
            PixelFormatGpu             pixelFormat;
            uint32                     poolId;
            TextureTypes::TextureTypes textureType;
            uint8                      numMipmaps;
            MetadataCacheEntry();
        };

        typedef map<IdString, MetadataCacheEntry>::type MetadataCacheMap;

        struct ResourceEntry
        {
            String      name;
            String      alias;
            String      resourceGroup;
            TextureGpu *texture;
            uint32      filters;
            bool        destroyRequested;

            ResourceEntry() : texture( 0 ) {}
            ResourceEntry( const String &_name, const String &_alias, const String &_resourceGroup,
                           TextureGpu *_texture, uint32 _filters ) :
                name( _name ),
                alias( _alias ),
                resourceGroup( _resourceGroup ),
                texture( _texture ),
                filters( _filters ),
                destroyRequested( false )
            {
            }
        };
        typedef map<IdString, ResourceEntry>::type ResourceEntryMap;

    protected:
        struct LoadRequest
        {
            String                   name;
            Archive                 *archive;
            ResourceLoadingListener *loadingListener;
            Image2                  *image;
            TextureGpu              *texture;
            /// Slice to upload this image to (in case we're uploading a 2D image into an cubemap)
            /// std::numeric_limits<uint32>::max() to disable it.
            uint32 sliceOrDepth;
            uint32 filters;
            /// Whether we should call "delete this->image" once we're done using the image.
            /// Otherwise you should listen for TextureGpuListener::ReadyForRendering
            /// message to know when we're done using the image.
            bool autoDeleteImage;
            /// Indicates we're going to GpuResidency::OnSystemRam instead of Resident
            bool toSysRam;

            LoadRequest( const String &_name, Archive *_archive,
                         ResourceLoadingListener *_loadingListener, Image2 *_image, TextureGpu *_texture,
                         uint32 _sliceOrDepth, uint32 _filters, bool _autoDeleteImage, bool _toSysRam ) :
                name( _name ),
                archive( _archive ),
                loadingListener( _loadingListener ),
                image( _image ),
                texture( _texture ),
                sliceOrDepth( _sliceOrDepth ),
                filters( _filters ),
                autoDeleteImage( _autoDeleteImage ),
                toSysRam( _toSysRam )
            {
            }
        };

        typedef vector<LoadRequest>::type LoadRequestVec;

        struct UsageStats
        {
            uint32         width;
            uint32         height;
            PixelFormatGpu formatFamily;
            size_t         accumSizeBytes;
            /// This value gets reset in worker thread every time UsageStats have
            /// been updated for this formatFamily. Main thread will decrease it
            /// with every fulfillUsageStats call. When it reaches 0, the
            /// stat will be removed.
            uint32 loopCount;
            UsageStats( uint32 _width, uint32 _height, uint32 _depthOrSlices,
                        PixelFormatGpu _formatFamily );
        };
        typedef vector<UsageStats>::type UsageStatsVec;

        /**
        @class QueuedImage
            When loading a texture (i.e. TextureGpuManager::scheduleLoadRequest or
            TextureGpuManager::scheduleLoadFromRam were called) the following will happen:

                1. Find if the texture has any pending transition or finishing loading
                    1a. If so place a task in mScheduledTasks
                    1b. Otherwise execute the task immediately
                2. The texture is placed from main thread in mThreadData[].loadRequests
                3. Worker thread parses that request. Assuming all goes smoothly (e.g. Image loaded
                   successfully, metadata cache was not out of date, etc) the Image is fully
                   loaded from disk into memory as an Image2.
                4. The texture is transitioned to Resident* since all metadata is available now,
                   but not yet ready for rendering. This step cannot happen if we must maintain a
                   SystemRam copy (i.e. strategy is AlwaysKeepSystemRamCopy; or we were asked
                   to load to OnSystemRam, instead of Resident)
                6. A QueuedImage is created.
                7. The LoadRequest is destroyed.
                8. Worker thread loops again, processing QueuedImages and LoadRequests

            QueuedImage will proceed to transfer from system RAM (step 3) to StagingTextures, and
            will remain alive until all of its data has been transfered. It will also be
            responsible for requesting more StagingTextures if needed.

            Once the QueuedImage is done, it will signal a ObjCmdBuffer::NotifyDataIsReady.

            If step 4 could not happen, then the QueuedImage will also perform the transition
            to resident* when we're done.

            (*) Transitions happen in main thread, however they're requested from worker thread
            via ObjCmdBuffer::TransitionToLoaded. See TextureGpuManager::addTransitionToLoadedCmd code

        @remarks
            QueuedImage class is used when is NEVER used when transitioning from OnStorage -> OnSystemRam

            There's a case in which the texture must be loaded in parts and thus more than one
            QueuedImage will be needed. See TextureGpuManager::PartialImage.

            In this case, then NotifyDataIsReady (and TransitionToLoaded if step 4 could not run)
            can only happen after all pending QueuedImage are over.
        */
        struct QueuedImage
        {
            Image2      image;
            bitset64    mipLevelBitSet;
            TextureGpu *dstTexture;
            bool        autoDeleteImage;
            /// See LoadRequest::sliceOrDepth
            uint32          dstSliceOrDepth;
            FilterBaseArray filters;
#ifdef OGRE_PROFILING_TEXTURES
            uint64 microsecondsTaken;
#endif

            QueuedImage( Image2 &srcImage, TextureGpu *_dstTexture, uint32 _dstSliceOrDepth,
                         FilterBaseArray &inOutFilters
#ifdef OGRE_PROFILING_TEXTURES
                         ,
                         uint64 _microsecondsTaken
#endif
            );
            void  destroy();
            bool  empty() const;
            bool  isMipSliceQueued( uint8 mipLevel, uint8 slice ) const;
            void  unqueueMipSlice( uint8 mipLevel, uint8 slice );
            uint8 getMinMipLevel() const;
            uint8 getMaxMipLevelPlusOne() const;
        };

        typedef vector<QueuedImage>::type QueuedImageVec;

        /**
        @class PartialImage
            In certain cases, more than one QueuedImage is needed because the texture is being
            loaded from multiple textures (i.e. cubemaps made from 6 different textures)

            This structure tracks how many faces have been loaded, so we know when it's done.
        */
        struct PartialImage
        {
            void  *sysRamPtr;
            uint32 numProcessedDepthOrSlices;
            bool   toSysRam;

            PartialImage();
            PartialImage( void *_sysRamPtr, bool _toSysRam );
        };

        typedef map<TextureGpu *, PartialImage>::type PartialImageMap;

        struct ThreadData
        {
            LoadRequestVec    loadRequests;
            ObjCmdBuffer     *objCmdBuffer;
            StagingTextureVec usedStagingTex;
        };
        struct StreamingData
        {
            StagingTextureVec availableStagingTex;  ///< Used by both threads. Needs mutex protection.
            QueuedImageVec    queuedImages;         ///< Used by mostly by worker thread. Needs mutex.
            UsageStatsVec     usageStats;  ///< Exclusively used by worker thread. No protection needed.
            UsageStatsVec     prevStats;   ///< Used by both threads.
            /// Set to true when worker thread iterates (meaning prevStats.loopCount
            /// can be decremented).
            /// Set to false by main thread every _update call.
            /// Needs mutex.
            bool workerThreadRan;
            /// Number of bytes preloaded by worker thread. Main thread resets this counter.
            size_t bytesPreloaded;
            /// See setWorkerThreadMinimumBudget
            /// Read by worker thread. Occasionally written by main thread. Not protected.
            uint32 maxSplitResolution;
            /// See setWorkerThreadMaxPerStagingTextureRequestBytes
            /// Read by worker thread. Occasionally written by main thread. Not protected.
            size_t maxPerStagingTextureRequestBytes;

            /// Resheduled textures are textures which were transitioned to Resident
            /// preemptively using the metadata cache, but it turned out to be wrong
            /// (out of date), so we need to do some ping pong first
            ///
            /// Used by worker thread. No protection needed.
            set<TextureGpu *>::type rescheduledTextures;

            /// Only used for textures that need more than one Image to load
            ///
            /// Used by worker thread. No protection needed (except in abortAllRequests).
            ///
            /// @see    TextureGpuManager::PartialImage
            PartialImageMap partialImages;
        };

        enum TasksType
        {
            TaskTypeResidencyTransition,
            TaskTypeDestroyTexture
        };

        struct ResidencyTransitionTask
        {
            GpuResidency::GpuResidency targetResidency;
            Image2                    *image;
            bool                       autoDeleteImage;
            bool                       reuploadOnly;
            bool                       bSkipMultiload;

            void init( GpuResidency::GpuResidency _targetResidency, Image2 *_image,
                       bool _autoDeleteImage, bool _reuploadOnly, bool _bSkipMultiload )
            {
                targetResidency = _targetResidency;
                image = _image;
                autoDeleteImage = _autoDeleteImage;
                reuploadOnly = _reuploadOnly;
                bSkipMultiload = _bSkipMultiload;
            }
        };

        struct ScheduledTasks
        {
            TasksType tasksType;
            union
            {
                ResidencyTransitionTask residencyTransitionTask;
            };

            ScheduledTasks();
        };

        typedef vector<ScheduledTasks>::type               ScheduledTasksVec;
        typedef map<TextureGpu *, ScheduledTasksVec>::type ScheduledTasksMap;

        DefaultMipmapGen::DefaultMipmapGen mDefaultMipmapGen;
        DefaultMipmapGen::DefaultMipmapGen mDefaultMipmapGenCubemaps;
        bool                               mAllowMemoryLess;
        bool                               mShuttingDown;
        std::atomic<bool>                  mUseMultiload;
        ThreadHandlePtr                    mWorkerThread;
        /// Main thread wakes, worker waits.
        WaitableEvent mWorkerWaitableEvent;
        /// Worker wakes, main thread waits. Used by waitForStreamingCompletion();
        WaitableEvent    mRequestToMainThreadEvent;
        LightweightMutex mLoadRequestsMutex;
        LightweightMutex mMutex;
        // Counts how many times mMutex.tryLock returned false in a row
        uint32        mTryLockMutexFailureCount;
        uint32        mTryLockMutexFailureLimit;
        uint64        mLoadRequestsCounter;
        bool          mLastUpdateIsStreamingDone;
        bool          mAddedNewLoadRequests;
        ThreadData    mThreadData[2];
        StreamingData mStreamingData;

        /// Threadpool for loading many textures in parallel. See setMultiLoadPool()
        std::vector<ThreadHandlePtr> mMultiLoadWorkerThreads;
        LoadRequestVec               mMultiLoads;
        LightweightMutex             mMultiLoadsMutex;
        Semaphore                    mMultiLoadsSemaphore;
        std::atomic<uint32>          mPendingMultiLoads;

        TexturePoolList  mTexturePool;
        ResourceEntryMap mEntries;
        /// Protects mEntries
        mutable LightweightMutex mEntriesMutex;

        size_t mEntriesToProcessPerIteration;
        size_t mMaxPreloadBytes;
        /// See BudgetEntry. Must be sorted by size in bytes (biggest entries first).
        BudgetEntryVec             mBudget;
        TextureGpuManagerListener *mTextureGpuManagerListener;
        size_t                     mStagingTextureMaxBudgetBytes;

        StagingTextureVec mUsedStagingTextures;
        StagingTextureVec mAvailableStagingTextures;

        StagingTextureVec mTmpAvailableStagingTex;

        MetadataCacheMap mMetadataCache;

        typedef vector<AsyncTextureTicket *>::type AsyncTextureTicketVec;
        AsyncTextureTicketVec                      mAsyncTextureTickets;

        struct DownloadToRamEntry
        {
            TextureGpu *texture;
            /// One per mip. Entries with nullptr means the ticket has already been copied and destroyed
            AsyncTextureTicketVec asyncTickets;
            uint8                *sysRamPtr;
            /// When resyncOnly == true, the download is from textures currently Resident that
            /// will remain Resident. Otherwise a Resident -> OnSystemRam transition will occur
            bool resyncOnly;
        };
        typedef vector<DownloadToRamEntry>::type DownloadToRamEntryVec;

        struct MissedListenerCall
        {
            TextureGpu                *texture;
            TextureGpuListener::Reason reason;
            MissedListenerCall( TextureGpu *_texture, TextureGpuListener::Reason _reason ) :
                texture( _texture ),
                reason( _reason )
            {
            }
        };
        typedef list<MissedListenerCall>::type MissedListenerCallList;

        ScheduledTasksMap      mScheduledTasks;
        DownloadToRamEntryVec  mDownloadToRamQueue;
        MissedListenerCallList mMissedListenerCalls;
        MissedListenerCallList mMissedListenerCallsTmp;
        bool                   mDelayListenerCalls;
        bool                   mIgnoreScheduledTasks;
#ifdef OGRE_PROFILING_TEXTURES
        bool mProfilingLoadingTime;
#endif

    public:
        /** While true, calls to createTexture & createOrRetrieveTexture will ignore
            and unset the TextureFlags::PrefersLoadingFromFileAsSRGB flag.

            This is useful if user is not doing PBR, or working in its own
            colour space manually handled.

            Default value is false

        @remarks
            PUBLIC VARIABLE. This variable can be altered directly.
            Changes are reflected immediately.

            Changes will be reflected on new textures. Existing textures no
            longer possess the information to know whether they were created
            w/ PrefersLoadingFromFileAsSRGB

            This value is not read nor write from the worker thread,
            thus it is thread-safe.

            Textures may still be loaded as SRGB if they explicitly request
            SRGB e.g. `texture->setPixelFormat( PFG_RGBA8_UNORM_SRGB )` was
            called or the texture is loaded from an OITD or DDS format
            specifically asking for PFG_RGBA8_UNORM_SRGB.

            What this flag controls is that if we're loading a regular texture
            asking for PFG_RGBA8_UNORM like PNG (and other linear formats) then
            should we honour PrefersLoadingFromFileAsSRGB flag or not.
        */
        bool mIgnoreSRgbPreference;

    protected:
        VaoManager   *mVaoManager;
        RenderSystem *mRenderSystem;

#ifdef OGRE_PROFILING_TEXTURES
        std::map<IdString, Timer> mProfilingData;
#endif

        // Be able to hold up to a 2x2 cubemap RGBA8 for when a
        // image raises an exception in the worker thread
        uint8 mErrorFallbackTexData[2u * 2u * 6u * 4u];

        void destroyAll();
        void abortAllRequests();
        void destroyAllStagingBuffers();
        void destroyAllTextures();
        void destroyAllPools();

        virtual TextureGpu *createTextureImpl( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                               IdString name, uint32 textureFlags,
                                               TextureTypes::TextureTypes initialType ) = 0;
        virtual StagingTexture *createStagingTextureImpl( uint32 width, uint32 height, uint32 depth,
                                                          uint32         slices,
                                                          PixelFormatGpu pixelFormat ) = 0;
        virtual void            destroyStagingTextureImpl( StagingTexture *stagingTexture ) = 0;

        virtual AsyncTextureTicket *createAsyncTextureTicketImpl( uint32 width, uint32 height,
                                                                  uint32 depthOrSlices,
                                                                  TextureTypes::TextureTypes textureType,
                                                                  PixelFormatGpu pixelFormatFamily ) = 0;

        /// Fills mTmpAvailableStagingTex with new StagingTextures that support formats &
        /// resolutions the worker thread couldn't upload because it lacked a compatible one.
        /// Assumes we're protected by mMutex! Called from main thread.
        void fulfillUsageStats();
        /// Fills mTmpAvailableStagingTex with new StagingTextures if there's not enough
        /// in there to meet our minimum budget as set in mBudget.
        /// Assumes we're protected by mMutex! Called from main thread.
        void fulfillMinimumBudget();

        /// See fulfillMinimumBudget and fulfillUsageStats
        /// Assumes we're protected by mMutex! Called from main thread.
        void fullfillBudget();

        /// Must be called from worker thread.
        void mergeUsageStatsIntoPrevStats();

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
        static void       processQueuedImage( QueuedImage &queuedImage, ThreadData &workerData,
                                              StreamingData &streamingData );

        static void addTransitionToLoadedCmd( ObjCmdBuffer *commandBuffer, TextureGpu *texture,
                                              void *sysRamCopy, bool toSysRam );

        /// Retrieves, in bytes, the memory consumed by StagingTextures in a container like
        /// mAvailableStagingTextures, which are textures waiting either to be reused, or to be
        /// destroyed.
        size_t getConsumedMemoryByStagingTextures( const StagingTextureVec &stagingTextures ) const;
        /** Checks if we've exceeded our memory budget for available staging textures.
                + If we haven't, it early outs and returns nullptr.
                + If we have, we'll start stalling the GPU until we find a StagingTexture that
                  can fit the requirements, and return that pointer.
                + If we couldn't find a fit, we start removing StagingTextures until we've
                  freed enough to stay below the budget; and return a nullptr.
        */
        StagingTexture *checkStagingTextureLimits( uint32 width, uint32 height, uint32 depth,
                                                   uint32 slices, PixelFormatGpu pixelFormat,
                                                   size_t minConsumptionRatioThreshold );

        /// Called from main thread. Processes mDownloadToRamQueue, which handles transitions from
        /// GpuResidency::Resident to GpuResidency::OnSystemRam when strategy is not
        /// GpuPageOutStrategy::AlwaysKeepSystemRamCopy; thus we need to download what was in GPU
        /// back to RAM.
        ///
        /// This function checks if the async tickets are done and performs the copy.
        /// It's an async download.
        ///
        /// @see    TextureGpuManager::_queueDownloadToRam
        void processDownloadToRamQueue();

    public:
        TextureGpuManager( VaoManager *vaoManager, RenderSystem *renderSystem );
        ~TextureGpuManager() override;

        void shutdown();

        /** Whether to use HW or SW mipmap generation when specifying
            TextureFilter::TypeGenerateDefaultMipmaps for loading files from textures.
            This setting has no effect for filters explicitly asking for HW mipmap generation.
        @param defaultMipmapGen
            Whether to enable HW mipmap generation for textures. Default is true.
        @param defaultMipmapGenCubemaps
            Whether to enable HW mipmap generation for cubemap textures. Default is false.
        */
        void setDefaultMipmapGeneration( DefaultMipmapGen::DefaultMipmapGen defaultMipmapGen,
                                         DefaultMipmapGen::DefaultMipmapGen defaultMipmapGenCubemaps );
        DefaultMipmapGen::DefaultMipmapGen getDefaultMipmapGeneration() const;
        DefaultMipmapGen::DefaultMipmapGen getDefaultMipmapGenerationCubemaps() const;

        /** When false, TextureFlags::TilerMemoryless will be ignored (including implicit MSAA surfaces).
            Useful if you're rendering a heavy scene and run out of tile memory on mobile / TBDR.
        @param bAllowMemoryLess
            True to allow TilerMemoryless. False otherwise.
            If HW does not support TilerMemoryless, this value will be forced to false.
        */
        void setAllowMemoryless( const bool bAllowMemoryLess );
        bool allowMemoryless() const { return mAllowMemoryLess; }

        const ResourceEntryMap &getEntries() const { return mEntries; }

        /// Must be called from main thread.
        void _reserveSlotForTexture( TextureGpu *texture );
        /// Must be called from main thread.
        void _releaseSlotFromTexture( TextureGpu *texture );

        /** Implements multiload. It is a simple job pool that picks the next request
            and loads an Image2.

            Then passes it to the worker thread as if the main thread had requested to
            load a texture from an Image2 pointer, instead of loading it from file
            or listener.

            i.e. it pretends the user loaded:
                tex->scheduleTransitionTo( GpuResidency::Resident, &image, autoDeleteImage = true );

            If there are any errors we abort and pass the raw LoadRequest to the streaming
            thread; and the streaming thread, trying to open this texture, should encounter
            the same error again and handle it properly.
        @param threadHandle
        @return
            Thread's return value.
        */
        unsigned long _updateTextureMultiLoadWorkerThread( ThreadHandle *threadHandle );

        unsigned long _updateStreamingWorkerThread( ThreadHandle *threadHandle );

    protected:
        /// This function processes a load request coming from main thread. It basically
        /// gets called once per Image to load. Usually that means once per texture,
        /// but in the case of Cubemaps being made up from multiple separate images,
        /// it may be called once per face.
        /// Must be called from worker thread.
        /// workerData is needed to pass it on to processQueuedImage
        void processLoadRequest( ObjCmdBuffer *commandBuffer, ThreadData &workerData,
                                 const LoadRequest &loadRequest );

    public:
        void _updateStreaming();

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

        /** Returns true if we're done loading all textures based on the return value of the
            last call to TextureGpuManager::_update and whether new tasks have been scheduled
            since then.

        @remark
            Do NOT call this in a loop e.g.

            @code
                // Do not do this
                while( textureGpuManager->isDoneStreaming() )
                    Sleep( 1 );
            @endcode

            Because it will spin forever!
            The return value of this function changes whenever TextureGpuManager::_update
            is called (directly or indirectly).

            The main purpose for this function is to poll whether we're done streaming
            so that e.g. users can show/hide a loading screen or loading icon.

            If you need to wait until all textures are done, use waitForStreamingCompletion
        */
        bool isDoneStreaming() const;

        /// Blocks main thread until all pending textures are fully loaded.
        void waitForStreamingCompletion();

        /** Calling waitForStreamingCompletion before Root::renderOneFrame should
            guarantee the render is perfect.

            Except... a new texture may be loaded while inside renderOneFrame.
            If that happens the render may not be perfect. You can solve that
            by rendering the frame again if you need all frames to be 'perfect':

            @code
                textureMgr->waitForStreamingCompletion();
                const oldValue = textureMgr->getLoadRequestsCounter();
                root->renderOneFrame();
                if( oldValue != textureMgr->getLoadRequestsCounter() )
                {
                    textureMgr->waitForStreamingCompletion();
                    root->renderOneFrame();
                }
            @endcode
        */
        uint64 getLoadRequestsCounter() const { return mLoadRequestsCounter; }

        /// Do not use directly. See TextureGpu::waitForMetadata & TextureGpu::waitForDataReady
        void _waitFor( TextureGpu *texture, bool metadataOnly );

        /// Do not use directly. See TextureGpu::waitForPendingSyncs
        void _waitForPendingGpuToCpuSyncs( TextureGpu *texture );

        /// Reserves and preallocates a pool with the given parameters
        /// Returns the master texture that owns the pool
        ///
        /// Destroy this pool with TextureGpuManager::destroyTexture
        TextureGpu *reservePoolId( uint32 poolId, uint32 width, uint32 height, uint32 numSlices,
                                   uint8 numMipmaps, PixelFormatGpu pixelFormat );

        bool hasPoolId( uint32 poolId, uint32 width, uint32 height, uint8 numMipmaps,
                        PixelFormatGpu pixelFormat ) const;

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
        TextureGpu *createTexture( const String &name, const String &aliasName,
                                   GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                   uint32 textureFlags, TextureTypes::TextureTypes initialType,
                                   const String &resourceGroup = BLANKSTRING, uint32 filters = 0,
                                   uint32 poolId = 0 );
        TextureGpu *createTexture( const String                          &name,
                                   GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                   uint32 textureFlags, TextureTypes::TextureTypes initialType,
                                   const String &resourceGroup = BLANKSTRING, uint32 filters = 0,
                                   uint32 poolId = 0 );
        TextureGpu *createOrRetrieveTexture( const String &name, const String &aliasName,
                                             GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                             uint32 textureFlags, TextureTypes::TextureTypes initialType,
                                             const String &resourceGroup = BLANKSTRING,
                                             uint32 filters = 0, uint32 poolId = 0 );
        TextureGpu *createOrRetrieveTexture( const String                          &name,
                                             GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                             uint32 textureFlags, TextureTypes::TextureTypes initialType,
                                             const String &resourceGroup = BLANKSTRING,
                                             uint32 filters = 0, uint32 poolId = 0 );
        /// Helper function to call createOrRetrieveTexture with common
        /// parameters used for 2D diffuse textures loaded from file.
        TextureGpu *createOrRetrieveTexture( const String &name, const String &aliasName,
                                             GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                             CommonTextureTypes::CommonTextureTypes type,
                                             const String &resourceGroup = BLANKSTRING,
                                             uint32        poolId = 0 );
        TextureGpu *createOrRetrieveTexture( const String                          &name,
                                             GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                             CommonTextureTypes::CommonTextureTypes type,
                                             const String &resourceGroup = BLANKSTRING,
                                             uint32        poolId = 0 );
        TextureGpu *findTextureNoThrow( IdString name ) const;

    protected:
        void destroyTextureImmediate( TextureGpu *texture );

    public:
        /** Destroys a texture

            Classes who wish to hold a weak reference should listen for TextureGpuListener::Deleted
            events and clear their pointers when the texture gets destroyed.

            Classes who wish to hold a stronger reference (note: it says 'stronger', not 'strong')
            should return true in TextureGpuListener::shouldStayLoaded, but it is not guaranteed
            to be honoured.

            Users should iterate through listeners and see if any listener's shouldStayLoaded
            returns true. If you still want to destroy the texture, the class should still
            be able to handle TextureGpuListener::Deleted gracefully.

            See MemoryGameState::unloadUnusedTextures in Tutorial_MemoryGameState.cpp

            Ogre doesn't call destroyTexture unless it's on shutdown or a specific Ogre-controlled
            texture (e.g. something related to PBS, Irradiance Fields, etc)

            Users are the ones in control of which textures get unloaded. It is suggested users
            group textures by criteria so that they can be loaded and unloaded in bulk (i.e.
            by relation to a level, or area in an open world game, by scene, etc)
        @param texture
        */
        void destroyTexture( TextureGpu *texture );

        /** Returns true if a texture with the given aliasName exists, or if a
            ResourceGroupListener provides such texture, or if such texture
            exists (i.e. as a file) in the ResourceGroupManager.

            This can return true regardless of whether the texture has been loaded or created.

            Not to be confused with findTextureNoThrow which only looks
            for already created textures.
        @param aliasName
        @param resourceGroup
        @return
            True if there is such texture (loaded or not)
            False if there is no such texture
        */
        bool hasTextureResource( const String &aliasName, const String &resourceGroup ) const;

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
        StagingTexture *getStagingTexture( uint32 width, uint32 height, uint32 depth, uint32 slices,
                                           PixelFormatGpu pixelFormat,
                                           size_t         minConsumptionRatioThreshold = 25u );
        void            removeStagingTexture( StagingTexture *stagingTexture );

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
        AsyncTextureTicket *createAsyncTextureTicket( uint32 width, uint32 height, uint32 depthOrSlices,
                                                      TextureTypes::TextureTypes textureType,
                                                      PixelFormatGpu             pixelFormatFamily );
        void                destroyAsyncTextureTicket( AsyncTextureTicket *ticket );
        void                destroyAllAsyncTextureTicket();

        void saveTexture( TextureGpu *texture, const String &folderPath,
                          set<String>::type &savedTextures, bool saveOitd, bool saveOriginal,
                          HlmsTextureExportListener *listener );

        /** Checks if the given format with the texture flags combination is supported

        @param format
        @param textureFlags
            See TextureFlags::TextureFlags
            Supported flags are:
                NotTexture
                RenderToTexture
                Uav
                AllowAutomipmaps

            When NotTexture is set, we don't check whether it's possible to sample from
            this texture. Note that some buggy Android drivers may report that it's not
            possible to sample from that texture when it actually is.
        @return
            True if supported. False otherwise
        */
        virtual bool checkSupport( PixelFormatGpu format, TextureTypes::TextureTypes textureType,
                                   uint32 textureFlags ) const;

    protected:
        /// Returns false if the entry was not found in the cache
        bool applyMetadataCacheTo( TextureGpu *texture );

    public:
        void _updateMetadataCache( TextureGpu *texture );
        void _removeMetadataCacheEntry( TextureGpu *texture );
        void importTextureMetadataCache( const String &filename, const char *jsonString,
                                         bool bCreateReservedPools );
        void exportTextureMetadataCache( String &outJson );

        void getMemoryStats( size_t &outTextureBytesCpu, size_t &outTextureBytesGpu,
                             size_t &outUsedStagingTextureBytes,
                             size_t &outAvailableStagingTextureBytes );

        void dumpStats() const;
        void dumpMemoryUsage( Log *log, Ogre::uint32 mask = ResidencyMask::All ) const;

        /// Sets a new listener. The old one will be destroyed with OGRE_DELETE
        /// See TextureGpuManagerListener. Pointer cannot be null.
        void setTextureGpuManagerListener( TextureGpuManagerListener *listener );

        /** OgreNext always performs background streaming to load textures in a worker thread.
            However there is only ONE background thread performing all work serially while
            the main thread can do other stuff (like rendering, even if textures aren't ready).

            There are times where you need to load many textures at once and having
            a threadpool of textures increases throughput.

            The threadpool will load N textures at once into RAM, and then send them
            to the background thread to upload them to the GPU.

            Once the background thread is ready, the main thread is signalled about the situation.

            Enabling the MultiLoad pool can give performance benefits in the following scenarios:

              - You are IO limited: the threadpool gets stalled but in the meantime the
                background thread still loads what's ready in System RAM into GPU RAM
              - You are ALU/CPU limited: Loading PNG/JPG files needs decoding.
                Loading multiple at once is better use of CPU resources.

            Enabling Multiload pool may increase memory consumption because you may end
            up with many images (more than numThreads) loaded in RAM.

            You can change this value at any time, but it is an expensive call as we need
            to synchronize with the threadpool and flush all work.

            Testing shows on an AMD Ryzen 5900X (12C/24T) loading of many PNG & JPG files of
            varying sizes (up to 2048x2048 RGBA8_UNORM) was cut from 1 second to 0.7 seconds
            in a Debug build.

            However a Lenovo TB-X6C6X (4C/4T) Android tablet, the same workload cut from
            5 seconds to 1 second, in a Release build.
        @remarks
            Because of the multithreaded nature, enabling this feature means textures may be
            loaded out of order.

            Without this feature, normally textures would be loaded in order, unless there was
            an issue (e.g. file couldn't be found, raising an exception; the metadata cache
            was out of date, etc).

            This can have an impact if you rely on the order (e.g. if you are using reservePoolId()).
            If you need to preserve ordering, you can use TextureGpu::scheduleTransition and
            set bSkipMultiload = true.

            Testing indicates the ideal value is somewhere between 4-8 threads.
            More threads and you get diminishing returns.
        @param numThreads
            How many number of threads to use for loading multiple textures.
            0 to disable this feature (Default).
        */
        void setMultiLoadPool( uint32 numThreads );

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
        @param maxSplitResolution
            Textures bigger than this resolution in any axis will be taken
            as "exceptions" or "spikes" that won't last long. e.g. if maxSplitResolution = 2048
            then a 2048x16, 67x2048, 2048x2048, and a 4096x4096 texture will all be considered abnormal.

            This can significantly affect how much memory we consume while streaming.
            A value of 0 means to keep current value.

            If an entry in the budget contains minNumSlices > 1 and minResolution >= maxSplitResolution
            then a lot of memory waste could end up being caused; thus we will warn to the Ogre.log
            if you set such setting.

            The default value in 32-bit systems and mobile is 2048
            The default value in 64-bit Desktop systems is 4096

            This setting is closely related to setWorkerThreadMaxPerStagingTextureRequestBytes,
            because a texture whose resolution is >= maxSplitResolution will force us to use
            multiple StagingTextures, thus relieving the pressure on memory and memory fragmentation.
        */
        void setWorkerThreadMinimumBudget( const BudgetEntryVec &budget, uint32 maxSplitResolution = 0 );

        const BudgetEntryVec &getBudget() const;

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

        /** The worker thread tracks how many data it is loading so the Main thread can request
            additional StagingTextures if necessary.

            One big StagingTexture reduces the amount of time we map memory so we can upload.

            However one big StagingTexture also means that if we've used 1 byte out of 200MB available,
            we have to wait until that byte has finished transferring (that usually means the
            StagingTexture becomes available 3 frames later); which can result in three big
            StagingTextures (one for each frame) which can be overkill.

            This function allows you to specify when we decide to break these
            requests in smaller pieces, which by default is set at 64MB
        @param maxPerStagingTextureRequestBytes
        */
        void setWorkerThreadMaxPerStagingTextureRequestBytes( size_t maxPerStagingTextureRequestBytes );

        /** The main thread tries to acquire a lock from the background thread,
            do something very quick, and release it.

            If the lock failed to acquire, we try again next time _update is called.
            However if this happens too often in a row, we should stall and wait indefinitely
            for the background thread.

            This function allows you to specify how many failures we have to let pass before
            we stall.
        @remarks
            This is a failsafe mechanism for edge case behaviors that should never happen.
            It is rare for the tryLock() to fail more than twice in a row.

            However if loading a several big files (e.g. large cubemaps) or loading from a slow medium
            (e.g. from the internet directly) many tryLock() failures could be common.

            If failure to acquire the lock is common and expected, small limit values could cause a
            lot of stutter, because e.g. a value of 3 could cause fps lag spikes every 3 frames.

            A sensible value such as 1200 means that a stall would only happen after 20 seconds of
            repeated failure if running at constant 60 fps.
        @param tryLockFailureLimit
            How many failures we have to wait for a stall, expressed in calls
            TextureGpuManager::_update. Usually there's one call to _update per frame, but there
            can be more.
            Use 0 to always stall
            Use std::numeric_limits<uint32>::max() for no failure limits (i.e. never stall)
        */
        void setTrylockMutexFailureLimit( uint32 tryLockFailureLimit );

        /** When enabled, we will profile the time it takes a texture
            to go from Resident to Ready and Log it.
        @param bProfile
            True to enable. False to disable profiling.
            Default value depends on Debug mode (default to true on Debug, false on Release)
        */
        void setProfileLoadingTime( bool bProfile );

#ifdef OGRE_PROFILING_TEXTURES
        bool getProfileLoadingTime() const { return mProfilingLoadingTime; }
#else
        bool getProfileLoadingTime() const { return false; }
#endif

        /// This function CAN be called from any thread
        const String *findAliasNameStr( IdString idName ) const;
        /// This function CAN be called from any thread
        const String *findResourceNameStr( IdString idName ) const;
        /// This function CAN be called from any thread
        const String *findResourceGroupStr( IdString idName ) const;

    protected:
        /// Implements TaskTypeResidencyTransition when doing any of the following transitions:
        ///     OnStorage   -> Resident
        ///     OnStorage   -> OnSystemRam
        ///     OnSystemRam -> Resident
        void taskLoadToSysRamOrResident( TextureGpu *texture, const ScheduledTasks &task );
        /// Implements TaskTypeResidencyTransition when doing any of the following transitions:
        ///     Resident    -> OnStorage
        ///     Resident    -> OnSystemRam
        ///     OnSystemRam -> OnStorage
        ///
        /// Also implements TaskTypeDestroyTexture
        void taskToUnloadOrDestroy( TextureGpu *texture, const ScheduledTasks &task );
        bool executeTask( TextureGpu *texture, TextureGpuListener::Reason reason,
                          const ScheduledTasks &task );
        void notifyTextureChanged( TextureGpu *texture, TextureGpuListener::Reason reason,
                                   bool ignoreDelay );

    public:
        /// @see    TextureGpuListener::notifyTextureChanged
        void notifyTextureChanged( TextureGpu *texture, TextureGpuListener::Reason reason,
                                   void *extraData ) override;

        RenderSystem *getRenderSystem() const;
        VaoManager   *getVaoManager() const;

    protected:
        void scheduleLoadRequest( TextureGpu *texture, Image2 *image, bool autoDeleteImage,
                                  bool toSysRam, bool reuploadOnly, bool bSkipMultiload );

        /// Transitions a texture from OnSystemRam to Resident; and asks the worker thread
        /// to transfer the data in the background.
        /// Because the Texture is already loaded (but on RAM), the texture's contents are
        /// authoritative, i.e. we bypass the metadata cache and ignore changes to the file on
        /// disk and loading listeners.
        void scheduleLoadFromRam( TextureGpu *texture );

        void scheduleLoadRequest( TextureGpu *texture, const String &name, const String &resourceGroup,
                                  uint32 filters, Image2 *image, bool autoDeleteImage, bool toSysRam,
                                  bool reuploadOnly, bool bSkipMultiload = false,
                                  bool   skipMetadataCache = false,
                                  uint32 sliceOrDepth = std::numeric_limits<uint32>::max() );

    public:
        void _scheduleUpdate( TextureGpu *texture, uint32 filters, Image2 *image, bool autoDeleteImage,
                              bool   skipMetadataCache = false,
                              uint32 sliceOrDepth = std::numeric_limits<uint32>::max() );

        void _scheduleTransitionTo( TextureGpu *texture, GpuResidency::GpuResidency targetResidency,
                                    Image2 *image, bool autoDeleteImage, bool reuploadOnly,
                                    bool bSkipMultiload );
        void _queueDownloadToRam( TextureGpu *texture, bool resyncOnly );
        /// When true we will ignore all tasks in mScheduledTasks and execute transitions immediately
        /// Caller is responsible for ensuring this is safe to do.
        ///
        /// The main reason for this function is that when the metadata cache is proven to be out of
        /// date and comes back to the main thread, we need to perform a
        /// Resident -> OnStorage -> Resident transition that bypasses pending operations,
        /// and pretend the texture has been in Resident all along.
        void _setIgnoreScheduledTasks( bool ignoreSchedTasks );
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
