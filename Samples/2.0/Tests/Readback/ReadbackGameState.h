
#ifndef _Demo_ReadbackGameState_H_
#define _Demo_ReadbackGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

#include "Threading/OgreUniformScalableTask.h"

namespace Ogre
{
    class HlmsUnlitDatablock;
}

namespace Demo
{
    class ReadbackGameState final : public TutorialGameState, public Ogre::UniformScalableTask
    {
        Ogre::HlmsUnlitDatablock *mUnlitDatablock;

        Ogre::uint32 mRgbaReference;
        Ogre::uint8 mRgbaResult[4];
        Ogre::TextureBox const *mTextureBox;
        bool mRaceConditionDetected;

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

    public:
        ReadbackGameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void update( float timeSinceLast ) override;

        void execute( size_t threadId, size_t numThreads ) override;
    };
}  // namespace Demo

#endif
