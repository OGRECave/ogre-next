
#include "GraphicsSystem.h"
#include "SdlInputHandler.h"
#include "Tutorial_SSAOGameState.h"

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
    class Tutorial_SSAOGraphicsSystem final : public GraphicsSystem
    {
        Ogre::CompositorWorkspace *setupCompositor() override
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            return compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(), mCamera,
                                                    "SSAOWorkspace", true );
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

            const char *c_locations[5] = { "2.0/scripts/materials/Tutorial_SSAO",
                                           "2.0/scripts/materials/Tutorial_SSAO/GLSL",
                                           "2.0/scripts/materials/Tutorial_SSAO/HLSL",
                                           "2.0/scripts/materials/Tutorial_SSAO/Metal",
                                           "2.0/scripts/materials/PbsMaterials" };

            for( size_t i = 0; i < 5; ++i )
            {
                Ogre::String dataFolderFull = dataFolder + c_locations[i];
                addResourceLocation( dataFolderFull, getMediaReadArchiveType(), "General" );
            }
        }

    public:
        Tutorial_SSAOGraphicsSystem( GameState *gameState ) : GraphicsSystem( gameState ) {}
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState, LogicSystem **outLogicSystem )
    {
        Tutorial_SSAOGameState *gfxGameState = new Tutorial_SSAOGameState(
            "This tutorial shows how to make depth-only horizontal based amibent occlusion (HBAO).\n"
            "The sample uses the compositor feature 'quad_normals camera_far_corners_view_space'\n"
            "in combination with a special shader so that we can unproject the depth sampled\n"
            "from the depth buffer and combine it with the frustum corners to get the view space\n"
            "projection.\n"
            "There is also example in the shader how to reconstruct view space normals from fragment "
            "position.\n"
            "Normals are used in SSAO to rotate hemisphere sample kernel pointing outwards\n"
            "from fragment's face.\n"
            "There are three steps: calculate occlusion (SSAO_HS_ps.glsl/hlsl),\n"
            "blur (SSAO_BlurV/H_ps.glsl/hlsl) and apply (SSAO_Apply_ps.glsl/hlsl).\n" );

        GraphicsSystem *graphicsSystem = new Tutorial_SSAOGraphicsSystem( gfxGameState );

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

    const char *MainEntryPoints::getWindowTitle() { return "SSAO Demo"; }
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
