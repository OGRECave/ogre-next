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
        MTLTextureDescriptor *desc = [MTLTextureDescriptor new];
        desc.mipmapLevelCount   = mNumMipmaps;
        desc.textureType        = getMetalTextureType();
        desc.width              = mWidth;
        desc.height             = mHeight;
        desc.depth              = getDepth();
        desc.arrayLength        = getNumSlices();
        desc.pixelFormat        = MetalMappings::get( mPixelFormat );
        desc.sampleCount        = 1u;

        if( mTextureType == TextureTypes::TypeCube || mTextureType == TextureTypes::TypeCubeArray )
            desc.arrayLength /= 6u;

        if( mMsaa > 1u && hasMsaaExplicitResolves() )
        {
            desc.textureType = MTLTextureType2DMultisample;
            desc.sampleCount = mMsaa;
        }

        desc.usage = MTLTextureUsageUnknown;
        if( isTexture() )
            desc.usage |= MTLTextureUsageShaderRead;
        if( isRenderToTexture() )
            desc.usage |= MTLTextureUsageRenderTarget;
        if( isUav() )
            desc.usage |= MTLTextureUsageShaderWrite;

        String textureName = getNameStr();

        MetalTextureGpuManager *textureManagerMetal =
                static_cast<MetalTextureGpuManager*>( mTextureManager );
        MetalDevice *device = textureManagerMetal->getDevice();

        mFinalTextureName = [device->mDevice newTextureWithDescriptor:desc];
        mFinalTextureName.label = [NSString stringWithUTF8String:textureName.c_str()];

        if( mMsaa > 1u && !hasMsaaExplicitResolves() )
        {
            desc.textureType    = MTLTextureType2DMultisample;
            desc.depth          = 1u;
            desc.arrayLength    = 1u;
            desc.sampleCount    = mMsaa;
            mMsaaFramebufferName = [device->mDevice newTextureWithDescriptor:desc];
            mMsaaFramebufferName.label = [NSString stringWithUTF8String:(textureName + "_MSAA").c_str()];
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::destroyInternalResourcesImpl(void)
    {
        if( !hasAutomaticBatching() )
        {
            if( mFinalTextureName )
                mFinalTextureName = 0;
        }
        else
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
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
            case TextureTypes::TypeCubeArray:   return MTLTextureTypeCubeArray;
#endif
            default:
                return (MTLTextureType)999;
        };
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::notifyDataIsReady(void)
    {
        assert( mResidencyStatus == GpuResidency::Resident );
        assert( mFinalTextureName );

        mDisplayTextureName = mFinalTextureName;

        notifyAllListenersTextureChanged( TextureGpuListener::ReadyForDisplay );
    }
    //-----------------------------------------------------------------------------------
    bool MetalTextureGpu::isDataReady(void) const
    {
        return mDisplayTextureName == mFinalTextureName;
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpu::_setToDisplayDummyTexture(void)
    {
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

        __unsafe_unretained id<MTLBlitCommandEncoder> blitEncoder = device->getBlitEncoder();
        for( uint32 slice=0; slice<numSlices; ++slice )
        {
            [blitEncoder copyFromTexture:this->getFinalTextureName()
                             sourceSlice:srcBox.sliceStart + this->getInternalSliceStart() + slice
                             sourceLevel:srcMipLevel
                            sourceOrigin:MTLOriginMake( srcBox.x, srcBox.y, srcBox.z )
                              sourceSize:MTLSizeMake( srcBox.width, srcBox.height, srcBox.depth )
                               toTexture:dstMetal->getFinalTextureName()
                        destinationSlice:dstBox.sliceStart + dstMetal->getInternalSliceStart() + slice
                        destinationLevel:dstMipLevel
                       destinationOrigin:MTLOriginMake( dstBox.x, dstBox.y, dstBox.z )];
        }
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
}
