/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2023 Torus Knot Software Ltd

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

#ifndef OgreParticleSystemManager2_H
#define OgreParticleSystemManager2_H

#include "OgrePrerequisites.h"

#include "OgreIdString.h"
#include "OgreRenderQueue.h"
#include "ParticleSystem/OgreParticle2.h"
#include "Threading/OgreSemaphore.h"

#include <map>

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    class ParticleSystemDef;

    class ParticleAffectorFactory2;
    class ParticleEmitterDefDataFactory;

    class ArrayAabb;

    class _OgreExport ParticleSystemManager2
    {
        SceneManager *ogre_nullable mSceneManager;

        FastArray<ParticleSystemDef *> mActiveParticleSystemDefs;

        std::map<IdString, ParticleSystemDef *> mParticleSystemDefMap;

        FastArray<BillboardSet *> mBillboardSets;

        IndexBufferPacked *mSharedIndexBuffer16;
        IndexBufferPacked *mSharedIndexBuffer32;
        uint32             mHighestPossibleQuota16;
        uint32             mHighestPossibleQuota32;

        float mTimeSinceLast;  // For threaded update.

        // There's one ParticleSystemManager2 owned by Root (the 'master') which holds all templates.
        // And one ParticleSystemManager2 per SceneManager that reference Root's.
        ParticleSystemManager2 *ogre_nullable mMaster;
        ObjectMemoryManager                  *mMemoryManager;

        Vector3                        mCameraPos;
        FastArray<ParticleSystemDef *> mActiveParticlesLeftToSort;  // GUARDED_BY( mSortMutex )
        LightweightMutex               mSortMutex;

        void calculateHighestPossibleQuota( VaoManager *vaoManager );
        void createSharedIndexBuffers( VaoManager *vaoManager );

        inline void tickParticles( size_t threadIdx, ArrayReal timeSinceLast, ParticleCpuData cpuData,
                                   ParticleGpuData *gpuData, const size_t numParticles,
                                   ParticleSystemDef *systemDef, ArrayAabb &inOutAabb );

        inline void sortAndPrepare( ParticleSystemDef *systemDef, const Vector3 &camPos,
                                    float timeSinceLast );

        void updateSerialPos();

    public:
        ParticleSystemManager2( SceneManager *ogre_nullable           sceneManager,
                                ParticleSystemManager2 *ogre_nullable master );
        ~ParticleSystemManager2();

        static void addEmitterFactory( ParticleEmitterDefDataFactory *factory );
        static void removeEmitterFactory( ParticleEmitterDefDataFactory *factory );

        static ParticleEmitterDefDataFactory *getFactory( IdString name );

        static void addAffectorFactory( ParticleAffectorFactory2 *factory );
        static void removeAffectorFactory( ParticleAffectorFactory2 *factory );

        static ParticleAffectorFactory2 *getAffectorFactory( IdString name );

        /** ParticleSystemManager2 must know the highest possible quota any of its particle
            systems may achieve.

            After the index buffer has been initialized, this value cannot be changed.

            After initialization you can create new ParticleSystemDef that set quotas
            lower than the already highest possible one. However you cannot set a quota
            larger than that.

            Make sure to call this function with the highest values you anticipate your particle
            systems will need.

            Note this value directly correlates to VRAM used.
            The amount of bytes consumed are : highestQuota32 * 6 * 4 bytes + highestQuota16 * 6 * 2
        @param highestQuota16
            The highest possible quota for ParticleSystemDefs that need less than 65536 particles.
            If unsure, you can set it to 65536.

            This value can be 0, however we may raise it if we find a ParticleSystemDef that needs it.
        @param highestQuota32
            The highest possible quota for ParticleSystemDefs that need more than 65535 particles.
            This value can be 0, however we may raise it if we find a ParticleSystemDef that needs it.
        */
        void setHighestPossibleQuota( uint16 highestQuota16, uint32 highestQuota32 );

        uint32 getHighestPossibleQuota16() const { return mHighestPossibleQuota16 / 4u; }
        uint32 getHighestPossibleQuota32() const { return mHighestPossibleQuota32 / 4u; }

        ParticleSystemDef *createParticleSystemDef( const String &name );

        /** Retrieves an existing ParticleSystemDef with the given name
            Throws if not found.
        @remarks
            If we 'this' belongs to a SceneManager and a ParticleSystemDef hasn't been
            cloned yet, we will look into Root's ParticleSystemManager2 to see if they
            have a master definition.

            We only throw if Root's version doesn't have a definition by that name either.
        @param name
            Name of the ParticleSystemDef.
        @param bAutoInit
            When true, we always call ParticleSystemDef::init after cloning.
            When false, we don't (useful if you want to customize).
        @return
            The ParticleSystemDef.
        */
        ParticleSystemDef *getParticleSystemDef( const String &name, bool bAutoInit = true );

        /** Returns true if this ParticleSystemManager2 has a ParticleSystemDef by that name.
        @param name
            Name of the ParticleSystemDef.
        @param bSearchInRoot
            When true, we also search in Root's ParticleSystemManager2 to see if they
            have a master definition.
        @return
            True if found.
            False if not found.
        */
        bool hasParticleSystemDef( const String &name, bool bSearchInRoot = true ) const;

        void destroyAllParticleSystems();

        /** Creates a BillboardSet.
        @return
            Pointer to newly created BillboardSet.
        */
        BillboardSet *createBillboardSet();

        /** Destroys a BillboardSet created with createBillboardSet().
        @param billboardSet
            Set to destroy.
        */
        void destroyBillboardSet( BillboardSet *billboardSet );

        /// Destroys all BillboardSet created with createBillboardSet().
        /// Do not hold any more references to those pointers as they will become dangling!
        void destroyAllBillboardSets();

        /** Instructs us to add all the ParticleSystemDef to the RenderQueue that match the given
            renderQueueId and pass the visibilityMask.
        @param threadIdx
        @param numThreads
        @param renderQueue
        @param renderQueueId
        @param visibilityMask
        @param includeNonCasters
        */
        void _addToRenderQueue( size_t threadIdx, size_t numThreads, RenderQueue *renderQueue,
                                uint8 renderQueueId, uint32 visibilityMask,
                                bool includeNonCasters ) const;

        IndexBufferPacked *_getSharedIndexBuffer( size_t maxQuota, VaoManager *vaoManager );

        /** All instances are sorted every frame to their distance to camera.

            Closer instances are given higher priority to emit. If instances reach the shared Quota
            (see ParticleSystemDef::setParticleQuota) then eventually over time far instances will run
            out of particles as they can no longer emit more, while close instances will still
            be emitting as much as possible.

        @remarks
            Only one camera position per SceneManager is supported.
            If rendering from multiple camera positions, consider using the most relevant position
            for the simulation.

            This value does not control rendering. It's not instantaneous. It merely tells the
            simulation which systems should be prioritized for emission for this frame.

        @param camPos
            Camera position
        */
        void setCameraPosition( const Vector3 &camPos ) { mCameraPos = camPos; }

        const Vector3 &getCameraPosition() const { return mCameraPos; }

        /** The order of function calls is:
                1. manager->prepareForUpdate( timeSinceLast ) (main thread)
                2. manager->_prepareParallel() (from many threads)
                3. manager->update() (main thread)
                    - This function will call _updateParallel from all threads.

            Prepares everything for caller to later call _prepareParallel() from all threads.
            This function must be called from main thread.
        @param timeSinceLast
            Time in seconds since last frame (to advance the particle simulation).
        */
        void prepareForUpdate( Real timeSinceLast );

        /** See prepareForUpdate()
            This function is called from multiple threads.

            Each thread handles a whole ParticleSystemDef. (i.e. 2 threads won't concurrently
            access the same ParticleSystemDef).

            It handles sorting them by distance to camera; and emits
            new particles that instances require.
        */
        void _prepareParallel();

        /** See prepareForUpdate()
            This function is called from multiple threads.
        @remarks
            Unlike _prepareParallel(), each thread concurrently access the same ParticleSystemDef.
            This function is in charge of initializing new particles.
            Each thread handles one particle (i.e. 2 threads won't concurrently access the same
            ParticleCpuData).
            @par
            We must split the parallel update in two stages because otherwise particles
            initialized by thread A could be ticked (tickParticles()) by thread B.
            See https://github.com/OGRECave/ogre-next/issues/480
        */
        void _updateParallel01( size_t threadIdx, size_t numThreads );

        /** See _updateParallel01().
        @remarks
            Unlike _prepareParallel(), each thread concurrently access the same ParticleSystemDef.
            This function is in charge of advancing the simulation of each particle forward.
            Each thread handles one particle (i.e. 2 threads won't concurrently access the same
            ParticleCpuData).
        */
        void _updateParallel02( size_t threadIdx, size_t numThreads );

        /// See prepareForUpdate()
        ///
        /// Must be called after prepareForUpdate() & _prepareParallel().
        /// This function must be called from main thread.
        void update();

        void _addParticleSystemDefAsActive( ParticleSystemDef *def );
        void _removeParticleSystemDefFromActive( ParticleSystemDef *def );
    };

    OGRE_ASSUME_NONNULL_END
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
