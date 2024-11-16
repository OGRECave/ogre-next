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

#include "OgreMetalTextureGpu.h"

#include "OgreException.h"
#include "OgreMetalDevice.h"
#include "OgreMetalMappings.h"
#include "OgreMetalTextureGpuManager.h"
#include "OgreRenderSystem.h"
#include "OgreStringConverter.h"
#include "OgreTextureBox.h"
#include "OgreTextureGpuListener.h"
#include "OgreVector2.h"
#include "Vao/OgreVaoManager.h"

#import "Metal/MTLBlitCommandEncoder.h"

namespace Ogre
{
    MetalTextureGpu::MetalTextureGpu( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                      VaoManager *vaoManager, IdString name, uint32 textureFlags,
                                      TextureTypes::TextureTypes initialType,
                                      TextureGpuManager *textureManager ) :
        TextureGpu( pageOutStrategy, vaoManager, name, textureFlags, initialType, textureManager ),
        mDisplayTextureName( 0 ),
        mFinalTextureName( 0 ),
        mMsaaFramebufferName( 0 )
    {
        _setToDisplayDummyTexture();
    }
    //-----------------------------------------------------------------------------------
    MetalTextureGpu::~MetalTextureGpu() { destroyInternalResourcesImpl(); }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::createInternalResourcesImpl()
    {
        if( mPixelFormat == PFG_NULL )
            return;  // Nothing to do

        MetalTextureGpuManager *textureManagerMetal =
            static_cast<MetalTextureGpuManager *>( mTextureManager );
        MetalDevice *device = textureManagerMetal->getDevice();

        MTLTextureDescriptor *desc = [MTLTextureDescriptor new];
        desc.mipmapLevelCount = mNumMipmaps;
        desc.textureType = getMetalTextureType();
        desc.width = mWidth;
        desc.height = mHeight;
        desc.depth = getDepth();
        desc.arrayLength = getNumSlices();
        desc.pixelFormat = MetalMappings::get( mPixelFormat, device );
        desc.sampleCount = 1u;
        desc.storageMode = MTLStorageModePrivate;

        if( mTextureType == TextureTypes::TypeCube || mTextureType == TextureTypes::TypeCubeArray )
            desc.arrayLength /= 6u;

        const RenderSystemCapabilities *capabilities =
            mTextureManager->getRenderSystem()->getCapabilities();
        const bool isTiler = capabilities->hasCapability( RSC_IS_TILER );
        if( isTiler && isTilerMemoryless() )
        {
            if( @available( iOS 10, macOS 11, * ) )
                desc.storageMode = MTLStorageModeMemoryless;
        }
        if( isMultisample() && hasMsaaExplicitResolves() )
        {
            desc.textureType = MTLTextureType2DMultisample;
            desc.sampleCount = mSampleDescription.getColourSamples();
        }

        desc.usage = MTLTextureUsageShaderRead;
        if( isRenderToTexture() )
            desc.usage |= MTLTextureUsageRenderTarget;
        if( isUav() )
            desc.usage |= MTLTextureUsageShaderWrite;
        if( isReinterpretable() )
            desc.usage |= MTLTextureUsagePixelFormatView;

        String textureName = getNameStr();

        mFinalTextureName = [device->mDevice newTextureWithDescriptor:desc];
        if( !mFinalTextureName )
        {
            size_t sizeBytes = PixelFormatGpuUtils::calculateSizeBytes(
                mWidth, mHeight, getDepth(), getNumSlices(), mPixelFormat, mNumMipmaps, 4u );
            if( isMultisample() && hasMsaaExplicitResolves() )
                sizeBytes *= mSampleDescription.getColourSamples();
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Out of GPU memory or driver refused.\n"
                         "Requested: " +
                             StringConverter::toString( sizeBytes ) + " bytes.",
                         "MetalTextureGpu::createInternalResourcesImpl" );
        }
        mFinalTextureName.label = @( textureName.c_str() );

