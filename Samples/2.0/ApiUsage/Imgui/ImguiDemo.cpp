
#include "GraphicsSystem.h"

#include "ImguiDemo.h"
#include "ImguiGameState.h"

#include "Compositor/OgreCompositorManager2.h"
#include "OgreConfigFile.h"
#include "OgreRoot.h"
#include "OgreWindow.h"

#include "OgreCompositorPassImguiProvider.h"
#include "OgreImguiManager.h"

// Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/Android/AndroidSystems.h"
#include "System/MainEntryPoints.h"

#include "imgui.h"
#if OGRE_USE_SDL2
#    include "backends/imgui_impl_sdl2.h"
#endif

#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
#    if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMainApp( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#    else
int mainApp( int argc, const char *argv[] )
#    endif
{
    return Demo::MainEntryPoints::mainAppSingleThreaded( DEMO_MAIN_ENTRY_PARAMS );
}
#endif

using namespace Demo;

ImguiGraphicsSystem::ImguiGraphicsSystem( GameState *gameState ) :
    GraphicsSystem( gameState ),
    mImguiManager( 0 ),
    mImguiCompoProvider( 0 )
{
}
#if OGRE_USE_SDL2
//-----------------------------------------------------------------------------
void ImguiGraphicsSystem::handleRawSdlEvent( const SDL_Event &evt )
{
    ImGui_ImplSDL2_ProcessEvent( &evt );
}
#endif
//-----------------------------------------------------------------------------
void ImguiGraphicsSystem::deinitialize()
{
    if( mImguiCompoProvider )
    {
        Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
        compositorManager->setCompositorPassProvider( 0 );
        delete mImguiCompoProvider;
        mImguiCompoProvider = 0;
    }

    delete mImguiManager;
    mImguiManager = 0;

    ImGui_ImplSDL2_Shutdown();

    GraphicsSystem::deinitialize();
}
//-----------------------------------------------------------------------------
Ogre::CompositorWorkspace *ImguiGraphicsSystem::setupCompositor()
{
    Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
    return compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(), mCamera,
                                            "ImguiWorkspace", true );
}
//-----------------------------------------------------------------------------
void ImguiGraphicsSystem::setupResources()
{
    GraphicsSystem::setupResources();

    Ogre::ConfigFile cf;
    cf.load( AndroidSystems::openFile( mResourcePath + "resources2.cfg" ) );

    Ogre::String originalDataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

    if( originalDataFolder.empty() )
        originalDataFolder = AndroidSystems::isAndroid() ? "/" : "./";
    else if( *( originalDataFolder.end() - 1 ) != '/' )
        originalDataFolder += "/";

    const char *c_locations[] = { "2.0/scripts/materials/Imgui" };

    for( size_t i = 0; i < sizeof( c_locations ) / sizeof( c_locations[0] ); ++i )
    {
        Ogre::String dataFolder = originalDataFolder + c_locations[i];
        addResourceLocation( dataFolder, getMediaReadArchiveType(), "General" );
    }

    mImguiManager = new Ogre::ImguiManager();
    mImguiCompoProvider = new Ogre::CompositorPassImguiProvider( mImguiManager );
    Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
    compositorManager->setCompositorPassProvider( mImguiCompoProvider );

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    mImguiManager->setupFont( io.Fonts, false );
    ImGui_ImplSDL2_InitForOther( mSdlWindow );
}
//-----------------------------------------------------------------------------
void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                     GraphicsSystem **outGraphicsSystem, GameState **outLogicGameState,
                                     LogicSystem **outLogicSystem )
{
    ImguiGameState *gfxGameState = new ImguiGameState( "" );

    GraphicsSystem *graphicsSystem = new ImguiGraphicsSystem( gfxGameState );

    gfxGameState->_notifyGraphicsSystem( graphicsSystem );

    *outGraphicsGameState = gfxGameState;
    *outGraphicsSystem = graphicsSystem;
}
//-----------------------------------------------------------------------------
void MainEntryPoints::destroySystems( GameState *graphicsGameState, GraphicsSystem *graphicsSystem,
                                      GameState *logicGameState, LogicSystem *logicSystem )
{
    delete graphicsSystem;
    delete graphicsGameState;
}
//-----------------------------------------------------------------------------
const char *MainEntryPoints::getWindowTitle() { return "Dear Imgui integration Sample"; }
