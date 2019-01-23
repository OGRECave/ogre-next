
#include "GraphicsSystem.h"
#include "Tutorial_MemoryGameState.h"

#include "OgreSceneManager.h"
#include "OgreCamera.h"
#include "OgreRoot.h"
#include "OgreWindow.h"
#include "OgreConfigFile.h"
#include "Compositor/OgreCompositorManager2.h"

//Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/MainEntryPoints.h"

#include "OgreRenderSystem.h"
#include "OgreTextureGpuManager.h"
#include "OgreStringConverter.h"

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
    class MemoryGraphicsSystem : public GraphicsSystem
    {
        virtual Ogre::CompositorWorkspace* setupCompositor()
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            return compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(), mCamera,
                                                    "LocalCubemapsWorkspace", true );
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

            const char *c_locations[5] =
            {
                "2.0/scripts/materials/PbsMaterials",
                "2.0/scripts/materials/LocalCubemaps/",
                "2.0/scripts/materials/LocalCubemaps/GLSL",
                "2.0/scripts/materials/LocalCubemaps/HLSL",
                "2.0/scripts/materials/LocalCubemaps/Metal"
            };

            for( size_t i=0; i<sizeof(c_locations) / sizeof(c_locations[0]); ++i )
            {
                Ogre::String dataFolder = originalDataFolder + c_locations[i];
                addResourceLocation( dataFolder, "FileSystem", "General" );
            }
        }

        virtual void initMiscParamsListener( Ogre::NameValuePairList &params )
        {
            //The default parameters may be fine, but they may be not for you.
            //Monitor what's your average and worst case consumption,
            //and then set these parameters accordingly.
            //
            //In particular monitor the output of VaoManager::getMemoryStats
            //to select the proper defaults best suited for your application.
            //A value of 0 bytes is a valid input.

            //Used by GL3+ & Metal
            params["VaoManager::CPU_INACCESSIBLE"] =
                    Ogre::StringConverter::toString( 8u * 1024u * 1024u );
            params["VaoManager::CPU_ACCESSIBLE_DEFAULT"] =
                    Ogre::StringConverter::toString( 4u * 1024u * 1024u );
            params["VaoManager::CPU_ACCESSIBLE_PERSISTENT"] =
                    Ogre::StringConverter::toString( 8u * 1024u * 1024u );
            params["VaoManager::CPU_ACCESSIBLE_PERSISTENT_COHERENT"] =
                    Ogre::StringConverter::toString( 4u * 1024u * 1024u );

            //Used by D3D11
            params["VaoManager::VERTEX_IMMUTABLE"] =
                    Ogre::StringConverter::toString( 4u * 1024u * 1024u );
            params["VaoManager::VERTEX_DEFAULT"] =
                    Ogre::StringConverter::toString( 8u * 1024u * 1024u );
            params["VaoManager::VERTEX_DYNAMIC"] =
                    Ogre::StringConverter::toString( 4u * 1024u * 1024u );
            params["VaoManager::INDEX_IMMUTABLE"] =
                    Ogre::StringConverter::toString( 4u * 1024u * 1024u );
            params["VaoManager::INDEX_DEFAULT"] =
                    Ogre::StringConverter::toString( 4u * 1024u * 1024u );
            params["VaoManager::INDEX_DYNAMIC"] =
                    Ogre::StringConverter::toString( 4u * 1024u * 1024u );
            params["VaoManager::SHADER_IMMUTABLE"] =
                    Ogre::StringConverter::toString( 4u * 1024u * 1024u );
            params["VaoManager::SHADER_DEFAULT"] =
                    Ogre::StringConverter::toString( 4u * 1024u * 1024u );
            params["VaoManager::SHADER_DYNAMIC"] =
                    Ogre::StringConverter::toString( 4u * 1024u * 1024u );
        }

    public:
        MemoryGraphicsSystem( GameState *gameState ) :
            GraphicsSystem( gameState )
        {
        }
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState,
                                         LogicSystem **outLogicSystem )
    {
        MemoryGameState *gfxGameState = new MemoryGameState(
        "This sample shows how to monitor consumed GPU (& CPU) memory and "
        "how to reduce memory consumption" );

        GraphicsSystem *graphicsSystem = new MemoryGraphicsSystem( gfxGameState );

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
        return "Memory Monitor";
    }
}
