
#include "Tutorial_OpenVR.h"
#include "Tutorial_OpenVRGameState.h"

#include "OgreSceneManager.h"
#include "OgreCamera.h"
#include "OgreRoot.h"
#include "OgreWindow.h"
#include "OgreConfigFile.h"
#include "Compositor/OgreCompositorManager2.h"
#include "OgreTextureGpuManager.h"

//Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/MainEntryPoints.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMainApp( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#else
int mainApp( int argc, const char *argv[] )
#endif
{
    return Demo::MainEntryPoints::mainAppSingleThreaded( DEMO_MAIN_ENTRY_PARAMS );
}

namespace Demo
{
    Ogre::CompositorWorkspace* Tutorial_OpenVRGraphicsSystem::setupCompositor()
    {
        Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
        return compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(), mCamera,
                                                "Tutorial_OpenVRWorkspace", true );
    }

    void Tutorial_OpenVRGraphicsSystem::setupResources(void)
    {
        GraphicsSystem::setupResources();

        Ogre::ConfigFile cf;
        cf.load(mResourcePath + "resources2.cfg");

        Ogre::String dataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

        if( dataFolder.empty() )
            dataFolder = "./";
        else if( *(dataFolder.end() - 1) != '/' )
            dataFolder += "/";

        dataFolder += "2.0/scripts/materials/PbsMaterials";

        addResourceLocation( dataFolder, "FileSystem", "General" );
    }

    //-----------------------------------------------------------------------------
    // Purpose: Helper to get a string from a tracked device property and turn it
    //			into a std::string
    //-----------------------------------------------------------------------------
    std::string Tutorial_OpenVRGraphicsSystem::GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice,
                                                                      vr::TrackedDeviceProperty prop,
                                                                      vr::TrackedPropertyError *peError)
    {
        vr::IVRSystem *vrSystem = vr::VRSystem();
        uint32_t unRequiredBufferLen = vrSystem->GetStringTrackedDeviceProperty( unDevice, prop,
                                                                                 NULL, 0, peError );
        if( unRequiredBufferLen == 0 )
            return "";

        char *pchBuffer = new char[ unRequiredBufferLen ];
        unRequiredBufferLen = vrSystem->GetStringTrackedDeviceProperty( unDevice, prop, pchBuffer,
                                                                        unRequiredBufferLen, peError );
        std::string sResult = pchBuffer;
        delete [] pchBuffer;
        return sResult;
    }

    void Tutorial_OpenVRGraphicsSystem::initOpenVR(void)
    {
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

        mStrDriver = GetTrackedDeviceString( vr::k_unTrackedDeviceIndex_Hmd,
                                             vr::Prop_TrackingSystemName_String );
        mStrDisplay = GetTrackedDeviceString( vr::k_unTrackedDeviceIndex_Hmd,
                                              vr::Prop_SerialNumber_String );

        initCompositorVR();

        uint32_t width, height;
        mHMD->GetRecommendedRenderTargetSize( &width, &height );

        Ogre::TextureGpuManager *textureManager = mRoot->getRenderSystem()->getTextureGpuManager();
        mVrTexture = textureManager->createOrRetrieveTexture( "OpenVR Both Eyes",
                                                              Ogre::GpuPageOutStrategy::Discard,
                                                              Ogre::TextureFlags::RenderToTexture,
                                                              Ogre::TextureTypes::Type2D );
        mVrTexture->setResolution( width, height );
        mVrTexture->setPixelFormat( Ogre::PFG_RGBA8_UNORM_SRGB );
        mVrTexture->scheduleTransitionTo( Ogre::GpuResidency::Resident );
    }

    void Tutorial_OpenVRGraphicsSystem::initCompositorVR(void)
    {
        if ( !vr::VRCompositor() )
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_RENDERINGAPI_ERROR,
                         "VR Compositor initialization failed. See log file for details",
                         "Tutorial_OpenVRGraphicsSystem::initCompositorVR" );
        }
    }

    void Tutorial_OpenVRGraphicsSystem::deinitialize(void)
    {
        if( mVrTexture )
        {
            Ogre::TextureGpuManager *textureManager = mRoot->getRenderSystem()->getTextureGpuManager();
            textureManager->destroyTexture( mVrTexture );
            mVrTexture = 0;
        }

        if( mHMD )
        {
            vr::VR_Shutdown();
            mHMD = NULL;
        }

        GraphicsSystem::deinitialize();
    }

    void Tutorial_OpenVRGraphicsSystem::createScene01(void)
    {
        initOpenVR();
        GraphicsSystem::createScene01();
    }

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState,
                                         LogicSystem **outLogicSystem )
    {
        Tutorial_OpenVRGameState *gfxGameState = new Tutorial_OpenVRGameState(
        "" );

        GraphicsSystem *graphicsSystem = new Tutorial_OpenVRGraphicsSystem( gfxGameState );

        gfxGameState->_notifyGraphicsSystem( graphicsSystem );

        *outGraphicsGameState = gfxGameState;
        *outGraphicsSystem = graphicsSystem;
    }

    void MainEntryPoints::destroySystems( GameState *graphicsGameState,
                                          GraphicsSystem *graphicsSystem,
                                          GameState *logicGameState,
                                          LogicSystem *logicSystem )
    {
        delete graphicsSystem;
        delete graphicsGameState;
    }

    const char* MainEntryPoints::getWindowTitle(void)
    {
        return "OpenVR Sample";
    }
}
