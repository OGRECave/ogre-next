
#include "Tutorial_OpenVR.h"
#include "Tutorial_OpenVRGameState.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "OgreCamera.h"
#include "OgreConfigFile.h"
#include "OgreHiddenAreaMeshVr.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreTextureGpuManager.h"
#include "OgreWindow.h"

// Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/MainEntryPoints.h"

#include "NullCompositorListener.h"
#include "OgreLogManager.h"
#include "OpenVRCompositorListener.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMainApp( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#else
int mainApp( int argc, const char *argv[] )
#endif
{
    return Demo::MainEntryPoints::mainAppSingleThreaded( DEMO_MAIN_ENTRY_PARAMS );
}

#define USE_OPEN_VR

extern const bool c_useRDM;

// Set this to false to disable Radial Density Mask (RDM) optimization
// The value is hardcoded in C++, however you can extend this to be
// controlled at runtime
//
// The main reason we allow this setting to be disabled is because it is
// causing glitches in NVIDIA GPUs in Linux, see https://github.com/OGRECave/ogre-next/issues/53
const bool c_useRDM = true;

namespace Demo
{
    Ogre::CompositorWorkspace *Tutorial_OpenVRGraphicsSystem::setupCompositor()
    {
        initOpenVR();

        Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
        Ogre::CompositorChannelVec channels( 2u );
        channels[0] = mRenderWindow->getTexture();
        channels[1] = mVrTexture;
        return compositorManager->addWorkspace( mSceneManager, channels, mCamera,
                                                "Tutorial_OpenVRMirrorWindowWorkspace", true );
    }

    void Tutorial_OpenVRGraphicsSystem::setupResources()
    {
        GraphicsSystem::setupResources();

        Ogre::ConfigFile cf;
        cf.load( mResourcePath + "resources2.cfg" );

        Ogre::String originalDataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

        if( originalDataFolder.empty() )
            originalDataFolder = "./";
        else if( *( originalDataFolder.end() - 1 ) != '/' )
            originalDataFolder += "/";

        const char *c_locations[] = { "Hlms/Common/GLSL",
                                      "Hlms/Common/HLSL",
                                      "Hlms/Common/Metal",
                                      "Compute/VR",
                                      "Compute/VR/Foveated",
                                      "2.0/scripts/materials/PbsMaterials" };

        for( size_t i = 0; i < sizeof( c_locations ) / sizeof( c_locations[0] ); ++i )
        {
            Ogre::String dataFolder = originalDataFolder + c_locations[i];
            addResourceLocation( dataFolder, "FileSystem", "General" );
        }
    }

    //-----------------------------------------------------------------------------
    // Purpose: Helper to get a string from a tracked device property and turn it
    //			into a std::string
    //-----------------------------------------------------------------------------
    std::string Tutorial_OpenVRGraphicsSystem::GetTrackedDeviceString(
        vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop,
        vr::TrackedPropertyError *peError )
    {
        vr::IVRSystem *vrSystem = vr::VRSystem();
        uint32_t unRequiredBufferLen =
            vrSystem->GetStringTrackedDeviceProperty( unDevice, prop, NULL, 0, peError );
        if( unRequiredBufferLen == 0 )
            return "";

        char *pchBuffer = new char[unRequiredBufferLen];
        unRequiredBufferLen = vrSystem->GetStringTrackedDeviceProperty( unDevice, prop, pchBuffer,
                                                                        unRequiredBufferLen, peError );
        std::string sResult = pchBuffer;
        delete[] pchBuffer;
        return sResult;
    }

