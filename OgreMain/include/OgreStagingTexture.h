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

#ifndef _OgreStagingTexture_H_
#define _OgreStagingTexture_H_

#include "OgreTextureGpu.h"
#include "OgrePixelFormatGpu.h"
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

    /** A StagingTexture is an intermediate texture that can be read and be written from both
        CPU & GPU.
        However the access in both cases is limited. GPUs can only copy (i.e. memcpy) to another
        real texture (can't be used directly as a texture or render target) and CPUs can only
        map it.
        In other words, a staging texture is an intermediate to transfer data between CPU & GPU
    @par
        How to write to 2 textures (texture0 & texture1; we pass twice the amount of slices
        to getStagingTexture):
        TextureGpuManager &textureManager = ...;
        StagingTexture *stagingTexture = textureManager.getStagingTexture( width, height, depth,
                                                                           slices * 2, pixelFormat );
        stagingTexture->startMapRegion();
        TextureBox box0 = stagingTexture->mapRegion( width, height, depth, slices, pixelFormat );
        ... write to box0.data ...
        TextureBox box1 = stagingTexture->mapRegion( width, height, depth, slices, pixelFormat );
        ... write to box1.data ...
        stagingTexture->stopMapRegion();
        stagingTexture->upload( box0, texture0 );
        stagingTexture->upload( box1, texture1 );
        textureManager.removeStagingTexture( stagingTexture );
        texture0->notifyDataIsReady();
        texture1->notifyDataIsReady();
    @par
        There are other possibilities, as you can request a StagingTexture with twice the
        width & height and then start calling mapRegion with smaller textures and we'll
        handle the packing. However you have to check mapRegion doesn't return nullptr
        in TextureBox::data. If so, that means we have ran out of space.
    @par
        Notably derived classes are:
            * StagingTextureBufferImpl
                * GL3PlusStagingTexture
                * MetalStagingTexture
            * D3D11StagingTexture
    */
    class _OgreExport StagingTexture : public RenderSysAlloc
    {
    protected:
        VaoManager  *mVaoManager;
        uint32      mLastFrameUsed;
        PixelFormatGpu mFormatFamily;

        bool        mMapRegionStarted;
#if OGRE_DEBUG_MODE
        bool        mUserQueriedIfUploadWillStall;
#endif

        virtual bool belongsToUs( const TextureBox &box ) = 0;
        virtual TextureBox mapRegionImpl( uint32 width, uint32 height, uint32 depth, uint32 slices,
                                          PixelFormatGpu pixelFormat ) = 0;

    public:
        StagingTexture( VaoManager *vaoManager, PixelFormatGpu formatFamily );
        virtual ~StagingTexture();

        /// D3D11 has restrictions about which StagingTextures can be uploaded to which textures
        /// based on texture families (for example all PFG_RGBA32_* belong to the same family).
        /// This function will return true if the StagingTexture can be used with the given format.
        /// On all the other RenderSystems, this nonsense does not exist thus it returns always
        /// true unless the request is so big it could never be fullfilled (it's larger than
        /// our maximum capacity)
        virtual bool supportsFormat( uint32 width, uint32 height, uint32 depth, uint32 slices,
                                     PixelFormatGpu pixelFormat ) const = 0;
        virtual bool isSmallerThan( const StagingTexture *other ) const = 0;

        /** Returns size in bytes. Note it's tagged as advanced use (via _underscore) because
            Just because a StagingTexture has enough available size, does not mean it can
            hold the data you'll want (a D3D11 StagingTexture of 256x512 cannot hold
            1024x1 texture data even though it has the available capacity)
        @return
            Size in bytes of this staging texture.
        */
        virtual size_t _getSizeBytes(void) = 0;

        /// Returns the format family it was requested. Note that in non-D3D11 RenderSystems,
        /// supportsFormat may return true despite a format not being from the same family.
        /// This information is mostly useful for keeping memory budgets consistent between
        /// different APIs (e.g. on D3D11 two StagingTextures, one that supports RGB8,
        /// another for BC1 of 64 MB each; on OpenGL we need to request two textures of 64MB
        /// each, and not just one because the first one can fulfill every request)
        PixelFormatGpu getFormatFamily(void) const                  { return mFormatFamily; }

        /// If it returns true, startMapRegion will stall.
        bool uploadWillStall(void);

        /** Must be called from main thread when the StagingBuffer is grabbed.
        @remarks
            Calling this function if you've already called upload() may stall.
            Grab another StagingTexture to prevent stall.
            See uploadWillStall.
        */
        virtual void startMapRegion(void);

        /** Can be called from worker thread, but not from multiple threads
            at the same time, also you can't call anything else either.
        @remarks
            You must have called startMapRegion before.
            Textures that are bigger than 2048x2048 can only map one slice at a time
            due to limitations in the D3D11 API.
        @return
            TextureBox to write to. Please note TextureBox::data may be null. If so, that
            means we don't have enough space to fulfill your request.
        */
        TextureBox mapRegion( uint32 width, uint32 height, uint32 depth, uint32 slices,
                              PixelFormatGpu pixelFormat );

        /// Must be called from main thread when the StagingBuffer is released.
        virtual void stopMapRegion(void);

        /** Uploads a region of data in srcBox (which must have been created with mapRegion)
            into dstTexture.
        @param srcBox
            The source data to copy from. Must have been created from mapRegion and must
            not have been altered (i.e. changed its internal variables)
            Values inside srcBox such as x, y, z & sliceStart will be ignored.
        @param dstTexture
            The destination texture. If dstBox is a null pointer, srcBox must match the
            texture dimensions exactly (x,y,z = 0; same resolution)
        @param cpuSrcBox
            A CPU-based copy that we can copy CPU -> CPU to our System RAM copy.
            This parameters must be present if skipSysRamCopy is false and
            the dstTexture strategy is GpuPageOutStrategy::AlwaysKeepSystemRamCopy
            or it is in OnSystemRam state.
        @param mipLevel
            Destination mipmap.
        @param dstBox
            Optional. Region inside dstTexture to copy to. Must have the same dimensions
            as srcBox.
            Values inside dstBox such as bytesPerRow, bytesPerImage & data will be ignored.
        @param skipSysRamCopy
            Whether to skip the copy to system RAM. Should only be used if the System RAM
            copy is already up to date, which is often the case when you're transitioning
            to Resident while loading at the same time.
            If misused, readbacks will be incorrect as data in CPU won't mirror that of
            the data in GPU, and possibly other bugs too.
        */
        virtual void upload( const TextureBox &srcBox, TextureGpu *dstTexture,
                             uint8 mipLevel, const TextureBox *cpuSrcBox=0,
                             const TextureBox *dstBox=0,
                             bool skipSysRamCopy=false );

        uint32 getLastFrameUsed(void) const             { return mLastFrameUsed; }
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
