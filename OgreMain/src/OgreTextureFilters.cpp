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

#include "OgreStableHeaders.h"
#include "OgreTextureFilters.h"

#include "OgreImage2.h"
#include "OgreTextureGpuManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreTextureBox.h"

namespace Ogre
{
namespace TextureFilter
{
    FilterBase::~FilterBase()
    {
    }
    //-----------------------------------------------------------------------------------
    void FilterBase::createFilters( uint32 filters, FilterBaseVec &outFilters,
                                    const TextureGpu *texture )
    {
        const TextureGpuManager *textureManager = texture->getTextureManager();
        const DefaultMipmapGen::DefaultMipmapGen defaultMipmapGeneration =
                textureManager->getDefaultMipmapGeneration();
        const DefaultMipmapGen::DefaultMipmapGen defaultMipmapGenerationCubemaps =
                textureManager->getDefaultMipmapGenerationCubemaps();

        FilterBaseVec filtersVec;
        filtersVec.swap( outFilters );

        if( filters & TextureFilter::TypePrepareForNormalMapping )
            filtersVec.push_back( OGRE_NEW TextureFilter::PrepareForNormalMapping() );
        if( filters & TextureFilter::TypeLeaveChannelR )
            filtersVec.push_back( OGRE_NEW TextureFilter::LeaveChannelR() );

        //Add mipmap generation as one of the last steps
        if( filters & TextureFilter::TypeGenerateDefaultMipmaps )
        {
            if( (filters & TextureFilter::TypeGenerateDefaultMipmaps) ==
                TextureFilter::TypeGenerateDefaultMipmaps )
            {
                const DefaultMipmapGen::DefaultMipmapGen mipmapGen =
                        texture->getTextureType() != TextureTypes::TypeCube ?
                                                         defaultMipmapGeneration :
                                                         defaultMipmapGenerationCubemaps;
                if( mipmapGen == DefaultMipmapGen::HwMode )
                    filtersVec.push_back( OGRE_NEW TextureFilter::GenerateHwMipmaps() );
                else if( mipmapGen == DefaultMipmapGen::SwMode )
                    filtersVec.push_back( OGRE_NEW TextureFilter::GenerateSwMipmaps() );
            }
            else if( filters & TextureFilter::TypeGenerateHwMipmaps )
                filtersVec.push_back( OGRE_NEW TextureFilter::GenerateHwMipmaps() );
            else
                filtersVec.push_back( OGRE_NEW TextureFilter::GenerateSwMipmaps() );
        }

        filtersVec.swap( outFilters );
    }
    //-----------------------------------------------------------------------------------
    void GenerateSwMipmaps::_executeStreaming( Image2 &image, TextureGpu *texture )
    {
        if( image.getNumMipmaps() > 1u )
            return; //Already has mipmaps

        Image2::Filter filter = Image2::FILTER_BILINEAR;
        if( image.getTextureType() == TextureTypes::TypeCube )
            filter = Image2::FILTER_GAUSSIAN;

        const bool isSRgb = PixelFormatGpuUtils::isSRgb( texture->getPixelFormat() );
        image.generateMipmaps( isSRgb, filter );
        texture->setNumMipmaps( image.getNumMipmaps() );
    }
    //-----------------------------------------------------------------------------------
    void GenerateHwMipmaps::_executeStreaming( Image2 &image, TextureGpu *texture )
    {
        //Cubemaps may be loaded as 6 separate images.
        //If one of them needs HW generation, then all faces need it.
        mNeedsMipmaps |= image.getNumMipmaps() <= 1u;

        if( mNeedsMipmaps && texture->getNumMipmaps() <= 1u )
        {
            texture->setNumMipmaps( PixelFormatGpuUtils::getMaxMipmapCount( texture->getWidth(),
                                                                            texture->getHeight(),
                                                                            texture->getDepth() ) );
        }
    }
    //-----------------------------------------------------------------------------------
    void GenerateHwMipmaps::_executeSerial( TextureGpu *texture )
    {
        TextureGpuManager *textureManager = texture->getTextureManager();
        TextureGpu *tempTexture = textureManager->createTexture( "___tempMipmapTexture",
                                                                 GpuPageOutStrategy::Discard,
                                                                 TextureFlags::RenderToTexture |
                                                                 TextureFlags::AllowAutomipmaps,
                                                                 texture->getTextureType() );
        tempTexture->copyParametersFrom( texture );
        tempTexture->scheduleTransitionTo( GpuResidency::Resident );
        TextureBox box = texture->getEmptyBox( 0 );
        texture->copyTo( tempTexture, box, 0, box, 0 );
        tempTexture->_autogenerateMipmaps();

        uint8 numMipmaps = texture->getNumMipmaps();
        for( size_t i=1u; i<numMipmaps; ++i )
        {
            box = texture->getEmptyBox( i );
            tempTexture->copyTo( texture, box, i, box, i );
        }

        textureManager->destroyTexture( tempTexture );
        tempTexture = 0;
    }
    //-----------------------------------------------------------------------------------
    void PrepareForNormalMapping::_executeStreaming( Image2 &image, TextureGpu *texture )
    {
        //PixelFormatGpuUtils::bulkPixelConversion( , )
        //image.
//        OGRE_MALLOC_SIMD
//        Image2 singleChannelImage;
//        singleChannelImage.loadDynamicImage(  );
        //TODO;
    }
    //-----------------------------------------------------------------------------------
    void LeaveChannelR::_executeStreaming( Image2 &image, TextureGpu *texture )
    {
        const PixelFormatGpu srcFormat = image.getPixelFormat();
        size_t numberOfChannels = PixelFormatGpuUtils::getNumberOfComponents( image.getPixelFormat() );
        if( numberOfChannels == 1u )
            return; //Already single channel

        const uint32 formatFlags = PixelFormatGpuUtils::getFlags( srcFormat );
        if( formatFlags & (PixelFormatGpuUtils::PFF_FLOAT_RARE  |
                           PixelFormatGpuUtils::PFF_DEPTH       |
                           PixelFormatGpuUtils::PFF_STENCIL     |
                           PixelFormatGpuUtils::PFF_COMPRESSED  |
                           PixelFormatGpuUtils::PFF_PALLETE )   ||
            formatFlags == 0 )
        {
            return;
        }

        //TODO: This routine is not Endianess-aware. But could be made
        //so by adding an offset for src[i+offset] in this switch. Or
        //just a call to the slower PixelFormatGpuUtils::bulkPixelConversion

        PixelFormatGpu dstFormat;
        switch( PixelFormatGpuUtils::getEquivalentLinear( srcFormat ) )
        {
        case PFG_RGBA32_FLOAT:
        case PFG_RGB32_FLOAT:
        case PFG_RG32_FLOAT:
            dstFormat = PFG_R32_FLOAT;
            break;
        case PFG_RGBA32_UINT:
        case PFG_RGB32_UINT:
        case PFG_RG32_UINT:
            dstFormat = PFG_R32_UINT;
            break;
        case PFG_RGBA32_SINT:
        case PFG_RGB32_SINT:
        case PFG_RG32_SINT:
            dstFormat = PFG_R32_SINT;
            break;
        case PFG_RGBA16_FLOAT:
        case PFG_RG16_FLOAT:
            dstFormat = PFG_R16_FLOAT;
            break;
        case PFG_RGBA16_UINT:
        case PFG_RG16_UINT:
            dstFormat = PFG_R16_UINT;
            break;
        case PFG_RGBA16_SINT:
        case PFG_RG16_SINT:
            dstFormat = PFG_R16_SINT;
            break;
        case PFG_RGBA16_UNORM:
        case PFG_RG16_UNORM:
            dstFormat = PFG_R16_UNORM;
            break;
        case PFG_RGBA16_SNORM:
        case PFG_RG16_SNORM:
            dstFormat = PFG_R16_SNORM;
            break;
        case PFG_RGBA8_UINT:
        case PFG_RG8_UINT:
            dstFormat = PFG_R8_UINT;
            break;
        case PFG_RGBA8_SINT:
        case PFG_RG8_SINT:
            dstFormat = PFG_R8_SINT;
            break;
        case PFG_RGBA8_UNORM:
        case PFG_RG8_UNORM:
            dstFormat = PFG_R8_UNORM;
            break;
        case PFG_RGBA8_SNORM:
        case PFG_RG8_SNORM:
            dstFormat = PFG_R8_SNORM;
            break;
        default:
            return; //Not supported
        }

        uint32 origWidth    = image.getWidth();
        uint32 origHeight   = image.getHeight();
        const uint8 numMipmaps= image.getNumMipmaps();

        const uint32 rowAlignment = 4u;
        const size_t dstSizeBytes = PixelFormatGpuUtils::calculateSizeBytes( origWidth, origHeight,
                                                                             image.getDepth(),
                                                                             image.getNumSlices(),
                                                                             dstFormat, numMipmaps,
                                                                             rowAlignment );

        void *data = OGRE_MALLOC_SIMD( dstSizeBytes, MEMCATEGORY_RESOURCE );

        const size_t srcBytesPerPixel = PixelFormatGpuUtils::getBytesPerPixel( srcFormat );
        const size_t dstBytesPerPixel = srcBytesPerPixel / numberOfChannels;

        for( uint8 mip=0; mip<numMipmaps; ++mip )
        {
            TextureBox srcBox = image.getData( mip );

            const uint32 width          = srcBox.width;
            const uint32 height         = srcBox.height;
            const uint32 depthOrSlices  = srcBox.getDepthOrSlices();

            TextureBox dstBox = srcBox;
            dstBox.bytesPerPixel= PixelFormatGpuUtils::getBytesPerPixel( dstFormat );
            dstBox.bytesPerRow  = PixelFormatGpuUtils::getSizeBytes( width, 1u, 1u, 1u, dstFormat, 4u );
            dstBox.bytesPerImage= PixelFormatGpuUtils::getSizeBytes( width, height, 1u, 1u,
                                                                     dstFormat, 4u );
            dstBox.data = PixelFormatGpuUtils::advancePointerToMip( data, width, height, srcBox.depth,
                                                                    srcBox.numSlices, mip, dstFormat );

            for( size_t z=0; z<depthOrSlices; ++z )
            {
                for( size_t y=0; y<height; ++y )
                {
                    uint8 const * RESTRICT_ALIAS src =
                            reinterpret_cast<uint8*RESTRICT_ALIAS>( srcBox.at( 0, y, z ) );
                    uint8 * RESTRICT_ALIAS dst =
                            reinterpret_cast<uint8*RESTRICT_ALIAS>( dstBox.at( 0, y, z ) );
                    for( size_t x=0; x<width; ++x )
                    {
                        for( size_t i=0; i<dstBytesPerPixel; ++i )
                            dst[i] = src[i];
                        src += srcBytesPerPixel;
                        dst += dstBytesPerPixel;
                    }
                }
            }
        }

        assert( image.getAutoDelete() && "This should be impossible. Memory will leak." );
        image.loadDynamicImage( data, origWidth, origHeight, image.getDepthOrSlices(),
                                image.getTextureType(), dstFormat, true, numMipmaps );
        texture->setPixelFormat( dstFormat );
    }
}
}
