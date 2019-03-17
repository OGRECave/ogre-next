
#include "Tutorial_TextureBakingGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreSceneManager.h"
#include "OgreItem.h"

#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreMesh2.h"
#include "OgreSubMesh2.h"

#include "OgreCamera.h"
#include "OgreRenderWindow.h"

#include "OgreHlmsUnlitDatablock.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsSamplerblock.h"

#include "OgreRoot.h"
#include "OgreHlmsManager.h"
#include "OgreHlms.h"
#include "OgreHlmsPbs.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorShadowNode.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h"

#include "OgreTextureGpuManager.h"
#include "OgreTextureFilters.h"
#include "OgreTextureBox.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreWindow.h"

#include "OgreWireAabb.h"

#include "OgreWireAabb.h"

using namespace Demo;

namespace Demo
{
    static const Ogre::uint32 c_areaLightsPoolId = 759384;
    static const Ogre::uint32 c_defaultWidth = 512u;
    static const Ogre::uint32 c_defaultHeight = 512u;
    static const Ogre::PixelFormatGpu c_defaultFormat = Ogre::PFG_RGBA8_UNORM_SRGB;
    static const Ogre::uint8 c_defaultNumMipmaps =
            Ogre::PixelFormatGpuUtils::getMaxMipmapCount( c_defaultWidth, c_defaultHeight );

    //We only want to bake the objects that have c_bakedObjVisibilityFlags set
    static const Ogre::uint32 c_bakedObjVisibilityFlags     = 1u << 20u;
    static const Ogre::uint32 c_renderObjVisibilityFlags    = 1u << 0u;
    static const Ogre::uint32 c_lightPlanesVisibilityFlag   = 1u << 1u;

