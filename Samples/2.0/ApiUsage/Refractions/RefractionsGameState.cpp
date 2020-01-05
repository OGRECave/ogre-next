
#include "RefractionsGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreItem.h"
#include "OgreSceneManager.h"

#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"

#include "OgreCamera.h"
#include "OgreWindow.h"

#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsSamplerblock.h"

#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreRoot.h"
#include "OgreTextureFilters.h"
#include "OgreTextureGpuManager.h"

using namespace Demo;

namespace Demo
{
    RefractionsGameState::RefractionsGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mAnimateObjects( true ),
        mNumSpheres( 0 ),
        mTransparencyValue( 0.15f ),
        mDebugVisibility( NormalSpheres )
    {
        memset( mSceneNode, 0, sizeof( mSceneNode ) );
    }
    //-----------------------------------------------------------------------------------
    /** @brief RefractionsGameState::createRefractivePlaceholder

        THIS IS OPTIONAL, BUT GREATLY FIXES SOME RENDERING ARTIFACTS

        Multiple refractions are not supported. But to make it worse, refractive objects
        rendered on top of other refractive objects often result in rendering artifacts.

        It is similar to Order Dependent Transparency issues (i.e. regular alpha blending) but
        with even more artifacts.

        Therefore there is a very simple yet effective workaround: Create a duplicate of the
        object rendered with regular alpha blending, and that actually outputs depth to the
        depth buffer.

        This greatly fixes some depth-related bugs and rendering order issues; while it also
        creates a 'fallback' because now refractive objects behind other refractive objects will
        appear (however they won't cause multiple refractions, you will be seeing the alpha blended
        version)

        The fallback might cause some glitches of its own, but this a much better alternative
    @param item
        Item to duplicate as fallback
    @param sceneNode
        Scene node containing the Item
    @param datablock
        Datablock to duplicate as fallback
    */
    void RefractionsGameState::createRefractivePlaceholder( Ogre::Item *item, Ogre::SceneNode *sceneNode,
                                                            Ogre::HlmsPbsDatablock *datablock )
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        item = sceneManager->createItem( item->getMesh()->getName(),
                                         Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                         item->isStatic() ? Ogre::SCENE_STATIC : Ogre::SCENE_DYNAMIC );
        item->setVisibilityFlags( 0x000000004u );
        item->setCastShadows( false );

