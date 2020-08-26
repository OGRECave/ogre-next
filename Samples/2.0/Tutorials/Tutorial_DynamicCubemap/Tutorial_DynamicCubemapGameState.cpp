
#include "Tutorial_DynamicCubemapGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreSceneManager.h"
#include "OgreItem.h"

#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreMesh2.h"

#include "OgreCamera.h"
#include "OgreWindow.h"

#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsSamplerblock.h"

#include "OgreRoot.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"

#include "OgreTextureGpuManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/OgreCompositorWorkspaceDef.h"

#include "Compositor/Pass/PassIblSpecular/OgreCompositorPassIblSpecularDef.h"

using namespace Demo;

namespace Demo
{
    DynamicCubemapGameState::DynamicCubemapGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mAnimateObjects( true ),
        mIblQuality( IblLow ),
        mCubeCamera( 0 ),
        mDynamicCubemapWorkspace( 0 )
    {
        memset( mSceneNode, 0, sizeof(mSceneNode) );
    }
    //-----------------------------------------------------------------------------------
    Ogre::CompositorWorkspace* DynamicCubemapGameState::setupCompositor()
    {
        // We first create the Cubemap workspace and pass it to the final workspace
        // that does the real rendering.
        //
        // If in your application you need to create a workspace but don't have a cubemap yet,
        // you can either programatically modify the workspace definition (which is cumbersome)
        // or just pass a PF_NULL texture that works as a dud and barely consumes any memory.
        // See Tutorial_Terrain for an example of PF_NULL dud.
        using namespace Ogre;

        Root *root = mGraphicsSystem->getRoot();
        SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        Window *renderWindow = mGraphicsSystem->getRenderWindow();
        Camera *camera = mGraphicsSystem->getCamera();
        CompositorManager2 *compositorManager = root->getCompositorManager2();

        if( mDynamicCubemapWorkspace )
        {
            compositorManager->removeWorkspace( mDynamicCubemapWorkspace );
            mDynamicCubemapWorkspace = 0;
        }

        uint32 iblSpecularFlag = 0;
        if( root->getRenderSystem()->getCapabilities()->hasCapability( RSC_COMPUTE_PROGRAM ) &&
            mIblQuality != MipmapsLowest )
        {
            iblSpecularFlag = TextureFlags::Uav | TextureFlags::Reinterpretable;
        }

        //A RenderTarget created with AllowAutomipmaps means the compositor still needs to
        //explicitly generate the mipmaps by calling generate_mipmaps. It's just an API
        //hint to tell the GPU we will be using the mipmaps auto generation routines.
        TextureGpuManager *textureManager = root->getRenderSystem()->getTextureGpuManager();
        mDynamicCubemap =
            textureManager->createOrRetrieveTexture( "DynamicCubemap",
                                                     GpuPageOutStrategy::Discard,          //
                                                     TextureFlags::RenderToTexture |       //
                                                         TextureFlags::AllowAutomipmaps |  //
                                                         iblSpecularFlag,                  //
                                                     TextureTypes::TypeCube );
        mDynamicCubemap->scheduleTransitionTo( GpuResidency::OnStorage );
        uint32 resolution = 512u;
        if( mIblQuality == MipmapsLowest )
            resolution = 1024u;
        else if( mIblQuality == IblLow )
            resolution = 256u;
        else
            resolution = 512u;
        mDynamicCubemap->setResolution( resolution, resolution );
        mDynamicCubemap->setNumMipmaps( PixelFormatGpuUtils::getMaxMipmapCount( resolution ) );
        if( mIblQuality != MipmapsLowest )
        {
            // Limit max mipmap to 16x16
            mDynamicCubemap->setNumMipmaps( mDynamicCubemap->getNumMipmaps() - 4u );
        }
        mDynamicCubemap->setPixelFormat( PFG_RGBA8_UNORM_SRGB );
        mDynamicCubemap->scheduleTransitionTo( GpuResidency::Resident );

        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms(Ogre::HLMS_PBS) );
        hlmsPbs->resetIblSpecMipmap( 0u );

        // Create the camera used to render to our cubemap
        if( !mCubeCamera )
        {
            mCubeCamera = sceneManager->createCamera( "CubeMapCamera", true, true );
            mCubeCamera->setFOVy( Degree(90) );
            mCubeCamera->setAspectRatio( 1 );
            mCubeCamera->setFixedYawAxis(false);
            mCubeCamera->setNearClipDistance(0.5);
            //The default far clip distance is way too big for a cubemap-capable camera,
            //hich prevents Ogre from better culling.
            mCubeCamera->setFarClipDistance( 10000 );
            mCubeCamera->setPosition( 0, 1.0, 0 );
        }

        // Note: You don't necessarily have to tie RenderWindow's use of MSAA with cubemap's MSAA
        // You could always use MSAA for the cubemap, or never use MSAA for the cubemap.
        // That's up to you. This sample is tying them together in order to showcase them. That's all.
        const IdString cubemapRendererNode = renderWindow->getSampleDescription().isMultisample()
                                                 ? "CubemapRendererNodeMsaa"
                                                 : "CubemapRendererNode";

