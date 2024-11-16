/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#include "OgreCommon.h"
#include "OgreGpuResource.h"
#include "OgrePixelFormatGpu.h"
#include "Vao/OgreBufferPacked.h"

#include "ogrestd/vector.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */

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
            // clang-format off
            PX, NX,
            PY, NY,
            PZ, NZ,
            // clang-format on
        };
    }  // namespace CubemapSide

    namespace TextureFlags
    {
        enum TextureFlags
        {
            // clang-format off
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
            ///
            /// See TextureGpuManager::mIgnoreSRgbPreference
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
            PoolOwner           = 1u << 13u,
            /// When this flag is present, the contents of a RenderToTexture or Uav
            /// may not be preserved between frames. Useful for RenderToTexture which are written or
            /// cleared first.
            ///
            /// Must not be used on textures whose contents need to be preserved between
            /// frames (e.g. HDR luminance change over time)
            ///
            /// If this flag is present, either RenderToTexture or Uav must be present
            DiscardableContent  = 1u << 14u,
            /// When this flag is present, we can save VRAM by using memoryless storage mode (Metal on
            /// iOS and Apple Silicon macOS). Choose the memoryless mode if your texture is a memoryless
            /// render target that’s temporarily populated and accessed by the GPU. Memoryless render
            /// targets are render targets that exist only in tile memory and are not backed by system
            /// memory. An example is a depth or stencil texture thatʼs used only within a render pass
            /// and isnʼt needed before or after GPU execution. This flag requires RenderToTexture
            TilerMemoryless = 1u << 15u,
            /// When this flag is present together with RenderToTexture it will use DepthBuffer with
            /// TilerMemoryless option
            TilerDepthMemoryless = 1u << 16u
            // clang-format on
        };
    }

    namespace TextureSourceType
    {
        enum TextureSourceType
        {
            Standard,           ///< Regular texture
            Shadow,             ///< Created by compositor, for shadow mapping
            Compositor,         ///< Created by compositor
            PoolOwner,          ///< TextureFlags::PoolOwner is set
            SharedDepthBuffer,  ///< Created automatically, may be shared and reused by multiple colour
                                /// targets
            NumTextureSourceTypes
        };
    }

    namespace CopyEncTransitionMode
    {
        /// Copy Encoder Transition modes to be used by TextureGpu::copyTo
        /// and TextureGpu::_autogenerateMipmaps
        enum CopyEncTransitionMode
        {
            /// The texture layout transitions are left to the copy encoder.
            /// Texture must NOT be in CopySrc or CopyDst.
            ///
            /// Texture will be marked as being in ResourceLayout::CopyEncoderManaged
            /// in the BarrierSolver
            ///
            /// Once the copy encoder is closed (e.g. implicitly or calling
            /// RenderSystem::endCopyEncoder) all auto managed textures will be
            /// transitioned to a default layout and BarrierSolver will be informed
            Auto,

            /// Texture is already transitioned directly via BarrierSolver to
            /// the expected CopySrc/CopyDst/MipmapGen.
            ///
            /// Main reason to do this because you were able to group many
            /// texture transitions together.
            ///
            /// Afterwards management is handed off to the Copy encoder: texture is
            /// tagged in BarrierSolver as CopyEncoderManaged and behaves like
            /// CopyEncTransitionMode::Auto.
            ///
            /// e.g.
            ///
            /// @code
            ///     solver.resolveTransition( resourceTransitions, src, ResourceLayout::CopySrc,
            ///                               ResourceAccess::Read, 0u );
            ///     renderSystem->executeResourceTransition( resourceTransitions );
            ///     src->copyTo( dst, dstBox, 0u, src->getEmptyBox( 0u ), 0u, keepResolvedTexSynced,
            ///                  CopyEncTransitionMode::AlreadyInLayoutThenAuto,
            ///                  CopyEncTransitionMode::Auto );
            /// @endcode
            AlreadyInLayoutThenAuto,

            /// Texture is already transitioned directly via BarrierSolver to
            /// the expected CopySrc/CopyDst/MipmapGen.
            ///
            /// After the copyTo/_autogenerateMipmaps calls, caller is
            /// responsible for using BarrierSolver again.
            ///
            /// Note that this can be very dangerous and not recommended!
            /// The following code is for example a race condition:
            ///
            /// @code
            ///     solver.resolveTransition( rt, a, ResourceLayout::CopySrc,
            ///                               ResourceAccess::Read, 0u );
            ///     renderSystem->executeResourceTransition( rt );
            ///     B->copyTo( A, ... );
            ///     C->copyTo( A, ... );
            /// @endcode
            ///
            /// The reason is that there should be a barrier between
            /// the 1st and 2nd copy; otherwise C copy into A may start
            /// before the copy from B into A finishes; thus A may not
            /// fully end up with C's contents.
            ///
            /// But if you're wary of placing the barriers correctly, you
            /// have full control and group as many layout transitions
            /// as possible
            AlreadyInLayoutThenManual
        };
    }  // namespace CopyEncTransitionMode

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
        uint32 mWidth;
        uint32 mHeight;
        /// For TypeCube this value is 6. For TypeCubeArray,
        /// it contains the number of cubemaps in the array * 6u.
        uint32 mDepthOrSlices;
        /// Set mNumMipmaps = 0 to auto generate until last level.
        /// mNumMipmaps = 1 means no extra mipmaps other than level 0.
        uint8             mNumMipmaps;
        SampleDescription mSampleDescription;
        SampleDescription mRequestedSampleDescription;

        /// Used when AutomaticBatching is set. It indicates in which slice
        /// our actual data is, inside a texture array which we do not own.
        uint16 mInternalSliceStart;

        /// This setting is for where the texture is created, e.g. its a compositor texture, a shadow
        /// texture or standard texture loaded for a mesh etc...
        ///
        /// This value is merely for statistical tracking purposes
        ///
        /// @see    TextureSourceType::TextureSourceType
        uint8 mSourceType;

        /// See TextureGpu::_isDataReadyImpl
        ///
        /// It is increased with every scheduleReupload/scheduleTransitionTo (to resident)
        /// It is decreased every time the loading is done
        ///
        /// _isDataReadyImpl can NOT return true if mDataReady != 0
        /// _isDataReadyImpl CAN return false if mDataReady == 0
        uint8 mDataPreparationsPending;

        /// This setting can only be altered if mResidencyStatus == OnStorage).
        TextureTypes::TextureTypes mTextureType;
        PixelFormatGpu             mPixelFormat;

        /// See TextureFlags::TextureFlags
        uint32 mTextureFlags;
        /// Used if hasAutomaticBatching() == true
        uint32 mPoolId;

        /// If this pointer is nullptr and mResidencyStatus == GpuResidency::OnSystemRam
        /// then that means the data is being loaded to SystemRAM
        uint8 *mSysRamCopy;

        TextureGpuManager *mTextureManager;
        /// Used if hasAutomaticBatching() == true
        TexturePool const *mTexturePool;

        vector<TextureGpuListener *>::type mListeners;

        virtual void createInternalResourcesImpl() = 0;
        virtual void destroyInternalResourcesImpl() = 0;

        void checkValidSettings();
        void transitionToResident();

    public:
        TextureGpu( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy, VaoManager *vaoManager,
                    IdString name, uint32 textureFlags, TextureTypes::TextureTypes initialType,
                    TextureGpuManager *textureManager );
        ~TextureGpu() override;

        void _resetTextureManager();

        /// Note: This returns the alias name of the texture.
        /// See TextureGpuManager::createOrRetrieveTexture
        String getNameStr() const override;
        /// Returns the real name (e.g. disk in file) of the resource.
        virtual String getRealResourceNameStr() const;
        virtual String getResourceGroupStr() const;

        String getSettingsDesc() const;

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
        @param bSkipMultiload
            Do not send this texture to be loaded by multiload (see TextureGpuManager::setMultiLoadPool)
            Useful if the order in which textures are being created into a pool must be preserved
            This value is implicitly true if image is not nullptr.
        */
        void unsafeScheduleTransitionTo( GpuResidency::GpuResidency nextResidency, Image2 *image = 0,
                                         bool autoDeleteImage = true, bool bSkipMultiload = false );

        /// Same as unsafeScheduleTransitionTo, but first checks if we're already
        /// in the residency state we want to go to, or if it has already
        /// been scheduled; thus it can be called multiple times
        void scheduleTransitionTo( GpuResidency::GpuResidency nextResidency, Image2 *image = 0,
                                   bool autoDeleteImage = true, bool bSkipMultiload = false );

        /** There are times where you want to reload a texture again (e.g. file on
            disk changed, uploading a new Image2, etc) without visual disruption.

            e.g. if you were to call:
            @code
                tex->scheduleTransitionTo( GpuResidency::OnStorage );
                tex->scheduleTransitionTo( GpuResidency::Resident, ... );
            @endcode

            you'll achieve the same result, however the texture becomes immediately
            unavailable causing a few frames were all the user sees is a blank texture
            until it is fully reloaded.

            This routine allows for an in-place hot-reload, where the old texture
            is swapped for the new one once it's done loading.

            This is also faster because DescriptorTextureSets don't change

        @remarks
            1. Assumes the last queued transition to perform is into
               Resident or OnSystemRam
            2. Visual hitches are unavoidable if metadata changes (e.g. new
               texture is of different pixel format, different number of
               mipmaps, resolution, etc)
               If that's the case, it is faster to transition to OnStorage,
               remove the metadata entry from cache, then to Resident again

        @param image
            See TextureGpu::unsafeScheduleTransitionTo
        @param autoDeleteImage
            Same TextureGpu::unsafeScheduleTransitionTo
        @param bSkipMultiload
            Same TextureGpu::unsafeScheduleTransitionTo
        */
        void scheduleReupload( Image2 *image = 0, bool autoDeleteImage = true,
                               bool bSkipMultiload = false );

        // See isMetadataReady for threadsafety on these functions.
        void   setResolution( uint32 width, uint32 height, uint32 depthOrSlices = 1u );
        uint32 getWidth() const;
        uint32 getHeight() const;
        uint32 getDepthOrSlices() const;
        /// For TypeCube & TypeCubeArray, this value returns 1.
        uint32 getDepth() const;
        /// For TypeCube this value returns 6.
        /// For TypeCubeArray, value returns numSlices * 6u.
        uint32 getNumSlices() const;
        /// Real API width accounting for TextureGpu::getOrientationMode
        /// If orientation mode is 90° or 270° then getInternalWidth returns the height and
        /// getInternalHeight returns the width
        uint32 getInternalWidth() const;
        /// Real API height accounting for TextureGpu::getOrientationMode. See getInternalWidth
        uint32 getInternalHeight() const;

        void  setNumMipmaps( uint8 numMipmaps );
        uint8 getNumMipmaps() const;

        uint16 getInternalSliceStart() const;

        virtual void               setTextureType( TextureTypes::TextureTypes textureType );
        TextureTypes::TextureTypes getTextureType() const;
        TextureTypes::TextureTypes getInternalTextureType() const;

        void _setSourceType( uint8 type );
        /// This setting is for where the texture is created, e.g. its a compositor texture, a shadow
        /// texture or standard texture loaded for a mesh etc...
        ///
        /// This value is merely for statistical tracking purposes
        uint8 getSourceType() const;

        /** Sets the pixel format.
        @remarks
            If prefersLoadingFromFileAsSRGB() returns true, the format may not be fully honoured
            (as we'll use the equivalent _SRGB variation).
        */
        void           setPixelFormat( PixelFormatGpu pixelFormat );
        PixelFormatGpu getPixelFormat() const;

        void setSampleDescription( SampleDescription desc );
        /// For internal use
        void _setSampleDescription( SampleDescription desc, SampleDescription validatedSampleDesc );

        /// Returns effective sample description supported by the API.
        /// Note it's only useful after having transitioned to resident.
        SampleDescription getSampleDescription() const;
        /// Returns original requested sample description, i.e. the raw input to setSampleDescription
        SampleDescription getRequestedSampleDescription() const;
        bool              isMultisample() const;

        void copyParametersFrom( TextureGpu *src );
        bool hasEquivalentParameters( TextureGpu *other ) const;

        virtual bool isMsaaPatternSupported( MsaaPatterns::MsaaPatterns pattern );
        /** Get the MSAA subsample locations.
            mSampleDescription.pattern must not be MsaaPatterns::Undefined.
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
        virtual void notifyDataIsReady() = 0;

        /// Forces downloading data from GPU to CPU, usually because the data on GPU changed
        /// and we're in strategy AlwaysKeepSystemRamCopy. May stall.
        void _syncGpuResidentToSystemRam();

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

        /**
        @param dst
        @param dstBox
        @param dstMipLevel
        @param srcBox
        @param srcMipLevel
        @param keepResolvedTexSynced
            When true, if dst is an MSAA texture and is implicitly resolved
            (i.e. dst->hasMsaaExplicitResolves() == false); the resolved texture
            is also kept up to date.

            Typically the reason to set this to false is if you plane on rendering more
            stuff to dst texture and then resolve.
        @param srcTransitionMode
            Transition mode for 'this'
        @param dstTransitionMode
            Transition mode for 'dst'
        */
        virtual void copyTo(
            TextureGpu *dst, const TextureBox &dstBox, uint8 dstMipLevel, const TextureBox &srcBox,
            uint8 srcMipLevel, bool keepResolvedTexSynced = true,
            CopyEncTransitionMode::CopyEncTransitionMode srcTransitionMode = CopyEncTransitionMode::Auto,
            CopyEncTransitionMode::CopyEncTransitionMode dstTransitionMode =
                CopyEncTransitionMode::Auto );

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
        virtual void   _setDepthBufferDefaults( uint16 depthBufferPoolId, bool preferDepthTexture,
                                                PixelFormatGpu desiredDepthBufferFormat );
        virtual uint16 getDepthBufferPoolId() const;
        virtual bool   getPreferDepthTexture() const;
        virtual PixelFormatGpu getDesiredDepthBufferFormat() const;

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

        /** Tells the API to let the HW autogenerate mipmaps. Assumes
            allowsAutoMipmaps() == true and isRenderToTexture() == true
        @param transitionMode
            See CopyEncTransitionMode::CopyEncTransitionMode
        */
        virtual void _autogenerateMipmaps( CopyEncTransitionMode::CopyEncTransitionMode transitionMode =
                                               CopyEncTransitionMode::Auto ) = 0;
        /// Only valid for TextureGpu classes.
        /// TODO: This may be moved to a different class.
        virtual void swapBuffers() {}

        bool hasAutomaticBatching() const;
        bool isTexture() const;
        bool isRenderToTexture() const;
        bool isUav() const;
        bool allowsAutoMipmaps() const;
        bool hasAutoMipmapAuto() const;
        bool hasMsaaExplicitResolves() const;
        bool isReinterpretable() const;
        bool prefersLoadingFromFileAsSRGB() const;
        bool isRenderWindowSpecific() const;
        bool requiresTextureFlipping() const;
        bool _isManualTextureFlagPresent() const;
        bool isManualTexture() const;
        bool isPoolOwner() const;
        bool isDiscardableContent() const;
        bool isTilerMemoryless() const { return ( mTextureFlags & TextureFlags::TilerMemoryless ) != 0; }
        bool isTilerDepthMemoryless() const
        {
            return ( mTextureFlags & TextureFlags::TilerDepthMemoryless ) != 0;
        }

        /// OpenGL RenderWindows are a bit specific:
        ///     * Their origins are upside down. Which means we need to flip Y.
        ///     * They can access resolved contents of MSAA even if hasMsaaExplicitResolves = true
        ///     * Headless windows return false since internally they're FBOs. However
        ///       isRenderWindowSpecific will return true
        virtual bool isOpenGLRenderWindow() const;

        /// PUBLIC VARIABLE. This variable can be altered directly.
        ///
        /// Changes are reflected immediately for new TextureGpus.
        /// Existing TextureGpus won't be affected
        static OrientationMode msDefaultOrientationMode;

        /** Sets the given orientation. 'this' must be a RenderTexture
            If Ogre wasn't build with OGRE_CONFIG_ENABLE_VIEWPORT_ORIENTATIONMODE,
            calls to this function will not stick (i.e. getOrientationMode always
            returns the same value)

            @see    TextureGpu::msDefaultOrientationMode
            @see    TextureGpu::getInternalWidth
            @see    TextureGpu::getInternalHeight
        @remarks
            Must be OnStorage.

            If OrientationMode == OR_DEGREE_90 or OR_DEGREE_270, the internal resolution
            if flipped. i.e. swap( width, height ).
            This is important if you need to perform copyTo operations or AsyncTextureTickets

            This setting has only been tested with Vulkan and is likely to malfunction
            with the other APIs if set to anything other than OR_DEGREE_0
        */
        virtual void            setOrientationMode( OrientationMode orientationMode );
        virtual OrientationMode getOrientationMode() const;

        ResourceLayout::Layout         getDefaultLayout( bool bIgnoreDiscardableFlag = false ) const;
        virtual ResourceLayout::Layout getCurrentLayout() const;

        /// Sets the layout the texture should be transitioned to after the next copy operation
        /// (once the copy encoder gets closed)
        ///
        /// This is specific to Vulkan & D3D12
        virtual void _setNextLayout( ResourceLayout::Layout layout );

        virtual void _setToDisplayDummyTexture() = 0;
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
        void   setTexturePoolId( uint32 poolId );
        uint32 getTexturePoolId() const { return mPoolId; }

        const TexturePool *getTexturePool() const { return mTexturePool; }

        void addListener( TextureGpuListener *listener );
        void removeListener( TextureGpuListener *listener );
        void notifyAllListenersTextureChanged( uint32 reason, void *extraData = 0 );

        const vector<TextureGpuListener *>::type &getListeners() const;

        virtual bool supportsAsDepthBufferFor( TextureGpu *colourTarget ) const;

        /// Writes the current contents of the render target to the named file.
        void writeContentsToFile( const String &filename, uint8 minMip, uint8 maxMip,
                                  bool automaticResolve = true );

        /// Writes the current contents of the render target to the memory.
        void copyContentsToMemory( TextureBox src, TextureBox dst, PixelFormatGpu dstFormat,
                                   bool automaticResolve = true );

        static const IdString msFinalTextureBuffer;
        static const IdString msMsaaTextureBuffer;

        virtual void getCustomAttribute( IdString name, void *pData ) {}

        bool isTextureGpu() const override;

        TextureGpuManager *getTextureManager() const;

        TextureBox getEmptyBox( uint8 mipLevel );
        TextureBox _getSysRamCopyAsBox( uint8 mipLevel );
        uint8     *_getSysRamCopy( uint8 mipLevel );
        /// Note: Returns non-zero even if there is no system ram copy.
        uint32 _getSysRamCopyBytesPerRow( uint8 mipLevel );
        /// Note: Returns non-zero even if there is no system ram copy.
        size_t _getSysRamCopyBytesPerImage( uint8 mipLevel );

        /// Returns total size in bytes used in GPU by this texture (not by its pool)
        /// including mipmaps.
        size_t getSizeBytes() const;

        /** It is threadsafe to call this function from main thread.
            If this returns false, then the following functions are not threadsafe:
            Setters must not be called, and getters may change from a worker thread:
                + setResolution
                + getWidth, getHeight, getDepth, getDepthOrSlices, getNumSlices
                + set/getPixelFormat
                + set/getNumMipmaps
                + set/getTextureType
                + getTexturePool
            Note that this function may return true but the worker thread
            may still be uploading to this texture. Use isDataReady to
            see if the worker thread is fully done with this texture.

        @remarks
            Function for querying/waiting for data and metadata to be ready
            are for blocking the main thread when a worker thread is loading
            the texture from file or a listener (i.e. isManualTexture returns false)
            otherwise you don't need to call these functions.
        */
        bool isMetadataReady() const;

        /// For internal use. Do not call directly.
        ///
        /// This function is the same isDataReady except it ignores pending residency changes,
        /// which is important when TextureGpuManager needs to know this information but the
        /// TextureGpu is transitioning (thus mPendingResidencyChanges is in an inconsistent state)
        virtual bool _isDataReadyImpl() const = 0;

        /// True if this texture is fully ready to be used for displaying.
        ///
        /// IMPORTANT: Always returns true if getResidencyStatus != GpuResidency::Resident
        /// and there are no pending residency transitions.
        ///
        /// Returns false while there are pending residency status
        ///
        /// If this is true, then isMetadataReady is also true.
        /// See isMetadataReady.
        bool isDataReady() const;

        /// Blocks main thread until metadata is ready. Afterwards isMetadataReady
        /// should return true. If it doesn't, then there was a problem loading
        /// the texture.
        /// See isMetadataReady remarks.
        void waitForMetadata();

        /// Blocks main thread until data is ready. Afterwards isDataReady
        /// should return true. If it doesn't, then there was a problem loading
        /// the texture.
        /// See isMetadataReady remarks.
        ///
        /// Q: What's the penalty for calling this function?
        ///
        /// A: We need to wait for the worker thread to finish all previous textures
        /// until it processes this one. The manager only has broad resolution so
        /// it may be also possible that we even have to wait the worker thread to
        /// process a few textures that came *after* this one too.
        ///
        /// Thus the cost can be anywhere from "very little" to "a lot" depending on the
        /// order in which other textures have been loaded.
        ///
        /// The real cost is that you lose valuable ability to hide loading times.
        /// If you must call this function, you can mitigate the problem:
        ///
        ///     1. All textures you need to wait for, load them *first* together, then
        ///        call TextureGpuManager::waitForStreamingCompletion (preferred) or
        ///        this function. Then proceed to load the rest of the textures.
        ///     2. If you can't do the above, call this function as late as possible
        void waitForData();
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
