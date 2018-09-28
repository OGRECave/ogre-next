/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2014 Torus Knot Software Ltd

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

#include "OgreGL3PlusAsyncTextureTicket.h"
#include "OgreGL3PlusTextureGpu.h"
#include "OgreGL3PlusMappings.h"
#include "Vao/OgreGL3PlusVaoManager.h"

#include "OgreTextureBox.h"
#include "OgreTextureGpuManager.h"
#include "OgrePixelFormatGpuUtils.h"

namespace Ogre
{
    GL3PlusAsyncTextureTicket::GL3PlusAsyncTextureTicket( uint32 width, uint32 height,
                                                          uint32 depthOrSlices,
                                                          TextureTypes::TextureTypes textureType,
                                                          PixelFormatGpu pixelFormatFamily,
                                                          GL3PlusVaoManager *vaoManager,
                                                          bool supportsGetTextureSubImage ) :
        AsyncTextureTicket( width, height, depthOrSlices, textureType,
                            pixelFormatFamily ),
        mVboName( 0 ),
        mTmpVboName( 0 ),
        mDownloadFrame( 0 ),
        mAccurateFence( 0 ),
        mVaoManager( vaoManager ),
        mSupportsGetTextureSubImage( supportsGetTextureSubImage )
    {
        mVboName = createBuffer( width, height, depthOrSlices );
    }
    //-----------------------------------------------------------------------------------
    GL3PlusAsyncTextureTicket::~GL3PlusAsyncTextureTicket()
    {
        if( mStatus == Mapped )
            unmap();

        OCGE( glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 ) );

        if( mVboName )
        {
            glDeleteBuffers( 1u, &mVboName );
            mVboName = 0;
        }
        if( mTmpVboName )
        {
            glDeleteBuffers( 1u, &mTmpVboName );
            mTmpVboName = 0;
        }

