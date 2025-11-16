
#include "ImguiGameState.h"

#include "CameraController.h"
#include "GraphicsSystem.h"
#include "ImguiDemo.h"
#include "OgreCamera.h"
#include "OgreImguiManager.h"
#include "OgreItem.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreSceneManager.h"

#include "imgui.h"

#if OGRE_USE_SDL2
#    include "backends/imgui_impl_sdl2.h"
#endif

using namespace Demo;

ImguiGameState::ImguiGameState( const Ogre::String &helpDescription ) :
    TutorialGameState( helpDescription )
{
}
//-----------------------------------------------------------------------------
void ImguiGameState::createScene01()
{
    Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

    Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane(
        "Plane v1", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        Ogre::Plane( Ogre::Vector3::UNIT_Y, 1.0f ), 50.0f, 50.0f, 1, 1, true, 1, 4.0f, 4.0f,
        Ogre::Vector3::UNIT_Z, Ogre::v1::HardwareBuffer::HBU_STATIC,
        Ogre::v1::HardwareBuffer::HBU_STATIC );

    Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createByImportingV1(
        "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, planeMeshV1.get(), true, true,
        true );

    {
        Ogre::Item *item = sceneManager->createItem( planeMesh, Ogre::SCENE_DYNAMIC );
        Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                                         ->createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->setPosition( 0, -1, 0 );
        sceneNode->attachObject( item );
    }

    Ogre::SceneNode *rootNode = sceneManager->getRootSceneNode();

    Ogre::Light *light = sceneManager->createLight();
    Ogre::SceneNode *lightNode = rootNode->createChildSceneNode();
    lightNode->attachObject( light );
    light->setPowerScale( Ogre::Math::PI );
    light->setType( Ogre::Light::LT_DIRECTIONAL );
    light->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );

    sceneManager->setAmbientLight( Ogre::ColourValue( 0.3f, 0.5f, 0.7f ) * 0.1f * 0.75f,
                                   Ogre::ColourValue( 0.6f, 0.45f, 0.3f ) * 0.065f * 0.75f,
                                   -light->getDirection() + Ogre::Vector3::UNIT_Y * 0.2f );

    mGraphicsSystem->getCamera()->setPosition( Ogre::Vector3( 0.0f, 40.0f, 120.0f ) );

    mCameraController = new CameraController( mGraphicsSystem, false );

    TutorialGameState::createScene01();
}
//-----------------------------------------------------------------------------
void ImguiGameState::update( float timeSinceLast )
{
    ImGuiIO &io = ImGui::GetIO();
    io.DeltaTime = timeSinceLast;
    ImGui::NewFrame();
#if OGRE_USE_SDL2
    ImGui_ImplSDL2_NewFrame();
#endif

    // Begin issuing imgui draw calls.
    // Don't do this in the frameRenderingQueued callback,
    // as if any of your logic alters Ogre state you will break things.
    bool show_demo_window = true;
    ImGui::ShowDemoWindow( &show_demo_window );

    OGRE_ASSERT_HIGH( dynamic_cast<ImguiGraphicsSystem *>( mGraphicsSystem ) );
    Ogre::ImguiManager *imguiManger =
        static_cast<ImguiGraphicsSystem *>( mGraphicsSystem )->getImguiManager();
    imguiManger->prepareForRender( mGraphicsSystem->getSceneManager() );

    TutorialGameState::update( timeSinceLast );
}
//-----------------------------------------------------------------------------
void ImguiGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
{
    TutorialGameState::generateDebugText( timeSinceLast, outText );
}
//-----------------------------------------------------------------------------
void ImguiGameState::mouseMoved( const SDL_Event &arg )
{
#if !OGRE_USE_SDL2
    ImGuiIO &io = ImGui::GetIO();
    if( arg.type == SDL_MOUSEMOTION )
        io.MousePos = ImVec2( float( arg.motion.x ), float( arg.motion.y ) );
    if( arg.type == SDL_MOUSEWHEEL )
    {
        io.MouseWheel = float( arg.wheel.y );
        io.MouseWheelH = float( arg.wheel.x );
    }
#endif
    // TutorialGameState::mouseMoved( arg );
}
//-----------------------------------------------------------------------------
void ImguiGameState::mousePressed( const SDL_MouseButtonEvent &arg, Ogre::uint8 id )
{
#if !OGRE_USE_SDL2
    ImGuiIO &io = ImGui::GetIO();
    io.MouseDown[arg.button == SDL_BUTTON_LEFT ? 0 : 1] = true;
#endif
}
//-----------------------------------------------------------------------------
void ImguiGameState::mouseReleased( const SDL_MouseButtonEvent &arg, Ogre::uint8 id )
{
#if !OGRE_USE_SDL2
    ImGuiIO &io = ImGui::GetIO();
    io.MouseDown[arg.button == SDL_BUTTON_LEFT ? 0 : 1] = false;
#endif
}
//-----------------------------------------------------------------------------
void ImguiGameState::textInput( const SDL_TextInputEvent &arg )
{
#if !OGRE_USE_SDL2
    ImGuiIO &io = ImGui::GetIO();
    io.AddInputCharactersUTF8( arg.text );
#endif
}
//-----------------------------------------------------------------------------
void ImguiGameState::keyEvent( const SDL_KeyboardEvent &arg, bool keyPressed )
{
#if !OGRE_USE_SDL2
    ImGuiIO &io = ImGui::GetIO();

    io.KeyShift = ( ( arg.keysym.mod & KMOD_SHIFT ) != 0 );
    io.KeyCtrl = ( ( arg.keysym.mod & KMOD_CTRL ) != 0 );
    io.KeyAlt = ( ( arg.keysym.mod & KMOD_ALT ) != 0 );
    io.KeySuper = ( ( arg.keysym.mod & KMOD_GUI ) != 0 );

    ImGuiKey translatedKey = ImGuiKey_LeftArrow;
    // TODO. e.g. something like this:
    // clang-format off
    ImGuiKey ToImGuiKey(SDL_Keycode kc)
    {
        switch (kc)
        {
        case SDLK_TAB:        return ImGuiKey_Tab;
        case SDLK_LEFT:       return ImGuiKey_LeftArrow;
        case SDLK_RIGHT:      return ImGuiKey_RightArrow;
        case SDLK_UP:         return ImGuiKey_UpArrow;
        case SDLK_DOWN:       return ImGuiKey_DownArrow;
        case SDLK_PAGEUP:     return ImGuiKey_PageUp;
        case SDLK_PAGEDOWN:   return ImGuiKey_PageDown;
        case SDLK_HOME:       return ImGuiKey_Home;
        case SDLK_END:        return ImGuiKey_End;
        case SDLK_INSERT:     return ImGuiKey_Insert;
        case SDLK_DELETE:     return ImGuiKey_Delete;
        case SDLK_BACKSPACE:  return ImGuiKey_Backspace;
        case SDLK_SPACE:      return ImGuiKey_Space;
        case SDLK_RETURN:     return ImGuiKey_Enter;
        case SDLK_ESCAPE:     return ImGuiKey_Escape;
        case SDLK_a:          return ImGuiKey_A;
        case SDLK_c:          return ImGuiKey_C;
        case SDLK_v:          return ImGuiKey_V;
        case SDLK_x:          return ImGuiKey_X;
        case SDLK_y:          return ImGuiKey_Y;
        case SDLK_z:          return ImGuiKey_Z;

        default:              return ImGuiKey_None;
        }
    }
    // clang-format on

    io.AddKeyEvent( ToImGuiKey( arg.keysym.sym ), keyPressed );
#endif
}
//-----------------------------------------------------------------------------
void ImguiGameState::keyPressed( const SDL_KeyboardEvent &arg )
{
    keyEvent( arg, true );
}
//-----------------------------------------------------------------------------
void ImguiGameState::keyReleased( const SDL_KeyboardEvent &arg )
{
    if( ( arg.keysym.mod & ~( KMOD_NUM | KMOD_CAPS | KMOD_LSHIFT | KMOD_RSHIFT ) ) != 0 )
    {
        TutorialGameState::keyReleased( arg );
        return;
    }
    keyEvent( arg, false );
    TutorialGameState::keyReleased( arg );
}
