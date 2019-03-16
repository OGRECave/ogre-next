
#include "UpdatingDecalsAndAreaLightTexGameState.h"
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

    static const char c_lightRadiusKeys[4]  = { 'Y', 'U', 'I', 'O' };
    static const char c_lightFileKeys[4]    = { 'H', 'J', 'K', 'L' };

    inline float sdBox( const Ogre::Vector2 &point, const Ogre::Vector2 &center,
                        const Ogre::Vector2 &halfSize )
    {
        Ogre::Vector2 p = point - center;
        Ogre::Vector2 d = Ogre::Vector2( abs(p.x), abs(p.y) ) - halfSize;
        Ogre::Vector2 dCeil( d );
        dCeil.makeCeil( Ogre::Vector2::ZERO );
        return dCeil.length() + Ogre::min( Ogre::max( d.x, d.y ), 0.0f );
    }
    inline float sdAnnularBox( const Ogre::Vector2 &point, const Ogre::Vector2 &center,
                               const Ogre::Vector2 &halfSize, float r )
    {
        return fabsf( sdBox( point, center, halfSize ) ) - r;
    }

    UpdatingDecalsAndAreaLightTexGameState::UpdatingDecalsAndAreaLightTexGameState(
            const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mAreaMaskTex( 0 ),
        mUseSynchronousMethod( false )
    {
        OGRE_STATIC_ASSERT( sizeof(c_lightRadiusKeys) /
                            sizeof(c_lightRadiusKeys[0]) >= c_numAreaLights );
        OGRE_STATIC_ASSERT( sizeof(c_lightFileKeys) /
                            sizeof(c_lightFileKeys[0]) >= c_numAreaLights );

        memset( mAreaLights, 0, sizeof( mLightNodes ) );
        memset( mAreaLights, 0, sizeof( mAreaLights ) );
        memset( mUseTextureFromFile, 0, sizeof( mUseTextureFromFile ) );

        for( size_t i=0; i<c_numAreaLights; ++i )
        {
            mLightTexRadius[i] = 0.05f;
        }
    }
    //-----------------------------------------------------------------------------------
    void UpdatingDecalsAndAreaLightTexGameState::createAreaMask( float radius, Ogre::Image2 &outImage )
    {
        //Please note the texture CAN be coloured. The sample uses a monochrome texture,
        //but you coloured textures are supported too. However they will burn a little
        //more GPU performance.
        const Ogre::uint32 texWidth = c_defaultWidth;
        const Ogre::uint32 texHeight = c_defaultHeight;
        const Ogre::PixelFormatGpu texFormat = c_defaultFormat;

        //Fill the texture with a hollow rectangle, 10-pixel thick.
        size_t sizeBytes = Ogre::PixelFormatGpuUtils::calculateSizeBytes(
                               texWidth, texHeight, 1u, 1u, texFormat, 1u, 4u );
        Ogre::uint8 *data = reinterpret_cast<Ogre::uint8*>(
                                OGRE_MALLOC_SIMD( sizeBytes, Ogre::MEMCATEGORY_GENERAL ) );
        outImage.loadDynamicImage( data, texWidth, texHeight, 1u,
                                   Ogre::TextureTypes::Type2D, texFormat,
                                   true, 1u );

        const float invTexWidth = 1.0f / texWidth;
        const float invTexHeight = 1.0f / texHeight;
        for( size_t y=0; y<texHeight; ++y )
        {
            for( size_t x=0; x<texWidth; ++x )
            {
                const Ogre::Vector2 uv( x * invTexWidth, y * invTexHeight );

                const float d = sdAnnularBox( uv,
                                              Ogre::Vector2( 0.5f ),
                                              Ogre::Vector2( 0.3f ),
                                              radius );
                if( d <= 0 )
                {
                    *data++ = 255;
                    *data++ = 255;
                    *data++ = 255;
                    *data++ = 255;
                }
                else
                {
                    *data++ = 0;
                    *data++ = 0;
                    *data++ = 0;
                    *data++ = 0;
                }
            }
        }

        //Generate the mipmaps so roughness works
        outImage.generateMipmaps( true, Ogre::Image2::FILTER_GAUSSIAN_HIGH );

        {
            //Ensure the lower mips have black borders. This is done to prevent certain artifacts,
            //Ensure the higher mips have grey borders. This is done to prevent certain artifacts.
            for( size_t i=0u; i<outImage.getNumMipmaps(); ++i )
            {
                Ogre::TextureBox dataBox = outImage.getData( static_cast<Ogre::uint8>( i ) );

                const Ogre::uint8 borderColour = i >= 5u ? 127u : 0u;
                const Ogre::uint32 currWidth = dataBox.width;
                const Ogre::uint32 currHeight = dataBox.height;

                const size_t bytesPerRow = dataBox.bytesPerRow;

                memset( dataBox.at( 0, 0, 0 ), borderColour, bytesPerRow );
                memset( dataBox.at( 0, (currHeight - 1u), 0 ), borderColour, bytesPerRow );

                for( size_t y=1; y<currWidth - 1u; ++y )
                {
                    Ogre::uint8 *left = reinterpret_cast<Ogre::uint8*>( dataBox.at( 0, y, 0 ) );
                    left[0] = borderColour;
                    left[1] = borderColour;
                    left[2] = borderColour;
                    left[3] = borderColour;
                    Ogre::uint8 *right =
                            reinterpret_cast<Ogre::uint8*>( dataBox.at( currWidth - 1u, y, 0 ) );
                    right[0] = borderColour;
                    right[1] = borderColour;
                    right[2] = borderColour;
                    right[3] = borderColour;
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void UpdatingDecalsAndAreaLightTexGameState::createAreaPlaneMesh(void)
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
    Ogre::HlmsDatablock* UpdatingDecalsAndAreaLightTexGameState::setupDatablockTextureForLight(
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

        if( light->mTextureLightMaskIdx != std::numeric_limits<Ogre::uint16>::max() )
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
    void UpdatingDecalsAndAreaLightTexGameState::createPlaneForAreaLight( Ogre::Light *light,
                                                                          size_t idx )
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
        planeNode->attachObject( item );

        //Math the plane size to that of the area light
        const Ogre::Vector2 rectSize = light->getRectSize();
        planeNode->setScale( rectSize.x, rectSize.y, 1.0f );

        /* For debugging ranges & AABBs
        Ogre::WireAabb *wireAabb = sceneManager->createWireAabb();
        wireAabb->track( light );*/
    }
    //-----------------------------------------------------------------------------------
    void UpdatingDecalsAndAreaLightTexGameState::createLight( const Ogre::Vector3 &position, size_t idx )
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
        light->setType( Ogre::Light::LT_AREA_APPROX );
        light->setRectSize( Ogre::Vector2( 5.0f, 5.0f ) );
        lightNode->setPosition( position );
        light->setDirection( Ogre::Vector3( 0, 0, 1 ).normalisedCopy() );
        light->setAttenuationBasedOnRadius( 10.0f, 0.01f );

//        //Control the diffuse mip (this is the default value)
//        light->mTexLightMaskDiffuseMipStart = (Ogre::uint16)(0.95f * 65535);

        createPlaneForAreaLight( light, idx );

        mAreaLights[idx] = light;
        mLightNodes[idx] = lightNode;
    }
    //-----------------------------------------------------------------------------------
    void UpdatingDecalsAndAreaLightTexGameState::setupLightTexture( size_t idx )
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();

        if( !mUseTextureFromFile[idx] )
        {
            Ogre::TextureGpu *areaTex =
                    textureMgr->createOrRetrieveTexture(
                        "AreaLightMask" + Ogre::StringConverter::toString( idx ),
                        Ogre::GpuPageOutStrategy::AlwaysKeepSystemRamCopy,
                        Ogre::TextureFlags::AutomaticBatching,
                        Ogre::TextureTypes::Type2D, Ogre::BLANKSTRING, 0u, c_areaLightsPoolId );

            Ogre::Image2 image;
            createAreaMask( mLightTexRadius[idx], image );

            bool canUseSynchronousUpload = areaTex->getNextResidencyStatus() ==
                                           Ogre::GpuResidency::Resident &&
                                           areaTex->isDataReady();
            if( mUseSynchronousMethod && canUseSynchronousUpload )
            {
                //If canUseSynchronousUpload is false, you can use areaTex->waitForData()
                //to still use sync method (assuming the texture is resident)
                image.uploadTo( areaTex, 0, areaTex->getNumMipmaps() - 1u );
            }
            else
            {
                //Asynchronous is preferred due to being done in the background. But the switch
                //Resident -> OnStorage -> Resident may cause undesired effects, so we
                //show how to do it synchronously

                //Tweak via _setAutoDelete so the internal data is copied as a pointer
                //instead of performing a deep copy of the data; while leaving the responsability
                //of freeing memory to imagePtr instead.
                image._setAutoDelete( false );
                Ogre::Image2 *imagePtr = new Ogre::Image2( image );
                imagePtr->_setAutoDelete( true );

                if( areaTex->getNextResidencyStatus() == Ogre::GpuResidency::Resident )
                    areaTex->scheduleTransitionTo( Ogre::GpuResidency::OnStorage );
                //Ogre will call "delete imagePtr" when done, because we're passing
                //true to autoDeleteImage argument in scheduleTransitionTo
                areaTex->scheduleTransitionTo( Ogre::GpuResidency::Resident, imagePtr, true );

                mAreaLights[idx]->setTexture( areaTex );
            }
        }
        else
        {
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
        }

        setupDatablockTextureForLight( mAreaLights[idx], idx );

        if( !mUseSynchronousMethod )
        {
            //If we don't wait, textures will flicker during async upload.
            //If you don't care about the glitch, avoid this call
            textureMgr->waitForStreamingCompletion();
        }
    }
    //-----------------------------------------------------------------------------------
    void UpdatingDecalsAndAreaLightTexGameState::createScene01(void)
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
        pbs->setAreaLightForwardSettings( c_numAreaLights, 0u );

        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        //Setup the floor
        Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane( "Plane v1",
                                            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                            Ogre::Plane( Ogre::Vector3::UNIT_Y, 1.0f ), 50.0f, 50.0f,
                                            1, 1, true, 1, 4.0f, 4.0f, Ogre::Vector3::UNIT_Z,
                                            Ogre::v1::HardwareBuffer::HBU_STATIC,
                                            Ogre::v1::HardwareBuffer::HBU_STATIC );

        Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createManual(
                    "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

        planeMesh->importV1( planeMeshV1.get(), true, true, true );

        {
            Ogre::Item *item = sceneManager->createItem( planeMesh, Ogre::SCENE_DYNAMIC );
            item->setDatablock( "MirrorLike" );
            Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                                                    createChildSceneNode( Ogre::SCENE_DYNAMIC );
            sceneNode->setPosition( 0, -1, 0 );
            sceneNode->attachObject( item );
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

        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
    void UpdatingDecalsAndAreaLightTexGameState::destroyScene(void)
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
    void UpdatingDecalsAndAreaLightTexGameState::update( float timeSinceLast )
    {
        TutorialGameState::update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    void UpdatingDecalsAndAreaLightTexGameState::generateDebugText( float timeSinceLast,
                                                                    Ogre::String &outText )
    {
        TutorialGameState::generateDebugText( timeSinceLast, outText );
        outText += "\nPress F2 to test sync/async methods ";
        outText += mUseSynchronousMethod ? "[Sync]" : "[Async]";
        outText += "\nHold [Shift] to change value in opposite direction";
        for( size_t i=0; i<c_numAreaLights; ++i )
        {
            outText += "\nRadius " + Ogre::StringConverter::toString( i ) + " [" +
                       c_lightRadiusKeys[i] + "]: ";
            outText += Ogre::StringConverter::toString( mLightTexRadius[i] );
        }
        outText += "\nPress J/K/L/O to switch to a texture from file";
    }
    //-----------------------------------------------------------------------------------
    void UpdatingDecalsAndAreaLightTexGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( (arg.keysym.mod & ~(KMOD_NUM|KMOD_CAPS|KMOD_LSHIFT|KMOD_RSHIFT)) != 0 )
        {
            TutorialGameState::keyReleased( arg );
            return;
        }

        if( arg.keysym.sym == SDLK_F2 )
        {
            mUseSynchronousMethod = !mUseSynchronousMethod;
        }

        bool keyHit = false;
        for( size_t i=0; i<c_numAreaLights && !keyHit; ++i )
        {
            SDL_Keycode keyCode = SDLK_a + c_lightRadiusKeys[i] - 'A';
            if( arg.keysym.sym == keyCode )
            {
                const bool reverse = (arg.keysym.mod & (KMOD_LSHIFT|KMOD_RSHIFT)) != 0;
                if( reverse )
                    mLightTexRadius[i] -= 0.01f;
                else
                    mLightTexRadius[i] += 0.01f;
                mLightTexRadius[i] = Ogre::Math::Clamp( mLightTexRadius[i], 0.0f, 1.0f );
                setupLightTexture( i );
                keyHit = true;
            }

            keyCode = SDLK_a + c_lightFileKeys[i] - 'A';
            if( arg.keysym.sym == keyCode )
            {
                mUseTextureFromFile[i] = !mUseTextureFromFile[i];
                setupLightTexture( i );
                keyHit = true;
            }
        }

        if( !keyHit )
        {
            TutorialGameState::keyReleased( arg );
        }
    }
}
