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
#ifndef OgreColourFaderAffector2FX2_H
#define OgreColourFaderAffector2FX2_H

#include "OgreParticleFX2Prerequisites.h"

#include "ParticleSystem/OgreParticleAffector2.h"

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    /** This plugin subclass of ParticleAffector allows you to alter the colour of particles.
    @remarks
        This class supplies the ParticleAffector implementation required to modify the colour of
        particle in mid-flight.
    */
    class _OgreParticleFX2Export ColourFaderAffector2FX2 : public ParticleAffector2
    {
    private:
        /** Command object for red adjust (see ParamCommand).*/
        class _OgrePrivate CmdRedAdjust1 final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Command object for green adjust (see ParamCommand).*/
        class _OgrePrivate CmdGreenAdjust1 final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Command object for blue adjust (see ParamCommand).*/
        class _OgrePrivate CmdBlueAdjust1 final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Command object for alpha adjust (see ParamCommand).*/
        class _OgrePrivate CmdAlphaAdjust1 final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Command object for red adjust (see ParamCommand).*/
        class _OgrePrivate CmdRedAdjust2 final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Command object for green adjust (see ParamCommand).*/
        class _OgrePrivate CmdGreenAdjust2 final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Command object for blue adjust (see ParamCommand).*/
        class _OgrePrivate CmdBlueAdjust2 final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Command object for alpha adjust (see ParamCommand).*/
        class _OgrePrivate CmdAlphaAdjust2 final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Command object for alpha adjust (see ParamCommand).*/
        class _OgrePrivate CmdStateChange final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Command object for setting min/max colour (see ParamCommand).*/
        class _OgrePrivate CmdMinColour final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Command object for setting min/max colour (see ParamCommand).*/
        class _OgrePrivate CmdMaxColour final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        static CmdRedAdjust1   msRedCmd1;
        static CmdRedAdjust2   msRedCmd2;
        static CmdGreenAdjust1 msGreenCmd1;
        static CmdGreenAdjust2 msGreenCmd2;
        static CmdBlueAdjust1  msBlueCmd1;
        static CmdBlueAdjust2  msBlueCmd2;
        static CmdAlphaAdjust1 msAlphaCmd1;
        static CmdAlphaAdjust2 msAlphaCmd2;
        static CmdStateChange  msStateCmd;
        static CmdMinColour    msMinColourCmd;
        static CmdMaxColour    msMaxColourCmd;

    protected:
        Vector4 mColourAdj1;
        Vector4 mColourAdj2;
        Vector4 mMinColour;
        Vector4 mMaxColour;
        Real    mStateChangeVal;

    public:
        ColourFaderAffector2FX2();

        void run( ParticleCpuData cpuData, size_t numParticles, ArrayReal timeSinceLast ) const override;

        /** Sets the minimum value to which the particles will be clamped against.
        @param rgba
            RGBA components stored in xyzw.
        */
        void setMinColour( const Vector4 &rgba );

        const Vector4 &getMinColour() const;

        /** Sets the maximum value to which the particles will be clamped against.
        @param rgba
            RGBA components stored in xyzw.
        */
        void setMaxColour( const Vector4 &rgba );

        const Vector4 &getMaxColour() const;

        /** Sets the colour adjustment to be made per second to particles.
        @param red, green, blue, alpha
            Sets the adjustment to be made to each of the colour components per second. These
            values will be added to the colour of all particles every second, scaled over each frame
            for a smooth adjustment.
        */
        void setAdjust1( float red, float green, float blue, float alpha = 0.0 );
        void setAdjust2( float red, float green, float blue, float alpha = 0.0 );
        /** Sets the red adjustment to be made per second to particles.
        @param red
            The adjustment to be made to the colour component per second. This
            value will be added to the colour of all particles every second, scaled over each frame
            for a smooth adjustment.
        */
        void setRedAdjust1( float red );
        void setRedAdjust2( float red );

        /// Gets the red adjustment to be made per second to particles. */
        float getRedAdjust1() const;
        float getRedAdjust2() const;

        /** Sets the green adjustment to be made per second to particles.
        @param green
            The adjustment to be made to the colour component per second. This
            value will be added to the colour of all particles every second, scaled over each frame
            for a smooth adjustment.
        */
        void setGreenAdjust1( float green );
        void setGreenAdjust2( float green );
        /// Gets the green adjustment to be made per second to particles. */
        float getGreenAdjust1() const;
        float getGreenAdjust2() const;
        /** Sets the blue adjustment to be made per second to particles.
        @param blue
            The adjustment to be made to the colour component per second. This
            value will be added to the colour of all particles every second, scaled over each frame
            for a smooth adjustment.
        */
        void setBlueAdjust1( float blue );
        void setBlueAdjust2( float blue );
        /// Gets the blue adjustment to be made per second to particles.
        float getBlueAdjust1() const;
        float getBlueAdjust2() const;

        /** Sets the alpha adjustment to be made per second to particles.
        @param alpha
            The adjustment to be made to the colour component per second. This
            value will be added to the colour of all particles every second, scaled over each frame
            for a smooth adjustment.
        */
        void setAlphaAdjust1( float alpha );
        void setAlphaAdjust2( float alpha );
        /// Gets the alpha adjustment to be made per second to particles.
        float getAlphaAdjust1() const;
        float getAlphaAdjust2() const;

        void setStateChange( Real NewValue );
        Real getStateChange() const;

        void _cloneFrom( const ParticleAffector2 *original ) override;

        String getType() const override;
    };

    class _OgrePrivate ColourFaderAffector2FX2Factory final : public ParticleAffectorFactory2
    {
        String getName() const override { return "ColourFader2"; }

        ParticleAffector2 *createAffector() override
        {
            ParticleAffector2 *p = new ColourFaderAffector2FX2();
            return p;
        }
    };

    OGRE_ASSUME_NONNULL_END
}  // namespace Ogre

#endif
