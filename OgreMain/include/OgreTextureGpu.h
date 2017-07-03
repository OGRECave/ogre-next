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
            /// Let the GPU decide.
            Undefined,
            /// The subsample locations follow a fixed known pattern.
            /// Call TextureGpu::getSubsampleLocations to get them.
            Standard,
            /// The subsample locations are centered in a grid.
            /// May not be supported by the GPU/API, in which case Standard will be used instead
            /// Call TextureGpu::isMsaaPatternSupported to check whether it will be honoured.
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
            /// Texture cannot be used as a regular texture (bound to SRV in D3D11 terms)
            NotTexture          = 1u << 0u,
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
            /// When present, you may be creating another TextureGpu that accesses
            /// the internal resources of this TextureGpu but with a different format
            /// (e.g. useful for viewing a PFG_RGBA8_UNORM_SRGB as PFG_RGBA8_UNORM)
            Reinterpretable     = 1u << 6u,
            /// Prefer loading from files as sRGB when possible.
            /// e.g. load PFG_RGBA8_UNORM as PFG_RGBA8_UNORM_SRGB
            PrefersLoadingAsSRGB= 1u << 7u,
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
            AutomaticBatching   = 1u << 8u
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

        /// Used when AutomaticBatching is set. It indicates in which slice
        /// our actual data is, inside a texture array which we do not own.
        uint16      mInternalSliceStart;

        /// This setting can only be altered if mResidencyStatus == OnStorage).
        TextureTypes::TextureTypes  mTextureType;
        PixelFormatGpu              mPixelFormat;

        /// See TextureFlags::TextureFlags
        uint32      mTextureFlags;
        BufferType  mBufferType;

        uint8       *mSysRamCopy;

        TextureGpuManager   *mTextureManager;
        /// Used if hasAutomaticBatching() == true
        TexturePool const   *mTexturePool;

        vector<TextureGpuListener*>::type mListeners;

        virtual void createInternalResourcesImpl(void) = 0;
        virtual void destroyInternalResourcesImpl(void) = 0;

        void checkValidSettings(void);
        void transitionToResident(void);

    public:
        TextureGpu( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                    VaoManager *vaoManager, IdString name, uint32 textureFlags,
                    TextureGpuManager *textureManager );
        virtual ~TextureGpu();

        virtual String getNameStr(void) const;

        /** Schedules an async transition in residency. If transitioning from
            OnStorage to Resident, it will read from file (ResourceGroup was set in createTexture)
            If transitioning from OnSystemRam to Resident, it will read from the pointer it has.
            Multiple transitions can be stack together.
        @remarks
            If you're not loading from file (i.e. you're creating it programatically),
            call _transitionTo & _setNextResidencyStatus directly.
            Once you've called scheduleTransitionTo at least once, calling _transitionTo
            is very dangerous, as there are race conditions.
        @param nextResidency
            The residency to change to.
        */
        void scheduleTransitionTo( GpuResidency::GpuResidency nextResidency );

        void upload( const TextureBox &box, uint8 mipmapLevel, uint32 slice );

        //AsyncTextureTicketPtr readRequest( const PixelBox &box );

        void setResolution( uint32 width, uint32 height, uint32 depthOrSlices=1u );
        uint32 getWidth(void) const;
        uint32 getHeight(void) const;
        uint32 getDepthOrSlices(void) const;
        /// For TypeCube & TypeCubeArray, this value returns 1.
        uint32 getDepth(void) const;
        /// For TypeCube this value returns 6.
        /// For TypeCubeArray, value returns numSlices * 6u.
        uint32 getNumSlices(void) const;

        void setNumMipmaps( uint8 numMipmaps );
        uint8 getNumMipmaps(void) const;

        uint32 getInternalSliceStart(void) const;

        void setTextureType( TextureTypes::TextureTypes textureType );
        TextureTypes::TextureTypes getTextureType(void) const;
        TextureTypes::TextureTypes getInternalTextureType(void) const;

        /** Sets the pixel format.
        @remarks
            If prefersLoadingAsSRGB() returns true, the format may not be fully honoured
            (as we'll use the equivalent _SRGB variation).
        */
        void setPixelFormat( PixelFormatGpu pixelFormat );
        PixelFormatGpu getPixelFormat(void) const;

        /// Note: Passing 0 will be forced to 1.
        void setMsaa( uint8 msaa );
        uint8 getMsaa(void) const;

        void setMsaaPattern( MsaaPatterns::MsaaPatterns pattern );
        MsaaPatterns::MsaaPatterns getMsaaPattern(void) const;
        virtual bool isMsaaPatternSupported( MsaaPatterns::MsaaPatterns pattern );
        /** Get the MSAA subsample locations. mMsaaPatterns must not be MsaaPatterns::Undefined.
        @param locations
            Outputs an array with the locations for each subsample. Values are in range [-1; 1]
        */
        virtual void getSubsampleLocations( vector<Vector2>::type locations ) = 0;

        /** This function may be called manually (if user is manually managing a texture)
            or automatically (e.g. loading from file, or automatic batching is enabled)
            Once you call this function, you're no longer in OnStorage mode; and will
            transition to either OnSystemRam or Resident depending on whether auto
            batching is enabled.
        @remarks
            Do NOT call this function yourself if you've created this function with
            AutomaticBatching as Ogre will call this, from a worker thread!
        @par
            Make sure you're done using mSysRamCopy before calling this function,
            as we may free that pointer.
        @par
            If you're calling _transitionTo yourself (i.e. you're not using scheduleTransitionTo)
            then you'll need to call _setNextResidencyStatus too, so that both getResidencyStatus
            and getNextResidencyStatus agree.
        @param sysRamCopy
            System RAM copy that backs this GPU data. May be null.
            Must've been allocated with OGRE_MALLOC_SIMD( size, MEMCATEGORY_RESOURCE );
            We will deallocate it.
            MUST respect _getSysRamCopyBytesPerRow & _getSysRamCopyBytesPerImage.
            If in doubt, use PixelFormatGpuUtils::getSizeBytes with rowAlignment = 4u;
        */
        void _transitionTo( GpuResidency::GpuResidency newResidency, uint8 *sysRamCopy );

        /// Notifies it is safe to use the real data. Everything has been uploaded.
        virtual void notifyDataIsReady(void) = 0;

        virtual void copyTo( TextureGpu *dst, const TextureBox &srcBox, uint8 srcMipLevel,
                             const TextureBox &dstBox, uint8 dstMipLevel );

        bool hasAutomaticBatching(void) const;
        bool isTexture(void) const;
        bool isRenderToTexture(void) const;
        bool isUav(void) const;
        bool allowsAutoMipmaps(void) const;
        bool hasAutoMipmapAuto(void) const;
        bool hasMsaaExplicitResolves(void) const;
        bool isReinterpretable(void) const;
        bool prefersLoadingAsSRGB(void) const;

        virtual void _setToDisplayDummyTexture(void) = 0;
        virtual void _notifyTextureSlotChanged( const TexturePool *newPool, uint16 slice );

        const TexturePool* getTexturePool(void) const           { return mTexturePool; }

        void addListener( TextureGpuListener *listener );
        void removeListener( TextureGpuListener *listener );
        void notifyAllListenersTextureChanged( uint32 reason );

        uint8* _getSysRamCopy( uint8 mipLevel );
        size_t _getSysRamCopyBytesPerRow( uint8 mipLevel );
        size_t _getSysRamCopyBytesPerImage( uint8 mipLevel );
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
