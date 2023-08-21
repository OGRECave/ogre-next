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

#ifndef OgreParticleAffector2_H
#define OgreParticleAffector2_H

#include "OgrePrerequisites.h"

#include "OgreStringInterface.h"
#include "ParticleSystem/OgreParticle2.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    /// Affectors are per ParticleSystemDef
    class _OgreExport ParticleAffector2 : public StringInterface
    {
    public:
        virtual void oneTimeInit() {}

        /// Returns true if initEmittedParticles() must be called on a particle that has been emitted.
        virtual bool needsInitialization() const { return false; }

        /// Returns true if ParticleSystemDef should default to something else
        /// other than ParticleRotationType::None.
        virtual bool wantsRotation() const { return false; }

        /** Initializes particles
            Can be called by multiple threads.

            @see EmitterDefData::initEmittedParticles
        @remarks
            If this is overloaded, then needsInitialization() must return true.
        */
        virtual void initEmittedParticles( ParticleCpuData /*cpuData*/,
                                           const EmittedParticle * /*newHandles*/,
                                           size_t /*numParticles*/ ) const
        {
        }

        virtual void run( ParticleCpuData cpuData, size_t numParticles,
                          ArrayReal timeSinceLast ) const = 0;

        virtual void _cloneFrom( const ParticleAffector2 *original ) = 0;

        /** Returns the name of the type of affector.
        @remarks
            This property is useful for determining the type of affector procedurally so another
            can be created.
        */
        virtual String getType() const = 0;
    };

    /** Abstract class defining the interface to be implemented by creators of ParticleAffector2
        subclasses.
    @remarks
        Plugins or 3rd party applications can add new types of particle affectors to Ogre by creating
        subclasses of the ParticleAffector2 class. Because multiple instances of these affectors may be
        required, a factory class to manage the instances is also required.
    @par
        ParticleAffector2Factory subclasses must allow the creation and destruction of ParticleAffector2
        subclasses. They must also be registered with the ParticleSystemManager. All factories have
        a name which identifies them, examples might be 'force_vector', 'attractor', or 'fader', and
        these can be also be used from particle system scripts.
    */
    class _OgreExport ParticleAffectorFactory2
    {
    public:
        virtual ~ParticleAffectorFactory2() = default;

        /// Returns the name of the factory, the name which identifies
        /// the particle affector type this factory creates.
        virtual String getName() const = 0;

        /// Creates a new affector instance.
        virtual ParticleAffector2 *createAffector() = 0;
    };

    OGRE_ASSUME_NONNULL_END
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
