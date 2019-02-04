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

#ifndef _OgreMetalStagingTexture_H_
#define _OgreMetalStagingTexture_H_

#include "OgreMetalPrerequisites.h"
#include "OgreStagingTextureBufferImpl.h"

#include "Vao/OgreMetalDynamicBuffer.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class _OgreMetalExport MetalStagingTexture : public StagingTextureBufferImpl
    {
        id<MTLBuffer>   mVboName;
        void            *mMappedPtr;
        MetalDevice     *mDevice;

        virtual bool belongsToUs( const TextureBox &box );
        virtual void* RESTRICT_ALIAS_RETURN mapRegionImplRawPtr(void);

    public:
        MetalStagingTexture( VaoManager *vaoManager, PixelFormatGpu formatFamily,
                             size_t sizeBytes, MetalDevice *device );
        virtual ~MetalStagingTexture();

        virtual void stopMapRegion(void);

        virtual void upload( const TextureBox &srcBox, TextureGpu *dstTexture,
                             uint8 mipLevel, const TextureBox *cpuSrcBox=0,
                             const TextureBox *dstBox=0, bool skipSysRamCopy=false );
    };
}

#include "OgreHeaderSuffix.h"

#endif
