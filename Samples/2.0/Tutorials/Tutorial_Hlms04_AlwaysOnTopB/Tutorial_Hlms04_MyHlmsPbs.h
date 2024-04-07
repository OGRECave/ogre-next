
#ifndef Demo_Hlms04MyHlmsPbs_H
#define Demo_Hlms04MyHlmsPbs_H

#include "OgreHlmsPbs.h"

namespace Ogre
{
    class MyHlmsPbs final : public HlmsPbs
    {
    public:
        /// This value is completely arbitrary. It just doesn't have to clash with anything else.
        /// We use it to tag which SubItems/SubEntities should have be always on top.
        static constexpr uint32 kClearDepthId = 2272546u;

        MyHlmsPbs( Archive *dataFolder, ArchiveVec *libraryFolders ) :
            HlmsPbs( dataFolder, libraryFolders )
        {
        }

        void calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces ) override
        {
            HlmsPbs::calculateHashForPreCreate( renderable, inOutPieces );

            // Check if the object has been tagged to always clear. If it does, then set
            // the respective property so the shader gets generated with extra code.
            // We don't want this on the shadow caster pass, so we don't overload
            // calculateHashForPreCaster().
            if( renderable->hasCustomParameter( kClearDepthId ) )
                setProperty( kNoTid, "clear_depth", 1 );
        }
    };
}  // namespace Ogre

#endif
