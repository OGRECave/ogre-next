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

#ifndef _OgreGL3PlusStagingTexture_H_
#define _OgreGL3PlusStagingTexture_H_

#include "OgreGL3PlusPrerequisites.h"
#include "OgreStagingTextureBufferImpl.h"

#include "Vao/OgreGL3PlusDynamicBuffer.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class _OgreGL3PlusExport GL3PlusStagingTexture : public StagingTextureBufferImpl
    {
        GL3PlusDynamicBuffer    *mDynamicBuffer;
        size_t                  mUnmapTicket;
        void                    *mMappedPtr;
        void                    *mLastMappedPtr;

        virtual bool belongsToUs( const TextureBox &box );
        virtual void* RESTRICT_ALIAS_RETURN mapRegionImplRawPtr(void);

        void uploadCubemap( const TextureBox &srcBox, PixelFormatGpu pixelFormat,
                            uint8 mipLevel, GLenum format, GLenum type,
                            GLint xPos, GLint yPos, GLint slicePos,
                            GLsizei width, GLsizei height, GLsizei numSlices );

    public:
        GL3PlusStagingTexture( VaoManager *vaoManager, PixelFormatGpu formatFamily,
                               size_t size, size_t internalBufferStart, size_t vboPoolIdx,
                               GL3PlusDynamicBuffer *dynamicBuffer );
        virtual ~GL3PlusStagingTexture();

        void _unmapBuffer(void);

        virtual void startMapRegion(void);
        virtual void stopMapRegion(void);

        virtual void upload( const TextureBox &srcBox, TextureGpu *dstTexture,
                             uint8 mipLevel, const TextureBox *cpuSrcBox=0,
                             const TextureBox *dstBox=0, bool skipSysRamCopy=false );

        GL3PlusDynamicBuffer* _getDynamicBuffer(void)           { return mDynamicBuffer; }
        void _resetDynamicBuffer(void)                          { mDynamicBuffer = 0; }
    };
}

#include "OgreHeaderSuffix.h"

#endif
