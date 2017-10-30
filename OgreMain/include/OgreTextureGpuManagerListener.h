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

#ifndef _OgreTextureGpuManagerListener_H_
#define _OgreTextureGpuManagerListener_H_

#include "OgrePrerequisites.h"
#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

	class _OgreExport TextureGpuManagerListener : public PassAlloc
    {
    public:
        virtual ~TextureGpuManagerListener() {}

		/** Ogre normally puts Textures into pools (a Type2DArray texture) for efficient rendering
			Note that only textures of the same resolution and format can be put together in
			the same pool. This creates two issues:
				* Unless it is known in advance, we do not know how large the array should be.
				  if we create a pool that can hold 64 entries but only 1 texture is actually
				  needed, then we waste a lot of GPU memory.
				* Large textures, such as 4096x4096 RGBA8 occupy a lot of memory 64MB.
				  These pools should not contain a large number of entries. For example
				  creating a pool of 8 entries of these textures means we'd be asking the
				  OS for 512MB of *contiguous* memory. Due to memory fragmentation, such
				  request is likely to fail and cause Out of Memory conditions.
		@param texture
			The first texture to which will be creating a pool based on its parameters
		@param textureManager
			The manager, in case you need more info.
		@return
			How many entries the pool should be able to hold.
		*/
		virtual size_t getNumSlicesFor( TextureGpu *texture, TextureGpuManager *textureManager ) = 0;
    };

	/** This is a Default implementation of TextureGpuManagerListener based on heuristics.
		These heuristics can be adjusted by modifying its member variables directly.
	*/
	class _OgreExport DefaultTextureGpuManagerListener : public TextureGpuManagerListener
	{
	public:
		/// Minimum slices per pool, regardless of maxBytesPerPool.
		/// It's also the starting num of slices. See mMaxResolutionToApplyMinSlices
		uint16 mMinSlicesPerPool[4];
		/// If texture resolution is <= maxResolutionToApplyMinSlices[i];
		/// we'll apply minSlicesPerPool[i]. Otherwise, we'll apply minSlicesPerPool[i+1]
		/// If resolution > mMaxResolutionToApplyMinSlices[N]; then minSlicesPerPool = 1;
		uint32 mMaxResolutionToApplyMinSlices[4];

		/// Whether non-power-of-2 textures should also be pooled, or we should return 1.
		bool mPackNonPow2;

		DefaultTextureGpuManagerListener();

		virtual size_t getNumSlicesFor( TextureGpu *texture, TextureGpuManager *textureManager );
	};

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
