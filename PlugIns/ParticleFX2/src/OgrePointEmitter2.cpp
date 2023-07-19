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

#include "OgrePointEmitter2.h"

#include "ParticleSystem/OgreParticle2.h"

using namespace Ogre;

#define TODO_colour

uint32 PointEmitter::genEmissionCount( const Real timeSinceLast,
                                       EmitterInstanceData &instanceData ) const
{
    if( instanceData.mEnabled )
    {
        // Keep fractions, otherwise a high frame rate will result in zero emissions!
        instanceData.mRemainder += mEmissionRate * timeSinceLast;
        const uint32 intRequest = (uint32)instanceData.mRemainder;
        instanceData.mRemainder -= (Real)intRequest;

        // Check duration
        if( mDurationMax != 0.0 )
        {
            instanceData.mDurationRemain -= timeSinceLast;
            if( mDurationRemain <= 0 )
            {
                // Disable, duration is out (takes effect next time)
                instanceData.setEnabled( false, *this );
            }
        }
        return intRequest;
    }
    else
    {
        // Check repeat
        if( mRepeatDelayMax != 0.0 )
        {
            instanceData.mRepeatDelayRemain -= timeSinceLast;
            if( instanceData.mRepeatDelayRemain <= 0 )
            {
                // Enable, repeat delay is out (takes effect next time)
                instanceData.setEnabled( true, *this );
            }
        }
        if( instanceData.mStartTime != 0.0 )
        {
            instanceData.mStartTime -= timeSinceLast;
            if( instanceData.mStartTime <= 0 )
            {
                instanceData.setEnabled( true, *this );
                instanceData.mStartTime = 0;
            }
        }

        return 0u;
    }
}
//-----------------------------------------------------------------------------
void PointEmitter::initEmittedParticles( ParticleCpuData cpuData, const uint32 *newHandles,
                                         size_t numParticles )
{
    for( size_t i = 0u; i < numParticles; ++i )
    {
        const size_t h = newHandles[i];
        const size_t j = h / ARRAY_PACKED_REALS;
        const size_t idx = h % ARRAY_PACKED_REALS;

        cpuData.mPosition[j].setFromVector3( mPosition, idx );
        reinterpret_cast<Real * RESTRICT_ALIAS>( cpuData.mRotation )[h] = 0.0f;

        Vector3 direction;
        this->genEmissionDirection( mPosition, direction );
        this->genEmissionVelocity( direction );
        cpuData.mDirection[j].setFromVector3( direction, idx );

        cpuData.mDimensions[j].setFromVector2( mDimensions, idx );

        TODO_colour;
        Ogre::ColourValue colour;
        this->genEmissionColour( colour );
        // cpuData.mColour[j].setFromVector4( colour, idx );

        reinterpret_cast<Real * RESTRICT_ALIAS>( cpuData.mTimeToLive )[h] =
            reinterpret_cast<Real * RESTRICT_ALIAS>( cpuData.mTotalTimeToLive )[h] =
                this->genEmissionTTL();
    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static const String kPointEmitterFactoryName = "point";
const String &PointEmitterFactory::getName() const
{
	return kPointEmitterFactoryName;
}
//-----------------------------------------------------------------------------
EmitterDefData *PointEmitterFactory::createEmitter()
{
	return new PointEmitter();
}
