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

#ifndef OgreParticleSystem2_H
#define OgreParticleSystem2_H

#include "OgrePrerequisites.h"

#include "OgreBitset.h"
#include "OgreMovableObject.h"
#include "OgreParticleSystem.h"
#include "ParticleSystem/OgreEmitter2.h"
#include "ParticleSystem/OgreParticle2.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    class ParticleAffector2;
    class EmitterDefData;
    struct EmitterInstanceData;
    class ParticleSystem2;

    static constexpr uint8 kParticleSystemDefaultRenderQueueId = 15u;

    namespace ParticleRotationType
    {
        enum ParticleRotationType
        {
            /// No rotation
            None,
            /// Rotate the particle's vertices around their facing direction
            Vertex,
            /// Rotate the particle's texture coordinates
            Texcoord
        };
    }

    /// A ParticleSystemDef does not form part of the scene.
    ///
    /// The reason it derives from ParticleSystem (& hence MovableObject) is to keep backwards
    /// compatibility with the interface of the older ParticleFX plugin for much easier
    /// migration/porting.
    ///
    /// Do NOT attach it to a SceneNode.
    ///
    /// For performance reasons, all particle system instances share the same RenderQueue ID.
    /// ParticleSystemDef must be cloned to use instances on another ID.
    class _OgreExport ParticleSystemDef : public ParticleSystem, protected Renderable
    {
    private:
        class _OgrePrivate CmdBillboardType final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
#if 0
        class _OgrePrivate CmdBillboardOrigin final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
#endif
        class _OgrePrivate CmdBillboardRotationType final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        class _OgrePrivate CmdCommonDirection final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        class _OgrePrivate CmdCommonUpVector final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        static CmdBillboardType msBillboardTypeCmd;
#if 0
        static CmdBillboardOrigin msBillboardOriginCmd;
#endif
        static CmdBillboardRotationType msBillboardRotationTypeCmd;
        static CmdCommonDirection       msCommonDirectionCmd;
        static CmdCommonUpVector        msCommonUpVectorCmd;

    public:
        static constexpr uint32 InvalidHandle = 0xFFFFFFFF;

    protected:
        friend class ParticleSystemManager2;

        String mName;

        ParticleSystemManager2 *mParticleSystemManager;

        /// Data common to all particles
        ConstBufferPacked *mGpuCommonData;
        /// Data for each individual particle set in an array
        ReadOnlyBufferPacked *mGpuData;

        Vector3 mCommonDirection;
        Vector3 mCommonUpVector;

        FastArray<EmitterDefData *>    mEmitters;
        FastArray<ParticleAffector2 *> mAffectors;
        FastArray<ParticleAffector2 *> mInitializableAffectors;

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

        bool mIsBillboardSet;

        ParticleRotationType::ParticleRotationType mRotationType;

        FastArray<ParticleSystem2 *> mParticleSystems;
        /// Contains ACTIVE particle systems to be processed this frame. Sorted by relevance/priority.
        FastArray<ParticleSystem2 *> mActiveParticleSystems;

        /// This is a "temporary" array used by ParticleSystemManager::updateSerial to store
        /// the newly created particles for ParticleSystemManager::update to process.
        FastArray<EmittedParticle> mNewParticles;

        /// One per thread.
        FastArray<FastArray<uint32>> mParticlesToKill;
        /// One per thread.
        FastArray<Aabb> mAabb;

        ParticleType::ParticleType mParticleType;

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

        /// Sorts all active particles in mParticleSystems by distance to camPos into
        /// mActiveParticleSystems so that closer to it have higher priority when requesting particles
        /// in a limited quota pool.
        void sortByDistanceTo( Vector3 camPos );

        void cloneTo( ParticleSystemDef *toClone );

    public:
        ParticleSystemDef( IdType id, ObjectMemoryManager *objectMemoryManager,
                           SceneManager *ogre_nullable manager,
                           ParticleSystemManager2 *particleSystemManager, const String &name,
                           const bool bIsBillboardSet );
        ~ParticleSystemDef() override;

        void setRenderer( const String &typeName ) override;

        uint32 getQuota() const;

        /** Clones this ParticleSystemDef with a new name into a new ParticleSystemManager2.
            Spawned particle instances are not cloned.

            This is useful if you need a different material or need to alter a few settings.
        @remarks
            This function can throw.

        @param newName
            New name. Must be unique in dstManager
        @param dstManager
            The ParticleSystemManager2 where it will be cloned into.
            If nullptr, it's the same ParticleSystemManager2 as the manager this definition belongs to.
        @return
            The cloned particle system definition.
        */
        ParticleSystemDef *clone( const String                         &newName,
                                  ParticleSystemManager2 *ogre_nullable dstManager = 0 );

        void init( VaoManager *vaoManager );

        void _destroy( VaoManager *ogre_nullable vaoManager );

        bool isInitialized() const;

        const String getName() const { return mName; }

        void setParticleQuota( size_t quota ) override;

        void setParticleType( ParticleType::ParticleType particleType );

        void setRotationType( ParticleRotationType::ParticleRotationType rotationType );

        ParticleRotationType::ParticleRotationType getRotationType() const;

        /// See setCommonDirection() and setCommonUpVector().
        /// Use this version if you have to often change both of these at the same time.
        void setCommonVectors( const Vector3 &commonDir, const Vector3 &commonUp );

        /** Use this to specify the common direction given to particles when type is
            ParticleType::OrientedCommon and ParticleType::PerpendicularCommon.
        @remarks
            Use ParticleType::OrientedCommon when you want oriented particles but you know they are
            always going to be oriented the same way (e.g. rain in calm weather). It is faster for the
            system to calculate the billboard vertices if they have a common direction.

            If you need to call both setCommonDirection() and setCommonUpVector() often,
            prefer using setCommonVectors().
        @param vec
            The direction for all particles.
        */
        void setCommonDirection( const Vector3 &vec );

        /// Returns the common direction for all particles. See setCommonDirection().
        const Vector3 &getCommonDirection() const;

        /** Use this to specify the common up-vector given to billboards of type
            ParticleType::PerpendicularCommon and ParticleType::PerpendicularSelf.
        @remarks
            Use ParticleType::PerpendicularSelf when you want oriented particles perpendicular to their
            own direction vector and don't face to camera. In this case, we need an additional vector
            to determine the billboard X, Y axis. The generated X axis perpendicular to both the own
            direction and up-vector, the Y axis will coplanar with both own direction and up-vector,
            and perpendicular to own direction.

            If you need to call both setCommonDirection() and setCommonUpVector() often,
            prefer using setCommonVectors().
        @param vec
            The up-vector for all particles.
        */
        void setCommonUpVector( const Vector3 &vec );

        /// Gets the common up-vector for all particles. See setCommonUpVector().
        const Vector3 &getCommonUpVector() const;

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

        /** If you know in advance how many times you will call addAffector() call this
            function to achieve optimum memory use. This is the same as std::vector::reserve.
            Optional.
        @param numAffectors
            Number of times you intend to call addAffector().
        */
        void reserveNumAffectors( size_t numAffectors );

        ParticleAffector2 *addAffector( IdString name );

        const FastArray<ParticleAffector2 *> &getAffectors() const { return mAffectors; }

        size_t getNumAffectors() const;

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
        /// Must iterate 2 times. However if getNumSimdActiveParticles() returns 3 (3 = 5 - 3 + 1),
        /// it will only iterate once. Rounding 3 up to ARRAY_PACKED_REALS is also no good, because if it
        /// returns 4, it still iterates once.
        ///
        /// It must return 8 (ARRAY_PACKED_REALS * 2).
        ///
        /// Note that there can be a lot of inactive particles between mFirstParticleIdx &
        /// mLastParticleIdx. However this should be rare since particle FXs tend to follow FIFO.
        size_t getNumSimdActiveParticles() const
        {
            size_t numSimdActiveParticles =
                ( ( mLastParticleIdx + ARRAY_PACKED_REALS - 1u ) / ARRAY_PACKED_REALS -
                  mFirstParticleIdx / ARRAY_PACKED_REALS ) *
                ARRAY_PACKED_REALS;

            // The previous formula can be off by one in some cases, e.g.
            //  getQuota = 12
            //  ARRAY_PACKED_REALS = 4
            //  mFirstParticleIdx = 1 -> rounds down to 0
            //  mLastParticleIdx = 13 -> rounds up to 16
            //  16 - 0 => 16 > 12
            // Thus we must ensure we never exceed the quota.
            numSimdActiveParticles = std::min<size_t>( numSimdActiveParticles, getQuota() );
            return numSimdActiveParticles;
        }

        /// Like getNumSimdActiveParticles(), we have to consider SIMD.
        ///
        /// However given the same example we only need to return 6, not 8.
        ///
        /// There's not much we can do about the waste at the beginning (in a API-compatible way),
        /// but we can do something about the last 2 vertices that are waste.
        ///
        /// This function is called "tighter" because it returns 6 instead of 8, but it is not fully
        /// "tight" because if that were the case, we'd return 3.
        size_t getParticlesToRenderTighter() const
        {
            size_t numSimdActiveParticles =
                mLastParticleIdx - ( ( mFirstParticleIdx / ARRAY_PACKED_REALS ) * ARRAY_PACKED_REALS );

            // The previous formula can be off by one in some cases, e.g.
            //  getQuota = 12
            //  ARRAY_PACKED_REALS = 4
            //  mFirstParticleIdx = 1 -> rounds down to 0
            //  mLastParticleIdx = 13 -> No rounding
            //  13 - 0 => 13 > 12
            // Thus we must ensure we never exceed the quota.
            numSimdActiveParticles = std::min<size_t>( numSimdActiveParticles, getQuota() );
            return numSimdActiveParticles;
        }

        inline static const ParticleSystemDef *castFromRenderable( const Renderable *a )
        {
            // OGRE_ASSERT_HIGH( dynamic_cast<const ParticleSystemDef *>( a ) );
            return static_cast<const ParticleSystemDef *>( a );
        }

        inline static Renderable *castToRenderable( ParticleSystemDef *a ) { return a; }

        ConstBufferPacked    *_getGpuCommonBuffer() const { return mGpuCommonData; }
        ReadOnlyBufferPacked *_getGpuDataBuffer() const { return mGpuData; }

        bool getUseIdentityWorldMatrix() const override { return true; }

        ParticleType::ParticleType getParticleType() const override;

        void _notifyAttached( Node *parent ) override;

        void getRenderOperation( v1::RenderOperation &op, bool casterPass ) override;
        void getWorldTransforms( Matrix4 *xform ) const override;

        const LightList &getLights() const override;
    };

    class _OgreExport ParticleSystem2 : public MovableObject
    {
        friend class ParticleSystemManager2;

        const ParticleSystemDef *mCreator;

        /// mEmitterInstanceData.size() == ParticleSystemDef::mEmitters.size()
        FastArray<EmitterInstanceData> mEmitterInstanceData;

        /// mNewParticlesPerEmitter.size() == ParticleSystemDef::mEmitters.size()
        FastArray<size_t> mNewParticlesPerEmitter;

    public:
        /// See MovableObject::mGlobalIndex.
        /// This one tracks our place in ParticleSystemDef::mParticleSystems
        size_t mParentDefGlobalIdx;

        ParticleSystem2( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *manager,
                         const ParticleSystemDef *creator );

        const String &getMovableType() const override;

        const ParticleSystemDef *getParticleSystemDef() const { return mCreator; }
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

#endif
