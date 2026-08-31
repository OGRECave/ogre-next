#include "GraphicsSystem.h"

#include "MeshLodV2GameState.h"

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
    class MeshLodV2GraphicsSystem final : public GraphicsSystem
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

            Ogre::String dataFolderSinbad = dataFolder;

            dataFolder += "2.0/scripts/materials/PbsMaterials";

            addResourceLocation( dataFolder, getMediaReadArchiveType(), "General" );

            // Sinbad.mesh is shipped in legacy v1 binary format -- loading it as v1
            // is unavoidable (that's the only format the asset exists in on disk).
            // The point of this sample is what happens AFTER that load: see
            // MeshLodV2GameState::loadAndConvertSinbadToV2() for the v1 -> v2 import
            // with NO LOD baked in, followed by native v2 LOD generation.
            dataFolderSinbad += "packs/Sinbad.zip";
            addResourceLocation( dataFolderSinbad, "Zip", "General" );
        }

    public:
        MeshLodV2GraphicsSystem( GameState *gameState ) : GraphicsSystem( gameState ) {}
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState, LogicSystem **outLogicSystem )
    {
        MeshLodV2GameState *gfxGameState = new MeshLodV2GameState(
            "Shows how to automatically generate LODs directly against a v2 Mesh,\n"
            "compared side-by-side for two different sources:\n"
            "  - A procedurally-built sphere: never touches v1 at all.\n"
            "  - Sinbad: loaded as v1 (the only format the asset ships in), imported\n"
            "    to v2 with NO LOD baked in during that import, then LOD-generated\n"
            "    directly against the v2 result -- unlike the original MeshLod\n"
            "    sample, which generates LOD on the v1 mesh BEFORE importing it.\n"
            "Both use the same native v2 MeshLodGenerator path once their v2 Mesh\n"
            "exists; only how that v2 Mesh first came to exist differs.\n"
            "Fly away to see both switch LOD levels (logged to console on change).\n"
            "Press F2 to toggle wireframe on both and see the triangle count drop." );

        GraphicsSystem *graphicsSystem = new MeshLodV2GraphicsSystem( gfxGameState );

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
        return "Automatic LOD Generation Sample (v2-native)";
    }
}  // namespace Demo