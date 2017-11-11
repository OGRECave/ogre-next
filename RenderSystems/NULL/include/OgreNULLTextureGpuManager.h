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

#ifndef _OgreNULLTextureGpuManager_H_
#define _OgreNULLTextureGpuManager_H_

#include "OgreNULLPrerequisites.h"
#include "OgreTextureGpuManager.h"

#include "OgreTextureGpu.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */
    class _OgreNULLExport NULLTextureGpuManager : public TextureGpuManager
    {
    protected:
        virtual TextureGpu* createTextureImpl( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                               IdString name, uint32 textureFlags,
                                               TextureTypes::TextureTypes initialType );
        virtual StagingTexture* createStagingTextureImpl( uint32 width, uint32 height, uint32 depth,
                                                          uint32 slices, PixelFormatGpu pixelFormat );
        virtual void destroyStagingTextureImpl( StagingTexture *stagingTexture );

        virtual AsyncTextureTicket* createAsyncTextureTicketImpl( uint32 width, uint32 height,
                                                                  uint32 depthOrSlices,
                                                                  TextureTypes::TextureTypes textureType,
                                                                  PixelFormatGpu pixelFormatFamily );

    public:
        NULLTextureGpuManager( VaoManager *vaoManager, RenderSystem *renderSystem );
        virtual ~NULLTextureGpuManager();

        TextureGpu* createTextureGpuWindow(void);
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
