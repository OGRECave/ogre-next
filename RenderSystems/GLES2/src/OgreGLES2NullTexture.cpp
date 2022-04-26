/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE-Next
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

#include "OgreGLES2NullTexture.h"
#include "OgreGLES2RenderSystem.h"
#include "OgreGLES2HardwarePixelBuffer.h"
#include "OgreGLES2RenderTexture.h"
#include "OgreGLES2FrameBufferObject.h"
#include "OgreRoot.h"
#include "OgreDepthBuffer.h"

namespace Ogre
{
    GLES2NullTexture::GLES2NullTexture( ResourceManager* creator,
                                            const String& name, ResourceHandle handle,
                                            const String& group, bool isManual,
                                            ManualResourceLoader* loader, GLES2Support& support )
        : GLES2Texture( creator, name, handle, group, isManual, loader, support )
    {
        if( !support.hasMinGLVersion( 4, 3 ) &&
            !support.checkExtension( "GL_ARB_framebuffer_no_attachments" ) )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "PF_NULL format requires OpenGL 4.3 or the "
                         "GL_ARB_framebuffer_no_attachments extension. "
                         "Try updating your video card drivers.",
                         "GLES2NullTexture::GLES2NullTexture" );
        }
    }
    //-----------------------------------------------------------------------------------
    GLES2NullTexture::~GLES2NullTexture()
    {
        // have to call this here rather than in Resource destructor
        // since calling virtual methods in base destructors causes crash
        if (isLoaded())
        {
            unload();
        }
        else
        {
            freeInternalResources();
        }
    }
    //-----------------------------------------------------------------------------------
    // Creation / loading methods
    void GLES2NullTexture::createInternalResourcesImpl()
    {
        _createSurfaceList();

        // Get final internal format.
        mFormat = getBuffer(0,0)->getFormat();

        mSize = calculateSize();
    }
    //-----------------------------------------------------------------------------------
    void GLES2NullTexture::freeInternalResourcesImpl()
    {
        mSurfaceList.clear();
    }
    //-----------------------------------------------------------------------------------
    void GLES2NullTexture::prepareImpl()
    {
    }
    //-----------------------------------------------------------------------------------
    void GLES2NullTexture::unprepareImpl()
    {
    }
    //-----------------------------------------------------------------------------------
    void GLES2NullTexture::loadImpl()
    {
        createRenderTexture();
    }
    //-----------------------------------------------------------------------------------
    void GLES2NullTexture::_createSurfaceList()
    {
        mSurfaceList.clear();

        for (uint8 face = 0; face < getNumFaces(); face++)
        {
            v1::HardwarePixelBuffer *buf = OGRE_NEW v1::GLES2NullPixelBuffer( this, mName,
                                                                                 mWidth, mHeight,
                                                                                 mDepth, mFormat );

            mSurfaceList.push_back( v1::HardwarePixelBufferSharedPtr(buf) );

            // Check for error
            if (buf->getWidth() == 0 ||
                buf->getHeight() == 0 ||
                buf->getDepth() == 0)
            {
                OGRE_EXCEPT(
                    Exception::ERR_RENDERINGAPI_ERROR,
                    "Zero sized texture surface on texture "+getName()+
                    " face "+StringConverter::toString(face)+
                    " mipmap 0"
                    ". The GL driver probably refused to create the texture.",
                    "GLES2NullTexture::_createSurfaceList");
            }
        }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
namespace v1
{
    GLES2NullPixelBuffer::GLES2NullPixelBuffer( GLES2NullTexture *parentTexture,
                                                      const String &baseName,
                                                      uint32 width, uint32 height,
                                                      uint32 Null, PixelFormat format ) :
        HardwarePixelBuffer( width, height, Null, format, false,
                             HardwareBuffer::HBU_STATIC_WRITE_ONLY, false, false ),
        mDummyRenderTexture( 0 )
    {
        String name = "NullTexture/" + StringConverter::toString((size_t)this) + "/" + baseName;

        mDummyRenderTexture = OGRE_NEW GLES2NullTextureTarget( parentTexture, name, this, 0 );
        mDummyRenderTexture->setDepthBufferPool( DepthBuffer::POOL_NO_DEPTH );

        //TODO: Should we do this?
        Root::getSingleton().getRenderSystem()->attachRenderTarget( *mDummyRenderTexture );
    }
    //-----------------------------------------------------------------------------------
    GLES2NullPixelBuffer::~GLES2NullPixelBuffer()
    {
        if( mDummyRenderTexture )
            Root::getSingleton().getRenderSystem()->destroyRenderTarget( mDummyRenderTexture->getName() );
    }
    //-----------------------------------------------------------------------------------
    PixelBox GLES2NullPixelBuffer::lockImpl( const Box &lockBox, LockOptions options )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "", "GLES2NullPixelBuffer::lockImpl" );
    }
    //-----------------------------------------------------------------------------------
    void GLES2NullPixelBuffer::unlockImpl()
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "", "GLES2NullPixelBuffer::unlockImpl" );
    }
    //-----------------------------------------------------------------------------------
    void GLES2NullPixelBuffer::_clearSliceRTT( size_t zoffset )
    {
        mDummyRenderTexture = 0;
    }
    //-----------------------------------------------------------------------------------
    void GLES2NullPixelBuffer::blitFromMemory( const PixelBox &src, const Box &dstBox )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "", "GLES2NullPixelBuffer::blitFromMemory" );
    }
    //-----------------------------------------------------------------------------------
    void GLES2NullPixelBuffer::blitToMemory( const Box &srcBox, const PixelBox &dst )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "", "GLES2NullPixelBuffer::blitToMemory" );
    }
    //-----------------------------------------------------------------------------------
    RenderTexture* GLES2NullPixelBuffer::getRenderTarget( size_t slice )
    {
        return mDummyRenderTexture;
    }
}
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    GLES2NullTextureTarget::GLES2NullTextureTarget( GLES2NullTexture *ultimateTextureOwner,
                                                          const String &name,
                                                          v1::HardwarePixelBuffer *buffer,
                                                          uint32 zoffset ) :
        RenderTexture( buffer, zoffset ),
        mUltimateTextureOwner( ultimateTextureOwner )
    {
        mName = name;
        mWidth      = ultimateTextureOwner->getWidth();
        mHeight     = ultimateTextureOwner->getHeight();
        mFormat     = ultimateTextureOwner->getFormat();
        mFSAA       = ultimateTextureOwner->getFSAA();
        mFSAAHint   = ultimateTextureOwner->getFSAAHint();
        mFsaaResolveDirty = true; //Should be permanent true.
    }
    //-----------------------------------------------------------------------------------
    GLES2NullTextureTarget::~GLES2NullTextureTarget()
    {
    }
    //-----------------------------------------------------------------------------------
    bool GLES2NullTextureTarget::attachDepthBuffer( DepthBuffer *depthBuffer, bool exactFormatMatch )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL,
                     "Null formats don't use a depth buffer. "
                     "Call setDepthBufferPool( DepthBuffer::POOL_NO_DEPTH ) "
                     "on this RTT before rendering!\n"
                     "If you're manually setting the compositor, "
                     "set TextureDefinition::depthBufferId to 0",
                     "GLES2NullTextureTarget::attachDepthBuffer" );

        return false;
    }
    //-----------------------------------------------------------------------------------
    void GLES2NullTextureTarget::getCustomAttribute( const String& name, void* pData )
    {
        if( name == "FBO" )
        {
            *static_cast<GLES2FrameBufferObject**>(pData) = 0;
        }
    }
}
