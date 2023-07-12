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

#include "OgrePrerequisites.h"

#include "OgreBitset.h"
#include "OgreMovableObject.h"
#include "OgreParticleSystem.h"
#include "ParticleSystem/OgreParticle2.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    class Affector;
    class EmitterDefData;
    struct EmitterInstanceData;
    class ParticleSystem2;

    /// A ParticleSystemDef does not form part of the scene.
    ///
    /// The reason it derives from ParticleSystem (& hence MovableObject) is to keep backwards
    /// compatibility with the interface of the older ParticleFX plugin for much easier
    /// migration/porting.
    ///
    /// Do NOT attach it to a SceneNode.
    class _OgreExport ParticleSystemDef : public ParticleSystem
    {
    public:
        static constexpr uint32 InvalidHandle = 0xFFFFFFFF;

    protected:
        friend class ParticleSystemManager2;

        String mName;

        ParticleSystemManager2 *mParticleSystemManager;

        ReadOnlyBufferPacked *mGpuData;

        FastArray<EmitterDefData *> mEmitters;
        FastArray<Affector *>       mAffectors;

        /// Particles are distributed across all instances.
        ///
        /// We don't use ArrayMemoryManager because we exploit the property that most particles
        /// are short lived and recreated in a circular buffer (i.e. particles are mostly FIFO).
        ///
        /// We also don't need to handle regrow: Once we reach the quota, no more particles are emitted.
        bitset64         mActiveParticles;
        ParticleCpuData  mParticleCpuData;
        ParticleGpuData *mParticleGpuData;
        /// Tracks first bit set in mActiveParticles. Inclusive. Range is [0; getQuota()).
        uint32 mFirstParticleIdx;
        /// Tracks one bit after the last bit set in mActiveParticles. Exclusive.
        ///
        /// Range is [0; getQuota() * 2) but the following must be true:
        ///     mLastParticleIdx - mFirstParticleIdx <= getQuota()
        uint32 mLastParticleIdx;
        bool   mParticleQuotaFull;

        FastArray<ParticleSystem2 *> mParticleSystems;
        /// Contains ACTIVE particle systems to be processed this frame. Sorted by relevance/priority.
        FastArray<ParticleSystem2 *> mActiveParticleSystems;

        /// This is a "temporary" array used by ParticleSystemManager::updateSerial to store
        /// the newly created particles for ParticleSystemManager::update to process.
        FastArray<uint32> mNewParticles;

        /// One per thread.
        FastArray<FastArray<uint32>> mParticlesToKill;

        uint32 allocParticle();

        void deallocParticle( uint32 handle );

        /** Gets the particle handle based on cpuData's current advanced pointers and its idx
        @param cpuData
            Collection of pointers. Must be advanced.
        @param idx
            In range [0; ARRAY_PACKED_REALS)
        @return
            Particle handle. See allocParticle() and deallocParticle().
        */
        uint32 getHandle( const ParticleCpuData &cpuData, size_t idx ) const;

        /// Returns the value we need to call ParticleCpuData::advancePack with so that
        /// we end up at the beginning of the first active particle.
        ///
        /// Note that mFirstParticleIdx may not be multiple of ARRAY_PACKED_REALS.
        size_t getActiveParticlesPackOffset() const { return mFirstParticleIdx / ARRAY_PACKED_REALS; }

    public:
        ParticleSystemDef( SceneManager *manager, ParticleSystemManager2 *particleSystemManager,
                           const String &name );
        ~ParticleSystemDef() override;

        uint32 getQuota() const;

        void init( VaoManager *vaoManager );

        void _destroy( VaoManager *vaoManager );

        bool isInitialized() const;

        const String getName() const { return mName; }

        void setParticleQuota( size_t quota ) override;

        /** If you know in advance how many times you will call addEmitter() call this
            function to achieve optimum memory use. This is the same as std::vector::reserve.
            Optional.
        @param numEmitters
            Number of times you intend to call addEmitter().
        */
        void reserveNumEmitters( size_t numEmitters );

        EmitterDefData *addEmitter( IdString name );

        const FastArray<EmitterDefData *> &getEmitters() const { return mEmitters; }

        size_t getNumEmitters() const;

        /// For internal use. Don't call this directly. See SceneManager::createParticleSystem2
        ParticleSystem2 *_createParticleSystem( ObjectMemoryManager *objectMemoryManager );

        /// For internal use. Don't call this directly. See SceneManager::destroyParticleSystem2
        void _destroyParticleSystem( ParticleSystem2 *system );

        /// For internal use. Don't call this directly. See SceneManager::destroyAllParticleSystems2
        void _destroyAllParticleSystems();

        ParticleCpuData getParticleCpuData() const { return mParticleCpuData; }

        /// Returns the number of active particles rounded up to match up SIMD processing.
        ///
        /// e.g.
        ///     - ARRAY_PACKED_REALS = 4
        ///     - mFirstParticleIdx = 3
        ///     - mLastParticleIdx = 6
        ///     - Num Active Particles = mLastParticleIdx - mFirstParticleIdx = 3
        ///     - Therefore getActiveParticlesPackOffset() = 0
        ///
        /// However we must process:
        ///     0. inactive
        ///     1. inactive
        ///     2. inactive
        ///     3. active
        ///
        ///     4. inactive
        ///     5. active
        ///     6. inactive
        ///     7. inactive
        ///
        /// That means the following code:
        ///
        /// @code
        ///     mParticleCpuData.advancePack( getActiveParticlesPackOffset() );
        ///     for( i = 0; i < getNumSimdActiveParticles(); i += ARRAY_PACKED_REALS )
        ///         mParticleCpuData.advancePack();
        /// @endcode
        ///
        /// Must iterate 2 times. However if getNumSimdActiveParticles() returns 3, it will
        /// only iterate once. Rounding 3 up to ARRAY_PACKED_REALS is also no good, because if it returns
        /// 4, it still iterates once.
        ///
        /// It must return 8 (ARRAY_PACKED_REALS * 2).
        ///
        /// Note that there can be a lot of inactive particles between mFirstParticleIdx &
        /// mLastParticleIdx. However this should be rare since particle FXs tend to follow FIFO.
        size_t getNumSimdActiveParticles() const
        {
            return ( ( mLastParticleIdx + ARRAY_PACKED_REALS - 1u ) / ARRAY_PACKED_REALS -
                     mFirstParticleIdx / ARRAY_PACKED_REALS ) *
                   ARRAY_PACKED_REALS;
        }
    };

    class _OgreExport ParticleSystem2 : public MovableObject
    {
        friend class ParticleSystemManager2;

        const ParticleSystemDef *mCreator;

        /// mEmitterInstanceData.size() == ParticleSystemDef::mEmitters.size()
        FastArray<EmitterInstanceData> mEmitterInstanceData;

        /// mNewParticlesPerEmitter.size() == ParticleSystemDef::mEmitters.size()
        FastArray<size_t> mNewParticlesPerEmitter;

        FastArray<uint32> mParticleHandles;

    public:
        /// See MovableObject::mGlobalIndex.
        /// This one tracks our place in ParticleSystemDef::mParticleSystems
        size_t mParentDefGlobalIdx;

        ParticleSystem2( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *manager,
                         const ParticleSystemDef *creator );

        const String &getMovableType() const override;

        const ParticleSystemDef *getParticleSystemDef() const { return mCreator; }

        const FastArray<uint32> &getParticleHandles() const { return mParticleHandles; }
    };

    /// Object for creating ParticleSystem2 instances
    class _OgreExport ParticleSystem2Factory final : public MovableObjectFactory
    {
    protected:
        MovableObject *createInstanceImpl( IdType id, ObjectMemoryManager *objectMemoryManager,
                                           SceneManager            *manager,
                                           const NameValuePairList *params = 0 ) override;

    public:
        ~ParticleSystem2Factory() override = default;

        static const String FACTORY_TYPE_NAME;

        const String &getType() const override;

        void destroyInstance( MovableObject *obj ) override;
    };

    OGRE_ASSUME_NONNULL_END
}  // namespace Ogre
#include "OgreHeaderSuffix.h"
