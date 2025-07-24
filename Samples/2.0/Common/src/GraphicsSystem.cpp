
#include "GraphicsSystem.h"
#include "GameState.h"
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
#    include "SdlInputHandler.h"
#endif
#include "GameEntity.h"

#include "OgreAbiUtils.h"
#include "OgreConfigFile.h"
#include "OgreException.h"
#include "OgreRoot.h"

#include "OgreCamera.h"
#include "OgreItem.h"

#include "OgreArchiveManager.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsUnlit.h"

#include "Compositor/OgreCompositorManager2.h"

#include "OgreOverlayManager.h"
#include "OgreOverlaySystem.h"

#include "OgreTextureGpuManager.h"

#include "OgreWindow.h"
#include "OgreWindowEventUtilities.h"

#include "OgreFileSystemLayer.h"

#include "OgreGpuProgramManager.h"
#include "OgreHlmsDiskCache.h"

#include "OgreLogManager.h"

#include "OgrePlatformInformation.h"

#include "System/Android/AndroidSystems.h"

#include "OgreAtmosphereComponent.h"
#ifdef OGRE_BUILD_COMPONENT_ATMOSPHERE
#    include "OgreAtmosphereNpr.h"
#endif

#include <fstream>

#if OGRE_USE_SDL2
#    include <SDL_syswm.h>
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
#    include "OSX/macUtils.h"
#    if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
#        include "System/iOS/iOSUtils.h"
#    else
#        include "System/OSX/OSXUtils.h"
#    endif
#endif

