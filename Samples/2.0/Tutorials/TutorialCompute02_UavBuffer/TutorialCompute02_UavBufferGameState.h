
#ifndef _Demo_TutorialCompute02_UavBufferGameState_H_
#define _Demo_TutorialCompute02_UavBufferGameState_H_

#include "OgreMaterial.h"
#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class TutorialCompute02_UavBufferGameState : public TutorialGameState
    {
        Ogre::SceneNode *mSceneNode;
        float mDisplacement;

        Ogre::MaterialPtr mDrawFromUavBufferMat;
        Ogre::HlmsComputeJob *mComputeJob;

        Ogre::uint32 mLastWindowWidth;
        Ogre::uint32 mLastWindowHeight;

    public:
        TutorialCompute02_UavBufferGameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void destroyScene() override;

        void update( float timeSinceLast ) override;
    };
}  // namespace Demo

#endif
