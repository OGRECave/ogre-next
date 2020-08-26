
#include "SceneFormatGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreSceneManager.h"
#include "OgreItem.h"

#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreMesh2.h"

#include "OgreCamera.h"

#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsSamplerblock.h"

#include "OgreRoot.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"

#include "OgreTextureGpuManager.h"
#include "OgreTextureFilters.h"

#include "OgreSceneFormatExporter.h"
#include "OgreSceneFormatImporter.h"
#include "OgreForwardPlusBase.h"

#include "../LocalCubemaps/LocalCubemapScene.h"
#include "Compositor/OgreCompositorManager2.h"
#include "Cubemaps/OgreParallaxCorrectedCubemap.h"

#include "InstantRadiosity/OgreInstantRadiosity.h"
#include "OgreIrradianceVolume.h"

#include "OgreDecal.h"
#include "OgreWireAabb.h"
#include "OgreTextureGpuManager.h"

#include "OgreFileSystemLayer.h"

using namespace Demo;

namespace Demo
{
    SceneFormatGameState::SceneFormatGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mInstantRadiosity( 0 ),
        mIrradianceVolume( 0 ),
        mParallaxCorrectedCubemap( 0 )
    {
        Ogre::FileSystemLayer filesystemLayer( OGRE_VERSION_NAME );
        mFullpathToFile = filesystemLayer.getWritablePath( "scene_format_test_scene" );
    }
    //-----------------------------------------------------------------------------------
    void SceneFormatGameState::destroyInstantRadiosity(void)
    {
        if( mIrradianceVolume )
        {
            if( mIrradianceVolume )
            {
                Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
                Ogre::Hlms *hlms = hlmsManager->getHlms( Ogre::HLMS_PBS );
                assert( dynamic_cast<Ogre::HlmsPbs*>( hlms ) );

                Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlms );

                if( hlmsPbs && hlmsPbs->getIrradianceVolume() == mIrradianceVolume )
                    hlmsPbs->setIrradianceVolume( 0 );

                delete mIrradianceVolume;
                mIrradianceVolume = 0;
            }
        }

