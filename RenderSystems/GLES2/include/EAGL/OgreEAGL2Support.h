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

#ifndef __EAGL2Support_H__
#define __EAGL2Support_H__

#include "OgreGLES2Support.h"

#ifdef __OBJC__
// Forward declarations
@class CAEAGLLayer;
@class UIWindow;
@class NSString;
typedef UIWindow *NativeWindowType;
#endif

namespace Ogre {
    class EAGL2Window;
    class EAGLES2Context;

    class _OgrePrivate EAGL2Support : public GLES2Support
    {
        public:
            EAGL2Support();
            virtual ~EAGL2Support();

            void start();
            void stop();
            void addConfig();
            String validateConfig();
            virtual String getDisplayName();
            CFDictionaryRef chooseGLConfig(const GLint *attribList, GLint *nElements);
            GLint getGLConfigAttrib(CFDictionaryRef fbConfig, GLint attribute, GLint *value);
            float getCurrentOSVersion() { return mCurrentOSVersion; }
            void * getProcAddress(const char* procname) const;
            RenderWindow * createWindow(bool autoCreateWindow,
                                           GLES2RenderSystem *renderSystem,
                                           const String& windowTitle);

            RenderWindow * newWindow(const String& name,
                                        unsigned int width, unsigned int height,
                                        bool fullScreen,
                                        const NameValuePairList *miscParams = 0);

#ifdef __OBJC__
            bool portraitIsSupported();
            bool interfaceOrientationIsSupported(NSString *orientation);

            EAGLES2Context * createNewContext(CAEAGLLayer *drawable, EAGLSharegroup *group) const;
            CFDictionaryRef getGLConfigFromContext(EAGLES2Context context);
            CFDictionaryRef getGLConfigFromDrawable(CAEAGLLayer *drawable, unsigned int *w, unsigned int *h);
#endif
            CFDictionaryRef selectGLConfig(const int* minAttribs, const int *maxAttribs);
        protected:
            float mCurrentOSVersion;
    };
}

#endif
