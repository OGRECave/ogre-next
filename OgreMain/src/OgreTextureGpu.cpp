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

#include "OgreTextureGpu.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreTextureGpuManager.h"
#include "OgreTextureBox.h"
#include "OgreTextureGpuListener.h"

#include "OgreException.h"

#define TODO_Create_download_ticket 1

namespace Ogre
{
    TextureGpu::TextureGpu( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                            VaoManager *vaoManager, IdString name, uint32 textureFlags,
                            TextureTypes::TextureTypes initialType,
                            TextureGpuManager *textureManager ) :
        GpuResource( pageOutStrategy, vaoManager, name ),
        mWidth( 0 ),
        mHeight( 0 ),
        mDepthOrSlices( 0 ),
        mNumMipmaps( 1 ),
        mMsaa( 1 ),
        mMsaaPattern( MsaaPatterns::Undefined ),
        mInternalSliceStart( 0 ),
        mTextureType( initialType ),
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
        assert( mListeners.empty() && "There are listeners out there for this TextureGpu! "
                "This could leave dangling pointers. Ensure you've cleaned up correctly." );
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
    void TextureGpu::scheduleTransitionTo( GpuResidency::GpuResidency nextResidency )
    {
        mNextResidencyStatus = nextResidency;
        mTextureManager->_scheduleTransitionTo( this, nextResidency );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::setResolution( uint32 width, uint32 height, uint32 depthOrSlices )
    {
        assert( mResidencyStatus == GpuResidency::OnStorage );
        mWidth = width;
        mHeight = height;
        if( mTextureType == TextureTypes::TypeCube )
            mDepthOrSlices = 6u;
        else
        {
            assert( (mTextureType != TextureTypes::TypeCubeArray || (mTextureType % 6u) == 0) &&
                    "depthOrSlices must be a multiple of 6 for TypeCubeArray textures!" );
            mDepthOrSlices = depthOrSlices;
        }
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
        return (mTextureType != TextureTypes::Type3D) ? 1u : mDepthOrSlices;
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
        if( mTextureType == TextureTypes::TypeCube )
            mDepthOrSlices = 6u;
    }
    //-----------------------------------------------------------------------------------
    TextureTypes::TextureTypes TextureGpu::getTextureType(void) const
    {
        return mTextureType;
    }
    //-----------------------------------------------------------------------------------
    TextureTypes::TextureTypes TextureGpu::getInternalTextureType(void) const
    {
        return hasAutomaticBatching() ? TextureTypes::Type2DArray : mTextureType;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::setPixelFormat( PixelFormatGpu pixelFormat )
    {
        assert( mResidencyStatus == GpuResidency::OnStorage );

        if( prefersLoadingAsSRGB() )
            pixelFormat = PixelFormatGpuUtils::getEquivalentSRGB( pixelFormat );

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
                             "Texture '" + getNameStr() + "': "
                             "MSAA Textures cannot have mipmaps (use explict resolves for that), "
                             "and must be either RenderToTexture or Uav",
                             "TextureGpu::checkValidSettings" );
            }

            if( mTextureType == TextureTypes::Type2DArray && !hasMsaaExplicitResolves() )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Texture '" + getNameStr() + "': "
                             "Only explicit resolves support Type2DArray",
                             "TextureGpu::checkValidSettings" );
            }

            if( mTextureType != TextureTypes::Type2D && mTextureType != TextureTypes::Type2DArray )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Texture '" + getNameStr() + "': "
                             "MSAA can only be used with Type2D or Type2DArray",
                             "TextureGpu::checkValidSettings" );
            }

            if( hasAutomaticBatching() )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Texture '" + getNameStr() + "': "
                             "MSAA textures cannot use AutomaticBatching",
                             "TextureGpu::checkValidSettings" );
            }
        }

        if( mTextureType == TextureTypes::Unknown )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Texture '" + getNameStr() + "': "
                         "TextureType cannot be TextureTypes::Unknown",
                         "TextureGpu::checkValidSettings" );
        }

        if( mPageOutStrategy == GpuPageOutStrategy::AlwaysKeepSystemRamCopy &&
            (isRenderToTexture() || isUav()) )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Cannot use AlwaysKeepSystemRamCopy with RenderToTexture or Uav",
                         "TextureGpu::checkValidSettings" );
        }

        if( hasAutomaticBatching() && (mTextureType != TextureTypes::Type2D ||
            isRenderToTexture() || isUav()) )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Texture '" + getNameStr() + "': "
                         "AutomaticBatching can only be used with Type2D textures, "
                         "and they cannot be RenderToTexture or Uav",
                         "TextureGpu::checkValidSettings" );
        }

        if( hasMsaaExplicitResolves() && mMsaa <= 1u )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Texture '" + getNameStr() + "': "
                         "A texture is requesting explicit resolves but MSAA is disabled.",
                         "TextureGpu::checkValidSettings" );
        }

        if( allowsAutoMipmaps() && !isRenderToTexture() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Texture '" + getNameStr() + "': "
                         "AllowAutomipmaps requires RenderToTexture.",
                         "TextureGpu::checkValidSettings" );
        }

        if( mWidth < 1u || mHeight < 1u || mDepthOrSlices < 1u || mPixelFormat == PFG_UNKNOWN )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Texture '" + getNameStr() + "': "
                         "Invalid settings!",
                         "TextureGpu::checkValidSettings" );
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
    void TextureGpu::_transitionTo( GpuResidency::GpuResidency newResidency, uint8 *sysRamCopy )
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
            notifyAllListenersTextureChanged( TextureGpuListener::GainedResidency );
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

                notifyAllListenersTextureChanged( TextureGpuListener::FromStorageToSysRam );
            }
            else
            {
                assert( (mSysRamCopy || mPageOutStrategy != GpuPageOutStrategy::AlwaysKeepSystemRamCopy)
                        && "We should already have a SysRAM copy if we were AlwaysKeepSystemRamCopy; or"
                        " we shouldn't have a SysRAM copy if we weren't in that strategy." );

                if( !mSysRamCopy )
                    TODO_Create_download_ticket;

                notifyAllListenersTextureChanged( TextureGpuListener::LostResidency );
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

            notifyAllListenersTextureChanged( TextureGpuListener::LostResidency );
        }

        mResidencyStatus = newResidency;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::copyTo( TextureGpu *dst, const TextureBox &dstBox, uint8 dstMipLevel,
                             const TextureBox &srcBox, uint8 srcMipLevel )
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
        return (mTextureFlags & TextureFlags::NotTexture) == 0;
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
        return (mTextureFlags & TextureFlags::MsaaExplicitResolve) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isReinterpretable(void) const
    {
        return (mTextureFlags & TextureFlags::Reinterpretable) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::prefersLoadingAsSRGB(void) const
    {
        return (mTextureFlags & TextureFlags::PrefersLoadingAsSRGB) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isRenderWindowSpecific(void) const
    {
        return (mTextureFlags & TextureFlags::RenderWindowSpecific) != 0;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::requiresTextureFlipping(void) const
    {
        return (mTextureFlags & TextureFlags::RequiresTextureFlipping) != 0;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::_notifyTextureSlotChanged( const TexturePool *newPool, uint16 slice )
    {
        mTexturePool = newPool;
        mInternalSliceStart = slice;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::addListener( TextureGpuListener *listener )
    {
        mListeners.push_back( listener );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::removeListener( TextureGpuListener *listener )
    {
        vector<TextureGpuListener*>::type::iterator itor = std::find( mListeners.begin(),
                                                                      mListeners.end(), listener );
        assert( itor != mListeners.end() );
        efficientVectorRemove( mListeners, itor );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::notifyAllListenersTextureChanged( uint32 reason )
    {
        //Iterate through a copy in case one of the listeners decides to remove itself.
        vector<TextureGpuListener*>::type listenersVec = mListeners;
        vector<TextureGpuListener*>::type::iterator itor = listenersVec.begin();
        vector<TextureGpuListener*>::type::iterator end  = listenersVec.end();

        while( itor != end )
        {
            (*itor)->notifyTextureChanged( this, static_cast<TextureGpuListener::Reason>( reason ) );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::supportsAsDepthBufferFor( TextureGpu *colourTarget ) const
    {
        assert( PixelFormatGpuUtils::isDepth( this->mPixelFormat ) );
        assert( !PixelFormatGpuUtils::isDepth( colourTarget->mPixelFormat ) );

        if( this->mWidth == colourTarget->mWidth &&
            this->mHeight == colourTarget->mHeight &&
            this->getMsaa() == colourTarget->getMsaa() &&
            this->getMsaaPattern() == colourTarget->getMsaaPattern() &&
            this->isRenderWindowSpecific() == colourTarget->isRenderWindowSpecific() )
        {
            return true;
        }

        return false;
    }
    //-----------------------------------------------------------------------------------
    TextureGpuManager* TextureGpu::getTextureManager(void) const
    {
        return mTextureManager;
    }
    //-----------------------------------------------------------------------------------
    TextureBox TextureGpu::getEmptyBox( uint8 mipLevel )
    {
        TextureBox retVal( std::max( 1u, mWidth >> mipLevel ),
                           std::max( 1u, mHeight >> mipLevel ),
                           std::max( 1u, getDepth() >> mipLevel ),
                           getNumSlices(),
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

        uint32 width            = mWidth;
        uint32 height           = mHeight;
        uint32 depth            = getDepth();
        const uint32 numSlices  = getNumSlices();
        void *data = PixelFormatGpuUtils::advancePointerToMip( mSysRamCopy, width, height, depth,
                                                               numSlices, mipLevel, mPixelFormat );

        TextureBox retVal( std::max( 1u, width >> mipLevel ),
                           std::max( 1u, height >> mipLevel ),
                           std::max( 1u, depth >> mipLevel ),
                           numSlices,
                           PixelFormatGpuUtils::getBytesPerPixel( mPixelFormat ),
                           this->_getSysRamCopyBytesPerRow( mipLevel ),
                           this->_getSysRamCopyBytesPerImage( mipLevel ) );
        retVal.data = data;
        if( PixelFormatGpuUtils::isCompressed( mPixelFormat ) )
            retVal.setCompressedPixelFormat( mPixelFormat );
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    uint8* TextureGpu::_getSysRamCopy( uint8 mipLevel )
    {
        if( !mSysRamCopy )
            return 0;

        assert( mipLevel < mNumMipmaps );

        uint32 width            = mWidth;
        uint32 height           = mHeight;
        uint32 depth            = getDepth();
        const uint32 numSlices  = getNumSlices();
        void *data = PixelFormatGpuUtils::advancePointerToMip( mSysRamCopy, width, height, depth,
                                                               numSlices, mipLevel, mPixelFormat );
        return reinterpret_cast<uint8*>( data );
    }
    //-----------------------------------------------------------------------------------
    size_t TextureGpu::_getSysRamCopyBytesPerRow( uint8 mipLevel )
    {
        assert( mipLevel < mNumMipmaps );
        uint32 width = std::max( mWidth >> mipLevel, 1u );
        return PixelFormatGpuUtils::getSizeBytes( width, 1u, 1u, 1u, mPixelFormat, 4u );
    }
    //-----------------------------------------------------------------------------------
    size_t TextureGpu::_getSysRamCopyBytesPerImage( uint8 mipLevel )
    {
        assert( mipLevel < mNumMipmaps );
        uint32 width  = std::max( mWidth >> mipLevel, 1u );
        uint32 height = std::max( mHeight >> mipLevel, 1u );
        return PixelFormatGpuUtils::getSizeBytes( width, height, 1u, 1u, mPixelFormat, 4u );
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpu::isMetadataReady(void) const
    {
        return mResidencyStatus == GpuResidency::Resident &&
               mNextResidencyStatus == GpuResidency::Resident;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::waitForMetadata(void)
    {
        if( isMetadataReady() )
            return;
        mTextureManager->_waitFor( this, true );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpu::waitForData(void)
    {
        if( isDataReady() )
            return;
        mTextureManager->_waitFor( this, false );
    }
}
