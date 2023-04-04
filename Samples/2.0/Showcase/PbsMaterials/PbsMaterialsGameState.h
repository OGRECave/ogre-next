
#ifndef _Demo_PbsMaterialsGameState_H_
#define _Demo_PbsMaterialsGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class PbsMaterialsGameState : public TutorialGameState
    {
        enum BrdfDiffuseFresnelMode
        {
            NoDiffuseFresnel,
            WithDiffuseFresnel,
            SeparateDiffuseFresnel,
            NumBrdfDiffuseFresnelModes
        };

        Ogre::SceneNode *mSceneNode[16];

        Ogre::SceneNode *mLightNodes[3];

        bool mAnimateObjects;

        size_t mNumSpheres;
        float mTransparencyValue;
        Ogre::uint8 mTransparencyMode;

        uint8_t mBrdfIdx;
        BrdfDiffuseFresnelMode mBrdfDiffuseFresnelMode;

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

        void setTransparencyToMaterials();

        bool currentBrdfSupportsDiffuseFresnel() const;

        uint32_t getBrdf() const;

        void setBrdfToMaterials();

    public:
        PbsMaterialsGameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
