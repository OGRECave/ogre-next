/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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

#include <android/log.h>
#include <android_native_app_glue.h>

#include "System/Android/AndroidSystems.h"
#include "System/MainEntryPoints.h"

#include "GameState.h"
#include "GraphicsSystem.h"
#include "LogicSystem.h"

#include "Compositor/OgreCompositorManager2.h"
#include "OgreCamera.h"
#include "OgreRoot.h"
#include "OgreTimer.h"
#include "OgreWindow.h"
#include "OgreWindowEventUtilities.h"

#include "OgreVulkanPlugin.h"
#include "Windowing/Android/OgreVulkanAndroidWindow.h"

#include <fstream>

//-----------------------------------------------------------------------------
std::string Android_getCacheFolder()
{
    android_app *app = Demo::AndroidSystems::getAndroidApp();

    JNIEnv *jni = 0;
    app->activity->vm->AttachCurrentThread( &jni, nullptr );

    ANativeActivity *nativeActivity = app->activity;
    jclass classDef = jni->GetObjectClass( nativeActivity->clazz );
    jmethodID getCacheFolder = jni->GetMethodID( classDef, "getCacheFolder", "()Ljava/lang/String;" );
    jstring cacheDir = (jstring)jni->CallObjectMethod( nativeActivity->clazz, getCacheFolder );

    const char *cacheDirCStr = jni->GetStringUTFChars( cacheDir, nullptr );
    std::string retVal( cacheDirCStr );
    jni->ReleaseStringUTFChars( cacheDir, cacheDirCStr );

    jni->DeleteLocalRef( cacheDir );
    jni->DeleteLocalRef( classDef );

    if( !retVal.empty() && retVal.back() != '/' )
        retVal += "/";

    app->activity->vm->DetachCurrentThread();

    return retVal;
}

namespace Demo
{
    struct AndroidAppController
    {
        GameState *mGraphicsGameState;
        GraphicsSystem *mGraphicsSystem;
        GameState *mLogicGameState;
        LogicSystem *mLogicSystem;

        Ogre::Timer mTimer;
        Ogre::uint64 mStartTime;
        double mAccumulator;

        bool mCriticalFailure;

        AndroidAppController() :
            mGraphicsGameState( 0 ),
            mGraphicsSystem( 0 ),
            mLogicGameState( 0 ),
            mLogicSystem( 0 ),
            mStartTime( 0 ),
            mAccumulator( 0 ),
            mCriticalFailure( false )
        {
        }

