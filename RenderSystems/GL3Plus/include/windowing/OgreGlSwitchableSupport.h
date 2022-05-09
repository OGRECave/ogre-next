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
#ifndef OGRE_GlSwitchableSupport_H
#define OGRE_GlSwitchableSupport_H

#include "OgreGL3PlusSupport.h"

namespace Ogre
{
    class _OgrePrivate GlSwitchableSupport : public GL3PlusSupport
    {
        enum InterfaceType
        {
            WindowNative,  // GLX / WGL / Cocoa
            HeadlessEgl    // EGL headless
        };

        struct Interface
        {
            InterfaceType   type;
            GL3PlusSupport *support;

            Interface( InterfaceType _type, GL3PlusSupport *_support ) :
                type( _type ),
                support( _support )
            {
            }
        };

        uint8                mSelectedInterface;
        bool                 mInterfaceSelected;
        FastArray<Interface> mAvailableInterfaces;

        static const char *getInterfaceName( InterfaceType interface );

        /**
         * Refresh config options to reflect dependencies
         */
        void refreshConfig();

    public:
        GlSwitchableSupport();
        ~GlSwitchableSupport();

        /// @copydoc GL3PlusSupport::addConfig
        void addConfig();

        /// @copydoc GL3PlusSupport::validateConfig
        String validateConfig();

        /// @copydoc GL3PlusSupport::setConfigOption
        void setConfigOption( const String &name, const String &value );

        /// @copydoc RenderSystem::getPriorityConfigOption
        virtual const char *getPriorityConfigOption( size_t idx ) const;

        /// @copydoc RenderSystem::getPriorityConfigOption
        virtual size_t getNumPriorityConfigOptions() const;

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

        uint8 findSelectedInterfaceIdx() const;
    };
}  // namespace Ogre

#endif  // OGRE_GlSwitchableSupport_H
