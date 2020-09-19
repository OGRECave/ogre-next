
#include "GraphicsSystem.h"
#include "PccPerPixelGridPlacementGameState.h"

#include "OgreSceneManager.h"
#include "OgreCamera.h"
#include "OgreRoot.h"
#include "OgreWindow.h"
#include "OgreConfigFile.h"
#include "Compositor/OgreCompositorManager2.h"

//Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/Android/AndroidSystems.h"
#include "System/MainEntryPoints.h"

#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMainApp( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#else
int mainApp( int argc, const char *argv[] )
#endif
{
    return Demo::MainEntryPoints::mainAppSingleThreaded( DEMO_MAIN_ENTRY_PARAMS );
}
#endif

namespace Demo
{
    class PccPerPixelGridPlacementGraphicsSystem : public GraphicsSystem
    {
        virtual Ogre::CompositorWorkspace* setupCompositor()
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            return compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(),
                                                    mCamera, "LocalCubemapsWorkspace", true );
        }

        virtual void setupResources(void)
        {
            GraphicsSystem::setupResources();

            Ogre::ConfigFile cf;
            cf.load( AndroidSystems::openFile( mResourcePath + "resources2.cfg" ) );

            Ogre::String originalDataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

            if( originalDataFolder.empty() )
                originalDataFolder = AndroidSystems::isAndroid() ? "/" : "./";
            else if( *(originalDataFolder.end() - 1) != '/' )
                originalDataFolder += "/";

            const char *c_locations[5] =
            {
                "2.0/scripts/materials/PccPerPixelGridPlacement/",
                "2.0/scripts/materials/PccPerPixelGridPlacement/GLSL",
                "2.0/scripts/materials/PccPerPixelGridPlacement/HLSL",
                "2.0/scripts/materials/PccPerPixelGridPlacement/Metal",
                "2.0/scripts/materials/TutorialSky_Postprocess"
            };

            for( size_t i=0; i<5; ++i )
            {
                Ogre::String dataFolder = originalDataFolder + c_locations[i];
                addResourceLocation( dataFolder, getMediaReadArchiveType(), "General" );
            }
        }

    public:
        PccPerPixelGridPlacementGraphicsSystem( GameState *gameState ) :
            GraphicsSystem( gameState )
        {
        }
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState,
                                         LogicSystem **outLogicSystem )
    {
        PccPerPixelGridPlacementGameState *gfxGameState = new PccPerPixelGridPlacementGameState(
        "Placing multiple PCC probes by hand can be difficult and even error prone.\n"
        "PccPerPixelGridPlacement aims at helping developers and artists by automatically\n"
        "finding ideal probe shape sizes that adjust to underlying geometry in the scene by\n"
        "only providing the overall enclosing AABB of the scene that needs to be covered by PCC.\n"
        "There are other settings such as overlap which trade quality and performance\n"
        "\n"
        "This sample depends on the media files:\n"
        "   * Samples/Media/2.0/scripts/Compositors/PccPerPixelGridPlacement.compositor\n"
        "   * Samples/Media/2.0/materials/PccPerPixelGridPlacement/*\n"
        "\n" );

        GraphicsSystem *graphicsSystem = new PccPerPixelGridPlacementGraphicsSystem( gfxGameState );

        gfxGameState->_notifyGraphicsSystem( graphicsSystem );

        *outGraphicsGameState = gfxGameState;
        *outGraphicsSystem = graphicsSystem;
    }

    void MainEntryPoints::destroySystems( GameState *graphicsGameState,
                                          GraphicsSystem *graphicsSystem,
                                          GameState *logicGameState,
                                          LogicSystem *logicSystem )
    {
        delete graphicsSystem;
        delete graphicsGameState;
    }

    const char* MainEntryPoints::getWindowTitle(void)
    {
        return "Automatically-placed Parallax Corrected Cubemap probes via PccPerPixelGridPlacement";
    }
}
