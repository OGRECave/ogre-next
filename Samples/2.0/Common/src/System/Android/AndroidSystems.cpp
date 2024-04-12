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

#include "System/Android/AndroidSystems.h"

#include "OgreArchiveManager.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
#    include "Android/OgreAPKFileSystemArchive.h"
#    include "Android/OgreAPKZipArchive.h"

#    include <android_native_app_glue.h>
#endif

namespace Demo
{
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
    static AndroidSystems g_andrSystem;
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
    AndroidSystems::AndroidSystems() : mAndroidApp( 0 ), mNativeWindow( 0 ), mJniProvider( 0 ) {}
    //-------------------------------------------------------------------------
    void AndroidSystems::setAndroidApp( android_app *androidApp )
    {
        g_andrSystem.mAndroidApp = androidApp;
    }
    //-------------------------------------------------------------------------
    android_app *AndroidSystems::getAndroidApp() { return g_andrSystem.mAndroidApp; }
    //-------------------------------------------------------------------------
    void AndroidSystems::setNativeWindow( ANativeWindow *nativeWindow )
    {
        g_andrSystem.mNativeWindow = nativeWindow;
    }
    //-------------------------------------------------------------------------
    ANativeWindow *AndroidSystems::getNativeWindow() { return g_andrSystem.mNativeWindow; }
    //-------------------------------------------------------------------------
    void AndroidSystems::setJniProvider( Ogre::AndroidJniProvider *provider )
    {
        g_andrSystem.mJniProvider = provider;
    }
    //-------------------------------------------------------------------------
    Ogre::AndroidJniProvider *AndroidSystems::getJniProvider() { return g_andrSystem.mJniProvider; }
    //-------------------------------------------------------------------------
    void AndroidSystems::registerArchiveFactories()
    {
        AAssetManager *assetMgr = g_andrSystem.mAndroidApp->activity->assetManager;
        Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();
        archiveManager.addArchiveFactory( new Ogre::APKFileSystemArchiveFactory( assetMgr ) );
        archiveManager.addArchiveFactory( new Ogre::APKZipArchiveFactory( assetMgr ) );
    }
    //-------------------------------------------------------------------------
    Ogre::DataStreamPtr AndroidSystems::openFile( const Ogre::String &filename )
    {
        Ogre::DataStreamPtr stream;

        AAssetManager *assetMgr = g_andrSystem.mAndroidApp->activity->assetManager;
        AAsset *asset = AAssetManager_open( assetMgr, filename.c_str(), AASSET_MODE_BUFFER );
        if( asset )
        {
            const size_t length = static_cast<size_t>( AAsset_getLength( asset ) );
            void *membuf = OGRE_MALLOC( length, Ogre::MEMCATEGORY_GENERAL );
            memcpy( membuf, AAsset_getBuffer( asset ), length );
            AAsset_close( asset );

            stream = Ogre::DataStreamPtr( new Ogre::MemoryDataStream( membuf, length, true, true ) );
        }
        return stream;
    }
    //-------------------------------------------------------------------------
    std::string AndroidSystems::getFilesDir( const bool bInternal )
    {
        const char *dataPath = bInternal ? g_andrSystem.mAndroidApp->activity->internalDataPath
                                         : g_andrSystem.mAndroidApp->activity->externalDataPath;

        std::string retVal;
        if( dataPath )
        {
            retVal = dataPath;
            if( !retVal.empty() && retVal.back() != '/' )
                retVal.push_back( '/' );
        }

        return retVal;
    }
    //-------------------------------------------------------------------------
    bool AndroidSystems::isAndroid() { return true; }
#else
    //-------------------------------------------------------------------------
    AndroidSystems::AndroidSystems() {}
    void AndroidSystems::setAndroidApp( android_app * ) {}
    android_app *AndroidSystems::getAndroidApp() { return 0; }
    void AndroidSystems::setNativeWindow( ANativeWindow * ) {}
    ANativeWindow *AndroidSystems::getNativeWindow() { return 0; }
    void AndroidSystems::setJniProvider( Ogre::AndroidJniProvider *provider ) {}
    Ogre::AndroidJniProvider *AndroidSystems::getJniProvider() { return 0; }
    std::string AndroidSystems::getFilesDir( const bool /*bInternal*/ ) { return ""; }
    bool AndroidSystems::isAndroid() { return false; }
    void AndroidSystems::registerArchiveFactories() {}
#endif
}  // namespace Demo
