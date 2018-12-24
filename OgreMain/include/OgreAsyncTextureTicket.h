/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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

#ifndef _OgreAsyncTextureTicket_H_
#define _OgreAsyncTextureTicket_H_

#include "OgrePrerequisites.h"
#include "OgreTextureGpu.h"
#include "OgreTextureGpuListener.h"
#include "OgreTextureBox.h"

namespace Ogre
{
    /** In Ogre 2.2 reading data from GPU back to CPU is asynchronous.
        See TextureGpuManager::createAsyncTextureTicket to generate a ticket.
        While the async transfer is being performed, you should be doing something else.
    @remarks
        If you call map() before the transfer is done, it will produce a stall as the
        CPU must wait for the GPU to finish all its pending operations.
    @par
        Use queryIsTransferDone to query if the transfer has finished. Beware not all
        APIs support querying async transfer status. In those cases there is no reliable
        way to determine when the transfer is done. An almost safe bet is to wait two
        frames before mapping.
    @par
        Call TextureGpuManager::destroyAsyncTextureTicket when you're done with this ticket.
    */
    class _OgreExport AsyncTextureTicket : public PassAlloc, public TextureGpuListener
    {
    public:
        enum Status
        {
            Ready,
            Downloading,
            Mapped
        };
        struct DelayedDownload
        {
            TextureGpu *textureSrc;
            uint8       mipLevel;
            bool        accurateTracking;
            bool        hasSrcBox;
            TextureBox  srcBox;

            DelayedDownload() :
                textureSrc( 0 ), mipLevel( 0 ), accurateTracking( false ),
                hasSrcBox( false ), srcBox() {}
            DelayedDownload( TextureGpu *_textureSrc, uint8 _mipLevel,
                             bool _accurateTracking, TextureBox *_srcBox ) :
                textureSrc( _textureSrc ),
                mipLevel( _mipLevel ),
                accurateTracking( _accurateTracking ),
                hasSrcBox( _srcBox != 0 ),
                srcBox( _srcBox ? *_srcBox : TextureBox() )
            {
            }
        };

    protected:
        Status  mStatus;
        uint32  mWidth;
        uint32  mHeight;
        uint32  mDepthOrSlices;
        TextureTypes::TextureTypes  mTextureType;
        PixelFormatGpu              mPixelFormatFamily;
        uint8   mNumInaccurateQueriesWasCalledInIssuingFrame;

        DelayedDownload     mDelayedDownload;

        virtual TextureBox mapImpl( uint32 slice ) = 0;
        virtual void unmapImpl(void) = 0;

        virtual void downloadFromGpu( TextureGpu *textureSrc, uint8 mipLevel,
                                      bool accurateTracking, TextureBox *srcBox=0 );

    public:
        AsyncTextureTicket( uint32 width, uint32 height, uint32 depthOrSlices,
                            TextureTypes::TextureTypes textureType,
                            PixelFormatGpu pixelFormatFamily );
        virtual ~AsyncTextureTicket();

        /// TextureGpuListener overload
        virtual void notifyTextureChanged( TextureGpu *texture,
                                           TextureGpuListener::Reason reason,
                                           void *extraData );

        /** Downloads textureSrc into this ticket.
            The size (resolution) of this ticket must match exactly of the region to download.
        @remarks
            The texture must either be Resident, or being streamed (i.e. scheduled to become Resident)
            If the texture isn't done streaming, the ticket will automatically schedule itself
            to perform the actual download once the streaming is done.
            If the Texture has a SystemRAM copy, it won't be used. You must access that directly,
            not via AsyncTextureTicket.
        @param textureSrc
            Texture to download from. Must be resident.
        @param mipLevel
            Mip level to download.
        @param accurateTracking
            When false, you will be mapping this texture much further along (i.e. after 2-3 frames)
            Useful when constantly streaming GPU content to the CPU with 3 frames delay.
            When true, we will accurately track the status of this transfer, which has higher
            driver overhead.
        @param srcBox
            When nullptr, we'll download the whole texture (its selected mip level)
            When not nullptr, we'll download the region within the texture.
            This region must resolution must match exactly that of this ticket (e.g.
            bytesPerRow may be much bigger than you expect!)
        @param bImmediate
            When bImmediate is true, we will download from the GPU if texture is currently
            resident and ready; even if there are pending residency changes.
            If it's not ready, we'll listen for when it is.
        */
        void download( TextureGpu *textureSrc, uint8 mipLevel,
                       bool accurateTracking, TextureBox *srcBox=0,
                       bool bImmediate=false );

        /** Maps the buffer for CPU access. Will stall if transfer from GPU memory to
            staging area hasn't finished yet. See queryIsTransferDone.
        @remarks
            Attempting to write to the returned pointer is undefined behavior.
            Be careful of TextureBox::bytesPerRow & bytesPerImage, when
            downloading a subregion of a texture, these values may not
            always be what you expect.
        @par
            NOTE: When mapping a texture bigger than 2048x2048; in D3D11 the
            the returned pointer will return TextureBox::numSlices = 1,
            In this case you will have to use that slice, unmap this
            ticket, and map it again with the next slice.
        @par
            See Image2::convertFromTexture for an example of how to use AyncTextureTicket
        @param slice
            First slice to map. Textures below 2048x2048 are guaranteed to
            be able to access [slice; getNumSlices).
            Textures above that will only be able to access [slice; slice+1)
        @return
            The pointer with the data read from the GPU. Read only.
        */
        TextureBox map( uint32 slice );

        /// Unmaps the pointer mapped with map().
        void unmap(void);

        /// See Image2::convertFromTexture for an example of how to use AyncTextureTicket
        virtual bool canMapMoreThanOneSlice(void) const     { return true; }

        uint32 getWidth(void) const;
        uint32 getHeight(void) const;
        uint32 getDepthOrSlices(void) const;
        /// For TypeCube & TypeCubeArray, this value returns 1.
        uint32 getDepth(void) const;
        /// For TypeCube this value returns 6.
        /// For TypeCubeArray, value returns numSlices * 6u.
        uint32 getNumSlices(void) const;
        PixelFormatGpu getPixelFormatFamily(void) const;

        size_t getBytesPerRow(void) const;
        size_t getBytesPerImage(void) const;

        virtual bool queryIsTransferDone(void);
    };
}

#endif
