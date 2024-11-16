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

#include "OgreStableHeaders.h"

#include "OgreTextureGpu.h"

#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreLwString.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreTextureBox.h"
#include "OgreTextureGpuListener.h"
#include "OgreTextureGpuManager.h"

// Needed by _resolveTo
#include "OgreRenderPassDescriptor.h"
#include "OgreRenderSystem.h"

namespace Ogre
{
    OrientationMode TextureGpu::msDefaultOrientationMode = OR_DEGREE_0;

    const IdString TextureGpu::msFinalTextureBuffer = IdString( "msFinalTextureBuffer" );
    const IdString TextureGpu::msMsaaTextureBuffer = IdString( "msMsaaTextureBuffer" );

    TextureGpu::TextureGpu( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                            VaoManager *vaoManager, IdString name, uint32 textureFlags,
                            TextureTypes::TextureTypes initialType, TextureGpuManager *textureManager ) :
        GpuResource( pageOutStrategy, vaoManager, name ),
        mWidth( 0 ),
        mHeight( 0 ),
        mDepthOrSlices( 0 ),
        mNumMipmaps( 1 ),
        mInternalSliceStart( 0 ),
        mSourceType( TextureSourceType::Standard ),
        mDataPreparationsPending( 0u ),
        mTextureType( initialType ),
        mPixelFormat( PFG_UNKNOWN ),
        mTextureFlags( textureFlags ),
        mPoolId( 0 ),
        mSysRamCopy( 0 ),
        mTextureManager( textureManager ),
        mTexturePool( 0 )
    {
        assert( !hasAutomaticBatching() ||
                ( hasAutomaticBatching() && isTexture() && !isRenderToTexture() && !isUav() ) );
        assert( ( !hasAutoMipmapAuto() || allowsAutoMipmaps() ) &&
                "AutomipmapsAuto requires AllowAutomipmaps" );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu::~TextureGpu()
    {
        assert( mListeners.empty() &&
                "There are listeners out there for this TextureGpu! "
                "notifyAllListenersTextureChanged( TextureGpuListener::Deleted ) wasn't called!"
                "This could leave dangling pointers. Ensure you've cleaned up correctly. "
                "Most likely there are Materials/Datablocks still using this texture" );

        if( mSysRamCopy )
        {
            OGRE_FREE_SIMD( mSysRamCopy, MEMCATEGORY_RESOURCE );
            mSysRamCopy = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::_resetTextureManager() { mTextureManager = 0; }
    //-----------------------------------------------------------------------------------
    String TextureGpu::getNameStr() const
    {
        String retVal;
        const String *nameStr = mTextureManager->findAliasNameStr( mName );

        if( nameStr )
            retVal = *nameStr;
        else
            retVal = mName.getFriendlyText();

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    String TextureGpu::getRealResourceNameStr() const
    {
        String retVal;
        const String *nameStr = mTextureManager->findResourceNameStr( mName );

        if( nameStr )
            retVal = *nameStr;
        else
            retVal = mName.getFriendlyText();

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    String TextureGpu::getResourceGroupStr() const
    {
        String retVal;
        const String *nameStr = mTextureManager->findResourceGroupStr( mName );

        if( nameStr )
            retVal = *nameStr;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    String TextureGpu::getSettingsDesc() const
    {
        char tmpBuffer[92];
        LwString desc( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

        desc.a( mWidth, "x", mHeight, "x", mDepthOrSlices, " " );
        mSampleDescription.getFsaaDesc( desc );
        desc.a( " ", PixelFormatGpuUtils::toString( mPixelFormat ) );
        if( isRenderWindowSpecific() )
            desc.a( " renderwindow" );

        return String( desc.c_str() );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::unsafeScheduleTransitionTo( GpuResidency::GpuResidency nextResidency, Image2 *image,
                                                 bool autoDeleteImage, bool bSkipMultiload )
    {
        mNextResidencyStatus = nextResidency;
        ++mPendingResidencyChanges;

        if( isManualTexture() )
        {
            OGRE_ASSERT_LOW( !image && "Image pointer must null for manual textures!" );
            // Transition immediately. There's nothing from file or listener to load.
            this->_transitionTo( nextResidency, (uint8 *)0 );
        }
        else
        {
            // Schedule transition, we'll be loading from a worker thread.
            mTextureManager->_scheduleTransitionTo( this, nextResidency, image, autoDeleteImage, false,
                                                    bSkipMultiload );
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::scheduleTransitionTo( GpuResidency::GpuResidency nextResidency, Image2 *image,
                                           bool autoDeleteImage, bool bSkipMultiload )
    {
        if( mNextResidencyStatus != nextResidency )
            unsafeScheduleTransitionTo( nextResidency, image, autoDeleteImage, bSkipMultiload );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::scheduleReupload( Image2 *image, bool autoDeleteImage, bool bSkipMultiload )
    {
        OGRE_ASSERT_LOW( mNextResidencyStatus != GpuResidency::OnStorage );
        OGRE_ASSERT_LOW( mDataPreparationsPending < std::numeric_limits<uint8>::max() &&
                         "Overflow. Too many transitions queued up" );

        if( mNextResidencyStatus == GpuResidency::OnSystemRam )
        {
            scheduleTransitionTo( Ogre::GpuResidency::OnStorage );
            scheduleTransitionTo( Ogre::GpuResidency::OnSystemRam, image, autoDeleteImage );
        }
        else
        {
            ++mDataPreparationsPending;
            mTextureManager->_scheduleTransitionTo( this, GpuResidency::Resident, image, autoDeleteImage,
                                                    true, bSkipMultiload );
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::setResolution( uint32 width, uint32 height, uint32 depthOrSlices )
    {
        assert( mResidencyStatus == GpuResidency::OnStorage || isRenderWindowSpecific() );
        mWidth = width;
        mHeight = height;
        if( mTextureType == TextureTypes::TypeCube )
            mDepthOrSlices = 6u;
        else
        {
            assert( ( mTextureType != TextureTypes::TypeCubeArray || ( mTextureType % 6u ) == 0 ) &&
                    "depthOrSlices must be a multiple of 6 for TypeCubeArray textures!" );
            mDepthOrSlices = depthOrSlices;
        }
    }
    //-----------------------------------------------------------------------------------
    uint32 TextureGpu::getWidth() const { return mWidth; }
    //-----------------------------------------------------------------------------------
    uint32 TextureGpu::getHeight() const { return mHeight; }
    //-----------------------------------------------------------------------------------
    uint32 TextureGpu::getDepthOrSlices() const { return mDepthOrSlices; }
    //-----------------------------------------------------------------------------------
    uint32 TextureGpu::getDepth() const
    {
        return ( mTextureType != TextureTypes::Type3D ) ? 1u : mDepthOrSlices;
    }
    //-----------------------------------------------------------------------------------
    uint32 TextureGpu::getNumSlices() const
    {
        return ( mTextureType != TextureTypes::Type3D ) ? mDepthOrSlices : 1u;
    }
    //-----------------------------------------------------------------------------------
    uint32 TextureGpu::getInternalWidth() const
    {
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        const OrientationMode orientationMode = getOrientationMode();
        if( orientationMode == OR_DEGREE_90 || orientationMode == OR_DEGREE_270 )
            return mHeight;
#endif
        return mWidth;
    }
    //-----------------------------------------------------------------------------------
    uint32 TextureGpu::getInternalHeight() const
    {
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        const OrientationMode orientationMode = getOrientationMode();
        if( orientationMode == OR_DEGREE_90 || orientationMode == OR_DEGREE_270 )
            return mWidth;
#endif
        return mHeight;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::setNumMipmaps( uint8 numMipmaps )
    {
        assert( mResidencyStatus == GpuResidency::OnStorage );
        assert( numMipmaps > 0u &&
                "A value of 0 is not valid. Did you mean 1? "
                "Old textures did not count the base mip, but TextureGpu does" );
        mNumMipmaps = numMipmaps;
    }
    //-----------------------------------------------------------------------------------
    uint8 TextureGpu::getNumMipmaps() const { return mNumMipmaps; }
    //-----------------------------------------------------------------------------------
    void TextureGpu::setTextureType( TextureTypes::TextureTypes textureType )
    {
        assert( mResidencyStatus == GpuResidency::OnStorage );
        mTextureType = textureType;
        if( mTextureType == TextureTypes::TypeCube )
            mDepthOrSlices = 6u;
    }
    //-----------------------------------------------------------------------------------
    TextureTypes::TextureTypes TextureGpu::getTextureType() const { return mTextureType; }
    //-----------------------------------------------------------------------------------
    TextureTypes::TextureTypes TextureGpu::getInternalTextureType() const
    {
        return hasAutomaticBatching() ? TextureTypes::Type2DArray : mTextureType;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::setPixelFormat( PixelFormatGpu pixelFormat )
    {
        assert( mResidencyStatus == GpuResidency::OnStorage );

        if( prefersLoadingFromFileAsSRGB() )
            pixelFormat = PixelFormatGpuUtils::getEquivalentSRGB( pixelFormat );

        mPixelFormat = pixelFormat;
    }
    //-----------------------------------------------------------------------------------
    PixelFormatGpu TextureGpu::getPixelFormat() const { return mPixelFormat; }
    //-----------------------------------------------------------------------------------
    uint16 TextureGpu::getInternalSliceStart() const { return mInternalSliceStart; }
    //-----------------------------------------------------------------------------------
    void TextureGpu::_setSourceType( uint8 type ) { mSourceType = type; }
    //-----------------------------------------------------------------------------------
    uint8 TextureGpu::getSourceType() const { return mSourceType; }
    //-----------------------------------------------------------------------------------
    void TextureGpu::setSampleDescription( SampleDescription desc )
    {
        assert( mResidencyStatus == GpuResidency::OnStorage );
        OGRE_ASSERT_LOW( desc.getColourSamples() > 0u );
        mRequestedSampleDescription = desc;
        mSampleDescription = desc;  // would be validated on transition to Resident state
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::_setSampleDescription( SampleDescription requestedSampleDesc,
                                            SampleDescription validatedSampleDesc )
    {
        assert( mResidencyStatus == GpuResidency::OnStorage );
        OGRE_ASSERT_LOW( requestedSampleDesc.getColourSamples() > 0u );
        OGRE_ASSERT_LOW( validatedSampleDesc.getColourSamples() > 0u );
        mRequestedSampleDescription = requestedSampleDesc;
        mSampleDescription = validatedSampleDesc;  // should be validated by caller
    }
    //-----------------------------------------------------------------------------------
    SampleDescription TextureGpu::getSampleDescription() const { return mSampleDescription; }
    //-----------------------------------------------------------------------------------
    SampleDescription TextureGpu::getRequestedSampleDescription() const
    {
        return mRequestedSampleDescription;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isMultisample() const { return mSampleDescription.isMultisample(); }
    //-----------------------------------------------------------------------------------
    void TextureGpu::copyParametersFrom( TextureGpu *src )
    {
        assert( this->mResidencyStatus == GpuResidency::OnStorage );

        this->mWidth = src->mWidth;
        this->mHeight = src->mHeight;
        this->mDepthOrSlices = src->mDepthOrSlices;
        this->mNumMipmaps = src->mNumMipmaps;
        this->mPixelFormat = src->mPixelFormat;
        this->mSampleDescription = src->mSampleDescription;
        this->mRequestedSampleDescription = src->mRequestedSampleDescription;
        this->mTextureType = src->mTextureType;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::hasEquivalentParameters( TextureGpu *other ) const
    {
        return this->mWidth == other->mWidth && this->mHeight == other->mHeight &&
               this->mDepthOrSlices == other->mDepthOrSlices &&
               this->mNumMipmaps == other->mNumMipmaps && this->mPixelFormat == other->mPixelFormat &&
               this->mSampleDescription == other->mSampleDescription &&
               this->mTextureType == other->mTextureType;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isMsaaPatternSupported( MsaaPatterns::MsaaPatterns pattern )
    {
        return pattern == MsaaPatterns::Undefined;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::checkValidSettings()
    {
        // Make sure depth buffers/textures always have MsaaExplicitResolve set (with or without MSAA).
        if( PixelFormatGpuUtils::isDepth( mPixelFormat ) )
            mTextureFlags |= TextureFlags::MsaaExplicitResolve;
        if( mPixelFormat == PFG_NULL && isTexture() )
            mTextureFlags |= TextureFlags::NotTexture;

        if( mSampleDescription.isMultisample() )
        {
            if( ( mNumMipmaps > 1u ) || ( !isRenderToTexture() && !isUav() ) )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Texture '" + getNameStr() +
                                 "': "
                                 "MSAA Textures cannot have mipmaps (use explict resolves for that), "
                                 "and must be either RenderToTexture or Uav",
                             "TextureGpu::checkValidSettings" );
            }

            if( mTextureType == TextureTypes::Type2DArray && !hasMsaaExplicitResolves() )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Texture '" + getNameStr() +
                                 "': "
                                 "Only explicit resolves support Type2DArray",
                             "TextureGpu::checkValidSettings" );
            }

            if( mTextureType != TextureTypes::Type2D && mTextureType != TextureTypes::Type2DArray )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Texture '" + getNameStr() +
                                 "': "
                                 "MSAA can only be used with Type2D or Type2DArray",
                             "TextureGpu::checkValidSettings" );
            }

            if( hasAutomaticBatching() )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Texture '" + getNameStr() +
                                 "': "
                                 "MSAA textures cannot use AutomaticBatching",
                             "TextureGpu::checkValidSettings" );
            }
        }

        if( mTextureType == TextureTypes::Unknown )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Texture '" + getNameStr() +
                             "': "
                             "TextureType cannot be TextureTypes::Unknown",
                         "TextureGpu::checkValidSettings" );
        }

        if( mPageOutStrategy == GpuPageOutStrategy::AlwaysKeepSystemRamCopy &&
            ( isRenderToTexture() || isUav() ) )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Cannot use AlwaysKeepSystemRamCopy with RenderToTexture or Uav",
                         "TextureGpu::checkValidSettings" );
        }

        if( hasAutomaticBatching() &&
            ( mTextureType != TextureTypes::Type2D || isRenderToTexture() || isUav() ) )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Texture '" + getNameStr() +
                             "': "
                             "AutomaticBatching can only be used with Type2D textures, "
                             "and they cannot be RenderToTexture or Uav",
                         "TextureGpu::checkValidSettings" );
        }

        if( allowsAutoMipmaps() && !isRenderToTexture() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Texture '" + getNameStr() +
                             "': "
                             "AllowAutomipmaps requires RenderToTexture.",
                         "TextureGpu::checkValidSettings" );
        }

        if( mWidth < 1u || mHeight < 1u || mDepthOrSlices < 1u || mPixelFormat == PFG_UNKNOWN )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Texture '" + getNameStr() +
                             "': "
                             "Invalid settings!",
                         "TextureGpu::checkValidSettings" );
        }

        mNumMipmaps = std::min( mNumMipmaps,
                                PixelFormatGpuUtils::getMaxMipmapCount( mWidth, mHeight, getDepth() ) );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::transitionToResident()
    {
        checkValidSettings();

        if( mRequestedSampleDescription.isMultisample() )
        {
            // RenderWindows already validated their SampleDescriptions and we cannot override
            // Thus we rely on Windows already having properly called _setSampleDescription
            // and just call the listeners
            if( !isRenderWindowSpecific() )
            {
                uint32 msaaTextureFlags = TextureFlags::NotTexture;
                if( hasMsaaExplicitResolves() )
                    msaaTextureFlags = mTextureFlags;

                mSampleDescription = mTextureManager->getRenderSystem()->validateSampleDescription(
                    mRequestedSampleDescription, mPixelFormat, msaaTextureFlags );
            }
            if( !( mSampleDescription == mRequestedSampleDescription ) )
                notifyAllListenersTextureChanged( TextureGpuListener::FsaaSettingAlteredByApi, 0 );
        }

        if( !hasAutomaticBatching() )
        {
            // At this point we should have all valid settings (pixel format, width, height)
            // Create our own resource
            createInternalResourcesImpl();
        }
        else
        {
            // Ask the manager for the internal resource.
            mTextureManager->_reserveSlotForTexture( this );
        }

        // Delegate to the manager (thread) for loading.
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::_transitionTo( GpuResidency::GpuResidency newResidency, uint8 *sysRamCopy,
                                    bool autoDeleteSysRamCopy )
    {
        assert( newResidency != mResidencyStatus );

        bool allowResidencyChange = true;
        TextureGpuListener::Reason listenerReason = TextureGpuListener::Unknown;

        if( mResidencyStatus == GpuResidency::Resident )
        {
            const uint8 savedValue = mDataPreparationsPending;
            mDataPreparationsPending = 0u;
            const bool bWasDataReady = _isDataReadyImpl();
            mDataPreparationsPending = savedValue;

            if( !bWasDataReady )
            {
                OGRE_ASSERT_LOW( mDataPreparationsPending > 0u && "This should never happen" );
                // If we reach here, then we're aborting a load. e.g.
                // user went to Resident then to OnStorage without ever
                // calling notifyDataIsReady()
                // This is perfectly valid, and can happen if the texture
                // metadata cache got out of date for example.
                --mDataPreparationsPending;
            }
        }

        if( newResidency == GpuResidency::Resident )
        {
            if( mPageOutStrategy != GpuPageOutStrategy::AlwaysKeepSystemRamCopy )
            {
                if( mSysRamCopy )
                {
                    if( autoDeleteSysRamCopy )
                        OGRE_FREE_SIMD( mSysRamCopy, MEMCATEGORY_RESOURCE );
                    mSysRamCopy = 0;
                }
            }
            else
            {
                assert( sysRamCopy && "Must provide a SysRAM copy if using AlwaysKeepSystemRamCopy" );
                mSysRamCopy = sysRamCopy;
            }

            OGRE_ASSERT_LOW( mDataPreparationsPending < std::numeric_limits<uint8>::max() &&
                             "Overflow. Too many transitions queued up" );
            ++mDataPreparationsPending;

            transitionToResident();
            listenerReason = TextureGpuListener::GainedResidency;
        }
        else if( newResidency == GpuResidency::OnSystemRam )
        {
            if( mResidencyStatus == GpuResidency::OnStorage )
            {
                OGRE_ASSERT_LOW( !mSysRamCopy &&
                                 "It should be impossible to have SysRAM copy in this stage!" );
                checkValidSettings();

                OGRE_ASSERT_LOW( sysRamCopy &&
                                 "Must provide a SysRAM copy when transitioning "
                                 "from OnStorage to OnSystemRam!" );
                mSysRamCopy = sysRamCopy;

                listenerReason = TextureGpuListener::FromStorageToSysRam;
            }
            else
            {
                OGRE_ASSERT_LOW(
                    ( ( mSysRamCopy &&
                        mPageOutStrategy == GpuPageOutStrategy::AlwaysKeepSystemRamCopy ) ||
                      ( !mSysRamCopy &&
                        mPageOutStrategy != GpuPageOutStrategy::AlwaysKeepSystemRamCopy ) ) &&
                    "We should already have a SysRAM copy if we were "
                    "AlwaysKeepSystemRamCopy; or we shouldn't have a"
                    "SysRAM copy if we weren't in that strategy." );

                OGRE_ASSERT_LOW( ( mPageOutStrategy == GpuPageOutStrategy::AlwaysKeepSystemRamCopy ||
                                   !sysRamCopy || mSysRamCopy == sysRamCopy ) &&
                                 "sysRamCopy must be nullptr or equal to mSysRamCopy when "
                                 "mPageOutStrategy != GpuPageOutStrategy::AlwaysKeepSystemRamCopy" );

                if( !mSysRamCopy )
                {
                    mTextureManager->_queueDownloadToRam( this, false );
                    allowResidencyChange = false;
                }
                else
                {
                    destroyInternalResourcesImpl();
                    listenerReason = TextureGpuListener::LostResidency;
                }
            }
        }
        else
        {
            if( mSysRamCopy )
            {
                if( autoDeleteSysRamCopy )
                    OGRE_FREE_SIMD( mSysRamCopy, MEMCATEGORY_RESOURCE );
                mSysRamCopy = 0;
            }

            destroyInternalResourcesImpl();

            if( mResidencyStatus == GpuResidency::Resident )
                listenerReason = TextureGpuListener::LostResidency;
            else
                listenerReason = TextureGpuListener::FromSysRamToStorage;
        }

        if( allowResidencyChange )
        {
            mResidencyStatus = newResidency;
            // Decrement mPendingResidencyChanges and prevent underflow
            mPendingResidencyChanges = std::max( mPendingResidencyChanges, 1u ) - 1u;
            notifyAllListenersTextureChanged( listenerReason );
        }

        if( isManualTexture() )
        {
            mNextResidencyStatus = mResidencyStatus;
            if( mResidencyStatus == GpuResidency::Resident )
                this->notifyDataIsReady();
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::_syncGpuResidentToSystemRam()
    {
        if( !isDataReady() )
        {
            LogManager::getSingleton().logMessage(
                "WARNING: TextureGpu::_syncGpuResidentToSystemRam will stall. "
                "If you see this often, then probably you're performing too many copyTo "
                "calls to an AlwaysKeepSystemRamCopy texture" );
        }
        waitForData();
        OGRE_ASSERT_LOW( mResidencyStatus == GpuResidency::Resident );
        ++mPendingResidencyChanges;
        mTextureManager->_queueDownloadToRam( this, true );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::waitForPendingSyncs()
    {
        if( isDataReady() )
            return;

        mTextureManager->_waitForPendingGpuToCpuSyncs( this );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::_notifySysRamDownloadIsReady( uint8 *sysRamPtr, bool resyncOnly )
    {
        OGRE_ASSERT_LOW( mResidencyStatus == GpuResidency::Resident &&
                         "GPU should be resident so the copy could've been made "
                         "and so we can perform the transition!" );

        TextureGpuListener::Reason listenerReason = TextureGpuListener::Unknown;

        if( !resyncOnly )
        {
            OGRE_ASSERT_LOW( mPageOutStrategy != GpuPageOutStrategy::AlwaysKeepSystemRamCopy &&
                             "This path should've never been hit as always have the RAM copy!" );
            OGRE_ASSERT_LOW( mSysRamCopy == (uint8 *)0 );

            destroyInternalResourcesImpl();

            mSysRamCopy = sysRamPtr;
            mResidencyStatus = GpuResidency::OnSystemRam;

            listenerReason = TextureGpuListener::LostResidency;
        }
        else
        {
            OGRE_ASSERT_LOW( mPageOutStrategy == GpuPageOutStrategy::AlwaysKeepSystemRamCopy &&
                             "This path should only hit if we always have the RAM copy!" );
            OGRE_ASSERT_LOW( mSysRamCopy != (uint8 *)0 );

            listenerReason = TextureGpuListener::ResidentToSysRamSync;
        }

        // Decrement mPendingResidencyChanges and prevent underflow
        mPendingResidencyChanges = std::max( mPendingResidencyChanges, 1u ) - 1u;

        notifyAllListenersTextureChanged( listenerReason );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::copyTo( TextureGpu *dst, const TextureBox &dstBox, uint8 dstMipLevel,
                             const TextureBox &srcBox, uint8 srcMipLevel, bool keepResolvedTexSynced,
                             CopyEncTransitionMode::CopyEncTransitionMode srcTransitionMode,
                             CopyEncTransitionMode::CopyEncTransitionMode dstTransitionMode )
    {
        assert( srcBox.equalSize( dstBox ) );
        assert( this != dst || !srcBox.overlaps( dstBox ) );
        assert( srcMipLevel < this->getNumMipmaps() && dstMipLevel < dst->getNumMipmaps() );
        OGRE_ASSERT_LOW( ( this->getOrientationMode() & 0x01 ) == ( dst->getOrientationMode() & 0x01 ) );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::_setDepthBufferDefaults( uint16 depthBufferPoolId, bool preferDepthTexture,
                                              PixelFormatGpu desiredDepthBufferFormat )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL,
                     "Texture must've been created with TextureFlags::RenderToTexture!",
                     "TextureGpu::_setDepthBufferDefaults" );
    }
    //-----------------------------------------------------------------------------------
    uint16 TextureGpu::getDepthBufferPoolId() const
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL,
                     "Texture must've been created with TextureFlags::RenderToTexture!",
                     "TextureGpu::getDepthBufferPoolId" );
        return 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::getPreferDepthTexture() const
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL,
                     "Texture must've been created with TextureFlags::RenderToTexture!",
                     "TextureGpu::getPreferDepthTexture" );
        return false;
    }
    //-----------------------------------------------------------------------------------
    PixelFormatGpu TextureGpu::getDesiredDepthBufferFormat() const
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL,
                     "Texture must've been created with TextureFlags::RenderToTexture!",
                     "TextureGpu::getDesiredDepthBufferFormat" );
        return PFG_UNKNOWN;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::_resolveTo( TextureGpu *resolveTexture )
    {
        if( !this->isMultisample() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Source Texture must be MSAA",
                         "TextureGpu::_resolveTo" );
        }
        if( this->isOpenGLRenderWindow() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "OpenGL MSAA RenderWindows cannot be resolved",
                         "TextureGpu::_resolveTo" );
        }
        if( PixelFormatGpuUtils::isDepth( this->getPixelFormat() ) )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Depth formats cannot be resolved!",
                         "TextureGpu::_resolveTo" );
        }
        if( this->getPixelFormat() != resolveTexture->getPixelFormat() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Source Texture and Resolve texture must have the same pixel formats!",
                         "TextureGpu::_resolveTo" );
        }
        if( !this->getEmptyBox( 0 ).equalSize( resolveTexture->getEmptyBox( 0 ) ) )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Source Texture and Resolve texture must have the same dimensions!",
                         "TextureGpu::_resolveTo" );
        }
        if( resolveTexture->isMultisample() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Resolve Texture must not be MSAA",
                         "TextureGpu::_resolveTo" );
        }
        if( !resolveTexture->isRenderToTexture() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Resolve Texture must be created with TextureFlags::RenderToTexture",
                         "TextureGpu::_resolveTo" );
        }

        RenderSystem *renderSystem = mTextureManager->getRenderSystem();
        RenderPassDescriptor *renderPassDescriptor = renderSystem->createRenderPassDescriptor();
        renderPassDescriptor->mColour[0].texture = this;
        renderPassDescriptor->mColour[0].resolveTexture = resolveTexture;

        renderPassDescriptor->mColour[0].loadAction = LoadAction::Load;
        // Set to both, because we don't want to lose the contents from this RTT.
        renderPassDescriptor->mColour[0].storeAction = StoreAction::StoreAndMultisampleResolve;
        renderPassDescriptor->entriesModified( RenderPassDescriptor::All );

        Vector4 fullVp( 0, 0, 1, 1 );
        renderSystem->beginRenderPassDescriptor( renderPassDescriptor, this, 0, &fullVp, &fullVp, 1u,
                                                 false, false );
        renderSystem->executeRenderPassDescriptorDelayedActions();
        renderSystem->endRenderPassDescriptor();
        renderSystem->destroyRenderPassDescriptor( renderPassDescriptor );
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::hasAutomaticBatching() const
    {
        return ( mTextureFlags & TextureFlags::AutomaticBatching ) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isTexture() const { return ( mTextureFlags & TextureFlags::NotTexture ) == 0; }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isRenderToTexture() const
    {
        return ( mTextureFlags & TextureFlags::RenderToTexture ) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isUav() const { return ( mTextureFlags & TextureFlags::Uav ) != 0; }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::allowsAutoMipmaps() const
    {
        return ( mTextureFlags & TextureFlags::AllowAutomipmaps ) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::hasAutoMipmapAuto() const
    {
        return ( mTextureFlags & TextureFlags::AutomipmapsAuto ) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::hasMsaaExplicitResolves() const
    {
        return ( mTextureFlags & TextureFlags::MsaaExplicitResolve ) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isReinterpretable() const
    {
        return ( mTextureFlags & TextureFlags::Reinterpretable ) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::prefersLoadingFromFileAsSRGB() const
    {
        return ( mTextureFlags & TextureFlags::PrefersLoadingFromFileAsSRGB ) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isRenderWindowSpecific() const
    {
        return ( mTextureFlags & TextureFlags::RenderWindowSpecific ) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::requiresTextureFlipping() const
    {
        return ( mTextureFlags & TextureFlags::RequiresTextureFlipping ) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::_isManualTextureFlagPresent() const
    {
        return ( mTextureFlags & TextureFlags::ManualTexture ) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isManualTexture() const
    {
        return ( mTextureFlags & ( TextureFlags::NotTexture | TextureFlags::Uav |
                                   TextureFlags::RenderToTexture | TextureFlags::ManualTexture ) ) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isPoolOwner() const { return ( mTextureFlags & TextureFlags::PoolOwner ) != 0; }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isDiscardableContent() const
    {
        return ( mTextureFlags & TextureFlags::DiscardableContent ) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isOpenGLRenderWindow() const { return false; }
    //-----------------------------------------------------------------------------------
    void TextureGpu::setOrientationMode( OrientationMode orientationMode )
    {
        OGRE_ASSERT_LOW( mResidencyStatus == GpuResidency::OnStorage || isRenderWindowSpecific() );
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "setOrientationMode must be done on a RenderTexture!",
                     "TextureGpu::setOrientationMode" );
    }
    //-----------------------------------------------------------------------------------
    OrientationMode TextureGpu::getOrientationMode() const { return OR_DEGREE_0; }
    //-----------------------------------------------------------------------------------
    ResourceLayout::Layout TextureGpu::getDefaultLayout( bool bIgnoreDiscardableFlag ) const
    {
        if( isDiscardableContent() && !bIgnoreDiscardableFlag )
            return ResourceLayout::Undefined;
        else if( isPoolOwner() )
            return ResourceLayout::Texture;
        else if( isRenderToTexture() )
            return ResourceLayout::RenderTarget;
        else if( isUav() )
            return ResourceLayout::Uav;

        return ResourceLayout::Texture;
    }
    //-----------------------------------------------------------------------------------
    ResourceLayout::Layout TextureGpu::getCurrentLayout() const { return getDefaultLayout(); }
    //-----------------------------------------------------------------------------------
    void TextureGpu::_setNextLayout( ResourceLayout::Layout layout )
    {
        OGRE_ASSERT_LOW( layout != ResourceLayout::CopyEncoderManaged &&
                         "Cannot explicitly transition CopyEncoderManaged layouts" );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::_notifyTextureSlotChanged( const TexturePool *newPool, uint16 slice )
    {
        mTexturePool = newPool;
        mInternalSliceStart = slice;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::setTexturePoolId( uint32 poolId )
    {
        OGRE_ASSERT_LOW( mResidencyStatus != GpuResidency::Resident );
        mPoolId = poolId;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::addListener( TextureGpuListener *listener ) { mListeners.push_back( listener ); }
    //-----------------------------------------------------------------------------------
    void TextureGpu::removeListener( TextureGpuListener *listener )
    {
        vector<TextureGpuListener *>::type::iterator itor =
            std::find( mListeners.begin(), mListeners.end(), listener );
        assert( itor != mListeners.end() );
        efficientVectorRemove( mListeners, itor );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::notifyAllListenersTextureChanged( uint32 _reason, void *extraData )
    {
        TextureGpuListener::Reason reason = static_cast<TextureGpuListener::Reason>( _reason );

        // Iterate through a copy in case one of the listeners decides to remove itself.
        vector<TextureGpuListener *>::type listenersVec = mListeners;
        vector<TextureGpuListener *>::type::iterator itor = listenersVec.begin();
        vector<TextureGpuListener *>::type::iterator endt = listenersVec.end();

        while( itor != endt )
        {
            ( *itor )->notifyTextureChanged( this, reason, extraData );
            ++itor;
        }

        if( mTextureManager )
            mTextureManager->notifyTextureChanged( this, reason, extraData );
    }
    //-----------------------------------------------------------------------------------
    const vector<TextureGpuListener *>::type &TextureGpu::getListeners() const { return mListeners; }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::supportsAsDepthBufferFor( TextureGpu *colourTarget ) const
    {
        assert( PixelFormatGpuUtils::isDepth( this->mPixelFormat ) );
        assert( !PixelFormatGpuUtils::isDepth( colourTarget->mPixelFormat ) );

        if( this->getInternalWidth() == colourTarget->getInternalWidth() &&
            this->getInternalHeight() == colourTarget->getInternalHeight() &&
            this->getSampleDescription() == colourTarget->getSampleDescription() &&
            this->isTilerMemoryless() ==
                ( colourTarget->isTilerMemoryless() || colourTarget->isTilerDepthMemoryless() ) &&
            this->isRenderWindowSpecific() == colourTarget->isRenderWindowSpecific() )
        {
            return true;
        }

        return false;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::writeContentsToFile( const String &filename, uint8 minMip, uint8 maxMip,
                                          bool automaticResolve )
    {
        Image2 image;
        image.convertFromTexture( this, minMip, maxMip, automaticResolve );
        image.save( filename, 0, image.getNumMipmaps() );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::copyContentsToMemory( TextureBox src, TextureBox dst, PixelFormatGpu dstFormat,
                                           bool automaticResolve )
    {
        Image2::copyContentsToMemory( this, src, dst, dstFormat, automaticResolve );
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isTextureGpu() const { return true; }
    //-----------------------------------------------------------------------------------
    TextureGpuManager *TextureGpu::getTextureManager() const { return mTextureManager; }
    //-----------------------------------------------------------------------------------
    TextureBox TextureGpu::getEmptyBox( uint8 mipLevel )
    {
        TextureBox retVal( std::max( 1u, mWidth >> mipLevel ), std::max( 1u, mHeight >> mipLevel ),
                           std::max( 1u, getDepth() >> mipLevel ), getNumSlices(),
                           PixelFormatGpuUtils::getBytesPerPixel( mPixelFormat ),
                           this->_getSysRamCopyBytesPerRow( mipLevel ),
                           this->_getSysRamCopyBytesPerImage( mipLevel ) );
        if( PixelFormatGpuUtils::isCompressed( mPixelFormat ) )
            retVal.setCompressedPixelFormat( mPixelFormat );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    TextureBox TextureGpu::_getSysRamCopyAsBox( uint8 mipLevel )
    {
        if( !mSysRamCopy )
            return TextureBox();

        assert( mipLevel < mNumMipmaps );

        waitForPendingSyncs();

        uint32 width = mWidth;
        uint32 height = mHeight;
        uint32 depth = getDepth();
        const uint32 numSlices = getNumSlices();
        void *data = PixelFormatGpuUtils::advancePointerToMip( mSysRamCopy, width, height, depth,
                                                               numSlices, mipLevel, mPixelFormat );

        TextureBox retVal( std::max( 1u, width >> mipLevel ),   //
                           std::max( 1u, height >> mipLevel ),  //
                           std::max( 1u, depth >> mipLevel ),   //
                           numSlices,                           //
                           PixelFormatGpuUtils::getBytesPerPixel( mPixelFormat ),
                           this->_getSysRamCopyBytesPerRow( mipLevel ),
                           this->_getSysRamCopyBytesPerImage( mipLevel ) );
        retVal.data = data;
        if( PixelFormatGpuUtils::isCompressed( mPixelFormat ) )
            retVal.setCompressedPixelFormat( mPixelFormat );
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    uint8 *TextureGpu::_getSysRamCopy( uint8 mipLevel )
    {
        if( !mSysRamCopy )
            return 0;

        assert( mipLevel < mNumMipmaps );

        waitForPendingSyncs();

        uint32 width = mWidth;
        uint32 height = mHeight;
        uint32 depth = getDepth();
        const uint32 numSlices = getNumSlices();
        void *data = PixelFormatGpuUtils::advancePointerToMip( mSysRamCopy, width, height, depth,
                                                               numSlices, mipLevel, mPixelFormat );
        return reinterpret_cast<uint8 *>( data );
    }
    //-----------------------------------------------------------------------------------
    uint32 TextureGpu::_getSysRamCopyBytesPerRow( uint8 mipLevel )
    {
        assert( mipLevel < mNumMipmaps );
        uint32 width = std::max( mWidth >> mipLevel, 1u );
        return (uint32)PixelFormatGpuUtils::getSizeBytes( width, 1u, 1u, 1u, mPixelFormat, 4u );
    }
    //-----------------------------------------------------------------------------------
    size_t TextureGpu::_getSysRamCopyBytesPerImage( uint8 mipLevel )
    {
        assert( mipLevel < mNumMipmaps );
        uint32 width = std::max( mWidth >> mipLevel, 1u );
        uint32 height = std::max( mHeight >> mipLevel, 1u );
        return PixelFormatGpuUtils::getSizeBytes( width, height, 1u, 1u, mPixelFormat, 4u );
    }
    //-----------------------------------------------------------------------------------
    size_t TextureGpu::getSizeBytes() const
    {
        if( mResidencyStatus == GpuResidency::OnStorage )
            return 0;

        size_t sizeBytes = PixelFormatGpuUtils::calculateSizeBytes(
            mWidth, mHeight, getDepth(), getNumSlices(), mPixelFormat, mNumMipmaps, 4u );
        if( mSampleDescription.isMultisample() )
        {
            if( hasMsaaExplicitResolves() )
                sizeBytes *= mSampleDescription.getColourSamples();
            else
                sizeBytes *= ( mSampleDescription.getColourSamples() + 1u );
        }

        return sizeBytes;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isMetadataReady() const
    {
        return ( ( mResidencyStatus == GpuResidency::Resident &&
                   mNextResidencyStatus == GpuResidency::Resident ) ||
                 ( mResidencyStatus == GpuResidency::OnSystemRam &&
                   mNextResidencyStatus != GpuResidency::OnStorage ) ) &&
               mPendingResidencyChanges == 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isDataReady() const { return _isDataReadyImpl() && mPendingResidencyChanges == 0u; }
    //-----------------------------------------------------------------------------------
    void TextureGpu::waitForMetadata()
    {
        if( isMetadataReady() )
            return;
        mTextureManager->_waitFor( this, true );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::waitForData()
    {
        if( isDataReady() )
            return;
        mTextureManager->_waitFor( this, false );
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    TextureGpuListener::~TextureGpuListener() {}
}  // namespace Ogre
