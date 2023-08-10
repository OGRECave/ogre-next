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
#ifndef OgreAreaEmitter2_H
#define OgreAreaEmitter2_H

#include "OgreParticleFX2Prerequisites.h"

#include "ParticleSystem/OgreEmitter2.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    /** Particle emitter which emits particles randomly from points inside
        an area (box, sphere, ellipsoid whatever subclasses choose to be).
    @remarks
        This is an empty superclass and needs to be subclassed. Basic particle
        emitter emits particles from/in an (unspecified) area. The
        initial direction of these particles can either be a single direction
        (i.e. a line), a random scattering inside a cone, or a random
        scattering in all directions, depending the 'angle' parameter, which
        is the angle across which to scatter the particles either side of the
        base direction of the emitter.
    */
    class _OgreParticleFX2Export AreaEmitter2 : public EmitterDefData
    {
    private:
        /** Command object for area emitter size (see ParamCommand).*/
        class _OgrePrivate CmdWidth final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /** Command object for area emitter size (see ParamCommand).*/
        class _OgrePrivate CmdHeight final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /** Command object for area emitter size (see ParamCommand).*/
        class _OgrePrivate CmdDepth final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /// Command objects
        static CmdWidth  msWidthCmd;
        static CmdHeight msHeightCmd;
        static CmdDepth  msDepthCmd;

    protected:
        /// Size of the area
        Vector3 mSize;

        /// Local axes, not normalised, their magnitude reflects area size
        Vector3 mXRange, mYRange, mZRange;

        /// Internal method for generating the area axes
        void genAreaAxes();
        /** Internal for initializing some defaults and parameters
        @return True if custom parameters need initialising
        */
        bool initDefaults();

    public:
        /** Overloaded to update the trans. matrix */
        void setDirection( const Vector3 &direction ) override;

        /** Sets the size of the area from which particles are emitted.
        @param
            size Vector describing the size of the area. The area extends
            around the center point by half the x, y and z components of
            this vector. The box is aligned such that it's local Z axis points
            along it's direction (see setDirection)
        */
        void setSize( const Vector3 &size );

        /** Sets the size of the area from which particles are emitted.
        @param x,y,z
            Individual axis lengths describing the size of the area. The area
            extends around the center point by half the x, y and z components
            of this vector. The box is aligned such that it's local Z axis
            points along it's direction (see setDirection)
        */
        void setSize( Real x, Real y, Real z );

        /** Sets the width (local x size) of the emitter. */
        void setWidth( Real width );
        /** Gets the width (local x size) of the emitter. */
        Real getWidth() const;
        /** Sets the height (local y size) of the emitter. */
        void setHeight( Real Height );
        /** Gets the height (local y size) of the emitter. */
        Real getHeight() const;
        /** Sets the depth (local y size) of the emitter. */
        void setDepth( Real Depth );
        /** Gets the depth (local y size) of the emitter. */
        Real getDepth() const;

        void _cloneFrom( const EmitterDefData *original ) override;
    };

    OGRE_ASSUME_NONNULL_END
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
