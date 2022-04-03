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
#ifndef __ColourFaderAffector_H__
#define __ColourFaderAffector_H__

#include "OgreParticleFXPrerequisites.h"

#include "OgreParticleAffector.h"
#include "OgreStringInterface.h"

namespace Ogre
{
    /** This plugin subclass of ParticleAffector allows you to alter the colour of particles.
    @remarks
        This class supplies the ParticleAffector implementation required to modify the colour of
        particle in mid-flight.
    */
    class _OgreParticleFXExport ColourFaderAffector : public ParticleAffector
    {
    public:
        /** Command object for red adjust (see ParamCommand).*/
        class CmdRedAdjust final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Command object for green adjust (see ParamCommand).*/
        class CmdGreenAdjust final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Command object for blue adjust (see ParamCommand).*/
        class CmdBlueAdjust final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Command object for alpha adjust (see ParamCommand).*/
        class CmdAlphaAdjust final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Default constructor. */
        ColourFaderAffector( ParticleSystem *psys );

        /** See ParticleAffector. */
        void _affectParticles( ParticleSystem *pSystem, Real timeElapsed ) override;

        /** Sets the colour adjustment to be made per second to particles.
        @param red, green, blue, alpha
            Sets the adjustment to be made to each of the colour components per second. These
            values will be added to the colour of all particles every second, scaled over each frame
            for a smooth adjustment.
        */
        void setAdjust( float red, float green, float blue, float alpha = 0.0 );
        /** Sets the red adjustment to be made per second to particles.
        @param red
            The adjustment to be made to the colour component per second. This
            value will be added to the colour of all particles every second, scaled over each frame
            for a smooth adjustment.
        */
        void setRedAdjust( float red );

        /** Gets the red adjustment to be made per second to particles. */
        float getRedAdjust() const;

        /** Sets the green adjustment to be made per second to particles.
        @param green
            The adjustment to be made to the colour component per second. This
            value will be added to the colour of all particles every second, scaled over each frame
            for a smooth adjustment.
        */
        void setGreenAdjust( float green );
        /** Gets the green adjustment to be made per second to particles. */
        float getGreenAdjust() const;
        /** Sets the blue adjustment to be made per second to particles.
        @param blue
            The adjustment to be made to the colour component per second. This
            value will be added to the colour of all particles every second, scaled over each frame
            for a smooth adjustment.
        */
        void setBlueAdjust( float blue );
        /** Gets the blue adjustment to be made per second to particles. */
        float getBlueAdjust() const;

        /** Sets the alpha adjustment to be made per second to particles.
        @param alpha
            The adjustment to be made to the colour component per second. This
            value will be added to the colour of all particles every second, scaled over each frame
            for a smooth adjustment.
        */
        void setAlphaAdjust( float alpha );
        /** Gets the alpha adjustment to be made per second to particles. */
        float getAlphaAdjust() const;

        static CmdRedAdjust   msRedCmd;
        static CmdGreenAdjust msGreenCmd;
        static CmdBlueAdjust  msBlueCmd;
        static CmdAlphaAdjust msAlphaCmd;

    protected:
        float mRedAdj;
        float mGreenAdj;
        float mBlueAdj;
        float mAlphaAdj;

        /** Internal method for adjusting while clamping to [0,1] */
        inline void applyAdjustWithClamp( float *pComponent, float adjust )
        {
            *pComponent += adjust;
            // Limit to 0
            if( *pComponent < 0.0 )
            {
                *pComponent = 0.0f;
            }
            // Limit to 1
            else if( *pComponent > 1.0 )
            {
                *pComponent = 1.0f;
            }
        }
    };

}  // namespace Ogre

#endif