        {
            CompositorNodeDef *nodeDef =
                compositorManager->getNodeDefinitionNonConst( cubemapRendererNode );
            const CompositorPassDefVec &passes =
                nodeDef->getTargetPass( nodeDef->getNumTargetPasses() - 1u )->getCompositorPasses();

            OGRE_ASSERT_HIGH( dynamic_cast<CompositorPassIblSpecularDef *>( passes.back() ) );
            CompositorPassIblSpecularDef *iblSpecPassDef =
                static_cast<CompositorPassIblSpecularDef *>( passes.back() );
            iblSpecPassDef->mForceMipmapFallback = mIblQuality == MipmapsLowest;
            iblSpecPassDef->mSamplesPerIteration = mIblQuality == IblLow ? 32.0f : 128.0f;
            iblSpecPassDef->mSamplesSingleIterationFallback = iblSpecPassDef->mSamplesPerIteration;
        }

        //Setup the cubemap's compositor.
        CompositorChannelVec cubemapExternalChannels( 1 );
        //Any of the cubemap's render targets will do
        cubemapExternalChannels[0] = mDynamicCubemap;

        const Ogre::String workspaceName( "Tutorial_DynamicCubemap_cubemap" );
        if( !compositorManager->hasWorkspaceDefinition( workspaceName ) )
        {
            CompositorWorkspaceDef *workspaceDef = compositorManager->addWorkspaceDefinition(
                                                                                    workspaceName );
            //"CubemapRendererNode" has been defined in scripts.
            //Very handy (as it 99% the same for everything)
            workspaceDef->connectExternal( 0, cubemapRendererNode, 0 );
        }

        ResourceLayoutMap initialCubemapLayouts;
        ResourceAccessMap initialCubemapUavAccess;
        mDynamicCubemapWorkspace =
                compositorManager->addWorkspace( sceneManager, cubemapExternalChannels, mCubeCamera,
                                                 workspaceName, true, -1, (UavBufferPackedVec*)0,
                                                 &initialCubemapLayouts, &initialCubemapUavAccess );

        //Now setup the regular renderer
        CompositorChannelVec externalChannels( 2 );
        //Render window
        externalChannels[0] = renderWindow->getTexture();
        externalChannels[1] = mDynamicCubemap;

