
#include "GraphicsSystem.h"
#include "UpdatingDecalsAndAreaLightTexGameState.h"

#include "Compositor/OgreCompositorManager2.h"
#include "OgreCamera.h"
#include "OgreConfigFile.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreWindow.h"

#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"

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
    class UpdatingDecalsAndAreaLightTexGraphicsSystem final : public GraphicsSystem
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
            cf.load( AndroidSystems::openFile( mResourcePath + "resources2.cfg" ) );

            Ogre::String dataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

            if( dataFolder.empty() )
                dataFolder = AndroidSystems::isAndroid() ? "/" : "./";
            else if( *( dataFolder.end() - 1 ) != '/' )
                dataFolder += "/";

            const size_t baseSize = dataFolder.size();

            dataFolder.resize( baseSize );
            dataFolder += "2.0/scripts/materials/PbsMaterials";
            addResourceLocation( dataFolder, getMediaReadArchiveType(), "General" );
            dataFolder.resize( baseSize );
            dataFolder += "2.0/scripts/materials/UpdatingDecalsAndAreaLightTex";
            addResourceLocation( dataFolder, getMediaReadArchiveType(), "General" );
        }

        void loadResources() override
        {
            GraphicsSystem::loadResources();

            Ogre::Hlms *hlms = mRoot->getHlmsManager()->getHlms( Ogre::HLMS_PBS );
            OGRE_ASSERT_HIGH( dynamic_cast<Ogre::HlmsPbs *>( hlms ) );
            Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlms );
            hlmsPbs->loadLtcMatrix();
        }

    public:
        UpdatingDecalsAndAreaLightTexGraphicsSystem( GameState *gameState ) : GraphicsSystem( gameState )
        {
        }
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState, LogicSystem **outLogicSystem )
    {
        UpdatingDecalsAndAreaLightTexGameState *gfxGameState =
            new UpdatingDecalsAndAreaLightTexGameState(
                "Shows how to create area light textures dynamically and individually.\n"
                "This method can also be used for the textures Decals use.\n"
                "This sample depends on the media files:\n"
                "   * Samples/Media/2.0/scripts/Compositors/ShadowMapDebugging.compositor\n"
                "   * Samples/Media/2.0/materials/PbsMaterials/*\n"
                "   * Samples/Media/2.0/materials/UpdatingDecalsAndAreaLightTex/*\n"
                "\n" );

        GraphicsSystem *graphicsSystem = new UpdatingDecalsAndAreaLightTexGraphicsSystem( gfxGameState );

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

    const char *MainEntryPoints::getWindowTitle() { return "Updating Decals and Area Lights' textures"; }
}  // namespace Demo
