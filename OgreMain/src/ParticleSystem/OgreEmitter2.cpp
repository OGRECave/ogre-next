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

#include "ParticleSystem/OgreEmitter2.h"

#include "OgreException.h"

using namespace Ogre;

EmitterDefData::EmitterDefData() : ParticleEmitter( nullptr )
{
}
//-----------------------------------------------------------------------------
void EmitterInstanceData::setEnabled( bool bEnabled, const EmitterDefData &emitter )
{
    mEnabled = bEnabled;
    if( bEnabled )
    {
        if( emitter.mDurationMin == emitter.mDurationMax )
        {
            mDurationRemain = emitter.mDurationMin;
        }
        else
        {
            mDurationRemain = Math::RangeRandom( emitter.mDurationMin, emitter.mDurationMax );
        }
    }
    else
    {
        // Reset repeat
        if( emitter.mRepeatDelayMin == emitter.mRepeatDelayMax )
        {
            mRepeatDelayRemain = emitter.mRepeatDelayMin;
        }
        else
        {
            mRepeatDelayRemain = Math::RangeRandom( emitter.mRepeatDelayMax, emitter.mRepeatDelayMin );
        }
    }
}
//-----------------------------------------------------------------------------
unsigned short EmitterDefData::_getEmissionCount( Real )
{
    OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "", "" );
    return 0u;
}