namespace Demo
{
    GraphicsSystem::GraphicsSystem( GameState *gameState, Ogre::String resourcePath,
                                    Ogre::ColourValue backgroundColour ) :
        BaseSystem( gameState ),
        mLogicSystem( 0 ),
#if OGRE_USE_SDL2
        mSdlWindow( 0 ),
        mInputHandler( 0 ),
#endif
        mRoot( 0 ),
        mRenderWindow( 0 ),
        mSceneManager( 0 ),
        mCamera( 0 ),
        mWorkspace( 0 ),
        mPluginsFolder( "./" ),
        mResourcePath( resourcePath ),
        mOverlaySystem( 0 ),
        mAccumTimeSinceLastLogicFrame( 0 ),
        mCurrentTransformIdx( 0 ),
        mThreadGameEntityToUpdate( 0 ),
        mThreadWeight( 0 ),
        mQuit( false ),
        mAlwaysAskForConfig( true ),
        mUseHlmsDiskCache( true ),
        mUseMicrocodeCache( true ),
        mBackgroundColour( backgroundColour )
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        // Note:  macBundlePath works for iOS too. It's misnamed.
        mResourcePath = Ogre::macBundlePath() + "/Contents/Resources/";
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        mResourcePath = Ogre::macBundlePath() + "/";
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        mPluginsFolder = mResourcePath;
#endif
        if( isWriteAccessFolder( mPluginsFolder, "Ogre.log" ) )
            mWriteAccessFolder = mPluginsFolder;
        else
        {
            Ogre::FileSystemLayer filesystemLayer( OGRE_VERSION_NAME );
            mWriteAccessFolder = filesystemLayer.getWritablePath( "" );
        }
    }
    //-----------------------------------------------------------------------------------
    GraphicsSystem::~GraphicsSystem()
    {
        if( mRoot )
        {
            Ogre::LogManager::getSingleton().logMessage(
                "WARNING: GraphicsSystem::deinitialize() not called!!!", Ogre::LML_CRITICAL );
        }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::setRequiePersistentDepthBuf( bool require )
    {
        mRequirePersistentDepthBuf = require;
    }
    //-----------------------------------------------------------------------------------
    bool GraphicsSystem::isWriteAccessFolder( const Ogre::String &folderPath,
                                              const Ogre::String &fileToSave )
    {
        if( !Ogre::FileSystemLayer::createDirectory( folderPath ) )
            return false;

        std::ofstream of( ( folderPath + fileToSave ).c_str(),
                          std::ios::out | std::ios::binary | std::ios::app );
        if( !of )
            return false;

        return true;
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::initialize( const Ogre::String &windowTitle )
    {
#if OGRE_USE_SDL2
        // if( SDL_Init( SDL_INIT_EVERYTHING ) != 0 )
        if( SDL_Init( SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER |
                      SDL_INIT_EVENTS ) != 0 )
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_INTERNAL_ERROR, "Cannot initialize SDL2!",
                         "GraphicsSystem::initialize" );
        }
#endif

        Ogre::String pluginsPath;
        // only use plugins.cfg if not static
#ifndef OGRE_STATIC_LIB
#    if OGRE_DEBUG_MODE && \
        !( ( OGRE_PLATFORM == OGRE_PLATFORM_APPLE ) || ( OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS ) )
        pluginsPath = mPluginsFolder + "plugins_d.cfg";
#    else
        pluginsPath = mPluginsFolder + "plugins.cfg";
#    endif
#endif

#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
        const Ogre::String cfgPath = mWriteAccessFolder + "ogre.cfg";
#else
        const Ogre::String cfgPath = "";
#endif

        {
            const Ogre::AbiCookie abiCookie = Ogre::generateAbiCookie();
            mRoot = OGRE_NEW Ogre::Root( &abiCookie, pluginsPath, cfgPath,
                                         mWriteAccessFolder + "Ogre.log", windowTitle );
        }

        AndroidSystems::registerArchiveFactories();

        mStaticPluginLoader.install( mRoot );

        // enable sRGB Gamma Conversion mode by default for all renderers,
        // but still allow to override it via config dialog
        Ogre::RenderSystemList::const_iterator itor = mRoot->getAvailableRenderers().begin();
        Ogre::RenderSystemList::const_iterator endt = mRoot->getAvailableRenderers().end();

        while( itor != endt )
        {
            Ogre::RenderSystem *rs = *itor;
            rs->setConfigOption( "sRGB Gamma Conversion", "Yes" );
            ++itor;
        }

        if( mAlwaysAskForConfig || !mRoot->restoreConfig() )
        {
            if( !mRoot->showConfigDialog() )
            {
                mQuit = true;
                return;
            }
        }

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        if( !mRoot->getRenderSystem() )
        {
            Ogre::RenderSystem *renderSystem =
                mRoot->getRenderSystemByName( "Metal Rendering Subsystem" );
            mRoot->setRenderSystem( renderSystem );
        }
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
        if( !mRoot->getRenderSystem() )
        {
            Ogre::RenderSystem *renderSystem =
                mRoot->getRenderSystemByName( "Vulkan Rendering Subsystem" );
            mRoot->setRenderSystem( renderSystem );
        }
#endif

        mRoot->initialise( false, windowTitle );

        Ogre::ConfigOptionMap &cfgOpts = mRoot->getRenderSystem()->getConfigOptions();

        int width = 1280;
        int height = 720;

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        {
            Ogre::Vector2 screenRes = iOSUtils::getScreenResolutionInPoints();
            width = static_cast<int>( screenRes.x );
            height = static_cast<int>( screenRes.y );
        }
#endif

        Ogre::ConfigOptionMap::iterator opt = cfgOpts.find( "Video Mode" );
        if( opt != cfgOpts.end() && !opt->second.currentValue.empty() )
        {
            // Ignore leading space
            const Ogre::String::size_type start = opt->second.currentValue.find_first_of( "012356789" );
            // Get the width and height
            Ogre::String::size_type widthEnd = opt->second.currentValue.find( ' ', start );
            // we know that the height starts 3 characters after the width and goes until the next space
            Ogre::String::size_type heightEnd = opt->second.currentValue.find( ' ', widthEnd + 3 );
            // Now we can parse out the values
            width = Ogre::StringConverter::parseInt( opt->second.currentValue.substr( 0, widthEnd ) );
            height = Ogre::StringConverter::parseInt(
                opt->second.currentValue.substr( widthEnd + 3, heightEnd ) );
        }

        Ogre::NameValuePairList params;
        bool fullscreen = Ogre::StringConverter::parseBool( cfgOpts["Full Screen"].currentValue );
#if OGRE_USE_SDL2
        unsigned int screen = 0;
        unsigned int posX = SDL_WINDOWPOS_CENTERED_DISPLAY( screen );
        unsigned int posY = SDL_WINDOWPOS_CENTERED_DISPLAY( screen );

        if( fullscreen )
        {
            posX = SDL_WINDOWPOS_UNDEFINED_DISPLAY( screen );
            posY = SDL_WINDOWPOS_UNDEFINED_DISPLAY( screen );
        }

        mSdlWindow = SDL_CreateWindow(
            windowTitle.c_str(),       // window title
            static_cast<int>( posX ),  // initial x position
            static_cast<int>( posY ),  // initial y position
            width,                     // width, in pixels
            height,                    // height, in pixels
            SDL_WINDOW_SHOWN | ( fullscreen ? SDL_WINDOW_FULLSCREEN : 0 ) | SDL_WINDOW_RESIZABLE );

        // Get the native whnd
        SDL_SysWMinfo wmInfo;
        SDL_VERSION( &wmInfo.version );

        if( SDL_GetWindowWMInfo( mSdlWindow, &wmInfo ) == SDL_FALSE )
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_INTERNAL_ERROR, "Couldn't get WM Info! (SDL2)",
                         "GraphicsSystem::initialize" );
        }

        Ogre::String winHandle;
        switch( wmInfo.subsystem )
        {
#    if defined( SDL_VIDEO_DRIVER_WINDOWS )
        case SDL_SYSWM_WINDOWS:
            // Windows code
            winHandle = Ogre::StringConverter::toString( (uintptr_t)wmInfo.info.win.window );
            break;
#    endif
#    if defined( SDL_VIDEO_DRIVER_WINRT )
        case SDL_SYSWM_WINRT:
            // Windows code
            winHandle = Ogre::StringConverter::toString( (uintptr_t)wmInfo.info.winrt.window );
            break;
#    endif
#    if defined( SDL_VIDEO_DRIVER_COCOA )
        case SDL_SYSWM_COCOA:
            winHandle = Ogre::StringConverter::toString( WindowContentViewHandle( wmInfo ) );
            break;
#    endif
#    if defined( SDL_VIDEO_DRIVER_X11 )
        case SDL_SYSWM_X11:
            winHandle = Ogre::StringConverter::toString( (uintptr_t)wmInfo.info.x11.window );
            params.insert( std::make_pair(
                "SDL2x11", Ogre::StringConverter::toString( (uintptr_t)&wmInfo.info.x11 ) ) );
            break;
#    endif
        default:
            OGRE_EXCEPT( Ogre::Exception::ERR_NOT_IMPLEMENTED, "Unexpected WM! (SDL2)",
                         "GraphicsSystem::initialize" );
            break;
        }