        delete mInstantRadiosity;
        mInstantRadiosity = 0;
    }
    //-----------------------------------------------------------------------------------
    void SceneFormatGameState::destroyParallaxCorrectCubemaps(void)
    {
        if( mParallaxCorrectedCubemap )
        {
            Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
            Ogre::Hlms *hlms = hlmsManager->getHlms( Ogre::HLMS_PBS );
            assert( dynamic_cast<Ogre::HlmsPbs*>( hlms ) );

            Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlms );

            if( hlmsPbs && hlmsPbs->getParallaxCorrectedCubemap() == mParallaxCorrectedCubemap )
                hlmsPbs->setParallaxCorrectedCubemap( 0 );

            delete mParallaxCorrectedCubemap;
            mParallaxCorrectedCubemap = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void SceneFormatGameState::resetScene(void)
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        destroyInstantRadiosity();
        destroyParallaxCorrectCubemaps();
        sceneManager->clearScene( false );

        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::TextureGpuManager *textureManager = root->getRenderSystem()->getTextureGpuManager();
        Ogre::TextureGpu *texture = 0;
        texture = textureManager->findTextureNoThrow( "decals_disabled_normals" );
        if( texture )
            texture->scheduleTransitionTo( Ogre::GpuResidency::OnStorage );
        texture = textureManager->findTextureNoThrow( "RawDecalTextureTest" );
        if( texture )
            texture->scheduleTransitionTo( Ogre::GpuResidency::OnStorage );
    }
    //-----------------------------------------------------------------------------------
    void SceneFormatGameState::setupParallaxCorrectCubemaps(void)
    {
        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms(Ogre::HLMS_PBS) );

        if( mParallaxCorrectedCubemap )
        {
            hlmsPbs->setParallaxCorrectedCubemap( 0 );

            delete mParallaxCorrectedCubemap;
            mParallaxCorrectedCubemap = 0;
        }

        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::CompositorManager2 *compositorManager = root->getCompositorManager2();
        Ogre::CompositorWorkspaceDef *workspaceDef = compositorManager->getWorkspaceDefinition(
                    "LocalCubemapsProbeWorkspace" );

        mParallaxCorrectedCubemap = new Ogre::ParallaxCorrectedCubemap(
                    Ogre::Id::generateNewId<Ogre::ParallaxCorrectedCubemap>(),
                    mGraphicsSystem->getRoot(),
                    mGraphicsSystem->getSceneManager(),
                    workspaceDef, 250, 1u << 25u );

        mParallaxCorrectedCubemap->setEnabled( true, 1024, 1024, Ogre::PFG_RGBA8_UNORM_SRGB );

        Ogre::CubemapProbe *probe = 0;
        Ogre::Aabb roomShape( Ogre::Vector3( -0.505, 3.400016, 5.066226 ),
                              Ogre::Vector3( 5.064587, 3.891282, 9.556003 ) );
        Ogre::Aabb probeArea;
        probeArea.mHalfSize = Ogre::Vector3( 5.064587, 3.891282, 3.891282 );

        const bool useMultipleProbes = true;

        if( useMultipleProbes )
        {
            //Probe 00
            probe = mParallaxCorrectedCubemap->createProbe();
            probe->setTextureParams( 1024, 1024 );
            probe->initWorkspace();

            probeArea.mCenter = Ogre::Vector3( -0.505, 3.400016, -0.598495 );
            probe->set( probeArea.mCenter, probeArea, Ogre::Vector3( 1.0f, 1.0f, 0.3f ),
                        Ogre::Matrix3::IDENTITY, roomShape );
        }

        //Probe 01
        probe = mParallaxCorrectedCubemap->createProbe();
        probe->setTextureParams( 1024, 1024 );
        probe->initWorkspace();

        probeArea.mCenter = Ogre::Vector3( -0.505, 3.400016, 5.423867 );
        probe->set( useMultipleProbes ? probeArea.mCenter : roomShape.mCenter,
                    useMultipleProbes ? probeArea : roomShape,
                    Ogre::Vector3( 1.0f, 1.0f, 0.3f ),
                    Ogre::Matrix3::IDENTITY, roomShape );

        if( useMultipleProbes )
        {
            //Probe 02
            probe = mParallaxCorrectedCubemap->createProbe();
            probe->setTextureParams( 1024, 1024 );
            probe->initWorkspace();

            probeArea.mCenter = Ogre::Vector3( -0.505, 3.400016, 10.657585 );
            probe->set( probeArea.mCenter, probeArea, Ogre::Vector3( 1.0f, 1.0f, 0.3f ),
                        Ogre::Matrix3::IDENTITY, roomShape );
        }

        hlmsPbs->setParallaxCorrectedCubemap( mParallaxCorrectedCubemap );
    }
    //-----------------------------------------------------------------------------------
    Ogre::TextureGpu* SceneFormatGameState::createRawDecalDiffuseTex()
    {
        //The diffuse texture create it in RAW mode, just to test it. That is, we create the 2D Array
        //texture manually, without the aid of HlmsTextureManager.
        //Because of simplicity, we use two Image instances, one to load the diffuse texture,
        //another to create an array of 8 slices, with the other 7 slices set to black.
        //It's not efficient, but this is for testing
        Ogre::Image2 origImage;
        //Load floor diffuse
        origImage.load( "floor_diffuse.PNG",
                        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
        origImage.generateMipmaps( true );

        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();

        //Create raw texture
        Ogre::TextureGpu *rawTex =
                textureMgr->createOrRetrieveTexture(
                    "RawDecalTextureTest", Ogre::GpuPageOutStrategy::SaveToSystemRam,
                    Ogre::TextureFlags::ManualTexture, Ogre::TextureTypes::Type2DArray );
        rawTex->setResolution( origImage.getWidth(), origImage.getHeight(), 8u );
        rawTex->setPixelFormat( Ogre::PixelFormatGpuUtils::
                                getEquivalentSRGB( origImage.getPixelFormat() ) );
        rawTex->setNumMipmaps( origImage.getNumMipmaps() );
        rawTex->scheduleTransitionTo( Ogre::GpuResidency::Resident );

        //Upload floor diffuse
        origImage.uploadTo( rawTex, 0, origImage.getNumMipmaps() - 1u, 0u );

        //Load grass diffuse
        origImage.load( "grassWalpha.tga",
                        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
        origImage.generateMipmaps( true );
        origImage.uploadTo( rawTex, 0, origImage.getNumMipmaps() - 1u, 1u );

        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        sceneManager->setDecalsDiffuse( rawTex );
        return rawTex;
    }
    //-----------------------------------------------------------------------------------
    void SceneFormatGameState::generateScene(void)
    {
        destroyInstantRadiosity();
        destroyParallaxCorrectCubemaps();

        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        const float armsLength = 2.5f;

        {
            //Ogre::WireAabb *wireAabb = sceneManager->createWireAabb();

            //Create the floor decal
            Ogre::Decal *decal = sceneManager->createDecal();
            Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode();
            sceneNode->attachObject( decal );
            sceneNode->setPosition( Ogre::Vector3( 0, 0.0, 5.0f ) );
            sceneNode->setOrientation( Ogre::Quaternion( Ogre::Degree( 45.0f ),
                                                         Ogre::Vector3::UNIT_Y ) );
            sceneNode->setScale( Ogre::Vector3( 5.0f, 0.5f, 5.0f ) );
            //wireAabb->track( decal );

            Ogre::TextureGpu* rawTex = createRawDecalDiffuseTex();
            decal->setDiffuseTextureRaw( rawTex, 0u ); //Slice 0 = floor

            //The normal map create it in managed mode, that is with
            //the help of TextureGpuManager's AutomaticBatching
            const Ogre::uint32 decalNormalId = 1;
            Ogre::Root *root = mGraphicsSystem->getRoot();
            Ogre::TextureGpuManager *textureManager = root->getRenderSystem()->getTextureGpuManager();

            Ogre::TextureGpu *textureNorm = 0;
            textureNorm = textureManager->createOrRetrieveTexture(
                              "floor_bump.PNG", Ogre::GpuPageOutStrategy::Discard,
                              Ogre::CommonTextureTypes::NormalMap,
                              Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                              decalNormalId );
            textureNorm->scheduleTransitionTo( Ogre::GpuResidency::Resident );

            decal->setNormalTexture( textureNorm );
            sceneManager->setDecalsNormals( textureNorm );

            //Create the grass decal
            decal = sceneManager->createDecal();
            sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode();
            sceneNode->attachObject( decal );
            sceneNode->setPosition( Ogre::Vector3( 0, 0.0, 5.0f ) );
            sceneNode->setOrientation( Ogre::Quaternion( Ogre::Degree( 45.0f ),
                                                         Ogre::Vector3::UNIT_Y ) );
            sceneNode->setScale( Ogre::Vector3( 2.0f, 0.5f, 2.0f ) );

            decal->setDiffuseTextureRaw( rawTex, 1u ); //Slice 1 = grass

            //The normal map create it in managed mode, that is with
            //the help of TextureGpuManager's AutomaticBatching
            //However we create a black image to deal with no normal maps
            Ogre::Image2 blackImage;
            textureNorm->waitForMetadata(); //We need to call getWidth & getHeight
            const Ogre::uint32 normalTexWidth = textureNorm->getWidth();
            const Ogre::uint32 normalTexHeight = textureNorm->getHeight();
            Ogre::uint8 *blackBuffer = reinterpret_cast<Ogre::uint8*>(
                                           OGRE_MALLOC_SIMD( normalTexWidth * normalTexHeight * 2u,
                                                             Ogre::MEMCATEGORY_RESOURCE ) );
            memset( blackBuffer, 0, normalTexWidth * normalTexHeight * 2u );
            blackImage.loadDynamicImage( blackBuffer, normalTexWidth, normalTexHeight, 1u,
                                         Ogre::TextureTypes::Type2D, Ogre::PFG_RG8_SNORM, true );
            blackImage.generateMipmaps( false, Ogre::Image2::FILTER_NEAREST );
            textureNorm = textureManager->createOrRetrieveTexture(
                               "decals_disabled_normals",
                               Ogre::GpuPageOutStrategy::Discard,
                               Ogre::TextureFlags::AutomaticBatching |
                               Ogre::TextureFlags::ManualTexture,
                               Ogre::TextureTypes::Type2D, Ogre::BLANKSTRING, 0, decalNormalId );
            textureNorm->setResolution( blackImage.getWidth(), blackImage.getHeight() );
            textureNorm->setNumMipmaps( blackImage.getNumMipmaps() );
            textureNorm->setPixelFormat( blackImage.getPixelFormat() );
            textureNorm->scheduleTransitionTo( Ogre::GpuResidency::Resident );
            blackImage.uploadTo( textureNorm, 0, textureNorm->getNumMipmaps() - 1u );

            decal->setNormalTexture( textureNorm );
        }

        {
            Ogre::Item *item = sceneManager->createItem(
                                   "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                   Ogre::SCENE_DYNAMIC );
            item->setDatablock( "Marble" );
            Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                                                    createChildSceneNode( Ogre::SCENE_DYNAMIC );
            sceneNode->setPosition( 0, -1.5, 0 );
            sceneNode->attachObject( item );

            //Change the addressing mode of the roughness map to wrap via code.
            //Detail maps default to wrap, but the rest to clamp.
            assert( dynamic_cast<Ogre::HlmsPbsDatablock*>( item->getSubItem(0)->getDatablock() ) );
            Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                                                            item->getSubItem(0)->getDatablock() );
            //Make a hard copy of the sampler block
            Ogre::HlmsSamplerblock samplerblock( *datablock->getSamplerblock( Ogre::PBSM_ROUGHNESS ) );
            samplerblock.mU = Ogre::TAM_WRAP;
            samplerblock.mV = Ogre::TAM_WRAP;
            samplerblock.mW = Ogre::TAM_WRAP;
            //Set the new samplerblock. The Hlms system will
            //automatically create the API block if necessary
            datablock->setSamplerblock( Ogre::PBSM_ROUGHNESS, samplerblock );
        }

        for( int i=0; i<4; ++i )
        {
            for( int j=0; j<4; ++j )
            {
                Ogre::String meshName;

                if( i == j )
                    meshName = "Sphere1000.mesh";
                else
                    meshName = "Cube_d.mesh";

                Ogre::Item *item = sceneManager->createItem( meshName,
                                                             Ogre::ResourceGroupManager::
                                                             AUTODETECT_RESOURCE_GROUP_NAME,
                                                             Ogre::SCENE_DYNAMIC );
                if( i % 2 == 0 )
                    item->setDatablock( "Rocks" );
                else
                    item->setDatablock( "Marble" );

                item->setVisibilityFlags( 0x000000001 );

                size_t idx = i * 4 + j;

                Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                                             createChildSceneNode( Ogre::SCENE_DYNAMIC );

                sceneNode->setPosition( (i - 1.5f) * armsLength,
                                        2.0f,
                                        (j - 1.5f) * armsLength );
                sceneNode->setScale( 0.65f, 0.65f, 0.65f );

                sceneNode->roll( Ogre::Radian( (Ogre::Real)idx ) );

                sceneNode->attachObject( item );
            }
        }

        {
            Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();

            assert( dynamic_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );

            Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms(Ogre::HLMS_PBS) );

            const int numX = 8;
            const int numZ = 8;

            const float armsLength = 1.0f;
            const float startX = (numX-1) / 2.0f;
            const float startZ = (numZ-1) / 2.0f;

            Ogre::Root *root = mGraphicsSystem->getRoot();
            Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();

            size_t numSpheres = 0;

            for( int x=0; x<numX; ++x )
            {
                for( int z=0; z<numZ; ++z )
                {
                    Ogre::String datablockName = "Test" + Ogre::StringConverter::toString( numSpheres++ );
                    Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                                hlmsPbs->getDatablock( datablockName ) );

                    if( !datablock )
                    {
                        datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                                        hlmsPbs->createDatablock( datablockName,
                                                                  datablockName,
                                                                  Ogre::HlmsMacroblock(),
                                                                  Ogre::HlmsBlendblock(),
                                                                  Ogre::HlmsParamVec() ) );
                    }

                    Ogre::TextureGpu *texture = textureMgr->createOrRetrieveTexture(
                                                    "SaintPetersBasilica.dds",
                                                    Ogre::GpuPageOutStrategy::Discard,
                                                    Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB,
                                                    Ogre::TextureTypes::TypeCube,
                                                    Ogre::ResourceGroupManager::
                                                    AUTODETECT_RESOURCE_GROUP_NAME,
                                                    Ogre::TextureFilter::TypeGenerateDefaultMipmaps );

                    datablock->setTexture( Ogre::PBSM_REFLECTION, texture );
                    datablock->setDiffuse( Ogre::Vector3( 0.0f, 1.0f, 0.0f ) );

                    datablock->setRoughness( std::max( 0.02f, x / std::max( 1.0f, (float)(numX-1) ) ) );
                    datablock->setFresnel( Ogre::Vector3( z / std::max( 1.0f, (float)(numZ-1) ) ), false );

                    Ogre::Item *item = sceneManager->createItem( "Sphere1000.mesh",
                                                                 Ogre::ResourceGroupManager::
                                                                 AUTODETECT_RESOURCE_GROUP_NAME,
                                                                 Ogre::SCENE_DYNAMIC );
                    item->setDatablock( datablock );
                    item->setVisibilityFlags( 0x000000002 );

                    Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                            createChildSceneNode( Ogre::SCENE_DYNAMIC );
                    sceneNode->setPosition( Ogre::Vector3( armsLength * x - startX,
                                                           1.0f,
                                                           armsLength * z - startZ ) );
                    sceneNode->attachObject( item );
                }
            }
        }

        Ogre::SceneNode *rootNode = sceneManager->getRootSceneNode();

        Ogre::Light *light = sceneManager->createLight();
        Ogre::SceneNode *lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( light );
        light->setPowerScale( 1.0f );
        light->setType( Ogre::Light::LT_DIRECTIONAL );
        light->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );

        sceneManager->setAmbientLight( Ogre::ColourValue( 0.3f, 0.5f, 0.7f ) * 0.1f * 0.75f,
                                       Ogre::ColourValue( 0.6f, 0.45f, 0.3f ) * 0.065f * 0.75f,
                                       -light->getDirection() + Ogre::Vector3::UNIT_Y * 0.2f );

        light = sceneManager->createLight();
        lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( light );
        light->setDiffuseColour( 0.8f, 0.4f, 0.2f ); //Warm
        light->setSpecularColour( 0.8f, 0.4f, 0.2f );
        light->setPowerScale( Ogre::Math::PI );
        light->setType( Ogre::Light::LT_SPOTLIGHT );
        lightNode->setPosition( -10.0f, 10.0f, 10.0f );
        light->setDirection( Ogre::Vector3( 1, -1, -1 ).normalisedCopy() );
        light->setAttenuationBasedOnRadius( 10.0f, 0.01f );

        light = sceneManager->createLight();
        lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( light );
        light->setDiffuseColour( 0.2f, 0.4f, 0.8f ); //Cold
        light->setSpecularColour( 0.2f, 0.4f, 0.8f );
        light->setPowerScale( Ogre::Math::PI );
        light->setType( Ogre::Light::LT_SPOTLIGHT );
        lightNode->setPosition( 10.0f, 10.0f, -10.0f );
        light->setDirection( Ogre::Vector3( -1, -1, 1 ).normalisedCopy() );
        light->setAttenuationBasedOnRadius( 10.0f, 0.01f );

        light = sceneManager->createLight();
        lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( light );
        light->setDiffuseColour( 0.2f, 0.2f, 0.2f );
        light->setSpecularColour( 0.2f, 0.2f, 0.2f );
        light->setPowerScale( Ogre::Math::PI );
        light->setType( Ogre::Light::LT_AREA_APPROX );
        lightNode->setPosition( 10.0f, 10.0f, 10.0f );
        light->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );
        light->setAttenuationBasedOnRadius( 10.0f, 0.01f );
        light->setRectSize( Ogre::Vector2( 5.0f, 5.0f ) );

        {
            Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
            assert( dynamic_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
            Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms(Ogre::HLMS_PBS) );

            Ogre::HlmsBlendblock blendblock;
            Ogre::HlmsMacroblock macroblock;

            struct DemoMaterials
            {
                Ogre::String matName;
                Ogre::ColourValue colour;
            };

            DemoMaterials materials[4] =
            {
                { "Red", Ogre::ColourValue::Red },
                { "Green", Ogre::ColourValue::Green },
                { "Blue", Ogre::ColourValue::Blue },
                { "Cream", Ogre::ColourValue::White },
            };

            for( int i=0; i<4; ++i )
            {
                Ogre::String finalName = materials[i].matName;

                Ogre::HlmsPbsDatablock *datablock;
                datablock = static_cast<Ogre::HlmsPbsDatablock*>( hlmsPbs->getDatablock( finalName ) );

                if( !datablock )
                {
                    datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                                    hlmsPbs->createDatablock( finalName, finalName,
                                                              macroblock, blendblock,
                                                              Ogre::HlmsParamVec() ) );
                }
                datablock->setBackgroundDiffuse( materials[i].colour );
                datablock->setFresnel( Ogre::Vector3( 0.1f ), false );
                datablock->setRoughness( 0.02 );
            }

            ::generateScene( sceneManager );

            mInstantRadiosity = new Ogre::InstantRadiosity( sceneManager, hlmsManager );
            mInstantRadiosity->mVplThreshold = 0.0005f;

            //Guide where to shoot the rays for directional lights the 3 windows + the
            //hole in the ceiling). We use a sphere radius of 30 to ensure when the directional
            //light's dir is towards -X, we actually hit walls (instead of going through these
            //walls and generating incorrect results).
            mInstantRadiosity->mAoI.push_back( Ogre::InstantRadiosity::AreaOfInterest(
                        Ogre::Aabb( Ogre::Vector3( -0.746887f, 7.543859f, 5.499001f ),
                                    Ogre::Vector3( 2.876101f, 2.716137f, 6.059607f ) * 0.5f ), 30.0f ) );
            mInstantRadiosity->mAoI.push_back( Ogre::InstantRadiosity::AreaOfInterest(
                        Ogre::Aabb( Ogre::Vector3( -6.26f, 3.969576f, 6.628003f ),
                                    Ogre::Vector3( 1.673888f, 6.04f, 1.3284f ) * 0.5f ), 30.0f ) );
            mInstantRadiosity->mAoI.push_back( Ogre::InstantRadiosity::AreaOfInterest(
                        Ogre::Aabb( Ogre::Vector3( -6.26f, 3.969576f, 3.083399f ),
                                    Ogre::Vector3( 1.673888f, 6.04f, 1.3284f ) * 0.5f ), 30.0f ) );
            mInstantRadiosity->mAoI.push_back( Ogre::InstantRadiosity::AreaOfInterest(
                        Ogre::Aabb( Ogre::Vector3( -6.26f, 3.969576f, -0.415852f ),
                                    Ogre::Vector3( 1.673888f, 6.04f, 1.3284f ) * 0.5f ), 30.0f ) );

            mInstantRadiosity->mNumRays = 128u;
            mInstantRadiosity->mNumRayBounces = 1u;
            mInstantRadiosity->mCellSize = 5.0f;
            mInstantRadiosity->build();

            sceneManager->setForwardClustered( true, 16, 8, 24, 96, 8, 0, 2, 50 );
            //Required by InstantRadiosity
            sceneManager->getForwardPlus()->setEnableVpls( true );
        }

        setupParallaxCorrectCubemaps();
    }
    //-----------------------------------------------------------------------------------
    void SceneFormatGameState::exportScene(void)
    {
        Ogre::SceneFormatExporter exporter( mGraphicsSystem->getRoot(),
                                            mGraphicsSystem->getSceneManager(),
                                            mInstantRadiosity );
        exporter.setUseBinaryFloatingPoint( true );
        exporter.exportSceneToFile( mFullpathToFile );
    }
    //-----------------------------------------------------------------------------------
    void SceneFormatGameState::importScene(void)
    {
        destroyInstantRadiosity();
        destroyParallaxCorrectCubemaps();

        Ogre::SceneFormatImporter importer( mGraphicsSystem->getRoot(),
                                            mGraphicsSystem->getSceneManager(),
                                            Ogre::BLANKSTRING );
        importer.importSceneFromFile( mFullpathToFile );
        importer.getInstantRadiosity( true, &mInstantRadiosity, &mIrradianceVolume );
        mParallaxCorrectedCubemap = importer.getParallaxCorrectedCubemap( true );

//        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
//        sceneManager->setForwardClustered( true, 16, 8, 24, 96, 2, 50 );
//        sceneManager->getForwardPlus()->setEnableVpls( true );
    }
    //-----------------------------------------------------------------------------------
    void SceneFormatGameState::createScene01(void)
    {
        Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane( "Plane v1",
                                            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                            Ogre::Plane( Ogre::Vector3::UNIT_Y, 1.0f ), 50.0f, 50.0f,
                                            1, 1, true, 1, 4.0f, 4.0f, Ogre::Vector3::UNIT_Z,
                                            Ogre::v1::HardwareBuffer::HBU_STATIC,
                                            Ogre::v1::HardwareBuffer::HBU_STATIC );

        Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createByImportingV1(
                    "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    planeMeshV1.get(), true, true, true );

        generateScene();
        mCameraController = new CameraController( mGraphicsSystem, false );
        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
    void SceneFormatGameState::destroyScene()
    {
        destroyInstantRadiosity();
        destroyParallaxCorrectCubemaps();
    }
    //-----------------------------------------------------------------------------------
    void SceneFormatGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
    {
        TutorialGameState::generateDebugText( timeSinceLast, outText );
        outText += "\nPress F2 to generate the scene from code.";
        outText += "\nPress F3 to export the current scene.";
        outText += "\nPress F4 to generate the scene from an imported file.";
        outText += "\nFile location: " + mFullpathToFile;
    }
    //-----------------------------------------------------------------------------------
    void SceneFormatGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( (arg.keysym.mod & ~(KMOD_NUM|KMOD_CAPS)) != 0 )
        {
            TutorialGameState::keyReleased( arg );
            return;
        }

        if( arg.keysym.sym == SDLK_F2 )
        {
            resetScene();
            generateScene();
        }
        else if( arg.keysym.sym == SDLK_F3 )
        {
            exportScene();
        }
        else if( arg.keysym.sym == SDLK_F4 )
        {
            resetScene();
            importScene();
        }
        else
        {
            TutorialGameState::keyReleased( arg );
        }
    }
}
