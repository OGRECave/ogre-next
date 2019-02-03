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
#ifndef _OgreImage2_H_
#define _OgreImage2_H_

#include "OgrePrerequisites.h"
#include "OgreTextureGpu.h"
#include "OgreCommon.h"

namespace Ogre {
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Image
    *  @{
    */

    /** Class representing an image file.
    @remarks
        The Image class usually holds uncompressed image data and is the
        only object that can be loaded in a texture. Image objects handle
        image data decoding themselves by the means of locating the correct
        Codec object for each data type.
    @par
        Typically, you would want to use an Image object to load a texture
        when extra processing needs to be done on an image before it is
        loaded or when you want to blit to an existing texture.
    */
    class _OgreExport Image2 : public ImageAlloc
    {
    protected:
        /// The width of the image in pixels
        uint32  mWidth;
        uint32  mHeight;
        uint32  mDepthOrSlices;
        uint8   mNumMipmaps;

        /// The pixel format of the image
        TextureTypes::TextureTypes  mTextureType;
        PixelFormatGpu              mPixelFormat;

        void *mBuffer;

        /// A bool to determine if we delete the buffer or the calling app does
        bool mAutoDelete;

        void flipAroundY( uint8 mipLevel );
        void flipAroundX( uint8 mipLevel, void *pTempBuffer );

    public:
        Image2();
        /// Copy-constructor - copies all the data from the target image.
        Image2( const Image &img );
        virtual ~Image2();

        /// Assignment operator - copies all the data from the target image.
        Image2& operator = ( const Image2 & img );

        /** Flips (mirrors) the image around the Y-axis. 
            @remarks
                An example of an original and flipped image:
                <pre>                
                originalimg
                00000000000
                00000000000
                00000000000
                00000000000
                00000000000
                ------------> flip axis
                00000000000
                00000000000
                00000000000
                00000000000
                00000000000
                originalimg
                </pre>
        */
        void flipAroundY(void);

        /** Flips (mirrors) the image around the X-axis.
        @remarks
            An example of an original and flipped image:
            <pre>
                    flip axis
                        |
            originalimg|gmilanigiro
            00000000000|00000000000
            00000000000|00000000000
            00000000000|00000000000
            00000000000|00000000000
            00000000000|00000000000
            </pre>
        */
        void flipAroundX(void);

        /** Stores a pointer to raw data in memory. The pixel format has to be specified.
        @remarks
            This method loads an image into memory held in the object.
        @note
            Whilst typically your image is likely to be a simple 2D image,
            you can define complex images including cube maps, volume maps,
            and images including custom mip levels. The layout of the
            internal memory should be:
            <ul><li>face 0, mip 0 (top), width x height (x depth)</li>
            <li>face 0, mip 1, width/2 x height/2 (x depth/2)</li>
            <li>.. remaining mips for face 0 .. </li>
            <li>face 1, mip 0 (top), width x height (x depth)</li>
            <li>.. and so on. </li>
            </ul>
        @param data
            The data pointer. Must've been allocated with
            OGRE_MALLOC_SIMD( sizeBytes, MEMCATEGORY_RESOURCE );
            and sizeBytes assumes a pitch with row alignment = 4u;
            Use PixelFormatGpuUtils::getSizeBytes( width, 1u, 1u, 1u, pixelFormat, 4u );
            to get the pitch.
        @param width
            Width of image
        @param height
            Height of image
        @param depthOrSlices
            Image Depth.
            For 3D images, it's the depth.
            For cubemaps it must be 6.
            For cubemap arrays it must be 6 x num_arrays.
            For the rest it must be 1.
        @param textureType
            The type of the texture.
        @param format
            Pixel Format
        @param autoDelete
            If memory associated with this buffer is to be destroyed
            with the Image object. Note: it's important that if you set
            this option to true, that you allocated the memory using OGRE_MALLOC_SIMD
            with a category of MEMCATEGORY_RESOURCE to ensure the freeing of memory
            matches up.
        @param numMipmaps
            The number of mipmaps the image data has inside. A value of 0 is invalid.
        @note
             The memory associated with this buffer is NOT destroyed with the
             Image object, unless autoDelete is set to true.
         */
        void loadDynamicImage( void *pData, uint32 width, uint32 height, uint32 depthOrSlices,
                               TextureTypes::TextureTypes textureType, PixelFormatGpu format,
                               bool autoDelete, uint8 numMipmaps=1u );

        /// Convenience function. Same as Image2::loadDynamicImage, but retrieves
        /// all metadata parameters from the input texture
        void loadDynamicImage( void *pData, bool autoDelete, const TextureGpu *texture );
        void loadDynamicImage( void *pData, bool autoDelete, const Image2 *image );

        /** Synchronously downloads the selected mips from a TextureGpu into this Image.
            This function is for convenience for when going async is not important.
        @param minMip
            First mipmap to download, inclusive.
        @param maxMip
            Last mipmap to download, inclusive.
        @param automaticResolve
            When true, we will take care of resolving explicit MSAA textures if necessary,
            so that the download from GPU works fine.
        */
        void convertFromTexture( TextureGpu *texture, uint8 minMip, uint8 maxMip,
                                 bool automaticResolve=true );

        /** Synchronously uploads the selected mips from this Image into a TextureGpu.
            This function is for convenience for when going async is not important.
        @param texture
            Texture to upload to. Must have the same resolution and
            same pixel format family as this Image.
            For simplicity, you can't upload mip 0 of this image into mip 1.
        @param minMip
            First mipmap to upload, inclusive.
        @param maxMip
            Last mipmap to upload, inclusive.
        @param dstZorSliceStart
            Destination offset in the texture
            (e.g. when dstZorSliceStart = 5, it uploads N slices between [5; 5+N) )
        @param srcDepthOrSlices
            How many slices from this Image2 to upload. Zero to upload all of them.
        */
        void uploadTo( TextureGpu *texture, uint8 minMip, uint8 maxMip,
                       uint32 dstZorSliceStart=0u, uint32 srcDepthOrSlices=0u );

        /** Loads an image file.
            @remarks
                This method loads an image into memory. Any format for which 
                an associated ImageCodec is registered can be loaded. 
                This can include complex formats like DDS with embedded custom 
                mipmaps, cube faces and volume textures.
                The type can be determined by calling getFormat().             
            @param
                filename Name of an image file to load.
            @param
                groupName Name of the resource group to search for the image
            @note
                The memory associated with this buffer is destroyed with the
                Image object.
        */
        void load( const String& filename, const String& groupName );

        /** Loads an image file from a stream.
            @remarks
                This method works in the same way as the filename-based load 
                method except it loads the image from a DataStream object. 
                This DataStream is expected to contain the 
                encoded data as it would be held in a file. 
                Any format for which an associated ImageCodec is registered 
                can be loaded. 
                This can include complex formats like DDS with embedded custom 
                mipmaps, cube faces and volume textures.
                The type can be determined by calling getFormat().             
            @param
                stream The source data.
            @param
                type The type of the image. Used to decide what decompression
                codec to use. Can be left blank if the stream data includes
                a header to identify the data.
            @see
                Image::load( const String& filename )
        */
        void load( DataStreamPtr& stream, const String& type = BLANKSTRING );
        
        /** Save the image as a file. 
        @remarks
            Saving and loading are implemented by back end (sometimes third 
            party) codecs.  Implemented saving functionality is more limited
            than loading in some cases. Particularly DDS file format support 
            is currently limited to true colour or single channel float32, 
            square, power of two textures with no mipmaps.  Volumetric support
            is currently limited to DDS files.
        */
        void save( const String& filename, uint8 mipLevel, uint8 numMipmaps );

        /** Encode the image and return a stream to the data. 
            @param formatextension An extension to identify the image format
                to encode into, e.g. "jpg" or "png"
        */
        DataStreamPtr encode( const String& formatextension, uint8 mipLevel, uint8 numMipmaps );

        /** Get colour value from a certain location in the image.
        @remarks
            This function is slow as we need to calculate the mipmap offset every time.
            If you need to call it often, prefer using Image2::getData and then call
            TextureBox::getColourAt instead.
        */
        ColourValue getColourAt( size_t x, size_t y, size_t z, uint8 mipLevel=0 ) const;

        /** Set colour value at a certain location in the image.
        @remarks
            This function is slow as we need to calculate the mipmap offset every time.
            If you need to call it often, prefer using Image2::getData and then call
            TextureBox::setColourAt instead.
        */
        void setColourAt( const ColourValue &cv, size_t x, size_t y, size_t z, uint8 mipLevel=0 );

        void* getRawBuffer(void)    { return mBuffer; }

        /// Returns a pointer to the internal image buffer.
        TextureBox getData( uint8 mipLevel ) const;

        uint32 getWidth(void) const;
        uint32 getHeight(void) const;
        uint32 getDepthOrSlices(void) const;
        /// For TypeCube & TypeCubeArray, this value returns 1.
        uint32 getDepth(void) const;
        /// For TypeCube this value returns 6.
        /// For TypeCubeArray, value returns numSlices * 6u.
        uint32 getNumSlices(void) const;
        uint8 getNumMipmaps(void) const;
        TextureTypes::TextureTypes getTextureType(void) const;
        PixelFormatGpu getPixelFormat(void) const;

        size_t getBytesPerRow( uint8 mipLevel ) const;
        size_t getBytesPerImage( uint8 mipLevel ) const;
        /// Returns total size in bytes used in GPU by this texture including mipmaps.
        size_t getSizeBytes(void) const;

        /// Delete all the memory held by this image, if owned by this image (not dynamic)
        void freeMemory();

        enum Filter
        {
            FILTER_NEAREST,
            FILTER_LINEAR,
            FILTER_BILINEAR,
            FILTER_BOX,
            FILTER_TRIANGLE,
            FILTER_BICUBIC,
            /// Applies gaussian filter over the image, then a point sampling reduction
            /// This is done at the same time (i.e. we don't blur pixels we ignore).
            FILTER_GAUSSIAN,
            /// Applies gaussian filter over the image, then bilinear downsamples.
            /// This prevents certain artifacts for some images when using FILTER_GAUSSIAN,
            /// like biasing towards certain direction. Not supported by cubemaps.
            FILTER_GAUSSIAN_HIGH,
        };

#if 0
        /** Scale a 1D, 2D or 3D image volume. 
            @param  src         PixelBox containing the source pointer, dimensions and format
            @param  dst         PixelBox containing the destination pointer, dimensions and format
            @param  filter      Which filter to use
            @remarks    This function can do pixel format conversion in the process.
            @note   dst and src can point to the same PixelBox object without any problem
        */
        static void scale(const PixelBox &src, const PixelBox &dst, Filter filter = FILTER_BILINEAR);
        
        /** Resize a 2D image, applying the appropriate filter. */
        void resize( uint32 width, uint32 height, Filter filter = FILTER_BILINEAR );
#endif

        /** Sets the proper downsampler functions to generate mipmaps
        @param format
        @param imageDownsampler2D [out]
        @param imageDownsamplerCube [out]
        @param imageBlur2D [out]
        @param gammaCorrected
            When true, force gamma correction on.
            If this value is false but format is of the sRGB family,
            gamma correction will still be used.
        @param depthOrSlices
            Required to properly calculate the return value
        @param textureType
            Required to properly calculate the return value
        @param filter
        @return
            True if mipmaps can be generated.
            False if mipmaps cannot be generated.
        */
        static bool getDownsamplerFunctions( PixelFormatGpu format,
                                             void **imageDownsampler2D,
                                             void **imageDownsamplerCube,
                                             void **imageBlur2D, bool gammaCorrected,
                                             uint32 depthOrSlices,
                                             TextureTypes::TextureTypes textureType,
                                             Filter filter );

        static bool supportsSwMipmaps( PixelFormatGpu format, uint32 depthOrSlices,
                                       TextureTypes::TextureTypes textureType,
                                       Filter filter );

        /** Generates the mipmaps for this image. For Cubemaps, the filtering is seamless; and a
            gaussian filter is recommended although it's slow.
        @remarks
            Cannot handle compressed formats.
            Gaussian filter is implemented with a generic 1-pass convolution matrix, which in
            turn means it is O( N^N ) instead of a 2-pass filter which is O( 2^N ); where
            N is the number of taps. The Gaussian filter is 5x5
        @param gammaCorrected
            True if the filter should be applied in linear space.
        @param filter
            The type of filter to use.
        @return
            False if failed to generate and mipmaps properties won't be changed. True on success.
        */
        bool generateMipmaps( bool gammaCorrected, Filter filter = FILTER_BILINEAR );

        /// Static function to get an image type string from a stream via magic numbers
        static String getFileExtFromMagic( DataStreamPtr &stream );

        void _setAutoDelete( bool autoDelete );
        bool getAutoDelete(void) const;
    };

    /** @} */
    /** @} */

} // namespace

#endif
