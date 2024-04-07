
#ifndef Demo_Hlms02MyHlmsPbs_H
#define Demo_Hlms02MyHlmsPbs_H

#include "OgreHlmsPbs.h"

namespace Ogre
{
    class MyHlmsPbs final : public HlmsPbs
    {
    public:
        /// This value is completely arbitrary. It just doesn't have to clash with anything else.
        /// We use it to tag which SubItems/SubEntities should have wind animation.
        static constexpr uint32 kWindId = 374117u;

        MyHlmsPbs( Archive *dataFolder, ArchiveVec *libraryFolders ) :
            HlmsPbs( dataFolder, libraryFolders )
        {
        }

        void calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces ) override
        {
            HlmsPbs::calculateHashForPreCreate( renderable, inOutPieces );

            // Check if the object has been tagged to have wind. If it does, then set
            // the respective property so the shader gets generated with wind animation.
            if( renderable->hasCustomParameter( kWindId ) )
                setProperty( kNoTid, "use_wind_animation", 1 );
        }

        void calculateHashForPreCaster( Renderable *renderable, PiecesMap *inOutPieces,
                                        const PiecesMap *nonCasterPieces ) override
        {
            HlmsPbs::calculateHashForPreCaster( renderable, inOutPieces, nonCasterPieces );

            // Do the same for the caster pass.
            if( renderable->hasCustomParameter( kWindId ) )
                setProperty( kNoTid, "use_wind_animation", 1 );
        }
    };
}  // namespace Ogre

#endif
