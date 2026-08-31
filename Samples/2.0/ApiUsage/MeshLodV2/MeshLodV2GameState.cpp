#include "MeshLodV2GameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreItem.h"
#include "OgreSceneManager.h"
#include "OgreSubItem.h"

#include "OgreMesh.h"
#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreSubMesh2.h"

#include "OgreCamera.h"
#include "OgreWindow.h"

#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreRoot.h"

#include "OgreLodConfig.h"
#include "OgreLodStrategyManager.h"
#include "OgreMeshLodGenerator.h"
#include "OgrePixelCountLodStrategy.h"

#include "OgreLogManager.h"
#include "OgreStringConverter.h"

#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"
#include "Vao/OgreVertexBufferPacked.h"

#include <SDL_keycode.h>

using namespace Demo;

namespace
{
    /// Dummy ManualResourceLoader: the sphere mesh's geometry is filled in directly
    /// by createProceduralSphereMeshV2() before load() is ever called, so there is
    /// nothing for prepareResource()/loadResource() to actually do. A loader still
    /// has to be supplied to createManual(), or Ogre logs a "no manual loader
    /// provided" warning.
    class ProceduralMeshLoader : public Ogre::ManualResourceLoader
    {
    public:
        void prepareResource( Ogre::Resource * ) override {}
        void loadResource( Ogre::Resource * ) override {}
    };

    ProceduralMeshLoader gProceduralMeshLoader;
}  // namespace