    Tutorial_TextureBakingGameState::Tutorial_TextureBakingGameState(
            const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mAreaMaskTex( 0 ),
        mBakedResult( 0 ),
        mBakedWorkspace( 0 ),
        mShowBakedTexWorkspace( 0 ),
        mFloorRender( 0 ),
        mFloorBaked( 0 ),
        mRenderingMode( RenderingMode::ShowRenderScene ),
        mBakeEveryFrame( false )
    {
        memset( mAreaLights, 0, sizeof( mAreaLights ) );
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_TextureBakingGameState::createAreaPlaneMesh(void)
    {
        Ogre::v1::MeshPtr lightPlaneMeshV1 =
                Ogre::v1::MeshManager::getSingleton().createPlane( "LightPlane v1",
                                            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                            Ogre::Plane( Ogre::Vector3::UNIT_Z, 0.0f ), 1.0f, 1.0f,
                                            1, 1, true, 1, 1.0f, 1.0f, Ogre::Vector3::UNIT_Y,
                                            Ogre::v1::HardwareBuffer::HBU_STATIC,
                                            Ogre::v1::HardwareBuffer::HBU_STATIC );
        Ogre::MeshPtr lightPlaneMesh = Ogre::MeshManager::getSingleton().createManual(
                    "LightPlane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
        lightPlaneMesh->importV1( lightPlaneMeshV1.get(), true, true, true );

        Ogre::v1::MeshManager::getSingleton().remove( lightPlaneMeshV1 );
    }
    //-----------------------------------------------------------------------------------
    Ogre::HlmsDatablock* Tutorial_TextureBakingGameState::setupDatablockTextureForLight(
            Ogre::Light *light, size_t idx )
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::Hlms *hlmsUnlit = root->getHlmsManager()->getHlms( Ogre::HLMS_UNLIT );

        //Setup an unlit material, double-sided, with textures
        //(if it has one) and same colour as the light.
        //IMPORTANT: these materials are never destroyed once they're not needed (they will
        //be destroyed by Ogre on shutdown). Watchout for this to prevent memory leaks in
        //a real implementation
        const Ogre::String materialName = "LightPlane Material" +
                                          Ogre::StringConverter::toString( idx );
        Ogre::HlmsMacroblock macroblock;
        macroblock.mCullMode = Ogre::CULL_NONE;
        Ogre::HlmsDatablock *datablockBase = hlmsUnlit->getDatablock( materialName );

        if( !datablockBase )
        {
            datablockBase = hlmsUnlit->createDatablock( materialName, materialName, macroblock,
                                                        Ogre::HlmsBlendblock(), Ogre::HlmsParamVec() );
        }

        assert( dynamic_cast<Ogre::HlmsUnlitDatablock*>( datablockBase ) );
        Ogre::HlmsUnlitDatablock *datablock = static_cast<Ogre::HlmsUnlitDatablock*>( datablockBase );

        if( light->mTextureLightMaskIdx != std::numeric_limits<Ogre::uint16>::max() &&
            light->getType() == Ogre::Light::LT_AREA_APPROX )
        {
            Ogre::HlmsSamplerblock samplerblock;
            samplerblock.mMaxAnisotropy = 8.0f;
            samplerblock.setFiltering( Ogre::TFO_ANISOTROPIC );

            datablock->setTexture( 0, light->getTexture(), &samplerblock );
        }

        datablock->setUseColour( true );
        datablock->setColour( light->getDiffuseColour() );

        return datablock;
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_TextureBakingGameState::createPlaneForAreaLight( Ogre::Light *light, size_t idx )
    {
        Ogre::HlmsDatablock *datablock = setupDatablockTextureForLight( light, idx );

        //Create the plane Item
        Ogre::SceneNode *lightNode = light->getParentSceneNode();
        Ogre::SceneNode *planeNode = lightNode->createChildSceneNode();

        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        Ogre::Item *item = sceneManager->createItem(
                               "LightPlane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
        item->setCastShadows( false );
        item->setDatablock( datablock );
        item->setVisibilityFlags( c_lightPlanesVisibilityFlag );
        planeNode->attachObject( item );

        //Math the plane size to that of the area light
        const Ogre::Vector2 rectSize = light->getRectSize();
        planeNode->setScale( rectSize.x, rectSize.y, 1.0f );

        /* For debugging ranges & AABBs
        Ogre::WireAabb *wireAabb = sceneManager->createWireAabb();
        wireAabb->track( light );*/
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_TextureBakingGameState::createLight( const Ogre::Vector3 &position, size_t idx )
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        Ogre::SceneNode *rootNode = sceneManager->getRootSceneNode();

        Ogre::Light *light = sceneManager->createLight();
        Ogre::SceneNode *lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( light );
        light->setDiffuseColour( 1.0f, 1.0f, 1.0f );
        light->setSpecularColour( 1.0f, 1.0f, 1.0f );
        //Increase the strength 10x to showcase this light. Area approx lights are not
        //physically based so the value is more arbitrary than the other light types
        light->setPowerScale( Ogre::Math::PI );
        if( idx == 0 )
            light->setType( Ogre::Light::LT_AREA_LTC );
        else
            light->setType( Ogre::Light::LT_AREA_APPROX );
        light->setRectSize( Ogre::Vector2( 5.0f, 5.0f ) );
        lightNode->setPosition( position );
        light->setDirection( Ogre::Vector3( 0, 0, 1 ).normalisedCopy() );
        light->setAttenuationBasedOnRadius( 10.0f, 0.01f );

//        //Control the diffuse mip (this is the default value)
//        light->mTexLightMaskDiffuseMipStart = (Ogre::uint16)(0.95f * 65535);

        createPlaneForAreaLight( light, idx );

        mAreaLights[idx] = light;
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_TextureBakingGameState::setupLightTexture( size_t idx )
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();

        const Ogre::String aliasName = "AreaLightMask" + Ogre::StringConverter::toString( idx );
        Ogre::TextureGpu *areaTex = textureMgr->findTextureNoThrow( aliasName );
        if( areaTex )
            textureMgr->destroyTexture( areaTex );

        //We know beforehand that floor_bump.PNG & co are 512x512. This is important!!!
        //(because it must match the resolution of the texture created via reservePoolId)
        const char *textureNames[4] = { "floor_bump.PNG", "grassWalpha.tga",
                                        "MtlPlat2.jpg", "Panels_Normal_Obj.png" };

        areaTex = textureMgr->createOrRetrieveTexture(
                      textureNames[idx % 4u],
                      "AreaLightMask" + Ogre::StringConverter::toString( idx ),
                      Ogre::GpuPageOutStrategy::Discard,
                      Ogre::CommonTextureTypes::Diffuse,
                      Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                      c_areaLightsPoolId );
        mAreaLights[idx]->setTexture( areaTex );

        setupDatablockTextureForLight( mAreaLights[idx], idx );

        //We must wait otherwise the bake will have wrong results.
        textureMgr->waitForStreamingCompletion();
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_TextureBakingGameState::createBakingTexture(void)
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();

        //Create texture
        mBakedResult = textureMgr->createOrRetrieveTexture( "BakingResult",
                                                            Ogre::GpuPageOutStrategy::SaveToSystemRam,
                                                            Ogre::TextureFlags::RenderToTexture,
                                                            Ogre::TextureTypes::Type2DArray );
        mBakedResult->setResolution( 512u, 512u, 1u );
        mBakedResult->setPixelFormat( Ogre::PFG_RGBA8_UNORM_SRGB );
        mBakedResult->setNumMipmaps( 1u );

        mBakedResult->scheduleTransitionTo( Ogre::GpuResidency::Resident );

        //Create workspace that will render to the texture
        Ogre::CompositorManager2 *compositorManager = root->getCompositorManager2();
        mBakedWorkspace = compositorManager->addWorkspace( sceneManager, mBakedResult,
                                                           mGraphicsSystem->getCamera(),
                                                           "UvBakingWorkspace", false );

        //Create a material that can be used to view those results
        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );

        Ogre::String datablockName = "BakeResultMaterial";
        Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                    hlmsPbs->createDatablock( datablockName,
                                              datablockName,
                                              Ogre::HlmsMacroblock(),
                                              Ogre::HlmsBlendblock(),
                                              Ogre::HlmsParamVec() ) );
        datablock->setTexture( Ogre::PBSM_EMISSIVE, mBakedResult );

        Ogre::CompositorChannelVec externalRenderTargets;
        externalRenderTargets.push_back( mGraphicsSystem->getRenderWindow()->getTexture() );
        externalRenderTargets.push_back( mBakedResult );
        mShowBakedTexWorkspace = compositorManager->addWorkspace( sceneManager, externalRenderTargets,
                                                                  mGraphicsSystem->getCamera(),
                                                                  "ShowBakingTextureWorkspace", false );
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_TextureBakingGameState::updateBakingTexture(void)
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        const Ogre::uint32 oldMask = sceneManager->getVisibilityMask();
        const Ogre::uint32 oldLightMask = sceneManager->getLightMask();
        const Ogre::ColourValue upperHemi = sceneManager->getAmbientLightUpperHemisphere();
        const Ogre::ColourValue lowerHemi = sceneManager->getAmbientLightLowerHemisphere();
        const Ogre::Vector3 hemiDir = sceneManager->getAmbientLightHemisphereDir();
        const Ogre::Real envmapScale = upperHemi.a;
        const Ogre::uint32 envFeatures = sceneManager->getEnvFeatures();

        sceneManager->setVisibilityMask( c_renderObjVisibilityFlags );
        sceneManager->setLightMask( 0xFFFFFFFFu );
        sceneManager->setAmbientLight( Ogre::ColourValue::Black, Ogre::ColourValue::Black,
                                       Ogre::Vector3::UNIT_Y, envmapScale );

        sceneManager->updateSceneGraph();

        mBakedWorkspace->_beginUpdate( false );
        mBakedWorkspace->_update();
        mBakedWorkspace->_endUpdate( false );

        sceneManager->clearFrameData();

        sceneManager->setVisibilityMask( oldMask );
        sceneManager->setLightMask( oldLightMask );
        sceneManager->setAmbientLight( upperHemi, lowerHemi, hemiDir, envmapScale, envFeatures );
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_TextureBakingGameState::createScene01(void)
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();

        //Reserve/create the texture for the area lights
        mAreaMaskTex = textureMgr->reservePoolId( c_areaLightsPoolId,
                                                  c_defaultWidth, c_defaultHeight,
                                                  c_numAreaLights,
                                                  c_defaultNumMipmaps,
                                                  c_defaultFormat );
        //Set the texture mask to PBS.
        Ogre::Hlms *hlms = root->getHlmsManager()->getHlms( Ogre::HLMS_PBS );
        assert( dynamic_cast<Ogre::HlmsPbs*>( hlms ) );
        Ogre::HlmsPbs *pbs = static_cast<Ogre::HlmsPbs*>( hlms );

        pbs->setAreaLightMasks( mAreaMaskTex );
        pbs->setAreaLightForwardSettings( c_numAreaLights - 1u, 1u );

        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        //Setup the floor
        Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane( "Plane v1",
                                            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                            Ogre::Plane( Ogre::Vector3::UNIT_Y, 1.0f ), 50.0f, 50.0f,
                                            1, 1, true, 1, 1.0f, 1.0f, Ogre::Vector3::UNIT_Z,
                                            Ogre::v1::HardwareBuffer::HBU_STATIC,
                                            Ogre::v1::HardwareBuffer::HBU_STATIC );

        Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createManual(
                    "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

        planeMesh->importV1( planeMeshV1.get(), true, true, true );

        createBakingTexture();

        {
            {
                //Create FloorMaterial, but use "none" as culling mode so all the faces
                //can be baked correctly, while leaving the caster's culling mode intact;
                //otherwise there is a lot of shadow acne.
                Ogre::HlmsDatablock *defaultMaterial = pbs->getDefaultDatablock();
                Ogre::HlmsDatablock *floorMat = defaultMaterial->clone( "FloorMaterial" );

                Ogre::HlmsMacroblock macroblock = *floorMat->getMacroblock();
                Ogre::HlmsMacroblock casterMacroblock = *floorMat->getMacroblock( true );

                macroblock.mCullMode = Ogre::CULL_NONE;
                floorMat->setMacroblock( macroblock, false );
                floorMat->setMacroblock( casterMacroblock, true );
            }

            mFloorRender = sceneManager->createItem( planeMesh, Ogre::SCENE_DYNAMIC );
            mFloorRender->setDatablock( "FloorMaterial" );
            Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                                                    createChildSceneNode( Ogre::SCENE_DYNAMIC );
            sceneNode->setPosition( 0, -1, 0 );
            sceneNode->attachObject( mFloorRender );
            mFloorRender->setVisibilityFlags( c_renderObjVisibilityFlags );
        }
        {
            mFloorBaked = sceneManager->createItem( planeMesh, Ogre::SCENE_DYNAMIC );
            mFloorBaked->setDatablock( "BakeResultMaterial" );
            mFloorRender->getParentSceneNode()->attachObject( mFloorBaked );
            mFloorBaked->setVisibilityFlags( c_bakedObjVisibilityFlags );
            mFloorBaked->setLightMask( 0 );
        }

        //Create the mesh template for all the lights (i.e. the billboard-like plane)
        createAreaPlaneMesh();

        Ogre::SceneNode *rootNode = sceneManager->getRootSceneNode();

        //Main directional light
        Ogre::Light *light = sceneManager->createLight();
        Ogre::SceneNode *lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( light );
        light->setPowerScale( 1.0f );
        light->setType( Ogre::Light::LT_DIRECTIONAL );
        light->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );

        for( size_t i=0; i<c_numAreaLights; ++i )
        {
            createLight( Ogre::Vector3( (i - (c_numAreaLights-1u) * 0.5f) * 10, 4.0f, 0.0f ), i );
            setupLightTexture( i );
        }

        mCameraController = new CameraController( mGraphicsSystem, false );

        Ogre::Camera *camera = mGraphicsSystem->getCamera();
        camera->setPosition( Ogre::Vector3( 0, 10, 25 ) );

        updateRenderingMode();

        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_TextureBakingGameState::destroyScene(void)
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();

        for( size_t i=0; i<c_numAreaLights; ++i )
        {
            if( mAreaLights[i] )
                textureMgr->destroyTexture( mAreaLights[i]->getTexture() );
        }

        //Don't forget to destroy mAreaMaskTex, otherwise this pool will leak!!!
        if( mAreaMaskTex )
        {
            textureMgr->destroyTexture( mAreaMaskTex );
            mAreaMaskTex = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_TextureBakingGameState::update( float timeSinceLast )
    {
        if( mBakeEveryFrame && mRenderingMode != RenderingMode::ShowRenderScene )
            updateBakingTexture();
        TutorialGameState::update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_TextureBakingGameState::updateRenderingMode(void)
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        sceneManager->setLightMask( 0xffffffff );

        if( mRenderingMode != RenderingMode::ShowBakedTexture )
        {
            mShowBakedTexWorkspace->setEnabled( false );
            mGraphicsSystem->getCompositorWorkspace()->setEnabled( true );
        }

        if( mRenderingMode == RenderingMode::ShowRenderScene )
        {
            sceneManager->setVisibilityMask( c_renderObjVisibilityFlags | c_lightPlanesVisibilityFlag );
        }
        else if( mRenderingMode == RenderingMode::ShowSceneWithBakedTexture )
        {
            sceneManager->setVisibilityMask( c_bakedObjVisibilityFlags | c_lightPlanesVisibilityFlag );
#if OGRE_NO_FINE_LIGHT_MASK_GRANULARITY
            sceneManager->setLightMask( 0 );
#endif
        }
        else
        {
            mShowBakedTexWorkspace->setEnabled( true );
            mGraphicsSystem->getCompositorWorkspace()->setEnabled( false );
        }
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_TextureBakingGameState::generateDebugText( float timeSinceLast,
                                                                    Ogre::String &outText )
    {
        TutorialGameState::generateDebugText( timeSinceLast, outText );
        outText += "\nPress F2 to show rendered scene";
        if( mRenderingMode == RenderingMode::ShowRenderScene )
            outText += " [Active]";
        outText += "\nPress F3 to show scene with baking";
        if( mRenderingMode == RenderingMode::ShowSceneWithBakedTexture )
            outText += " [Active]";
        outText += "\nPress F4 to show baked texture";
        if( mRenderingMode == RenderingMode::ShowBakedTexture )
            outText += " [Active]";

        if( mRenderingMode != RenderingMode::ShowRenderScene )
        {
            outText += "\nPress F5 to update baking results every frame ";
            outText += mBakeEveryFrame ? "[On]" : "[Off]";
        }
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_TextureBakingGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( (arg.keysym.mod & ~(KMOD_NUM|KMOD_CAPS|KMOD_LSHIFT|KMOD_RSHIFT)) != 0 )
        {
            TutorialGameState::keyReleased( arg );
            return;
        }

        if( arg.keysym.sym == SDLK_F2 )
        {
            mRenderingMode = RenderingMode::ShowRenderScene;
            updateRenderingMode();
        }
        else if( arg.keysym.sym == SDLK_F3 )
        {
            mRenderingMode = RenderingMode::ShowSceneWithBakedTexture;
            updateBakingTexture();
            updateRenderingMode();
        }
        else if( arg.keysym.sym == SDLK_F4 )
        {
            mRenderingMode = RenderingMode::ShowBakedTexture;
            updateBakingTexture();
            updateRenderingMode();
        }
        else if( arg.keysym.sym == SDLK_F5 )
        {
            mBakeEveryFrame = !mBakeEveryFrame;
        }
        else
        {
            TutorialGameState::keyReleased( arg );
        }
    }
}
