
#include "LocalCubemapsManualProbesGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreItem.h"
#include "OgreSceneManager.h"

#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"

#include "OgreCamera.h"

#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsSamplerblock.h"

#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreRoot.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorWorkspaceDef.h"

#include "OgreLwString.h"

#include "Cubemaps/OgreParallaxCorrectedCubemap.h"
#include "Cubemaps/OgreParallaxCorrectedCubemapAuto.h"

#include "LocalCubemapsManualProbesScene.h"

#include "OgreMaterial.h"
#include "OgreMaterialManager.h"
#include "OgrePass.h"
#include "OgreTechnique.h"
#include "OgreTextureUnitState.h"

using namespace Demo;

namespace Demo
{
    LocalCubemapsManualProbesGameState::LocalCubemapsManualProbesGameState(
        const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mParallaxCorrectedCubemap( 0 ),
        mParallaxCorrectedCubemapAuto( 0 ),
        mParallaxCorrectedCubemapOrig( 0 ),
        mPerPixelReflections( false ),
        mWasManualProbe( true )
    {
        memset( mMaterials, 0, sizeof( mMaterials ) );
    }
    //-----------------------------------------------------------------------------------
    void LocalCubemapsManualProbesGameState::setupParallaxCorrectCubemaps()
    {
        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );

        if( mParallaxCorrectedCubemap )
        {
            hlmsPbs->setParallaxCorrectedCubemap( 0 );

            delete mParallaxCorrectedCubemap;
            mParallaxCorrectedCubemap = 0;
            mParallaxCorrectedCubemapAuto = 0;
            mParallaxCorrectedCubemapOrig = 0;
        }

        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::CompositorManager2 *compositorManager = root->getCompositorManager2();
        Ogre::CompositorWorkspaceDef *workspaceDef =
            compositorManager->getWorkspaceDefinition( "LocalCubemapsProbeWorkspace" );

        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        if( !mPerPixelReflections )
        {
            // Disable Forward Clustered since it's not required anymore
            sceneManager->setForwardClustered( false, 0, 0, 0, 0, 0, 0, 0, 0 );

            mParallaxCorrectedCubemapOrig = new Ogre::ParallaxCorrectedCubemap(
                Ogre::Id::generateNewId<Ogre::ParallaxCorrectedCubemap>(), mGraphicsSystem->getRoot(),
                mGraphicsSystem->getSceneManager(), workspaceDef, 250, 1u << 25u );
            mParallaxCorrectedCubemapOrig->setEnabled( true, 1024, 1024, Ogre::PFG_RGBA8_UNORM_SRGB );
            mParallaxCorrectedCubemap = mParallaxCorrectedCubemapOrig;
        }
        else
        {
            // Per pixel reflections REQUIRE Forward Clustered
            sceneManager->setForwardClustered( true, 16, 8, 24, 4, 0, 2, 2, 50 );

            mParallaxCorrectedCubemapAuto = new Ogre::ParallaxCorrectedCubemapAuto(
                Ogre::Id::generateNewId<Ogre::ParallaxCorrectedCubemapAuto>(),
                mGraphicsSystem->getRoot(), mGraphicsSystem->getSceneManager(), workspaceDef );
            mParallaxCorrectedCubemapAuto->setEnabled( true, 1024, 1024, 4, Ogre::PFG_RGBA8_UNORM_SRGB );
            mParallaxCorrectedCubemap = mParallaxCorrectedCubemapAuto;
        }

        Ogre::CubemapProbe *probe = 0;
        Ogre::Vector3 probeCenter;
        Ogre::Aabb probeArea;
        Ogre::Aabb probeShape;

        // Probe 00
        probe = mParallaxCorrectedCubemap->createProbe();
        probe->setTextureParams( 1024, 1024, true );
        probe->initWorkspace();

        probeCenter = Ogre::Vector3( -0.0, 1.799999f, 0.232415f );
        probeArea.mCenter = Ogre::Vector3( -0.0, 12.011974f, 0.232414f );
        probeArea.mHalfSize = Ogre::Vector3( 12.589478f, 24.297745f, 10.569476f ) * 0.5;
        probeShape.mCenter = Ogre::Vector3( -0.0, 12.011974f, 0.232414f );
        probeShape.mHalfSize = Ogre::Vector3( 12.589478f, 24.297745f, 10.569476f ) * 0.5;
        probe->set( probeCenter, probeArea, Ogre::Vector3( 1.0f, 1.0f, 0.3f ), Ogre::Matrix3::IDENTITY,
                    probeShape );

        // Probe 01
        probe = mParallaxCorrectedCubemap->createProbe();
        probe->setTextureParams( 1024, 1024, true );
        probe->initWorkspace();

        probeCenter = Ogre::Vector3( -5.232418f, 1.799997f, 18.313454f );
        probeArea.mCenter = Ogre::Vector3( -5.263578f, 12.011974f, 13.145589f );
        probeArea.mHalfSize = Ogre::Vector3( 2.062322f, 24.297745f, 17.866102f ) * 0.5;
        probeShape.mCenter = Ogre::Vector3( -5.211694f, 12.011974f, 8.478205f );
        probeShape.mHalfSize = Ogre::Vector3( 2.166091f, 24.297745f, 27.200872f ) * 0.5;
        probe->set( probeCenter, probeArea, Ogre::Vector3( 1.0f, 1.0f, 0.3f ), Ogre::Matrix3::IDENTITY,
                    probeShape );

        // Probe 02
        probe = mParallaxCorrectedCubemap->createProbe();
        probe->setTextureParams( 1024, 1024, true );
        probe->initWorkspace();

        probeCenter = Ogre::Vector3( 3.767576f, 1.799997f, 20.84387f );
        probeArea.mCenter = Ogre::Vector3( 2.632758f, 12.011975f, 22.444103f );
        probeArea.mHalfSize = Ogre::Vector3( 10.365083f, 24.297745f, 21.705084f ) * 0.5;
        probeShape.mCenter = Ogre::Vector3( 3.752187f, 12.011975f, 22.444103f );
        probeShape.mHalfSize = Ogre::Vector3( 8.126225f, 24.297745f, 21.705084f ) * 0.5;
        probe->set( probeCenter, probeArea, Ogre::Vector3( 0.7f, 1.0f, 0.3f ), Ogre::Matrix3::IDENTITY,
                    probeShape );

        // Probe 03
        probe = mParallaxCorrectedCubemap->createProbe();
        probe->setTextureParams( 1024, 1024, true );
        probe->initWorkspace();

        probeCenter = Ogre::Vector3( -2.565753f, 1.799996f, 20.929661f );
        probeArea.mCenter = Ogre::Vector3( -2.703529f, 12.011974f, 21.099365f );
        probeArea.mHalfSize = Ogre::Vector3( 7.057773f, 24.297745f, 2.166093f ) * 0.5;
        probeShape.mCenter = Ogre::Vector3( 0.767578f, 12.011974f, 21.099365f );
        probeShape.mHalfSize = Ogre::Vector3( 13.999986f, 24.297745f, 2.166093f ) * 0.5;
        probe->set( probeCenter, probeArea, Ogre::Vector3( 0.7f, 1.0f, 0.3f ), Ogre::Matrix3::IDENTITY,
                    probeShape );

        hlmsPbs->setParallaxCorrectedCubemap( mParallaxCorrectedCubemap );

        //        Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().load(
        //                    "SkyPostprocess",
        //                    Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME ).
        //                staticCast<Ogre::Material>();
        //        Ogre::TextureUnitState *tu =
        //        mat->getBestTechnique()->getPass(0)->getTextureUnitState(0); tu->setTexture(
        //        mParallaxCorrectedCubemap->getBlendCubemap() );
    }
    //-----------------------------------------------------------------------------------
    void LocalCubemapsManualProbesGameState::createScene01()
    {
        setupParallaxCorrectCubemaps();

        // Setup a scene similar to that of PBS sample, except
        // we apply the cubemap to everything via C++ code
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        sceneManager->setForwardClustered( true, 16, 8, 24, 96, 0, 3, 2, 50 );

        Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane(
            "Plane v1", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::Plane( Ogre::Vector3::UNIT_Y, 1.0f ), 50.0f, 50.0f, 1, 1, true, 1, 4.0f, 4.0f,
            Ogre::Vector3::UNIT_Z, Ogre::v1::HardwareBuffer::HBU_STATIC,
            Ogre::v1::HardwareBuffer::HBU_STATIC );

        Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createByImportingV1(
            "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, planeMeshV1.get(), true,
            true, true );

        {
            Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
            assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
            Ogre::HlmsPbs *hlmsPbs =
                static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );

            Ogre::HlmsBlendblock blendblock;
            Ogre::HlmsMacroblock macroblock;

            struct DemoMaterials
            {
                Ogre::String matName;
                Ogre::ColourValue colour;
            };

            DemoMaterials materials[4] = {
                { "Red", Ogre::ColourValue::Red },
                { "Green", Ogre::ColourValue::Green },
                { "Blue", Ogre::ColourValue::Blue },
                { "Cream", Ogre::ColourValue::White },
            };

            for( int i = 0; i < 4; ++i )
            {
                for( int j = 0; j < 4; ++j )
                {
                    Ogre::String finalName =
                        materials[i].matName + "_P" + Ogre::StringConverter::toString( j );

                    Ogre::HlmsPbsDatablock *datablock;
                    datablock = static_cast<Ogre::HlmsPbsDatablock *>( hlmsPbs->createDatablock(
                        finalName, finalName, macroblock, blendblock, Ogre::HlmsParamVec() ) );
                    datablock->setBackgroundDiffuse( materials[i].colour );
                    datablock->setFresnel( Ogre::Vector3( 0.1f ), false );
                    datablock->setRoughness( 0.02f );
                    // datablock->setCubemapProbe( mParallaxCorrectedCubemap->getProbes()[j] );
                    mMaterials[i * 4 + j] = datablock;
                }
            }
        }

