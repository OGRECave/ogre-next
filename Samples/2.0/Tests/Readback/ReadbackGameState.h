
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
    class ReadbackGameState : public TutorialGameState, public Ogre::UniformScalableTask
    {
        Ogre::HlmsUnlitDatablock *mUnlitDatablock;

        Ogre::uint32 mRgbaReference;
        Ogre::TextureBox const *mTextureBox;
        bool mRaceConditionDetected;

        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

    public:
        ReadbackGameState( const Ogre::String &helpDescription );

        virtual void createScene01( void );
        virtual void update( float timeSinceLast );

        virtual void execute( size_t threadId, size_t numThreads );
    };
}  // namespace Demo

#endif
