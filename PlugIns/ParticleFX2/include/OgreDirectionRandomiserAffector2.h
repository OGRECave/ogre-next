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
#ifndef OgreDirectionRandomiserAffector2_H
#define OgreDirectionRandomiserAffector2_H

#include "OgreParticleFX2Prerequisites.h"

#include "ParticleSystem/OgreParticleAffector2.h"

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    /** This class defines a ParticleAffector which applies randomness to the movement of the particles.
    @remarks
        This affector (see ParticleAffector) applies randomness to the movement of the particles by
        changing the direction vectors.
    @par
        The most important parameter to control the effect is randomness. It controls the range in which
    changes are applied to each axis of the direction vector. The parameter scope can be used to limit
    the effect to a certain percentage of the particles.
    */
    class _OgreParticleFX2Export DirectionRandomiserAffector2 : public ParticleAffector2
    {
    private:
        /** Command object for randomness (see ParamCommand).*/
        class _OgrePrivate CmdRandomness final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Command object for scope (see ParamCommand).*/
        class _OgrePrivate CmdScope final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Command object for keep_velocity (see ParamCommand).*/
        class _OgrePrivate CmdKeepVelocity final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /// Command objects
        static CmdRandomness   msRandomnessCmd;
        static CmdScope        msScopeCmd;
        static CmdKeepVelocity msKeepVelocityCmd;

    protected:
        Real mRandomness;
        Real mScope;
        bool mKeepVelocity;

    public:
        DirectionRandomiserAffector2();

        void run( ParticleCpuData cpuData, size_t numParticles, ArrayReal timeSinceLast ) const override;

        /// Sets the randomness to apply to the particles in a system.
        void setRandomness( Real force );
        /// Sets the scope (percentage of particles which are randomised).
        void setScope( Real force );
        /// Set flag which detemines whether particle speed is changed.
        void setKeepVelocity( bool keepVelocity );

        /// Gets the randomness to apply to the particles in a system.
        Real getRandomness() const;
        /// Gets the scope (percentage of particles which are randomised).
        Real getScope() const;
        /// Gets flag which detemines whether particle speed is changed.
        bool getKeepVelocity() const;

        void _cloneFrom( const ParticleAffector2 *original ) override;

        String getType() const override;
    };

    class _OgrePrivate DirectionRandomiserAffectorFactory2 final : public ParticleAffectorFactory2
    {
        String getName() const override { return "DirectionRandomiser"; }

        ParticleAffector2 *createAffector() override
        {
            ParticleAffector2 *p = new DirectionRandomiserAffector2();
            return p;
        }
    };

    OGRE_ASSUME_NONNULL_END
}  // namespace Ogre

#endif
