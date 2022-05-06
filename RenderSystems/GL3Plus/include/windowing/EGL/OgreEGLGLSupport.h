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
#ifndef OGRE_EGLGLSupport_H
#define OGRE_EGLGLSupport_H

#include "OgreGL3PlusSupport.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace Ogre
{
    class _OgrePrivate EGLGLSupport : public GL3PlusSupport
    {
        FastArray<String>       mDeviceNames;
        FastArray<EGLDeviceEXT> mDevices;

        EGLDisplay mEglDisplay;

        /**
         * Refresh config options to reflect dependencies
         */
        void refreshConfig();

    public:
        EGLGLSupport();
        ~EGLGLSupport();

        /// @copydoc GL3PlusSupport::addConfig
        void addConfig();

        /// @copydoc GL3PlusSupport::validateConfig
        String validateConfig();

        /// @copydoc GL3PlusSupport::setConfigOption
        void setConfigOption( const String &name, const String &value );

        /// @copydoc GL3PlusSupport::createWindow
        Window *createWindow( bool autoCreateWindow, GL3PlusRenderSystem *renderSystem,
                              const String &windowTitle );

        /// @copydoc Root::createRenderWindow
        Window *newWindow( const String &name, uint32 width, uint32 height, bool fullScreen,
                           const NameValuePairList *miscParams = 0 );

        /// @copydoc GL3PlusSupport::start
        void start();

        /// @copydoc GL3PlusSupport::stop
        void stop();

        /// @copydoc GL3PlusSupport::getProcAddress
        void *getProcAddress( const char *procname ) const;

        ::EGLContext createNewContext( EGLDisplay eglDisplay, EGLConfig eglCfg,
                                       ::EGLContext shareList ) const;

        /// Get the Display connection used for rendering
        /// This function establishes the initial connection when necessary.
        EGLDisplay getGLDisplay();

        uint32 getSelectedDeviceIdx() const;
    };
}  // namespace Ogre

#endif  // OGRE_EGLGLSupport_H
