/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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
#ifndef _OgreImageCodec2_H_
#define _OgreImageCodec2_H_

#include "OgreCodec.h"
#include "OgreTextureBox.h"
#include "OgreTextureGpu.h"

namespace Ogre {

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Image
    *  @{
    */
    /** Codec specialized in images.
        @remarks
            The users implementing subclasses of ImageCodec2 are required to return
            a valid pointer to a ImageData class from the decode(...) function.
    */
    class _OgreExport ImageCodec2 : public Codec
    {
    public:
        virtual ~ImageCodec2();
        /** Codec return class for images. Has information about the size and the
            pixel format of the image. */
        class _OgrePrivate ImageData2 : public Codec::CodecData
        {
        public:
            ImageData2() :
                textureType( TextureTypes::Unknown ),
                format( PFG_UNKNOWN ),
                numMipmaps( 1u ),
                freeOnDestruction( true )
            {
            }
            virtual ~ImageData2()
            {
                if( freeOnDestruction && box.data )
                {
                    OGRE_FREE_SIMD( box.data, MEMCATEGORY_RESOURCE );
                    box.data = 0;
                }
            }

            TextureBox                  box;
            TextureTypes::TextureTypes  textureType;
            PixelFormatGpu              format;
            uint8                       numMipmaps;
            bool                        freeOnDestruction;

        public:
            virtual String dataType() const
            {
                return "ImageData2";
            }
        };

    public:
        virtual String getDataType() const
        {
            return "ImageCodec2";
        }
    };

    /** @} */
    /** @} */
} // namespace

#endif
