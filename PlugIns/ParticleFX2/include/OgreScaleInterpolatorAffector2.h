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
#ifndef OgreScaleInterpolatorAffector2_H
#define OgreScaleInterpolatorAffector2_H

#include "OgreParticleFX2Prerequisites.h"

#include "ParticleSystem/OgreParticleAffector2.h"

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    class _OgreParticleFX2Export ScaleInterpolatorAffector2 : public ParticleAffector2
    {
    public:
        // this is something of a hack..
        // needs to be replaced with something more..
        // ..elegant
        enum
        {
            MAX_STAGES = 6
        };

    private:
        /** Command object for red adjust (see ParamCommand).*/
        class _OgrePrivate CmdScaleAdjust final : public ParamCommand
        {
        public:
            size_t mIndex;

        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Command object for red adjust (see ParamCommand).*/
        class _OgrePrivate CmdTimeAdjust final : public ParamCommand
        {
        public:
            size_t mIndex;

        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        static CmdScaleAdjust msScaleCmd[MAX_STAGES];
        static CmdTimeAdjust  msTimeCmd[MAX_STAGES];

    protected:
        ArrayReal mScaleAdj[MAX_STAGES];
        ArrayReal mTimeAdj[MAX_STAGES];

    public:
        ScaleInterpolatorAffector2();

        void run( ParticleCpuData cpuData, size_t numParticles, ArrayReal timeSinceLast ) const override;

        void setScaleAdjust( size_t index, Real scale );
        Real getScaleAdjust( size_t index ) const;

        void setTimeAdjust( size_t index, Real time );
        Real getTimeAdjust( size_t index ) const;

        void _cloneFrom( const ParticleAffector2 *original ) override;

        String getType() const override;
    };

    class _OgrePrivate ScaleInterpolatorAffectorFactory2 final : public ParticleAffectorFactory2
    {
        String getName() const override { return "ScaleInterpolator"; }

        ParticleAffector2 *createAffector() override
        {
            ParticleAffector2 *p = new ScaleInterpolatorAffector2();
            return p;
        }
    };

    OGRE_ASSUME_NONNULL_END
}  // namespace Ogre

#endif
