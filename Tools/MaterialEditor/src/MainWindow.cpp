#include "MainWindow.h"

#include "CmdSettings.h"
#include "Constants.h"
#include "Core/wxOgreRenderWindow.h"
#include "DatablockList.h"
#include "LightPanel.h"
#include "MeshList.h"
#include "PbsParametersPanel.h"
#include "PbsTexturePanel.h"
#include "ProjectSettings.h"

#include <wx/aui/aui.h>
#include <wx/wx.h>

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/Pass/OgreCompositorPassProvider.h"
#include "OgreAbiUtils.h"
#include "OgreHlms.h"
#include "OgreHlmsCompute.h"
#include "OgreHlmsDiskCache.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsUnlit.h"
#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgrePlatformInformation.h"

#include "OgreRoot.h"
#include "OgreSubMesh2.h"
#include "OgreWindow.h"

#include <fstream>

namespace Ogre
{
    // Dummy class to prevent crashes when loading Compositor scripts.
    class DummyCompositorPassProvider final : public CompositorPassProvider
    {
        CompositorPassDef *addPassDef( CompositorPassType passType, IdString customId,
                                       CompositorTargetDef *parentTargetDef,
                                       CompositorNodeDef *parentNodeDef ) override
        {
            return new CompositorPassDef( passType, parentTargetDef );
        }

        CompositorPass *addPass( const CompositorPassDef *definition, Camera *defaultCamera,
                                 CompositorNode *parentNode, const RenderTargetViewDef *rtvDef,
                                 SceneManager *sceneManager ) override
        {
            return nullptr;
        }
    };
}  // namespace Ogre

const Ogre::Quaternion kCoordConventions[CoordinateConvention::NumCoordinateConventions] = {
    Ogre::Quaternion( Ogre::Degree( -90.0f ), Ogre::Vector3::UNIT_Z ),  //
    Ogre::Quaternion::IDENTITY,                                         //
    Ogre::Quaternion( Ogre::Degree( 90.0f ), Ogre::Vector3::UNIT_X )
};

MainWindow::MainWindow( wxWindow *parent, const CmdSettings &cmdSettings ) :
    MainWindowBase( parent ),
    m_root( 0 ),
    m_sceneManager( 0 ),
    m_camera( 0 ),
    m_cameraNode( 0 ),
    m_workspace( 0 ),
    m_wxOgreRenderWindow( 0 ),
    m_undoSystem( this ),
    m_wxAuiManager( 0 ),
    m_mainNotebook( 0 ),
    m_mainNotebook2( 0 ),
    m_pbsParametersPanel( 0 ),
    m_pbsTexturePanel( 0 ),
    m_lightPanel( 0 ),
    m_datablockList( 0 ),
    m_meshList( 0 ),
    m_projectSettings( 0 ),
    m_activeDatablock( 0 ),
    m_activeItem( 0 ),
    m_activeEntity( 0 ),
    m_objSceneNode( 0 ),
    m_mouseX( 0 ),
    m_mouseY( 0 ),
    m_wasLeftPressed( false ),
    m_wasRightPressed( false ),
    m_useMicrocodeCache( true ),
    m_useHlmsDiskCache( true )
{
#ifndef __WXMSW__
    // Set config directory to user home directory.
    m_writeAccessFolder = std::string( wxGetHomeDir().mb_str() ) + "/.config/ogre-next-material-editor/";
    {
        wxString configDir( m_writeAccessFolder.c_str(), wxConvUTF8 );
        if( !wxDirExists( configDir ) )
        {
            if( !wxMkdir( configDir ) )
            {
                wxMessageBox( wxT( "Warning, no R/W access to " ) + configDir +
                                  wxT( "\nMaterial Editor may not function properly or crash." ),
                              wxT( "ACCESS ERROR" ), wxOK | wxICON_ERROR | wxCENTRE );
                m_writeAccessFolder = "";
            }
        }
        else
        {
            // Folder already exists, but we don't own it
            if( !wxIsReadable( configDir ) || !wxIsWritable( configDir ) )
            {
                wxMessageBox( wxT( "Warning, no R/W access to " ) + configDir +
                                  wxT( "\nMaterial Editor may not function properly or crash." ),
                              wxT( "ACCESS ERROR" ), wxOK | wxICON_ERROR | wxCENTRE );
            }
        }
    }
#else
    // TODO: Use wxStandardPaths::GetUserConfigDir()
    // Windows: use User/AppData
    TCHAR path[MAX_PATH];
    if( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path ) != S_OK ) )
    {
        // Need to convert to OEM codepage so that fstream can
        // use it properly on international systems.
#    if defined( _UNICODE ) || defined( UNICODE )
        int size_needed =
            WideCharToMultiByte( CP_OEMCP, 0, path, (int)wcslen( path ), NULL, 0, NULL, NULL );
        m_writeAccessFolder = std::string( size_needed, 0 );
        WideCharToMultiByte( CP_OEMCP, 0, path, (int)wcslen( path ), &m_writeAccessFolder[0],
                             size_needed, NULL, NULL );
#    else
        TCHAR oemPath[MAX_PATH];
        CharToOem( path, oemPath );
        m_writeAccessFolder = std::string( oemPath );
#    endif
        m_writeAccessFolder += "\\ogre-next-material-editor\\";

        // Attempt to create directory where config files go
        if( !CreateDirectoryA( m_writeAccessFolder.c_str(), NULL ) &&
            GetLastError() != ERROR_ALREADY_EXISTS )
        {
            // Couldn't create directory (no write access?),
            // fall back to current working dir
            m_writeAccessFolder = "";
        }
    }
