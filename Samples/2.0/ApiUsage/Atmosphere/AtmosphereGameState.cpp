
#include "AtmosphereGameState.h"

#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreCamera.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsSamplerblock.h"
#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreSceneManager.h"

#include "OgreAtmosphereNpr.h"

using namespace Demo;

namespace Demo
{
    AtmosphereGameState::AtmosphereGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mTimeOfDay( Ogre::Math::PI * 0.1f ),
        mAzimuth( 0 ),
        mMultiplePresets( false ),
        mAtmosphere( 0 ),
        mSunLight( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    void AtmosphereGameState::createScene01()
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        const float armsLength = 2.5f;

        Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane(
            "Plane v1", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::Plane( Ogre::Vector3::UNIT_Y, 1.0f ), 50.0f, 50.0f, 1, 1, true, 1, 4.0f, 4.0f,
            Ogre::Vector3::UNIT_Z, Ogre::v1::HardwareBuffer::HBU_STATIC,
            Ogre::v1::HardwareBuffer::HBU_STATIC );

        Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createByImportingV1(
            "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, planeMeshV1.get(), true,
            true, true );

        {
            Ogre::Item *item = sceneManager->createItem( planeMesh, Ogre::SCENE_DYNAMIC );
            item->setDatablock( "Marble" );
            Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                                             ->createChildSceneNode( Ogre::SCENE_DYNAMIC );
            sceneNode->setPosition( 0, -1, 0 );
            sceneNode->attachObject( item );

            // Change the addressing mode of the roughness map to wrap via code.
            // Detail maps default to wrap, but the rest to clamp.
            assert( dynamic_cast<Ogre::HlmsPbsDatablock *>( item->getSubItem( 0 )->getDatablock() ) );
            Ogre::HlmsPbsDatablock *datablock =
                static_cast<Ogre::HlmsPbsDatablock *>( item->getSubItem( 0 )->getDatablock() );
            // Make a hard copy of the sampler block
            Ogre::HlmsSamplerblock samplerblock( *datablock->getSamplerblock( Ogre::PBSM_ROUGHNESS ) );
            samplerblock.mU = Ogre::TAM_WRAP;
            samplerblock.mV = Ogre::TAM_WRAP;
            samplerblock.mW = Ogre::TAM_WRAP;
            // Set the new samplerblock. The Hlms system will
            // automatically create the API block if necessary
            datablock->setSamplerblock( Ogre::PBSM_ROUGHNESS, samplerblock );
        }

        for( int i = 0; i < 4; ++i )
        {
            for( int j = 0; j < 4; ++j )
            {
                Ogre::String meshName;

                if( i == j )
                    meshName = "Sphere1000.mesh";
                else
                    meshName = "Cube_d.mesh";

                Ogre::Item *item = sceneManager->createItem(
                    meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                    Ogre::SCENE_DYNAMIC );
                if( i % 2 == 0 )
                    item->setDatablock( "Rocks" );
                else
                    item->setDatablock( "Marble" );

                item->setVisibilityFlags( 0x000000001 );

                const size_t idx = static_cast<size_t>( i * 4 + j );

                Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                                                 ->createChildSceneNode( Ogre::SCENE_DYNAMIC );

                sceneNode->setPosition( ( float( i ) - 1.5f ) * armsLength, 2.0f,
                                        ( float( j ) - 1.5f ) * armsLength );
                sceneNode->setScale( 0.65f, 0.65f, 0.65f );

                sceneNode->roll( Ogre::Radian( (Ogre::Real)idx ) );

                sceneNode->attachObject( item );
            }
        }

        Ogre::SceneNode *rootNode = sceneManager->getRootSceneNode();

        mSunLight = sceneManager->createLight();
        Ogre::SceneNode *lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( mSunLight );
        mSunLight->setPowerScale( 1.0f );
        mSunLight->setType( Ogre::Light::LT_DIRECTIONAL );
        mSunLight->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );

        sceneManager->setAmbientLight( Ogre::ColourValue( 0.3f, 0.5f, 0.7f ) * 0.1f * 0.75f,
                                       Ogre::ColourValue( 0.6f, 0.45f, 0.3f ) * 0.065f * 0.75f,
                                       -mSunLight->getDirection() + Ogre::Vector3::UNIT_Y * 0.2f );

        Ogre::Light *light = sceneManager->createLight();
        lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( light );
        light->setDiffuseColour( 0.8f, 0.4f, 0.2f );  // Warm
        light->setSpecularColour( 0.8f, 0.4f, 0.2f );
        light->setPowerScale( Ogre::Math::PI );
        light->setType( Ogre::Light::LT_SPOTLIGHT );
        lightNode->setPosition( -10.0f, 10.0f, 10.0f );
        light->setDirection( Ogre::Vector3( 1, -1, -1 ).normalisedCopy() );
        light->setAttenuationBasedOnRadius( 10.0f, 0.01f );

