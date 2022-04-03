/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-present Torus Knot Software Ltd

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

#ifndef _Ogre_VulkanBufferInterface_H_
#define _Ogre_VulkanBufferInterface_H_

#include "OgreVulkanPrerequisites.h"

#include "Vao/OgreBufferInterface.h"

namespace Ogre
{
    class _OgreVulkanExport VulkanBufferInterface final : public BufferInterface
    {
    protected:
        // clang-format off
        size_t  mVboPoolIdx;
        VkBuffer mVboName;
        void    *mMappedPtr;

        size_t  mUnmapTicket;
        VulkanDynamicBuffer *mDynamicBuffer;
        // clang-format on

        /** See BufferPacked::map.
        @param bAdvanceFrame
            When false, it doesn't really advance the mBuffer->mFinalBufferStart pointer,
            it just calculates the next index without advancing and returns its value.
            i.e. the value after advancing (was 0, would be incremented to 1, function returns 1).
        @return
            The 'next frame' index in the range [0; vaoManager->getDynamicBufferMultiplier())
            i.e. the value after advancing (was 0, gets incremented to 1, function returns 1).
        */
        size_t advanceFrame( bool bAdvanceFrame );

    public:
        VulkanBufferInterface( size_t vboPoolIdx, VkBuffer vboName, VulkanDynamicBuffer *dynamicBuffer );
        ~VulkanBufferInterface() override;

        // clang-format off
        size_t getVboPoolIndex()                { return mVboPoolIdx; }
        VkBuffer getVboName() const             { return mVboName; }
        // clang-format on

        void _setVboPoolIndex( size_t newVboPool ) { mVboPoolIdx = newVboPool; }

        /// Only use this function for the first upload
        void _firstUpload( void *data, size_t elementStart, size_t elementCount );

        void *RESTRICT_ALIAS_RETURN map( size_t elementStart, size_t elementCount,
                                         MappingState prevMappingState,
                                         bool advanceFrame = true ) override;
        void unmap( UnmapOptions unmapOption, size_t flushStartElem = 0,
                    size_t flushSizeElem = 0 ) override;
        void advanceFrame() override;
        void regressFrame() override;

        void copyTo( BufferInterface *dstBuffer, size_t dstOffsetBytes, size_t srcOffsetBytes,
                     size_t sizeBytes ) override;
        void *getVulkanDataPtr() { return mMappedPtr; };
    };
}  // namespace Ogre

#endif
