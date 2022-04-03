
//---------------------------------------------------------------------------------------
// This tutorial shows nothing fancy. Only how to setup Ogre to render to
// a window, without any dependency.
//
// This samples has many hardcoded paths (e.g. it assumes the current working directory has R/W access)
// which means it will work in Windows and Linux, Mac may work.
//
// See the next tutorials on how to handles all OSes and how to properly setup a robust render loop
//---------------------------------------------------------------------------------------

#include "OgreAbiUtils.h"
#include "OgreArchiveManager.h"
#include "OgreCamera.h"
#include "OgreConfigFile.h"
#include "OgreRoot.h"
#include "OgreWindow.h"

#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsUnlit.h"

#include "Compositor/OgreCompositorManager2.h"

#include "OgreWindowEventUtilities.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#    include "OSX/macUtils.h"
#endif
static void registerHlms()
{
    using namespace Ogre;

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    // Note:  macBundlePath works for iOS too. It's misnamed.
    const String resourcePath = Ogre::macResourcesPath();
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
    const String resourcePath = Ogre::macBundlePath() + "/";
#else
    String resourcePath = "";
#endif

    ConfigFile cf;
    cf.load( resourcePath + "resources2.cfg" );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
    String rootHlmsFolder = macBundlePath() + '/' + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#else
    String rootHlmsFolder = resourcePath + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#endif

    if( rootHlmsFolder.empty() )
        rootHlmsFolder = "./";
    else if( *( rootHlmsFolder.end() - 1 ) != '/' )
        rootHlmsFolder += "/";

    // At this point rootHlmsFolder should be a valid path to the Hlms data folder

    HlmsUnlit *hlmsUnlit = 0;
    HlmsPbs *hlmsPbs = 0;

    // For retrieval of the paths to the different folders needed
    String mainFolderPath;
    StringVector libraryFoldersPaths;
    StringVector::const_iterator libraryFolderPathIt;
    StringVector::const_iterator libraryFolderPathEn;

    ArchiveManager &archiveManager = ArchiveManager::getSingleton();

    {
        // Create & Register HlmsUnlit
        // Get the path to all the subdirectories used by HlmsUnlit
        HlmsUnlit::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
        Archive *archiveUnlit =
            archiveManager.load( rootHlmsFolder + mainFolderPath, "FileSystem", true );
        ArchiveVec archiveUnlitLibraryFolders;
        libraryFolderPathIt = libraryFoldersPaths.begin();
        libraryFolderPathEn = libraryFoldersPaths.end();
        while( libraryFolderPathIt != libraryFolderPathEn )
        {
            Archive *archiveLibrary =
                archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, "FileSystem", true );
            archiveUnlitLibraryFolders.push_back( archiveLibrary );
            ++libraryFolderPathIt;
        }

        // Create and register the unlit Hlms
        hlmsUnlit = OGRE_NEW HlmsUnlit( archiveUnlit, &archiveUnlitLibraryFolders );
        Root::getSingleton().getHlmsManager()->registerHlms( hlmsUnlit );
    }

    {
        // Create & Register HlmsPbs
        // Do the same for HlmsPbs:
        HlmsPbs::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
        Archive *archivePbs = archiveManager.load( rootHlmsFolder + mainFolderPath, "FileSystem", true );

        // Get the library archive(s)
        ArchiveVec archivePbsLibraryFolders;
        libraryFolderPathIt = libraryFoldersPaths.begin();
        libraryFolderPathEn = libraryFoldersPaths.end();
        while( libraryFolderPathIt != libraryFolderPathEn )
        {
            Archive *archiveLibrary =
                archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, "FileSystem", true );
            archivePbsLibraryFolders.push_back( archiveLibrary );
            ++libraryFolderPathIt;
        }

        // Create and register
        hlmsPbs = OGRE_NEW HlmsPbs( archivePbs, &archivePbsLibraryFolders );
        Root::getSingleton().getHlmsManager()->registerHlms( hlmsPbs );
    }

    RenderSystem *renderSystem = Root::getSingletonPtr()->getRenderSystem();
    if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
    {
        // Set lower limits 512kb instead of the default 4MB per Hlms in D3D 11.0
        // and below to avoid saturating AMD's discard limit (8MB) or
        // saturate the PCIE bus in some low end machines.
        bool supportsNoOverwriteOnTextureBuffers;
        renderSystem->getCustomAttribute( "MapNoOverwriteOnDynamicBufferSRV",
                                          &supportsNoOverwriteOnTextureBuffers );

        if( !supportsNoOverwriteOnTextureBuffers )
        {
            hlmsPbs->setTextureBufferDefaultSize( 512 * 1024 );
            hlmsUnlit->setTextureBufferDefaultSize( 512 * 1024 );
        }
    }
}