        light = sceneManager->createLight();
        lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( light );
        light->setDiffuseColour( 0.2f, 0.4f, 0.8f );  // Cold
        light->setSpecularColour( 0.2f, 0.4f, 0.8f );
        light->setPowerScale( Ogre::Math::PI );
        light->setType( Ogre::Light::LT_SPOTLIGHT );
        lightNode->setPosition( 10.0f, 10.0f, -10.0f );
        light->setDirection( Ogre::Vector3( -1, -1, 1 ).normalisedCopy() );
        light->setAttenuationBasedOnRadius( 10.0f, 0.01f );

        mAtmosphere =
            OGRE_NEW Ogre::AtmosphereNpr( sceneManager->getDestinationRenderSystem()->getVaoManager() );
        mAtmosphere->setLight( mSunLight );
        mAtmosphere->setSky( sceneManager, true );

        Ogre::AtmosphereNpr::PresetArray presets;

        presets.push_back( Ogre::AtmosphereNpr::Preset() );
        presets.back().time = 0.05f;

        presets.push_back( Ogre::AtmosphereNpr::Preset() );
        presets.back().time = 0.5f;  // Midday
        presets.back().densityCoeff = 0.58f;

        presets.push_back( Ogre::AtmosphereNpr::Preset() );
        presets.back().time = 0.95f;

        presets.push_back( Ogre::AtmosphereNpr::Preset() );
        presets.back().time = -0.95f;
        presets.back().densityCoeff = 0.08f;
        presets.back().sunPower = presets.back().linkedLightPower = 0.01f;
        presets.back().skyPower = 0.2f;
        presets.back().linkedSceneAmbientUpperPower *= 0.001f;
        presets.back().linkedSceneAmbientLowerPower *= 0.001f;

        presets.push_back( Ogre::AtmosphereNpr::Preset() );
        presets.back().time = -0.5f;  // Midnight
        presets.back().densityCoeff = 0.021f;
        presets.back().sunPower = presets.back().linkedLightPower = 0.005f;
        presets.back().skyPower = 0.2f;
        presets.back().linkedSceneAmbientUpperPower *= 0.001f;
        presets.back().linkedSceneAmbientLowerPower *= 0.001f;

        presets.push_back( Ogre::AtmosphereNpr::Preset() );
        presets.back().time = -0.05f;
        presets.back().densityCoeff = 0.08f;
        presets.back().sunPower = presets.back().linkedLightPower = 0.01f;
        presets.back().skyPower = 0.2f;
        presets.back().linkedSceneAmbientUpperPower *= 0.001f;
        presets.back().linkedSceneAmbientLowerPower *= 0.001f;

        mAtmosphere->setPresets( presets );

