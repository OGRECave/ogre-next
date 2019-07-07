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

#include "OgreMetalTextureGpu.h"
#include "OgreMetalMappings.h"
#include "OgreMetalTextureGpuManager.h"

#include "OgreTextureGpuListener.h"
#include "OgreTextureBox.h"
#include "OgreVector2.h"

#include "Vao/OgreVaoManager.h"

#include "OgreMetalDevice.h"
#include "OgreStringConverter.h"
#include "OgreException.h"

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
    MetalTextureGpu::~MetalTextureGpu()
    {
        destroyInternalResourcesImpl();
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::createInternalResourcesImpl(void)
    {
        if( mPixelFormat == PFG_NULL )
            return; //Nothing to do

        MTLTextureDescriptor *desc = [MTLTextureDescriptor new];
        desc.mipmapLevelCount   = mNumMipmaps;
        desc.textureType        = getMetalTextureType();
        desc.width              = mWidth;
        desc.height             = mHeight;
        desc.depth              = getDepth();
        desc.arrayLength        = getNumSlices();
        desc.pixelFormat        = MetalMappings::get( mPixelFormat );
        desc.sampleCount        = 1u;
        desc.storageMode        = MTLStorageModePrivate;

        if( mTextureType == TextureTypes::TypeCube )
            desc.arrayLength /= 6u;

        if( mMsaa > 1u && hasMsaaExplicitResolves() )
        {
            desc.textureType = MTLTextureType2DMultisample;
            desc.sampleCount = mMsaa;
        }

        desc.usage = MTLTextureUsageShaderRead;
        if( isRenderToTexture() )
            desc.usage |= MTLTextureUsageRenderTarget;
        if( isUav() )
            desc.usage |= MTLTextureUsageShaderWrite;
        if( isReinterpretable() )
            desc.usage |= MTLTextureUsagePixelFormatView;

        String textureName = getNameStr();

        MetalTextureGpuManager *textureManagerMetal =
                static_cast<MetalTextureGpuManager*>( mTextureManager );
        MetalDevice *device = textureManagerMetal->getDevice();

        mFinalTextureName = [device->mDevice newTextureWithDescriptor:desc];
        if( !mFinalTextureName )
        {
            size_t sizeBytes = PixelFormatGpuUtils::calculateSizeBytes( mWidth, mHeight, getDepth(),
                                                                        getNumSlices(),
                                                                        mPixelFormat, mNumMipmaps, 4u );
            if( mMsaa > 1u && hasMsaaExplicitResolves() )
                sizeBytes *= mMsaa;
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Out of GPU memory or driver refused.\n"
                         "Requested: " + StringConverter::toString( sizeBytes ) + " bytes.",
                         "MetalTextureGpu::createInternalResourcesImpl" );
        }
        mFinalTextureName.label = [NSString stringWithUTF8String:textureName.c_str()];

        if( mMsaa > 1u && !hasMsaaExplicitResolves() )
        {
            desc.textureType    = MTLTextureType2DMultisample;
            desc.depth          = 1u;
            desc.arrayLength    = 1u;
            desc.sampleCount    = mMsaa;
            desc.usage          = MTLTextureUsageRenderTarget;
            mMsaaFramebufferName = [device->mDevice newTextureWithDescriptor:desc];
            if( !mMsaaFramebufferName )
            {
                size_t sizeBytes = PixelFormatGpuUtils::calculateSizeBytes( mWidth, mHeight, getDepth(),
                                                                            getNumSlices(),
                                                                            mPixelFormat, mNumMipmaps, 4u );
                sizeBytes *= mMsaa;
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "Out of GPU memory or driver refused (MSAA surface).\n"
                             "Requested: " + StringConverter::toString( sizeBytes ) + " bytes.",
                             "MetalTextureGpu::createInternalResourcesImpl" );
            }
            mMsaaFramebufferName.label = [NSString stringWithUTF8String:(textureName + "_MSAA").c_str()];
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::destroyInternalResourcesImpl(void)
    {
        if( mFinalTextureName )
            mFinalTextureName = 0;
        if( mMsaaFramebufferName )
            mMsaaFramebufferName = 0;

        if( hasAutomaticBatching() )
        {
            if( mTexturePool )
            {
                //This will end up calling _notifyTextureSlotChanged,
                //setting mTexturePool & mInternalSliceStart to 0
                mTextureManager->_releaseSlotFromTexture( this );
            }
        }

        _setToDisplayDummyTexture();
    }
    //-----------------------------------------------------------------------------------
    MTLTextureType MetalTextureGpu::getMetalTextureType(void) const
    {
        switch( mTextureType )
        {
            case TextureTypes::Unknown:         return MTLTextureType2D;
            case TextureTypes::Type1D:          return MTLTextureType1D;
            case TextureTypes::Type1DArray:     return MTLTextureType1DArray;
            case TextureTypes::Type2D:          return MTLTextureType2D;
            case TextureTypes::Type2DArray:     return MTLTextureType2DArray;
            case TextureTypes::TypeCube:        return MTLTextureTypeCube;
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS || __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_11_0
            case TextureTypes::TypeCubeArray:   return MTLTextureTypeCubeArray;
#endif
            case TextureTypes::Type3D:          return MTLTextureType3D;
            default:
                return (MTLTextureType)999;
        };
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::notifyDataIsReady(void)
    {
        assert( mResidencyStatus == GpuResidency::Resident );
        assert( mFinalTextureName || mPixelFormat == PFG_NULL );

        mDisplayTextureName = mFinalTextureName;

        notifyAllListenersTextureChanged( TextureGpuListener::ReadyForRendering );
    }
    //-----------------------------------------------------------------------------------
    bool MetalTextureGpu::_isDataReadyImpl(void) const
    {
        return mDisplayTextureName == mFinalTextureName;
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::_setToDisplayDummyTexture(void)
    {
        if( !mTextureManager )
        {
            assert( isRenderWindowSpecific() );
            return; //This can happen if we're a window and we're on shutdown
        }

        MetalTextureGpuManager *textureManagerMetal =
                static_cast<MetalTextureGpuManager*>( mTextureManager );
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
            assert( dynamic_cast<MetalTextureGpu*>( mTexturePool->masterTexture ) );
            MetalTextureGpu *masterTexture = static_cast<MetalTextureGpu*>(mTexturePool->masterTexture);
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
                                  const TextureBox &srcBox, uint8 srcMipLevel )
    {
        TextureGpu::copyTo( dst, dstBox, dstMipLevel, srcBox, srcMipLevel );

        assert( dynamic_cast<MetalTextureGpu*>( dst ) );

        MetalTextureGpu *dstMetal = static_cast<MetalTextureGpu*>( dst );
        MetalTextureGpuManager *textureManagerMetal =
                static_cast<MetalTextureGpuManager*>( mTextureManager );
        MetalDevice *device = textureManagerMetal->getDevice();

        uint32 numSlices = srcBox.numSlices;

        id<MTLTexture> srcTextureName = this->mFinalTextureName;
        id<MTLTexture> dstTextureName = dstMetal->mFinalTextureName;

        //Source has explicit resolves. If destination doesn't,
        //we must copy to its internal MSAA surface.
        if( this->mMsaa > 1u && this->hasMsaaExplicitResolves() )
        {
            if( !dstMetal->hasMsaaExplicitResolves() )
                dstTextureName = dstMetal->mMsaaFramebufferName;
        }
        //Destination has explicit resolves. If source doesn't,
        //we must copy from its internal MSAA surface.
        if( dstMetal->mMsaa > 1u && dstMetal->hasMsaaExplicitResolves() )
        {
            if( !this->hasMsaaExplicitResolves() )
                srcTextureName = this->mMsaaFramebufferName;
        }

        __unsafe_unretained id<MTLBlitCommandEncoder> blitEncoder = device->getBlitEncoder();
        for( uint32 slice=0; slice<numSlices; ++slice )
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

        //Must keep the resolved texture up to date.
        if( dstMetal->mMsaa > 1u && !dstMetal->hasMsaaExplicitResolves() )
        {
            device->endAllEncoders();

            MTLRenderPassDescriptor *passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
            passDesc.colorAttachments[0].texture = dstMetal->mMsaaFramebufferName;
            passDesc.colorAttachments[0].resolveTexture = dstMetal->mFinalTextureName;
            passDesc.colorAttachments[0].loadAction = MTLLoadActionLoad;
            passDesc.colorAttachments[0].storeAction = MTLStoreActionMultisampleResolve;

            for( uint32 slice=0; slice<numSlices; ++slice )
            {
                passDesc.colorAttachments[0].slice = dstBox.sliceStart +
                                                     dstMetal->getInternalSliceStart() + slice;
                passDesc.colorAttachments[0].resolveSlice = dstBox.sliceStart +
                                                            dstMetal->getInternalSliceStart() + slice;
                passDesc.colorAttachments[0].level = dstMipLevel;
                passDesc.colorAttachments[0].resolveLevel = dstMipLevel;
            }

            device->mRenderEncoder =
                    [device->mCurrentCommandBuffer renderCommandEncoderWithDescriptor:passDesc];
            device->endRenderEncoder( false );
        }

        //Do not perform the sync if notifyDataIsReady hasn't been called yet (i.e. we're
        //still building the HW mipmaps, and the texture will never be ready)
        if( dst->_isDataReadyImpl() &&
            dst->getGpuPageOutStrategy() == GpuPageOutStrategy::AlwaysKeepSystemRamCopy )
        {
            dst->_syncGpuResidentToSystemRam();
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::_autogenerateMipmaps(void)
    {
        MetalTextureGpuManager *textureManagerMetal =
                static_cast<MetalTextureGpuManager*>( mTextureManager );
        MetalDevice *device = textureManagerMetal->getDevice();
        __unsafe_unretained id<MTLBlitCommandEncoder> blitEncoder = device->getBlitEncoder();
        [blitEncoder generateMipmapsForTexture: mFinalTextureName];
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

        if( mMsaa > 1u && hasMsaaExplicitResolves() )
            texType = MTLTextureType2DMultisample;

        if( cubemapsAs2DArrays &&
            (mTextureType == TextureTypes::TypeCube ||
             mTextureType == TextureTypes::TypeCubeArray) )
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

        return [mDisplayTextureName newTextureViewWithPixelFormat:MetalMappings::get( pixelFormat )
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
        return getView( texSlot.pixelFormat, texSlot.mipmapLevel, 1u,
                        texSlot.textureArrayIndex, false, true );
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::getSubsampleLocations( vector<Vector2>::type locations )
    {
        locations.reserve( mMsaa );
        if( mMsaa <= 1u )
        {
            locations.push_back( Vector2( 0.0f, 0.0f ) );
        }
        else
        {
            assert( mMsaaPattern != MsaaPatterns::Undefined );

            //TODO
        }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    MetalTextureGpuRenderTarget::MetalTextureGpuRenderTarget(
            GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
            VaoManager *vaoManager, IdString name, uint32 textureFlags,
            TextureTypes::TextureTypes initialType,
            TextureGpuManager *textureManager ) :
        MetalTextureGpu( pageOutStrategy, vaoManager, name,
                         textureFlags, initialType, textureManager ),
        mDepthBufferPoolId( 1u ),
        mPreferDepthTexture( false ),
        mDesiredDepthBufferFormat( PFG_UNKNOWN )
    {
        if( mPixelFormat == PFG_NULL )
            mDepthBufferPoolId = 0;
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpuRenderTarget::_setDepthBufferDefaults(
            uint16 depthBufferPoolId, bool preferDepthTexture, PixelFormatGpu desiredDepthBufferFormat )
    {
        assert( isRenderToTexture() );
        mDepthBufferPoolId          = depthBufferPoolId;
        mPreferDepthTexture         = preferDepthTexture;
        mDesiredDepthBufferFormat   = desiredDepthBufferFormat;
    }
    //-----------------------------------------------------------------------------------
    uint16 MetalTextureGpuRenderTarget::getDepthBufferPoolId(void) const
    {
        return mDepthBufferPoolId;
    }
    //-----------------------------------------------------------------------------------
    bool MetalTextureGpuRenderTarget::getPreferDepthTexture(void) const
    {
        return mPreferDepthTexture;
    }
    //-----------------------------------------------------------------------------------
    PixelFormatGpu MetalTextureGpuRenderTarget::getDesiredDepthBufferFormat(void) const
    {
        return mDesiredDepthBufferFormat;
    }
}