class MyWindowEventListener final : public Ogre::WindowEventListener
{
    bool mQuit;

public:
    MyWindowEventListener() : mQuit( false ) {}
    void windowClosed( Ogre::Window *rw ) override { mQuit = true; }

    bool getQuit() const { return mQuit; }
};

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#else
int main( int argc, const char *argv[] )
#endif
{
    using namespace Ogre;

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    const String pluginsFolder = macResourcesPath();
    const String writeAccessFolder = macLogPath();
#else
    const String pluginsFolder = "./";
    const String writeAccessFolder = pluginsFolder;
#endif

#ifndef OGRE_STATIC_LIB
#    if OGRE_DEBUG_MODE && \
        !( ( OGRE_PLATFORM == OGRE_PLATFORM_APPLE ) || ( OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS ) )
    const char *pluginsFile = "plugins_d.cfg";
#    else
    const char *pluginsFile = "plugins.cfg";
#    endif
#else
    const char *pluginsFile = 0;  // TODO
#endif

    const Ogre::AbiCookie abiCookie = Ogre::generateAbiCookie();
    Root *root = OGRE_NEW Root( &abiCookie, pluginsFolder + pluginsFile,  //
                                writeAccessFolder + "ogre.cfg",           //
                                writeAccessFolder + "Ogre.log" );

    if( !root->showConfigDialog() )
        return -1;

    // Initialize Root
    root->getRenderSystem()->setConfigOption( "sRGB Gamma Conversion", "Yes" );
    Window *window = root->initialise( true, "Tutorial 00: Basic" );

    registerHlms();

    // Create SceneManager
    const size_t numThreads = 1u;
    SceneManager *sceneManager = root->createSceneManager( ST_GENERIC, numThreads, "ExampleSMInstance" );

    // Create & setup camera
    Camera *camera = sceneManager->createCamera( "Main Camera" );

    // Position it at 500 in Z direction
    camera->setPosition( Vector3( 0, 5, 15 ) );
    // Look back along -Z
    camera->lookAt( Vector3( 0, 0, 0 ) );
    camera->setNearClipDistance( 0.2f );
    camera->setFarClipDistance( 1000.0f );
    camera->setAutoAspectRatio( true );

    // Setup a basic compositor with a blue clear colour
    CompositorManager2 *compositorManager = root->getCompositorManager2();
    const String workspaceName( "Demo Workspace" );
    const ColourValue backgroundColour( 0.2f, 0.4f, 0.6f );
    compositorManager->createBasicWorkspaceDef( workspaceName, backgroundColour, IdString() );
    compositorManager->addWorkspace( sceneManager, window->getTexture(), camera, workspaceName, true );

    MyWindowEventListener myWindowEventListener;
    WindowEventUtilities::addWindowEventListener( window, &myWindowEventListener );

    bool bQuit = false;

    while( !bQuit )
    {
        WindowEventUtilities::messagePump();
        bQuit |= myWindowEventListener.getQuit();
        if( !bQuit )
            bQuit |= !root->renderOneFrame();
    }

    WindowEventUtilities::removeWindowEventListener( window, &myWindowEventListener );

    OGRE_DELETE root;
    root = 0;

    return 0;
}
