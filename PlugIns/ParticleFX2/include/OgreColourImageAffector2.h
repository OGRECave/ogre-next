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
#ifndef OgreColourImageAffector2_H
#define OgreColourImageAffector2_H

#include "OgreParticleFX2Prerequisites.h"

#include "ParticleSystem/OgreParticleAffector2.h"

#include "OgreImage2.h"

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    class _OgreParticleFX2Export ColourImageAffector2 : public ParticleAffector2
    {
    private:
        /** Command object for red adjust (see ParamCommand).*/
        class _OgrePrivate CmdImageAdjust final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        static CmdImageAdjust msImageCmd;

    protected:
        FastArray<Vector4> mColourData;
        String             mColourImageName;
        bool               mInitialized;

    public:
        ColourImageAffector2();

        void run( ParticleCpuData cpuData, size_t numParticles, ArrayReal timeSinceLast ) const override;

        void oneTimeInit() override;

        void setImageAdjust( const String &name );

        const String &getImageAdjust() const;

        void _cloneFrom( const ParticleAffector2 *original ) override;

        String getType() const override;
    };

    class _OgrePrivate ColourImageAffectorFactory2 final : public ParticleAffectorFactory2
    {
        String getName() const override { return "ColourImage"; }

        ParticleAffector2 *createAffector() override
        {
            ParticleAffector2 *p = new ColourImageAffector2();
            return p;
        }
    };
    OGRE_ASSUME_NONNULL_END
}  // namespace Ogre

#endif