#endif

    //	SetIcon( wxIcon(wxT("Resources/OgreIcon.ico")) );

    // Create the Advanced UI system to handle dockable windows.
    m_wxAuiManager = new wxAuiManager( this );

    // Initialize Ogre and the control that renders it.
    initOgre( cmdSettings.setupRenderSystems );

    m_mainNotebook = new wxAuiNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        wxAUI_NB_BOTTOM | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE |
                                            wxAUI_NB_SCROLL_BUTTONS | wxAUI_NB_TAB_EXTERNAL_MOVE );
    m_mainNotebook2 = new wxAuiNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                         wxAUI_NB_BOTTOM | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE |
                                             wxAUI_NB_SCROLL_BUTTONS | wxAUI_NB_TAB_EXTERNAL_MOVE );
    m_pbsParametersPanel = new PbsParametersPanel( this );
    m_pbsTexturePanel = new PbsTexturePanel( this );
    m_lightPanel = new LightPanel( this );
    m_datablockList = new DatablockList( this );
    m_meshList = new MeshList( this );
    m_projectSettings = new ProjectSettings( this );
    m_projectSettings->loadInternalSettings( m_writeAccessFolder );

    m_mainNotebook->AddPage( m_pbsParametersPanel, wxT( "PBS Settings" ) );
    m_mainNotebook->AddPage( m_pbsTexturePanel, wxT( "PBS Textures" ) );
    m_mainNotebook->AddPage( m_lightPanel, wxT( "Lights" ) );
    m_mainNotebook2->AddPage( m_datablockList, wxT( "Materials" ) );
    m_mainNotebook2->AddPage( m_meshList, wxT( "Meshes" ) );

    m_wxAuiManager->AddPane( m_mainNotebook, wxAuiPaneInfo()
                                                 .Name( wxT( "TabsPane" ) )
                                                 .Caption( wxT( "Tabs" ) )
                                                 .MinSize( 300, -1 )
                                                 .Left()
                                                 .Layer( 1 )
                                                 .CloseButton( false )
                                                 .PaneBorder( false ) );
    m_wxAuiManager->AddPane( m_mainNotebook2, wxAuiPaneInfo()
                                                  .Name( wxT( "TabsPane2" ) )
                                                  .Caption( wxT( "Tabs2" ) )
                                                  .MinSize( 300, -1 )
                                                  .Right()
                                                  .Layer( 1 )
                                                  .CloseButton( false )
                                                  .PaneBorder( false ) );

    // Commit changes made to the AUI manager.
    m_wxAuiManager->Update();

    m_mainNotebook2->Split( 0, wxBOTTOM );

    loadSettings();

    setCoordinateConvention( CoordinateConvention::yUp );

    // Ogre::HlmsManager *hlmsManager = m_root->getHlmsManager();
    // setActiveDatablock( hlmsManager->getHlms( Ogre::HLMS_PBS )
    //                         ->createDatablock( "Test", "Test", Ogre::HlmsMacroblock(),
    //                                            Ogre::HlmsBlendblock(), Ogre::HlmsParamVec() ) );
}
//-----------------------------------------------------------------------------
MainWindow::~MainWindow()
{
    m_projectSettings->saveInternalSettings( m_writeAccessFolder );
    saveHlmsDiskCache();

    if( m_wxOgreRenderWindow )
    {
        m_wxOgreRenderWindow->Destroy();
        m_wxOgreRenderWindow = 0;
    }

    if( m_root )
    {
        Ogre::CompositorManager2 *compositorManager = m_root->getCompositorManager2();
        delete compositorManager->getCompositorPassProvider();
        compositorManager->setCompositorPassProvider( nullptr );

        delete m_root;
        m_root = 0;
    }

    if( m_wxAuiManager )
    {
        m_wxAuiManager->UnInit();
        delete m_wxAuiManager;
        m_wxAuiManager = 0;
    }
}
//-----------------------------------------------------------------------------
void MainWindow::loadSettings()
{
    // Load wxAUI layout
    std::ifstream myFile( ( m_writeAccessFolder + c_layoutSettingsFile ).c_str(),
                          std::ios_base::in | std::ios_base::ate | std::ios_base::binary );
    if( myFile.is_open() )
    {
        std::string layoutString;
        layoutString.resize( size_t( myFile.tellg() ) );
        myFile.seekg( 0 );
        myFile.read( &layoutString[0], std::streamsize( layoutString.size() ) );
        m_wxAuiManager->LoadPerspective( wxString( layoutString.c_str(), wxConvUTF8 ), true );
        myFile.close();
    }
}
//-----------------------------------------------------------------------------
void MainWindow::initOgre( bool bForceSetup )
{
#ifndef __WXMSW__
    // Set config directory.
    const std::string c_pluginsCfg = "";
#else
    const std::string c_pluginsCfg = "";
#endif

#if OGRE_DEBUG_MODE && \
    !( ( OGRE_PLATFORM == OGRE_PLATFORM_APPLE ) || ( OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS ) )
    const Ogre::String pluginsFile = "plugins_d.cfg";
#else
    const Ogre::String pluginsFile = "plugins.cfg";
#endif

    {
        const Ogre::AbiCookie abiCookie = Ogre::generateAbiCookie();
        m_root = new Ogre::Root( &abiCookie, c_pluginsCfg + pluginsFile,
                                 m_writeAccessFolder + "ogre.cfg", m_writeAccessFolder + "Ogre.log" );
    }

    if( bForceSetup || !m_root->restoreConfig() )
        m_root->showConfigDialog();

    m_root->initialise( false );

    wxOgreRenderWindow::SetOgreRoot( m_root );
    m_wxOgreRenderWindow = new wxOgreRenderWindow( this, wxID_ANY );
    m_wxOgreRenderWindow->Show();
    m_wxOgreRenderWindow->setRenderWindowListener( this );

    m_wxOgreRenderWindow->SetFocus();

    createSystems();

    m_wxAuiManager->AddPane( m_wxOgreRenderWindow, wxAuiPaneInfo()
                                                       .Name( wxT( "RenderWindow" ) )
                                                       .Caption( wxT( "OGRE Render Window" ) )
                                                       .CenterPane()
                                                       .PaneBorder( false )
                                                       .MinSize( 256, 256 )
                                                       .CloseButton( false ) );
}
//-----------------------------------------------------------------------------
void MainWindow::createSystems()
{
    m_sceneManager = m_root->createSceneManager( Ogre::ST_GENERIC, 1u );
    m_camera = m_sceneManager->createCamera( "Main Camera" );
    m_camera->setPosition( Ogre::Vector3( 0.0f, 0.0f, 1.25f ) );
    m_camera->setOrientation( Ogre::Quaternion::IDENTITY );
    m_camera->setNearClipDistance( 0.02f );
    m_camera->setFarClipDistance( 1000.0f );
    m_camera->setAutoAspectRatio( true );

    m_cameraNode = m_sceneManager->getRootSceneNode()->createChildSceneNode();
    m_cameraNode = m_cameraNode->createChildSceneNode();
    m_camera->detachFromParent();
    m_cameraNode->attachObject( m_camera );
    m_cameraNode->pitch( Ogre::Degree( -45.0f ) );

    m_objSceneNode = m_sceneManager->getRootSceneNode()->createChildSceneNode();

    m_root->addFrameListener( this );

    m_root->clearEventTimes();

    m_sceneManager->setAmbientLight( Ogre::ColourValue( 0.2f, 0.2f, 0.2f ),
                                     Ogre::ColourValue( 0.1f, 0.1f, 0.1f ), Ogre::Vector3::UNIT_Y );

    loadResources();

    Ogre::CompositorManager2 *compositorManager = m_root->getCompositorManager2();
    compositorManager->setCompositorPassProvider( new Ogre::DummyCompositorPassProvider() );

    /*const Ogre::String workspaceName( "MaterialEditor Workspace" );
    if( !compositorManager->hasWorkspaceDefinition( workspaceName ) )
    {
        compositorManager->createBasicWorkspaceDef( workspaceName, Ogre::ColourValue( 0.2f, 0.4f, 0.6f ),
                                                    Ogre::IdString() );
    }*/
    const Ogre::String workspaceName( "HdrWorkspaceMsaa" );

    m_workspace = compositorManager->addWorkspace( m_sceneManager,
                                                   m_wxOgreRenderWindow->GetRenderWindow()->getTexture(),
                                                   m_camera, workspaceName, true );
}
//-----------------------------------------------------------------------------
void MainWindow::addResourceLocation( const Ogre::String &archName, const Ogre::String &typeName,
                                      const Ogre::String &secName )
{
#if( OGRE_PLATFORM == OGRE_PLATFORM_APPLE ) || ( OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS )
    // OS X does not set the working directory relative to the app,
    // In order to make things portable on OS X we need to provide
    // the loading with it's own bundle path location
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
        Ogre::String( Ogre::macBundlePath() + "/" + archName ), typeName, secName );
