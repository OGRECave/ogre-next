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
#include "OgreTextureGpuManager.h"
#include "OgreTextureBox.h"

#include "OgreException.h"

#define TODO_Create_download_ticket 1

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
        mTextureManager( textureManager ),
        mTexturePool( 0 )
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
    String TextureGpu::getNameStr(void) const
    {
        String retVal;
        const String *nameStr = mTextureManager->findNameStr( mName );

        if( nameStr )
            retVal = *nameStr;
        else
            retVal = mName.getFriendlyText();

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::setResolution( uint32 width, uint32 height, uint32 depthOrSlices )
    {
        assert( mResidencyStatus == GpuResidency::OnStorage );
        mWidth = width;
        mHeight = height;
        mDepthOrSlices = depthOrSlices;
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
    void TextureGpu::setNumMipmaps( uint8 numMipmaps )
    {
        assert( mResidencyStatus == GpuResidency::OnStorage );
        mNumMipmaps = numMipmaps;
    }
    //-----------------------------------------------------------------------------------
    uint8 TextureGpu::getNumMipmaps(void) const
    {
        return mNumMipmaps;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::setTextureType( TextureTypes::TextureTypes textureType )
    {
        assert( mResidencyStatus == GpuResidency::OnStorage );
        mTextureType = textureType;
    }
    //-----------------------------------------------------------------------------------
    TextureTypes::TextureTypes TextureGpu::getTextureType(void) const
    {
        return mTextureType;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::setPixelFormat( PixelFormatGpu pixelFormat )
    {
        assert( mResidencyStatus == GpuResidency::OnStorage );
        mPixelFormat = pixelFormat;
    }
    //-----------------------------------------------------------------------------------
    PixelFormatGpu TextureGpu::getPixelFormat(void) const
    {
        return mPixelFormat;
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
    void TextureGpu::checkValidSettings(void)
    {
        if( mMsaa > 1u )
        {
            if( (mNumMipmaps > 1) || isRenderToTexture() || isUav() )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "MSAA Textures cannot have mipmaps (use explict resolves for that), "
                             "and must be either RenderToTexture or Uav",
                             "TextureGpu::transitionToResident" );
            }

            if( mTextureType == TextureTypes::Type2DArray && !hasMsaaExplicitResolves() )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Only explicit resolves support Type2DArray",
                             "TextureGpu::transitionToResident" );
            }

            if( mTextureType != TextureTypes::Type2D && mTextureType != TextureTypes::Type2DArray )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "MSAA can only be used with Type2D or Type2DArray",
                             "TextureGpu::transitionToResident" );
            }

            if( hasAutomaticBatching() )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "MSAA textures cannot use AutomaticBatching",
                             "TextureGpu::transitionToResident" );
            }
        }

        if( mPageOutStrategy == GpuPageOutStrategy::AlwaysKeepSystemRamCopy &&
            (isRenderToTexture() || isUav()) )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Cannot use AlwaysKeepSystemRamCopy with RenderToTexture or Uav",
                         "TextureGpu::transitionToResident" );
        }

        if( hasAutomaticBatching() && (mTextureType != TextureTypes::Type2D ||
            isRenderToTexture() || isUav()) )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "AutomaticBatching can only be used with Type2D textures, "
                         "and they cannot be RenderToTexture or Uav",
                         "TextureGpu::transitionToResident" );
        }

        if( hasMsaaExplicitResolves() && mMsaa <= 1u )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "A texture is requesting explicit resolves but MSAA is disabled.",
                         "TextureGpu::transitionToResident" );
        }

        if( mWidth < 1u || mHeight < 1u || mDepthOrSlices < 1u || mPixelFormat == PFG_UNKNOWN )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Invalid settings!",
                         "TextureGpu::transitionToResident" );
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::transitionToResident(void)
    {
        checkValidSettings();

        if( !hasAutomaticBatching() )
        {
            //At this point we should have all valid settings (pixel format, width, height)
            //Create our own resource
            createInternalResourcesImpl();
        }
        else
        {
            //Ask the manager for the internal resource.
            mTextureManager->_reserveSlotForTexture( this );
        }

        //Delegate to the manager (thread) for loading.
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::transitionTo( GpuResidency::GpuResidency newResidency, uint8 *sysRamCopy )
    {
        assert( newResidency != mResidencyStatus );

        if( newResidency == GpuResidency::Resident )
        {
            if( mPageOutStrategy != GpuPageOutStrategy::AlwaysKeepSystemRamCopy )
            {
                if( mSysRamCopy )
                {
                    OGRE_FREE_SIMD( mSysRamCopy, MEMCATEGORY_RESOURCE );
                    mSysRamCopy = 0;
                }
            }
            else
            {
                assert( sysRamCopy && "Must provide a SysRAM copy if using AlwaysKeepSystemRamCopy" );
                mSysRamCopy = sysRamCopy;
            }

            transitionToResident();
        }
        else if( newResidency == GpuResidency::OnSystemRam )
        {
            if( mResidencyStatus == GpuResidency::OnStorage )
            {
                assert( mSysRamCopy && "It should be impossible to have SysRAM copy in this stage!" );
                checkValidSettings();

                assert( sysRamCopy &&
                        "Must provide a SysRAM copy when transitioning from OnStorage to OnSystemRam!" );
                mSysRamCopy = sysRamCopy;
            }
            else
            {
                assert( (mSysRamCopy || mPageOutStrategy != GpuPageOutStrategy::AlwaysKeepSystemRamCopy)
                        && "We should already have a SysRAM copy if we were AlwaysKeepSystemRamCopy; or"
                        " we shouldn't have a SysRAM copy if we weren't in that strategy." );

                if( !mSysRamCopy )
                    TODO_Create_download_ticket;
            }
        }
        else
        {
            if( mSysRamCopy )
            {
                OGRE_FREE_SIMD( mSysRamCopy, MEMCATEGORY_RESOURCE );
                mSysRamCopy = 0;
            }

            destroyInternalResourcesImpl();
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::copyTo( TextureGpu *dst, const TextureBox &srcBox, uint8 srcMipLevel,
                             const TextureBox &dstBox, uint8 dstMipLevel )
    {
        assert( srcBox.equalSize( dstBox ) );
        assert( this != dst || !srcBox.overlaps( dstBox ) );
        assert( srcMipLevel < this->getNumMipmaps() && dstMipLevel < dst->getNumMipmaps() );
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
    void TextureGpu::_notifyTextureSlotChanged( const TexturePool *newPool, uint16 slice )
    {
        mTexturePool = newPool;
        mInternalSliceStart = slice;
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
