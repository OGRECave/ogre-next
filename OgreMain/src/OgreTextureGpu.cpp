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

#define TODO_createResourcesIfThisIsRttOrUavOrManual 1
#define TODO_automaticBatching_or_manualFromOnStorage 1

namespace Ogre
{
    TextureGpu::TextureGpu( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                            VaoManager *vaoManager, uint32 textureFlags ) :
        GpuResource( pageOutStrategy, vaoManager ),
        mWidth( 0 ),
        mHeight( 0 ),
        mDepthOrSlices( 0 ),
        mNumMipmaps( 1 ),
        mMsaa( 1 ),
        mMsaaPattern( MsaaPatterns::Undefined ),
        mTextureType( TextureTypes::Unknown ),
        mPixelFormat( PFG_UNKNOWN ),
        mTextureFlags( textureFlags )
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
    void TextureGpu::_init(void)
    {
        if( TODO_createResourcesIfThisIsRttOrUavOrManual )
        {
            //At this point we should have all valid settings (pixel format, width, height)
        }

        //if( msaa > 1 && ((mNumMipmaps > 1) || !isRenderToTexture() && !isUav()) )
        //  OGRE_EXCEPT()

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
}
