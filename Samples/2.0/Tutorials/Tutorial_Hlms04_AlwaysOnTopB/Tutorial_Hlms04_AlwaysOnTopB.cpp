
#include "Tutorial_Hlms04_AlwaysOnTopB.h"

#include "Tutorial_Hlms04_AlwaysOnTopBGameState.h"

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
        Hlms04AlwaysOnTopBGameState *gfxGameState = new Hlms04AlwaysOnTopBGameState(
            "Shows how to keep an object rendered always on top using an HLMS customization, a cloned\n"
            "Item and advanced depth functionality.\n"
            "This is very useful in e.g. First Person Shooters, because they must always show the \n"
            "hand and weapon always on top and never clip through walls.\n"
            "\n"
            "Unlike Tutorial_Hlms04_AlwaysOnTopB, this method *always* guarantees the object will be on "
            "top.\n"
            "This technique involves the following steps:\n"
            " 1. Setup a material (called 'ClearDepth') with channel_mask set to 0 and depth function "
            "set to 'always pass'\n"
            " 2. Create a clone of the Item at the exact same location.\n"
            " 3. Disable unwanted functionality on the clone, such as shadow casting.\n"
            " 4. The Clone must be drawn after the objects we want be on top. The Original must be "
            "drawn after the clone.\n"
            " 5. Use Hlms customization to have the clone zero-out the depth buffer.\n"
            "\n"
            "This means the object will be rendered twice: Once to clear the depth it's supposed to\n"
            "occupy and a 2nd time to perform the actual drawing.\n"
            "Compared to the previous tutorial, this technique:\n\n"
            " - Requires more setup.\n"
            " - 100% guarantees the object is always rendered on top.\n"
            " - Is a bit more expensive.\n"
            " - Can be modified so that other objects are rendered afterwards (meaning that your\n"
            "   object is not universally on top of everything, but rather always on top of only the\n"
            "   objects that preceeded it).\n"
            " - It is relatively mobile friendly but not as good (the depth manipulation may turn "
            "off some optimizations).\n"
            " - The advanced setup (i.e. clone handling) means toggling this feature on/off is harder.\n"
            "\n"
            "This sample depends on the media files:\n"
            "   * Samples/Media/2.0/materials/Tutorial_Hlms04_AlwaysOnTopB\n" );

        GraphicsSystem *graphicsSystem = new Hlms04AlwaysOnTopBGraphicsSystem( gfxGameState );

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

    const char *MainEntryPoints::getWindowTitle() { return "Tutoroial: Hlms03 Always on Top (A)"; }
}  // namespace Demo
