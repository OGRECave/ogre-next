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
#include "OgreEllipsoidEmitter2.h"

/* Implements an Emitter whose emitting points all lie inside an ellipsoid.
   See <http://mathworld.wolfram.com/Ellipsoid.html> for mathematical details.

  If the lengths of two axes of an ellipsoid are the same, the figure is
  called a 'spheroid' (depending on whether c < a or c > a, an 'oblate
  spheroid' or 'prolate spheroid', respectively), and if all three are the
  same, it is a 'sphere' (ball).
*/

using namespace Ogre;

//-----------------------------------------------------------------------------
EllipsoidEmitter2::EllipsoidEmitter2() { initDefaults(); }
//-----------------------------------------------------------------------------
void EllipsoidEmitter2::initEmittedParticles( ParticleCpuData cpuData, const EmittedParticle *newHandles,
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

        // First we create a random point inside a bounding sphere with a
        // radius of 1 (this is easy to do). The distance of the point from
        // 0,0,0 must be <= 1 (== 1 means on the surface and we count this as
        // inside, too).

        while( true )
        {
            // three random values for one random point in 3D space

            x = Math::SymmetricRandom();
            y = Math::SymmetricRandom();
            z = Math::SymmetricRandom();

            // the distance of x,y,z from 0,0,0 is sqrt(x*x+y*y+z*z), but
            // as usual we can omit the sqrt(), since sqrt(1) == 1 and we
            // use the 1 as boundary:
            if( x * x + y * y + z * z <= 1 )
            {
                break;  // found one valid point inside
            }
        }

        // scale the found point to the ellipsoid's size and move it
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
static const String kEllipsoidEmitterFactoryName = "Ellipsoid";
const String &EllipsoidEmitter2::getType() const { return kEllipsoidEmitterFactoryName; }
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
const String &EllipsoidEmitterFactory2::getName() const { return kEllipsoidEmitterFactoryName; }
//-----------------------------------------------------------------------------------------
EmitterDefData *EllipsoidEmitterFactory2::createEmitter() { return new EllipsoidEmitter2(); }
