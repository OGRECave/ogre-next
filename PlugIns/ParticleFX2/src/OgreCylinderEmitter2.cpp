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
// Original author: Tels <http://bloodgate.com>, released as public domain
#include "OgreCylinderEmitter2.h"

// Implements an Emitter whose emitting points all lie inside a cylinder.

using namespace Ogre;

//-----------------------------------------------------------------------
CylinderEmitter2::CylinderEmitter2() : AreaEmitter2() { initDefaults(); }
//-----------------------------------------------------------------------
void CylinderEmitter2::initEmittedParticles( ParticleCpuData cpuData, const EmittedParticle *newHandles,
                                             const size_t numParticles )
{
    const Vector3 position = mPosition;
    const Vector2 dimensions = mDimensions;
    const Vector3 xRange = mXRange;
    const Vector3 yRange = mYRange;
    const Vector3 zRange = mZRange;

    for( size_t i = 0u; i < numParticles; ++i )
    {
        const size_t h = newHandles[i].handle;
        const size_t j = h / ARRAY_PACKED_REALS;
        const size_t idx = h % ARRAY_PACKED_REALS;

        Real x, y, z;

        // First we create a random point inside a bounding cylinder with a
        // radius and height of 1 (this is easy to do). The distance of the
        // point from 0,0,0 must be <= 1 (== 1 means on the surface and we
        // count this as inside, too).

        while( true )
        {
            /* ClearSpace not yet implemeted

            */
            // three random values for one random point in 3D space
            x = Math::SymmetricRandom();
            y = Math::SymmetricRandom();
            z = Math::SymmetricRandom();

            // the distance of x,y from 0,0 is sqrt(x*x+y*y), but
            // as usual we can omit the sqrt(), since sqrt(1) == 1 and we
            // use the 1 as boundary. z is not taken into account, since
            // all values in the z-direction are inside the cylinder:
            if( x * x + y * y <= 1 )
            {
                break;  // found one valid point inside
            }
        }

        // scale the found point to the cylinder's size and move it
        // relatively to the center of the emitter point
        const Vector3 localPos = position + x * xRange + y * yRange + z * zRange;
        cpuData.mPosition[j].setFromVector3( newHandles[i].pos + newHandles[i].rot * localPos, idx );
        reinterpret_cast<Real * RESTRICT_ALIAS>( cpuData.mRotation )[h] = 0.0f;

        Vector3 direction;
        this->genEmissionDirection( localPos, direction );
        this->genEmissionVelocity( direction );
        direction = newHandles[i].rot * direction;
        cpuData.mDirection[j].setFromVector3( direction, idx );

        cpuData.mDimensions[j].setFromVector2( dimensions, idx );

        Ogre::ColourValue colour;
        this->genEmissionColour( colour );
        cpuData.mColour[j].setFromVector4( Vector4( colour.ptr() ), idx );

        reinterpret_cast<Real * RESTRICT_ALIAS>( cpuData.mTimeToLive )[h] =
            reinterpret_cast<Real * RESTRICT_ALIAS>( cpuData.mTotalTimeToLive )[h] =
                this->genEmissionTTL();
    }
}
//-----------------------------------------------------------------------------------
static const String kCylinderEmitterFactoryName = "Cylinder";
const String &CylinderEmitter2::getType() const { return kCylinderEmitterFactoryName; }
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
const String &CylinderEmitterFactory2::getName() const { return kCylinderEmitterFactoryName; }
//-----------------------------------------------------------------------------------
EmitterDefData *CylinderEmitterFactory2::createEmitter() { return new CylinderEmitter2(); }