#    if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
        params.insert( std::make_pair( "externalWindowHandle", winHandle ) );
#    else
        params.insert( std::make_pair( "parentWindowHandle", winHandle ) );
#    endif
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
        params.insert( std::make_pair(
            "ANativeWindow",
            Ogre::StringConverter::toString( (uintptr_t)AndroidSystems::getNativeWindow() ) ) );
        params.insert( std::make_pair(
            "AndroidJniProvider",
            Ogre::StringConverter::toString( (uintptr_t)AndroidSystems::getJniProvider() ) ) );
#endif

        params.insert( std::make_pair( "title", windowTitle ) );
        params.insert( std::make_pair( "gamma", cfgOpts["sRGB Gamma Conversion"].currentValue ) );
        if( cfgOpts.find( "VSync Method" ) != cfgOpts.end() )
            params.insert( std::make_pair( "vsync_method", cfgOpts["VSync Method"].currentValue ) );
        params.insert( std::make_pair( "FSAA", cfgOpts["FSAA"].currentValue ) );
        params.insert( std::make_pair( "vsync", cfgOpts["VSync"].currentValue ) );
        params.insert( std::make_pair( "reverse_depth", "Yes" ) );

        if( mRequirePersistentDepthBuf )
        {
            params.insert( std::make_pair( "memoryless_depth_buffer", "No" ) );
        }
        else
        {
            params.insert( std::make_pair( "memoryless_depth_buffer", "Yes" ) );
        }

        initMiscParamsListener( params );

        mRenderWindow = Ogre::Root::getSingleton().createRenderWindow(
            windowTitle,                                                      //
            static_cast<uint32_t>( width ), static_cast<uint32_t>( height ),  //
            fullscreen, &params );

        mOverlaySystem = OGRE_NEW Ogre::v1::OverlaySystem();

        setupResources();
        loadResources();
        chooseSceneManager();
        createCamera();
        mWorkspace = setupCompositor();

#if OGRE_USE_SDL2
        mInputHandler =
            new SdlInputHandler( mSdlWindow, mCurrentGameState, mCurrentGameState, mCurrentGameState );
#endif

        BaseSystem::initialize();

#if OGRE_PROFILING
        Ogre::Profiler::getSingleton().setEnabled( true );
#    if OGRE_PROFILING == OGRE_PROFILING_INTERNAL
        Ogre::Profiler::getSingleton().endProfile( "" );
#    endif
#    if OGRE_PROFILING == OGRE_PROFILING_INTERNAL_OFFLINE
        Ogre::Profiler::getSingleton().getOfflineProfiler().setDumpPathsOnShutdown(
            mWriteAccessFolder + "ProfilePerFrame", mWriteAccessFolder + "ProfileAccum" );