        datablock = reinterpret_cast<Ogre::HlmsPbsDatablock *>(
            datablock->clone( *datablock->getNameStr() + "_placeHolder" ) );
        datablock->setTransparency( datablock->getTransparency() * datablock->getTransparency(),
                                    Ogre::HlmsPbsDatablock::Transparent );
        Ogre::HlmsMacroblock macroblock;
        datablock->setMacroblock( macroblock );
        item->setDatablock( datablock );
        sceneNode->attachObject( item );
    }
    //-----------------------------------------------------------------------------------
    void RefractionsGameState::createRefractiveWall( void )
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );

        // Create a cube and make it very thin
        Ogre::Item *item = sceneManager->createItem(
            "Cube_d.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
            Ogre::SCENE_DYNAMIC );
        item->setVisibilityFlags( 0x000000002u );

        Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                                         ->createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->setPosition( 0, 2.0f, 0 );
        sceneNode->setScale( 0.05f, 2.0f, 3.75f );
        sceneNode->attachObject( item );

        // Create the refractive material for this wall
        Ogre::String datablockName = "RefractiveWall";
        Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock *>(
            hlmsPbs->createDatablock( datablockName, datablockName, Ogre::HlmsMacroblock(),
                                      Ogre::HlmsBlendblock(), Ogre::HlmsParamVec() ) );
        // Assign a normal map so the refractions are much more visually pleasing (and obvious)
        datablock->setTexture( Ogre::PBSM_NORMAL, "floor_bump.PNG" );

        // Do not cast shadows!
        item->setCastShadows( false );

        // These settings affect refractions directly

        // Set the material to refractive, 15%
        datablock->setTransparency( 0.15f, Ogre::HlmsPbsDatablock::Refractive );
        datablock->setFresnel( Ogre::Vector3( 0.5f ), false );
        datablock->setRefractionStrength( 0.2f );

        // This call is very important. Refractive materials must be rendered during the
        // refractive pass (see Samples/Media/2.0/scripts/Compositors/Refractions.compositor)
        // We set it to 200 because we want this big wall to be rendered *before* the spheres
        // to avoid some artifacts
        item->setRenderQueueGroup( 200u );

        item->setDatablock( datablock );

        createRefractivePlaceholder( item, sceneNode, datablock );
    }
    //-----------------------------------------------------------------------------------
    void RefractionsGameState::createRefractiveSphere( const int x, const int z, const int numX,
                                                       const int numZ, const float armsLength,
                                                       const float startX, const float startZ )
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );

        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();

        // Create a sphere
        Ogre::Item *item = sceneManager->createItem(
            "Sphere1000.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
            Ogre::SCENE_DYNAMIC );
        item->setVisibilityFlags( 0x000000002u );

        Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                                         ->createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->setPosition(
            Ogre::Vector3( armsLength * x - startX, 1.0f, armsLength * z - startZ ) );
        sceneNode->attachObject( item );

        // Create the refractive material for this sphere
        Ogre::String datablockName = "Test" + Ogre::StringConverter::toString( mNumSpheres );
        Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock *>(
            hlmsPbs->createDatablock( datablockName, datablockName, Ogre::HlmsMacroblock(),
                                      Ogre::HlmsBlendblock(), Ogre::HlmsParamVec() ) );

        Ogre::TextureGpu *texture = textureMgr->createOrRetrieveTexture(
            "SaintPetersBasilica.dds", Ogre::GpuPageOutStrategy::Discard,
            Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB, Ogre::TextureTypes::TypeCube,
            Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
            Ogre::TextureFilter::TypeGenerateDefaultMipmaps );

        datablock->setTexture( Ogre::PBSM_REFLECTION, texture );
        datablock->setDiffuse( Ogre::Vector3( 0.0f, 1.0f, 0.0f ) );

        datablock->setRoughness( std::max( 0.02f, x / Ogre::max( 1, (float)( numX - 1 ) ) ) );
        datablock->setFresnel( Ogre::Vector3( z / Ogre::max( 1, (float)( numZ - 1 ) ) ), false );

        // Do not cast shadows!
        item->setCastShadows( false );

        // Set the material to refractive
        datablock->setTransparency( mTransparencyValue, Ogre::HlmsPbsDatablock::Refractive );

        // This call is very important. Refractive materials must be rendered during the
        // refractive pass (see Samples/Media/2.0/scripts/Compositors/Refractions.compositor)
        // We set it to 201 because we want these smaller spheres to be rendered *after* the big
        // wall to avoid some artifacts
        item->setRenderQueueGroup( 200u );

        item->setDatablock( datablock );

        createRefractivePlaceholder( item, sceneNode, datablock );

        ++mNumSpheres;
    }
    //-----------------------------------------------------------------------------------
    void RefractionsGameState::createScene01( void )
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        const float armsLength = 2.5f;

        Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane(
            "Plane v1", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::Plane( Ogre::Vector3::UNIT_Y, 1.0f ), 50.0f, 50.0f, 1, 1, true, 1, 4.0f, 4.0f,
            Ogre::Vector3::UNIT_Z, Ogre::v1::HardwareBuffer::HBU_STATIC,
            Ogre::v1::HardwareBuffer::HBU_STATIC );

        Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createManual(
            "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

        planeMesh->importV1( planeMeshV1.get(), true, true, true );

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

        createRefractiveWall();

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

                size_t idx = i * 4 + j;

                mSceneNode[idx] = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                                      ->createChildSceneNode( Ogre::SCENE_DYNAMIC );

                mSceneNode[idx]->setPosition( ( i - 1.5f ) * armsLength, 2.0f,
                                              ( j - 1.5f ) * armsLength );
                mSceneNode[idx]->setScale( 0.65f, 0.65f, 0.65f );

                mSceneNode[idx]->roll( Ogre::Radian( (Ogre::Real)idx ) );

                mSceneNode[idx]->attachObject( item );
            }
        }

        {
            mNumSpheres = 0;

            const int numX = 8;
            const int numZ = 8;

            const float armsLength = 1.0f;
            const float startX = ( numX - 1 ) / 2.0f;
            const float startZ = ( numZ - 1 ) / 2.0f;

            for( int x = 0; x < numX; ++x )
            {
                for( int z = 0; z < numZ; ++z )
                    createRefractiveSphere( x, z, numX, numZ, armsLength, startX, startZ );
            }
        }

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
        lightNode->setPosition( -10.0f, 10.0f, 10.0f );
        light->setDirection( Ogre::Vector3( 1, -1, -1 ).normalisedCopy() );
        light->setAttenuationBasedOnRadius( 10.0f, 0.01f );

        mLightNodes[1] = lightNode;

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

        mLightNodes[2] = lightNode;

        mCameraController = new CameraController( mGraphicsSystem, false );

        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
    void RefractionsGameState::update( float timeSinceLast )
    {
        if( mAnimateObjects )
        {
            for( int i = 0; i < 16; ++i )
                mSceneNode[i]->yaw( Ogre::Radian( timeSinceLast * i * 0.125f ) );
        }

        TutorialGameState::update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    void RefractionsGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
    {
        Ogre::uint32 visibilityMask = mGraphicsSystem->getSceneManager()->getVisibilityMask();

        const char *c_debugVisibility[] = { "[NoSpheres]",                        //
                                            "[Normal Spheres]",                   //
                                            "[Only Refractions No placeholder]",  //
                                            "[Only Placeholder No refractions]" };

        TutorialGameState::generateDebugText( timeSinceLast, outText );
        outText += "\nPress F2 to toggle animation. ";
        outText += mAnimateObjects ? "[On]" : "[Off]";
        outText += "\nPress F3 to show/hide animated objects. ";
        outText += ( visibilityMask & 0x000000001 ) ? "[On]" : "[Off]";
        outText += "\nPress F4 to show/hide palette of spheres. ";
        outText += c_debugVisibility[mDebugVisibility];
        outText += "\n+/- to change transparency. [";
        outText += Ogre::StringConverter::toString( mTransparencyValue ) + "]";
    }
    //-----------------------------------------------------------------------------------
    void RefractionsGameState::setTransparencyToMaterials( void )
    {
        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );

        for( size_t i = 0; i < mNumSpheres; ++i )
        {
            Ogre::String datablockName = "Test" + Ogre::StringConverter::toString( i );
            Ogre::HlmsPbsDatablock *datablock =
                static_cast<Ogre::HlmsPbsDatablock *>( hlmsPbs->getDatablock( datablockName ) );

            datablock->setTransparency( mTransparencyValue, Ogre::HlmsPbsDatablock::Refractive );

            datablockName += "_placeHolder";
            datablock = static_cast<Ogre::HlmsPbsDatablock *>( hlmsPbs->getDatablock( datablockName ) );

            datablock->setTransparency( mTransparencyValue * mTransparencyValue,
                                        Ogre::HlmsPbsDatablock::Transparent );
        }
    }
    //-----------------------------------------------------------------------------------
    void RefractionsGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( ( arg.keysym.mod & ~( KMOD_NUM | KMOD_CAPS | KMOD_LSHIFT | KMOD_RSHIFT ) ) != 0 )
        {
            TutorialGameState::keyReleased( arg );
            return;
        }

        if( arg.keysym.sym == SDLK_F2 )
        {
            mAnimateObjects = !mAnimateObjects;
        }
        else if( arg.keysym.sym == SDLK_F3 )
        {
            Ogre::uint32 visibilityMask = mGraphicsSystem->getSceneManager()->getVisibilityMask();
            bool showMovingObjects = ( visibilityMask & 0x00000001 );
            showMovingObjects = !showMovingObjects;
            visibilityMask &= ~0x00000001u;
            visibilityMask |= (Ogre::uint32)showMovingObjects;
            mGraphicsSystem->getSceneManager()->setVisibilityMask( visibilityMask );
        }
        else if( arg.keysym.sym == SDLK_F4 )
        {
            const bool reverse = ( arg.keysym.mod & ( KMOD_LSHIFT | KMOD_RSHIFT ) ) != 0;
            if( !reverse )
            {
                mDebugVisibility = static_cast<DebugVisibility>( ( mDebugVisibility + 1u ) %
                                                                 ( ShowOnlyPlaceholder + 1u ) );
            }
            else
            {
                mDebugVisibility =
                    static_cast<DebugVisibility>( ( mDebugVisibility + ShowOnlyPlaceholder + 1u - 1u ) %
                                                  ( ShowOnlyPlaceholder + 1u ) );
            }

            Ogre::uint32 visibilityMask = mGraphicsSystem->getSceneManager()->getVisibilityMask();
            switch( mDebugVisibility )
            {
            case NoSpheres:
                visibilityMask &= ~( 0x00000002u | 0x00000004u );
                break;
            case NormalSpheres:
                visibilityMask |= 0x00000002u | 0x00000004u;
                break;
            case ShowOnlyRefractions:
                visibilityMask &= ~( 0x00000004u );
                visibilityMask |= 0x00000002u;
                break;
            case ShowOnlyPlaceholder:
                visibilityMask &= ~( 0x00000002u );
                visibilityMask |= 0x00000004u;
                break;
            }
            mGraphicsSystem->getSceneManager()->setVisibilityMask( visibilityMask );
        }
        else if( arg.keysym.scancode == SDL_SCANCODE_KP_PLUS )
        {
            if( mTransparencyValue < 1.0f )
            {
                mTransparencyValue += 0.1f;
                mTransparencyValue = Ogre::min( mTransparencyValue, 1.0f );
                setTransparencyToMaterials();
            }
        }
        else if( arg.keysym.scancode == SDL_SCANCODE_MINUS ||
                 arg.keysym.scancode == SDL_SCANCODE_KP_MINUS )
        {
            if( mTransparencyValue > 0.0f )
            {
                mTransparencyValue -= 0.1f;
                mTransparencyValue = Ogre::max( mTransparencyValue, 0.0f );
                setTransparencyToMaterials();
            }
        }
        else
        {
            TutorialGameState::keyReleased( arg );
        }
    }
}  // namespace Demo