        mCameraController = new CameraController( mGraphicsSystem, false );

        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
    void AtmosphereGameState::destroyScene()
    {
        OGRE_DELETE mAtmosphere;
        mAtmosphere = 0;
    }
    //-----------------------------------------------------------------------------------
    void AtmosphereGameState::update( float timeSinceLast )
    {
        const Ogre::Vector3 sunDir(
            Ogre::Quaternion( Ogre::Radian( mAzimuth ), Ogre::Vector3::UNIT_Y ) *
            Ogre::Vector3( cosf( fabsf( mTimeOfDay ) ), -sinf( fabsf( mTimeOfDay ) ), 0.0 )
                .normalisedCopy() );
        if( mMultiplePresets )
            mAtmosphere->updatePreset( sunDir, mTimeOfDay / Ogre::Math::PI );
        else
            mAtmosphere->setSunDir( sunDir, fabsf( mTimeOfDay ) / Ogre::Math::PI );

        TutorialGameState::update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    template <typename T>
    void AtmosphereGameState::description( const char *keys, const char *setting, T value,
                                           Ogre::String &outText, bool bForce ) const
    {
        outText += "\n";
        if( mMultiplePresets && !bForce )
        {
            outText += setting;
        }
        else
        {
            outText += keys;
            outText += " to change ";
            outText += setting;
        }

        outText += " [";
        outText += Ogre::StringConverter::toString( value );
        outText += "]";
    }
    //-----------------------------------------------------------------------------------
    void AtmosphereGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
    {
        TutorialGameState::generateDebugText( timeSinceLast, outText );

        if( mDisplayHelpMode == 1 )
        {
            char tmp[128];
            Ogre::LwString str( Ogre::LwString::FromEmptyPointer( tmp, sizeof( tmp ) ) );
            Ogre::Vector3 camPos = mGraphicsSystem->getCamera()->getPosition();

            using namespace Ogre;

            description( "+/-", "Time of Day", mTimeOfDay * 180.0f / Math::PI, outText, true );
            description( "9/6", "Azimuth", mAzimuth * 180.0f / Math::PI, outText, true );
            description( "[Shift+]Y", "Density", mAtmosphere->getPreset().densityCoeff, outText );
            description( "[Shift+]U", "Density Diffusion", mAtmosphere->getPreset().densityDiffusion,
                         outText );
            description( "[Shift+]I", "Horizon Limit", mAtmosphere->getPreset().horizonLimit, outText );
            description( "[Shift+]O", "Fog Density", mAtmosphere->getPreset().fogDensity, outText );
            outText += "\nSpace to toggle multiple presets mode. [";
            outText += StringConverter::toString( mMultiplePresets ) + "]";

            outText += "\n\nCamera: ";
            str.a( "[", LwString::Float( camPos.x, 2, 2 ), ", ", LwString::Float( camPos.y, 2, 2 ), ", ",
                   LwString::Float( camPos.z, 2, 2 ), "]" );
            outText += str.c_str();
            outText += "\nLightDir: ";
            str.clear();
            str.a( "[", LwString::Float( mSunLight->getDirection().x, 2, 2 ), ", ",
                   LwString::Float( mSunLight->getDirection().y, 2, 2 ), ", ",
                   LwString::Float( mSunLight->getDirection().z, 2, 2 ), "]" );
            outText += str.c_str();
        }
    }
    //-----------------------------------------------------------------------------------
    void AtmosphereGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( ( arg.keysym.mod & ~( KMOD_NUM | KMOD_CAPS | KMOD_LSHIFT | KMOD_RSHIFT ) ) != 0 )
        {
            TutorialGameState::keyReleased( arg );
            return;
        }

        if( arg.keysym.scancode == SDL_SCANCODE_KP_PLUS )
        {
            mTimeOfDay += 0.1f;
            if( mTimeOfDay >= Ogre::Math::PI )
                mTimeOfDay = -Ogre::Math::PI + std::fmod( mTimeOfDay, (float)Ogre::Math::PI );
        }
        else if( arg.keysym.scancode == SDL_SCANCODE_MINUS ||
                 arg.keysym.scancode == SDL_SCANCODE_KP_MINUS )
        {
            mTimeOfDay -= 0.1f;
            if( mTimeOfDay <= -Ogre::Math::PI )
                mTimeOfDay = Ogre::Math::PI + std::fmod( mTimeOfDay, (float)Ogre::Math::PI );
        }
        else if( arg.keysym.scancode == SDL_SCANCODE_KP_9 )
        {
            mAzimuth += 0.1f;
            mAzimuth = fmodf( mAzimuth, Ogre::Math::TWO_PI );
        }
        else if( arg.keysym.scancode == SDL_SCANCODE_KP_6 )
        {
            mAzimuth -= 0.1f;
            mAzimuth = fmodf( mAzimuth, Ogre::Math::TWO_PI );
            if( mAzimuth < 0 )
                mAzimuth = Ogre::Math::TWO_PI + mAzimuth;
        }
        else if( arg.keysym.sym == SDLK_y && !mMultiplePresets )
        {
            Ogre::AtmosphereNpr::Preset preset = mAtmosphere->getPreset();

            float incrementVal = 0.05f;
            if( fabsf( preset.densityCoeff ) < incrementVal )
                incrementVal = 0.01f;
            if( fabsf( preset.densityCoeff ) < incrementVal )
                incrementVal = 0.0025f;

            if( !( arg.keysym.mod & ( KMOD_LSHIFT | KMOD_RSHIFT ) ) )
                preset.densityCoeff += incrementVal;
            else
                preset.densityCoeff -= incrementVal;

            mAtmosphere->setPreset( preset );
        }
        else if( arg.keysym.sym == SDLK_u && !mMultiplePresets )
        {
            Ogre::AtmosphereNpr::Preset preset = mAtmosphere->getPreset();
            if( !( arg.keysym.mod & ( KMOD_LSHIFT | KMOD_RSHIFT ) ) )
                preset.densityDiffusion += 0.25f;
            else
                preset.densityDiffusion -= 0.25f;

            mAtmosphere->setPreset( preset );
        }
        else if( arg.keysym.sym == SDLK_i && !mMultiplePresets )
        {
            Ogre::AtmosphereNpr::Preset preset = mAtmosphere->getPreset();
            if( !( arg.keysym.mod & ( KMOD_LSHIFT | KMOD_RSHIFT ) ) )
                preset.horizonLimit += 0.005f;
            else
                preset.horizonLimit -= 0.005f;

            mAtmosphere->setPreset( preset );
        }
        else if( arg.keysym.sym == SDLK_o && !mMultiplePresets )
        {
            Ogre::AtmosphereNpr::Preset preset = mAtmosphere->getPreset();
            if( !( arg.keysym.mod & ( KMOD_LSHIFT | KMOD_RSHIFT ) ) )
                preset.fogDensity += 0.005f;
            else
                preset.fogDensity -= 0.005f;

            preset.fogDensity = std::max( preset.fogDensity, 0.0f );

            mAtmosphere->setPreset( preset );
        }
        else if( arg.keysym.sym == SDLK_SPACE )
        {
            mMultiplePresets = !mMultiplePresets;
            if( mMultiplePresets )
            {
                const Ogre::Vector3 sunDir(
                    Ogre::Quaternion( Ogre::Radian( mAzimuth ), Ogre::Vector3::UNIT_Y ) *
                    Ogre::Vector3( cosf( fabsf( mTimeOfDay ) ), -sinf( fabsf( mTimeOfDay ) ), 0.0 )
                        .normalisedCopy() );
                mAtmosphere->updatePreset( sunDir, mTimeOfDay / Ogre::Math::PI );
            }
        }
        else
        {
            TutorialGameState::keyReleased( arg );
        }
    }
}  // namespace Demo
