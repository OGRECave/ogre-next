/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#ifndef _Demo_AndroidSystems_H_
#define _Demo_AndroidSystems_H_

#include "OgrePrerequisites.h"

struct android_app;
struct ANativeWindow;

namespace Ogre
{
    class AndroidJniProvider;
}

namespace Demo
{
    /// Utility class to load plugins statically
    class AndroidSystems
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
        android_app   *mAndroidApp;
        ANativeWindow *mNativeWindow;

        Ogre::AndroidJniProvider *mJniProvider;
#endif

    public:
        AndroidSystems();

        static void         setAndroidApp( android_app *androidApp );
        static android_app *getAndroidApp();

        static void           setNativeWindow( ANativeWindow *nativeWindow );
        static ANativeWindow *getNativeWindow();

        static void                      setJniProvider( Ogre::AndroidJniProvider *provider );
        static Ogre::AndroidJniProvider *getJniProvider();

        /**
        On Android platforms:
            Opens a file in an APK and returns a DataStream smart pointer that can be read from
        On non-Android platforms:
            Returns the same string as received (passthrough), so that receiver can use traditional
            file-reading interfaces to open the file
        */
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
        static Ogre::DataStreamPtr openFile( const Ogre::String &filename );
#else
        static const Ogre::String &openFile( const Ogre::String &filename ) { return filename; }
#endif

        /** Returns a R/W folder for private usage
        @param bInternal
            When true, returns an internal path that cannot be accessed by users
            or other apps on non-rooted phones (unless the APK is marked as debuggable)

            When false, returns an external path that can be accessed by users,
            although most apps are restricted and cannot see (it can often
            be accessed by hooking the phone to the PC via USB)
            This folder is usually in
                /storage/emulated/0/Android/data/com.example.projectname/files
            But for most users they access it by going to
                Android/data/com.example.projectname/files
        */
        static std::string getFilesDir( const bool bInternal = true );

        static bool isAndroid();

        static void registerArchiveFactories();
    };
}  // namespace Demo

#endif
