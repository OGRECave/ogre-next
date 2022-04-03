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

#ifndef __GLES2Texture_H__
#define __GLES2Texture_H__

#include "OgreGLES2Prerequisites.h"
#include "OgreGLES2Support.h"
#include "OgrePlatform.h"
#include "OgreRenderTexture.h"
#include "OgreTexture.h"
#include "OgreHardwarePixelBuffer.h"
#include "OgreGLES2ManagedResource.h"

namespace Ogre {
    class _OgreGLES2Export GLES2Texture : public Texture MANAGED_RESOURCE
    {
        public:
            // Constructor
            GLES2Texture(ResourceManager* creator, const String& name, ResourceHandle handle,
                const String& group, bool isManual, ManualResourceLoader* loader, 
                GLES2Support& support);

            virtual ~GLES2Texture();

            void createRenderTexture();
            /// @copydoc Texture::getBuffer
            v1::HardwarePixelBufferSharedPtr getBuffer(size_t face, size_t mipmap);

            // Takes the OGRE texture type (1d/2d/3d/cube) and returns the appropriate GL one
            GLenum getGLES2TextureTarget() const;

            GLuint getGLID() const
            {
                return mTextureID;
            }
            
            void getCustomAttribute(const String& name, void* pData);

        protected:
            /// @copydoc Texture::createInternalResourcesImpl
            void createInternalResourcesImpl();
            /// @copydoc Resource::prepareImpl
            void prepareImpl();
            /// @copydoc Resource::unprepareImpl
            void unprepareImpl();
            /// @copydoc Resource::loadImpl
            void loadImpl();
            /// @copydoc Resource::freeInternalResourcesImpl
            void freeInternalResourcesImpl();

            /** Internal method, create GLHardwarePixelBuffers for every face and
             mipmap level. This method must be called after the GL texture object was created,
             the number of mipmaps was set (GL_TEXTURE_MAX_LEVEL) and glTexImageXD was called to
             actually allocate the buffer
             */
            void _createSurfaceList();

            virtual void _autogenerateMipmaps();

            /// Used to hold images between calls to prepare and load.
            typedef SharedPtr<vector<Image>::type > LoadedImages;

            /** Vector of images that were pulled from disk by
             prepareLoad but have yet to be pushed into texture memory
             by loadImpl.  Images should be deleted by loadImpl and unprepareImpl.
             */
            LoadedImages mLoadedImages;

            /// Create gl texture
            void _createGLTexResource();
        
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
            /** See AndroidResource. */
            virtual void notifyOnContextLost();
        
            /** See AndroidResource. */
            virtual void notifyOnContextReset();
#endif

            GLuint mTextureID;
            GLES2Support& mGLSupport;
            
            /// Vector of pointers to subsurfaces
            typedef vector<v1::HardwarePixelBufferSharedPtr>::type SurfaceList;
            SurfaceList mSurfaceList;

    };
}

#endif
