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

#include "OgreGLES2DepthTexture.h"
#include "OgreGLES2RenderSystem.h"
#include "OgreGLES2HardwarePixelBuffer.h"
#include "OgreGLES2RenderTexture.h"
#include "OgreGLES2FrameBufferObject.h"
#include "OgreGLES2DepthBuffer.h"
#include "OgreRoot.h"

namespace Ogre
{
    GLES2DepthTexture::GLES2DepthTexture( bool shareableDepthBuffer, ResourceManager* creator,
                                              const String& name, ResourceHandle handle,
                                              const String& group, bool isManual,
                                              ManualResourceLoader* loader, GLES2Support& support )
        : GLES2Texture( creator, name, handle, group, isManual, loader, support ),
          mShareableDepthBuffer( shareableDepthBuffer )
    {
        mMipmapsHardwareGenerated = true;
    }
    //-----------------------------------------------------------------------------------
    GLES2DepthTexture::~GLES2DepthTexture()
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
    void GLES2DepthTexture::_setGlTextureName( GLuint textureName )
    {
        mTextureID = textureName;
    }
    //-----------------------------------------------------------------------------------
    // Creation / loading methods
    void GLES2DepthTexture::createInternalResourcesImpl(void)
    {
        _createSurfaceList();

        // Get final internal format.
        mFormat = getBuffer(0,0)->getFormat();

        mSize = calculateSize();
    }
    //-----------------------------------------------------------------------------------
    void GLES2DepthTexture::freeInternalResourcesImpl()
    {
        mSurfaceList.clear();
        mTextureID = 0;
    }
    //-----------------------------------------------------------------------------------
    void GLES2DepthTexture::prepareImpl()
    {
    }
    //-----------------------------------------------------------------------------------
    void GLES2DepthTexture::unprepareImpl()
    {
    }
    //-----------------------------------------------------------------------------------
    void GLES2DepthTexture::loadImpl()
    {
        createRenderTexture();
    }
    //-----------------------------------------------------------------------------------
    void GLES2DepthTexture::_createSurfaceList()
    {
        mSurfaceList.clear();

        for (uint8 face = 0; face < getNumFaces(); face++)
        {
            v1::HardwarePixelBuffer *buf = OGRE_NEW v1::GLES2DepthPixelBuffer( this, mName,
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
                    "GLES2DepthTexture::_createSurfaceList");
            }
        }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
namespace v1
{
    GLES2DepthPixelBuffer::GLES2DepthPixelBuffer( GLES2DepthTexture *parentTexture,
                                                      const String &baseName,
                                                      uint32 width, uint32 height,
                                                      uint32 depth, PixelFormat format ) :
        HardwarePixelBuffer( width, height, depth, format, false,
                             HardwareBuffer::HBU_STATIC_WRITE_ONLY, false, false ),
        mDummyRenderTexture( 0 )
    {
        String name = "DepthTexture/" + StringConverter::toString((size_t)this) + "/" + baseName;

        mDummyRenderTexture = OGRE_NEW GLES2DepthTextureTarget( parentTexture, name, this, 0 );
        mDummyRenderTexture->setPreferDepthTexture( true );
        mDummyRenderTexture->setDesiredDepthBufferFormat( format );

        RenderSystem *renderSystem = Root::getSingleton().getRenderSystem();
        renderSystem->setDepthBufferFor( mDummyRenderTexture, true );

        //TODO: Should we do this?
        Root::getSingleton().getRenderSystem()->attachRenderTarget( *mDummyRenderTexture );
    }
    //-----------------------------------------------------------------------------------
    GLES2DepthPixelBuffer::~GLES2DepthPixelBuffer()
    {
        if( mDummyRenderTexture )
            Root::getSingleton().getRenderSystem()->destroyRenderTarget( mDummyRenderTexture->getName() );
    }
    //-----------------------------------------------------------------------------------
    PixelBox GLES2DepthPixelBuffer::lockImpl( const Box &lockBox, LockOptions options )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "", "GLES2DepthPixelBuffer::lockImpl" );
    }
    //-----------------------------------------------------------------------------------
    void GLES2DepthPixelBuffer::unlockImpl(void)
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "", "GLES2DepthPixelBuffer::unlockImpl" );
    }
    //-----------------------------------------------------------------------------------
    void GLES2DepthPixelBuffer::_clearSliceRTT( size_t zoffset )
    {
        mDummyRenderTexture = 0;
    }
    //-----------------------------------------------------------------------------------
    void GLES2DepthPixelBuffer::blitFromMemory( const PixelBox &src, const Box &dstBox )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "", "GLES2DepthPixelBuffer::blitFromMemory" );
    }
    //-----------------------------------------------------------------------------------
    void GLES2DepthPixelBuffer::blitToMemory( const Box &srcBox, const PixelBox &dst )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "", "GLES2DepthPixelBuffer::blitToMemory" );
    }
    //-----------------------------------------------------------------------------------
    RenderTexture* GLES2DepthPixelBuffer::getRenderTarget( size_t slice )
    {
        return mDummyRenderTexture;
    }
}
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    GLES2DepthTextureTarget::GLES2DepthTextureTarget( GLES2DepthTexture *ultimateTextureOwner,
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

        if( !ultimateTextureOwner->getShareableDepthBuffer() )
            mDepthBufferPoolId = DepthBuffer::POOL_NON_SHAREABLE;
    }
    //-----------------------------------------------------------------------------------
    GLES2DepthTextureTarget::~GLES2DepthTextureTarget()
    {
    }
    //-----------------------------------------------------------------------------------
    void GLES2DepthTextureTarget::setDepthBufferPool( uint16 poolId )
    {
        const uint16 oldPoolId = mDepthBufferPoolId;

        RenderTexture::setDepthBufferPool( poolId );

        if( oldPoolId != poolId )
        {
            RenderSystem *renderSystem = Root::getSingleton().getRenderSystem();
            renderSystem->setDepthBufferFor( this, true );
        }
    }
    //-----------------------------------------------------------------------------------
    bool GLES2DepthTextureTarget::attachDepthBuffer( DepthBuffer *depthBuffer, bool exactFormatMatch )
    {
        bool retVal = RenderTexture::attachDepthBuffer( depthBuffer, exactFormatMatch );

        if( mDepthBuffer )
        {
            assert( dynamic_cast<GLES2DepthBuffer*>(mDepthBuffer) );
            GLES2DepthBuffer *gles2DepthBuffer = static_cast<GLES2DepthBuffer*>(mDepthBuffer);
            mUltimateTextureOwner->_setGlTextureName( gles2DepthBuffer->getDepthBuffer() );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void GLES2DepthTextureTarget::detachDepthBuffer(void)
    {
        RenderTexture::detachDepthBuffer();
        mUltimateTextureOwner->_setGlTextureName( 0 );
    }
    //-----------------------------------------------------------------------------------
    void GLES2DepthTextureTarget::getFormatsForPso(
            PixelFormat outFormats[OGRE_MAX_MULTIPLE_RENDER_TARGETS],
            bool outHwGamma[OGRE_MAX_MULTIPLE_RENDER_TARGETS] ) const
    {
        for( size_t i=0; i<OGRE_MAX_MULTIPLE_RENDER_TARGETS; ++i )
        {
            outFormats[i] = PF_NULL;
            outHwGamma[i] = false;
        }
    }
    //-----------------------------------------------------------------------------------
    void GLES2DepthTextureTarget::getCustomAttribute( const String& name, void* pData )
    {
        if( name == "FBO" )
        {
            *static_cast<GLES2FrameBufferObject**>(pData) = 0;
        }
        else if (name == "GL_MULTISAMPLEFBOID")
        {
            if( mDepthBuffer )
            {
                assert( dynamic_cast<GLES2DepthBuffer*>(mDepthBuffer) );
                GLES2DepthBuffer *gles2DepthBuffer = static_cast<GLES2DepthBuffer*>(mDepthBuffer);
                *static_cast<GLuint*>(pData) = gles2DepthBuffer->getDepthBuffer();
            }
        }
    }
}
