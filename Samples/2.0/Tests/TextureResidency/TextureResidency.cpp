
#include "GraphicsSystem.h"
#include "TextureResidencyGameState.h"

#include "Compositor/OgreCompositorManager2.h"
#include "OgreCamera.h"
#include "OgreConfigFile.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreWindow.h"

// Declares WinMain / main
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
    class TextureResidencyGraphicsSystem final : public GraphicsSystem
    {
        Ogre::CompositorWorkspace *setupCompositor() override
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            mWorkspace = compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(),
                                                          mCamera, "ShadowMapDebuggingWorkspace", true );
            return mWorkspace;
        }

        void setupResources() override
        {
            GraphicsSystem::setupResources();

            Ogre::ConfigFile cf;
            cf.load( mResourcePath + "resources2.cfg" );

            Ogre::String dataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

            if( dataFolder.empty() )
                dataFolder = "./";
            else if( *( dataFolder.end() - 1 ) != '/' )
                dataFolder += "/";

            dataFolder += "2.0/scripts/materials/Tutorial_SMAA";

            addResourceLocation( dataFolder, getMediaReadArchiveType(), "General" );
        }

    public:
        TextureResidencyGraphicsSystem( GameState *gameState ) : GraphicsSystem( gameState ) {}
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState, LogicSystem **outLogicSystem )
    {
        TextureResidencyGameState *gfxGameState = new TextureResidencyGameState( "" );

        GraphicsSystem *graphicsSystem = new TextureResidencyGraphicsSystem( gfxGameState );

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

    const char *MainEntryPoints::getWindowTitle() { return "Texture Residency"; }
}  // namespace Demo
