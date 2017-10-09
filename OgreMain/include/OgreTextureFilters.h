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

        static void createFilters( uint32 filters, FilterBaseArray &outFilters,
                                   const TextureGpu *texture );
    };
    //-----------------------------------------------------------------------------------
    class _OgreExport GenerateSwMipmaps : public FilterBase
    {
    public:
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
        virtual void _executeStreaming( Image2 &image, TextureGpu *texture );
    };
    //-----------------------------------------------------------------------------------
    class _OgreExport LeaveChannelR : public FilterBase
    {
    public:
        virtual void _executeStreaming( Image2 &image, TextureGpu *texture );
    };
}
    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
