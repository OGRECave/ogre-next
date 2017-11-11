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

#include "OgreGL3PlusTextureGpuManager.h"
#include "OgreGL3PlusMappings.h"
#include "OgreGL3PlusTextureGpu.h"
#include "OgreGL3PlusTextureGpuWindow.h"
#include "OgreGL3PlusStagingTexture.h"
#include "OgreGL3PlusAsyncTextureTicket.h"
#include "OgreGL3PlusSupport.h"

#include "Vao/OgreGL3PlusVaoManager.h"

#include "OgrePixelFormatGpuUtils.h"
#include "OgreVector2.h"

#include "OgreException.h"

namespace Ogre
{
    GL3PlusTextureGpuManager::GL3PlusTextureGpuManager( VaoManager *vaoManager,
                                                        RenderSystem *renderSystem,
                                                        const GL3PlusSupport &support ) :
        TextureGpuManager( vaoManager, renderSystem ),
        mSupport( support )
    {
        memset( mBlankTexture, 0, sizeof(mBlankTexture) );
        memset( mTmpFbo, 0, sizeof(mTmpFbo) );

        OCGE( glGenTextures( TextureTypes::Type3D, &mBlankTexture[1u] ) );
        mBlankTexture[TextureTypes::Unknown] = mBlankTexture[TextureTypes::Type2D];

        const GLenum targets[] =
        {
            GL_NONE,
            GL_TEXTURE_1D,
            GL_TEXTURE_1D_ARRAY,
            GL_TEXTURE_2D,
            GL_TEXTURE_2D_ARRAY,
            GL_TEXTURE_CUBE_MAP,
            GL_TEXTURE_CUBE_MAP_ARRAY,
            GL_TEXTURE_3D
        };

        //Must be large enough to hold the biggest transfer we'll do.
        uint8 c_whiteData[4*4*6*4];
        uint8 c_blackData[4*4*6*4];
        memset( c_whiteData, 0xff, sizeof( c_whiteData ) );
        memset( c_blackData, 0x00, sizeof( c_blackData ) );

        for( int i=1; i<=TextureTypes::Type3D; ++i )
        {
            OCGE( glBindTexture( targets[i], mBlankTexture[i] ) );
            OCGE( glTexParameteri( targets[i], GL_TEXTURE_BASE_LEVEL, 0 ) );
            OCGE( glTexParameteri( targets[i], GL_TEXTURE_MAX_LEVEL, 0 ) );
            OCGE( glTexParameteri( targets[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST ) );
            OCGE( glTexParameteri( targets[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST ) );
            OCGE( glTexParameteri( targets[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ) );
            OCGE( glTexParameteri( targets[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ) );
            OCGE( glTexParameteri( targets[i], GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE ) );
            OCGE( glTexParameteri( targets[i], GL_TEXTURE_MAX_LEVEL, 0 ) );

            switch( i )
            {
            case TextureTypes::Unknown:
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE, "Ogre should never hit this path",
                             "GL3PlusTextureGpuManager::GL3PlusTextureGpuManager" );
                break;
            case TextureTypes::Type1D:
                OCGE( glTexStorage1D( targets[i], 1, GL_RGBA8, 4 ) );
                OCGE( glTexSubImage1D( targets[i], 0, 0, 4, GL_RGBA,
                                       GL_UNSIGNED_INT_8_8_8_8_REV, c_whiteData ) );
                break;
            case TextureTypes::Type1DArray:
                OCGE( glTexStorage2D( targets[i], 1, GL_RGBA8, 4, 1 ) );
                OCGE( glTexSubImage2D( targets[i], 0, 0, 0, 4, 1, GL_RGBA,
                                       GL_UNSIGNED_INT_8_8_8_8_REV, c_whiteData ) );
                break;
            case TextureTypes::Type2D:
                OCGE( glTexStorage2D( targets[i], 1, GL_RGBA8, 4, 4 ) );
                OCGE( glTexSubImage2D( targets[i], 0, 0, 0, 4, 4, GL_RGBA,
                                       GL_UNSIGNED_INT_8_8_8_8_REV, c_whiteData ) );
                break;
            case TextureTypes::TypeCube:
                OCGE( glTexStorage2D( targets[i], 1, GL_RGBA8, 4, 4 ) );
                for( int j=0; j<6; ++j )
                {
                    OCGE( glTexSubImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, 0, 0, 4, 4, GL_RGBA,
                                           GL_UNSIGNED_INT_8_8_8_8_REV, c_blackData ) );
                }
                break;
            case TextureTypes::TypeCubeArray:
                OCGE( glTexStorage3D( targets[i], 1, GL_RGBA8, 4, 4, 6 ) );
                OCGE( glTexSubImage3D( targets[i], 0, 0, 0, 0, 4, 4, 6, GL_RGBA,
                                       GL_UNSIGNED_INT_8_8_8_8_REV, c_blackData ) );
                break;
            case TextureTypes::Type2DArray:
            case TextureTypes::Type3D:
                OCGE( glTexStorage3D( targets[i], 1, GL_RGBA8, 4, 4, 4 ) );
                OCGE( glTexSubImage3D( targets[i], 0, 0, 0, 0, 4, 4, 4, GL_RGBA,
                                       GL_UNSIGNED_INT_8_8_8_8_REV, c_whiteData ) );
                break;
            }
        }

        OCGE( glGenFramebuffers( 2u, mTmpFbo ) );
    }
    //-----------------------------------------------------------------------------------
    GL3PlusTextureGpuManager::~GL3PlusTextureGpuManager()
    {
        destroyAll();

        OCGE( glDeleteFramebuffers( 2u, mTmpFbo ) );
        memset( mTmpFbo, 0, sizeof(mTmpFbo) );

        OCGE( glDeleteTextures( TextureTypes::Type3D - 1u, &mBlankTexture[1u] ) );
        memset( mBlankTexture, 0, sizeof(mBlankTexture) );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* GL3PlusTextureGpuManager::createTextureGpuWindow( GL3PlusContext *context,
                                                                  Window *window )
    {
        //GL3+ needs to set MsaaExplicitResolve so that "resolveTexture" in RenderPassDescriptor
        //can remain empty (because the system will be performing the resolve).
        return OGRE_NEW GL3PlusTextureGpuWindow( GpuPageOutStrategy::Discard, mVaoManager,
                                                 "RenderWindow",
                                                 TextureFlags::NotTexture|
                                                 TextureFlags::RenderToTexture|
                                                 TextureFlags::MsaaExplicitResolve|
                                                 TextureFlags::RenderWindowSpecific,
                                                 TextureTypes::Type2D, this, context, window );
    }
    //-----------------------------------------------------------------------------------
    GLuint GL3PlusTextureGpuManager::getBlankTextureGlName(
            TextureTypes::TextureTypes textureType ) const
    {
        return mBlankTexture[textureType];
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* GL3PlusTextureGpuManager::createTextureImpl(
            GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
            IdString name, uint32 textureFlags, TextureTypes::TextureTypes initialType )
    {
        GL3PlusTextureGpu *retVal = 0;
        if( textureFlags & TextureFlags::RenderToTexture )
        {
            retVal = OGRE_NEW GL3PlusTextureGpuRenderTarget(
                         pageOutStrategy, mVaoManager, name,
                         textureFlags | TextureFlags::RequiresTextureFlipping,
                         initialType, this );
        }
        else
        {
            retVal = OGRE_NEW GL3PlusTextureGpu(
                         pageOutStrategy, mVaoManager, name,
                         textureFlags | TextureFlags::RequiresTextureFlipping,
                         initialType, this );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    StagingTexture* GL3PlusTextureGpuManager::createStagingTextureImpl( uint32 width, uint32 height,
                                                                        uint32 depth,
                                                                        uint32 slices,
                                                                        PixelFormatGpu pixelFormat )
    {
        const uint32 rowAlignment = 4u;
        const size_t sizeBytes = PixelFormatGpuUtils::getSizeBytes( width, height, depth, slices,
                                                                    pixelFormat, rowAlignment );

        GL3PlusVaoManager *vaoManager = static_cast<GL3PlusVaoManager*>( mVaoManager );
        return vaoManager->createStagingTexture( PixelFormatGpuUtils::getFamily( pixelFormat ),
                                                 sizeBytes );
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuManager::destroyStagingTextureImpl( StagingTexture *stagingTexture )
    {
        assert( dynamic_cast<GL3PlusStagingTexture*>( stagingTexture ) );

        GL3PlusVaoManager *vaoManager = static_cast<GL3PlusVaoManager*>( mVaoManager );
        vaoManager->destroyStagingTexture( static_cast<GL3PlusStagingTexture*>( stagingTexture ) );
    }
    //-----------------------------------------------------------------------------------
    AsyncTextureTicket* GL3PlusTextureGpuManager::createAsyncTextureTicketImpl(
            uint32 width, uint32 height, uint32 depthOrSlices,
            TextureTypes::TextureTypes textureType, PixelFormatGpu pixelFormatFamily )
    {
        GL3PlusVaoManager *vaoManager = static_cast<GL3PlusVaoManager*>( mVaoManager );
        bool supportsGetTextureSubImage = mSupport.hasMinGLVersion( 4, 5 ) ||
                                          mSupport.checkExtension( "GL_ARB_get_texture_sub_image" );
        return OGRE_NEW GL3PlusAsyncTextureTicket( width, height, depthOrSlices, textureType,
                                                   pixelFormatFamily, vaoManager,
                                                   supportsGetTextureSubImage );
    }
}
