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

#ifndef _OgreStagingTextureBufferImpl_H_
#define _OgreStagingTextureBufferImpl_H_

#include "OgreStagingTexture.h"
#include "OgreTextureBox.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    /** This implementation can be used by all RenderSystem APIs except D3D11,
        which is why this implementation is part of OgreMain
    */
    class _OgreExport StagingTextureBufferImpl : public StagingTexture
    {
    protected:
        size_t mInternalBufferStart;
        size_t mCurrentOffset;
        size_t mSize;
        size_t mVboPoolIdx;

        virtual DECL_MALLOC void* RESTRICT_ALIAS_RETURN mapRegionImplRawPtr(void) = 0;
        virtual TextureBox mapRegionImpl( uint32 width, uint32 height, uint32 depth, uint32 slices,
                                          PixelFormatGpu pixelFormat );

    public:
        StagingTextureBufferImpl( VaoManager *vaoManager, PixelFormatGpu formatFamily,
                                  size_t size, size_t internalBufferStart, size_t vboPoolIdx );
        virtual ~StagingTextureBufferImpl();

        virtual bool supportsFormat( uint32 width, uint32 height, uint32 depth, uint32 slices,
                                     PixelFormatGpu pixelFormat ) const;
        virtual bool isSmallerThan( const StagingTexture *other ) const;
        virtual size_t _getSizeBytes(void);

        /// @copydoc StagingTexture::notifyStartMapRegion
        virtual void startMapRegion(void);

        size_t _getInternalTotalSizeBytes(void) const   { return mSize; }
        size_t _getInternalBufferStart(void) const      { return mInternalBufferStart; }
        size_t getVboPoolIndex(void)                    { return mVboPoolIdx; }
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