    void Tutorial_OpenVRGraphicsSystem::initOpenVR()
    {
        const Ogre::IdString workspaceName =
            c_useRDM ? "Tutorial_OpenVRWorkspaceRDM" : "Tutorial_OpenVRWorkspaceNoRDM";
#ifdef USE_OPEN_VR
        // Loading the SteamVR Runtime
        vr::EVRInitError eError = vr::VRInitError_None;
        mHMD = vr::VR_Init( &eError, vr::VRApplication_Scene );

        if( eError != vr::VRInitError_None )
        {
            mHMD = 0;
            Ogre::String errorMsg = "Unable to init VR runtime: ";
            errorMsg += vr::VR_GetVRInitErrorAsEnglishDescription( eError );
            OGRE_EXCEPT( Ogre::Exception::ERR_RENDERINGAPI_ERROR, errorMsg,
                         "Tutorial_OpenVRGraphicsSystem::initOpenVR" );
        }

        mStrDriver = "No Driver";
        mStrDisplay = "No Display";

        mStrDriver =
            GetTrackedDeviceString( vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String );
        mStrDisplay =
            GetTrackedDeviceString( vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String );
        mDeviceModelNumber =
            GetTrackedDeviceString( vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_ModelNumber_String );

        initCompositorVR();

        uint32_t width, height;
        mHMD->GetRecommendedRenderTargetSize( &width, &height );

        Ogre::TextureGpuManager *textureManager = mRoot->getRenderSystem()->getTextureGpuManager();
        // Radial Density Mask requires the VR texture to be UAV & reinterpretable
        mVrTexture = textureManager->createOrRetrieveTexture(
            "OpenVR Both Eyes", Ogre::GpuPageOutStrategy::Discard,
            Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::Uav |
                Ogre::TextureFlags::Reinterpretable,
            Ogre::TextureTypes::Type2D );
        mVrTexture->setResolution( width << 1u, height );
        mVrTexture->setPixelFormat( Ogre::PFG_RGBA8_UNORM_SRGB );
        if( !c_useRDM )
            mVrTexture->setSampleDescription( Ogre::SampleDescription( 4u ) );
        mVrTexture->scheduleTransitionTo( Ogre::GpuResidency::Resident );

        mVrCullCamera = mSceneManager->createCamera( "VrCullCamera" );

        Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
        mVrWorkspace = compositorManager->addWorkspace( mSceneManager, mVrTexture, mCamera,
                                                        workspaceName, true, 0 );

        createHiddenAreaMeshVR();

        mOvrCompositorListener = new OpenVRCompositorListener(
            mHMD, vr::VRCompositor(), mVrTexture, mRoot, mVrWorkspace, mCamera, mVrCullCamera );
#else
        Ogre::TextureGpuManager *textureManager = mRoot->getRenderSystem()->getTextureGpuManager();
        // Radial Density Mask requires the VR texture to be UAV & reinterpretable
        mVrTexture = textureManager->createOrRetrieveTexture(
            "OpenVR Both Eyes", Ogre::GpuPageOutStrategy::Discard,
            Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::Uav |
                Ogre::TextureFlags::Reinterpretable,
            Ogre::TextureTypes::Type2D );
        mVrTexture->setResolution( 3704u, 2056u );
        mVrTexture->setPixelFormat( Ogre::PFG_RGBA8_UNORM_SRGB );
        if( !c_useRDM )
            mVrTexture->setSampleDescription( Ogre::SampleDescription( 4u ) );
        mVrTexture->scheduleTransitionTo( Ogre::GpuResidency::Resident );

        mVrCullCamera = mSceneManager->createCamera( "VrCullCamera" );

        Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
        mVrWorkspace = compositorManager->addWorkspace( mSceneManager, mVrTexture, mCamera,
                                                        workspaceName, true, 0 );

        mDeviceModelNumber = "Vive";  // Pretend we have a Vive so the HAM works.
        createHiddenAreaMeshVR();

        mNullCompositorListener =
            new NullCompositorListener( mVrTexture, mRoot, mVrWorkspace, mCamera, mVrCullCamera );
#endif
    }

    void Tutorial_OpenVRGraphicsSystem::initCompositorVR()
    {
        if( !vr::VRCompositor() )
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_RENDERINGAPI_ERROR,
                         "VR Compositor initialization failed. See log file for details",
                         "Tutorial_OpenVRGraphicsSystem::initCompositorVR" );
        }
    }

    void Tutorial_OpenVRGraphicsSystem::createHiddenAreaMeshVR()
    {
        try
        {
            Ogre::ConfigFile cfgFile;
            cfgFile.load( mResourcePath + "HiddenAreaMeshVr.cfg" );

            Ogre::HiddenAreaVrSettings setting;
            setting = Ogre::HiddenAreaMeshVrGenerator::loadSettings( mDeviceModelNumber, cfgFile );
            if( setting.tessellation > 0u )
                Ogre::HiddenAreaMeshVrGenerator::generate( "HiddenAreaMeshVr.mesh", setting );
        }
        catch( Ogre::FileNotFoundException &e )
        {
            Ogre::LogManager &logManager = Ogre::LogManager::getSingleton();
            logManager.logMessage( e.getDescription() );
            logManager.logMessage( "HiddenAreaMeshVR optimization won't be available" );
        }
    }

    void Tutorial_OpenVRGraphicsSystem::deinitialize()
    {
        delete mOvrCompositorListener;
        mOvrCompositorListener = 0;

        delete mNullCompositorListener;
        mNullCompositorListener = 0;

        if( mVrTexture )
        {
            Ogre::TextureGpuManager *textureManager = mRoot->getRenderSystem()->getTextureGpuManager();
            textureManager->destroyTexture( mVrTexture );
            mVrTexture = 0;
        }

        if( mVrCullCamera )
        {
            mSceneManager->destroyCamera( mVrCullCamera );
            mVrCullCamera = 0;
        }

        if( mHMD )
        {
            vr::VR_Shutdown();
            mHMD = NULL;
        }

        GraphicsSystem::deinitialize();
    }

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState, LogicSystem **outLogicSystem )
    {
        Tutorial_OpenVRGameState *gfxGameState = new Tutorial_OpenVRGameState( "" );

        GraphicsSystem *graphicsSystem = new Tutorial_OpenVRGraphicsSystem( gfxGameState );

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

    const char *MainEntryPoints::getWindowTitle() { return "OpenVR Sample"; }
}  // namespace Demo
