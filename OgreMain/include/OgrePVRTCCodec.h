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
#ifndef __OgrePVRTCCodec_H__
#define __OgrePVRTCCodec_H__

#include "OgreImageCodec2.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Image
     *  @{
     */

    /** Codec specialized in loading PVRTC (PowerVR) images.
    @remarks
        We implement our own codec here since we need to be able to keep PVRTC
        data compressed if the card supports it.
    */
    class _OgreExport PVRTCCodec : public ImageCodec2
    {
    protected:
        String mType;

        static void flipEndian( void *pData, size_t size,
                                size_t count );  // invokes Bitwise::bswapChunks() if OGRE_ENDIAN_BIG
        static void flipEndian( void  *pData,
                                size_t size );  // invokes Bitwise::bswapBuffer() if OGRE_ENDIAN_BIG

        /// Single registered codec instance
        static PVRTCCodec *msInstance;

    public:
        PVRTCCodec();
        virtual ~PVRTCCodec() {}

        /// @copydoc Codec::encode
        DataStreamPtr encode( MemoryDataStreamPtr &input, CodecDataPtr &pData ) const;
        /// @copydoc Codec::encodeToFile
        void encodeToFile( MemoryDataStreamPtr &input, const String &outFileName,
                           CodecDataPtr &pData ) const;
        /// @copydoc Codec::decode
        DecodeResult decode( DataStreamPtr &input ) const;
        /// @copydoc Codec::magicNumberToFileExt
        String magicNumberToFileExt( const char *magicNumberPtr, size_t maxbytes ) const;

        virtual String getType() const;

        /// Static method to startup and register the PVRTC codec
        static void startup();
        /// Static method to shutdown and unregister the PVRTC codec
        static void shutdown();

    private:
        /// Decode PVRTCV2 image format
        DecodeResult decodeV2( DataStreamPtr &stream ) const;

        /// Decode PVRTCV3 image format
        DecodeResult decodeV3( DataStreamPtr &stream ) const;
    };
    /** @} */
    /** @} */

}  // namespace Ogre

#endif
