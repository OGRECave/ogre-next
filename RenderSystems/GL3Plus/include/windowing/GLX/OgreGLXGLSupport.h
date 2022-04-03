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
#ifndef OGRE_GLXGLSupport_H
#define OGRE_GLXGLSupport_H

#include "OgreGL3PlusSupport.h"
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#include <GL/glxext.h>

namespace Ogre
{
    class _OgrePrivate GLXGLSupport final : public GL3PlusSupport
    {
    public:
        GLXGLSupport();
        ~GLXGLSupport() override;

        Atom mAtomDeleteWindow;
        Atom mAtomFullScreen;
        Atom mAtomState;

        /** @copydoc see GL3PlusSupport::addConfig */
        void addConfig() override;

        /** @copydoc see GL3PlusSupport::validateConfig */
        String validateConfig() override;

        /** @copydoc see GL3PlusSupport::setConfigOption */
        void setConfigOption( const String &name, const String &value ) override;

        /// @copydoc GL3PlusSupport::createWindow
        Window *createWindow( bool autoCreateWindow, GL3PlusRenderSystem *renderSystem,
                              const String &windowTitle ) override;

        /// @copydoc RenderSystem::createRenderWindow
        Window *newWindow( const String &name, uint32 width, uint32 height, bool fullScreen,
                           const NameValuePairList *miscParams = 0 ) override;

        /** @copydoc see GL3PlusSupport::start */
        void start() override;

        /** @copydoc see GL3PlusSupport::stop */
        void stop() override;

        /** @copydoc see GL3PlusSupport::initialiseExtensions */
        void initialiseExtensions() override;

        /** @copydoc see GL3PlusSupport::getProcAddress */
        void *getProcAddress( const char *procname ) const override;

        // The remaining functions are internal to the GLX Rendersystem:

        /**
         * Get the name of the display and screen used for rendering
         *
         * Ogre normally opens its own connection to the X server
         * and renders onto the screen where the user logged in
         *
         * However, if Ogre is passed a current GL context when the first
         * RenderTarget is created, then it will connect to the X server
         * using the same connection as that GL context and direct all
         * subsequent rendering to the screen targeted by that GL context.
         *
         * @returns              Display name.
         */
        String getDisplayName();

        /**
         * Get the Display connection used for rendering
         *
         * This function establishes the initial connection when necessary.
         *
         * @returns              Display connection
         */
        Display *getGLDisplay();

        /**
         * Get the Display connection used for window management & events
         *
         * @returns              Display connection
         */
        Display *getXDisplay();

        /**
         * Switch video modes
         *
         * @param width   Receiver for requested and final width
         * @param height         Receiver for requested and final drawable height
         * @param height         Receiver for requested and final drawable frequency
         */
        void switchMode( uint32 width, uint32 height, short frequency );

        /**
         * Switch back to original video mode
         */
        void switchMode();

        /**
         * Loads an icon from an Ogre resource into the X Server. This currently only
         * works for 24 and 32 bit displays. The image must be findable by the Ogre
         * resource system, and of format PF_A8R8G8B8.
         *
         * @param display       X display
         * @param name     Name of image to load
         * @param pix           Receiver for the output pixmap
         * @param mask     Receiver for the output mask (alpha bitmap)
         * @returns               true on success
         */
        bool loadIcon( const String &name, Pixmap *pix, Pixmap *mask );

        /**
         * Get the GLXFBConfig used to create a ::GLXContext
         *
         * @param drawable   GLXContext
         * @returns               GLXFBConfig used to create the context
         */
        GLXFBConfig getFBConfigFromContext( ::GLXContext context );

        /**
         * Get the GLXFBConfig used to create a GLXDrawable.
         * Caveat: GLX version 1.3 is needed when the drawable is a GLXPixmap
         *
         * @param drawable   GLXDrawable
         * @param width   Receiver for the drawable width
         * @param height         Receiver for the drawable height
         * @returns               GLXFBConfig used to create the drawable
         */
        GLXFBConfig getFBConfigFromDrawable( GLXDrawable drawable, unsigned int *width,
                                             unsigned int *height );

        /**
         * Select an FBConfig given a list of required and a list of desired properties
         *
         * @param minAttribs FBConfig attributes that must be provided with minimum values
         * @param maxAttribs FBConfig attributes that are desirable with maximum values
         * @returns               GLXFBConfig with attributes or 0 when unsupported.
         */
        GLXFBConfig selectFBConfig( const int *minAttribs, const int *maxAttribs );

        /**
         * Gets a GLXFBConfig compatible with a VisualID
         *
         * Some platforms fail to implement glXGetFBconfigFromVisualSGIX as
         * part of the GLX_SGIX_fbconfig extension, but this portable
         * alternative suffices for the creation of compatible contexts.
         *
         * @param visualid   VisualID
         * @returns               FBConfig for VisualID
         */
        GLXFBConfig getFBConfigFromVisualID( VisualID visualid );

        /**
         * Portable replacement for glXChooseFBConfig
         */
        GLXFBConfig *chooseFBConfig( const GLint *attribList, GLint *nElements );

        /**
         * Portable replacement for glXCreateNewContext
         */
        ::GLXContext createNewContext( GLXFBConfig fbConfig, GLint renderType, ::GLXContext shareList,
                                       GLboolean direct ) const;

        /**
         * Portable replacement for glXGetFBConfigAttrib
         */
        GLint getFBConfigAttrib( GLXFBConfig fbConfig, GLint attribute, GLint *value );

        /**
         * Portable replacement for glXGetVisualFromFBConfig
         */
        XVisualInfo *getVisualFromFBConfig( GLXFBConfig fbConfig );

    private:
        /**
         * Refresh config options to reflect dependencies
         */
        void refreshConfig();

        Display *mGLDisplay;  // used for GL/GLX commands
        Display *mXDisplay;   // used for other X commands and events
        bool     mIsExternalDisplay;

        typedef std::pair<uint, uint>       ScreenSize;
        typedef short                       Rate;
        typedef std::pair<ScreenSize, Rate> VideoMode;
        typedef std::vector<VideoMode>      VideoModes;

        VideoModes mVideoModes;
        VideoMode  mOriginalMode;
        VideoMode  mCurrentMode;

        StringVector mSampleLevels;
    };
}  // namespace Ogre

#endif  // OGRE_GLXGLSupport_H
