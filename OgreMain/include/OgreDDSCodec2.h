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
#ifndef _OgreDDSCodec2_H_
#define _OgreDDSCodec2_H_

#include "OgreImageCodec2.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Image
     *  @{
     */

    // Forward declarations
    struct DXTColourBlock;
    struct DXTExplicitAlphaBlock;
    struct DXTInterpolatedAlphaBlock;

    /** Codec specialized in loading DDS (Direct Draw Surface) images.
    @remarks
        We implement our own codec here since we need to be able to keep DXT
        data compressed if the card supports it.
    */
    class _OgreExport DDSCodec2 : public ImageCodec2
    {
    private:
        String mType;

        static void flipEndian( void *pData, size_t size,
                                size_t count );  // invokes Bitwise::bswapChunks() if OGRE_ENDIAN_BIG
        static void flipEndian( void  *pData,
                                size_t size );  // invokes Bitwise::bswapBuffer() if OGRE_ENDIAN_BIG

        PixelFormatGpu convertFourCCFormat( uint32 fourcc ) const;
        PixelFormatGpu convertDXToOgreFormat( uint32 fourcc ) const;
        PixelFormatGpu convertPixelFormat( uint32 rgbBits, uint32 rMask, uint32 gMask, uint32 bMask,
                                           uint32 aMask, bool isSigned ) const;

        /// Unpack DXT colours into array of 16 colour values
        void unpackDXTColour( PixelFormatGpu pf, const DXTColourBlock &block, ColourValue *pCol ) const;
        /// Unpack DXT alphas into array of 16 colour values
        void unpackDXTAlpha( const DXTExplicitAlphaBlock &block, ColourValue *pCol ) const;
        /// Unpack DXT alphas into array of 16 colour values
        void unpackDXTAlpha( const DXTInterpolatedAlphaBlock &block, ColourValue *pCol ) const;

        /// Single registered codec instance
        static DDSCodec2 *msInstance;

    public:
        DDSCodec2();
        ~DDSCodec2() override {}

        /// @copydoc Codec::encode
        DataStreamPtr encode( MemoryDataStreamPtr &input, CodecDataPtr &pData ) const override;
        /// @copydoc Codec::encodeToFile
        void encodeToFile( MemoryDataStreamPtr &input, const String &outFileName,
                           CodecDataPtr &pData ) const override;
        /// @copydoc Codec::decode
        DecodeResult decode( DataStreamPtr &input ) const override;

        /// @copydoc Codec::magicNumberToFileExt
        String magicNumberToFileExt( const char *magicNumberPtr, size_t maxbytes ) const override;

        String getType() const override;

        /// Static method to startup and register the DDS codec
        static void startup();
        /// Static method to shutdown and unregister the DDS codec
        static void shutdown();
    };
    /** @} */
    /** @} */

}  // namespace Ogre

#endif