        void init()
        {
            if( mCriticalFailure )
                return;

            if( !mGraphicsSystem )
            {
                MainEntryPoints::createSystems( &mGraphicsGameState, &mGraphicsSystem, &mLogicGameState,
                                                &mLogicSystem );
                try
                {
                    mGraphicsSystem->initialize( MainEntryPoints::getWindowTitle() );
                }
                catch( Ogre::RenderingAPIException &e )
                {
                    ANativeActivity *nativeActivity = Demo::AndroidSystems::getAndroidApp()->activity;

                    JNIEnv *jni = 0;
                    nativeActivity->vm->AttachCurrentThread( &jni, nullptr );

                    jclass classDef = jni->GetObjectClass( nativeActivity->clazz );
                    jstring errorMsgJStr = jni->NewStringUTF( e.getDescription().c_str() );
                    jstring detailedErrorMsgJStr = jni->NewStringUTF( e.getFullDescription().c_str() );
                    jmethodID showNoVulkanAlertMethod = jni->GetMethodID(
                        classDef, "showNoVulkanAlert", "(Ljava/lang/String;Ljava/lang/String;)V" );
                    jni->CallVoidMethod( nativeActivity->clazz, showNoVulkanAlertMethod, errorMsgJStr,
                                         detailedErrorMsgJStr );
                    jni->DeleteLocalRef( errorMsgJStr );
                    jni->DeleteLocalRef( detailedErrorMsgJStr );
                    jni->DeleteLocalRef( classDef );

                    // Let them leak. We can't do a clean shutdown. If we're here then there's
                    // nothing we can do. Let's just avoid crashing.
                    mLogicSystem = 0;
                    mGraphicsSystem = 0;

                    mCriticalFailure = true;

                    Demo::AndroidSystems::setNativeWindow( 0 );
                    nativeActivity->vm->DetachCurrentThread();
                    return;
                }

                if( mLogicSystem )
                    mLogicSystem->initialize();

                {
                    // Disable Swappy if a previous run determined it was buggy.
                    struct stat buffer;
                    if( stat( ( Android_getCacheFolder() + "SwappyDisabled" ).c_str(), &buffer ) == 0 )
                    {
                        Ogre::LogManager::getSingleton().logMessage(
                            "Disabling Swappy Frame Pacing because previous runs determined it's buggy "
                            "on this "
                            "device.",
                            Ogre::LML_CRITICAL );

                        OGRE_ASSERT_HIGH( dynamic_cast<Ogre::VulkanRenderSystem *>(
                            mGraphicsSystem->getSceneManager()->getDestinationRenderSystem() ) );
                        Ogre::VulkanRenderSystem *renderSystem = static_cast<Ogre::VulkanRenderSystem *>(
                            mGraphicsSystem->getSceneManager()->getDestinationRenderSystem() );
                        renderSystem->setSwappyFramePacing( false );
                    }
                }

                mGraphicsSystem->createScene01();
                if( mLogicSystem )
                    mLogicSystem->createScene01();

                mGraphicsSystem->createScene02();
                if( mLogicSystem )
                    mLogicSystem->createScene02();

                mStartTime = mTimer.getMicroseconds();
                mAccumulator = MainEntryPoints::Frametime;
            }
            else
            {
                Ogre::Window *windowBase = mGraphicsSystem->getRenderWindow();
                OGRE_ASSERT_HIGH( dynamic_cast<Ogre::VulkanAndroidWindow *>( windowBase ) );
                Ogre::VulkanAndroidWindow *window =
                    static_cast<Ogre::VulkanAndroidWindow *>( windowBase );
                window->setNativeWindow( Demo::AndroidSystems::getNativeWindow() );
            }
        }

        void destroy()
        {
            Demo::AndroidSystems::setNativeWindow( 0 );
            if( mGraphicsSystem )
            {
                Ogre::Window *windowBase = mGraphicsSystem->getRenderWindow();
                OGRE_ASSERT_HIGH( dynamic_cast<Ogre::VulkanAndroidWindow *>( windowBase ) );
                Ogre::VulkanAndroidWindow *window =
                    static_cast<Ogre::VulkanAndroidWindow *>( windowBase );
                window->setNativeWindow( 0 );

                // Cache the fact that Swappy is buggy on this device, using an empty file.
                OGRE_ASSERT_HIGH( dynamic_cast<Ogre::VulkanRenderSystem *>(
                    mGraphicsSystem->getSceneManager()->getDestinationRenderSystem() ) );
                Ogre::VulkanRenderSystem *renderSystem = static_cast<Ogre::VulkanRenderSystem *>(
                    mGraphicsSystem->getSceneManager()->getDestinationRenderSystem() );
                if( !renderSystem->getSwappyFramePacing() )
                {
                    std::ofstream outFile( Android_getCacheFolder() + "SwappyDisabled",
                                           std::ios::out | std::ios::binary );
                }
            }
        }

        void updateMainLoop()
        {
            const Ogre::uint64 endTime = mTimer.getMicroseconds();
            double timeSinceLast = ( endTime - mStartTime ) / 1000000.0;
            timeSinceLast = std::min( 1.0, timeSinceLast );  // Prevent from going haywire.
            mStartTime = endTime;

            while( mAccumulator >= MainEntryPoints::Frametime && mLogicSystem )
            {
                mLogicSystem->beginFrameParallel();
                mLogicSystem->update( static_cast<float>( MainEntryPoints::Frametime ) );
                mLogicSystem->finishFrameParallel();

                mLogicSystem->finishFrame();
                mGraphicsSystem->finishFrame();

                mAccumulator -= MainEntryPoints::Frametime;
            }

            mGraphicsSystem->beginFrameParallel();
            mGraphicsSystem->update( static_cast<float>( timeSinceLast ) );
            mGraphicsSystem->finishFrameParallel();
            if( !mLogicSystem )
                mGraphicsSystem->finishFrame();

            mAccumulator += timeSinceLast;
        }
    };

