/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2021 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include "Tutorial_TerrainGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"
#include "Utils/MeshUtils.h"

#include "OgreSceneManager.h"

#include "OgreRoot.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include "OgreCamera.h"
#include "OgreWindow.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "Terra/Hlms/OgreHlmsTerra.h"
#include "Terra/Hlms/OgreHlmsTerraDatablock.h"
#include "Terra/Hlms/PbsListener/OgreHlmsPbsTerraShadows.h"
#include "Terra/Terra.h"
#include "Terra/TerraShadowMapper.h"

#include "OgreTextureGpuManager.h"

#include "OgreGpuProgramManager.h"
#include "OgreLwString.h"

#include "OgreItem.h"

#ifdef OGRE_BUILD_COMPONENT_ATMOSPHERE
#    include "OgreAtmosphereNpr.h"
#endif

using namespace Demo;

namespace Demo
{
    Tutorial_TerrainGameState::Tutorial_TerrainGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mLockCameraToGround( false ),
        mTriplanarMappingEnabled( false ),
        mTimeOfDay( Ogre::Math::PI * /*0.25f*/ /*0.55f*/ 0.1f ),
        mAzimuth( 0 ),
        mTerra( 0 ),
        mSunLight( 0 ),
        mHlmsPbsTerraShadows( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_TerrainGameState::createScene01()
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        // Render terrain after most objects, to improve performance by taking advantage of early Z
        mTerra =
            new Ogre::Terra( Ogre::Id::generateNewId<Ogre::MovableObject>(),
                             &sceneManager->_getEntityMemoryManager( Ogre::SCENE_STATIC ), sceneManager,
                             11u, root->getCompositorManager2(), mGraphicsSystem->getCamera(), false );
        mTerra->setCastShadows( false );

        // mTerra->load( "Heightmap.png", Ogre::Vector3::ZERO, Ogre::Vector3( 256.0f, 1.0f, 256.0f ),
        // false ); mTerra->load( "Heightmap.png", Ogre::Vector3( 64.0f, 0, 64.0f ), Ogre::Vector3(
        // 128.0f, 5.0f, 128.0f ), false ); mTerra->load( "Heightmap.png", Ogre::Vector3( 64.0f, 0, 64.0f
        // ), Ogre::Vector3( 1024.0f, 5.0f, 1024.0f ), false ); mTerra->load( "Heightmap.png",
        // Ogre::Vector3( 64.0f, 0, 64.0f ), Ogre::Vector3( 4096.0f * 4, 15.0f * 64.0f*4, 4096.0f * 4 ),
        // false );
        mTerra->load( "Heightmap.png", Ogre::Vector3( 64.0f, 4096.0f * 0.5f, 64.0f ),
                      Ogre::Vector3( 4096.0f, 4096.0f, 4096.0f ), false, false );
        // mTerra->load( "Heightmap.png", Ogre::Vector3( 64.0f, 4096.0f * 0.5f, 64.0f ), Ogre::Vector3(
        // 14096.0f, 14096.0f, 14096.0f ), false );

        Ogre::SceneNode *rootNode = sceneManager->getRootSceneNode( Ogre::SCENE_STATIC );
        Ogre::SceneNode *sceneNode = rootNode->createChildSceneNode( Ogre::SCENE_STATIC );
        sceneNode->attachObject( mTerra );

        Ogre::HlmsManager *hlmsManager = root->getHlmsManager();
        Ogre::HlmsDatablock *datablock = hlmsManager->getDatablock( "TerraExampleMaterial" );
        //        Ogre::HlmsDatablock *datablock = hlmsManager->getHlms( Ogre::HLMS_USER3
        //        )->getDefaultDatablock(); Ogre::HlmsMacroblock macroblock; macroblock.mPolygonMode =
        //        Ogre::PM_WIREFRAME;
        // datablock->setMacroblock( macroblock );
        mTerra->setDatablock( datablock );

        {
            mHlmsPbsTerraShadows = new Ogre::HlmsPbsTerraShadows();
            mHlmsPbsTerraShadows->setTerra( mTerra );
            // Set the PBS listener so regular objects also receive terrain shadows
            Ogre::Hlms *hlmsPbs = root->getHlmsManager()->getHlms( Ogre::HLMS_PBS );
            hlmsPbs->setListener( mHlmsPbsTerraShadows );
        }

        mSunLight = sceneManager->createLight();
        Ogre::SceneNode *lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( mSunLight );
        mSunLight->setPowerScale( Ogre::Math::PI );
        mSunLight->setType( Ogre::Light::LT_DIRECTIONAL );
        mSunLight->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );

        sceneManager->setAmbientLight( Ogre::ColourValue( 0.33f, 0.61f, 0.98f ) * 0.01f,
                                       Ogre::ColourValue( 0.02f, 0.53f, 0.96f ) * 0.01f,
                                       Ogre::Vector3::UNIT_Y );

#ifdef OGRE_BUILD_COMPONENT_ATMOSPHERE
        mGraphicsSystem->createAtmosphere( mSunLight );
        OGRE_ASSERT_HIGH( dynamic_cast<Ogre::AtmosphereNpr *>( sceneManager->getAtmosphere() ) );
        Ogre::AtmosphereNpr *atmosphere =
            static_cast<Ogre::AtmosphereNpr *>( sceneManager->getAtmosphere() );
        Ogre::AtmosphereNpr::Preset p = atmosphere->getPreset();
        p.fogDensity = 0.00004f;
        p.fogBreakMinBrightness = 0.05f;
        atmosphere->setPreset( p );
#endif

        mCameraController = new CameraController( mGraphicsSystem, false );
        mCameraController->mCameraBaseSpeed = 200.f;
        mCameraController->mCameraSpeedBoost = 3.0f;
        mGraphicsSystem->getCamera()->setFarClipDistance( 100000.0f );
        mGraphicsSystem->getCamera()->setPosition( -10.0f, 80.0f, 10.0f );

        MeshUtils::importV1Mesh( "tudorhouse.mesh",
                                 Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );

        // Create some meshes to show off terrain shadows.
        Ogre::Item *item = sceneManager->createItem(
            "tudorhouse.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
            Ogre::SCENE_STATIC );
        Ogre::Vector3 objPos( 3.5f, 4.5f, -2.0f );
        mTerra->getHeightAt( objPos );
        objPos.y += -std::min( item->getLocalAabb().getMinimum().y, Ogre::Real( 0.0f ) ) * 0.01f - 0.5f;
        sceneNode = rootNode->createChildSceneNode( Ogre::SCENE_STATIC, objPos );
        sceneNode->scale( 0.01f, 0.01f, 0.01f );
        sceneNode->attachObject( item );

        item = sceneManager->createItem( "tudorhouse.mesh",
                                         Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                         Ogre::SCENE_STATIC );
        objPos = Ogre::Vector3( -3.5f, 4.5f, -2.0f );
        mTerra->getHeightAt( objPos );
        objPos.y += -std::min( item->getLocalAabb().getMinimum().y, Ogre::Real( 0.0f ) ) * 0.01f - 0.5f;
        sceneNode = rootNode->createChildSceneNode( Ogre::SCENE_STATIC, objPos );
        sceneNode->scale( 0.01f, 0.01f, 0.01f );
        sceneNode->attachObject( item );

        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_TerrainGameState::destroyScene()
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::Hlms *hlmsPbs = root->getHlmsManager()->getHlms( Ogre::HLMS_PBS );

        // Unset the PBS listener and destroy it
        if( hlmsPbs->getListener() == mHlmsPbsTerraShadows )
        {
            hlmsPbs->setListener( 0 );
            delete mHlmsPbsTerraShadows;
            mHlmsPbsTerraShadows = 0;
        }

        delete mTerra;
        mTerra = 0;

