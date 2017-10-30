/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2017 Torus Knot Software Ltd

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
#include "OgreTextureGpuManagerListener.h"
#include "OgreTextureGpu.h"
#include "OgreBitwise.h"

namespace Ogre
{
	DefaultTextureGpuManagerListener::DefaultTextureGpuManagerListener() :
        mPackNonPow2( false )
	{
		mMinSlicesPerPool[0] = 16;
		mMinSlicesPerPool[1] = 8;
		mMinSlicesPerPool[2] = 4;
		mMinSlicesPerPool[3] = 2;
		mMaxResolutionToApplyMinSlices[0] = 256;
		mMaxResolutionToApplyMinSlices[1] = 512;
		mMaxResolutionToApplyMinSlices[2] = 1024;
		mMaxResolutionToApplyMinSlices[3] = 4096;
	}
	//-----------------------------------------------------------------------------------
    size_t DefaultTextureGpuManagerListener::getNumSlicesFor( TextureGpu *texture,
                                                              TextureGpuManager *textureManager )
	{
		uint32 maxResolution = std::max( texture->getWidth(), texture->getHeight() );
		uint16 minSlicesPerPool = 1u;

		for( int i=0; i<4; ++i )
		{
            if( maxResolution <= mMaxResolutionToApplyMinSlices[i] )
			{
                minSlicesPerPool = mMinSlicesPerPool[i];
				break;
			}
		}

		if( !mPackNonPow2 )
		{
		   if( !Bitwise::isPO2( texture->getWidth() ) || !Bitwise::isPO2( texture->getHeight() ) )
			   minSlicesPerPool = 1u;
		}

		return minSlicesPerPool;
	}
}