namespace Demo
{
    MeshLodV2GameState::MeshLodV2GameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mSphereItem( 0 ),
        mSinbadItem( 0 ),
        mSphereDatablock( 0 ),
        mSinbadDatablock( 0 ),
        mWireframeOn( false ),
        mLastLoggedLodSphere( 0 ),
        mLastLoggedLodSinbad( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    Ogre::MeshPtr MeshLodV2GameState::createProceduralSphereMeshV2( const Ogre::String &meshName,
                                                                    float radius, unsigned numRings,
                                                                    unsigned numSegments )
    {
        Ogre::VaoManager *vaoManager = mGraphicsSystem->getRoot()->getRenderSystem()->getVaoManager();

        const unsigned rowStride = numSegments + 1u;
        const size_t numVertices = ( numRings + 1u ) * rowStride;
        const size_t numTriangles = static_cast<size_t>( numRings ) * numSegments * 2u;
        const size_t numIndices = numTriangles * 3u;

        // POS(3) + NORMAL(3) + UV(2) = 8 floats per vertex. No tangent: this sample
        // doesn't use a normal map, so we don't need one.
        const size_t floatsPerVertex = 8u;
        float *vertexData = reinterpret_cast<float *>( OGRE_MALLOC_SIMD(
            numVertices * floatsPerVertex * sizeof( float ), Ogre::MEMCATEGORY_GEOMETRY ) );

        size_t vOffset = 0u;
        for( unsigned r = 0u; r <= numRings; ++r )
        {
            const float v = static_cast<float>( r ) / static_cast<float>( numRings );
            const float phi = v * Ogre::Math::PI;  // 0 (north pole) .. PI (south pole)
            const float sinPhi = Ogre::Math::Sin( phi );
            const float cosPhi = Ogre::Math::Cos( phi );

            for( unsigned s = 0u; s <= numSegments; ++s )
            {
                const float u = static_cast<float>( s ) / static_cast<float>( numSegments );

                float nx, ny, nz;

                if( s == numSegments )
                {
                    // Wrap-around seam column (theta == 2*PI, the same physical
                    // point as s == 0's theta == 0). Copy the first column's
                    // position/normal bit-for-bit instead of recomputing via
                    // sin/cos(2*PI): floating point doesn't guarantee
                    // sin(2*PI) == sin(0) exactly, and that tiny divergence was
                    // enough to flip winding/normals on a sliver of triangles right
                    // at the seam, backface-culling them and showing the background
                    // through a visible crack (reproducible even at full LOD-0
                    // detail, so this was never a LOD-collapse bug).
                    const size_t firstColumnOffset =
                        vOffset - static_cast<size_t>( s ) * floatsPerVertex;
                    nx = vertexData[firstColumnOffset + 3];
                    ny = vertexData[firstColumnOffset + 4];
                    nz = vertexData[firstColumnOffset + 5];
                }
                else
                {
                    const float theta = u * Ogre::Math::TWO_PI;
                    const float sinTheta = Ogre::Math::Sin( theta );
                    const float cosTheta = Ogre::Math::Cos( theta );

                    nx = sinPhi * cosTheta;
                    ny = cosPhi;
                    nz = sinPhi * sinTheta;
                }

                vertexData[vOffset + 0] = nx * radius;
                vertexData[vOffset + 1] = ny * radius;
                vertexData[vOffset + 2] = nz * radius;
                vertexData[vOffset + 3] = nx;
                vertexData[vOffset + 4] = ny;
                vertexData[vOffset + 5] = nz;
                vertexData[vOffset + 6] = u;
                vertexData[vOffset + 7] = v;

                vOffset += floatsPerVertex;
            }
        }

        Ogre::uint32 *indexData = reinterpret_cast<Ogre::uint32 *>(
            OGRE_MALLOC_SIMD( numIndices * sizeof( Ogre::uint32 ), Ogre::MEMCATEGORY_GEOMETRY ) );
        size_t iOffset = 0u;
        for( unsigned r = 0u; r < numRings; ++r )
        {
            for( unsigned s = 0u; s < numSegments; ++s )
            {
                const Ogre::uint32 a = r * rowStride + s;
                const Ogre::uint32 b = a + rowStride;
                const Ogre::uint32 c = a + 1u;
                const Ogre::uint32 d = b + 1u;

                indexData[iOffset + 0] = a;
                indexData[iOffset + 1] = b;
                indexData[iOffset + 2] = c;

                indexData[iOffset + 3] = c;
                indexData[iOffset + 4] = b;
                indexData[iOffset + 5] = d;

                iOffset += 6u;
            }
        }

        Ogre::VertexElement2Vec vertexElements;
        vertexElements.push_back( Ogre::VertexElement2( Ogre::VET_FLOAT3, Ogre::VES_POSITION ) );
        vertexElements.push_back( Ogre::VertexElement2( Ogre::VET_FLOAT3, Ogre::VES_NORMAL ) );
        vertexElements.push_back(
            Ogre::VertexElement2( Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES ) );

        Ogre::VertexBufferPacked *vertexBuffer = vaoManager->createVertexBuffer(
            vertexElements, numVertices, Ogre::BT_IMMUTABLE, vertexData, true );

        Ogre::VertexBufferPackedVec vertexBuffers;
        vertexBuffers.push_back( vertexBuffer );

        Ogre::IndexBufferPacked *indexBuffer = vaoManager->createIndexBuffer(
            Ogre::IndexBufferPacked::IT_32BIT, numIndices, Ogre::BT_IMMUTABLE, indexData, true );

        Ogre::VertexArrayObject *vao =
            vaoManager->createVertexArrayObject( vertexBuffers, indexBuffer, Ogre::OT_TRIANGLE_LIST );

        Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().createManual(
            meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, &gProceduralMeshLoader );

        Ogre::SubMesh *subMesh = mesh->createSubMesh();
        // Regular rendering and shadow casting share the same Vao (the common case,
        // and the only one LodOutputProviderMeshV2 fully supports today -- see its
        // bakeLodLevel() comment about independent shadow Vaos).
        subMesh->mVao[Ogre::VpNormal].push_back( vao );
        subMesh->mVao[Ogre::VpShadow].push_back( vao );

        const Ogre::Aabb aabb( Ogre::Vector3::ZERO, Ogre::Vector3( radius, radius, radius ) );
        mesh->_setBounds( aabb, false );
        mesh->_setBoundingSphereRadius( radius );

        // Geometry is already filled in above; load() just transitions the Resource
        // state machine to LOADSTATE_LOADED so Item::_initialise() will accept it
        // (it calls mMesh->load() then checks isLoaded()).
        mesh->load();

        return mesh;
    }
    //-----------------------------------------------------------------------------------
    void MeshLodV2GameState::generateLodLevelsV2( const Ogre::MeshPtr &meshV2,
                                                  const Ogre::String &logLabel )
    {
        Ogre::MeshPtr meshCopy = meshV2;  // getAutoconfig takes a non-const MeshPtr&

        Ogre::LodConfig lodConfig;

        Ogre::MeshLodGenerator lodGenerator;
        lodGenerator.getAutoconfig( meshCopy, lodConfig );

        lodConfig.strategy = Ogre::LodStrategyManager::getSingleton().getDefaultStrategy();

        // getAutoconfig()'s reductionValue/reductionMethod already scale correctly
        // with each mesh's own bounding radius, so those are kept as-is. Its
        // distance heuristic does NOT scale with mesh size at all though -- it's a
        // fixed number, so we replace it with explicit screen-coverage-ratio
        // thresholds instead. Since ScreenRatioPixelCountLodStrategy is specifically
        // designed to be scale-independent (that's the entire point of "ratio" over
        // "absolute distance" or "absolute pixel count"), the SAME four thresholds
        // work sensibly for both the sphere and Sinbad here despite their very
        // different actual sizes -- which is itself a nice demonstration that this
        // approach was the right fix, not just a per-mesh tuning hack.
        OgreAssert( lodConfig.levels.size() == 4u, "Expected exactly 4 levels from getAutoconfig" );
        lodConfig.levels[0].distance = 0.35f;
        lodConfig.levels[1].distance = 0.15f;
        lodConfig.levels[2].distance = 0.05f;
        lodConfig.levels[3].distance = 0.015f;

        // This is the entire point of the sample: no v1 mesh, no v1 import, no
        // round-trip is needed from THIS point on. LOD levels are generated directly
        // against meshV2 and land directly on its SubMesh::mVao[VpNormal]/[VpShadow]
        // arrays.
        lodGenerator.generateLodLevels( lodConfig );

        Ogre::LogManager::getSingleton().logMessage(
            "[MeshLodV2] Generated " + Ogre::StringConverter::toString( lodConfig.levels.size() ) +
            " LOD levels directly against v2 mesh '" + meshV2->getName() + "' (" + logLabel + ")." );
    }
    //-----------------------------------------------------------------------------------
    Ogre::MeshPtr MeshLodV2GameState::loadAndConvertSinbadToV2()
    {
        // Sinbad.mesh ships in legacy v1 binary format -- loading it as v1 here is
        // unavoidable, that's the only format the asset exists in on disk.
        Ogre::v1::MeshPtr meshV1 = Ogre::v1::MeshManager::getSingleton().load(
            "Sinbad.mesh", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC );

        // Import v1 -> v2 with ZERO LOD levels baked in -- this is the key
        // difference from the original MeshLod sample, which runs
        // MeshLodGenerator against meshV1 BEFORE this import step (so the import
        // carries pre-baked LOD across via SubMesh::importBuffersFromV1's
        // mLodFaceList loop). Here, generateLodLevelsV2() runs entirely afterwards,
        // directly against the v2 result, through the exact same v2-native
        // LodInputProviderMeshV2/LodOutputProviderMeshV2 path the procedural sphere
        // uses -- proving the new generator works on a real, skinned,
        // multi-submesh mesh, not just a clean procedural one.
        Ogre::MeshPtr meshV2 = Ogre::MeshManager::getSingleton().createByImportingV1(
            "Sinbad_v2_native_lod.mesh", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            meshV1.get(), true, true, true );
        meshV2->load();

        // The v1 mesh has done its only job (letting us produce a v2 Mesh at all,
        // since that's the format the asset ships in) and is no longer needed --
        // everything from here on operates purely on meshV2.
        meshV1->unload();
        Ogre::v1::MeshManager::getSingleton().remove( meshV1 );

        return meshV2;
    }
    //-----------------------------------------------------------------------------------
    void MeshLodV2GameState::createScene01()
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

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

        Ogre::LodStrategyManager::getSingleton().setDefaultStrategy(
            Ogre::ScreenRatioPixelCountLodStrategy::getSingletonPtr() );

        // Build the sphere directly as a v2 mesh. High subdivision (96x48 = ~9200
        // triangles) so the LOD reduction is visually unmistakable, especially with
        // the F2 wireframe toggle. Deliberately a LOCAL variable (see the comment on
        // createProceduralSphereMeshV2()'s declaration) -- once createItem() below
        // has its own reference, this local going out of scope at the end of this
        // function is correct and expected.
        Ogre::MeshPtr sphereMeshV2 =
            createProceduralSphereMeshV2( "ProceduralLodSphereV2", 2.0f, 48u, 96u );
        generateLodLevelsV2( sphereMeshV2, "procedurally-built sphere, never touched v1" );

        Ogre::MeshPtr sinbadMeshV2 = loadAndConvertSinbadToV2();
        generateLodLevelsV2( sinbadMeshV2,
                             "Sinbad, v1-imported with no LOD then LOD-generated natively on v2" );

        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );

        mSphereDatablock = static_cast<Ogre::HlmsPbsDatablock *>(
            hlmsPbs->createDatablock( "LodSphereV2", "LodSphereV2", Ogre::HlmsMacroblock(),
                                      Ogre::HlmsBlendblock(), Ogre::HlmsParamVec() ) );
        mSphereDatablock->setDiffuse( Ogre::Vector3( 0.6f, 0.6f, 0.65f ) );

        mSphereItem = sceneManager->createItem( sphereMeshV2, Ogre::SCENE_DYNAMIC );
        mSphereItem->setDatablock( mSphereDatablock );

        Ogre::SceneNode *sphereNode = rootNode->createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sphereNode->setPosition( Ogre::Vector3( -3.0f, 2.0f, 0.0f ) );
        sphereNode->attachObject( mSphereItem );

        mSinbadDatablock = static_cast<Ogre::HlmsPbsDatablock *>(
            hlmsPbs->createDatablock( "LodSinbadV2", "LodSinbadV2", Ogre::HlmsMacroblock(),
                                      Ogre::HlmsBlendblock(), Ogre::HlmsParamVec() ) );
        mSinbadDatablock->setDiffuse( Ogre::Vector3( 0.65f, 0.55f, 0.45f ) );

        mSinbadItem = sceneManager->createItem( sinbadMeshV2, Ogre::SCENE_DYNAMIC );
        for( size_t i = 0; i < mSinbadItem->getNumSubItems(); ++i )
            mSinbadItem->getSubItem( i )->setDatablock( mSinbadDatablock );

