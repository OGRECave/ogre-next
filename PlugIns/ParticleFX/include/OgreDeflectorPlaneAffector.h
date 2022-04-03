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
#ifndef __DeflectorPlaneAffector_H__
#define __DeflectorPlaneAffector_H__

#include "OgreParticleFXPrerequisites.h"

#include "OgreParticleAffector.h"
#include "OgreVector3.h"

namespace Ogre
{
    /** This class defines a ParticleAffector which deflects particles.
    @remarks
        This affector (see ParticleAffector) offers a simple (and inaccurate) physical deflection.
        All particles which hit the plane are reflected.
    @par
        The plane is defined by a point (plane_point) and the normal (plane_normal).
        In addition it is possible to change the strength of the recoil by using the bounce parameter.
    */
    class _OgreParticleFXExport DeflectorPlaneAffector : public ParticleAffector
    {
    public:
        /** Command object for plane point (see ParamCommand).*/
        class CmdPlanePoint final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Command object for plane normal (see ParamCommand).*/
        class CmdPlaneNormal final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Command object for bounce (see ParamCommand).*/
        class CmdBounce final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /// Default constructor
        DeflectorPlaneAffector( ParticleSystem *psys );

        /** See ParticleAffector. */
        void _affectParticles( ParticleSystem *pSystem, Real timeElapsed ) override;

        /** Sets the plane point of the deflector plane. */
        void setPlanePoint( const Vector3 &pos );

        /** Gets the plane point of the deflector plane. */
        Vector3 getPlanePoint() const;

        /** Sets the plane normal of the deflector plane. */
        void setPlaneNormal( const Vector3 &normal );

        /** Gets the plane normal of the deflector plane. */
        Vector3 getPlaneNormal() const;

        /** Sets the bounce value of the deflection. */
        void setBounce( Real bounce );

        /** Gets the bounce value of the deflection. */
        Real getBounce() const;

        /// Command objects
        static CmdPlanePoint  msPlanePointCmd;
        static CmdPlaneNormal msPlaneNormalCmd;
        static CmdBounce      msBounceCmd;

    protected:
        /// deflector plane point
        Vector3 mPlanePoint;
        /// deflector plane normal vector
        Vector3 mPlaneNormal;

        /// bounce factor (0.5 means 50 percent)
        Real mBounce;
    };

}  // namespace Ogre

#endif