        if( mAccurateFence )
        {
            OCGE( glDeleteSync( mAccurateFence ) );
            mAccurateFence = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    GLuint GL3PlusAsyncTextureTicket::createBuffer( uint32 width, uint32 height, uint32 depthOrSlices )
    {
        GLuint vboName;

        OCGE( glGenBuffers( 1, &vboName ) );
        OCGE( glBindBuffer( GL_PIXEL_PACK_BUFFER, vboName ) );

        const uint32 rowAlignment = 4u;
        const size_t sizeBytes = PixelFormatGpuUtils::getSizeBytes( width, height, depthOrSlices,
                                                                    1u, mPixelFormatFamily,
                                                                    rowAlignment );

        if( mVaoManager->supportsArbBufferStorage() )
        {
            OCGE( glBufferStorage( GL_PIXEL_PACK_BUFFER, sizeBytes, 0, GL_MAP_READ_BIT ) );
        }
        else
        {
            OCGE( glBufferData( GL_PIXEL_PACK_BUFFER, sizeBytes, 0, GL_STREAM_READ ) );
        }

        return vboName;
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusAsyncTextureTicket::downloadFromGpu( TextureGpu *textureSrc, uint8 mipLevel,
                                                     bool accurateTracking, TextureBox *srcBox )
    {
        AsyncTextureTicket::downloadFromGpu( textureSrc, mipLevel, accurateTracking, srcBox );

        mDownloadFrame = mVaoManager->getFrameCount();

        if( mAccurateFence )
        {
            OCGE( glDeleteSync( mAccurateFence ) );
            mAccurateFence = 0;
        }

        TextureBox srcTextureBox;
        TextureBox fullSrcTextureBox( textureSrc->getEmptyBox( mipLevel ) );

        if( !srcBox )
            srcTextureBox = fullSrcTextureBox;
        else
        {
            srcTextureBox = *srcBox;
            srcTextureBox.bytesPerRow   = fullSrcTextureBox.bytesPerRow;
            srcTextureBox.bytesPerPixel = fullSrcTextureBox.bytesPerPixel;
            srcTextureBox.bytesPerImage = fullSrcTextureBox.bytesPerImage;
        }

        if( textureSrc->hasAutomaticBatching() )
        {
            fullSrcTextureBox.sliceStart= textureSrc->getInternalSliceStart();
            fullSrcTextureBox.numSlices = textureSrc->getTexturePool()->masterTexture->getNumSlices();

            srcTextureBox.sliceStart += textureSrc->getInternalSliceStart();
        }

        const size_t bytesPerPixel =
                PixelFormatGpuUtils::getBytesPerPixel( textureSrc->getPixelFormat() );

        const GLint rowLength   = bytesPerPixel > 0 ? (srcTextureBox.bytesPerRow / bytesPerPixel) : 0;
        const GLint imageHeight = (srcTextureBox.bytesPerRow > 0) ?
                                      (srcTextureBox.bytesPerImage / srcTextureBox.bytesPerRow) : 0;

        OCGE( glPixelStorei( GL_PACK_ALIGNMENT, 4 ) );
        OCGE( glPixelStorei( GL_PACK_ROW_LENGTH, rowLength) );
        OCGE( glPixelStorei( GL_PACK_IMAGE_HEIGHT, imageHeight ) );

        const TextureTypes::TextureTypes textureType = textureSrc->getInternalTextureType();
        const PixelFormatGpu pixelFormat = textureSrc->getPixelFormat();

        assert( dynamic_cast<GL3PlusTextureGpu*>( textureSrc ) );
        GL3PlusTextureGpu *srcTextureGl = static_cast<GL3PlusTextureGpu*>( textureSrc );

        const GLenum targetGl   = srcTextureGl->getGlTextureTarget();
        const GLuint texName    = srcTextureGl->getFinalTextureName();

        OCGE( glBindTexture( targetGl, texName ) );
        OCGE( glBindBuffer( GL_PIXEL_PACK_BUFFER, mVboName ) );

        GLint xPos          = static_cast<GLint>( srcTextureBox.x );
        GLint yPos          = static_cast<GLint>( srcTextureBox.y );
        GLint zPos          = static_cast<GLint>( srcTextureBox.z );
        GLint slicePos      = static_cast<GLint>( srcTextureBox.sliceStart );
        GLsizei width       = static_cast<GLsizei>( srcTextureBox.width );
        GLsizei height      = static_cast<GLsizei>( srcTextureBox.height );
        GLsizei depthOrSlices= static_cast<GLsizei>( srcTextureBox.getDepthOrSlices() );

        if( textureType == TextureTypes::Type1DArray )
        {
            yPos            = slicePos;
            slicePos        = 1;
            height          = depthOrSlices;
            depthOrSlices   = 1u;
        }

        //We need to use glGetTextureSubImage & glGetCompressedTextureSubImage,
        //which is only available since GL4.5. If this isn't possible, then
        //we need to download the whole texture into a dummy temporary buffer.
        if( !fullSrcTextureBox.equalSize( srcTextureBox ) && !mSupportsGetTextureSubImage )
        {
            if( !mTmpVboName || !mSubregion.equalSize( srcTextureBox ) )
            {
                if( mTmpVboName )
                    glDeleteBuffers( 1u, &mTmpVboName );
                //Create temporary BO to hold the whole thing.
                //When mapping we'll use the subregion via
                //bytesPer* variables.
                mTmpVboName = createBuffer( fullSrcTextureBox.width, fullSrcTextureBox.height,
                                            fullSrcTextureBox.getDepthOrSlices() );
            }

            OCGE( glBindBuffer( GL_PIXEL_PACK_BUFFER, mTmpVboName ) );
            mSubregion = srcTextureBox;
            mSubregion.bytesPerPixel    = fullSrcTextureBox.bytesPerPixel;
            mSubregion.bytesPerRow      = fullSrcTextureBox.bytesPerRow;
            mSubregion.bytesPerImage    = fullSrcTextureBox.bytesPerImage;
        }
        else if( mTmpVboName )
        {
            glDeleteBuffers( 1u, &mTmpVboName );
            mTmpVboName = 0;
            mSubregion = TextureBox();
        }

        if( !textureSrc->isRenderWindowSpecific() )
        {
            if( fullSrcTextureBox.equalSize( srcTextureBox ) || !mSupportsGetTextureSubImage )
            {
                //We can use glGetTexImage & glGetCompressedTexImage (cubemaps need a special path)
                if( !PixelFormatGpuUtils::isCompressed( pixelFormat ) )
                {
                    GLenum format, type;
                    GL3PlusMappings::getFormatAndType( pixelFormat, format, type );

                    if( textureType != TextureTypes::TypeCube )
                    {
                        OCGE( glGetTexImage( targetGl, mipLevel, format, type, 0 ) );
                    }
                    else
                    {
                        for( size_t i=0; i<(size_t)depthOrSlices; ++i )
                        {
                            const GLenum targetCubeGl =
                                    static_cast<GLenum>( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i );
                            OCGE( glGetTexImage( targetCubeGl, mipLevel, format, type,
                                                 reinterpret_cast<void*>(
                                                     srcTextureBox.bytesPerImage * i ) ) );
                        }
                    }
                }
                else
                {
                    if( textureType != TextureTypes::TypeCube )
                    {
                        OCGE( glGetCompressedTexImage( targetGl, mipLevel, 0 ) );
                    }
                    else
                    {
                        for( size_t i=0; i<(size_t)depthOrSlices; ++i )
                        {
                            const GLenum targetCubeGl =
                                    static_cast<GLenum>( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i );
                            OCGE( glGetCompressedTexImage( targetCubeGl, mipLevel,
                                                           reinterpret_cast<void*>(
                                                               srcTextureBox.bytesPerImage * i ) ) );
                        }
                    }
                }
            }
            else
            {
                //We need to use glGetTextureSubImage & glGetCompressedTextureSubImage,
                //which is only available since GL4.5. Support is here. Yay!
                if( !PixelFormatGpuUtils::isCompressed( pixelFormat ) )
                {
                    //Use INT_MAX as buffer size, OpenGL already
                    //knows the size because the buffer is bound.
                    GLenum format, type;
                    GL3PlusMappings::getFormatAndType( pixelFormat, format, type );
                    OCGE( glGetTextureSubImage( texName, mipLevel, xPos, yPos,
                                                std::max( zPos, slicePos ),
                                                width, height, depthOrSlices, format, type,
                                                std::numeric_limits<int>::max(), 0 ) );
                }
                else
                {
                    OCGE( glGetCompressedTextureSubImage( texName, mipLevel, xPos, yPos,
                                                          std::max( zPos, slicePos ),
                                                          width, height, depthOrSlices,
                                                          std::numeric_limits<int>::max(), 0 ) );
                }
            }
        }
        else
        {
            GLenum format, type;
            GL3PlusMappings::getFormatAndType( pixelFormat, format, type );
            OCGE( glReadPixels( xPos, yPos, width, height, format, type, 0 ) );
        }

        if( accurateTracking )
        {
            OCGE( mAccurateFence = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 ) );
        }
    }
    //-----------------------------------------------------------------------------------
    TextureBox GL3PlusAsyncTextureTicket::mapImpl( uint32 slice )
    {
        waitForDownloadToFinish();

        TextureBox retVal;

        GLuint vboName = mTmpVboName ? mTmpVboName : mVboName;
        OCGE( glBindBuffer( GL_PIXEL_PACK_BUFFER, vboName ) );

        size_t sizeBytes = 0;
        const uint32 rowAlignment = 4u;

        if( mTmpVboName )
        {
            retVal = mSubregion;
            sizeBytes = retVal.bytesPerImage * std::max( mSubregion.getMaxZ(),
                                                         mSubregion.getMaxSlice() );
        }
        else
        {
            retVal = TextureBox( mWidth, mHeight, getDepth(), getNumSlices(),
                                 PixelFormatGpuUtils::getBytesPerPixel( mPixelFormatFamily ),
                                 getBytesPerRow(), getBytesPerImage() );
            sizeBytes = PixelFormatGpuUtils::getSizeBytes( mWidth, mHeight, mDepthOrSlices,
                                                           1u, mPixelFormatFamily,
                                                           rowAlignment );
        }

        if( PixelFormatGpuUtils::isCompressed( mPixelFormatFamily ) )
            retVal.setCompressedPixelFormat( mPixelFormatFamily );

        retVal.data = glMapBufferRange( GL_PIXEL_PACK_BUFFER, 0, sizeBytes, GL_MAP_READ_BIT );

        if( mTmpVboName )
        {
            //Offset to the beginning of the region.
            retVal.data = retVal.at( retVal.x, retVal.y, std::max( retVal.z,
                                                                   retVal.sliceStart + slice ) );
            retVal.x = 0;
            retVal.y = 0;
            retVal.z = 0;
            retVal.sliceStart = 0;
        }
        else
        {
            retVal.data = retVal.at( 0, 0, slice );
            retVal.numSlices -= slice;
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusAsyncTextureTicket::unmapImpl(void)
    {
        GLuint vboName = mTmpVboName ? mTmpVboName : mVboName;
        OCGE( glBindBuffer( GL_PIXEL_PACK_BUFFER, vboName ) );
        OCGE( glUnmapBuffer( GL_PIXEL_PACK_BUFFER ) );
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusAsyncTextureTicket::waitForDownloadToFinish(void)
    {
        if( mStatus != Downloading )
            return; //We're done.

        if( mAccurateFence )
        {
            mAccurateFence = GL3PlusVaoManager::waitFor( mAccurateFence );
        }
        else
        {
            mVaoManager->waitForSpecificFrameToFinish( mDownloadFrame );
            mNumInaccurateQueriesWasCalledInIssuingFrame = 0;
        }

        mStatus = Ready;
    }
    //-----------------------------------------------------------------------------------
    bool GL3PlusAsyncTextureTicket::queryIsTransferDone(void)
    {
        if( !AsyncTextureTicket::queryIsTransferDone() )
        {
            //Early out. The texture is not even finished being ready.
            //We didn't even start the actual download.
            return false;
        }

        bool retVal = false;

        if( mStatus != Downloading )
        {
            retVal = true;
        }
        else if( mAccurateFence )
        {
            //Ask GL API to return immediately and tells us about the fence
            GLenum waitRet = glClientWaitSync( mAccurateFence, 0, 0 );
            if( waitRet == GL_ALREADY_SIGNALED || waitRet == GL_CONDITION_SATISFIED )
            {
                OCGE( glDeleteSync( mAccurateFence ) );
                mAccurateFence = 0;
                if( mStatus != Mapped )
                    mStatus = Ready;
                retVal = true;
            }
        }
        else
        {
            if( mDownloadFrame == mVaoManager->getFrameCount() )
            {
                if( mNumInaccurateQueriesWasCalledInIssuingFrame > 3 )
                {
                    //Use is not calling vaoManager->update(). Likely it's stuck in an
                    //infinite loop checking if we're done, but we'll always return false.
                    //If so, switch to accurate tracking.
                    OCGE( mAccurateFence = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 ) );
                    OCGE( glFlush() );

                    LogManager::getSingleton().logMessage(
                                "WARNING: Calling AsyncTextureTicket::queryIsTransferDone too "
                                "often with innacurate tracking in the same frame this transfer "
                                "was issued. Switching to accurate tracking. If this is an accident, "
                                "wait until you've rendered a few frames before checking if it's done. "
                                "If this is on purpose, consider calling AsyncTextureTicket::download()"
                                " with accurate tracking enabled.", LML_CRITICAL );
                }

                ++mNumInaccurateQueriesWasCalledInIssuingFrame;
            }

            //We're downloading but have no fence. That means we don't have accurate tracking.
            retVal = mVaoManager->isFrameFinished( mDownloadFrame );
            ++mNumInaccurateQueriesWasCalledInIssuingFrame;
        }

        return retVal;
    }
}
