/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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

#ifndef _OgreVulkanXcbWindow_H_
#define _OgreVulkanXcbWindow_H_

#include "OgreVulkanPrerequisites.h"

#include "OgreVulkanWindow.h"

#include "OgreStringVector.h"

#include "OgreHeaderPrefix.h"

struct xcb_connection_t;
struct xcb_screen_t;
typedef uint32_t xcb_window_t;
typedef uint32_t xcb_atom_t;

namespace Ogre
{
    class _OgreVulkanExport VulkanXcbWindow : public VulkanWindow
    {
        xcb_connection_t *mConnection;
        xcb_screen_t *mScreen;
        xcb_window_t mXcbWindow;

        xcb_atom_t mWmProtocols;
        xcb_atom_t mWmDeleteWindow;
        xcb_atom_t mWmNetState;
        xcb_atom_t mWmFullscreen;

        bool mVisible;
        bool mHidden;
        bool mIsTopLevel;
        bool mIsExternal;

        void initConnection( void );

        void createWindow( const String &windowName, uint32 width, uint32 height,
                           const NameValuePairList *miscParams );

    public:
        void switchMode( uint32 width, uint32 height, uint32 frequencyNum, uint32 frequencyDen );
        void switchFullScreen( const bool bFullscreen );

    public:
        VulkanXcbWindow( const String &title, uint32 width, uint32 height, bool fullscreenMode );
        ~VulkanXcbWindow();

        static const char *getRequiredExtensionName( void );

        virtual void destroy( void );
        virtual void _initialize( TextureGpuManager *textureGpuManager,
                                  const NameValuePairList *miscParams );

        virtual void reposition( int32 left, int32 top );
        virtual void requestFullscreenSwitch( bool goFullscreen, bool borderless, uint32 monitorIdx,
                                              uint32 width, uint32 height, uint32 frequencyNumerator,
                                              uint32 frequencyDenominator );
        virtual void requestResolution( uint32 width, uint32 height );
        virtual void windowMovedOrResized( void );

        virtual void _setVisible( bool visible );
        virtual bool isVisible( void ) const;
        virtual void setHidden( bool hidden );
        virtual bool isHidden( void ) const;

        virtual void getCustomAttribute( IdString name, void *pData );
    };

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
