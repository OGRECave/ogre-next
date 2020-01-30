
#include "GraphicsSystem.h"
#include "RefractionsGameState.h"

#include "OgreSceneManager.h"
#include "OgreCamera.h"
#include "OgreRoot.h"
#include "OgreWindow.h"
#include "OgreConfigFile.h"
#include "Compositor/OgreCompositorManager2.h"

//Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/MainEntryPoints.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMainApp( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#else
int mainApp( int argc, const char *argv[] )
#endif
{
    return Demo::MainEntryPoints::mainAppSingleThreaded( DEMO_MAIN_ENTRY_PARAMS );
}

namespace Demo
{
    class RefractionsGraphicsSystem : public GraphicsSystem
    {
        virtual Ogre::CompositorWorkspace* setupCompositor()
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            const bool useMsaa = mRenderWindow->isMultisample();

            //ScreenSpaceReflections::setupSSR( useMsaa, true, compositorManager );

            Ogre::String compositorName = "RefractionsWorkspace";
            if( useMsaa )
                compositorName = "RefractionsWorkspaceMsaa";

            return compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(),
                                                    mCamera, compositorName, true );
        }

        virtual void setupResources(void)
        {
            GraphicsSystem::setupResources();

            Ogre::ConfigFile cf;
            cf.load(mResourcePath + "resources2.cfg");

            Ogre::String dataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

            if( dataFolder.empty() )
                dataFolder = "./";
            else if( *(dataFolder.end() - 1) != '/' )
                dataFolder += "/";

            dataFolder += "2.0/scripts/materials/PbsMaterials";

            addResourceLocation( dataFolder, "FileSystem", "General" );
        }

    public:
        RefractionsGraphicsSystem( GameState *gameState ) :
            GraphicsSystem( gameState )
        {
        }
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState,
                                         LogicSystem **outLogicSystem )
    {
        RefractionsGameState *gfxGameState = new RefractionsGameState(
        "Shows how to use Refractions.\n"
        "Refractions must be rendered in its own special scene pass, with depth\n"
        "writes disabled.\n"
        "\n"
        "Multiple refractions are not supported. But to make it worse, refractive objects\n"
        "rendered on top of other refractive objects often result in rendering artifacts.\n"
        "\n"
        "It is similar to Order Dependent Transparency issues (i.e. regular alpha blending) but\n"
        "with even more artifacts.\n"
        "\n"
        "Therefore there is a very simple yet effective workaround: Create a duplicate of the\n"
        "object rendered with regular alpha blending, and that actually outputs depth to the\n"
        "depth buffer.\n"
        "\n"
        "This greatly fixes some depth-related bugs and rendering order issues; while it also\n"
        "creates a 'fallback' because now refractive objects behind other refractive objects will\n"
        "appear (however they won't cause multiple refractions, you will be seeing the alpha blended\n"
        "version)\n"
        "\n"
        "The fallback might cause some glitches of its own, but this a much better alternative\n"
        "Toggle F4 to see refractions without the placeholder\n"
        "\n"
        "This sample depends on the media files:\n"
        "   * Samples/Media/2.0/scripts/Compositors/Refractions.compositor\n"
        "\n"
        "LEGAL: Uses Saint Peter's Basilica (C) by Emil Persson under CC Attrib 3.0 Unported\n"
        "See Samples/Media/materials/textures/Cubemaps/License.txt for more information." );

        GraphicsSystem *graphicsSystem = new RefractionsGraphicsSystem( gfxGameState );

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
        return "Refractions Sample";
    }
}
