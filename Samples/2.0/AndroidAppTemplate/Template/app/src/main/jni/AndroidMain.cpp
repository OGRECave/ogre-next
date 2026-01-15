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

#include "AndroidVulkanTools.h"

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
#include "Threading/OgreCondVariable.h"

#include "OgreVulkanPlugin.h"
#include "Windowing/Android/OgreVulkanAndroidWindow.h"

#include "Threading/OgreWaitableEvent.h"

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

        // Custom Looper Data begins.

        bool destroyRequested;
        bool mNativeWindowChangeRequested;  // GUARDED_BY( mWindowMutex )

        Ogre::LightweightMutex mCmdMutex;
        Ogre::CondVariable mWindowMutex;
        Ogre::WaitableEvent mWaitableEvent;

        std::vector<std::pair<android_app *, int32_t>> cmds[2];  // GUARDED_BY( mCmdMutex )
        ANativeWindow *mRequestedNativeWindow;                   // GUARDED_BY( mWindowMutex )

        // Custom Looper Data ends.

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
            {
                Demo::AndroidSystems::setNativeWindow( 0 );
                return;
            }

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

        void windowDestroyed()
        {
            if( mGraphicsSystem )
            {
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

        Ogre::VulkanAndroidWindow *getVulkanWindow()
        {
            if( mGraphicsSystem )
            {
                // Due to race conditions, windowBase can be nullptr even if mGraphicsSystem
                // isn't (because one thread is still in mGraphicsSystem->initialize()
                // while another thread is calling getVulkanWindow().
                Ogre::Window *windowBase = mGraphicsSystem->getRenderWindow();
                if( windowBase )
                {
                    OGRE_ASSERT_HIGH( dynamic_cast<Ogre::VulkanAndroidWindow *>( windowBase ) );
                    return static_cast<Ogre::VulkanAndroidWindow *>( windowBase );
                }
            }
            return nullptr;
        }

        // Must return true while we're blocking. Returns false if we can continue.
        static bool blockWhileInitializingWindow( void *userData )
        {
            const AndroidAppController *app = reinterpret_cast<const AndroidAppController *>( userData );
            return app->mNativeWindowChangeRequested;
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

// Forward the next main command.
void handle_cmd( android_app *app, int32_t cmd )
{
    {
        Ogre::ScopedLock lock( g_appController.mCmdMutex );
        g_appController.cmds[0].push_back( { app, cmd } );
    }

    bool bFullSync = false;

    switch( cmd )
    {
    case APP_CMD_INIT_WINDOW:
    case APP_CMD_TERM_WINDOW:
    {
        Ogre::ScopedCvLock lock( g_appController.mWindowMutex );

        ANativeWindow *nextWindow = 0;

        if( cmd == APP_CMD_INIT_WINDOW )
        {
            __android_log_print( ANDROID_LOG_INFO, "OgreSamples",
                                 "Requesting / Delaying NativeWindow creation" );
            OGRE_ASSERT_MEDIUM( app->window );
            nextWindow = app->window;
        }
        else
        {
            __android_log_print( ANDROID_LOG_INFO, "OgreSamples",
                                 "Requesting / Delaying NativeWindow destruction" );
            if( Ogre::Workarounds::mVkSyncWindowInit )
                bFullSync = true;
        }

        if( g_appController.mRequestedNativeWindow != nextWindow )
        {
            // mRequestedNativeWindow can be non-null if two requests queued up in a row.
            if( g_appController.mRequestedNativeWindow )
            {
                ANativeWindow_release( g_appController.mRequestedNativeWindow );
                __android_log_print(
                    ANDROID_LOG_VERBOSE, "OgreSamples", "[ANativeWindow][RELEASE][looper thread] %#llx",
                    (unsigned long long)(void *)g_appController.mRequestedNativeWindow );
            }
            if( nextWindow )
            {
                ANativeWindow_acquire( nextWindow );
                __android_log_print( ANDROID_LOG_VERBOSE, "OgreSamples",
                                     "[ANativeWindow][ACQUIRE][looper thread] %#llx",
                                     (unsigned long long)(void *)nextWindow );
            }
        }
        g_appController.mRequestedNativeWindow = nextWindow;
        g_appController.mNativeWindowChangeRequested = true;

        Ogre::VulkanAndroidWindow *window = g_appController.getVulkanWindow();
        if( window )
            window->requestNativeWindowChange();
        break;
    }
    }

    if( bFullSync )
    {
        g_appController.mWaitableEvent.wake();
        g_appController.mWindowMutex.wait( Demo::AndroidAppController::blockWhileInitializingWindow,
                                           &g_appController );
    }
}

static unsigned long main_graphics_thread( Ogre::ThreadHandle *threadHandle )
{
    while( !g_appController.destroyRequested )
    {
        {
            Ogre::ScopedLock lock( g_appController.mCmdMutex );
            g_appController.cmds[0].swap( g_appController.cmds[1] );
        }
        {
            Ogre::ScopedCvLock lock( g_appController.mWindowMutex );
            if( g_appController.mNativeWindowChangeRequested )
            {
                Ogre::Threads::Sleep( 2000 );
            }
        }

        bool notifyFullSyncThreads = false;
        {
            // This is not best practice because we change the logic out of order. If we receive:
            //  APP_CMD_INIT_WINDOW
            //  APP_CMD_WINDOW_RESIZED
            //  APP_CMD_TERM_WINDOW
            //
            // We'll end up processing them as:
            //  APP_CMD_TERM_WINDOW
            //  APP_CMD_WINDOW_RESIZED
            //
            // (yes, INIT_WINDOW got eaten by TERM_WINDOW). However we are forced to do this
            // because actual good practice would use need an expensive APP_CMD_INIT_WINDOW for
            // initialization and stall in APP_CMD_TERM_WINDOW to guarantee the GPU is no longer in use.
            // But we can't do that, because according to Google "good practice" is not having ANRs
            // and making sure command messages are processed ASAP.
            Ogre::ScopedCvLock lock( g_appController.mWindowMutex );
            if( g_appController.mNativeWindowChangeRequested )
            {
                Demo::AndroidSystems::setNativeWindow( g_appController.mRequestedNativeWindow );

                if( !g_appController.mGraphicsSystem )
                {
                    // Do NOT do anything here (except workaround), because the mutex would
                    // block the UI thread, causing ANRs.

                    // We will call g_appController.init() init later, outside the mutex.
                    // For the time being, g_appController.mRequestedNativeWindow will be set to nullptr
                    // but NOT released (we'll release it later via AndroidSystems::getNativeWindow)
#ifdef OGRE_VK_WORKAROUND_SYNC_WINDOW_INIT
                    if( g_appController.mRequestedNativeWindow && Ogre::Workarounds::mVkSyncWindowInit )
                    {
                        // We're forced to do it synchronously, due to driver bugs.
                        __android_log_print( ANDROID_LOG_INFO, "OgreSamples",
                                             "Creating GraphicsSystem" );
                        g_appController.init();
                        ANativeWindow_release( g_appController.mRequestedNativeWindow );
                        __android_log_print(
                            ANDROID_LOG_VERBOSE, "OgreSamples",
                            "[ANativeWindow][RELEASE][main thread1] %#llx",
                            (unsigned long long)(void *)g_appController.mRequestedNativeWindow );
                    }
#endif
                }
                else
                {
                    if( g_appController.mRequestedNativeWindow )
                    {
                        __android_log_print( ANDROID_LOG_INFO, "OgreSamples",
                                             "Flushing NativeWindow creation" );
                    }
                    else
                    {
                        __android_log_print( ANDROID_LOG_INFO, "OgreSamples",
                                             "Flushing NativeWindow destruction" );
                    }

                    Ogre::VulkanAndroidWindow *window = g_appController.getVulkanWindow();
                    window->flushQueuedNativeWindowChanges( g_appController.mRequestedNativeWindow );
                    if( g_appController.mRequestedNativeWindow )
                    {
                        // flushQueuedNativeWindowChanges() just increased reference count, and we
                        // already did that in handle_cmd(), so decrease it to avoid leaking it later.
                        ANativeWindow_release( g_appController.mRequestedNativeWindow );
                        __android_log_print(
                            ANDROID_LOG_VERBOSE, "OgreSamples",
                            "[ANativeWindow][RELEASE][main thread0] %#llx",
                            (unsigned long long)(void *)g_appController.mRequestedNativeWindow );
                    }
                    else
                    {
                        g_appController.windowDestroyed();
                    }
                }
                g_appController.mRequestedNativeWindow = 0;
                g_appController.mNativeWindowChangeRequested = false;
                notifyFullSyncThreads = Ogre::Workarounds::mVkSyncWindowInit;
            }
        }

        if( notifyFullSyncThreads )
            g_appController.mWindowMutex.notifyAll();

        if( !g_appController.mGraphicsSystem
#ifdef OGRE_VK_WORKAROUND_SYNC_WINDOW_INIT
            && !Ogre::Workarounds::mVkSyncWindowInit
#endif
        )
        {
            // We delayed initialization to be done after releasing the mutex.
            ANativeWindow *nativeWindow = Demo::AndroidSystems::getNativeWindow();
            if( nativeWindow )
            {
                __android_log_print( ANDROID_LOG_INFO, "OgreSamples", "Creating GraphicsSystem" );
                g_appController.init();
                ANativeWindow_release( nativeWindow );
                __android_log_print( ANDROID_LOG_VERBOSE, "OgreSamples",
                                     "[ANativeWindow][RELEASE][main thread1] %#llx",
                                     (unsigned long long)(void *)nativeWindow );

                {
                    // It is possible handle_cmd() in another thread got a request but saw
                    // g_appController.getVulkanWindow() return nullptr because we were still
                    // initializing. If that happened, tell the window it is dirty.
                    Ogre::ScopedCvLock lock( g_appController.mWindowMutex );
                    if( g_appController.mNativeWindowChangeRequested )
                    {
                        Ogre::VulkanAndroidWindow *window = g_appController.getVulkanWindow();
                        if( window )
                            window->requestNativeWindowChange();
                    }
                }
            }
        }

        // Process all the cmds forwarded to this thread.
        for( const std::pair<android_app *, int32_t> &pair : g_appController.cmds[1] )
        {
            const int32_t cmd = pair.second;

            switch( cmd )
            {
            case APP_CMD_INIT_WINDOW:
            case APP_CMD_TERM_WINDOW:
                break;
            case APP_CMD_CONTENT_RECT_CHANGED:
            case APP_CMD_WINDOW_RESIZED:
                __android_log_print( ANDROID_LOG_INFO, "OgreSamples", "windowMovedOrResized: %d", cmd );
                // We got crash reports from getting _RESIZED events w/ mGraphicsSystem being a nullptr.
                // That's Android for you.
                if( g_appController.mGraphicsSystem &&
                    g_appController.mGraphicsSystem->getRenderWindow() )
                {
                    g_appController.mGraphicsSystem->getRenderWindow()->windowMovedOrResized();
                }
                break;
            default:
                __android_log_print( ANDROID_LOG_INFO, "OgreSamples", "event not handled: %d", cmd );
            }
        }
        g_appController.cmds[1].clear();

        if( Demo::AndroidSystems::getNativeWindow() )
            g_appController.updateMainLoop();
        else
            g_appController.mWaitableEvent.wait();
    }

    return 0;
}

THREAD_DECLARE( main_graphics_thread );

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

    testEarlyVulkanWorkarounds();

    Ogre::ThreadHandlePtr threadHandles[1];
    threadHandles[0] = Ogre::Threads::CreateThread( THREAD_GET( main_graphics_thread ), 0, nullptr );

    // Main loop
    do
    {
        // Used to poll the events in the main loop
        int events;
        android_poll_source *source;

        int ident = ALooper_pollOnce( -1, nullptr, &events, (void **)&source );
        while( ident >= 0 )
        {
            if( source != NULL )
                source->process( app, source );
            ident = ALooper_pollOnce( 0, nullptr, &events, (void **)&source );
        }

        g_appController.mWaitableEvent.wake();
    } while( app->destroyRequested == 0 );

    g_appController.destroyRequested = true;
    g_appController.mWaitableEvent.wake();

    Ogre::Threads::WaitForThreads( 1, threadHandles );
}