#    endif
#endif
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::deinitialize()
    {
        BaseSystem::deinitialize();

        saveTextureCache();
        saveHlmsDiskCache();

        if( mSceneManager )
        {
            Ogre::AtmosphereComponent *atmosphere = mSceneManager->getAtmosphereRaw();
            OGRE_DELETE atmosphere;

            mSceneManager->removeRenderQueueListener( mOverlaySystem );
        }

        OGRE_DELETE mOverlaySystem;
        mOverlaySystem = 0;

#if OGRE_USE_SDL2
        delete mInputHandler;
        mInputHandler = 0;
#endif

        OGRE_DELETE mRoot;
        mRoot = 0;

#if OGRE_USE_SDL2
        if( mSdlWindow )
        {
            // Restore desktop resolution on exit
            SDL_SetWindowFullscreen( mSdlWindow, 0 );
            SDL_DestroyWindow( mSdlWindow );
            mSdlWindow = 0;
        }

        SDL_Quit();
#endif
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::update( float timeSinceLast )
    {
        Ogre::WindowEventUtilities::messagePump();

#if OGRE_USE_SDL2
        SDL_Event evt;
        while( SDL_PollEvent( &evt ) )
        {
            switch( evt.type )
            {
            case SDL_WINDOWEVENT:
                handleWindowEvent( evt );
                break;
            case SDL_QUIT:
                mQuit = true;
                break;
            default:
                break;
            }

            mInputHandler->_handleSdlEvents( evt );
        }
#endif

        BaseSystem::update( timeSinceLast );

        if( mRenderWindow->isVisible() )
            mQuit |= !mRoot->renderOneFrame();

        mAccumTimeSinceLastLogicFrame += timeSinceLast;

        // SDL_SetWindowPosition( mSdlWindow, 0, 0 );
        /*SDL_Rect rect;
        SDL_GetDisplayBounds( 0, &rect );
        SDL_GetDisplayBounds( 0, &rect );*/
    }
//-----------------------------------------------------------------------------------
#if OGRE_USE_SDL2
    void GraphicsSystem::handleWindowEvent( const SDL_Event &evt )
    {
        switch( evt.window.event )
        {
        /*case SDL_WINDOWEVENT_MAXIMIZED:
            SDL_SetWindowBordered( mSdlWindow, SDL_FALSE );
            break;
        case SDL_WINDOWEVENT_MINIMIZED:
        case SDL_WINDOWEVENT_RESTORED:
            SDL_SetWindowBordered( mSdlWindow, SDL_TRUE );
            break;*/
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            int w, h;
            SDL_GetWindowSize( mSdlWindow, &w, &h );
#    if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
            mRenderWindow->requestResolution( static_cast<uint32_t>( w ), static_cast<uint32_t>( h ) );
#    endif
            mRenderWindow->windowMovedOrResized();
            break;
        case SDL_WINDOWEVENT_RESIZED:
#    if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
            mRenderWindow->requestResolution( static_cast<uint32_t>( evt.window.data1 ),
                                              static_cast<uint32_t>( evt.window.data2 ) );
#    endif
            mRenderWindow->windowMovedOrResized();
            break;
        case SDL_WINDOWEVENT_CLOSE:
            break;
        case SDL_WINDOWEVENT_SHOWN:
            mRenderWindow->_setVisible( true );
            break;
        case SDL_WINDOWEVENT_HIDDEN:
            mRenderWindow->_setVisible( false );
            break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            mRenderWindow->setFocused( true );
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            mRenderWindow->setFocused( false );
            break;
        }
    }
#endif
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::processIncomingMessage( Mq::MessageId messageId, const void *data )
    {
        switch( messageId )
        {
        case Mq::LOGICFRAME_FINISHED:
        {
            Ogre::uint32 newIdx = *reinterpret_cast<const Ogre::uint32 *>( data );

            if( newIdx != std::numeric_limits<Ogre::uint32>::max() )
            {
                mAccumTimeSinceLastLogicFrame = 0;
                // Tell the LogicSystem we're no longer using the index previous to the current one.
                this->queueSendMessage(
                    mLogicSystem, Mq::LOGICFRAME_FINISHED,
                    ( mCurrentTransformIdx + NUM_GAME_ENTITY_BUFFERS - 1 ) % NUM_GAME_ENTITY_BUFFERS );

                assert( ( mCurrentTransformIdx + 1 ) % NUM_GAME_ENTITY_BUFFERS == newIdx &&
                        "Graphics is receiving indices out of order!!!" );

                // Get the new index the LogicSystem is telling us to use.
                mCurrentTransformIdx = newIdx;
            }
        }
        break;
        case Mq::GAME_ENTITY_ADDED:
            gameEntityAdded( reinterpret_cast<const GameEntityManager::CreatedGameEntity *>( data ) );
            break;
        case Mq::GAME_ENTITY_REMOVED:
            gameEntityRemoved( *reinterpret_cast<GameEntity *const *>( data ) );
            break;
        case Mq::GAME_ENTITY_SCHEDULED_FOR_REMOVAL_SLOT:
            // Acknowledge/notify back that we're done with this slot.
            this->queueSendMessage( mLogicSystem, Mq::GAME_ENTITY_SCHEDULED_FOR_REMOVAL_SLOT,
                                    *reinterpret_cast<const Ogre::uint32 *>( data ) );
            break;
        default:
            break;
        }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::addResourceLocation( const Ogre::String &archName, const Ogre::String &typeName,
                                              const Ogre::String &secName )
    {
#if( OGRE_PLATFORM == OGRE_PLATFORM_APPLE ) || ( OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS )
        // OS X does not set the working directory relative to the app,
        // In order to make things portable on OS X we need to provide
        // the loading with it's own bundle path location
        Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
            Ogre::String( Ogre::macBundlePath() + "/" + archName ), typeName, secName );
#else
        Ogre::ResourceGroupManager::getSingleton().addResourceLocation( archName, typeName, secName );
#endif
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::loadTextureCache()
    {
#if !OGRE_NO_JSON
        Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();
        Ogre::Archive *rwAccessFolderArchive =
            archiveManager.load( mWriteAccessFolder, "FileSystem", true );
        try
        {
            const Ogre::String filename = "textureMetadataCache.json";
            if( rwAccessFolderArchive->exists( filename ) )
            {
                Ogre::DataStreamPtr stream = rwAccessFolderArchive->open( filename );
                std::vector<char> fileData;
                fileData.resize( stream->size() + 1 );
                if( !fileData.empty() )
                {
                    stream->read( &fileData[0], stream->size() );
                    // Add null terminator just in case (to prevent bad input)
                    fileData.back() = '\0';
                    Ogre::TextureGpuManager *textureManager =
                        mRoot->getRenderSystem()->getTextureGpuManager();
                    textureManager->importTextureMetadataCache( stream->getName(), &fileData[0], false );
                }
            }
            else
            {
                Ogre::LogManager::getSingleton().logMessage( "[INFO] Texture cache not found at " +
                                                             mWriteAccessFolder +
                                                             "/textureMetadataCache.json" );
            }
        }
        catch( Ogre::Exception &e )
        {
            Ogre::LogManager::getSingleton().logMessage( e.getFullDescription() );
        }

        archiveManager.unload( rwAccessFolderArchive );
#endif
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::saveTextureCache()
    {
        if( mRoot->getRenderSystem() )
        {
            Ogre::TextureGpuManager *textureManager = mRoot->getRenderSystem()->getTextureGpuManager();
            if( textureManager )
            {
                Ogre::String jsonString;
                textureManager->exportTextureMetadataCache( jsonString );
                const Ogre::String path = mWriteAccessFolder + "/textureMetadataCache.json";
                std::ofstream file( path.c_str(), std::ios::binary | std::ios::out );
                if( file.is_open() )
                    file.write( jsonString.c_str(), static_cast<std::streamsize>( jsonString.size() ) );
                file.close();
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::loadHlmsDiskCache()
    {
        if( !mUseMicrocodeCache && !mUseHlmsDiskCache )
            return;

        Ogre::HlmsManager *hlmsManager = mRoot->getHlmsManager();
        Ogre::HlmsDiskCache diskCache( hlmsManager );

        Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();

        Ogre::Archive *rwAccessFolderArchive =
            archiveManager.load( mWriteAccessFolder, "FileSystem", true );

        if( mUseMicrocodeCache /* mUsePipelineCache */ )
        {
            const Ogre::String filename = "pipelineCache.cache";
            if( rwAccessFolderArchive->exists( filename ) )
            {
                Ogre::DataStreamPtr pipelineCacheFile = rwAccessFolderArchive->open( filename );
                mRoot->getRenderSystem()->loadPipelineCache( pipelineCacheFile );
            }
        }

        if( mUseMicrocodeCache )
        {
            // Make sure the microcode cache is enabled.
            Ogre::GpuProgramManager::getSingleton().setSaveMicrocodesToCache( true );
            const Ogre::String filename = "microcodeCodeCache.cache";
            if( rwAccessFolderArchive->exists( filename ) )
            {
                Ogre::DataStreamPtr shaderCacheFile = rwAccessFolderArchive->open( filename );
                Ogre::GpuProgramManager::getSingleton().loadMicrocodeCache( shaderCacheFile );
            }
        }

        if( mUseHlmsDiskCache )
        {
            const size_t numThreads =
                std::max<size_t>( 1u, Ogre::PlatformInformation::getNumLogicalCores() );
            for( size_t i = Ogre::HLMS_LOW_LEVEL + 1u; i < Ogre::HLMS_MAX; ++i )
            {
                Ogre::Hlms *hlms = hlmsManager->getHlms( static_cast<Ogre::HlmsTypes>( i ) );
                if( hlms )
                {
                    Ogre::String filename =
                        "hlmsDiskCache" + Ogre::StringConverter::toString( i ) + ".bin";

                    try
                    {
                        if( rwAccessFolderArchive->exists( filename ) )
                        {
                            Ogre::DataStreamPtr diskCacheFile = rwAccessFolderArchive->open( filename );
                            diskCache.loadFrom( diskCacheFile );
                            diskCache.applyTo( hlms, numThreads );
                        }
                    }
                    catch( Ogre::Exception & )
                    {
                        Ogre::LogManager::getSingleton().logMessage(
                            "Error loading cache from " + mWriteAccessFolder + "/" + filename +
                            "! If you have issues, try deleting the file "
                            "and restarting the app" );
                    }
                }
            }
        }

        archiveManager.unload( mWriteAccessFolder );
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::saveHlmsDiskCache()
    {
        if( mRoot->getRenderSystem() && Ogre::GpuProgramManager::getSingletonPtr() &&
            ( mUseMicrocodeCache || mUseHlmsDiskCache ) )
        {
            Ogre::HlmsManager *hlmsManager = mRoot->getHlmsManager();
            Ogre::HlmsDiskCache diskCache( hlmsManager );

            Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();

            Ogre::Archive *rwAccessFolderArchive =
                archiveManager.load( mWriteAccessFolder, "FileSystem", false );

            if( mUseHlmsDiskCache )
            {
                for( size_t i = Ogre::HLMS_LOW_LEVEL + 1u; i < Ogre::HLMS_MAX; ++i )
                {
                    Ogre::Hlms *hlms = hlmsManager->getHlms( static_cast<Ogre::HlmsTypes>( i ) );
                    if( hlms && hlms->isShaderCodeCacheDirty() )
                    {
                        diskCache.copyFrom( hlms );

                        Ogre::DataStreamPtr diskCacheFile = rwAccessFolderArchive->create(
                            "hlmsDiskCache" + Ogre::StringConverter::toString( i ) + ".bin" );
                        diskCache.saveTo( diskCacheFile );
                    }
                }
            }

            if( Ogre::GpuProgramManager::getSingleton().isCacheDirty() && mUseMicrocodeCache )
            {
                const Ogre::String filename = "microcodeCodeCache.cache";
                Ogre::DataStreamPtr shaderCacheFile = rwAccessFolderArchive->create( filename );
                Ogre::GpuProgramManager::getSingleton().saveMicrocodeCache( shaderCacheFile );
            }

            if( mUseMicrocodeCache /* mUsePipelineCache */ )
            {
                const Ogre::String filename = "pipelineCache.cache";
                Ogre::DataStreamPtr shaderCacheFile = rwAccessFolderArchive->create( filename );
                mRoot->getRenderSystem()->savePipelineCache( shaderCacheFile );
            }

            archiveManager.unload( mWriteAccessFolder );
        }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::setupResources()
    {
        // Load resource paths from config file
        Ogre::ConfigFile cf;
        cf.load( AndroidSystems::openFile( mResourcePath + "resources2.cfg" ) );

        // Go through all sections & settings in the file
        Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

        Ogre::String secName, typeName, archName;
        while( seci.hasMoreElements() )
        {
            secName = seci.peekNextKey();
            Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();

            if( secName != "Hlms" )
            {
                Ogre::ConfigFile::SettingsMultiMap::iterator i;
                for( i = settings->begin(); i != settings->end(); ++i )
                {
                    typeName = i->first;
                    archName = i->second;
                    addResourceLocation( archName, typeName, secName );
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::registerHlms()
    {
        Ogre::ConfigFile cf;
        cf.load( AndroidSystems::openFile( mResourcePath + "resources2.cfg" ) );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        Ogre::String rootHlmsFolder =
            Ogre::macBundlePath() + '/' + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#else
        Ogre::String rootHlmsFolder = mResourcePath + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#endif

        if( rootHlmsFolder.empty() )
            rootHlmsFolder = AndroidSystems::isAndroid() ? "/" : "./";
        else if( *( rootHlmsFolder.end() - 1 ) != '/' )
            rootHlmsFolder += "/";

        // At this point rootHlmsFolder should be a valid path to the Hlms data folder

        Ogre::HlmsUnlit *hlmsUnlit = 0;
        Ogre::HlmsPbs *hlmsPbs = 0;

        // For retrieval of the paths to the different folders needed
        Ogre::String mainFolderPath;
        Ogre::StringVector libraryFoldersPaths;
        Ogre::StringVector::const_iterator libraryFolderPathIt;
        Ogre::StringVector::const_iterator libraryFolderPathEn;

        Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();

        const Ogre::String &archiveType = getMediaReadArchiveType();

        {
            // Create & Register HlmsUnlit
            // Get the path to all the subdirectories used by HlmsUnlit
            Ogre::HlmsUnlit::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
            Ogre::Archive *archiveUnlit =
                archiveManager.load( rootHlmsFolder + mainFolderPath, archiveType, true );
            Ogre::ArchiveVec archiveUnlitLibraryFolders;
            libraryFolderPathIt = libraryFoldersPaths.begin();
            libraryFolderPathEn = libraryFoldersPaths.end();
            while( libraryFolderPathIt != libraryFolderPathEn )
            {
                Ogre::Archive *archiveLibrary =
                    archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, archiveType, true );
                archiveUnlitLibraryFolders.push_back( archiveLibrary );
                ++libraryFolderPathIt;
            }

            // Create and register the unlit Hlms
            hlmsUnlit = OGRE_NEW Ogre::HlmsUnlit( archiveUnlit, &archiveUnlitLibraryFolders );
            Ogre::Root::getSingleton().getHlmsManager()->registerHlms( hlmsUnlit );
        }

        {
            // Create & Register HlmsPbs
            // Do the same for HlmsPbs:
            Ogre::HlmsPbs::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
            Ogre::Archive *archivePbs =
                archiveManager.load( rootHlmsFolder + mainFolderPath, archiveType, true );

            // Get the library archive(s)
            Ogre::ArchiveVec archivePbsLibraryFolders;
            libraryFolderPathIt = libraryFoldersPaths.begin();
            libraryFolderPathEn = libraryFoldersPaths.end();
            while( libraryFolderPathIt != libraryFolderPathEn )
            {
                Ogre::Archive *archiveLibrary =
                    archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, archiveType, true );
                archivePbsLibraryFolders.push_back( archiveLibrary );
                ++libraryFolderPathIt;
            }

            // Create and register
            hlmsPbs = OGRE_NEW Ogre::HlmsPbs( archivePbs, &archivePbsLibraryFolders );
            Ogre::Root::getSingleton().getHlmsManager()->registerHlms( hlmsPbs );
        }

        Ogre::RenderSystem *renderSystem = mRoot->getRenderSystem();
        if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
        {
            // Set lower limits 512kb instead of the default 4MB per Hlms in D3D 11.0
            // and below to avoid saturating AMD's discard limit (8MB) or
            // saturate the PCIE bus in some low end machines.
            bool supportsNoOverwriteOnTextureBuffers;
            renderSystem->getCustomAttribute( "MapNoOverwriteOnDynamicBufferSRV",
                                              &supportsNoOverwriteOnTextureBuffers );

            if( !supportsNoOverwriteOnTextureBuffers )
            {
                hlmsPbs->setTextureBufferDefaultSize( 512 * 1024 );
                hlmsUnlit->setTextureBufferDefaultSize( 512 * 1024 );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::loadResources()
    {
        registerHlms();

        loadTextureCache();
        loadHlmsDiskCache();

        // Initialise, parse scripts etc
        Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups( true );

        try
        {
            mRoot->getHlmsManager()->loadBlueNoise();
        }
        catch( Ogre::FileNotFoundException &e )
        {
            Ogre::LogManager::getSingleton().logMessage( e.getFullDescription(), Ogre::LML_CRITICAL );
            Ogre::LogManager::getSingleton().logMessage(
                "WARNING: Blue Noise textures could not be loaded.", Ogre::LML_CRITICAL );
        }

        // Initialize resources for LTC area lights and accurate specular reflections (IBL)
        Ogre::Hlms *hlms = mRoot->getHlmsManager()->getHlms( Ogre::HLMS_PBS );
        OGRE_ASSERT_HIGH( dynamic_cast<Ogre::HlmsPbs *>( hlms ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlms );
        try
        {
            hlmsPbs->loadLtcMatrix();
        }
        catch( Ogre::FileNotFoundException &e )
        {
            Ogre::LogManager::getSingleton().logMessage( e.getFullDescription(), Ogre::LML_CRITICAL );
            Ogre::LogManager::getSingleton().logMessage(
                "WARNING: LTC matrix textures could not be loaded. Accurate specular IBL reflections "
                "and LTC area lights won't be available or may not function properly!",
                Ogre::LML_CRITICAL );
        }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::chooseSceneManager()
    {
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
        // Debugging multithreaded code is a PITA, disable it.
        const size_t numThreads = 1;
#else
        // getNumLogicalCores() may return 0 if couldn't detect
        const size_t numThreads = std::max<size_t>( 1, Ogre::PlatformInformation::getNumLogicalCores() );
#endif
        // Create the SceneManager, in this case a generic one
        mSceneManager = mRoot->createSceneManager( Ogre::ST_GENERIC, numThreads, "ExampleSMInstance" );

        mSceneManager->addRenderQueueListener( mOverlaySystem );
        mSceneManager->getRenderQueue()->setSortRenderQueue(
            Ogre::v1::OverlayManager::getSingleton().mDefaultRenderQueueId,
            Ogre::RenderQueue::StableSort );

        // Set sane defaults for proper shadow mapping
        mSceneManager->setShadowDirectionalLightExtrusionDistance( 500.0f );
        mSceneManager->setShadowFarDistance( 500.0f );
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::createCamera()
    {
        mCamera = mSceneManager->createCamera( "Main Camera" );

        // Position it at 500 in Z direction
        mCamera->setPosition( Ogre::Vector3( 0, 5, 15 ) );
        // Look back along -Z
        mCamera->lookAt( Ogre::Vector3( 0, 0, 0 ) );
        mCamera->setNearClipDistance( 0.2f );
        mCamera->setFarClipDistance( 1000.0f );
        mCamera->setAutoAspectRatio( true );
    }
    //-----------------------------------------------------------------------------------
    Ogre::CompositorWorkspace *GraphicsSystem::setupCompositor()
    {
        Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();

        const Ogre::String workspaceName( "Demo Workspace" );
        if( !compositorManager->hasWorkspaceDefinition( workspaceName ) )
        {
            compositorManager->createBasicWorkspaceDef( workspaceName, mBackgroundColour,
                                                        Ogre::IdString() );
        }

        return compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(), mCamera,
                                                workspaceName, true );
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::initMiscParamsListener( Ogre::NameValuePairList &params ) {}
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::createAtmosphere( Ogre::Light *sunLight )
    {
#ifdef OGRE_BUILD_COMPONENT_ATMOSPHERE
        {
            Ogre::AtmosphereComponent *atmosphere = mSceneManager->getAtmosphereRaw();
            OGRE_DELETE atmosphere;
        }

        Ogre::AtmosphereNpr *atmosphere =
            OGRE_NEW Ogre::AtmosphereNpr( mRoot->getRenderSystem()->getVaoManager() );

        {
            // Preserve the Power Scale explicitly set by the sample
            Ogre::AtmosphereNpr::Preset preset = atmosphere->getPreset();
            preset.linkedLightPower = sunLight->getPowerScale();
            atmosphere->setPreset( preset );
        }

        atmosphere->setSunDir(
            sunLight->getDirection(),
            std::asin( Ogre::Math::Clamp( -sunLight->getDirection().y, -1.0f, 1.0f ) ) /
                Ogre::Math::PI );
        atmosphere->setLight( sunLight );
        atmosphere->setSky( mSceneManager, true );
#endif
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::setAlwaysAskForConfig( bool alwaysAskForConfig )
    {
        mAlwaysAskForConfig = alwaysAskForConfig;
    }
    //-----------------------------------------------------------------------------------
    const char *GraphicsSystem::getMediaReadArchiveType() const
    {
#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
        return "FileSystem";
#else
        return "APKFileSystem";
#endif
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::stopCompositor()
    {
        if( mWorkspace )
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            compositorManager->removeWorkspace( mWorkspace );
            mWorkspace = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::restartCompositor()
    {
        stopCompositor();
        mWorkspace = setupCompositor();
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    struct GameEntityCmp
    {
        bool operator()( const GameEntity *_l, const Ogre::Matrix4 *RESTRICT_ALIAS _r ) const
        {
            const Ogre::Transform &transform = _l->mSceneNode->_getTransform();
            return &transform.mDerivedTransform[transform.mIndex] < _r;
        }

        bool operator()( const Ogre::Matrix4 *RESTRICT_ALIAS _r, const GameEntity *_l ) const
        {
            const Ogre::Transform &transform = _l->mSceneNode->_getTransform();
            return _r < &transform.mDerivedTransform[transform.mIndex];
        }

        bool operator()( const GameEntity *_l, const GameEntity *_r ) const
        {
            const Ogre::Transform &lTransform = _l->mSceneNode->_getTransform();
            const Ogre::Transform &rTransform = _r->mSceneNode->_getTransform();
            return &lTransform.mDerivedTransform[lTransform.mIndex] <
                   &rTransform.mDerivedTransform[rTransform.mIndex];
        }
    };
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::gameEntityAdded( const GameEntityManager::CreatedGameEntity *cge )
    {
        Ogre::SceneNode *sceneNode =
            mSceneManager->getRootSceneNode( cge->gameEntity->mType )
                ->createChildSceneNode( cge->gameEntity->mType, cge->initialTransform.vPos,
                                        cge->initialTransform.qRot );

        sceneNode->setScale( cge->initialTransform.vScale );

        cge->gameEntity->mSceneNode = sceneNode;

        if( cge->gameEntity->mMoDefinition->moType == MoTypeItem )
        {
            Ogre::Item *item = mSceneManager->createItem( cge->gameEntity->mMoDefinition->meshName,
                                                          cge->gameEntity->mMoDefinition->resourceGroup,
                                                          cge->gameEntity->mType );

            Ogre::StringVector materialNames = cge->gameEntity->mMoDefinition->submeshMaterials;
            size_t minMaterials = std::min( materialNames.size(), item->getNumSubItems() );

            for( size_t i = 0; i < minMaterials; ++i )
            {
                item->getSubItem( i )->setDatablockOrMaterialName(
                    materialNames[i], cge->gameEntity->mMoDefinition->resourceGroup );
            }

            cge->gameEntity->mMovableObject = item;
        }

        sceneNode->attachObject( cge->gameEntity->mMovableObject );

        // Keep them sorted on how Ogre's internal memory manager assigned them memory,
        // to avoid false cache sharing when we update the nodes concurrently.
        const Ogre::Transform &transform = sceneNode->_getTransform();
        GameEntityVec::iterator itGameEntity = std::lower_bound(
            mGameEntities[cge->gameEntity->mType].begin(), mGameEntities[cge->gameEntity->mType].end(),
            &transform.mDerivedTransform[transform.mIndex], GameEntityCmp() );
        mGameEntities[cge->gameEntity->mType].insert( itGameEntity, cge->gameEntity );
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::gameEntityRemoved( GameEntity *toRemove )
    {
        const Ogre::Transform &transform = toRemove->mSceneNode->_getTransform();
        GameEntityVec::iterator itGameEntity = std::lower_bound(
            mGameEntities[toRemove->mType].begin(), mGameEntities[toRemove->mType].end(),
            &transform.mDerivedTransform[transform.mIndex], GameEntityCmp() );

        assert( itGameEntity != mGameEntities[toRemove->mType].end() && *itGameEntity == toRemove );
        mGameEntities[toRemove->mType].erase( itGameEntity );

        toRemove->mSceneNode->getParentSceneNode()->removeAndDestroyChild( toRemove->mSceneNode );
        toRemove->mSceneNode = 0;

        assert( dynamic_cast<Ogre::Item *>( toRemove->mMovableObject ) );

        mSceneManager->destroyItem( static_cast<Ogre::Item *>( toRemove->mMovableObject ) );
        toRemove->mMovableObject = 0;
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::updateGameEntities( const GameEntityVec &gameEntities, float weight )
    {
        mThreadGameEntityToUpdate = &gameEntities;
        mThreadWeight = weight;

        // Note: You could execute a non-blocking scalable task and do something else, you should
        // wait for the task to finish right before calling renderOneFrame or before trying to
        // execute another UserScalableTask (you would have to be careful, but it could work).
        mSceneManager->executeUserScalableTask( this, true );
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::execute( size_t threadId, size_t numThreads )
    {
        size_t currIdx = mCurrentTransformIdx;
        size_t prevIdx =
            ( mCurrentTransformIdx + NUM_GAME_ENTITY_BUFFERS - 1 ) % NUM_GAME_ENTITY_BUFFERS;

        const size_t objsPerThread =
            ( mThreadGameEntityToUpdate->size() + ( numThreads - 1 ) ) / numThreads;
        const size_t toAdvance = std::min( threadId * objsPerThread, mThreadGameEntityToUpdate->size() );

        GameEntityVec::const_iterator itor =
            mThreadGameEntityToUpdate->begin() + static_cast<ptrdiff_t>( toAdvance );
        GameEntityVec::const_iterator end =
            mThreadGameEntityToUpdate->begin() +
            static_cast<ptrdiff_t>(
                std::min( toAdvance + objsPerThread, mThreadGameEntityToUpdate->size() ) );
        while( itor != end )
        {
            GameEntity *gEnt = *itor;
            Ogre::Vector3 interpVec = Ogre::Math::lerp( gEnt->mTransform[prevIdx]->vPos,
                                                        gEnt->mTransform[currIdx]->vPos, mThreadWeight );
            gEnt->mSceneNode->setPosition( interpVec );

            interpVec = Ogre::Math::lerp( gEnt->mTransform[prevIdx]->vScale,
                                          gEnt->mTransform[currIdx]->vScale, mThreadWeight );
            gEnt->mSceneNode->setScale( interpVec );

            Ogre::Quaternion interpQ = Ogre::Quaternion::nlerp(
                mThreadWeight, gEnt->mTransform[prevIdx]->qRot, gEnt->mTransform[currIdx]->qRot, true );
            gEnt->mSceneNode->setOrientation( interpQ );

            ++itor;
        }
    }
}  // namespace Demo
