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

#include "OgreBoxEmitter2.h"

#include "ParticleSystem/OgreParticle2.h"

using namespace Ogre;

//-----------------------------------------------------------------------------
BoxEmitter2::BoxEmitter2()
{
    initDefaults( "Box" );
}
//-----------------------------------------------------------------------------
void BoxEmitter2::initEmittedParticles( ParticleCpuData cpuData, const uint32 *newHandles,
                                        size_t numParticles )
{
    for( size_t i = 0u; i < numParticles; ++i )
    {
        const size_t h = newHandles[i];
        const size_t j = h / ARRAY_PACKED_REALS;
        const size_t idx = h % ARRAY_PACKED_REALS;

        Vector3 xOff, yOff, zOff;

        xOff = Math::SymmetricRandom() * mXRange;
        yOff = Math::SymmetricRandom() * mYRange;
        zOff = Math::SymmetricRandom() * mZRange;

        cpuData.mPosition[j].setFromVector3( mPosition + xOff + yOff + zOff, idx );
        reinterpret_cast<Real * RESTRICT_ALIAS>( cpuData.mRotation )[h] = 0.0f;

        Vector3 direction;
        this->genEmissionDirection( mPosition, direction );
        this->genEmissionVelocity( direction );
        cpuData.mDirection[j].setFromVector3( direction, idx );

        cpuData.mDimensions[j].setFromVector2( mDimensions, idx );

        Ogre::ColourValue colour;
        this->genEmissionColour( colour );
        cpuData.mColour[j].setFromVector4( Vector4( colour.ptr() ), idx );

        reinterpret_cast<Real * RESTRICT_ALIAS>( cpuData.mTimeToLive )[h] =
            reinterpret_cast<Real * RESTRICT_ALIAS>( cpuData.mTotalTimeToLive )[h] =
                this->genEmissionTTL();
    }
}
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
static const String kBoxEmitterFactoryName = "box";
const String &BoxEmitterFactory2::getName() const
{
    return kBoxEmitterFactoryName;
}
//-----------------------------------------------------------------------------------
EmitterDefData *BoxEmitterFactory2::createEmitter()
{
    return new BoxEmitter2();
}
