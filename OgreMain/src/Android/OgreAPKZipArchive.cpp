#include "Android/OgreAPKZipArchive.h"

#include <OgreLogManager.h>
#include <OgreStringConverter.h>

namespace Ogre
{
    //-----------------------------------------------------------------------
    const String &APKZipArchiveFactory::getType() const
    {
        static String type = "APKZip";
        return type;
    }
    //-----------------------------------------------------------------------
}  // namespace Ogre
