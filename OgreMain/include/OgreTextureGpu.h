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
            Center,
            /// All subsamples are at 0, 0; effectively "disabling" msaa.
            CenterZero
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
            /// Texture can use mipmap autogeneration. This flag is NOT necessary
            /// for TextureFilter::TypeGenerateHwMipmaps, as this filter will
            /// create a temporary resource.
            /// AllowAutomipmaps is thought for RenderToTexture textures.
            AllowAutomipmaps    = 1u << 3u,
            /// Texture will auto generate mipmaps every time it's dirty, automatically.
            /// Requires AllowAutomipmaps.
            AutomipmapsAuto     = 1u << 4u,
            /** MSAA rendering is an antialiasing technique. MSAA works by rendering
                to a special surface (an MSAA surface) and once we're done, "resolving"
                from MSAA surface into a regular texture for later sampling.
            @par
                Without explicit resolves, Ogre will automatically resolve the MSAA
                surface into the texture whenever it detects you will be sampling
                from this texture.
            @par
                However there are cases where you want to directly access the MSAA
                surface directly for advanced special effects (i.e. via Texture2DMS in
                HLSL).
                For cases like that, use MsaaExplicitResolve; which will let you to
                manually manage MSAA surfaces and when you want to resolve it.
            */
            MsaaExplicitResolve = 1u << 5u,
            /// When present, you may be creating another TextureGpu that accesses
            /// the internal resources of this TextureGpu but with a different format
            /// (e.g. useful for viewing a PFG_RGBA8_UNORM_SRGB as PFG_RGBA8_UNORM)
            Reinterpretable     = 1u << 6u,
            /// Prefer loading FROM FILES as sRGB when possible.
            /// e.g. load PFG_RGBA8_UNORM as PFG_RGBA8_UNORM_SRGB
            /// This flag does not affect RenderTextures, UAVs, or manually created textures.
            /// If you're manually creating sRGB textures, set PFG_RGBA8_UNORM_SRGB directly
            PrefersLoadingFromFileAsSRGB = 1u << 7u,
            /// Indicates this texture contains a RenderWindow. In several APIs render windows
            /// have particular limitations:
            ///     * Cannot be sampled as textures (i.e. NotTexture will be set)
            ///     * OpenGL cannot share the depth & stencil buffers with other textures.
            ///     * Metal requires special maintenance.
            ///     * etc.
            RenderWindowSpecific    = 1u << 9u,
            RequiresTextureFlipping = 1u << 10u,
            /// Indicates this texture will be filled by the user, and won't be loaded
            /// from file or a listener from within a worker thread. This flag
            /// is implicit if NotTexture, RenderToTexture or Uav are set.
            ManualTexture           = 1u << 11u,
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
            AutomaticBatching   = 1u << 12u,
            /// For internal use. Indicates whether this texture is the owner
            /// of a TextureGpuManager::TexturePool, which are used
            /// to hold regular textures using AutomaticBatching
            PoolOwner           = 1u << 13u
        };
    }
    /**
    @remarks
        Internal layout of data in memory:
        @verbatim
            Mip0 -> Slice 0, Slice 1, ..., Slice N
            Mip1 -> Slice 0, Slice 1, ..., Slice N
            ...
            MipN -> Slice 0, Slice 1, ..., Slice N
        @endverbatim

        The layout for 3D volume and array textures is the same. The only thing that changes
        is that for 3D volumes, the number of slices also decreases with each mip, while
        for array textures it is kept constant.

        For 1D array textures, the number of slices is stored in mDepthOrSlices, not in Height.

        For code reference, look at _getSysRamCopyAsBox implementation, and TextureBox::at
        Each row of pixels is aligned to 4 bytes (except for compressed formats that require
        more strict alignments, such as alignment to the block).

        A TextureGpu loaded from file has the following life cycle, usually:
            1. At creation it's mResidencyStatus = GpuResidency::OnStorage
            2. Loading is scheduled via scheduleTransitionTo.
               mNextResidencyStatus = GpuResidency::Resident
            3. Texture transitions to resident. mResidencyStatus = GpuResidency::Resident
               isMetadataReady returns true. How fast this happens depends on whether
               there was a metadata cache or not.
            4. If there is a metadata cache, and the cache turned out to be wrong (e.g. it
               lied or was out of date), the texture will transition
               back to OnStorage and the whole process repeats from step 1.
            5. Texture finishes loading. notifyDataIsReady gets called and now
               isDataReady returns true.
    */
    class _OgreExport TextureGpu : public GpuTrackedResource, public GpuResource
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
        /// Used if hasAutomaticBatching() == true
        uint32		mPoolId;

        /// If this pointer is nullptr and mResidencyStatus == GpuResidency::OnSystemRam
        /// then that means the data is being loaded to SystemRAM
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
                    TextureTypes::TextureTypes initialType,
                    TextureGpuManager *textureManager );
        virtual ~TextureGpu();

        void _resetTextureManager(void);

        /// Note: This returns the alias name of the texture.
        /// See TextureGpuManager::createOrRetrieveTexture
        virtual String getNameStr(void) const;
        /// Returns the real name (e.g. disk in file) of the resource.
        virtual String getRealResourceNameStr(void) const;
        virtual String getResourceGroupStr(void) const;
        String getSettingsDesc(void) const;

        /** Schedules an async transition in residency. If transitioning from
            OnStorage to Resident, it will read from file (ResourceGroup was set in createTexture)
            If transitioning from OnSystemRam to Resident, it will read from the pointer it has.
            Multiple transitions can be stack together.
        @remarks
            If you're not loading from file (i.e. you're creating it programatically),
            call _transitionTo & _setNextResidencyStatus directly.
            Once you've called scheduleTransitionTo at least once, calling _transitionTo
            is very dangerous, as there are race conditions.

            @see    TextureGpu::scheduleTransitionTo
        @param nextResidency
            The residency to change to.
        @param image
            Pointer to image if you want to load the texture from memory instead of loading
            it from file or a listener.
            Pointer must be null if this is a manual texture.
            Pointer must NOT be a stack variable nor be deleted immediately.
            The actual loading is postponed until the request reaches the worker thread.
            That means the image pointer is safe to delete once you receive the
            TextureGpuListener::Reason::ReadyForRendering message.
        @param autoDeleteImage
            Whether we should call "delete image" once we're done using the image.
            Otherwise you must listen for TextureGpuListener::ReadyForRendering
            message to know when we're done using the image.
        */
        void unsafeScheduleTransitionTo( GpuResidency::GpuResidency nextResidency,
                                         Image2 *image=0, bool autoDeleteImage=true );

        /// Same as unsafeScheduleTransitionTo, but first checks if we're already
        /// in the residency state we want to go to, or if it has already
        /// been scheduled; thus it can be called multiple times
        void scheduleTransitionTo( GpuResidency::GpuResidency nextResidency,
                                   Image2 *image=0, bool autoDeleteImage=true );

        // See isMetadataReady for threadsafety on these functions.
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

        virtual void setTextureType( TextureTypes::TextureTypes textureType );
        TextureTypes::TextureTypes getTextureType(void) const;
        TextureTypes::TextureTypes getInternalTextureType(void) const;

        /** Sets the pixel format.
        @remarks
            If prefersLoadingFromFileAsSRGB() returns true, the format may not be fully honoured
            (as we'll use the equivalent _SRGB variation).
        */
        void setPixelFormat( PixelFormatGpu pixelFormat );
        PixelFormatGpu getPixelFormat(void) const;

        /** In almost all cases, getPixelFormat will match with getInternalPixelFormat.
        @par
            However there are a few exceptions, suchs as D3D11's render window where
            the internal pixel format will be most likely PFG_RGBA8_UNORM however
            getPixelFormat will report PFG_RGBA8_UNORM_SRGB if HW gamma correction was asked.
        @remarks
            The real internal pixel format only is relevant for operations like _resolveTo
        @return
            The real pixel format, and not the one we pretend it is.
        */
        virtual PixelFormatGpu getInternalPixelFormat(void) const;

        /// Note: Passing 0 will be forced to 1.
        void setMsaa( uint8 msaa );
        uint8 getMsaa(void) const;

        void copyParametersFrom( TextureGpu *src );
        bool hasEquivalentParameters( TextureGpu *other ) const;
        void setHlmsProperties( Hlms *hlms, LwString &propBaseName );

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

            Make sure you're done using mSysRamCopy before calling this function,
            as we may free that pointer (unless autoDeleteSysRamCopyOnResident = false).

            If you're calling _transitionTo yourself (i.e. you're not using scheduleTransitionTo)
            then you'll need to call _setNextResidencyStatus too, so that both getResidencyStatus
            and getNextResidencyStatus agree.
        @param sysRamCopy
            System RAM copy that backs this GPU data. May be null.
            Must've been allocated with OGRE_MALLOC_SIMD( size, MEMCATEGORY_RESOURCE );
            We will deallocate it.
            MUST respect _getSysRamCopyBytesPerRow & _getSysRamCopyBytesPerImage.
            If in doubt, use PixelFormatGpuUtils::getSizeBytes with rowAlignment = 4u;

            This param must be nullptr or equal to get_getSysRamCopy when going from
            Resident to OnSystemRam and strategy is not AlwaysKeepSystemRamCopy; as we
            will async download the content from the GPU.
        @param autoDeleteSysRamCopy
            When true, we free mSysRamCopy as we should.
            When false, caller is responsible for deleting this pointer else it will leak!
        */
        void _transitionTo( GpuResidency::GpuResidency newResidency, uint8 *sysRamCopy,
                            bool autoDeleteSysRamCopy = true );

        /// Notifies it is safe to use the real data. Everything has been uploaded.
        virtual void notifyDataIsReady(void) = 0;

        /// Forces downloading data from GPU to CPU, usually because the data on GPU changed
        /// and we're in strategy AlwaysKeepSystemRamCopy. May stall.
        void _syncGpuResidentToSystemRam(void);

    protected:
        /// Stalls until GPU -> CPU transfers (i.e. _syncGpuResidentToSystemRam) are done
        /// waitForData implicitly does this. This function exists because there are times
        /// where Ogre needs to know this info, and calling waitForData would never return
        /// true because the texture is in an inconsistent state.
        void waitForPendingSyncs();

    public:
        /// Do not call directly. Will change mResidencyStatus from GpuResidency::Resident to
        /// GpuResidency::OnSystemRam
        void _notifySysRamDownloadIsReady( uint8 *sysRamPtr, bool resyncOnly );

        virtual void copyTo( TextureGpu *dst, const TextureBox &dstBox, uint8 dstMipLevel,
                             const TextureBox &srcBox, uint8 srcMipLevel );

        /** These 3 values  are used as defaults for the compositor to use, but they may be
            explicitly overriden by a RenderPassDescriptor.
            Particularly required when passing the textures between nodes as input and
            output (since only the TextureGpu pointer is passed, and thus this information is lost)
        @remarks
            Changing these settings won't take immediate effect because they're only used
            when creating the compositor.
        @param depthBufferPoolId
            Sets the pool ID this RenderTarget should query from. Default value is POOL_DEFAULT.
            Set to POOL_NO_DEPTH to avoid using a DepthBuffer (or manually controlling it)
        @param preferDepthTexture
            Whether this RT should be attached to a depth texture, or a regular depth buffer.
            On older GPUs, preferring depth textures may result in certain depth precisions
            to not be available (or use integer precision instead of floating point, etc).
            True to use depth textures. False otherwise (default).
        @param desiredDepthBufferFormat
            Ogre will try to honour this request, but if it's not supported,
            you may get lower precision.
            All formats will try to fallback to PF_D24_UNORM_S8_UINT if not supported.
            Must be one of the following:
                PFG_D24_UNORM_S8_UINT
                PFG_D16_UNORM
                PFG_D32_FLOAT
                PFG_D32_FLOAT_X24_S8_UINT
        */
        virtual void _setDepthBufferDefaults( uint16 depthBufferPoolId, bool preferDepthTexture,
                                              PixelFormatGpu desiredDepthBufferFormat );
        virtual uint16 getDepthBufferPoolId(void) const;
        virtual bool getPreferDepthTexture(void) const;
        virtual PixelFormatGpu getDesiredDepthBufferFormat(void) const;

        /** Immediately resolves this texture to the resolveTexture argument.
            Source must be MSAA texture, destination must be non-MSAA.
        @remarks
            This function may be slow on some APIs and should only be used when required,
            for example, to capture the screen from an explicit MSAA target and save it
            to disk only on user demand.
            If you need to call this often (like once per frame or more), then consider setting
            a Compositor with CompositorNode::mLocalRtvs::resolveTextureName set so that the
            compositor automatically resolves the texture every frame as efficiently as
            possible.
        */
        void _resolveTo( TextureGpu *resolveTexture );

        /// Tells the API to let the HW autogenerate mipmaps. Assumes the
        /// allowsAutoMipmaps() == true and isRenderToTexture() == true
        virtual void _autogenerateMipmaps(void) = 0;
        /// Only valid for TextureGpu classes.
        /// TODO: This may be moved to a different class.
        virtual void swapBuffers(void) {}

        bool hasAutomaticBatching(void) const;
        bool isTexture(void) const;
        bool isRenderToTexture(void) const;
        bool isUav(void) const;
        bool allowsAutoMipmaps(void) const;
        bool hasAutoMipmapAuto(void) const;
        bool hasMsaaExplicitResolves(void) const;
        bool isReinterpretable(void) const;
        bool prefersLoadingFromFileAsSRGB(void) const;
        bool isRenderWindowSpecific(void) const;
        bool requiresTextureFlipping(void) const;
        bool _isManualTextureFlagPresent(void) const;
        bool isManualTexture(void) const;
        bool isPoolOwner(void) const;

        /// OpenGL RenderWindows are a bit specific:
        ///     * Their origins are upside down. Which means we need to flip Y.
        ///     * They can access resolved contents of MSAA even if hasMsaaExplicitResolves = true
        virtual bool isOpenGLRenderWindow(void) const;

        virtual void _setToDisplayDummyTexture(void) = 0;
        virtual void _notifyTextureSlotChanged( const TexturePool *newPool, uint16 slice );

        /** 2D Texture with automatic batching will be merged with other textures into the
            same pool as one big 2D Array texture behind the scenes.

            For two textures to be placed in the same pool (assuming it's not full)
            the following must match:
                Width, Height, PixelFormat, number of mipmaps, poolID

            Pool ID is an arbitrary value with no actual meaning. This is ID
            allows you to prevent certain textures from being group together.
            For example, you may want all textures from Level 0 to be grouped
            together while Level 1 gets grouped together in a different pool

            @see	TextureFlags::AutomaticBatching
            @see	TextureGpuManager::reservePoolId
        @remarks
            This value cannot be changed while the texture is resident (i.e. because
            it has already been assigned to a pool)
        @param poolId
            Arbitrary value. Default value is 0.
        */
        void setTexturePoolId( uint32 poolId );
        uint32 getTexturePoolId(void) const						{ return mPoolId; }
        const TexturePool* getTexturePool(void) const           { return mTexturePool; }

        void addListener( TextureGpuListener *listener );
        void removeListener( TextureGpuListener *listener );
        void notifyAllListenersTextureChanged( uint32 reason, void *extraData=0 );

        const vector<TextureGpuListener*>::type& getListeners(void) const;

        virtual bool supportsAsDepthBufferFor( TextureGpu *colourTarget ) const;

        /// Writes the current contents of the render target to the named file.
        void writeContentsToFile( const String& filename, uint8 minMip, uint8 maxMip,
                                  bool automaticResolve=true );

        virtual void getCustomAttribute( IdString name, void *pData ) {}

        TextureGpuManager* getTextureManager(void) const;

        TextureBox getEmptyBox( uint8 mipLevel );
        TextureBox _getSysRamCopyAsBox( uint8 mipLevel );
        uint8* _getSysRamCopy( uint8 mipLevel );
        /// Note: Returns non-zero even if there is no system ram copy.
        size_t _getSysRamCopyBytesPerRow( uint8 mipLevel );
        /// Note: Returns non-zero even if there is no system ram copy.
        size_t _getSysRamCopyBytesPerImage( uint8 mipLevel );

        /// Returns total size in bytes used in GPU by this texture (not by its pool)
        /// including mipmaps.
        size_t getSizeBytes(void) const;

        /** It is threadsafe to call this function from main thread.
            If this returns false, then the following functions are not threadsafe:
            Setters must not be called, and getters may change from a worker thread:
                * setResolution
                * getWidth, getHeight, getDepth, getDepthOrSlices, getNumSlices
                * set/getPixelFormat
                * set/getNumMipmaps
                * set/getTextureType
                * getTexturePool
            Note that this function may return true but the worker thread
            may still be uploading to this texture. Use isDataReady to
            see if the worker thread is fully done with this texture.

        @remarks
            Function for querying/waiting for data and metadata to be ready
            are for blocking the main thread when a worker thread is loading
            the texture from file or a listener (i.e. isManualTexture returns false)
            otherwise you don't need to call these functions.
        */
        bool isMetadataReady(void) const;

        /// For internal use. Do not call directly.
        ///
        /// This function is the same isDataReady except it ignores pending residency changes,
        /// which is important when TextureGpuManager needs to know this information but the
        /// TextureGpu is transitioning (thus mPendingResidencyChanges is in an inconsistent state)
        virtual bool _isDataReadyImpl(void) const = 0;

        /// True if this texture is fully ready to be used for displaying.
        ///
        /// IMPORTANT: Always returns true if getResidencyStatus != GpuResidency::Resident
        /// and there are no pending residency transitions.
        ///
        /// Returns false while there are pending residency status
        ///
        /// If this is true, then isMetadataReady is also true.
        /// See isMetadataReady.
        bool isDataReady(void);

        /// Blocks main thread until metadata is ready. Afterwards isMetadataReady
        /// should return true. If it doesn't, then there was a problem loading
        /// the texture.
        /// See isMetadataReady remarks.
        void waitForMetadata(void);

        /// Blocks main thread until data is ready. Afterwards isDataReady
        /// should return true. If it doesn't, then there was a problem loading
        /// the texture.
        /// See isMetadataReady remarks.
        void waitForData(void);
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
