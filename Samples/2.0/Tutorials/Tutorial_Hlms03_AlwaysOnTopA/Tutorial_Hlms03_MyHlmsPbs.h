
#ifndef Demo_Hlms03MyHlmsPbs_H
#define Demo_Hlms03MyHlmsPbs_H

#include "OgreHlmsPbs.h"

namespace Ogre
{
    class MyHlmsPbs final : public HlmsPbs
    {
    public:
        /// This value is completely arbitrary. It just doesn't have to clash with anything else.
        /// We use it to tag which SubItems/SubEntities should have be always on top.
        static constexpr uint32 kAlwaysOnTopId = 6841455u;

        MyHlmsPbs( Archive *dataFolder, ArchiveVec *libraryFolders ) :
            HlmsPbs( dataFolder, libraryFolders )
        {
        }

        void calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces ) override
        {
            HlmsPbs::calculateHashForPreCreate( renderable, inOutPieces );

            // Check if the object has been tagged to be always on top. If it does, then set
            // the respective property so the shader gets generated with extra code.
            // We don't want this on the shadow caster pass, so we don't overload
            // calculateHashForPreCaster().
            if( renderable->hasCustomParameter( kAlwaysOnTopId ) )
                setProperty( kNoTid, "always_on_top", 1 );
        }
    };
}  // namespace Ogre

#endif
