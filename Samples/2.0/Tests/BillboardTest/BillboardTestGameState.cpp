
#include "BillboardTestGameState.h"

#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreBillboardSet.h"
#include "OgreCamera.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsSamplerblock.h"
#include "OgreHlmsUnlit.h"
#include "OgreHlmsUnlitDatablock.h"
#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"

namespace Demo
{
    BillboardTestGameState::BillboardTestGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription )
    {
    }
    //-----------------------------------------------------------------------------------
    void BillboardTestGameState::createScene01()
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

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
            Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                                             ->createChildSceneNode( Ogre::SCENE_DYNAMIC );
            sceneNode->setPosition( 0, -1, 0 );
            sceneNode->attachObject( item );
        }

        Ogre::SceneNode *rootNode = sceneManager->getRootSceneNode();

        Ogre::Light *light = sceneManager->createLight();
        Ogre::SceneNode *lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( light );
        light->setPowerScale( 1.0f );
        light->setType( Ogre::Light::LT_DIRECTIONAL );
        light->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );
        mCameraController = new CameraController( mGraphicsSystem, false );

        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();

        Ogre::HlmsUnlitDatablock *datablock = 0;

        Ogre::HlmsSamplerblock samplerBlock;
        samplerBlock.setAddressingMode( Ogre::TAM_WRAP );

        Ogre::HlmsBlendblock blendBlock;
        blendBlock.setBlendType( Ogre::SBT_TRANSPARENT_ALPHA );

        Ogre::HlmsMacroblock macroBlock;
        macroBlock.mDepthCheck = false;
        macroBlock.mDepthWrite = false;

        const Ogre::String datablockName = "BillBoardDatablock";
        OGRE_ASSERT( dynamic_cast<Ogre::HlmsUnlit *>( hlmsManager->getHlms( Ogre::HLMS_UNLIT ) ) );
        Ogre::HlmsUnlit *hlmsUnlit =
            static_cast<Ogre::HlmsUnlit *>( hlmsManager->getHlms( Ogre::HLMS_UNLIT ) );

        datablock = static_cast<Ogre::HlmsUnlitDatablock *>(
            hlmsUnlit->createDatablock( Ogre::IdString( datablockName ), datablockName, macroBlock,
                                        blendBlock, Ogre::HlmsParamVec() ) );
        datablock->setTexture( 0u, "leaf.png" );
        datablock->setSamplerblock( 0, samplerBlock );
        datablock->setTextureUvSource( 0, 0 );
        datablock->setTextureSwizzle( 0, Ogre::HlmsUnlitDatablock::R_MASK,
                                      Ogre::HlmsUnlitDatablock::G_MASK, Ogre::HlmsUnlitDatablock::B_MASK,
                                      Ogre::HlmsUnlitDatablock::A_MASK );

        Ogre::ColourValue col;
        col = Ogre::ColourValue( 1.0f, 0.0f, 0.0f, 1.0f );

        Ogre::RGBA rgb_colour;
        Ogre::BGRA bgr_colour;
        Ogre::Root::getSingleton().convertColourValue( col, &rgb_colour );
        Ogre::Root::getSingleton().convertColourValue( col, &bgr_colour );

        OGRE_ASSERT( ( rgb_colour == bgr_colour ) && ( rgb_colour == 4278190335 ) );
        OGRE_ASSERT( Ogre::v1::VertexElement::getBestColourVertexElementType() ==
                     Ogre::VET_COLOUR_ABGR );

        //  BestColourVertexElementType Vulkan (VET_COLOUR_ARGB) OpenGL (VET_COLOUR_ABGR)
        switch( Ogre::v1::VertexElement::getBestColourVertexElementType() )
        {
        //  Vulkan workaround swap R and B colour channels
        case Ogre::VET_COLOUR_ARGB:
            // col = Ogre::ColourValue(col.b,col.g,col.r,col.a);
            break;
        default:
            break;
        }

        const float size = 10.0f;

        Ogre::v1::BillboardSet *bbs = sceneManager->createBillboardSet( 16 );
        bbs->setDefaultDimensions( size, size );
        bbs->setDatablock( datablock );

        for( int i = 0; i < 2; i++ )
        {
            for( int j = 0; j < 2; j++ )
            {
                const Ogre::Vector3 pos =
                    Ogre::Vector3( ( i - 1 ) * bbs->getDefaultWidth(),
                                   size + ( j - 1 ) * bbs->getDefaultHeight() - 10.0f, -10.0f );
                bbs->createBillboard( pos, col );
            }
        }
        Ogre::SceneNode *sceneNode = rootNode->createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->attachObject( bbs );

        TutorialGameState::createScene01();
    }
}  // namespace Demo
