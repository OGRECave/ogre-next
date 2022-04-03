
#ifndef _Demo_PostprocessingGameState_H_
#define _Demo_PostprocessingGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

#include "OgreIdString.h"
#include "OgreStringVector.h"

namespace Demo
{
    class PostprocessingGameState : public TutorialGameState
    {
        Ogre::StringVector mCompositorNames;
        size_t mCurrentPage;

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

        void importV1Mesh( const Ogre::String &meshName );

        /// Creates textures needed by some of the postprocessing effects.
        void createCustomTextures();

        /// Creates hard coded postfilter effects from code instead of scripts.
        /// Just to show how to do it. Needs to be called before or inside
        /// setupCompositor; since setupCompositor modifies the workspace
        /// definition to add all the postprocessing nodes beforehand, but
        /// disabled.
        void createExtraEffectsFromCode();

        /// Shows two of the many possible ways to toggle a postprocess FX
        /// on and off in real time.
        void togglePostprocess( Ogre::IdString nodeName );

    public:
        PostprocessingGameState( const Ogre::String &helpDescription );

        Ogre::CompositorWorkspace *setupCompositor();

        void createScene01() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
