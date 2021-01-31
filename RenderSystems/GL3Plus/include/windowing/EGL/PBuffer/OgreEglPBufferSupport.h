/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE
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
#ifndef OGRE_EglPBufferSupport_H
#define OGRE_EglPBufferSupport_H

#include "OgreGL3PlusSupport.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace Ogre
{
    class _OgrePrivate EglPBufferSupport : public GL3PlusSupport
    {
    public:
        struct DeviceData
        {
            String name;
            FastArray<SampleDescription> fsaa;

            EGLDisplay eglDisplay;
            EGLConfig eglCfg;
            EGLSurface eglSurf;
            ::EGLContext eglCtx;

            DeviceData() : eglDisplay( 0 ), eglCfg( 0 ), eglSurf( 0 ), eglCtx( 0 ) {}

            bool operator==( const String &otherName ) const { return name == otherName; }
        };

    protected:
        FastArray<DeviceData> mDeviceData;
        FastArray<EGLDeviceEXT> mDevices;

        /**
         * Refresh config options to reflect dependencies
         */
        void refreshConfig( void );

        void initDevice( const size_t deviceIdx );
        void destroyDevice( const size_t deviceIdx );

    public:
        EglPBufferSupport();
        ~EglPBufferSupport();

        /// @copydoc see GL3PlusSupport::addConfig
        void addConfig( void );

        /// @copydoc see GL3PlusSupport::validateConfig
        String validateConfig( void );

        /// @copydoc see RenderSystem::getPriorityConfigOption
        virtual const char* getPriorityConfigOption( size_t idx ) const;

        /// @copydoc see RenderSystem::getPriorityConfigOption
        virtual size_t getNumPriorityConfigOptions( void ) const;

        /// @copydoc see GL3PlusSupport::setConfigOption
        void setConfigOption( const String &name, const String &value );

        /// @copydoc GL3PlusSupport::createWindow
        Window *createWindow( bool autoCreateWindow, GL3PlusRenderSystem *renderSystem,
                              const String &windowTitle );

        /// @copydoc RenderSystem::createRenderWindow
        Window *newWindow( const String &name, uint32 width, uint32 height, bool fullScreen,
                           const NameValuePairList *miscParams = 0 );

        /// @copydoc see GL3PlusSupport::start
        void start();

        /// @copydoc see GL3PlusSupport::stop
        void stop();

        /// @copydoc see GL3PlusSupport::getProcAddress
        void *getProcAddress( const char *procname ) const;

        const DeviceData *getCurrentDevice( void );

        /// Get the Display connection used for rendering
        /// This function establishes the initial connection when necessary.
        EGLDisplay getGLDisplay( void );

        uint32 getSelectedDeviceIdx( void ) const;
    };
}  // namespace Ogre

#endif  // OGRE_EglPBufferSupport_H
