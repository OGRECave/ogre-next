
#include "GraphicsSystem.h"
#include "MemoryCleanupGameState.h"

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
    class MemoryCleanupGraphicsSystem : public GraphicsSystem
    {
        virtual Ogre::CompositorWorkspace* setupCompositor()
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            mWorkspace = compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(),
                                                          mCamera, "ShadowMapDebuggingWorkspace", true );
            return mWorkspace;
        }

        virtual void initMiscParamsListener( Ogre::NameValuePairList &params )
        {
            //Used by GL3+ & Metal
            params["VaoManager::CPU_INACCESSIBLE"] = "0";
            params["VaoManager::CPU_ACCESSIBLE_DEFAULT"] = "0";
            params["VaoManager::CPU_ACCESSIBLE_PERSISTENT"] = "0";
            params["VaoManager::CPU_ACCESSIBLE_PERSISTENT_COHERENT"] = "0";

            //Used by D3D11
            params["VaoManager::VERTEX_IMMUTABLE"] = "0";
            params["VaoManager::VERTEX_DEFAULT"] = "0";
            params["VaoManager::VERTEX_DYNAMIC"] = "0";
            params["VaoManager::INDEX_IMMUTABLE"] = "0";
            params["VaoManager::INDEX_DEFAULT"] = "0";
            params["VaoManager::INDEX_DYNAMIC"] = "0";
            params["VaoManager::SHADER_IMMUTABLE"] = "0";
            params["VaoManager::SHADER_DEFAULT"] = "0";
            params["VaoManager::SHADER_DYNAMIC"] = "0";
        }

    public:
        MemoryCleanupGraphicsSystem( GameState *gameState ) :
            GraphicsSystem( gameState )
        {
        }
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState,
                                         LogicSystem **outLogicSystem )
    {
        MemoryCleanupGameState *gfxGameState = new MemoryCleanupGameState(
        "" );

        GraphicsSystem *graphicsSystem = new MemoryCleanupGraphicsSystem( gfxGameState );

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
        return "Memory Cleanup";
    }
}
