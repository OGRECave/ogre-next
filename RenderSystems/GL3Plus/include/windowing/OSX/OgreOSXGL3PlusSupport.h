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

#ifndef OGRE_OSXGLSupport_H
#define OGRE_OSXGLSupport_H

#include "OgreGL3PlusSupport.h"

namespace Ogre
{
    class GL3PlusPBuffer;
    
class OSXGL3PlusSupport : public GL3PlusSupport
{
public:
    OSXGL3PlusSupport();

    virtual ~OSXGL3PlusSupport();

    /// @copydoc GL3PlusSupport::addConfig
    void addConfig( void ) override;

    /// @copydoc GL3PlusSupport::validateConfig
    String validateConfig( void ) override;

    /// @copydoc GL3PlusSupport::createWindow
    Window* createWindow( bool autoCreateWindow, GL3PlusRenderSystem* renderSystem, const String& windowTitle ) override;

    /// @copydoc GL3PlusSupport::newWindow
    virtual Window* newWindow( const String &name, unsigned int width, unsigned int height, 
        bool fullScreen, const NameValuePairList *miscParams = 0 ) override;
    
    /// @copydoc GL3PlusSupport::start
    void start() override;

    /// @copydoc GL3PlusSupport::stop
    void stop() override;

    /// @copydoc GL3PlusSupport::getProcAddress
    void* getProcAddress(const char *procname) const override;

private:
    // Core Foundation Array callback function for sorting, must be static for the function ptr
    static CFComparisonResult _compareModes(const void *val1, const void *val2, void *context);

}; // class OSXGL3PlusSupport

} // namespace Ogre

#endif // OGRE_OSXGLSupport_H