        return compositorManager->addWorkspace( sceneManager, externalChannels, camera,
                                                "Tutorial_DynamicCubemapWorkspace",
                                                true, -1, (UavBufferPackedVec*)0,
                                                &initialCubemapLayouts, &initialCubemapUavAccess );
    }
    //-----------------------------------------------------------------------------------
    void DynamicCubemapGameState::createScene01(void)
    {
        //Setup a scene similar to that of PBS sample, except
        //we apply the cubemap to everything via C++ code
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        const float armsLength = 2.5f;

        Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane( "Plane v1",
                                            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                            Ogre::Plane( Ogre::Vector3::UNIT_Y, 1.0f ), 50.0f, 50.0f,
                                            1, 1, true, 1, 4.0f, 4.0f, Ogre::Vector3::UNIT_Z,
                                            Ogre::v1::HardwareBuffer::HBU_STATIC,
                                            Ogre::v1::HardwareBuffer::HBU_STATIC );

        Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createByImportingV1(
                    "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    planeMeshV1.get(), true, true, true );

        {
            Ogre::Item *item = sceneManager->createItem( planeMesh, Ogre::SCENE_DYNAMIC );
            item->setDatablock( "Marble" );
            Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                                                    createChildSceneNode( Ogre::SCENE_DYNAMIC );
            sceneNode->setPosition( 0, -1, 0 );
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
            datablock->setTexture( Ogre::PBSM_REFLECTION, mDynamicCubemap );
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

                size_t idx = i * 4u + j;

                mSceneNode[idx] = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                        createChildSceneNode( Ogre::SCENE_DYNAMIC );

                mSceneNode[idx]->setPosition( (i - 1.5f) * armsLength,
                                              2.0f,
                                              (j - 1.5f) * armsLength );
                mSceneNode[idx]->setScale( 0.65f, 0.65f, 0.65f );

                mSceneNode[idx]->roll( Ogre::Radian( (Ogre::Real)idx ) );

                mSceneNode[idx]->attachObject( item );
                mObjects.push_back( item );
            }
        }

        {
            size_t numSpheres = 0;
            Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();

            assert( dynamic_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );

            Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms(Ogre::HLMS_PBS) );

            {
                Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                            hlmsPbs->getDatablock( "Rocks" ) );
                datablock->setTexture( Ogre::PBSM_REFLECTION, mDynamicCubemap );
            }

            const int numX = 8;
            const int numZ = 8;

            const float armsLength = 1.0f;
            const float startX = (numX-1) / 2.0f;
            const float startZ = (numZ-1) / 2.0f;

            for( int x=0; x<numX; ++x )
            {
                for( int z=0; z<numZ; ++z )
                {
                    Ogre::String datablockName = "Test" + Ogre::StringConverter::toString( numSpheres++ );
                    Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                                hlmsPbs->createDatablock( datablockName,
                                                          datablockName,
                                                          Ogre::HlmsMacroblock(),
                                                          Ogre::HlmsBlendblock(),
                                                          Ogre::HlmsParamVec() ) );

                    //Set the dynamic cubemap to these materials.
                    datablock->setTexture( Ogre::PBSM_REFLECTION, mDynamicCubemap );
                    datablock->setDiffuse( Ogre::Vector3( 0.0f, 1.0f, 0.0f ) );

                    datablock->setRoughness( std::max( 0.02f, x / std::max( 1.0f, (float)(numX-1) ) ) );
                    datablock->setFresnel( Ogre::Vector3( z / std::max( 1.0f, (float)(numZ-1) ) ), false );

                    Ogre::Item *item = sceneManager->createItem( "Sphere1000.mesh",
                                                                 Ogre::ResourceGroupManager::
                                                                 AUTODETECT_RESOURCE_GROUP_NAME,
                                                                 Ogre::SCENE_DYNAMIC );
                    item->setDatablock( datablock );
                    item->setVisibilityFlags( 0x000000002 | 0x00000004 );

                    Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                            createChildSceneNode( Ogre::SCENE_DYNAMIC );
                    sceneNode->setPosition( Ogre::Vector3( armsLength * x - startX,
                                                           1.0f,
                                                           armsLength * z - startZ ) );
                    sceneNode->attachObject( item );
                    mSpheres.push_back( item );
                }
            }

            // At startup resetIblSpecMipmap() was called but no scene was yet set
            // and thus no cubemap, thus num mipmap was set to 1 (which is wrong).
            hlmsPbs->resetIblSpecMipmap( 0u );
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
        light->setDiffuseColour( 0.8f, 0.4f, 0.2f ); //Warm
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
        light->setDiffuseColour( 0.2f, 0.4f, 0.8f ); //Cold
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
    void DynamicCubemapGameState::destroyScene(void)
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        Ogre::CompositorManager2 *compositorManager = root->getCompositorManager2();

        compositorManager->removeWorkspace( mDynamicCubemapWorkspace );
        mDynamicCubemapWorkspace = 0;

        Ogre::TextureGpuManager *textureManager = root->getRenderSystem()->getTextureGpuManager();
        textureManager->destroyTexture( mDynamicCubemap );
        mDynamicCubemap = 0;

        sceneManager->destroyCamera( mCubeCamera );
        mCubeCamera = 0;
    }
    //-----------------------------------------------------------------------------------
    void DynamicCubemapGameState::update( float timeSinceLast )
    {
        if( mAnimateObjects )
        {
            for( int i=0; i<16; ++i )
                mSceneNode[i]->yaw( Ogre::Radian(timeSinceLast * i * 0.125f) );
        }

        TutorialGameState::update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    void DynamicCubemapGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
    {
        Ogre::uint32 visibilityMask = mGraphicsSystem->getSceneManager()->getVisibilityMask();

        const char *c_iblQuality[] =
        {
            "[Lowest]",
            "[Low]",
            "[High]"
        };

        TutorialGameState::generateDebugText( timeSinceLast, outText );
        outText += "\nF2 to toggle animation. ";
        outText += mAnimateObjects ? "[On]" : "[Off]";
        outText += "\nF3 to show/hide animated objects. ";
        outText += (visibilityMask & 0x000000001) ? "[On]" : "[Off]";
        outText += "\nF4 to show/hide spheres from the reflection. ";
        outText += (mSpheres.back()->getVisibilityFlags() & 0x000000004) ? "[On]" : "[Off]";
        outText += "\nF5 to change reflection IBL quality ";
        outText += c_iblQuality[mIblQuality];
    }
    //-----------------------------------------------------------------------------------
    void DynamicCubemapGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( (arg.keysym.mod & ~(KMOD_NUM|KMOD_CAPS)) != 0 )
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
            bool showMovingObjects = (visibilityMask & 0x00000001);
            showMovingObjects = !showMovingObjects;
            visibilityMask &= ~0x00000001u;
            visibilityMask |= (Ogre::uint32)showMovingObjects;
            mGraphicsSystem->getSceneManager()->setVisibilityMask( visibilityMask );
        }
        else if( arg.keysym.sym == SDLK_F4 )
        {
            std::vector<Ogre::MovableObject*>::const_iterator itor = mSpheres.begin();
            std::vector<Ogre::MovableObject*>::const_iterator end  = mSpheres.end();
            while( itor != end )
            {
                Ogre::uint32 visibilityMask = (*itor)->getVisibilityFlags();
                bool showPalette = (visibilityMask & 0x00000004u) != 0u;
                showPalette = !showPalette;
                visibilityMask &= ~0x00000004u;
                visibilityMask |= (Ogre::uint32)(showPalette) << 2;

                (*itor)->setVisibilityFlags( visibilityMask );
                ++itor;
            }
        }
        else if( arg.keysym.sym == SDLK_F5 )
        {
            mIblQuality = static_cast<IblQuality>( (mIblQuality + 1u) % (IblHigh + 1u) );
            mGraphicsSystem->restartCompositor();
        }
        else
        {
            TutorialGameState::keyReleased( arg );
        }
    }
}
