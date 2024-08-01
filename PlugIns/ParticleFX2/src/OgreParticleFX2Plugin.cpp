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

#include "OgreParticleFX2Plugin.h"

#include "OgreAbiUtils.h"
#include "OgreBoxEmitter2.h"
#include "OgreColourFaderAffector2FX2.h"
#include "OgreColourFaderAffectorFX2.h"
#include "OgreColourImageAffector2.h"
#include "OgreColourInterpolatorAffector2.h"
#include "OgreCylinderEmitter2.h"
#include "OgreDeflectorPlaneAffector2.h"
#include "OgreDirectionRandomiserAffector2.h"
#include "OgreEllipsoidEmitter2.h"
#include "OgreHlms.h"
#include "OgreHollowEllipsoidEmitter2.h"
#include "OgreLinearForceAffector2.h"
#include "OgrePointEmitter2.h"
#include "OgreRingEmitter2.h"
#include "OgreRotationAffector2.h"
#include "OgreScaleAffector2.h"
#include "OgreScaleInterpolatorAffector2.h"
#include "ParticleSystem/OgreParticleSystemManager2.h"

using namespace Ogre;

static const String sPluginName = "ParticleFX2";
//-----------------------------------------------------------------------------
const String &ParticleFX2Plugin::getName() const
{
    return sPluginName;
}
//-----------------------------------------------------------------------------
void ParticleFX2Plugin::install( const NameValuePairList * )
{
    Hlms::_setHasParticleFX2Plugin( true );

    // -- Create all new particle emitter factories --
    ParticleEmitterDefDataFactory *pEmitFact;

    // Emitters
    pEmitFact = new BoxEmitterFactory2();
    ParticleSystemManager2::addEmitterFactory( pEmitFact );
    mEmitterFactories.push_back( pEmitFact );
    pEmitFact = new CylinderEmitterFactory2();
    ParticleSystemManager2::addEmitterFactory( pEmitFact );
    mEmitterFactories.push_back( pEmitFact );
    pEmitFact = new EllipsoidEmitterFactory2();
    ParticleSystemManager2::addEmitterFactory( pEmitFact );
    mEmitterFactories.push_back( pEmitFact );
    pEmitFact = new HollowEllipsoidEmitterFactory2();
    ParticleSystemManager2::addEmitterFactory( pEmitFact );
    mEmitterFactories.push_back( pEmitFact );
    pEmitFact = new PointEmitterFactory2();
    ParticleSystemManager2::addEmitterFactory( pEmitFact );
    mEmitterFactories.push_back( pEmitFact );
    pEmitFact = new RingEmitterFactory2();
    ParticleSystemManager2::addEmitterFactory( pEmitFact );
    mEmitterFactories.push_back( pEmitFact );

    ParticleAffectorFactory2 *pAffectorFact;

    // Affectors
    pAffectorFact = new ColourFaderAffectorFX2Factory();
    ParticleSystemManager2::addAffectorFactory( pAffectorFact );
    mAffectorFactories.push_back( pAffectorFact );
    pAffectorFact = new ColourFaderAffector2FX2Factory();
    ParticleSystemManager2::addAffectorFactory( pAffectorFact );
    mAffectorFactories.push_back( pAffectorFact );
    pAffectorFact = new ColourImageAffectorFactory2();
    ParticleSystemManager2::addAffectorFactory( pAffectorFact );
    mAffectorFactories.push_back( pAffectorFact );
    pAffectorFact = new ColourInterpolatorAffectorFactory2();
    ParticleSystemManager2::addAffectorFactory( pAffectorFact );
    mAffectorFactories.push_back( pAffectorFact );
    pAffectorFact = new DeflectorPlaneAffectorFactory2();
    ParticleSystemManager2::addAffectorFactory( pAffectorFact );
    mAffectorFactories.push_back( pAffectorFact );
    pAffectorFact = new DirectionRandomiserAffectorFactory2();
    ParticleSystemManager2::addAffectorFactory( pAffectorFact );
    mAffectorFactories.push_back( pAffectorFact );
    pAffectorFact = new LinearForceAffectorFactory2();
    ParticleSystemManager2::addAffectorFactory( pAffectorFact );
    mAffectorFactories.push_back( pAffectorFact );
    pAffectorFact = new RotationAffectorFactory2();
    ParticleSystemManager2::addAffectorFactory( pAffectorFact );
    mAffectorFactories.push_back( pAffectorFact );
    pAffectorFact = new ScaleAffectorFactory2();
    ParticleSystemManager2::addAffectorFactory( pAffectorFact );
    mAffectorFactories.push_back( pAffectorFact );
    pAffectorFact = new ScaleInterpolatorAffectorFactory2();
    ParticleSystemManager2::addAffectorFactory( pAffectorFact );
    mAffectorFactories.push_back( pAffectorFact );
}
//-----------------------------------------------------------------------------
void ParticleFX2Plugin::initialise()
{
    // nothing to do
}
//-----------------------------------------------------------------------------
void ParticleFX2Plugin::shutdown()
{
    // nothing to do
}
//-----------------------------------------------------------------------------
void ParticleFX2Plugin::uninstall()
{
    // destroy
    for( ParticleAffectorFactory2 *affectorFactory : mAffectorFactories )
    {
        ParticleSystemManager2::removeAffectorFactory( affectorFactory );
        delete affectorFactory;
    }

    for( ParticleEmitterDefDataFactory *emitterFactory : mEmitterFactories )
    {
        ParticleSystemManager2::removeEmitterFactory( emitterFactory );
        delete emitterFactory;
    }

    Hlms::_setHasParticleFX2Plugin( false );
}
//-----------------------------------------------------------------------------
void ParticleFX2Plugin::getAbiCookie( AbiCookie &outAbiCookie )
{
    outAbiCookie = generateAbiCookie();
}
