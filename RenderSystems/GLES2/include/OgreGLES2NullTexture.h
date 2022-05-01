/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE-Next
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org/

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

#ifndef __GLES2NullTexture_H__
#define __GLES2NullTexture_H__

#include "OgreGLES2Texture.h"

namespace Ogre
{
    class _OgreGLES2Export GLES2NullTexture : public GLES2Texture
    {
    public:
        // Constructor
        GLES2NullTexture( ResourceManager *creator, const String &name, ResourceHandle handle,
                          const String &group, bool isManual, ManualResourceLoader *loader,
                          GLES2Support &support );

        virtual ~GLES2NullTexture();

    protected:
        /// @copydoc Texture::createInternalResourcesImpl
        virtual void createInternalResourcesImpl();
        /// @copydoc Resource::freeInternalResourcesImpl
        virtual void freeInternalResourcesImpl();

        virtual void _autogenerateMipmaps() {}

        /// @copydoc Resource::prepareImpl
        virtual void prepareImpl();
        /// @copydoc Resource::unprepareImpl
        virtual void unprepareImpl();
        /// @copydoc Resource::loadImpl
        virtual void loadImpl();

        /// internal method, create GLES2HardwarePixelBuffers for every face and mipmap level.
        void _createSurfaceList();
    };

    namespace v1
    {
        class _OgreGLES2Export GLES2NullPixelBuffer : public HardwarePixelBuffer
        {
        protected:
            RenderTexture *mDummyRenderTexture;

            virtual PixelBox lockImpl( const Box &lockBox, LockOptions options );
            virtual void unlockImpl();

            /// Notify HardwarePixelBuffer of destruction of render target.
            virtual void _clearSliceRTT( size_t zoffset );

        public:
            GLES2NullPixelBuffer( GLES2NullTexture *parentTexture, const String &baseName, uint32 width,
                                  uint32 height, uint32 Null, PixelFormat format );
            virtual ~GLES2NullPixelBuffer();

            virtual void blitFromMemory( const PixelBox &src, const Box &dstBox );
            virtual void blitToMemory( const Box &srcBox, const PixelBox &dst );
            virtual RenderTexture *getRenderTarget( size_t slice = 0 );
        };
    }  // namespace v1

    class _OgreGLES2Export GLES2NullTextureTarget : public RenderTexture  // GLES2RenderTexture
    {
        GLES2NullTexture *mUltimateTextureOwner;

    public:
        GLES2NullTextureTarget( GLES2NullTexture *ultimateTextureOwner, const String &name,
                                v1::HardwarePixelBuffer *buffer, uint32 zoffset );
        virtual ~GLES2NullTextureTarget();

        virtual bool requiresTextureFlipping() const { return true; }

        virtual bool getForceDisableColourWrites() const { return true; }

        /// Null buffers never resolve; only colour buffers do. (we need mFsaaResolveDirty to be always
        /// true so that the proper path is taken in GLES2Texture::getGLID)
        virtual void setFsaaResolveDirty() {}
        virtual void setFsaaResolved() {}

        /// Notifies the ultimate texture owner the buffer changed
        virtual bool attachDepthBuffer( DepthBuffer *depthBuffer, bool exactFormatMatch );

        void getCustomAttribute( const String &name, void *pData );
    };
}  // namespace Ogre

#endif
