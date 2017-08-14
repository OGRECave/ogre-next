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

#include "OgreGL3PlusTextureGpu.h"
#include "OgreGL3PlusMappings.h"
#include "OgreGL3PlusTextureGpuManager.h"
#include "OgreGL3PlusSupport.h"

#include "OgreTextureGpuListener.h"
#include "OgreTextureBox.h"
#include "OgreVector2.h"

#include "Vao/OgreVaoManager.h"

#include "OgreException.h"

#define TODO_use_StagingTexture_with_GPU_GPU_visibility 1

namespace Ogre
{
    GL3PlusTextureGpu::GL3PlusTextureGpu( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                          VaoManager *vaoManager, IdString name, uint32 textureFlags,
                                          TextureTypes::TextureTypes initialType,
                                          TextureGpuManager *textureManager ) :
        TextureGpu( pageOutStrategy, vaoManager, name, textureFlags, initialType, textureManager ),
        mDisplayTextureName( 0 ),
        mGlTextureTarget( GL_NONE ),
        mFinalTextureName( 0 ),
        mMsaaFramebufferName( 0 )
    {
        _setToDisplayDummyTexture();
    }
    //-----------------------------------------------------------------------------------
    GL3PlusTextureGpu::~GL3PlusTextureGpu()
    {
        destroyInternalResourcesImpl();
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::createInternalResourcesImpl(void)
    {
        GLenum format = GL3PlusMappings::get( mPixelFormat );

        if( !hasMsaaExplicitResolves() )
        {
            OCGE( glGenTextures( 1u, &mFinalTextureName ) );

            mGlTextureTarget = GL3PlusMappings::get( mTextureType );

            OCGE( glBindTexture( mGlTextureTarget, mFinalTextureName ) );
            OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_BASE_LEVEL, 0 ) );
            OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_MAX_LEVEL, 0 ) );
            OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST ) );
            OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST ) );
            OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ) );
            OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ) );
            OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE ) );
            OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_MAX_LEVEL, mNumMipmaps - 1u ) );

            switch( mTextureType )
            {
            case TextureTypes::Unknown:
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                             "Texture '" + getNameStr() + "': "
                             "Ogre should never hit this path",
                             "GL3PlusTextureGpu::createInternalResourcesImpl" );
                break;
            case TextureTypes::Type1D:
                OCGE( glTexStorage1D( GL_TEXTURE_1D, GLsizei(mNumMipmaps), format, GLsizei(mWidth) ) );
                break;
            case TextureTypes::Type1DArray:
                OCGE( glTexStorage2D( GL_TEXTURE_1D_ARRAY, GLsizei(mNumMipmaps), format,
                                      GLsizei(mWidth), GLsizei(mDepthOrSlices) ) );
                break;
            case TextureTypes::Type2D:
                OCGE( glTexStorage2D( GL_TEXTURE_2D, GLsizei(mNumMipmaps), format,
                                      GLsizei(mWidth), GLsizei(mHeight) ) );
                break;
            case TextureTypes::Type2DArray:
                OCGE( glTexStorage3D( GL_TEXTURE_2D_ARRAY, GLsizei(mNumMipmaps), format,
                                      GLsizei(mWidth), GLsizei(mHeight), GLsizei(mDepthOrSlices) ) );
                break;
            case TextureTypes::TypeCube:
                OCGE( glTexStorage2D( GL_TEXTURE_CUBE_MAP, GLsizei(mNumMipmaps), format,
                                      GLsizei(mWidth), GLsizei(mHeight) ) );
                break;
            case TextureTypes::TypeCubeArray:
                OCGE( glTexStorage3D( GL_TEXTURE_CUBE_MAP_ARRAY, GLsizei(mNumMipmaps), format,
                                      GLsizei(mWidth), GLsizei(mHeight), GLsizei(mDepthOrSlices) ) );
                break;
            case TextureTypes::Type3D:
                OCGE( glTexStorage3D( GL_TEXTURE_3D, GLsizei(mNumMipmaps), format,
                                      GLsizei(mWidth), GLsizei(mHeight), GLsizei(mDepthOrSlices) ) );
                break;
            }

            //Allocate internal buffers for automipmaps before we load anything into them
            if( allowsAutoMipmaps() )
                OCGE( glGenerateMipmap( mGlTextureTarget ) );
        }

        if( mMsaa > 1u )
        {
            const GLboolean fixedsamplelocations = mMsaaPattern != MsaaPatterns::Undefined;

            if( !hasMsaaExplicitResolves() )
            {
                OCGE( glGenRenderbuffers( 1, &mMsaaFramebufferName ) );
                OCGE( glBindRenderbuffer( GL_RENDERBUFFER, mMsaaFramebufferName ) );
                OCGE( glRenderbufferStorageMultisample( GL_RENDERBUFFER, mMsaa, format,
                                                        GLsizei(mWidth), GLsizei(mHeight) ) );
                OCGE( glBindRenderbuffer( GL_RENDERBUFFER, 0 ) );
            }
            else
            {
                OCGE( glGenTextures( 1u, &mFinalTextureName ) );

                assert( mTextureType == TextureTypes::Type2D ||
                        mTextureType == TextureTypes::Type2DArray );
                mGlTextureTarget =
                        mTextureType == TextureTypes::Type2D ? GL_TEXTURE_2D_MULTISAMPLE :
                                                               GL_TEXTURE_2D_MULTISAMPLE_ARRAY;

                OCGE( glBindTexture( mGlTextureTarget, mFinalTextureName ) );
                OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_BASE_LEVEL, 0 ) );
                OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_MAX_LEVEL, 0 ) );
                OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST ) );
                OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST ) );
                OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ) );
                OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ) );
                OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE ) );
                OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_MAX_LEVEL, mNumMipmaps - 1u ) );

                if( mTextureType == TextureTypes::Type2D )
                {
                    glTexImage2DMultisample( mGlTextureTarget, mMsaa, format,
                                             GLsizei(mWidth), GLsizei(mHeight), fixedsamplelocations );
                }
                else
                {
                    glTexImage3DMultisample( mGlTextureTarget, mMsaa, format,
                                             GLsizei(mWidth), GLsizei(mHeight), GLsizei(mDepthOrSlices),
                                             fixedsamplelocations );
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::destroyInternalResourcesImpl(void)
    {
        if( !hasAutomaticBatching() )
        {
            if( mFinalTextureName )
            {
                glDeleteTextures( 1, &mFinalTextureName );
                mFinalTextureName = 0;
            }
            if( mMsaaFramebufferName )
            {
                glDeleteTextures( 1, &mMsaaFramebufferName );
                mMsaaFramebufferName = 0;
            }
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
    void GL3PlusTextureGpu::notifyDataIsReady(void)
    {
        assert( mResidencyStatus == GpuResidency::Resident );
        assert( mFinalTextureName );

        mDisplayTextureName = mFinalTextureName;

        notifyAllListenersTextureChanged( TextureGpuListener::ReadyForDisplay );
    }
    //-----------------------------------------------------------------------------------
    bool GL3PlusTextureGpu::isDataReady(void) const
    {
        return mDisplayTextureName == mFinalTextureName;
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::_setToDisplayDummyTexture(void)
    {
        GL3PlusTextureGpuManager *textureManagerGl =
                static_cast<GL3PlusTextureGpuManager*>( mTextureManager );
        if( hasAutomaticBatching() )
        {
            mDisplayTextureName = textureManagerGl->getBlankTextureGlName( TextureTypes::Type2DArray );
            mGlTextureTarget = GL_TEXTURE_2D_ARRAY;
        }
        else
        {
            mDisplayTextureName = textureManagerGl->getBlankTextureGlName( mTextureType );
            mGlTextureTarget = GL3PlusMappings::get( mTextureType );
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::_notifyTextureSlotChanged( const TexturePool *newPool, uint16 slice )
    {
        TextureGpu::_notifyTextureSlotChanged( newPool, slice );

        _setToDisplayDummyTexture();

        mGlTextureTarget = GL_TEXTURE_2D_ARRAY;

        if( mTexturePool )
        {
            assert( dynamic_cast<GL3PlusTextureGpu*>( mTexturePool->masterTexture ) );
            GL3PlusTextureGpu *masterTexture = static_cast<GL3PlusTextureGpu*>(mTexturePool->masterTexture);
            mFinalTextureName = masterTexture->mFinalTextureName;
        }

        notifyAllListenersTextureChanged( TextureGpuListener::PoolTextureSlotChanged );
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::setTextureType( TextureTypes::TextureTypes textureType )
    {
        const TextureTypes::TextureTypes oldType = mTextureType;
        TextureGpu::setTextureType( textureType );

        if( oldType != mTextureType && mDisplayTextureName != mFinalTextureName )
            _setToDisplayDummyTexture();
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::copyTo( TextureGpu *dst, const TextureBox &dstBox, uint8 dstMipLevel,
                                    const TextureBox &srcBox, uint8 srcMipLevel )
    {
        TextureGpu::copyTo( dst, dstBox, dstMipLevel, srcBox, srcMipLevel );

        if( dst->isRenderWindowSpecific() )
        {
            OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                         "TODO: Copying to RenderWindow",
                         "GL3PlusTextureGpu::copyTo" );
        }

        assert( dynamic_cast<GL3PlusTextureGpu*>( dst ) );

        GL3PlusTextureGpu *dstGl = static_cast<GL3PlusTextureGpu*>( dst );
        GL3PlusTextureGpuManager *textureManagerGl =
                static_cast<GL3PlusTextureGpuManager*>( mTextureManager );
        const GL3PlusSupport &support = textureManagerGl->getGlSupport();

        if( support.hasMinGLVersion( 4, 3 ) || support.checkExtension( "GL_ARB_copy_image" ) )
        {
            OCGE( glCopyImageSubData( this->mFinalTextureName, this->mGlTextureTarget,
                                      srcMipLevel, srcBox.x, srcBox.y, srcBox.z,
                                      dstGl->mFinalTextureName, dstGl->mGlTextureTarget,
                                      dstMipLevel, dstBox.x, dstBox.y, dstBox.z,
                                      srcBox.width, srcBox.height, srcBox.getDepthOrSlices() ) );
        }
        /*TODO
        else if( support.checkExtension( "GL_NV_copy_image" ) )
        {
            OCGE( glCopyImageSubDataNV( this->mFinalTextureName, this->mGlTextureTarget,
                                        srcMipLevel, srcBox.x, srcBox.y, srcBox.z,
                                        dstGl->mFinalTextureName, dstGl->mGlTextureTarget,
                                        dstMipLevel, dstBox.x, dstBox.y, dstBox.z,
                                        srcBox.width, srcBox.height, srcBox.getDepthOrSlices() ) );
        }*/
        /*TODO: These are for OpenGL ES 3.0+
        else if( support.checkExtension( "GL_OES_copy_image" ) )
        {
            OCGE( glCopyImageSubDataOES( this->mFinalTextureName, this->mGlTextureTarget,
                                         srcMipLevel, srcBox.x, srcBox.y, srcBox.z,
                                         dstGl->mFinalTextureName, dstGl->mGlTextureTarget,
                                         dstMipLevel, dstBox.x, dstBox.y, dstBox.z,
                                         srcBox.width, srcBox.height, srcBox.getDepthOrSlices() ) );
        }
        else if( support.checkExtension( "GL_EXT_copy_image" ) )
        {
            OCGE( glCopyImageSubDataEXT( this->mFinalTextureName, this->mGlTextureTarget,
                                         srcMipLevel, srcBox.x, srcBox.y, srcBox.z,
                                         dstGl->mFinalTextureName, dstGl->mGlTextureTarget,
                                         dstMipLevel, dstBox.x, dstBox.y, dstBox.z,
                                         srcBox.width, srcBox.height, srcBox.getDepthOrSlices() ) );
        }*/
        else
        {
//            GLenum format, type;
//            GL3PlusMappings::getFormatAndType( mPixelFormat, format, type );
//            glGetTexImage( this->mFinalTextureName, srcMipLevel, format, type,  );
            //glGetCompressedTexImage
            TODO_use_StagingTexture_with_GPU_GPU_visibility;
            OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "", "GL3PlusTextureGpu::copyTo" );
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::_autogenerateMipmaps(void)
    {
        if( !mFinalTextureName )
            return;

        const GLenum texTarget = getGlTextureTarget();
        OCGE( glBindTexture( texTarget, mFinalTextureName ) );
        OCGE( glGenerateMipmap( texTarget ) );
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::getSubsampleLocations( vector<Vector2>::type locations )
    {
        locations.reserve( mMsaa );
        if( mMsaa <= 1u )
        {
            locations.push_back( Vector2( 0.0f, 0.0f ) );
        }
        else
        {
            assert( mMsaaPattern != MsaaPatterns::Undefined );

            float vals[2];
            for( int i=0; i<mMsaa; ++i )
            {
                glGetMultisamplefv( GL_SAMPLE_POSITION, i, vals );
                locations.push_back( Vector2( vals[0], vals[1] ) * 2.0f - 1.0f );
            }
        }
    }
}
