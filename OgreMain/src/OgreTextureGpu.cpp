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

#include "OgreTextureGpu.h"
#include "OgrePixelFormatGpuUtils.h"

#define TODO_createResourcesIfThisIsRttOrUavOrManual 1
#define TODO_automaticBatching_or_manualFromOnStorage 1

namespace Ogre
{
    TextureGpu::TextureGpu( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                            VaoManager *vaoManager, IdString name, uint32 textureFlags,
                            TextureGpuManager *textureManager ) :
        GpuResource( pageOutStrategy, vaoManager, name ),
        mWidth( 0 ),
        mHeight( 0 ),
        mDepthOrSlices( 0 ),
        mNumMipmaps( 1 ),
        mMsaa( 1 ),
        mMsaaPattern( MsaaPatterns::Undefined ),
        mInternalSliceStart( 0 ),
        mTextureType( TextureTypes::Unknown ),
        mPixelFormat( PFG_UNKNOWN ),
        mTextureFlags( textureFlags ),
        mSysRamCopy( 0 ),
        mTextureManager( textureManager )
    {
        assert( !hasAutomaticBatching() ||
                (hasAutomaticBatching() && isTexture() && !isRenderToTexture() && !isUav()) );
        assert( (!hasAutoMipmapAuto() || allowsAutoMipmaps()) &&
                "AutomipmapsAuto requires AllowAutomipmaps" );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu::~TextureGpu()
    {
    }
    //-----------------------------------------------------------------------------------
    uint32 TextureGpu::getWidth(void) const
    {
        return mWidth;
    }
    //-----------------------------------------------------------------------------------
    uint32 TextureGpu::getHeight(void) const
    {
        return mHeight;
    }
    //-----------------------------------------------------------------------------------
    uint32 TextureGpu::getDepthOrSlices(void) const
    {
        return mDepthOrSlices;
    }
    //-----------------------------------------------------------------------------------
    uint32 TextureGpu::getDepth(void) const
    {
        return (mTextureType == TextureTypes::Type3D) ? 1u : mDepthOrSlices;
    }
    //-----------------------------------------------------------------------------------
    uint32 TextureGpu::getNumSlices(void) const
    {
        return (mTextureType != TextureTypes::Type3D) ? mDepthOrSlices : 1u;
    }
    //-----------------------------------------------------------------------------------
    TextureTypes::TextureTypes TextureGpu::getTextureTypes(void) const
    {
        return mTextureType;
    }
    //-----------------------------------------------------------------------------------
    PixelFormatGpu TextureGpu::getPixelFormat(void) const
    {
        return mPixelFormat;
    }
    //-----------------------------------------------------------------------------------
    uint8 TextureGpu::getNumMipmaps(void) const
    {
        return mNumMipmaps;
    }
    //-----------------------------------------------------------------------------------
    uint32 TextureGpu::getInternalSliceStart(void) const
    {
        return mInternalSliceStart;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::setMsaa( uint8 msaa )
    {
        mMsaa = std::max<uint8>( msaa, 1u );
    }
    //-----------------------------------------------------------------------------------
    uint8 TextureGpu::getMsaa(void) const
    {
        return mMsaa;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::setMsaaPattern( MsaaPatterns::MsaaPatterns pattern )
    {
        mMsaaPattern = pattern;
    }
    //-----------------------------------------------------------------------------------
    MsaaPatterns::MsaaPatterns TextureGpu::getMsaaPattern(void) const
    {
        return mMsaaPattern;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isMsaaPatternSupported( MsaaPatterns::MsaaPatterns pattern )
    {
        return pattern == MsaaPatterns::Undefined;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::_init(void)
    {
        if( TODO_createResourcesIfThisIsRttOrUavOrManual )
        {
            //At this point we should have all valid settings (pixel format, width, height)
        }

        //if( msaa > 1 && ((mNumMipmaps > 1) || !isRenderToTexture() && !isUav()) )
        //  OGRE_EXCEPT()

        //if( hasMsaaExplicitResolves() && mMsaa <= 1u )
        //  OGRE_EXCEPT()

        //if( msaa > 1u && mTextureType != TextureTypes::Type2D && mTextureType != TextureTypes::Type2DArray )
        //  OGRE_EXCEPT()

        //if( msaa > 1u && mTextureType == TextureTypes::Type2DArray && !hasMsaaExplicitResolves() )
        //  OGRE_EXCEPT( only explicit resolves );

        //if( msaa > 1u && mNumMipmaps > 1u )
        //  OGRE_EXCEPT( If you want mipmaps, use explicit resolves );

        if( TODO_automaticBatching_or_manualFromOnStorage )
        {
            //Delegate to the manager (thread) for loading.
        }
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::hasAutomaticBatching(void) const
    {
        return (mTextureFlags & TextureFlags::AutomaticBatching) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isTexture(void) const
    {
        return (mTextureFlags & TextureFlags::Texture) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isRenderToTexture(void) const
    {
        return (mTextureFlags & TextureFlags::RenderToTexture) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isUav(void) const
    {
        return (mTextureFlags & TextureFlags::Uav) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::allowsAutoMipmaps(void) const
    {
        return (mTextureFlags & TextureFlags::AllowAutomipmaps) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::hasAutoMipmapAuto(void) const
    {
        return (mTextureFlags & TextureFlags::AutomipmapsAuto) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::hasMsaaExplicitResolves(void) const
    {
        return (mTextureFlags & TextureFlags::MsaaExplicitResolve);
    }
    //-----------------------------------------------------------------------------------
    uint8* TextureGpu::_getSysRamCopy(void)
    {
        return mSysRamCopy;
    }
    //-----------------------------------------------------------------------------------
    size_t TextureGpu::_getSysRamCopyBytesPerRow(void)
    {
        return PixelFormatGpuUtils::getSizeBytes( mWidth, 1u, 1u, 1u, mPixelFormat, 4u );
    }
    //-----------------------------------------------------------------------------------
    size_t TextureGpu::_getSysRamCopyBytesPerImage(void)
    {
        return PixelFormatGpuUtils::getSizeBytes( mWidth, mHeight, 1u, 1u, mPixelFormat, 4u );
    }
}
