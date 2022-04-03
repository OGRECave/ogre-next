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
#ifndef _OgreFreeImageCodec2_H_
#define _OgreFreeImageCodec2_H_

#include "OgreImageCodec2.h"

#include "ogrestd/list.h"

// Forward-declaration to avoid external dependency on FreeImage
struct FIBITMAP;

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Image
     *  @{
     */
    /** Codec specialized in images loaded using FreeImage.
        @remarks
            The users implementing subclasses of ImageCodec are required to return
            a valid pointer to a ImageData class from the decode(...) function.
    */
    class _OgreExport FreeImageCodec2 : public ImageCodec2
    {
    private:
        String       mType;
        unsigned int mFreeImageType;

        typedef list<ImageCodec2 *>::type RegisteredCodecList;
        static RegisteredCodecList        msCodecList;

    public:
        FreeImageCodec2( const String &type, unsigned int fiType );
        ~FreeImageCodec2() override {}

        /** Common encoding routine. */
        FIBITMAP *encodeBitmap( MemoryDataStreamPtr &input, CodecDataPtr &pData ) const;
        /// @copydoc Codec::encode
        DataStreamPtr encode( MemoryDataStreamPtr &input, CodecDataPtr &pData ) const override;
        /// @copydoc Codec::encodeToFile
        void encodeToFile( MemoryDataStreamPtr &input, const String &outFileName,
                           CodecDataPtr &pData ) const override;
        /// @copydoc Codec::decode
        DecodeResult decode( DataStreamPtr &input ) const override;

        virtual String getType() const override;

        /// @copydoc Codec::magicNumberToFileExt
        String magicNumberToFileExt( const char *magicNumberPtr, size_t maxbytes ) const override;

        /// Static method to startup FreeImage and register the FreeImage codecs
        static void startup();
        /// Static method to shutdown FreeImage and unregister the FreeImage codecs
        static void shutdown();
    };
    /** @} */
    /** @} */

}  // namespace Ogre

#endif
