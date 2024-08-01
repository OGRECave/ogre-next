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

#ifndef OgreEmitter2_H
#define OgreEmitter2_H

#include "OgrePrerequisites.h"

#include "OgreParticleEmitter.h"
#include "ParticleSystem/OgreParticle2.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    class EmitterDefData;

    /// EmitterInstanceData are per ParticleSystem
    struct _OgreExport EmitterInstanceData
    {
        /// Don't set directly. Use setEnabled().
        bool mEnabled;
        Real mRemainder;
        Real mDurationRemain;
        Real mRepeatDelayRemain;
        Real mStartTime;

        void setEnabled( bool bEnabled, const EmitterDefData &emitter );
    };

    /// EmitterDefData are per ParticleSystemDef
    class _OgreExport EmitterDefData : protected ParticleEmitter
    {
    protected:
        friend struct EmitterInstanceData;

        Vector2 mDimensions;

    public:
        EmitterDefData();

        void setInitialDimensions( const Vector2 &dim );

        const Vector2 &getInitialDimensions() const { return mDimensions; }

        /// Override so we don't use it accidentally (we use genEmissionCount() instead).
        unsigned short _getEmissionCount( Real timeElapsed ) final;

        /** Must be called by 1 thread.
        @param timeSinceLast
        @param instanceData[in/out]
        @return
            Total number of particles to emit.
        */
        uint32 genEmissionCount( Real timeSinceLast, EmitterInstanceData &instanceData ) const;

        /** Initializes particles
            Can be called by multiple threads.
        @param cpuData
        @param newHandles
        @param numParticles
            Number of particles to initialize.
        */
        virtual void initEmittedParticles( ParticleCpuData cpuData, const EmittedParticle *newHandles,
                                           size_t numParticles ) = 0;

        virtual void _cloneFrom( const EmitterDefData *original );

        /// ParticleEmitter is a protected base class of EmitterDefData.
        ///
        /// This is because the new system was designed to be backwards compatible
        /// as much as possible with old the system, however it may not map 1:1.
        /// Thus in order to access the old interface, one must cast explicitly.
        ParticleEmitter *asParticleEmitter() { return this; }

        const ParticleEmitter *asParticleEmitter() const { return this; }
    };

    /** Abstract class defining the interface to be implemented by creators of EmitterDefData
    subclasses.
    @remarks
        Plugins or 3rd party applications can add new types of particle emitters to Ogre by creating
        subclasses of the EmitterDefData class. Because multiple instances of these emitters may be
        required, a factory class to manage the instances is also required.
    @par
        ParticleEmitterDefDataFactory subclasses must allow the creation and destruction of
        EmitterDefData subclasses. They must also be registered with the ParticleSystemManager2. All
        factories have a name which identifies them, examples might be 'point', 'cone', or 'box', and
        these can be also be used from particle system scripts.
    */
    class _OgreExport ParticleEmitterDefDataFactory
    {
    public:
        virtual ~ParticleEmitterDefDataFactory() = default;

        /// Returns the name of the factory, the name which identifies the particle emitter type this
        ///	factory creates.
        virtual const String &getName() const = 0;

        /// Creates a new emitter instance.
        virtual EmitterDefData *createEmitter() = 0;
    };

    OGRE_ASSUME_NONNULL_END
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
