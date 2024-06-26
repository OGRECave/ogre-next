
#include "ManyMaterialsGameState.h"

#include "GraphicsSystem.h"

#include "OgreHlmsUnlitDatablock.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceDef.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h"
#include "OgreCamera.h"
#include "OgreHlms.h"
#include "OgreItem.h"
#include "OgreLogManager.h"
#include "OgreLwString.h"
#include "OgreMesh.h"
#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreTextureBox.h"
#include "OgreTextureGpu.h"
#include "OgreTextureGpuManager.h"

using namespace Demo;

ManyMaterialsGameState::ManyMaterialsGameState( const Ogre::String &helpDescription ) :
    TutorialGameState( helpDescription ),
    mCurrentMaterialId( 0u ),
    mPlaneItem( 0 ),
    mRgbaReference( 0 ),
    mTextureBox( 0 ),
    mErrorDetected( false )
{
}
//-----------------------------------------------------------------------------------
Ogre::HlmsUnlitDatablock *ManyMaterialsGameState::createUnlitDatablock( const Ogre::ColourValue &colour )
{
    Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
    Ogre::Hlms *hlmsUnlit = hlmsManager->getHlms( Ogre::HLMS_UNLIT );

    const std::string datablockId = "Test/" + Ogre::StringConverter::toString( mCurrentMaterialId++ );

    Ogre::HlmsUnlitDatablock *newDatablock = (Ogre::HlmsUnlitDatablock *)hlmsUnlit->createDatablock(
        datablockId, datablockId, Ogre::HlmsMacroblock(), Ogre::HlmsBlendblock(), Ogre::HlmsParamVec(),
        true );

    newDatablock->setUseColour( true );
    newDatablock->setColour( colour );

    return newDatablock;
}
//-----------------------------------------------------------------------------------
Ogre::HlmsUnlitDatablock *ManyMaterialsGameState::generateMaterial( size_t newDummyMaterials,
                                                                    const Ogre::ColourValue &colour )
{
    for( size_t i = 0u; i < newDummyMaterials; ++i )
        createUnlitDatablock( Ogre::ColourValue::White );
    return createUnlitDatablock( colour );
}
//-----------------------------------------------------------------------------------
void ManyMaterialsGameState::createScene01()
{
    Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

    Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane(
        "Plane v1", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        Ogre::Plane( Ogre::Vector3::UNIT_Z, 0.0f ), 50.0f, 50.0f, 1, 1, true, 1, 4.0f, 4.0f,
        Ogre::Vector3::UNIT_Y, Ogre::v1::HardwareBuffer::HBU_STATIC,
        Ogre::v1::HardwareBuffer::HBU_STATIC );

    Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createByImportingV1(
        "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, planeMeshV1.get(), true, true,
        true );

    {
        // We must alter the AABB because we want to always pass frustum culling
        // Otherwise frustum culling may hide bugs in the projection matrix math
        planeMesh->load();
        Ogre::Aabb aabb = planeMesh->getAabb();
        aabb.mHalfSize.z = aabb.mHalfSize.x;
        planeMesh->_setBounds( aabb );
    }

    Ogre::Item *item = sceneManager->createItem( planeMesh, Ogre::SCENE_DYNAMIC );
    Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                                     ->createChildSceneNode( Ogre::SCENE_DYNAMIC );
    sceneNode->setScale( Ogre::Vector3( 1000.0f ) );
    sceneNode->attachObject( item );
    mPlaneItem = item;

    Ogre::Light *light = sceneManager->createLight();
    Ogre::SceneNode *lightNode = sceneManager->getRootSceneNode()->createChildSceneNode();
    lightNode->attachObject( light );
    light->setPowerScale( Ogre::Math::PI );  // Since we don't do HDR, counter the PBS' division by
                                             // PI
    light->setType( Ogre::Light::LT_DIRECTIONAL );
    light->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );

    Ogre::Camera *camera = mGraphicsSystem->getCamera();
    camera->setPosition( 0, 0, 0 );
    camera->setOrientation( Ogre::Quaternion::IDENTITY );

    camera->setNearClipDistance( 0.5f );
    sceneNode->setPosition( 0, 0, -camera->getNearClipDistance() - 50.0f );

    {
        using namespace Ogre;
        CompositorManager2 *compositorManager = mGraphicsSystem->getRoot()->getCompositorManager2();
        CompositorNodeDef *nodeDef = compositorManager->addNodeDefinition( "ManyMaterials Node" );

        // Input texture
        nodeDef->addTextureSourceName( "WindowRT", 0, TextureDefinitionBase::TEXTURE_INPUT );

        nodeDef->setNumTargetPass( 1 );
        {
            CompositorTargetDef *targetDef = nodeDef->addTargetPass( "WindowRT" );
            targetDef->setNumPasses( 1 );
            {
                {
                    CompositorPassSceneDef *passScene =
                        static_cast<CompositorPassSceneDef *>( targetDef->addPass( PASS_SCENE ) );
                    passScene->setAllClearColours( Ogre::ColourValue( 1.0f, 0.5f, 0.0f, 1.0f ) );
                    passScene->setAllLoadActions( LoadAction::Clear );
                    passScene->mIncludeOverlays = false;
                }
            }
        }

        CompositorWorkspaceDef *workDef =
            compositorManager->addWorkspaceDefinition( "ManyMaterials Workspace" );
        workDef->connectExternal( 0, nodeDef->getName(), 0 );
    }

    TutorialGameState::createScene01();
}
//-----------------------------------------------------------------------------------
void ManyMaterialsGameState::update( float timeSinceLast )
{
    Ogre::TextureGpuManager *textureManager =
        mGraphicsSystem->getRoot()->getRenderSystem()->getTextureGpuManager();

    Ogre::TextureGpu *manyMaterialsTex =
        textureManager->createTexture( "ManyMaterials Tex", Ogre::GpuPageOutStrategy::Discard,
                                       Ogre::TextureFlags::RenderToTexture, Ogre::TextureTypes::Type2D );
    const Ogre::uint32 resolution = 96u;
    manyMaterialsTex->setResolution( resolution, resolution );
    manyMaterialsTex->setPixelFormat( Ogre::PFG_RGBA8_UNORM );
    manyMaterialsTex->scheduleTransitionTo( Ogre::GpuResidency::Resident );

    // const Ogre::PixelFormatGpu pixelFormat = manyMaterialsTex->getPixelFormat();

    mGraphicsSystem->getSceneManager()->updateSceneGraph();

    Ogre::CompositorManager2 *compositorManager = mGraphicsSystem->getRoot()->getCompositorManager2();
    Ogre::CompositorWorkspace *workspace = compositorManager->addWorkspace(
        mGraphicsSystem->getSceneManager(), manyMaterialsTex, mGraphicsSystem->getCamera(),
        "ManyMaterials Workspace", false );

    const size_t iterations = 200u;

    for( size_t i = 0u; i < iterations; ++i )
    {
        // Choose random colour
        Ogre::ColourValue randColour(
            Ogre::Math::RangeRandom( 0.0f, 1.0f ), Ogre::Math::RangeRandom( 0.0f, 1.0f ),
            Ogre::Math::RangeRandom( 0.0f, 1.0f ), Ogre::Math::RangeRandom( 0.0f, 1.0f ) );

        // Quantize
        const Ogre::RGBA asRgba = randColour.getAsRGBA();
        randColour.setAsRGBA( asRgba );
        mRgbaReference = asRgba;

        // Use a prime number as extra materials.
        Ogre::HlmsUnlitDatablock *newMaterial = generateMaterial( 67u, randColour );

        mPlaneItem->setDatablock( newMaterial );

        uint32_t rgba = randColour.getAsABGR();
        const uint8_t *rgba8 = reinterpret_cast<const uint8_t *>( &rgba );

        Ogre::LogManager::getSingleton().logMessage(
            "Testing colour: " + std::to_string( rgba8[0] ) + " " + std::to_string( rgba8[1] ) + " " +
                std::to_string( rgba8[2] ) + " " + std::to_string( rgba8[3] ),
            Ogre::LML_CRITICAL );

        workspace->_validateFinalTarget();
        workspace->_beginUpdate( false );
        workspace->_update();
        workspace->_endUpdate( false );

        Ogre::Image2 image;
        image.convertFromTexture( manyMaterialsTex, 0u, 0u );
        // image.save( "/home/matias/tttt.png",0,1 );

        Ogre::TextureBox box = image.getData( 0u );
        mTextureBox = &box;

        mGraphicsSystem->getSceneManager()->executeUserScalableTask( this, true );
        // execute(0u,1u);

        if( mErrorDetected )
        {
            Ogre::LogManager::getSingleton().logMessage(
                "Mismatch detected!. Expected value: " + std::to_string( rgba8[0] ) + " " +
                    std::to_string( rgba8[1] ) + " " + std::to_string( rgba8[2] ) + " " +
                    std::to_string( rgba8[3] ) + " Got instead: " + std::to_string( mRgbaResult[0] ) +
                    " " + std::to_string( mRgbaResult[1] ) + " " + std::to_string( mRgbaResult[2] ) +
                    " " + std::to_string( mRgbaResult[3] ),
                Ogre::LML_CRITICAL );

            mErrorDetected = false;

            OGRE_EXCEPT( Ogre::Exception::ERR_RT_ASSERTION_FAILED, "Mismatch detected!",
                         "Test failed!" );
        }
    }

    compositorManager->removeWorkspace( workspace );
    textureManager->destroyTexture( manyMaterialsTex );

    mGraphicsSystem->setQuit();

    TutorialGameState::update( timeSinceLast );
}
//-----------------------------------------------------------------------------------
void ManyMaterialsGameState::execute( size_t threadId, size_t numThreads )
{
    const Ogre::uint32 rgbaRef = mRgbaReference;
    const Ogre::TextureBox box = *mTextureBox;

    const uint8_t *refValue = reinterpret_cast<const Ogre::uint8 *>( &rgbaRef );

    const size_t heightToProcess = std::max<size_t>( 1u, box.height / numThreads );
    const size_t heightStart = heightToProcess * threadId;

    // Clamp heightEnd (in case there's more threads than rows)
    // Ceil heightEnd for the last thread (when box.height / numThreads is not perfectly divisible)
    size_t heightEnd = heightToProcess * ( threadId + 1u );
    if( ( threadId + 2u ) * heightToProcess > box.height )
        heightEnd = box.height;

    for( size_t y = heightStart; y < heightEnd; ++y )
    {
        for( size_t x = 0u; x < box.width; ++x )
        {
            const Ogre::uint8 *dataPtr = reinterpret_cast<const Ogre::uint8 *>( box.data ) +
                                         y * box.bytesPerRow + x * box.bytesPerPixel;

            // const Ogre::ColourValue readValue = box.getColourAt( x, y, 0u, pixelFormat );
            // if( readValue != randColour )
            if( dataPtr[0] != refValue[3] || dataPtr[1] != refValue[2] || dataPtr[2] != refValue[1] ||
                dataPtr[3] != refValue[0] )
            {
                if( !mErrorDetected )
                {
                    mRgbaResult[0] = dataPtr[0];
                    mRgbaResult[1] = dataPtr[1];
                    mRgbaResult[2] = dataPtr[2];
                    mRgbaResult[3] = dataPtr[3];
                }
                mErrorDetected = true;
            }
        }
    }
}
