
#include "GraphicsSystem.h"
#include "Tutorial_TextureBakingGameState.h"

#include "OgreSceneManager.h"
#include "OgreCamera.h"
#include "OgreRoot.h"
#include "OgreWindow.h"
#include "OgreConfigFile.h"
#include "Compositor/OgreCompositorManager2.h"

#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"

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
    class Tutorial_TextureBakingGraphicsSystem : public GraphicsSystem
    {
        virtual Ogre::CompositorWorkspace* setupCompositor()
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            mWorkspace = compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(),
                                                          mCamera, "ShadowMapDebuggingWorkspace", true );
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

            const char *c_locations[] =
            {
                "2.0/scripts/materials/Tutorial_TextureBaking",
                "2.0/scripts/materials/Tutorial_TextureBaking/GLSL",
                "2.0/scripts/materials/Tutorial_TextureBaking/HLSL",
                "2.0/scripts/materials/Tutorial_TextureBaking/Metal"
            };

            for( size_t i=0; i<sizeof(c_locations) / sizeof(c_locations[0]); ++i )
            {
                Ogre::String dataFolder = originalDataFolder + c_locations[i];
                addResourceLocation( dataFolder, "FileSystem", "General" );
            }
        }

        virtual void loadResources(void)
        {
            GraphicsSystem::loadResources();

            Ogre::Hlms *hlms = mRoot->getHlmsManager()->getHlms( Ogre::HLMS_PBS );
            OGRE_ASSERT_HIGH( dynamic_cast<Ogre::HlmsPbs*>( hlms ) );
            Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlms );
            hlmsPbs->loadLtcMatrix();
        }

    public:
        Tutorial_TextureBakingGraphicsSystem( GameState *gameState ) :
            GraphicsSystem( gameState )
        {
        }
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState,
                                         LogicSystem **outLogicSystem )
    {
        Tutorial_TextureBakingGameState *gfxGameState = new Tutorial_TextureBakingGameState(
        "Shows how to bake the render result of Ogre into a texture (e.g. for lightmaps).\n"
        "You can either bake all lights, or bake just the most expensive lights to\n"
        "later combine it with more dynamic (non-baked) lights while using light masks\n"
        "to filter lights.\n"
        "This can be a simple yet very effective way to increase performance with a high\n"
        "number of lights.\n"
        "Note that specular lighting is dependent on camera location; thus camera position when \n"
        "baking IS important. We left specular on to show this effect, which you can experiment\n"
        "by pressing F4, then F5, then moving the camera.\n"
        "Also note that if the baked plane goes out of camera, it will get culled!!!.\n"
        "This sample depends on the media files:\n"
        "   * Samples/Media/2.0/scripts/Compositors/UvBaking.compositor\n"
        "   * Samples/Media/2.0/materials/Tutorial_TextureBaking/*\n"
        "\n" );

        GraphicsSystem *graphicsSystem = new Tutorial_TextureBakingGraphicsSystem( gfxGameState );

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
        return "Texture Baking";
    }
}
