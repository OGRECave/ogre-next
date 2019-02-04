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

#include "OgreMetalStagingTexture.h"
#include "OgreMetalTextureGpu.h"

#include "OgreMetalDevice.h"
#include "OgreStringConverter.h"

#import "Metal/MTLBlitCommandEncoder.h"

namespace Ogre
{
    MetalStagingTexture::MetalStagingTexture( VaoManager *vaoManager, PixelFormatGpu formatFamily,
                                              size_t sizeBytes, MetalDevice *device ) :
        StagingTextureBufferImpl( vaoManager, formatFamily, sizeBytes, 0, 0 ),
        mMappedPtr( 0 ),
        mDevice( device )
    {
        MTLResourceOptions resourceOptions = MTLResourceCPUCacheModeWriteCombined |
                                             MTLResourceStorageModeShared;
        mVboName = [mDevice->mDevice newBufferWithLength:sizeBytes options:resourceOptions];
        if( !mVboName )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Out of GPU memory or driver refused.\n"
                         "Requested: " + StringConverter::toString( sizeBytes ) + " bytes.",
                         "MetalStagingTexture::MetalStagingTexture" );
        }
        mMappedPtr = [mVboName contents];
    }
    //-----------------------------------------------------------------------------------
    MetalStagingTexture::~MetalStagingTexture()
    {
        mMappedPtr = 0;
        mVboName = 0;
    }
    //-----------------------------------------------------------------------------------
    bool MetalStagingTexture::belongsToUs( const TextureBox &box )
    {
        return box.data >= mMappedPtr &&
               box.data <= static_cast<uint8*>( mMappedPtr ) + mCurrentOffset;
    }
    //-----------------------------------------------------------------------------------
    void* RESTRICT_ALIAS_RETURN MetalStagingTexture::mapRegionImplRawPtr(void)
    {
        return static_cast<uint8*>( mMappedPtr ) + mCurrentOffset;
    }
    //-----------------------------------------------------------------------------------
    void MetalStagingTexture::stopMapRegion(void)
    {
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
//        NSRange range = NSMakeRange( 0, mCurrentOffset );
//        [mVboName didModifyRange:range];
#endif
        StagingTextureBufferImpl::stopMapRegion();
    }
    //-----------------------------------------------------------------------------------
    void MetalStagingTexture::upload( const TextureBox &srcBox, TextureGpu *dstTexture,
                                      uint8 mipLevel, const TextureBox *cpuSrcBox,
                                      const TextureBox *dstBox, bool skipSysRamCopy )
    {
        StagingTextureBufferImpl::upload( srcBox, dstTexture, mipLevel,
                                          cpuSrcBox, dstBox, skipSysRamCopy );

        __unsafe_unretained id<MTLBlitCommandEncoder> blitEncoder = mDevice->getBlitEncoder();

        size_t bytesPerRow      = srcBox.bytesPerRow;
        size_t bytesPerImage    = srcBox.bytesPerImage;

        // PVR textures must have 0 on these.
        if( mFormatFamily == PFG_PVRTC2_2BPP || mFormatFamily == PFG_PVRTC2_2BPP_SRGB ||
            mFormatFamily == PFG_PVRTC2_4BPP || mFormatFamily == PFG_PVRTC2_4BPP_SRGB ||
            mFormatFamily == PFG_PVRTC_RGB2  || mFormatFamily == PFG_PVRTC_RGB2_SRGB  ||
            mFormatFamily == PFG_PVRTC_RGB4  || mFormatFamily == PFG_PVRTC_RGB4_SRGB  ||
            mFormatFamily == PFG_PVRTC_RGBA2 || mFormatFamily == PFG_PVRTC_RGBA2_SRGB ||
            mFormatFamily == PFG_PVRTC_RGBA4 || mFormatFamily == PFG_PVRTC_RGBA4_SRGB )
        {
            bytesPerRow = 0;
            bytesPerImage = 0;
        }

        // Recommended by documentation to set this value to 0 for non-3D textures.
        if( dstTexture->getTextureType() != TextureTypes::Type3D )
            bytesPerImage = 0;

        assert( dynamic_cast<MetalTextureGpu*>( dstTexture ) );
        MetalTextureGpu *dstTextureMetal = static_cast<MetalTextureGpu*>( dstTexture );

        const size_t srcOffset = reinterpret_cast<uint8*>( srcBox.data ) -
                                 reinterpret_cast<uint8*>( mMappedPtr );
        const NSUInteger destinationSlice = (dstBox ? dstBox->sliceStart : 0) +
                                            dstTexture->getInternalSliceStart();
        NSUInteger xPos = static_cast<NSUInteger>( dstBox ? dstBox->x : 0 );
        NSUInteger yPos = static_cast<NSUInteger>( dstBox ? dstBox->y : 0 );
        NSUInteger zPos = static_cast<NSUInteger>( dstBox ? dstBox->z : 0 );

        for( NSUInteger i=0; i<srcBox.numSlices; ++i )
        {
            [blitEncoder copyFromBuffer:mVboName
                           sourceOffset:srcOffset + srcBox.bytesPerImage * i
                      sourceBytesPerRow:bytesPerRow
                    sourceBytesPerImage:bytesPerImage
                             sourceSize:MTLSizeMake( srcBox.width, srcBox.height, srcBox.depth )
                              toTexture:dstTextureMetal->getFinalTextureName()
                       destinationSlice:destinationSlice + i
                       destinationLevel:mipLevel
                      destinationOrigin:MTLOriginMake( xPos, yPos, zPos )];
        }
    }
}
