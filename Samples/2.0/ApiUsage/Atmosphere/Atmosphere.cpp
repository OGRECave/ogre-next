
#include "GraphicsSystem.h"

#include "AtmosphereGameState.h"

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
    class AtmosphereGraphicsSystem final : public GraphicsSystem
    {
        Ogre::CompositorWorkspace *setupCompositor() override
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            return compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(), mCamera,
                                                    "PbsMaterialsWorkspace", true );
        }

        void setupResources() override
        {
            GraphicsSystem::setupResources();

            Ogre::ConfigFile cf;
            cf.load( AndroidSystems::openFile( mResourcePath + "resources2.cfg" ) );

            Ogre::String dataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

            if( dataFolder.empty() )
                dataFolder = AndroidSystems::isAndroid() ? "/" : "./";
            else if( *( dataFolder.end() - 1 ) != '/' )
                dataFolder += "/";

            dataFolder += "2.0/scripts/materials/PbsMaterials";

            addResourceLocation( dataFolder, getMediaReadArchiveType(), "General" );
        }

    public:
        AtmosphereGraphicsSystem( GameState *gameState ) : GraphicsSystem( gameState ) {}
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState, LogicSystem **outLogicSystem )
    {
        AtmosphereGameState *gfxGameState = new AtmosphereGameState(
            "Shows how to use the Atmosphere NPR (non-physically-based-render) Component\n"
            "Despite being NPR (because it's not based on a physical model), it is quite convincing\n"
            "and can be used in PBS and HDR samples. There is HlmsPbs & HlmsTerra integration\n"
            "so that there is fog applied to objects based on the atmosphere\n"
            "\n"
            "It supports blending between multiple presets at different Times of Day (day & night).\n"
            "This allows fine tunning the brightness at different timedays, specially the fake GI\n"
            "multipliers in LDR.\n"
            "PbsMaterials, HDR & Terrain sample will try to use this component when possible\n"
            "to showcase its use and integration."
            "\n"
            "This sample depends on the media files:\n"
            "   * Samples/Media/2.0/scripts/materials/Common/Atmosphere.material\n"
            "   * Samples/Media/2.0/scripts/materials/Common (shaders in Atmosphere.material)\n"
            "\n" );

        GraphicsSystem *graphicsSystem = new AtmosphereGraphicsSystem( gfxGameState );

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

    const char *MainEntryPoints::getWindowTitle() { return "PBS Materials Sample"; }
}  // namespace Demo
