/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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
#ifndef __DamperAffector_H__
#define __DamperAffector_H__

#include "OgreParticleFXPrerequisites.h"
#include "OgreParticleAffector.h"
#include "OgreVector3.h"


namespace Ogre {

    /** This class defines a ParticleAffector which decelerates particles in a system.
    @remarks
        This affector (see ParticleAffector) applies a damping force, such as fluid viscosity, to a particle system.
        The forces are applied in the direction opposite to the particle's velocity vector, until it comes to
        a complete stop.
    */
    class _OgreParticleFXExport DamperAffector : public ParticleAffector
    {
    public:
        /** Command object for constant damping parameter (see ParamCommand).*/
        class CmdConstantDamping : public ParamCommand
        {
        public:
            String doGet(const void* target) const;
            void doSet(void* target, const String& val);
        };

        /** Command object for linear damping parameter (see ParamCommand).*/
        class CmdLinearDamping : public ParamCommand
        {
        public:
            String doGet(const void* target) const;
            void doSet(void* target, const String& val);
        };
        
        /** Command object for quadratic damping parameter (see ParamCommand).*/
        class CmdQuadraticDamping : public ParamCommand
        {
        public:
            String doGet(const void* target) const;
            void doSet(void* target, const String& val);
        };

        /// Default constructor
        DamperAffector(ParticleSystem* psys);

        /** See ParticleAffector. */
        void _affectParticles(ParticleSystem* pSystem, Real timeElapsed);

        /** Sets the constant deceleration to apply to the particles in a system. */
        void setConstantDamping(Real d);

        /** Gets the constant deceleration to apply to the particles in a system. */
        Real getConstantDamping(void) const;
        
        /** Sets the linear deceleration to apply to the particles in a system. */
        void setLinearDamping(Real d);

        /** Gets the linear deceleration to apply to the particles in a system. */
        Real getLinearDamping(void) const;
        
        /** Sets the quadratic deceleration to apply to the particles in a system. */
        void setQuadraticDamping(Real d);

        /** Gets the quadratic deceleration to apply to the particles in a system. */
        Real getQuadraticDamping(void) const;

        /// Command objects
        static CmdConstantDamping  msConstantDampingCmd;
        static CmdLinearDamping    msLinearDampingCmd;
        static CmdQuadraticDamping msQuadraticDampingCmd;

    protected:
        /// Constant deceleration in m/s^2
        Real mConstantDamping;
        
        /// Deceleration factor which increases linearly with particle speed
        Real mLinearDamping;
        
        /// Deceleration factor which increases quadratically with particle speed
        Real mQuadraticDamping;
    };


}


#endif
