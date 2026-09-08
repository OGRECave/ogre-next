#ifndef _Demo_MeshLodV2GameState_H_
#define _Demo_MeshLodV2GameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"
#include "OgreHlmsPbsDatablock.h"

namespace Demo
{
    class MeshLodV2GameState : public TutorialGameState
    {
        Ogre::SceneNode *mLightNodes[3];

        Ogre::Item *mSphereItem;

        /// Sinbad, imported v1 -> v2 with NO LOD baked in during that import (unlike
        /// the original MeshLod sample), then LOD-generated directly against the
        /// resulting v2 mesh via the same v2-native code path the sphere uses. A
        /// real, skinned, multi-submesh mesh as a comparison/stress case alongside
        /// the procedural sphere.
        Ogre::Item *mSinbadItem;

        Ogre::HlmsPbsDatablock *mSphereDatablock;
        Ogre::HlmsPbsDatablock *mSinbadDatablock;
        bool mWireframeOn;

        /// Last LOD level logged per-Item, so update() only logs on an actual change
        /// instead of every frame.
        Ogre::uint8 mLastLoggedLodSphere;
        Ogre::uint8 mLastLoggedLodSinbad;

        /// Builds a UV sphere directly as a v2 Mesh: creates the Mesh via
        /// MeshManager::createManual(), fills a SubMesh's mVao[VpNormal]/[VpShadow]
        /// directly via VaoManager::createVertexBuffer()/createIndexBuffer()/
        /// createVertexArrayObject(), and sets bounds -- no v1 mesh involved.
        /// Returns the MeshPtr as a local value deliberately -- see createScene01():
        /// only the Item created from it should keep it alive long-term, not a
        /// GameState member (a GameState-owned MeshPtr outlives GraphicsSystem's own
        /// teardown order and crashes on exit reading an already-destroyed
        /// VaoManager).
        Ogre::MeshPtr createProceduralSphereMeshV2( const Ogre::String &meshName, float radius,
                                                    unsigned numRings, unsigned numSegments );

        /// Loads Sinbad.mesh as v1 (unavoidable -- that's the format the shipped
        /// asset is in), imports it straight to v2 with createByImportingV1() while
        /// it still has zero LOD levels, then discards the v1 mesh entirely. LOD
        /// generation happens afterwards, directly against the v2 result, via
        /// generateLodLevelsV2() below -- unlike the original MeshLod sample, which
        /// generates LOD on the v1 mesh first and imports the result.
        Ogre::MeshPtr loadAndConvertSinbadToV2();

        /// Builds the LOD levels directly against a v2 mesh via
        /// MeshLodGenerator::getAutoconfig()/generateLodLevels() -- the actual point
        /// of this sample. Shared between the sphere and Sinbad since both use the
        /// same explicit screen-coverage-ratio thresholds (see the comment at the
        /// call site for why those thresholds are mesh-size-independent).
        void generateLodLevelsV2( const Ogre::MeshPtr &meshV2, const Ogre::String &logLabel );

    public:
        MeshLodV2GameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void update( float timeSinceLast ) override;
        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif