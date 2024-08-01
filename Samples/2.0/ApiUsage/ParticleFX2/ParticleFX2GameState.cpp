
#include "ParticleFX2GameState.h"

#include "CameraController.h"
#include "GraphicsSystem.h"
#include "OgreCamera.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsSamplerblock.h"
#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreTextureFilters.h"
#include "OgreTextureGpuManager.h"
#include "OgreWindow.h"
#include "ParticleSystem/OgreBillboardSet2.h"
#include "ParticleSystem/OgreParticleSystemManager2.h"

#include "OgreParticleSystem.h"

#include "ParticleSystem/OgreParticleSystem2.h"

using namespace Demo;

ParticleFX2GameState::ParticleFX2GameState( const Ogre::String &helpDescription ) :
    TutorialGameState( helpDescription ),
    mTime( 0.0f ),
    mCurrBlendingMethod( BlendingMethod::AlphaHashingA2C )
{
}
//-----------------------------------------------------------------------------------
void ParticleFX2GameState::createScene01()
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

    applyBlendingMethodToMaterials();

    {
        Ogre::ParticleSystem2 *pSystem3 = sceneManager->createParticleSystem2( "Particle/SpherePBS" );
        Ogre::SceneNode *sceneNode = rootNode->createChildSceneNode();
        sceneNode->attachObject( pSystem3 );
        sceneNode->setPosition( -150.0f, 0.0f, 0.0f );
    }

    Ogre::ParticleSystem2 *pSystem0 = sceneManager->createParticleSystem2( "Examples/PurpleFountain" );
    Ogre::ParticleSystem2 *pSystem1 = sceneManager->createParticleSystem2( "Examples/Aureola" );
    Ogre::ParticleSystem2 *pSystem2 = sceneManager->createParticleSystem2( "Particle/SmokePBS" );

    Ogre::BillboardSet *billboardSet = sceneManager->createBillboardSet2();
    // Try to be as tight as possible. In this sample we only need 1.
    billboardSet->setParticleQuota( 1u );
    billboardSet->setMaterialName( "Marble",
                                   Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
    billboardSet->init( sceneManager->getDestinationRenderSystem()->getVaoManager() );
    Ogre::Billboard billboard = billboardSet->allocBillboard();
    billboard.set( Ogre::Vector3::ZERO, Ogre::Vector3::NEGATIVE_UNIT_Z, Ogre::Vector2( 15.0f, 9.0f ),
                   Ogre::ColourValue::White );

    Ogre::SceneNode *sceneNode = rootNode->createChildSceneNode();
    sceneNode->attachObject( pSystem0 );
    sceneNode = rootNode->createChildSceneNode();
    sceneNode->attachObject( pSystem1 );
    sceneNode = rootNode->createChildSceneNode();
    sceneNode->setPosition( Ogre::Vector3( 10, 0, 0 ) );
    sceneNode->attachObject( pSystem2 );

    Ogre::Item *particleSystem3_item = sceneManager->createItem( planeMesh, Ogre::SCENE_DYNAMIC );
    particleSystem3_item->setDatablock( "Marble" );
    mParticleSystem3_RootSceneNode = rootNode->createChildSceneNode();
    sceneNode = mParticleSystem3_RootSceneNode->createChildSceneNode();
    sceneNode->setScale( Ogre::Vector3( 0.2f ) );
    sceneNode->attachObject( particleSystem3_item );

    Ogre::ParticleSystem2 *pSystem3 = sceneManager->createParticleSystem2( "Particle/SmokeUnlit" );
    mParticleSystem3_RootSceneNode->attachObject( pSystem3 );
    mParticleSystem3_EmmitterSceneNode = mParticleSystem3_RootSceneNode->createChildSceneNode();
    Ogre::Item *particleSystem3_emmiter_item =
        sceneManager->createItem( planeMesh, Ogre::SCENE_DYNAMIC );
    particleSystem3_emmiter_item->setDatablock( "Marble" );
    mParticleSystem3_EmmitterSceneNode->setScale( Ogre::Vector3( 0.01f ) );
    mParticleSystem3_EmmitterSceneNode->attachObject( particleSystem3_emmiter_item );

    mGraphicsSystem->getCamera()->setPosition( Ogre::Vector3( 0.0f, 40.0f, 120.0f ) );

    mCameraController = new CameraController( mGraphicsSystem, false );

    TutorialGameState::createScene01();
}
//-----------------------------------------------------------------------------
void ParticleFX2GameState::update( float timeSinceLast )
{
    mTime += timeSinceLast;

    if( mTime > 10.0f )
        mTime = 0.0f;

    mParticleSystem3_RootSceneNode->setPosition(
        Ogre::Vector3( -50.0f * mTime / 10.0f + 25.0f, 2, -50 ) );
    mParticleSystem3_EmmitterSceneNode->setPosition(
        Ogre::Vector3( 20.0f * mTime / 10.0f - 10.f, 0.5, 20.0f * mTime / 10.0f - 10 ) );

    // Tell the ParticleSystem where the camera is.
    Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
    sceneManager->getParticleSystemManager2()->setCameraPosition(
        mGraphicsSystem->getCamera()->getDerivedPosition() );

    TutorialGameState::update( timeSinceLast );
}
//-----------------------------------------------------------------------------
void ParticleFX2GameState::applyBlendingMethodToMaterials()
{
    Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
    Ogre::HlmsDatablock *datablock;

    datablock = hlmsManager->getDatablock( "Particle/SmokeUnlit" );
    applyBlendingMethodToMaterial( datablock );
    datablock = hlmsManager->getDatablock( "Particle/SmokePBS" );
    applyBlendingMethodToMaterial( datablock );
    datablock = hlmsManager->getDatablock( "Particle/SpherePBS" );
    applyBlendingMethodToMaterial( datablock );
}
//-----------------------------------------------------------------------------
void ParticleFX2GameState::applyBlendingMethodToMaterial( Ogre::HlmsDatablock *datablock )
{
    using namespace Ogre;

    if( mCurrBlendingMethod == BlendingMethod::AlphaHashing ||
        mCurrBlendingMethod == BlendingMethod::AlphaHashingA2C )
    {
        HlmsBlendblock blendblock = *datablock->getBlendblock();
        blendblock.setBlendType( SBT_REPLACE );
        blendblock.mAlphaToCoverage = mCurrBlendingMethod == BlendingMethod::AlphaHashingA2C
                                          ? HlmsBlendblock::A2cEnabledMsaaOnly
                                          : HlmsBlendblock::A2cDisabled;
        datablock->setBlendblock( blendblock );

        HlmsMacroblock macroblock = *datablock->getMacroblock();
        macroblock.mDepthWrite = true;
        datablock->setMacroblock( macroblock );
        datablock->setAlphaHashing( true );
    }
    else
    {
        HlmsBlendblock blendblock = *datablock->getBlendblock();
        blendblock.setBlendType( SBT_TRANSPARENT_ALPHA );
        blendblock.mAlphaToCoverage = HlmsBlendblock::A2cDisabled;
        datablock->setBlendblock( blendblock );

        HlmsMacroblock macroblock = *datablock->getMacroblock();
        macroblock.mDepthWrite = false;
        datablock->setMacroblock( macroblock );
        datablock->setAlphaHashing( false );
    }
}
//-----------------------------------------------------------------------------
void ParticleFX2GameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
{
    TutorialGameState::generateDebugText( timeSinceLast, outText );
    outText += "\nPress F2 to toggle blending methods. ";
    switch( mCurrBlendingMethod )
    {
    case BlendingMethod::NumBlendingMethods:
    case BlendingMethod::AlphaHashing:
        outText += "[Alpha Hashing]";
        break;
    case BlendingMethod::AlphaHashingA2C:
        outText += "[Alpha Hashing + A2C]";
        break;
    case BlendingMethod::AlphaBlending:
        outText += "[Alpha Blending]";
        break;
    }
}
//-----------------------------------------------------------------------------
void ParticleFX2GameState::keyReleased( const SDL_KeyboardEvent &arg )
{
    if( ( arg.keysym.mod & ~( KMOD_NUM | KMOD_CAPS | KMOD_LSHIFT | KMOD_RSHIFT ) ) != 0 )
    {
        TutorialGameState::keyReleased( arg );
        return;
    }

    if( arg.keysym.sym == SDLK_F2 )
    {
        const bool reverse = ( arg.keysym.mod & ( KMOD_LSHIFT | KMOD_RSHIFT ) ) != 0;
        if( reverse )
        {
            mCurrBlendingMethod = static_cast<BlendingMethod::BlendingMethod>(
                ( mCurrBlendingMethod + BlendingMethod::NumBlendingMethods - 1u ) %
                BlendingMethod::NumBlendingMethods );
        }
        else
        {
            mCurrBlendingMethod = static_cast<BlendingMethod::BlendingMethod>(
                ( mCurrBlendingMethod + 1u ) % BlendingMethod::NumBlendingMethods );
        }

        applyBlendingMethodToMaterials();
    }
    else
    {
        TutorialGameState::keyReleased( arg );
    }
}
