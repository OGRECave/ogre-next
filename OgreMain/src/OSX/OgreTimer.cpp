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
#include "OgreTimer.h"
#include <sys/time.h>

using namespace Ogre;

//--------------------------------------------------------------------------------//
Timer::Timer()
{
    reset();
}

//--------------------------------------------------------------------------------//
Timer::~Timer()
{
}

//--------------------------------------------------------------------------------//
void Timer::reset()
{
    zeroClock = clock();
    gettimeofday( &start, NULL );
}

//--------------------------------------------------------------------------------//
uint64 Timer::getMilliseconds()
{
    struct timeval now;
    gettimeofday( &now, NULL );
    return static_cast<uint64>( now.tv_sec - start.tv_sec ) * 1000ul +
           static_cast<uint64>( ( now.tv_usec - start.tv_usec ) / 1000l );
}

//--------------------------------------------------------------------------------//
uint64 Timer::getMicroseconds()
{
    struct timeval now;
    gettimeofday( &now, NULL );
    return static_cast<uint64>( now.tv_sec - start.tv_sec ) * 1000000ul +
           static_cast<uint64>( now.tv_usec - start.tv_usec );
}

//-- Common Across All Timers ----------------------------------------------------//
uint64 Timer::getMillisecondsCPU()
{
    clock_t newClock = clock();
    return (uint64)( (float)( newClock - zeroClock ) / ( (float)CLOCKS_PER_SEC / 1000.0 ) );
}

//-- Common Across All Timers ----------------------------------------------------//
uint64 Timer::getMicrosecondsCPU()
{
    clock_t newClock = clock();
    return (uint64)( (float)( newClock - zeroClock ) / ( (float)CLOCKS_PER_SEC / 1000000.0 ) );
}
