
#include "EndFrameOnceFailureGameState.h"

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
#include "Vao/OgreVaoManager.h"

using namespace Demo;

namespace Demo
{
    EndFrameOnceFailureGameState::EndFrameOnceFailureGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mUnlitDatablock( 0 ),
        mRgbaReference( 0 ),
        mTextureBox( 0 ),
        mRaceConditionDetected( false ),
        mCurrFrame( 0u )
    {
    }
    //-----------------------------------------------------------------------------------
    void EndFrameOnceFailureGameState::createScene01()
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        Ogre::TextureGpuManager *textureManager =
            mGraphicsSystem->getRoot()->getRenderSystem()->getTextureGpuManager();
        textureManager->setStagingTextureMaxBudgetBytes( 1u );

        Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane(
            "Plane v1", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::Plane( Ogre::Vector3::UNIT_Z, 0.0f ), 50.0f, 50.0f, 1, 1, true, 1, 4.0f, 4.0f,
            Ogre::Vector3::UNIT_Y, Ogre::v1::HardwareBuffer::HBU_STATIC,
            Ogre::v1::HardwareBuffer::HBU_STATIC );

        Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createByImportingV1(
            "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, planeMeshV1.get(), true,
            true, true );

        {
            Ogre::Hlms *hlmsUnlit =
                mGraphicsSystem->getRoot()->getHlmsManager()->getHlms( Ogre::HLMS_UNLIT );

            mUnlitDatablock = static_cast<Ogre::HlmsUnlitDatablock *>( hlmsUnlit->createDatablock(
                "EndFrameOnceFailure Test Material", "EndFrameOnceFailure Test Material",
                Ogre::HlmsMacroblock(), Ogre::HlmsBlendblock(), Ogre::HlmsParamVec() ) );
            mUnlitDatablock->setUseColour( true );
        }

        {
            // We must alter the AABB because we want to always pass frustum culling
            // Otherwise frustum culling may hide bugs in the projection matrix math
            planeMesh->load();
            Ogre::Aabb aabb = planeMesh->getAabb();
            aabb.mHalfSize.z = aabb.mHalfSize.x;
            planeMesh->_setBounds( aabb );
        }

