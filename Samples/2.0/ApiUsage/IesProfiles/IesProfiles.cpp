
#include "GraphicsSystem.h"
#include "IesProfilesGameState.h"

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
    class IesProfilesGraphicsSystem final : public GraphicsSystem
    {
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
            dataFolder += "2.0/scripts/materials/IesProfiles";
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
        IesProfilesGraphicsSystem( GameState *gameState ) : GraphicsSystem( gameState ) {}
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState, LogicSystem **outLogicSystem )
    {
        IesProfilesGameState *gfxGameState = new IesProfilesGameState(
            "Shows how to use IES photometric profiles.\n"
            "This sample depends on the media files:\n"
            "   * Samples/Media/2.0/materials/IesProfiles/*\n"
            "\n" );

        GraphicsSystem *graphicsSystem = new IesProfilesGraphicsSystem( gfxGameState );

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

    const char *MainEntryPoints::getWindowTitle() { return "IES Photometric Profile lights"; }
}  // namespace Demo
