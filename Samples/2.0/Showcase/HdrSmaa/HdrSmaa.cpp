
#include "../Hdr/HdrGameState.h"
#include "GraphicsSystem.h"

#include "Compositor/OgreCompositorManager2.h"
#include "OgreCamera.h"
#include "OgreConfigFile.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreWindow.h"

// Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/Android/AndroidSystems.h"
#include "System/MainEntryPoints.h"

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

namespace Demo
{
    class HdrSmaaGraphicsSystem final : public GraphicsSystem
    {
        Ogre::CompositorWorkspace *setupCompositor() override
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            Ogre::RenderSystem *renderSystem = mRoot->getRenderSystem();
            const Ogre::RenderSystemCapabilities *caps = renderSystem->getCapabilities();

            Ogre::String compositorName = "HdrSmaaWorkspace";
            if( mRenderWindow->isMultisample() &&
                caps->hasCapability( Ogre::RSC_EXPLICIT_FSAA_RESOLVE ) )
                compositorName = "HdrWorkspaceMsaa";

            return compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(), mCamera,
                                                    compositorName, true );
        }

        void setupResources() override
        {
            GraphicsSystem::setupResources();

            Ogre::ConfigFile cf;
            cf.load( AndroidSystems::openFile( mResourcePath + "resources2.cfg" ) );

            Ogre::String originalDataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

            if( originalDataFolder.empty() )
                originalDataFolder = AndroidSystems::isAndroid() ? "/" : "./";
            else if( *( originalDataFolder.end() - 1 ) != '/' )
                originalDataFolder += "/";

            const char *c_locations[11] = {
                "2.0/scripts/materials/Tutorial_SMAA",
                "2.0/scripts/materials/Tutorial_SMAA/GLSL",
                "2.0/scripts/materials/Tutorial_SMAA/HLSL",
                "2.0/scripts/materials/Tutorial_SMAA/Metal",
                "2.0/scripts/materials/Tutorial_SMAA/Vulkan",
                "2.0/scripts/materials/HDR",
                "2.0/scripts/materials/HDR/GLSL",
                "2.0/scripts/materials/HDR/HLSL",
                "2.0/scripts/materials/HDR/Metal",
                "2.0/scripts/materials/PbsMaterials",
                "2.0/scripts/materials/HDR_SMAA",
            };

            for( size_t i = 0; i < 11; ++i )
            {
                Ogre::String dataFolder = originalDataFolder + c_locations[i];
                addResourceLocation( dataFolder, getMediaReadArchiveType(), "General" );
            }
        }

    public:
        HdrSmaaGraphicsSystem( GameState *gameState ) : GraphicsSystem( gameState ) {}
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState, LogicSystem **outLogicSystem )
    {
        HdrGameState *gfxGameState = new HdrGameState(
            "This samples shows how to use HDR in combination with SMAA\n"
            "See HDR sample and Tutorial_SMAA for separate if you wish to see them individually.\n"
            "This sample depends on the media files:\n"
            "   * Samples/Media/2.0/materials/Common/Copyback.material (Copyback_1xFP32_ps)\n"
            "   * Samples/Media/2.0/materials/HDR/*.*\n"
            "   * Samples/Media/2.0/materials/Tutorial_SMAA/*.*\n"
            "   * Samples/Media/2.0/materials/HDR_SMAA/*.*\n"
            "\n"
            "\n"
            "LEGAL: Uses Saint Peter's Basilica (C) by Emil Persson under CC Attrib 3.0 Unported\n"
            "See Samples/Media/materials/textures/Cubemaps/License.txt for more information." );

        GraphicsSystem *graphicsSystem = new HdrSmaaGraphicsSystem( gfxGameState );

        gfxGameState->_notifyGraphicsSystem( graphicsSystem );

        *outGraphicsGameState = gfxGameState;
        *outGraphicsSystem = graphicsSystem;
    }

    void MainEntryPoints::destroySystems( GameState *graphicsGameState, GraphicsSystem *graphicsSystem,
                                          GameState *logicGameState, LogicSystem *logicSystem )
    {
        delete graphicsSystem;
        delete graphicsGameState;
    }

    const char *MainEntryPoints::getWindowTitle() { return "High Dynamic Range (HDR) + SMAA Sample"; }
}  // namespace Demo