    class DemoJniProvider final : public Ogre::AndroidJniProvider
    {
        void acquire( JNIEnv **env, jobject *activity ) override
        {
            android_app *app = AndroidSystems::getAndroidApp();
            app->activity->vm->AttachCurrentThread( env, nullptr );
            *activity = app->activity->clazz;
        }

        void release( JNIEnv * ) override
        {
            android_app *app = AndroidSystems::getAndroidApp();
            app->activity->vm->DetachCurrentThread();
        }
    };
}  // namespace Demo

static Demo::AndroidAppController g_appController;
static Demo::DemoJniProvider g_demoJniProvider;

// Process the next main command.
void handle_cmd( android_app *app, int32_t cmd )
{
    switch( cmd )
    {
    case APP_CMD_INIT_WINDOW:
        // The window is being shown, get it ready.
        Demo::AndroidSystems::setAndroidApp( app );
        Demo::AndroidSystems::setNativeWindow( app->window );
        g_appController.init();
        break;
    case APP_CMD_TERM_WINDOW:
        __android_log_print( ANDROID_LOG_INFO, "OgreSamples", "g_appController.destroy(): %d", cmd );
        g_appController.destroy();
        break;
    case APP_CMD_CONTENT_RECT_CHANGED:
    case APP_CMD_WINDOW_RESIZED:
        __android_log_print( ANDROID_LOG_INFO, "OgreSamples", "windowMovedOrResized: %d", cmd );
        // We got crash reports from getting _RESIZED events w/ mGraphicsSystem being a nullptr.
        // That's Android for you.
        if( g_appController.mGraphicsSystem && g_appController.mGraphicsSystem->getRenderWindow() )
            g_appController.mGraphicsSystem->getRenderWindow()->windowMovedOrResized();
        break;
    default:
        __android_log_print( ANDROID_LOG_INFO, "OgreSamples", "event not handled: %d", cmd );
    }
}

void android_main( struct android_app *app )
{
    // Set the callback to process system events
    app->onAppCmd = handle_cmd;

    Demo::AndroidSystems::setAndroidApp( app );
    Demo::AndroidSystems::setJniProvider( &g_demoJniProvider );

    // !!! IMPORTANT !!!
    //
    // Swappy defaults to AutoVSyncInterval_AutoPipeline.
    // But we change it to PipelineForcedOn because it's the behavior most users coming from PC
    // expect (AutoVSyncInterval_AutoPipeline can be counter-intuitive because it can lock the
    // pacing to lower framerates)
    //
    // What actual setting you wish to use (or expose to user) depends on how much love,
    // testing & optimization you put into your Android app.
    //
    // !!! IMPORTANT !!!
    Ogre::VulkanAndroidWindow::setFramePacingSwappyAutoMode(
        Ogre::VulkanAndroidWindow::PipelineForcedOn );

    // Used to poll the events in the main loop
    int events;
    android_poll_source *source;

    // Main loop
    do
    {
        int ident = ALooper_pollOnce( Demo::AndroidSystems::getNativeWindow() ? 0 : -1, nullptr, &events,
                                      (void **)&source );
        while( ident >= 0 )
        {
            if( source != NULL )
                source->process( app, source );
            ident = ALooper_pollOnce( 0, nullptr, &events, (void **)&source );
        }

        // render if vulkan is ready
        if( Demo::AndroidSystems::getNativeWindow() )
            g_appController.updateMainLoop();
    } while( app->destroyRequested == 0 );
}
