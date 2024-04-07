/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2024 Torus Knot Software Ltd

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

#include "OgreStableHeaders.h"

#include "ParticleSystem/OgreBillboard2.h"

#include "ParticleSystem/OgreBillboardSet2.h"

using namespace Ogre;

void Billboard::setVisible( const bool bVisible )
{
    ParticleCpuData cpuData = mBillboardSet->getParticleCpuData();
    reinterpret_cast<Real * RESTRICT_ALIAS>( cpuData.mTimeToLive )[mHandle] = bVisible ? 1.0f : 0.0f;
}
//-----------------------------------------------------------------------------
void Billboard::setPosition( const Vector3 &pos )
{
    ParticleCpuData cpuData = mBillboardSet->getParticleCpuData();

    const size_t j = mHandle / ARRAY_PACKED_REALS;
    const size_t idx = mHandle % ARRAY_PACKED_REALS;

    cpuData.mPosition[j].setFromVector3( pos, idx );
}
//-----------------------------------------------------------------------------
void Billboard::setDirection( const Vector3 &vDir )
{
    ParticleCpuData cpuData = mBillboardSet->getParticleCpuData();

    const size_t j = mHandle / ARRAY_PACKED_REALS;
    const size_t idx = mHandle % ARRAY_PACKED_REALS;

    cpuData.mDirection[j].setFromVector3( vDir, idx );
}
//-----------------------------------------------------------------------------
void Billboard::setDirection( const Vector3 &vDir, const Ogre::Radian rot )
{
    ParticleCpuData cpuData = mBillboardSet->getParticleCpuData();

    const size_t h = mHandle;
    const size_t j = h / ARRAY_PACKED_REALS;
    const size_t idx = h % ARRAY_PACKED_REALS;

    cpuData.mDirection[j].setFromVector3( vDir, idx );
    reinterpret_cast<Radian * RESTRICT_ALIAS>( cpuData.mRotation )[h] = rot;
}
//-----------------------------------------------------------------------------
void Billboard::setRotation( const Ogre::Radian rot )
{
    ParticleCpuData cpuData = mBillboardSet->getParticleCpuData();
    reinterpret_cast<Radian * RESTRICT_ALIAS>( cpuData.mRotation )[mHandle] = rot;
}
//-----------------------------------------------------------------------------
void Billboard::setDimensions( const Vector2 &dim )
{
    ParticleCpuData cpuData = mBillboardSet->getParticleCpuData();

    const size_t j = mHandle / ARRAY_PACKED_REALS;
    const size_t idx = mHandle % ARRAY_PACKED_REALS;

    cpuData.mDimensions[j].setFromVector2( dim, idx );
}
//-----------------------------------------------------------------------------
void Billboard::setColour( const ColourValue &colour )
{
    ParticleCpuData cpuData = mBillboardSet->getParticleCpuData();

    const size_t j = mHandle / ARRAY_PACKED_REALS;
    const size_t idx = mHandle % ARRAY_PACKED_REALS;

    cpuData.mColour[j].setFromVector4( colour.toVector4(), idx );
}
//-----------------------------------------------------------------------------
void Billboard::set( const Vector3 &pos, const Vector3 &dir, const Vector2 &dim,
                     const ColourValue &colour, const Ogre::Radian rot )
{
    ParticleCpuData cpuData = mBillboardSet->getParticleCpuData();

    const size_t h = mHandle;
    const size_t j = h / ARRAY_PACKED_REALS;
    const size_t idx = h % ARRAY_PACKED_REALS;

    cpuData.mPosition[j].setFromVector3( pos, idx );
    cpuData.mDirection[j].setFromVector3( dir, idx );
    cpuData.mDimensions[j].setFromVector2( dim, idx );
    cpuData.mColour[j].setFromVector4( colour.toVector4(), idx );
    reinterpret_cast<Radian * RESTRICT_ALIAS>( cpuData.mRotation )[h] = rot;
}