        generateScene( sceneManager );

        Ogre::SceneNode *rootNode = sceneManager->getRootSceneNode();

        Ogre::Light *light = sceneManager->createLight();
        Ogre::SceneNode *lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( light );
        light->setPowerScale( 1.0f );
        light->setType( Ogre::Light::LT_DIRECTIONAL );
        light->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );

        mLightNodes[0] = lightNode;

        sceneManager->setAmbientLight( Ogre::ColourValue( 0.3f, 0.5f, 0.7f ) * 0.1f * 0.75f,
                                       Ogre::ColourValue( 0.6f, 0.45f, 0.3f ) * 0.065f * 0.75f,
                                       -light->getDirection() + Ogre::Vector3::UNIT_Y * 0.2f );

        light = sceneManager->createLight();
        lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( light );
        light->setDiffuseColour( 0.8f, 0.4f, 0.2f );  // Warm
        light->setSpecularColour( 0.8f, 0.4f, 0.2f );
        light->setPowerScale( Ogre::Math::PI );
        light->setType( Ogre::Light::LT_SPOTLIGHT );
        // lightNode->setPosition( -2.0f, 6.0f, 10.0f );
        lightNode->setPosition( -12.0f, 6.0f, 8.0f );
        light->setDirection( Ogre::Vector3( 1.5, -1, -0.5 ).normalisedCopy() );
        light->setAttenuationBasedOnRadius( 10.0f, 0.01f );
        light->setSpotlightOuterAngle( Ogre::Degree( 80.0f ) );

        mLightNodes[1] = lightNode;

        light = sceneManager->createLight();
        lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( light );
        light->setDiffuseColour( 0.2f, 0.4f, 0.8f );  // Cold
        light->setSpecularColour( 0.2f, 0.4f, 0.8f );
        light->setPowerScale( Ogre::Math::PI );
        light->setType( Ogre::Light::LT_SPOTLIGHT );
        lightNode->setPosition( 2.0f, 6.0f, -3.0f );
        light->setDirection( Ogre::Vector3( -0.5, -1, 0.5 ).normalisedCopy() );
        light->setAttenuationBasedOnRadius( 10.0f, 0.01f );
        light->setSpotlightOuterAngle( Ogre::Degree( 80.0f ) );

        mLightNodes[2] = lightNode;

        mCameraController = new CameraController( mGraphicsSystem, false );
        mCameraController->mCameraBaseSpeed = 1.0f;
        mCameraController->mCameraSpeedBoost = 10.0f;

        Ogre::Camera *camera = mGraphicsSystem->getCamera();
        camera->setPosition( Ogre::Vector3( 3.767576f, 1.799997f, 20.84387f ) );
        camera->lookAt( camera->getPosition() + Ogre::Vector3( -1, 0, 0 ) );
        if( mParallaxCorrectedCubemapAuto )
            mParallaxCorrectedCubemapAuto->setUpdatedTrackedDataFromCamera( camera );
        if( mParallaxCorrectedCubemapOrig )
            mParallaxCorrectedCubemapOrig->setUpdatedTrackedDataFromCamera( camera );

        TutorialGameState::createScene01();

        if( mParallaxCorrectedCubemapAuto )
            mParallaxCorrectedCubemapAuto->updateAllDirtyProbes();
        if( mParallaxCorrectedCubemapOrig )
            mParallaxCorrectedCubemapOrig->updateAllDirtyProbes();

        // Updates the probe after assigning the manual ones, as results will be different.
        // Whether they look better or worse depends on how good you've subdivided the
        // scene and assigned the manual probes.
        const Ogre::CubemapProbeVec &probes = mParallaxCorrectedCubemap->getProbes();
        for( size_t i = 0; i < 4 * 4; ++i )
            mMaterials[i]->setCubemapProbe( probes[i % 4] );
    }
    //-----------------------------------------------------------------------------------
    void LocalCubemapsManualProbesGameState::destroyScene()
    {
        for( int i = 0; i < 4 * 4; ++i )
            mMaterials[i]->setCubemapProbe( 0 );

        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );
        hlmsPbs->setParallaxCorrectedCubemap( 0 );

        delete mParallaxCorrectedCubemap;
        mParallaxCorrectedCubemap = 0;
        mParallaxCorrectedCubemapAuto = 0;
        mParallaxCorrectedCubemapOrig = 0;
    }
    //-----------------------------------------------------------------------------------
    void LocalCubemapsManualProbesGameState::update( float timeSinceLast )
    {
        /*if( mAnimateObjects )
        {
            for( int i=0; i<16; ++i )
                mSceneNode[i]->yaw( Ogre::Radian(timeSinceLast * float( i ) * 0.125f) );
        }*/

        // Have the parallax corrected cubemap system keep track of the camera.
        Ogre::Camera *camera = mGraphicsSystem->getCamera();
        if( mParallaxCorrectedCubemapAuto )
            mParallaxCorrectedCubemapAuto->setUpdatedTrackedDataFromCamera( camera );
        if( mParallaxCorrectedCubemapOrig )
            mParallaxCorrectedCubemapOrig->setUpdatedTrackedDataFromCamera( camera );

        // camera->setPosition( Ogre::Vector3( -0.505, 3.400016, 5.423867 ) );
        // camera->setPosition( -1.03587, 2.50012, 3.62891 );
        // camera->setOrientation( Ogre::Quaternion::IDENTITY );

        TutorialGameState::update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    void LocalCubemapsManualProbesGameState::generateDebugText( float timeSinceLast,
                                                                Ogre::String &outText )
    {
        char tmpBuffer[64];
        Ogre::LwString roughnessStr(
            Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
        roughnessStr.a( Ogre::LwString::Float( mMaterials[0]->getRoughness(), 2 ) );

        TutorialGameState::generateDebugText( timeSinceLast, outText );
        outText += "\nPress F2/F3 to adjust material roughness: ";
        outText += roughnessStr.c_str();
        if( !mPerPixelReflections )
        {
            outText += "\nPress F4 to toggle mode. ";
            outText += mMaterials[0]->getCubemapProbe() == 0 ? "[Auto]" : "[Manual]";
        }
        outText += "\nPress F5 to toggle per pixel reflections. ";
        outText += mPerPixelReflections ? "[Per Pixel]" : "[Unified]";
        if( mParallaxCorrectedCubemapOrig )
        {
            outText += "\nProbes blending: ";
            outText += Ogre::StringConverter::toString(
                mParallaxCorrectedCubemapOrig->getNumCollectedProbes() );
        }

        Ogre::Camera *camera = mGraphicsSystem->getCamera();
        outText += "\nCamera: ";
        outText += Ogre::StringConverter::toString( camera->getPosition().x ) + ", " +
                   Ogre::StringConverter::toString( camera->getPosition().y ) + ", " +
                   Ogre::StringConverter::toString( camera->getPosition().z );
    }
    //-----------------------------------------------------------------------------------
    void LocalCubemapsManualProbesGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( ( arg.keysym.mod & ~( KMOD_NUM | KMOD_CAPS ) ) != 0 )
        {
            TutorialGameState::keyReleased( arg );
            return;
        }

        if( arg.keysym.sym == SDLK_F2 )
        {
            float roughness = mMaterials[0]->getRoughness();
            for( int i = 0; i < 4 * 4; ++i )
                mMaterials[i]->setRoughness( Ogre::Math::Clamp( roughness - 0.1f, 0.02f, 1.0f ) );
        }
        else if( arg.keysym.sym == SDLK_F3 )
        {
            float roughness = mMaterials[0]->getRoughness();
            for( int i = 0; i < 4 * 4; ++i )
                mMaterials[i]->setRoughness( Ogre::Math::Clamp( roughness + 0.1f, 0.02f, 1.0f ) );
        }
        else if( arg.keysym.sym == SDLK_F4 )
        {
            if( !mPerPixelReflections )
            {
                const Ogre::CubemapProbeVec &probes = mParallaxCorrectedCubemap->getProbes();
                for( size_t i = 0; i < 4 * 4; ++i )
                {
                    if( mMaterials[i]->getCubemapProbe() )
                        mMaterials[i]->setCubemapProbe( 0 );
                    else
                        mMaterials[i]->setCubemapProbe( probes[i % 4] );
                }
            }
        }
        else if( arg.keysym.sym == SDLK_F5 )
        {
            mPerPixelReflections = !mPerPixelReflections;

            if( mPerPixelReflections )
            {
                // Save current setting in case we're toggled back again
                mWasManualProbe = mMaterials[0]->getCubemapProbe() != 0;

                // Ensure manual probes are not in use!!!
                for( int i = 0; i < 4 * 4; ++i )
                {
                    if( mMaterials[i]->getCubemapProbe() )
                        mMaterials[i]->setCubemapProbe( 0 );
                }
            }

            setupParallaxCorrectCubemaps();
            if( mParallaxCorrectedCubemapAuto )
                mParallaxCorrectedCubemapAuto->updateAllDirtyProbes();
            if( mParallaxCorrectedCubemapOrig )
                mParallaxCorrectedCubemapOrig->updateAllDirtyProbes();

            if( !mPerPixelReflections && mWasManualProbe )
            {
                // Restore manual probes
                const Ogre::CubemapProbeVec &probes = mParallaxCorrectedCubemap->getProbes();
                for( size_t i = 0; i < 4 * 4; ++i )
                    mMaterials[i]->setCubemapProbe( probes[i % 4] );
            }
        }
        else
        {
            TutorialGameState::keyReleased( arg );
        }
    }
}  // namespace Demo