        Ogre::SceneNode *sinbadNode = rootNode->createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sinbadNode->setPosition( Ogre::Vector3( 3.0f, 0.0f, 0.0f ) );
        sinbadNode->setScale( Ogre::Vector3( 1.0f ) );
        sinbadNode->attachObject( mSinbadItem );

        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
    void MeshLodV2GameState::update( float timeSinceLast )
    {
        if( mSphereItem )
        {
            const Ogre::uint8 currentLod = mSphereItem->getCurrentMeshLod();
            if( currentLod != mLastLoggedLodSphere )
            {
                mLastLoggedLodSphere = currentLod;
                Ogre::LogManager::getSingleton().logMessage(
                    "[MeshLodV2] Sphere switched to LOD level " +
                    Ogre::StringConverter::toString( static_cast<unsigned>( currentLod ) ) );
            }
        }

        if( mSinbadItem )
        {
            const Ogre::uint8 currentLod = mSinbadItem->getCurrentMeshLod();
            if( currentLod != mLastLoggedLodSinbad )
            {
                mLastLoggedLodSinbad = currentLod;
                Ogre::LogManager::getSingleton().logMessage(
                    "[MeshLodV2] Sinbad switched to LOD level " +
                    Ogre::StringConverter::toString( static_cast<unsigned>( currentLod ) ) );
            }
        }

        TutorialGameState::update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    void MeshLodV2GameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( arg.keysym.sym == SDLK_F2 )
        {
            mWireframeOn = !mWireframeOn;

            const Ogre::PolygonMode mode = mWireframeOn ? Ogre::PM_WIREFRAME : Ogre::PM_SOLID;

            Ogre::HlmsMacroblock sphereMacroblock( *mSphereDatablock->getMacroblock() );
            sphereMacroblock.mPolygonMode = mode;
            mSphereDatablock->setMacroblock( sphereMacroblock );

            Ogre::HlmsMacroblock sinbadMacroblock( *mSinbadDatablock->getMacroblock() );
            sinbadMacroblock.mPolygonMode = mode;
            mSinbadDatablock->setMacroblock( sinbadMacroblock );
            return;
        }

        TutorialGameState::keyReleased( arg );
    }
    //-----------------------------------------------------------------------------------
}  // namespace Demo