#else
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation( archName, typeName, secName );
#endif
}
//-----------------------------------------------------------------------------
void MainWindow::loadResources()
{
    // Load resource paths from config file
    Ogre::ConfigFile cf;
    cf.load( "./resources2.cfg" );

    Ogre::String originalDataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
    if( !originalDataFolder.empty() && *( originalDataFolder.end() - 1 ) != '/' )
        originalDataFolder += "/";

    const char *c_locations[] = {
        "materials/textures/Cubemaps",       "2.0/scripts/materials/Common",
        "2.0/scripts/materials/Common/Any",  "2.0/scripts/materials/Common/GLSL",
        "2.0/scripts/materials/Common/HLSL", "2.0/scripts/materials/Common/Metal",
        "2.0/scripts/Compositors",           "2.0/scripts/materials/HDR",
        "2.0/scripts/materials/HDR/GLSL",    "2.0/scripts/materials/HDR/HLSL",
        "2.0/scripts/materials/HDR/Metal"
    };

    Ogre::ResourceGroupManager &resourceGroupManager = Ogre::ResourceGroupManager::getSingleton();

    for( size_t i = 0u; i < sizeof( c_locations ) / sizeof( c_locations[0] ); ++i )
    {
        resourceGroupManager.addResourceLocation(
            originalDataFolder + c_locations[i], "FileSystem",
            Ogre::ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME );
    }

#if 0
    // Load resource paths from config file
    Ogre::ConfigFile cf;
    cf.load( "./resources2.cfg" );

    // Go through all sections & settings in the file
    Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

    Ogre::String secName, typeName, archName;
    while( seci.hasMoreElements() )
    {
        secName = seci.peekNextKey();
        Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();

        if( secName != "Hlms" )
        {
            Ogre::ConfigFile::SettingsMultiMap::iterator i;
            for( i = settings->begin(); i != settings->end(); ++i )
            {
                typeName = i->first;
                archName = i->second;
                addResourceLocation( archName, typeName, secName );
            }
        }
    }
#endif

    registerHlms();

#ifndef DEBUG
    // Do NOT dump our shaders in the Release build. It's very unprofessional.
    Ogre::HlmsManager *hlmsManager = m_root->getHlmsManager();

    for( size_t i = 0u; i < Ogre::HlmsTypes::HLMS_MAX; ++i )
    {
        Ogre::Hlms *hlms = hlmsManager->getHlms( static_cast<Ogre::HlmsTypes>( i ) );
        if( hlms )
            hlms->setDebugOutputPath( false, false );
    }
    {
        Ogre::Hlms *hlms = hlmsManager->getComputeHlms();
        if( hlms )
            hlms->setDebugOutputPath( false, false );
    }
#endif

    loadHlmsDiskCache();

    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups( true );
}
//-----------------------------------------------------------------------------
void MainWindow::registerHlms()
{
    Ogre::ConfigFile cf;
    cf.load( "./resources2.cfg" );

    Ogre::String resourcePath = "";

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
    Ogre::String rootHlmsFolder =
        Ogre::macBundlePath() + '/' + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#else
    Ogre::String rootHlmsFolder = resourcePath + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#endif

    if( rootHlmsFolder.empty() )
        rootHlmsFolder = "./";
    else if( *( rootHlmsFolder.end() - 1 ) != '/' )
        rootHlmsFolder += "/";

    // At this point rootHlmsFolder should be a valid path to the Hlms data folder

    Ogre::HlmsUnlit *hlmsUnlit = 0;
    Ogre::HlmsPbs *hlmsPbs = 0;

    // For retrieval of the paths to the different folders needed
    Ogre::String mainFolderPath;
    Ogre::StringVector libraryFoldersPaths;
    Ogre::StringVector::const_iterator libraryFolderPathIt;
    Ogre::StringVector::const_iterator libraryFolderPathEn;

    Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();
    Ogre::HlmsManager *hlmsManager = m_root->getHlmsManager();

    {
        // Create & Register HlmsUnlit
        // Get the path to all the subdirectories used by HlmsUnlit
        Ogre::HlmsUnlit::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
        Ogre::Archive *archiveUnlit =
            archiveManager.load( rootHlmsFolder + mainFolderPath, "FileSystem", true );
        Ogre::ArchiveVec archiveUnlitLibraryFolders;
        libraryFolderPathIt = libraryFoldersPaths.begin();
        libraryFolderPathEn = libraryFoldersPaths.end();
        while( libraryFolderPathIt != libraryFolderPathEn )
        {
            Ogre::Archive *archiveLibrary =
                archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, "FileSystem", true );
            archiveUnlitLibraryFolders.push_back( archiveLibrary );
            ++libraryFolderPathIt;
        }

        // Create and register the unlit Hlms
        hlmsUnlit = OGRE_NEW Ogre::HlmsUnlit( archiveUnlit, &archiveUnlitLibraryFolders );
        hlmsManager->registerHlms( hlmsUnlit );
    }

    {
        // Create & Register HlmsPbs
        // Do the same for HlmsPbs:
        Ogre::HlmsPbs::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
        Ogre::Archive *archivePbs =
            archiveManager.load( rootHlmsFolder + mainFolderPath, "FileSystem", true );

        // Get the library archive(s)
        Ogre::ArchiveVec archivePbsLibraryFolders;
        libraryFolderPathIt = libraryFoldersPaths.begin();
        libraryFolderPathEn = libraryFoldersPaths.end();
        while( libraryFolderPathIt != libraryFolderPathEn )
        {
            Ogre::Archive *archiveLibrary =
                archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, "FileSystem", true );
            archivePbsLibraryFolders.push_back( archiveLibrary );
            ++libraryFolderPathIt;
        }

        // Create and register
        hlmsPbs = OGRE_NEW Ogre::HlmsPbs( archivePbs, &archivePbsLibraryFolders );
        hlmsManager->registerHlms( hlmsPbs );
    }

    Ogre::RenderSystem *renderSystem = m_root->getRenderSystem();
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
//-----------------------------------------------------------------------------
void MainWindow::loadHlmsDiskCache()
{
    if( !m_useMicrocodeCache && !m_useHlmsDiskCache )
        return;

    Ogre::HlmsManager *hlmsManager = m_root->getHlmsManager();
    Ogre::HlmsDiskCache diskCache( hlmsManager );

    Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();

    Ogre::Archive *rwAccessFolderArchive =
        archiveManager.load( m_writeAccessFolder, "FileSystem", true );

    if( m_useMicrocodeCache )
    {
        // Make sure the microcode cache is enabled.
        Ogre::GpuProgramManager::getSingleton().setSaveMicrocodesToCache( true );
        const Ogre::String filename = "microcodeCodeCache.cache";
        if( rwAccessFolderArchive->exists( filename ) )
        {
            Ogre::DataStreamPtr shaderCacheFile = rwAccessFolderArchive->open( filename );
            Ogre::GpuProgramManager::getSingleton().loadMicrocodeCache( shaderCacheFile );
        }
    }

    if( m_useHlmsDiskCache )
    {
        const size_t numThreads =
            std::max<size_t>( 1u, Ogre::PlatformInformation::getNumLogicalCores() );

        for( size_t i = Ogre::HLMS_LOW_LEVEL + 1u; i < Ogre::HLMS_MAX; ++i )
        {
            Ogre::Hlms *hlms = hlmsManager->getHlms( static_cast<Ogre::HlmsTypes>( i ) );
            if( hlms )
            {
                Ogre::String filename = "hlmsDiskCache" + Ogre::StringConverter::toString( i ) + ".bin";

                try
                {
                    if( rwAccessFolderArchive->exists( filename ) )
                    {
                        Ogre::DataStreamPtr diskCacheFile = rwAccessFolderArchive->open( filename );
                        diskCache.loadFrom( diskCacheFile );
                        diskCache.applyTo( hlms, numThreads );
                    }
                }
                catch( Ogre::Exception & )
                {
                    Ogre::LogManager::getSingleton().logMessage(
                        "Error loading cache from " + m_writeAccessFolder + "/" + filename +
                        "! If you have issues, try deleting the file "
                        "and restarting the app" );
                }
            }
        }
    }

    archiveManager.unload( m_writeAccessFolder );
}
//-----------------------------------------------------------------------------
void MainWindow::saveHlmsDiskCache()
{
    if( m_root && m_root->getRenderSystem() && Ogre::GpuProgramManager::getSingletonPtr() &&
        ( m_useMicrocodeCache || m_useHlmsDiskCache ) )
    {
        Ogre::HlmsManager *hlmsManager = m_root->getHlmsManager();
        Ogre::HlmsDiskCache diskCache( hlmsManager );

        Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();

        Ogre::Archive *rwAccessFolderArchive =
            archiveManager.load( m_writeAccessFolder, "FileSystem", false );

        if( m_useHlmsDiskCache )
        {
            for( size_t i = Ogre::HLMS_LOW_LEVEL + 1u; i < Ogre::HLMS_MAX; ++i )
            {
                Ogre::Hlms *hlms = hlmsManager->getHlms( static_cast<Ogre::HlmsTypes>( i ) );
                if( hlms )
                {
                    diskCache.copyFrom( hlms );

                    Ogre::DataStreamPtr diskCacheFile = rwAccessFolderArchive->create(
                        "hlmsDiskCache" + Ogre::StringConverter::toString( i ) + ".bin" );
                    diskCache.saveTo( diskCacheFile );
                }
            }
        }

        if( Ogre::GpuProgramManager::getSingleton().isCacheDirty() && m_useMicrocodeCache )
        {
            const Ogre::String filename = "microcodeCodeCache.cache";
            Ogre::DataStreamPtr shaderCacheFile = rwAccessFolderArchive->create( filename );
            Ogre::GpuProgramManager::getSingleton().saveMicrocodeCache( shaderCacheFile );
        }

        archiveManager.unload( m_writeAccessFolder );
    }
}
//-----------------------------------------------------------------------------
bool MainWindow::frameStarted( const Ogre::FrameEvent &evt )
{
    m_pbsParametersPanel->syncDatablockFromUI();
    return true;
}
//-----------------------------------------------------------------------------
void MainWindow::setCoordinateConvention( CoordinateConvention::CoordinateConvention newConvention )
{
    m_coordinateConvention = newConvention;
    m_cameraNode->getParent()->setOrientation( kCoordConventions[newConvention] );
    m_lightPanel->setCoordinateConvention( newConvention );

    m_sceneManager->setAmbientLight(
        m_sceneManager->getAmbientLightUpperHemisphere(),
        m_sceneManager->getAmbientLightLowerHemisphere(), kCoordConventions[newConvention].yAxis(),
        m_sceneManager->getAmbientLightUpperHemisphere().a, m_sceneManager->getEnvFeatures() );

    m_menuView->Check( wxID_MENUCOORDINATE_X_UP, m_coordinateConvention == CoordinateConvention::xUp );
    m_menuView->Check( wxID_MENUCOORDINATE_Y_UP, m_coordinateConvention == CoordinateConvention::yUp );
    m_menuView->Check( wxID_MENUCOORDINATE_Z_UP, m_coordinateConvention == CoordinateConvention::zUp );
}
//-----------------------------------------------------------------------------
void MainWindow::originCenterYCamera()
{
    const Ogre::MovableObject *object = getActiveObject();
    if( object )
    {
        Ogre::Vector3 vCenter = Ogre::Vector3( 0.0f, object->getLocalAabb().mCenter.y, 0.0f );
        m_camera->setPosition( Ogre::Vector3::UNIT_Z * object->getLocalRadius() * 1.8f );
        m_cameraNode->setOrientation( Ogre::Quaternion::IDENTITY );
        m_cameraNode->pitch( Ogre::Degree( -45.0f ) );
        m_cameraNode->getParent()->setPosition( vCenter );
        m_cameraNode->setPosition( Ogre::Vector3::ZERO );
    }
    else
        originCamera();
}
//-----------------------------------------------------------------------------
void MainWindow::originCamera()
{
    float factor = 10.0f;
    const Ogre::MovableObject *object = getActiveObject();
    if( object )
        factor = object->getLocalRadius() * 1.8f;

    m_camera->setPosition( Ogre::Vector3::UNIT_Z * factor );
    m_cameraNode->setOrientation( Ogre::Quaternion::IDENTITY );
    m_cameraNode->pitch( Ogre::Degree( -45.0f ) );
    m_cameraNode->getParent()->setPosition( Ogre::Vector3::ZERO );
    m_cameraNode->setPosition( Ogre::Vector3::ZERO );
}
//-----------------------------------------------------------------------------
void MainWindow::centerMeshCamera()
{
    const Ogre::MovableObject *object = getActiveObject();
    if( object )
    {
        const Ogre::Vector3 vCenter = object->getLocalAabb().mCenter;
        m_camera->setPosition( Ogre::Vector3::UNIT_Z * object->getLocalRadius() * 1.8f );
        m_cameraNode->setOrientation( Ogre::Quaternion::IDENTITY );
        m_cameraNode->pitch( Ogre::Degree( -45.0f ) );
        m_cameraNode->getParent()->setPosition( vCenter );
        m_cameraNode->setPosition( Ogre::Vector3::ZERO );
    }
}
//-----------------------------------------------------------------------------
void MainWindow::rotateCamera( const int32_t x, const int32_t y )
{
    m_cameraNode->yaw( Ogre::Degree( Ogre::Real( -x ) * 0.4f ), Ogre::Node::TS_PARENT );
    m_cameraNode->pitch( Ogre::Degree( Ogre::Real( -y ) * 0.4f ) );
}
//-----------------------------------------------------------------------------
void MainWindow::zoomInCamera( Ogre::Real wheelDelta )
{
    float factor = 1.0f;
    const Ogre::MovableObject *object = getActiveObject();
    if( object )
        factor = object->getLocalRadius() * 0.5f;

    m_camera->move( Ogre::Vector3::UNIT_Z * wheelDelta * factor );

    // Clamp max zoom in, to keep going use slideCamera
    if( m_camera->getPosition().z < 0.0f )
        m_camera->setPosition( Ogre::Vector3::ZERO );
}
//-----------------------------------------------------------------------------
void MainWindow::moveCamera( int x, int y )
{
    const Ogre::MovableObject *object = getActiveObject();
    if( object )
    {
        Ogre::Vector3 vTrans( Ogre::Real( -x ), Ogre::Real( y ), 0 );
        vTrans *= object->getLocalRadius() * 0.005f;
        m_cameraNode->translate( vTrans, Ogre::Node::TS_LOCAL );
    }
}
//-----------------------------------------------------------------------------
void MainWindow::slideCamera( int x, int z )
{
    const Ogre::MovableObject *object = getActiveObject();
    if( object )
    {
        Ogre::Vector3 vTrans( Ogre::Real( -x ), 0, Ogre::Real( -z ) );
        vTrans *= object->getLocalRadius() * 0.005f;
        m_cameraNode->translate( vTrans, Ogre::Node::TS_LOCAL );
    }
}
//-----------------------------------------------------------------------------
void MainWindow::OnMouseEvents( wxMouseEvent &evt )
{
    evt.Skip();

    const int32_t oldX = m_mouseX;
    const int32_t oldY = m_mouseY;

    if( evt.LeftIsDown() )
    {
        evt.GetPosition( &m_mouseX, &m_mouseY );

        if( m_wasLeftPressed )
        {
            if( !evt.ShiftDown() )
                rotateCamera( m_mouseX - oldX, m_mouseY - oldY );
            else
                moveCamera( m_mouseX - oldX, m_mouseY - oldY );
        }
    }
    else if( evt.RightIsDown() )
    {
        evt.GetPosition( &m_mouseX, &m_mouseY );

        if( m_wasRightPressed )
        {
            if( !evt.ShiftDown() )
                zoomInCamera( Ogre::Real( m_mouseY - oldY ) * 0.05f );
            else
                slideCamera( m_mouseX - oldX, m_mouseY - oldY );
        }
    }

    // TODO: Wheel event seems being missed...
    if( evt.GetWheelRotation() )
        zoomInCamera( -Ogre::Real( evt.GetWheelRotation() ) / Ogre::Real( evt.GetWheelDelta() ) );

    m_wasLeftPressed = evt.LeftIsDown();
    m_wasRightPressed = evt.RightIsDown();

    m_wxOgreRenderWindow->Update();
}
//-----------------------------------------------------------------------------
void MainWindow::OnKeyDown( wxKeyEvent &evt )
{
    evt.Skip();
}
//-----------------------------------------------------------------------------
void MainWindow::OnKeyUp( wxKeyEvent &evt )
{
    evt.Skip();
}
//-----------------------------------------------------------------------------
void MainWindow::OnMenuSelection( wxCommandEvent &event )
{
    switch( event.GetId() )
    {
    case wxID_NEW:
        if( m_projectSettings->ShowModal() == wxID_OK )
        {
            unloadForNewProject();
            m_projectSettings->newProject( m_root->getHlmsManager() );
            Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups( true );
            m_datablockList->populateFromDatabase();
            m_meshList->populateFromDatabase();

            m_pbsTexturePanel->unsetEnvMapFromAllDatablocks();
            m_projectSettings->saveProject( m_root->getHlmsManager(), *m_lightPanel );
            m_pbsTexturePanel->notifyMeshChanged();
        }
        break;
    case wxID_OPEN:
    {
        const wxString projPath = m_projectSettings->openProjectModal();
        if( !projPath.empty() )
        {
            unloadForNewProject();
            m_projectSettings->openProject( projPath, m_root->getHlmsManager(), *m_lightPanel );
            Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups( true );
            m_datablockList->populateFromDatabase();
            m_meshList->populateFromDatabase();
        }
        break;
    }
    case wxID_SAVE:
        m_pbsTexturePanel->unsetEnvMapFromAllDatablocks();
        m_projectSettings->saveProject( m_root->getHlmsManager(), *m_lightPanel );
        m_pbsTexturePanel->notifyMeshChanged();
        break;
    case wxID_PREFERENCES:
        m_projectSettings->ShowModal();
        break;

    case wxID_UNDO:
        m_undoSystem.performUndo();
        break;
    case wxID_REDO:
        m_undoSystem.performRedo();
        break;

        // Submenu Camera
    case wxID_MENUCAMERAORIGIN:
        originCamera();
        break;
    case wxID_MENUCAMCENTERMESH:
        centerMeshCamera();
        break;
    case wxID_MENUCAMERAORIGINCENTERY:
        originCenterYCamera();
        break;
        // Submenu Coordinate Convention
    case wxID_MENUCOORDINATE_X_UP:
        setCoordinateConvention( CoordinateConvention::xUp );
        centerMeshCamera();
        break;
    case wxID_MENUCOORDINATE_Y_UP:
        setCoordinateConvention( CoordinateConvention::yUp );
        centerMeshCamera();
        break;
    case wxID_MENUCOORDINATE_Z_UP:
        setCoordinateConvention( CoordinateConvention::zUp );
        centerMeshCamera();
        break;
    }
    event.Skip();
}
//-----------------------------------------------------------------------------
void MainWindow::setActiveDatablock( Ogre::HlmsDatablock *ogre_nullable datablock,
                                     const bool bUpdateDatablockListSelection )
{
    m_activeDatablock = datablock;
    m_pbsParametersPanel->refreshFromDatablock();
    m_pbsParametersPanel->refreshSubMeshList();
    m_pbsTexturePanel->refreshFromDatablockAndApplyEnvMap();
    if( bUpdateDatablockListSelection )
        m_datablockList->notifyDatablockSelectChanged();
}
//-----------------------------------------------------------------------------
void MainWindow::unloadForNewProject()
{
    if( m_activeItem )
    {
        m_sceneManager->destroyItem( m_activeItem );
        m_activeItem = 0;
    }
    if( m_activeEntity )
    {
        m_sceneManager->destroyEntity( m_activeEntity );
        m_activeEntity = 0;
    }

    setActiveDatablock( nullptr, false );
    m_undoSystem.clearBuffers();
}
//-----------------------------------------------------------------------------
bool MainWindow::loadMeshAsItem( const Ogre::String &meshName, const Ogre::String &resourceGroup )
{
    Ogre::Item *item = 0;
    try
    {
        // "Unoptimize" the mesh for shadow mapping: The user may toggle settings that require UVs
        // (e.g. alpha testing or alpha hashing). They will crash if the mesh doesn't have UVs
        // (which happens if the mesh was optimized for shadows).
        //
        // There's several solutions:
        //  1. Just unoptimize all meshes (what we do).
        //  2. Unoptimize the specific submesh, save the optimized copy in case the setting
        //     is turned off again. (or don't).
        //  3. Toggle shadow casting for the movable object.
        //
        // Since we don't know the user's intentions; just unoptimize everything (easiest path).
        Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().load( meshName, resourceGroup );
        mesh->prepareForShadowMapping( true );

        item = m_sceneManager->createItem( meshName, resourceGroup );
    }
    catch( Ogre::Exception & )
    {
        return false;
    }

    if( m_activeItem )
    {
        m_sceneManager->destroyItem( m_activeItem );
        m_activeItem = 0;
    }
    if( m_activeEntity )
    {
        m_sceneManager->destroyEntity( m_activeEntity );
        m_activeEntity = 0;
    }
    m_objSceneNode->attachObject( item );
    m_activeItem = item;

    return true;
}
//-----------------------------------------------------------------------------
bool MainWindow::loadMeshAsV1Entity( const Ogre::String &meshName, const Ogre::String &resourceGroup )
{
    Ogre::v1::Entity *entity = 0;
    try
    {
        Ogre::v1::MeshPtr mesh = Ogre::v1::MeshManager::getSingleton().load( meshName, resourceGroup );
        mesh->prepareForShadowMapping( true );  // See loadMeshAsItem().

        entity = m_sceneManager->createEntity( meshName, resourceGroup );
    }
    catch( Ogre::Exception & )
    {
        return false;
    }

    if( m_activeItem )
    {
        m_sceneManager->destroyItem( m_activeItem );
        m_activeItem = 0;
    }
    if( m_activeEntity )
    {
        m_sceneManager->destroyEntity( m_activeEntity );
        m_activeEntity = 0;
    }
    m_objSceneNode->attachObject( entity );
    m_activeEntity = entity;

    return true;
}
//-----------------------------------------------------------------------------
void MainWindow::setActiveMesh( const Ogre::String &meshName, const Ogre::String &resourceGroup )
{
    if( !loadMeshAsItem( meshName, resourceGroup ) )
    {
        if( !loadMeshAsV1Entity( meshName, resourceGroup ) )
        {
            wxMessageBox(
                wxT( "Could not open mesh = " ) + meshName + wxT( " group = " ) + resourceGroup,
                wxT( "Mesh Open Error" ), wxOK | wxICON_ERROR | wxCENTRE );
            return;
        }
    }

    m_pbsParametersPanel->refreshSubMeshList();
    m_pbsTexturePanel->notifyMeshChanged();
    m_datablockList->populateFromDatabase( true );
    m_lightPanel->notifyMeshChanged();

    centerMeshCamera();
}
//-----------------------------------------------------------------------------
Ogre::MovableObject *MainWindow::getActiveObject()
{
    if( m_activeItem )
        return m_activeItem;
    return m_activeEntity;
}
//-----------------------------------------------------------------------------
const Ogre::String &MainWindow::getOriginalMaterialNameForActiveObject( const size_t submeshIdx ) const
{
    if( m_activeItem )
    {
        return m_activeItem->getMesh()->getSubMesh( (unsigned int)submeshIdx )->getMaterialName();
    }
    else if( m_activeEntity )
    {
        return m_activeEntity->getMesh()->getSubMesh( (unsigned int)submeshIdx )->getMaterialName();
    }
    return Ogre::BLANKSTRING;
}
//-----------------------------------------------------------------------------
const Ogre::String &MainWindow::getActiveMeshResourceGroup() const
{
    if( m_activeItem )
        return m_activeItem->getMesh()->getGroup();
    else if( m_activeEntity )
        return m_activeEntity->getMesh()->getGroup();
    return Ogre::BLANKSTRING;
}