        Ogre::Item *item = sceneManager->createItem( planeMesh, Ogre::SCENE_DYNAMIC );
        item->setDatablock( mUnlitDatablock );
        Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                                         ->createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->setScale( Ogre::Vector3( 1000.0f ) );
        sceneNode->attachObject( item );

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
            CompositorNodeDef *nodeDef =
                compositorManager->addNodeDefinition( "EndFrameOnceFailure Node" );

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
                compositorManager->addWorkspaceDefinition( "EndFrameOnceFailure Workspace" );
            workDef->connectExternal( 0, nodeDef->getName(), 0 );
        }

        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
    void EndFrameOnceFailureGameState::simulateSpuriousBufferDisposal()
    {
        Ogre::RenderSystem *renderSystem = mGraphicsSystem->getRoot()->getRenderSystem();
        Ogre::VaoManager *vaoManager = renderSystem->getVaoManager();

        Ogre::IndexBufferPacked *dummyBuffer = vaoManager->createIndexBuffer(
            Ogre::IT_16BIT, 16000, Ogre::BT_DYNAMIC_PERSISTENT, 0, false );
        dummyBuffer->map( 0, dummyBuffer->getNumElements() );
        dummyBuffer->unmap( Ogre::UO_UNMAP_ALL );
        vaoManager->destroyIndexBuffer( dummyBuffer );
    }
    //-----------------------------------------------------------------------------------
    void EndFrameOnceFailureGameState::update( float timeSinceLast )
    {
        Ogre::RenderSystem *renderSystem = mGraphicsSystem->getRoot()->getRenderSystem();
        Ogre::TextureGpuManager *textureManager = renderSystem->getTextureGpuManager();
        Ogre::VaoManager *vaoManager = renderSystem->getVaoManager();

        simulateSpuriousBufferDisposal();

        Ogre::TextureGpu *readbackTex = textureManager->createTexture(
            "EndFrameOnceFailure Tex", Ogre::GpuPageOutStrategy::Discard,
            Ogre::TextureFlags::RenderToTexture, Ogre::TextureTypes::Type2D );
        const Ogre::uint32 resolution = 96u;
        readbackTex->setResolution( resolution, resolution );
        readbackTex->setPixelFormat( Ogre::PFG_RGBA8_UNORM );
        readbackTex->scheduleTransitionTo( Ogre::GpuResidency::Resident );

        // const Ogre::PixelFormatGpu pixelFormat = readbackTex->getPixelFormat();

        mGraphicsSystem->getSceneManager()->updateSceneGraph();

        Ogre::CompositorManager2 *compositorManager =
            mGraphicsSystem->getRoot()->getCompositorManager2();
        Ogre::CompositorWorkspace *workspace = compositorManager->addWorkspace(
            mGraphicsSystem->getSceneManager(), readbackTex, mGraphicsSystem->getCamera(),
            "EndFrameOnceFailure Workspace", false );

        const size_t iterations = size_t( std::ceil( Ogre::Math::RangeRandom( 1.0f, 50.0f ) ) );
        for( size_t i = 0u; i < iterations; ++i )
        {
            if( mCurrFrame % 64u < 32u )
            {
                // Check that multiple VaoManager::_update in a row don't break.
                for( size_t i = 0u; i < iterations; ++i )
                    simulateSpuriousBufferDisposal();
                vaoManager->_update();
            }
            else
            {
                // Check that unexpected calls to _endFrameOnce don't break.
                simulateSpuriousBufferDisposal();

                // Choose random colour
                Ogre::ColourValue randColour(
                    Ogre::Math::RangeRandom( 0.0f, 1.0f ), Ogre::Math::RangeRandom( 0.0f, 1.0f ),
                    Ogre::Math::RangeRandom( 0.0f, 1.0f ), Ogre::Math::RangeRandom( 0.0f, 1.0f ) );

                // Quantize
                const Ogre::RGBA asRgba = randColour.getAsRGBA();
                randColour.setAsRGBA( asRgba );
                mRgbaReference = asRgba;

                mUnlitDatablock->setColour( randColour );

                uint32_t rgba = randColour.getAsABGR();
                const uint8_t *rgba8 = reinterpret_cast<const uint8_t *>( &rgba );

                Ogre::LogManager::getSingleton().logMessage(
                    "Testing colour: " + std::to_string( rgba8[0] ) + " " + std::to_string( rgba8[1] ) +
                        " " + std::to_string( rgba8[2] ) + " " + std::to_string( rgba8[3] ),
                    Ogre::LML_CRITICAL );

                workspace->_validateFinalTarget();
                workspace->_beginUpdate( false );
                workspace->_update();
                workspace->_endUpdate( false );

                Ogre::Image2 image;
                image.convertFromTexture( readbackTex, 0u, 0u );

                renderSystem->_endFrameOnce();

                if( ( i & 0x1u ) == 0u )
                {
                    Ogre::TextureBox box = image.getData( 0u );
                    mTextureBox = &box;

                    mGraphicsSystem->getSceneManager()->executeUserScalableTask( this, true );
                    // execute(0u,1u);

                    if( mRaceConditionDetected )
                    {
                        Ogre::LogManager::getSingleton().logMessage(
                            "Race condition detected!. Expected value: " + std::to_string( rgba8[0] ) +
                                " " + std::to_string( rgba8[1] ) + " " + std::to_string( rgba8[2] ) +
                                " " + std::to_string( rgba8[3] ) +
                                " Got instead: " + std::to_string( mRgbaResult[0] ) + " " +
                                std::to_string( mRgbaResult[1] ) + " " +
                                std::to_string( mRgbaResult[2] ) + " " +
                                std::to_string( mRgbaResult[3] ),
                            Ogre::LML_CRITICAL );

                        mRaceConditionDetected = false;

                        OGRE_EXCEPT( Ogre::Exception::ERR_RT_ASSERTION_FAILED,
                                     "Race condition detected!", "Test failed!" );
                    }
                }
            }
        }

        compositorManager->removeWorkspace( workspace );
        textureManager->destroyTexture( readbackTex );

        // We don't to give a seizure to users
        mUnlitDatablock->setColour( Ogre::ColourValue::Black );

        TutorialGameState::update( timeSinceLast );

        ++mCurrFrame;
    }
    //-----------------------------------------------------------------------------------
    void EndFrameOnceFailureGameState::execute( size_t threadId, size_t numThreads )
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
                if( dataPtr[0] != refValue[3] || dataPtr[1] != refValue[2] ||
                    dataPtr[2] != refValue[1] || dataPtr[3] != refValue[0] )
                {
                    if( !mRaceConditionDetected )
                    {
                        mRgbaResult[0] = dataPtr[0];
                        mRgbaResult[1] = dataPtr[1];
                        mRgbaResult[2] = dataPtr[2];
                        mRgbaResult[3] = dataPtr[3];
                    }
                    mRaceConditionDetected = true;
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void EndFrameOnceFailureGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
    {
        TutorialGameState::generateDebugText( timeSinceLast, outText );
        outText +=
            "\nThis test draws a random colour to an offscreen RTT and downloads\n"
            "its contents. If the colour doesn't match we throw an error.";
    }
}  // namespace Demo
