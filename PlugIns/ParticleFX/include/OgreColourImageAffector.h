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
#ifndef __ColourImageAffector_H__
#define __ColourImageAffector_H__

#include "OgreParticleFXPrerequisites.h"

#include "OgreColourValue.h"
#include "OgreImage2.h"
#include "OgreParticleAffector.h"
#include "OgreStringInterface.h"

namespace Ogre
{
    class _OgreParticleFXExport ColourImageAffector : public ParticleAffector
    {
    public:
        /** Command object for red adjust (see ParamCommand).*/
        class CmdImageAdjust final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /** Default constructor. */
        ColourImageAffector( ParticleSystem *psys );

        /** See ParticleAffector. */
        void _initParticle( Particle *pParticle ) override;

        /** See ParticleAffector. */
        void _affectParticles( ParticleSystem *pSystem, Real timeElapsed ) override;

        void   setImageAdjust( String name );
        String getImageAdjust() const;

        static CmdImageAdjust msImageCmd;

    protected:
        Image2 mColourImage;
        bool   mColourImageLoaded;
        String mColourImageName;

        /** Internal method to load the image */
        void _loadImage();
    };

}  // namespace Ogre

#endif
