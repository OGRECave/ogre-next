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

#ifndef _OgreTextureGpu_H_
#define _OgreTextureGpu_H_

#include "OgreGpuResource.h"
#include "OgrePixelFormatGpu.h"

#include "Vao/OgreBufferPacked.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    namespace MsaaPatterns
    {
        enum MsaaPatterns
        {
            Undefined,
            Standard,
            Center
        };
    }

    namespace TextureTypes
    {
        enum TextureTypes
        {
            Unknown,
            Type1D,
            Type1DArray,
            Type2D,
            Type2DArray,
            TypeCube,
            TypeCubeArray,
            Type3D
        };
    }

    namespace CubemapSide
    {
        /// Please note that due to ancient GPU history reasons, cubemaps are always
        /// treated as left handed. Which means +Z = Forward (which contradicts Ogre
        /// and OpenGL common conventions of being right handed, where -Z = Forward)
        enum CubemapSide
        {
            PX, NX,
            PY, NY,
            PZ, NZ,
        };
    }

    namespace TextureFlags
    {
        enum TextureFlags
        {
            /// Texture can be used as a regular texture (bound to SRV in D3D11 terms)
            Texture             = 1u << 0u,
            /// Texture can be used as an RTT (FBO in GL terms)
            RenderToTexture     = 1u << 1u,
            /// Texture can be used as an UAV
            Uav                 = 1u << 2u,
            /// Texture can use mipmap autogeneration
            AllowAutomipmaps    = 1u << 3u,
            /// Texture will auto generate mipmaps every time it's dirty, automatically.
            /// Requires AllowAutomipmaps.
            AutomipmapsAuto     = 1u << 4u,
            ///
            MsaaExplicitResolve = 1u << 5u,
            /// When not present:
            /// The Texture is exactly the type requested (e.g. 2D texture won't
            /// get a 2D array instead)
            /// While a texture is transitioning to Resident, no 64x64 is used,
            /// but the 4x4 dummy one will be used instead (blank texture).
            ///
            /// When this bit is set:
            /// The Texture can be of a different type. Most normally we’ll treat
            /// 2D textures internally as a slice to a 2D array texture.
            /// Ogre will keep three API objects:
            ///     1. A single 4x4 texture. Blank.
            ///     2. An array of 2D textures of 64x64. One of its slices will
            ///        contain the mips of the texture being loaded
            ///     3. An array of 2D textures in which one of its slices the fully
            ///        resident texture will live.
            /// Each time we change the internal API object, HlmsDatablocks need to be
            /// notified so it can pack the arrays, update the slices to the GPU, and
            /// compute the texture hashes.
            /// All of that (except updating slices to the GPU) can be done in a
            /// worker thread, then all the values swapped w/ the Datablock’s.
            AutomaticBatching   = 1u << 6u
        };
    }

    class _OgreExport TextureGpu : public GpuResource
    {
    protected:
        uint32      mWidth;
        uint32      mHeight;
        /// For TypeCube this value is 6. For TypeCubeArray,
        /// it contains the number of cubemaps in the array * 6u.
        uint32      mDepthOrSlices;
        /// Set mNumMipmaps = 0 to auto generate until last level.
        /// mNumMipmaps = 1 means no extra mipmaps other than level 0.
        uint8       mNumMipmaps;
        uint8       mMsaa;
        MsaaPatterns::MsaaPatterns  mMsaaPattern;

        /// This setting can only be altered if mResidencyStatus == OnStorage).
        TextureTypes::TextureTypes  mTextureType;
        PixelFormatGpu              mPixelFormat;

        /// See TextureFlags::TextureFlags
        uint32      mTextureFlags;
        BufferType  mBufferType;

        virtual void createInternalResourcesImpl(void) = 0;
        virtual void destroyInternalResourcesImpl(void) = 0;

    public:
        TextureGpu( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                    VaoManager *vaoManager, uint32 textureFlags );
        virtual ~TextureGpu();

        void upload( const TextureBox &box, uint8 mipmapLevel, uint32 slice );

        //AsyncTextureTicketPtr readRequest( const PixelBox &box );

        uint32 getWidth(void) const;
        uint32 getHeight(void) const;
        uint32 getDepthOrSlices(void) const;
        /// For TypeCube & TypeCubeArray, this value returns 1.
        uint32 getDepth(void) const;
        /// For TypeCube this value returns 6.
        /// For TypeCubeArray, value returns numSlices * 6u.
        uint32 getNumSlices(void) const;
        TextureTypes::TextureTypes getTextureTypes(void) const;
        PixelFormatGpu getPixelFormat(void) const;

        /// Note: Passing 0 will be forced to 1.
        void setMsaa( uint8 msaa );
        uint8 getMsaa(void) const;

        void setMsaaPattern( MsaaPatterns::MsaaPatterns pattern );
        MsaaPatterns::MsaaPatterns getMsaaPattern(void) const;

        void _init(void);

        bool hasAutomaticBatching(void) const;
        bool isTexture(void) const;
        bool isRenderToTexture(void) const;
        bool isUav(void) const;
        bool allowsAutoMipmaps(void) const;
        bool hasAutoMipmapAuto(void) const;
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