        TutorialGameState::destroyScene();
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_TerrainGameState::update( float timeSinceLast )
    {
        // mSunLight->setDirection( Ogre::Vector3( cosf( mTimeOfDay ), -sinf( mTimeOfDay ), -1.0
        // ).normalisedCopy() ); mSunLight->setDirection( Ogre::Vector3( 0, -sinf( mTimeOfDay ), -1.0
        // ).normalisedCopy() );
        mSunLight->setDirection(
            Ogre::Quaternion( Ogre::Radian( mAzimuth ), Ogre::Vector3::UNIT_Y ) *
            Ogre::Vector3( cosf( mTimeOfDay ), -sinf( mTimeOfDay ), 0.0 ).normalisedCopy() );
        // mSunLight->setDirection( -Ogre::Vector3::UNIT_Y );

#ifdef OGRE_BUILD_COMPONENT_ATMOSPHERE
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        OGRE_ASSERT_HIGH( dynamic_cast<Ogre::AtmosphereNpr *>( sceneManager->getAtmosphere() ) );
        Ogre::AtmosphereNpr *atmosphere =
            static_cast<Ogre::AtmosphereNpr *>( sceneManager->getAtmosphere() );
        atmosphere->setSunDir( mSunLight->getDirection(), mTimeOfDay / Ogre::Math::PI );
#endif

        // Do not call update() while invisible, as it will cause an assert because the frames
        // are not advancing, but we're still mapping the same GPU region over and over.
        if( mGraphicsSystem->getRenderWindow()->isVisible() )
        {
            // Force update the shadow map every frame to avoid the feeling we're "cheating" the
            // user in this sample with higher framerates than what he may encounter in many of
            // his possible uses.
            const float lightEpsilon = 0.0f;
            mTerra->update( mSunLight->getDerivedDirectionUpdated(), lightEpsilon );
        }

        TutorialGameState::update( timeSinceLast );

        // Camera must be locked to ground *after* we've moved it. Otherwise
        // fast motion may go below the terrain for 1 or 2 frames.
        Ogre::Camera *camera = mGraphicsSystem->getCamera();
        Ogre::Vector3 camPos = camera->getPosition();
        if( mLockCameraToGround && mTerra->getHeightAt( camPos ) )
            camera->setPosition( camPos + Ogre::Vector3::UNIT_Y * 10.0f );
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_TerrainGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
    {
        TutorialGameState::generateDebugText( timeSinceLast, outText );

        if( mDisplayHelpMode == 0 )
        {
            outText += "\nCtrl+F4 will reload Terra's shaders.";
        }
        else if( mDisplayHelpMode == 1 )
        {
            char tmp[128];
            Ogre::LwString str( Ogre::LwString::FromEmptyPointer( tmp, sizeof( tmp ) ) );
            Ogre::Vector3 camPos = mGraphicsSystem->getCamera()->getPosition();

            using namespace Ogre;

            outText += "\nF2 Lock Camera to Ground: [";
            outText += mLockCameraToGround ? "Yes]" : "No]";
            outText += "\nT Enable triplanar mapping: [";
            outText += mTriplanarMappingEnabled ? "Yes]" : "No]";
            outText += "\n+/- to change time of day. [";
            outText += StringConverter::toString( mTimeOfDay * 180.0f / Math::PI ) + "]";
            outText += "\n9/6 to change azimuth. [";
            outText += StringConverter::toString( mAzimuth * 180.0f / Math::PI ) + "]";
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
    void Tutorial_TerrainGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( arg.keysym.sym == SDLK_F4 && ( arg.keysym.mod & ( KMOD_LCTRL | KMOD_RCTRL ) ) )
        {
            // Hot reload of Terra shaders.
            Ogre::Root *root = mGraphicsSystem->getRoot();
            Ogre::HlmsManager *hlmsManager = root->getHlmsManager();

            Ogre::Hlms *hlms = hlmsManager->getHlms( Ogre::HLMS_USER3 );
            Ogre::GpuProgramManager::getSingleton().clearMicrocodeCache();
            hlms->reloadFrom( hlms->getDataFolder() );
        }
        else if( ( arg.keysym.mod & ~( KMOD_NUM | KMOD_CAPS ) ) != 0 )
        {
            TutorialGameState::keyReleased( arg );
            return;
        }

        if( arg.keysym.scancode == SDL_SCANCODE_KP_PLUS )
        {
            mTimeOfDay += 0.1f;
            mTimeOfDay = std::min( mTimeOfDay, (float)Ogre::Math::PI );
        }
        else if( arg.keysym.scancode == SDL_SCANCODE_MINUS ||
                 arg.keysym.scancode == SDL_SCANCODE_KP_MINUS )
        {
            mTimeOfDay -= 0.1f;
            mTimeOfDay = std::max( mTimeOfDay, 0.0f );
        }

        if( arg.keysym.scancode == SDL_SCANCODE_KP_9 )
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

        if( arg.keysym.scancode == SDL_SCANCODE_F2 )
        {
            mLockCameraToGround = !mLockCameraToGround;
        }
        else
        {
            TutorialGameState::keyReleased( arg );
        }

        if( arg.keysym.scancode == SDL_SCANCODE_T )
        {
            mTriplanarMappingEnabled = !mTriplanarMappingEnabled;

            Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
            Ogre::HlmsTerraDatablock *datablock = static_cast<Ogre::HlmsTerraDatablock *>(
                hlmsManager->getDatablock( "TerraExampleMaterial" ) );

            datablock->setDetailTriplanarDiffuseEnabled( mTriplanarMappingEnabled );
            datablock->setDetailTriplanarNormalEnabled( mTriplanarMappingEnabled );
            datablock->setDetailTriplanarRoughnessEnabled( mTriplanarMappingEnabled );
            datablock->setDetailTriplanarMetalnessEnabled( mTriplanarMappingEnabled );

            Ogre::Vector2 terrainDimensions = mTerra->getXZDimensions();

            Ogre::Vector4 detailMapOffsetScale[2];
            detailMapOffsetScale[0] = datablock->getDetailMapOffsetScale( 0 );
            detailMapOffsetScale[1] = datablock->getDetailMapOffsetScale( 1 );

            // Switch between "common" UV mapping and world coordinates-based UV mapping (and vice versa)
            if( mTriplanarMappingEnabled )
            {
                detailMapOffsetScale[0].z = 1.0f / ( terrainDimensions.x / detailMapOffsetScale[0].z );
                detailMapOffsetScale[0].w = 1.0f / ( terrainDimensions.y / detailMapOffsetScale[0].w );
                detailMapOffsetScale[1].z = 1.0f / ( terrainDimensions.x / detailMapOffsetScale[1].z );
                detailMapOffsetScale[1].w = 1.0f / ( terrainDimensions.y / detailMapOffsetScale[1].w );
            }
            else
            {
                detailMapOffsetScale[0].z *= terrainDimensions.x;
                detailMapOffsetScale[0].w *= terrainDimensions.y;
                detailMapOffsetScale[1].z *= terrainDimensions.x;
                detailMapOffsetScale[1].w *= terrainDimensions.y;
            }

            datablock->setDetailMapOffsetScale( 0, detailMapOffsetScale[0] );
            datablock->setDetailMapOffsetScale( 1, detailMapOffsetScale[1] );
        }
    }
}  // namespace Demo
