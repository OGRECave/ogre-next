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
#ifndef _OgreRenderQueue_H_
#define _OgreRenderQueue_H_

#include "OgrePrerequisites.h"

#include "OgreHlmsCommon.h"
#include "OgreIteratorWrappers.h"
#include "OgreSharedPtr.h"
#include "Threading/OgreLightweightMutex.h"
#include "Threading/OgreSemaphore.h"

#include "OgreHeaderPrefix.h"

#include <atomic>

namespace Ogre
{
    class Camera;
    class MovableObject;

    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup RenderSystem
     *  @{
     */

    struct QueuedRenderable
    {
        uint64               hash;
        Renderable          *renderable;
        MovableObject const *movableObject;

        QueuedRenderable() : hash( 0 ), renderable( 0 ), movableObject( 0 ) {}
        QueuedRenderable( uint64 _hash, Renderable *_renderable, const MovableObject *_movableObject ) :
            hash( _hash ),
            renderable( _renderable ),
            movableObject( _movableObject )
        {
        }

        bool operator<( const QueuedRenderable &_r ) const { return this->hash < _r.hash; }
    };

    class ParallelHlmsCompileQueue
    {
    public:
        struct Request
        {
            /// Must stay alive until worker thread is done with it. In the case of warmUpSerial &
            /// updateWarmUpThread, we store instead an index to RenderQueue::mPendingPassCaches
            /// because otherwise the iterators keep getting invalidated.
            HlmsCache const *passCache;
            HlmsCache       *reservedStubEntry;
            QueuedRenderable queuedRenderable;
            uint32           renderableHash;
            uint32           finalHash;
        };

    protected:
        std::vector<Request> mRequests;  // GUARDED_BY( mMutex )
        LightweightMutex     mMutex;
        Semaphore            mSemaphore;
        std::atomic<bool>    mKeepCompiling;

        bool               mExceptionFound;     // GUARDED_BY( mMutex )
        std::exception_ptr mThreadedException;  // GUARDED_BY( mMutex )

    public:
        ParallelHlmsCompileQueue();

        inline void pushRequest( const Request &&request )
        {
            ScopedLock lock( mMutex );
            mRequests.emplace_back( request );
            mSemaphore.increment();
        }

        inline void pushWarmUpRequest( const Request &&request ) { mRequests.emplace_back( request ); }

        /** Starts worker threads (job queue) so they start accepting work every time pushRequest()
            gets called and will keep compiling those shaders until stopAndWait() is called.

            The work is done in updateThread() and is in charge of compiling shaders AND generating PSOs.
        @remarks
            This function must not be called if RenderSystem::supportsMultithreadedShaderCompilation
            is false.
        @param sceneManager
        */
        void start( SceneManager *sceneManager );
        /** Signals worker threads we won't be submitting more work, so they should stop once they're
            done compiling all pending shaders / PSOs.

            Will wait until all shaders are done.
        @param sceneManager
        */
        void stopAndWait( SceneManager *sceneManager );
        /// The actual work done by the job queues.
        void updateThread( size_t threadIdx, HlmsManager *hlmsManager );

        /// Similar to start() and stopAndWait() at the same time: It assumes all work has already been
        /// gathered in mRequests via pushWarmUpRequest() (instead of gather it as we go)
        /// and fires all threads to compile the shaders and PSOs in parallel.
        ///
        /// It will wait until all threads are done.
        void fireWarmUpParallel( SceneManager *sceneManager );

        /// The actual work done by fireWarmUpParallel().
        void updateWarmUpThread( size_t threadIdx, HlmsManager *hlmsManager,
                                 const HlmsCache *passCaches );

        /// Serial alternative of fireWarmUpParallel() + updateWarmUpThread() for when
        /// RenderSystem::supportsMultithreadedShaderCompilation is false.
        void warmUpSerial( HlmsManager *hlmsManager, const HlmsCache *passCaches );
    };

    /** Class to manage the scene object rendering queue.
        @remarks
            Objects are grouped by material to minimise rendering state changes. The map from
            material to renderable object is wrapped in a class for ease of use.
        @par
            This class includes the concept of 'queue groups' which allows the application
            adding the renderable to specifically schedule it so that it is included in
            a discrete group. Good for separating renderables into the main scene,
            backgrounds and overlays, and also could be used in the future for more
            complex multipass routines like stenciling.
        @remarks
            By default, movables will be assigned the following render queue ID:\n
            Decal:              0 \n
            Light:              0-5 (depends on light type, cannot be changed) \n
            Item:               10 \n
            Rectangle2D:        10 \n
            v1::Entity:         110 \n
            v1::Rectangle2D:    110 \n
            ParticleSystem (v1):110 \n
            ParticleSystem (v2):kParticleSystemDefaultRenderQueueId \n
            [.. more unlisted ..]
        @remarks
            By default, the render queues have the following mode set: \n
            0-99:    FAST \n
            100-199: V1_FAST \n
            200-224: FAST \n
            225-255: V1_FAST
    */
    class _OgreExport RenderQueue : public OgreAllocatedObj
    {
    public:
        enum Modes
        {
            /// This is the slowest mode. Renders the same or similar way to how Ogre 1.x
            /// rendered meshes. Only v1 entities can be put in this type of queue.
            /// Ideal for low level materials, InstancedEntity via InstanceManager,
            /// and any other weird custom object.
            V1_LEGACY,

            /// Renders v1 entities using HLMS materials with some of the new benefits,
            /// but some deprecated features from Ogre 1.x might not be available
            /// or work properly (like global instancing buffer).
            /// Ideal for most v1 entities, particle effects and billboards using HLMS
            /// materials.
            /// Cannot be used by RenderSystems that don't support constant buffers
            /// (i.e. OpenGL ES 2)
            V1_FAST,

            /// Renders v2 items using HLMS materials with minimum driver overhead
            /// Recommended method for rendering extremely large amounts of objects
            /// of homogeneous and heterogenous meshes, with many different kinds
            /// of materials and textures.
            /// Only v2 Items can be put in this queue.
            FAST,

            /// Renders ParticleSystemDef (i.e. ParticleFX2 plugin).
            /// Don't put v1 Particle Systems here.
            PARTICLE_SYSTEM,
        };

        enum RqSortMode
        {
            DisableSort,
            NormalSort,
            StableSort,
        };

    private:
        typedef FastArray<QueuedRenderable> QueuedRenderableArray;

        struct ThreadRenderQueue
        {
            QueuedRenderableArray q;
            /// The padding prevents false cache sharing when multithreading.
            uint8 padding[128];
        };

        typedef FastArray<ThreadRenderQueue> QueuedRenderableArrayPerThread;

        struct RenderQueueGroup
        {
            QueuedRenderableArrayPerThread mQueuedRenderablesPerThread;
            QueuedRenderableArray          mQueuedRenderables;
            RqSortMode                     mSortMode;
            bool                           mSorted;
            Modes                          mMode;

            RenderQueueGroup() : mSortMode( NormalSort ), mSorted( false ), mMode( FAST ) {}
        };

        typedef vector<IndirectBufferPacked *>::type IndirectBufferPackedVec;

        struct PsoCreateEntry
        {
            uint32           finalHash;
            uint16           cacheIdx;
            bool             casterPass;
            QueuedRenderable queuedRenderable;

            bool operator<( const PsoCreateEntry &b ) const { return this->finalHash < b.finalHash; }
            bool operator<( const uint32_t otherHash ) const { return this->finalHash < otherHash; }
        };
        typedef std::vector<PsoCreateEntry> PsoCreateEntryVec;

        RenderQueueGroup mRenderQueues[256];

        HlmsManager  *mHlmsManager;
        SceneManager *mSceneManager;
        VaoManager   *mVaoManager;
        Root         *mRoot;

        bool                  mLastWasCasterPass;
        uint32                mLastVaoName;
        v1::VertexData const *mLastVertexData;
        v1::IndexData const  *mLastIndexData;
        uint32                mLastTextureHash;

        CommandBuffer          *mCommandBuffer;
        IndirectBufferPackedVec mFreeIndirectBuffers;
        IndirectBufferPackedVec mUsedIndirectBuffers;

        HlmsCache mPassCache[HLMS_MAX];

        uint32 mRenderingStarted;

        std::vector<HlmsCache> mPendingPassCaches;

        ParallelHlmsCompileQueue mParallelHlmsCompileQueue;

        /** Returns a new (or an existing) indirect buffer that can hold the requested number of
        draws.
        @param numDraws
            Number of draws the indirect buffer is expected to hold. It must be an upper limit.
            The caller may end up using less draws if he desires.
        @return
            Pointer to usable indirect buffer
        */
        IndirectBufferPacked *getIndirectBuffer( size_t numDraws );

        FORCEINLINE void addRenderable( size_t threadIdx, uint8 renderQueueId, bool casterPass,
                                        Renderable *pRend, const MovableObject *pMovableObject,
                                        bool isV1 );

        void renderES2( RenderSystem *rs, bool casterPass, bool dualParaboloid,
                        HlmsCache passCache[HLMS_MAX], const RenderQueueGroup &renderQueueGroup );

        uint8 *renderParticles( RenderSystem *rs, bool casterPass, HlmsCache passCache[],
                                const RenderQueueGroup   &renderQueueGroup,
                                ParallelHlmsCompileQueue *parallelCompileQueue,
                                IndirectBufferPacked *indirectBuffer, uint8 *indirectDraw,
                                uint8 *startIndirectDraw );

        /// Renders in a compatible way with GL 3.3 and D3D11. Can only render V2 objects
        /// (i.e. Items, VertexArrayObject)
        unsigned char *renderGL3( RenderSystem *rs, bool casterPass, bool dualParaboloid,
                                  HlmsCache passCache[], const RenderQueueGroup &renderQueueGroup,
                                  ParallelHlmsCompileQueue *parallelCompileQueue,
                                  IndirectBufferPacked *indirectBuffer, unsigned char *indirectDraw,
                                  unsigned char *startIndirectDraw );
        void renderGL3V1( RenderSystem *rs, bool casterPass, bool dualParaboloid, HlmsCache passCache[],
                          const RenderQueueGroup   &renderQueueGroup,
                          ParallelHlmsCompileQueue *parallelCompileQueue );

        void warmUpShaders( bool casterPass, const RenderQueueGroup &renderQueueGroup );

    public:
        RenderQueue( HlmsManager *hlmsManager, SceneManager *sceneManager, VaoManager *vaoManager );
        ~RenderQueue();

        void _releaseManualHardwareResources();

        /// Empty the queue - should only be called by SceneManagers.
        void clear();

        /** The RenderQueue keeps track of API state to avoid redundant state change passes
            Calling this function forces the RenderQueue to re-set the Macro- & Blendblocks,
            shaders, and any other API dependendant calls on the next render.
        @remarks
            Calling this function inside render or renderES2 won't have any effect.
        */
        void clearState();

        /// Add a renderable (Ogre v1.x) object to the queue. @see addRenderable
        void addRenderableV1( uint8 renderQueueId, bool casterPass, Renderable *pRend,
                              const MovableObject *pMovableObject );

        /** Add a renderable (Ogre v2.0, i.e. Items; they use VAOs) object to the queue.
        @remarks
            If sorting mode is set to DisableSort (RenderQueue::setSortRenderQueue)
            for the given renderQueueId, then the order in which renderables are
            added via RenderQueue::addRenderableV2 (and V1) determines the
            render order (the first added get rendered first) within that renderQueueId.
        @param threadIdx
            The unique index of the thread from which this function is called from.
            Valid range is [0; SceneManager::mNumWorkerThreads)
        @param renderQueueId
            The ID of the render queue. Must be the same as the ID in
            pMovableObject->getRenderQueueGroup()
        @param casterPass
            Whether we're performing the shadow mapping pass.
        @param pRend
            Pointer to the Renderable to be added to the queue.
        @param pMovableObject
            Pointer to the MovableObject linked to the Renderable.
        */
        void addRenderableV2( size_t threadIdx, uint8 renderQueueId, bool casterPass, Renderable *pRend,
                              const MovableObject *pMovableObject );

        /** If you need to call RenderQueue::render, then you must call this function.
            This function MUST be called (all listed functions are called in this order):
                1. After RenderSystem::beginRenderPassDescriptor
                2. After SceneManager::fireRenderQueueStarted
                3. Before RenderSystem::executeRenderPassDescriptorDelayedActions
                4. Before RenderQueue::render

            Note that fireRenderQueueStarted just fires arbitrary listeners. You don't have
            to call that function if you are sure you don't need it.

            To clarify functions are called in this order:
            @code
                mRenderSystem->beginRenderPassDescriptor();
                mSceneManager->fireRenderQueueStarted();
                mRenderQueue->renderPassPrepare();
                mRenderSystem->executeRenderPassDescriptorDelayedActions();
                mRenderQueue->render();
            @endcode

            Calling these functions in proper order is needed for best compatibility with Metal
        @param casterPass
        @param dualParaboloid
        */
        void renderPassPrepare( bool casterPass, bool dualParaboloid );

        void render( RenderSystem *rs, uint8 firstRq, uint8 lastRq, bool casterPass,
                     bool dualParaboloid );

        /** Simulates what render() would do. But doesn't actually render and skips as much as possible
            We are only interested in generating the PSOs for compiling them early.
        @param firstRq
        @param lastRq
        @param casterPass
        */
        void warmUpShadersCollect( uint8 firstRq, uint8 lastRq, bool casterPass );
        void warmUpShadersTrigger( RenderSystem *rs );

        void _warmUpShadersThread( size_t threadIdx );

        void _compileShadersThread( size_t threadIdx );

        /// Don't call this too often. Only renders v1 objects at the moment.
        void renderSingleObject( Renderable *pRend, const MovableObject *pMovableObject,
                                 RenderSystem *rs, bool casterPass, bool dualParaboloid );

        /// Called when the frame has fully ended (ALL passes have been executed to all RTTs)
        void frameEnded();

        /** Sets the mode for the RenderQueue ID. @see RenderQueue::Modes
        @param rqId
            ID of the render queue
        @param newMode
            The new mode to use.
        */
        void               setRenderQueueMode( uint8 rqId, RenderQueue::Modes newMode );
        RenderQueue::Modes getRenderQueueMode( uint8 rqId ) const;

        /** Sets whether we should sort the render queue ID every frame.
        @param rqId
            ID of the render queue
        @param bSort
            When false, the render queue group won't be sorted. Useful when the RQ
            needs to be drawn exactly in the order that Renderables were added,
            or when you have a deep CPU bottleneck where the time taken to
            sort hurts more than it is supposed to help.
        */
        void       setSortRenderQueue( uint8 rqId, RqSortMode sortMode );
        RqSortMode getSortRenderQueue( uint8 rqId ) const;
    };

#define OGRE_RQ_MAKE_MASK( x ) ( ( 1 << ( x ) ) - 1 )

    class _OgreExport RqBits
    {
    public:
        static const int SubRqIdBits;
        static const int TransparencyBits;
        static const int MacroblockBits;
        static const int ShaderBits;
        static const int MeshBits;
        static const int TextureBits;
        static const int DepthBits;

        static const int SubRqIdShift;
        static const int TransparencyShift;
        static const int MacroblockShift;
        static const int ShaderShift;
        static const int MeshShift;
        static const int TextureShift;
        static const int DepthShift;

        static const int DepthShiftTransp;
        static const int MacroblockShiftTransp;
        static const int ShaderShiftTransp;
        static const int MeshShiftTransp;
        static const int TextureShiftTransp;
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