        if( isMultisample() && !hasMsaaExplicitResolves() )
        {
            if( isTiler )
            {
                // mMsaaFramebufferName is always Memoryless because the user *NEVER* has access to it
                // and we always auto-resolve in the same pass, thus MSAA contents are always transient.
                if( @available( iOS 10, macOS 11, * ) )
                    desc.storageMode = MTLStorageModeMemoryless;
            }
            desc.textureType = MTLTextureType2DMultisample;
            desc.depth = 1u;
            desc.arrayLength = 1u;
            desc.sampleCount = mSampleDescription.getColourSamples();
            desc.usage = MTLTextureUsageRenderTarget;
            mMsaaFramebufferName = [device->mDevice newTextureWithDescriptor:desc];
            if( !mMsaaFramebufferName )
            {
                size_t sizeBytes = PixelFormatGpuUtils::calculateSizeBytes(
                    mWidth, mHeight, getDepth(), getNumSlices(), mPixelFormat, mNumMipmaps, 4u );
                sizeBytes *= mSampleDescription.getColourSamples();
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "Out of GPU memory or driver refused (MSAA surface).\n"
                             "Requested: " +
                                 StringConverter::toString( sizeBytes ) + " bytes.",
                             "MetalTextureGpu::createInternalResourcesImpl" );
            }
            mMsaaFramebufferName.label = @( ( textureName + "_MSAA" ).c_str() );
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::destroyInternalResourcesImpl()
    {
        if( mFinalTextureName )
            mFinalTextureName = 0;
        if( mMsaaFramebufferName )
            mMsaaFramebufferName = 0;

        if( hasAutomaticBatching() )
        {
            if( mTexturePool )
            {
                // This will end up calling _notifyTextureSlotChanged,
                // setting mTexturePool & mInternalSliceStart to 0
                mTextureManager->_releaseSlotFromTexture( this );
            }
        }

        _setToDisplayDummyTexture();
    }
    //-----------------------------------------------------------------------------------
    MTLTextureType MetalTextureGpu::getMetalTextureType() const
    {
        switch( mTextureType )
        {
        case TextureTypes::Unknown:
            return MTLTextureType2D;
        case TextureTypes::Type1D:
            return MTLTextureType1D;
        case TextureTypes::Type1DArray:
            return MTLTextureType1DArray;
        case TextureTypes::Type2D:
            return MTLTextureType2D;
        case TextureTypes::Type2DArray:
            return MTLTextureType2DArray;
        case TextureTypes::TypeCube:
            return MTLTextureTypeCube;
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS || __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_11_0
        case TextureTypes::TypeCubeArray:
            return MTLTextureTypeCubeArray;
#endif
        case TextureTypes::Type3D:
            return MTLTextureType3D;
        default:
            return (MTLTextureType)999;
        };
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::notifyDataIsReady()
    {
        assert( mResidencyStatus == GpuResidency::Resident );
        assert( mFinalTextureName || mPixelFormat == PFG_NULL );

        OGRE_ASSERT_LOW( mDataPreparationsPending > 0u &&
                         "Calling notifyDataIsReady too often! Remove this call"
                         "See https://github.com/OGRECave/ogre-next/issues/101" );
        --mDataPreparationsPending;

        mDisplayTextureName = mFinalTextureName;

        notifyAllListenersTextureChanged( TextureGpuListener::ReadyForRendering );
    }
    //-----------------------------------------------------------------------------------
    bool MetalTextureGpu::_isDataReadyImpl() const
    {
        return mDisplayTextureName == mFinalTextureName && mDataPreparationsPending == 0u;
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::_setToDisplayDummyTexture()
    {
        if( !mTextureManager )
        {
            assert( isRenderWindowSpecific() );
            return;  // This can happen if we're a window and we're on shutdown
        }

        MetalTextureGpuManager *textureManagerMetal =
            static_cast<MetalTextureGpuManager *>( mTextureManager );
        if( hasAutomaticBatching() )
        {
            mDisplayTextureName =
                textureManagerMetal->getBlankTextureMetalName( TextureTypes::Type2DArray );
        }
        else
        {
            mDisplayTextureName = textureManagerMetal->getBlankTextureMetalName( mTextureType );
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::_notifyTextureSlotChanged( const TexturePool *newPool, uint16 slice )
    {
        TextureGpu::_notifyTextureSlotChanged( newPool, slice );

        _setToDisplayDummyTexture();

        if( mTexturePool )
        {
            assert( dynamic_cast<MetalTextureGpu *>( mTexturePool->masterTexture ) );
            MetalTextureGpu *masterTexture =
                static_cast<MetalTextureGpu *>( mTexturePool->masterTexture );
            mFinalTextureName = masterTexture->mFinalTextureName;
        }

        notifyAllListenersTextureChanged( TextureGpuListener::PoolTextureSlotChanged );
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::setTextureType( TextureTypes::TextureTypes textureType )
    {
        const TextureTypes::TextureTypes oldType = mTextureType;
        TextureGpu::setTextureType( textureType );

        if( oldType != mTextureType && mDisplayTextureName != mFinalTextureName )
            _setToDisplayDummyTexture();
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::copyTo( TextureGpu *dst, const TextureBox &dstBox, uint8 dstMipLevel,
                                  const TextureBox &srcBox, uint8 srcMipLevel,
                                  bool keepResolvedTexSynced,
                                  CopyEncTransitionMode::CopyEncTransitionMode srcTransitionMode,
                                  CopyEncTransitionMode::CopyEncTransitionMode dstTransitionMode )
    {
        TextureGpu::copyTo( dst, dstBox, dstMipLevel, srcBox, srcMipLevel, srcTransitionMode,
                            dstTransitionMode );

        assert( dynamic_cast<MetalTextureGpu *>( dst ) );

        MetalTextureGpu *dstMetal = static_cast<MetalTextureGpu *>( dst );
        MetalTextureGpuManager *textureManagerMetal =
            static_cast<MetalTextureGpuManager *>( mTextureManager );
        MetalDevice *device = textureManagerMetal->getDevice();

        uint32 numSlices = srcBox.numSlices;

        id<MTLTexture> srcTextureName = this->mFinalTextureName;
        id<MTLTexture> dstTextureName = dstMetal->mFinalTextureName;

        if( this->isMultisample() && !this->hasMsaaExplicitResolves() )
            srcTextureName = this->mMsaaFramebufferName;
        if( dstMetal->isMultisample() && !dstMetal->hasMsaaExplicitResolves() )
            dstTextureName = dstMetal->mMsaaFramebufferName;

        __unsafe_unretained id<MTLBlitCommandEncoder> blitEncoder = device->getBlitEncoder();
        for( uint32 slice = 0; slice < numSlices; ++slice )
        {
            [blitEncoder copyFromTexture:srcTextureName
                             sourceSlice:srcBox.sliceStart + this->getInternalSliceStart() + slice
                             sourceLevel:srcMipLevel
                            sourceOrigin:MTLOriginMake( srcBox.x, srcBox.y, srcBox.z )
                              sourceSize:MTLSizeMake( srcBox.width, srcBox.height, srcBox.depth )
                               toTexture:dstTextureName
                        destinationSlice:dstBox.sliceStart + dstMetal->getInternalSliceStart() + slice
                        destinationLevel:dstMipLevel
                       destinationOrigin:MTLOriginMake( dstBox.x, dstBox.y, dstBox.z )];
        }

        // Must keep the resolved texture up to date.
        if( dstMetal->isMultisample() && !dstMetal->hasMsaaExplicitResolves() && keepResolvedTexSynced )
        {
            device->endAllEncoders();

            MTLRenderPassDescriptor *passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
            passDesc.colorAttachments[0].texture = dstMetal->mMsaaFramebufferName;
            passDesc.colorAttachments[0].resolveTexture = dstMetal->mFinalTextureName;
            passDesc.colorAttachments[0].loadAction = MTLLoadActionLoad;
            passDesc.colorAttachments[0].storeAction = MTLStoreActionMultisampleResolve;

            for( uint32 slice = 0; slice < numSlices; ++slice )
            {
                passDesc.colorAttachments[0].slice =
                    dstBox.sliceStart + dstMetal->getInternalSliceStart() + slice;
                passDesc.colorAttachments[0].resolveSlice =
                    dstBox.sliceStart + dstMetal->getInternalSliceStart() + slice;
                passDesc.colorAttachments[0].level = dstMipLevel;
                passDesc.colorAttachments[0].resolveLevel = dstMipLevel;
            }

            device->mRenderEncoder =
                [device->mCurrentCommandBuffer renderCommandEncoderWithDescriptor:passDesc];
            device->endRenderEncoder( false );
        }

        // Do not perform the sync if notifyDataIsReady hasn't been called yet (i.e. we're
        // still building the HW mipmaps, and the texture will never be ready)
        if( dst->_isDataReadyImpl() &&
            dst->getGpuPageOutStrategy() == GpuPageOutStrategy::AlwaysKeepSystemRamCopy )
        {
            dst->_syncGpuResidentToSystemRam();
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::_autogenerateMipmaps( CopyEncTransitionMode::CopyEncTransitionMode
                                                /*transitionMode*/ )
    {
        MetalTextureGpuManager *textureManagerMetal =
            static_cast<MetalTextureGpuManager *>( mTextureManager );
        MetalDevice *device = textureManagerMetal->getDevice();
        __unsafe_unretained id<MTLBlitCommandEncoder> blitEncoder = device->getBlitEncoder();
        [blitEncoder generateMipmapsForTexture:mFinalTextureName];
    }
    //-----------------------------------------------------------------------------------
    id<MTLTexture> MetalTextureGpu::getView( PixelFormatGpu pixelFormat, uint8 mipLevel,
                                             uint8 numMipmaps, uint16 arraySlice,
                                             bool cubemapsAs2DArrays, bool forUav )
    {
        if( pixelFormat == PFG_UNKNOWN )
        {
            pixelFormat = mPixelFormat;
            if( forUav )
                pixelFormat = PixelFormatGpuUtils::getEquivalentLinear( pixelFormat );
        }
        MTLTextureType texType = this->getMetalTextureType();

        if( isMultisample() && hasMsaaExplicitResolves() )
            texType = MTLTextureType2DMultisample;

        if( ( cubemapsAs2DArrays || forUav ) &&          //
            ( mTextureType == TextureTypes::TypeCube ||  //
              mTextureType == TextureTypes::TypeCubeArray ) )
        {
            texType = MTLTextureType2DArray;
        }

        if( !numMipmaps )
            numMipmaps = mNumMipmaps - mipLevel;

        OGRE_ASSERT_LOW( numMipmaps <= mNumMipmaps - mipLevel &&
                         "Asking for more mipmaps than the texture has!" );

        NSRange mipLevels;
        mipLevels = NSMakeRange( mipLevel, numMipmaps );
        NSRange slices;
        slices = NSMakeRange( arraySlice, this->getNumSlices() - arraySlice );

        MetalTextureGpuManager *textureManagerMetal =
            static_cast<MetalTextureGpuManager *>( mTextureManager );
        MetalDevice *device = textureManagerMetal->getDevice();

        return
            [mDisplayTextureName newTextureViewWithPixelFormat:MetalMappings::get( pixelFormat, device )
                                                   textureType:texType
                                                        levels:mipLevels
                                                        slices:slices];
    }
    //-----------------------------------------------------------------------------------
    id<MTLTexture> MetalTextureGpu::getView( DescriptorSetTexture2::TextureSlot texSlot )
    {
        return getView( texSlot.pixelFormat, texSlot.mipmapLevel, texSlot.numMipmaps,
                        texSlot.textureArrayIndex, texSlot.cubemapsAs2DArrays, false );
    }
    //-----------------------------------------------------------------------------------
    id<MTLTexture> MetalTextureGpu::getView( DescriptorSetUav::TextureSlot texSlot )
    {
        return getView( texSlot.pixelFormat, texSlot.mipmapLevel, 1u, texSlot.textureArrayIndex, false,
                        true );
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::getSubsampleLocations( vector<Vector2>::type locations )
    {
        locations.reserve( mSampleDescription.getColourSamples() );
        if( mSampleDescription.getColourSamples() <= 1u )
        {
            locations.push_back( Vector2( 0.0f, 0.0f ) );
        }
        else
        {
            assert( mSampleDescription.getMsaaPattern() != MsaaPatterns::Undefined );

            // TODO
        }
    }

    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::getCustomAttribute( IdString name, void *pData )
    {
        if( name == msFinalTextureBuffer )
        {
            *static_cast<const void **>( pData ) = (const void *)CFBridgingRetain( mFinalTextureName );
        }
        else if( name == msMsaaTextureBuffer )
        {
            *static_cast<const void **>( pData ) =
                (const void *)CFBridgingRetain( mMsaaFramebufferName );
        }
    }

    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    MetalTextureGpuRenderTarget::MetalTextureGpuRenderTarget(
        GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy, VaoManager *vaoManager, IdString name,
        uint32 textureFlags, TextureTypes::TextureTypes initialType,
        TextureGpuManager *textureManager ) :
        MetalTextureGpu( pageOutStrategy, vaoManager, name, textureFlags, initialType, textureManager ),
        mDepthBufferPoolId( 1u ),
        mPreferDepthTexture( false ),
        mDesiredDepthBufferFormat( PFG_UNKNOWN )
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        ,
        mOrientationMode( msDefaultOrientationMode )
#endif
    {
        if( mPixelFormat == PFG_NULL )
            mDepthBufferPoolId = 0;
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpuRenderTarget::_setDepthBufferDefaults( uint16 depthBufferPoolId,
                                                               bool preferDepthTexture,
                                                               PixelFormatGpu desiredDepthBufferFormat )
    {
        assert( isRenderToTexture() );
        OGRE_ASSERT_MEDIUM( mSourceType != TextureSourceType::SharedDepthBuffer &&
                            "Cannot call _setDepthBufferDefaults on a shared depth buffer!" );
        mDepthBufferPoolId = depthBufferPoolId;
        mPreferDepthTexture = preferDepthTexture;
        mDesiredDepthBufferFormat = desiredDepthBufferFormat;
    }
    //-----------------------------------------------------------------------------------
    uint16 MetalTextureGpuRenderTarget::getDepthBufferPoolId() const { return mDepthBufferPoolId; }
    //-----------------------------------------------------------------------------------
    bool MetalTextureGpuRenderTarget::getPreferDepthTexture() const { return mPreferDepthTexture; }
    //-----------------------------------------------------------------------------------
    PixelFormatGpu MetalTextureGpuRenderTarget::getDesiredDepthBufferFormat() const
    {
        return mDesiredDepthBufferFormat;
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpuRenderTarget::setOrientationMode( OrientationMode orientationMode )
    {
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        mOrientationMode = orientationMode;
#endif
    }
    //-----------------------------------------------------------------------------------
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
    OrientationMode MetalTextureGpuRenderTarget::getOrientationMode() const { return mOrientationMode; }
#endif
}  // namespace Ogre
