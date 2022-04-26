
#include "GraphicsSystem.h"
#include "SdlInputHandler.h"
#include "Tutorial_SMAAGameState.h"

#include "OgreTimer.h"
#include "OgreWindow.h"
#include "Threading/OgreThreads.h"

#include "Compositor/OgreCompositorManager2.h"
#include "OgreArchive.h"
#include "OgreArchiveManager.h"
#include "OgreConfigFile.h"
#include "OgreRoot.h"

#include "OgreHlms.h"
#include "OgreHlmsDatablock.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsPrerequisites.h"

// Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/Android/AndroidSystems.h"
#include "System/MainEntryPoints.h"

namespace Demo
{
    class Tutorial_SMAAGraphicsSystem final : public GraphicsSystem
    {
        Ogre::CompositorWorkspace *setupCompositor() override
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            return compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(), mCamera,
                                                    "TutorialSMAA_Workspace", true );
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

            const char *c_locations[6] = {
                "2.0/scripts/materials/Tutorial_SMAA",
                "2.0/scripts/materials/Tutorial_SMAA/GLSL",
                "2.0/scripts/materials/Tutorial_SMAA/HLSL",
                "2.0/scripts/materials/Tutorial_SMAA/Metal",
                "2.0/scripts/materials/Tutorial_SMAA/Vulkan",
                "2.0/scripts/materials/Tutorial_SMAA/TutorialCompositorScript",
            };

            for( size_t i = 0; i < 6; ++i )
            {
                Ogre::String dataFolderFull = dataFolder + c_locations[i];
                addResourceLocation( dataFolderFull, getMediaReadArchiveType(), "General" );
            }
        }

    public:
        Tutorial_SMAAGraphicsSystem( GameState *gameState ) : GraphicsSystem( gameState ) {}
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState, LogicSystem **outLogicSystem )
    {
        Tutorial_SMAAGameState *gfxGameState = new Tutorial_SMAAGameState( "TBD" );

        GraphicsSystem *graphicsSystem = new Tutorial_SMAAGraphicsSystem( gfxGameState );

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

    const char *MainEntryPoints::getWindowTitle()
    {
        return "SMAA (Enhanced Subpixel Morphological Antialiasing) Demo";
    }
}  // namespace Demo

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
