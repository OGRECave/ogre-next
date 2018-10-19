
#include "GraphicsSystem.h"
#include "ParticlesGameState.h"

#include "OgreRoot.h"
#include "OgreRenderWindow.h"
#include "Compositor/OgreCompositorManager2.h"
#include "OgreConfigFile.h"

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
    class ParticlesGraphicsSystem : public GraphicsSystem
    {
        virtual Ogre::CompositorWorkspace* setupCompositor()
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            mWorkspace = compositorManager->addWorkspace( mSceneManager, mRenderWindow, mCamera,
                                                          "ShadowMapDebuggingWorkspace", true );
            return mWorkspace;
        }
        
        virtual void setupResources(void)
        {
            GraphicsSystem::setupResources();

            Ogre::ConfigFile cf;
            cf.load(mResourcePath + "resources2.cfg");

            Ogre::String originalDataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

            if( originalDataFolder.empty() )
                originalDataFolder = "./";
            else if( *(originalDataFolder.end() - 1) != '/' )
                originalDataFolder += "/";

            const char *c_locations[3] =
            {
                "2.0/scripts/materials/PbsMaterials",
                "2.0/scripts/particles",
                "2.0/scripts/materials/ParticleSystems"
            };
            
            for (size_t i = 0; i<3; ++i)
            {
                Ogre::String dataFolder = originalDataFolder + c_locations[i];
                addResourceLocation(dataFolder, "FileSystem", "General");
            }
        }

    public:
        ParticlesGraphicsSystem( GameState *gameState ) :
            GraphicsSystem( gameState )
        {
        }
    };
    
    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState,
                                         LogicSystem **outLogicSystem )
    {
        ParticlesGameState *gfxGameState = new ParticlesGameState(
        "This sample shows how to use Particle Systems with HLMS. \n"
         );

        GraphicsSystem *graphicsSystem = new ParticlesGraphicsSystem( gfxGameState );

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
        return "Particle Systems";
    }
}
