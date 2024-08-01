
#ifndef _Demo_ManyMaterialsGameState_H_
#define _Demo_ManyMaterialsGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

#include "Threading/OgreUniformScalableTask.h"

namespace Ogre
{
    class HlmsUnlitDatablock;
}

namespace Demo
{
    class ManyMaterialsGameState final : public TutorialGameState, public Ogre::UniformScalableTask
    {
        size_t mCurrentMaterialId;

        Ogre::Item *mPlaneItem;

        Ogre::uint32 mRgbaReference;
        Ogre::uint8 mRgbaResult[4];
        Ogre::TextureBox const *mTextureBox;
        bool mErrorDetected;

        Ogre::HlmsUnlitDatablock *createUnlitDatablock( const Ogre::ColourValue &colour );

        Ogre::HlmsUnlitDatablock *generateMaterial( size_t numDummyMaterials,
                                                    const Ogre::ColourValue &colour );

    public:
        ManyMaterialsGameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void update( float timeSinceLast ) override;

        void execute( size_t threadId, size_t numThreads ) override;
    };
}  // namespace Demo

#endif
