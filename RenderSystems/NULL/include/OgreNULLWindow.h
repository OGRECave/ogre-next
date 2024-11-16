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
#ifndef _OgreNULLWindow_H_
#define _OgreNULLWindow_H_

#include "OgreNULLPrerequisites.h"

#include "OgreWindow.h"

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    class NULLWindow : public Window
    {
        bool mClosed;

    public:
        NULLWindow( const String &title, uint32 width, uint32 height, bool fullscreenMode );
        ~NULLWindow() override;

        void destroy() override;

        void reposition( int32 left, int32 top ) override;
        void requestResolution( uint32 width, uint32 height ) override;
        void requestFullscreenSwitch( bool goFullscreen, bool borderless, uint32 monitorIdx,
                                      uint32 width, uint32 height, uint32 frequencyNumerator,
                                      uint32 frequencyDenominator ) override;
        bool isClosed() const override;

        void _setVisible( bool visible ) override;
        bool isVisible() const override;
        void setHidden( bool hidden ) override;
        bool isHidden() const override;
        void _initialize( TextureGpuManager                     *textureGpuManager,
                          const NameValuePairList *ogre_nullable miscParams ) override;
        void swapBuffers() override;
    };

    OGRE_ASSUME_NONNULL_END
}  // namespace Ogre

#endif
