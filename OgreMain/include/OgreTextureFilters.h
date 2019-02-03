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

#ifndef _OgreTextureGpuFilter_H_
#define _OgreTextureGpuFilter_H_

#include "OgrePrerequisites.h"
#include "OgrePixelFormatGpu.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */
    namespace TextureFilter
    {
        class FilterBase;
    }
    typedef FastArray<TextureFilter::FilterBase*> FilterBaseArray;

namespace TextureFilter
{
    enum FilterTypes
    {
        TypeGenerateSwMipmaps               = 1u << 0u,
        TypeGenerateHwMipmaps               = 1u << 1u,
        TypePrepareForNormalMapping         = 1u << 2u,
        TypeLeaveChannelR                   = 1u << 3u,

        TypeGenerateDefaultMipmaps          = TypeGenerateSwMipmaps|TypeGenerateHwMipmaps
    };

    class _OgreExport FilterBase : public ResourceAlloc
    {
    public:
        virtual ~FilterBase();

        /// Gets executed from worker thread, right after the Image was loaded from
        /// file and we're done setting the metadata to the Texture. Beware the
        /// texture may or may not have been transitioned to resident yet (it's
        /// likely not resident, but 2nd face and onwards of cubemaps will be
        /// resident)
        virtual void _executeStreaming( Image2 &image, TextureGpu *texture ) {}

        /// Gets executed after the TextureGpu is fully resident and fully loaded.
        /// (except for the steps this filter is supposed to do)
        virtual void _executeSerial( TextureGpu *texture ) {}

    protected:
        /**
        @param filters
        @return
            See DefaultMipmapGen::DefaultMipmapGen
        */
        static uint8 selectMipmapGen( uint32 filters, const Image2 &image,
                                      const TextureGpuManager *textureManager );
    public:
        static void createFilters( uint32 filters, FilterBaseArray &outFilters,
                                   const TextureGpu *texture, const Image2 &image,
                                   bool toSysRam );
        static void destroyFilters( FilterBaseArray &inOutFilters );

        /// Simulates as if the given filters were applied, producing
        /// the resulting number mipmaps & PixelFormat
        ///
        /// When a TextureGpu transitions OnStorage -> Resident, we use the metadata
        /// cache and later compare if the cache was up to date.
        /// To check if it's up to date, we need to know the final number of mipmaps
        /// and final pixel format. Thus this function is needed in this case.
        ///
        /// However then transitioning OnStorage -> OnSystemRam, the cache is not
        /// used, because A. the metadata is not needed (it cannot optimize the
        /// shader) and B. the number of mipmaps may not match.
        /// This can happen because the HW mipmap filter won't be run.
        /// This function is not needed in such case.
        ///
        /// When transitioning OnSystemRam -> Resident, we already have all the
        /// metadata except for the mipmaps, as the HW mipmap filter will be run.
        /// Thus we need this function so we can set the number of mipmaps
        /// to the final value, immediately transition to Resident, and start
        /// loading the image on the background thread without ping-pong.
        static void simulateFiltersForCacheConsistency( uint32 filters, const Image2 &image,
                                                        const TextureGpuManager *textureGpuManager,
                                                        uint8 &inOutNumMipmaps,
                                                        PixelFormatGpu &inOutPixelFormat );
    };
    //-----------------------------------------------------------------------------------
    class _OgreExport GenerateSwMipmaps : public FilterBase
    {
    public:
        /// See Image2::Filter
        static uint32 getFilter( const Image2 &image );
        virtual void _executeStreaming( Image2 &image, TextureGpu *texture );
    };
    //-----------------------------------------------------------------------------------
    class _OgreExport GenerateHwMipmaps : public FilterBase
    {
        bool mNeedsMipmaps;
    public:
        GenerateHwMipmaps() : mNeedsMipmaps( false ) {}
        virtual void _executeStreaming( Image2 &image, TextureGpu *texture );
        virtual void _executeSerial( TextureGpu *texture );
    };
    //-----------------------------------------------------------------------------------
    class _OgreExport PrepareForNormalMapping : public FilterBase
    {
    public:
        static PixelFormatGpu getDestinationFormat( PixelFormatGpu srcFormat );
        virtual void _executeStreaming( Image2 &image, TextureGpu *texture );
    };
    //-----------------------------------------------------------------------------------
    class _OgreExport LeaveChannelR : public FilterBase
    {
    public:
        static PixelFormatGpu getDestinationFormat( PixelFormatGpu srcFormat );
        virtual void _executeStreaming( Image2 &image, TextureGpu *texture );
    };
}
    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
