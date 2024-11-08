
#include "Tutorial_Hlms05_CustomizationPerObjData.h"

#include "Tutorial_Hlms05_CustomizationPerObjDataGameState.h"

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
    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState, LogicSystem **outLogicSystem )
    {
        Hlms05CustomizationPerObjDataGameState *gfxGameState =
            new Hlms05CustomizationPerObjDataGameState(
                "Tutorial_Hlms05_CustomizationPerObjData showed how to show send a custom colour.\n"
                "However such sample forces all customized objects to share the same colour\n"
                "(or at most, a limited number of multiple colours).\n"
                "\n"
                "By design OgreNext disallows sending arbitrary per object data because such\n"
                "functionality can easily get expensive, as it must live in a hot path: It happens\n"
                "per object + per pass + per frame.\n"
                "\n"
                "However there are cases where users *want* to apply per-object custom data.\n"
                "This sample shows how to do that. Performance is not really taken into consideration.\n"
                "There may be ways to improve the performance hit depending on what assumptions you\n"
                "can make, but this samples tries to be flexible.\n"
                "\n"
                "The code for this sample was sponsored by Open Robotics.\n"
                "A variation of this implementation can be found at:\n"
                "https://github.com/gazebosim/gz-rendering/blob/gz-rendering8/ogre2/src/"
                "Ogre2GzHlmsPbsPrivate.cc \n"
                "https://github.com/gazebosim/gz-rendering/blob/gz-rendering8/ogre2/src/"
                "Ogre2GzHlmsPbsPrivate.h \n"
                "https://github.com/gazebosim/gz-rendering/blob/gz-rendering8/ogre2/src/"
                "Ogre2GzHlmsSharedPrivate.cc \n"
                "https://github.com/gazebosim/gz-rendering/blob/gz-rendering8/ogre2/src/"
                "Ogre2GzHlmsSharedPrivate.h \n"
                "\n"
                "\n"
                "This sample depends on the media files:\n"
                "   * Samples/Media/2.0/materials/Tutorial_Hlms05_CustomizationPerObjData\n" );

        GraphicsSystem *graphicsSystem = new Hlms05CustomizationPerObjDataGraphicsSystem( gfxGameState );

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

    const char *MainEntryPoints::getWindowTitle() { return "Tutoroial: Hlms02 Customization"; }
}  // namespace Demo
