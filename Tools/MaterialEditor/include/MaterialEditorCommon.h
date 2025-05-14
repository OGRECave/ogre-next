
#pragma once

#include "OgrePrerequisites.h"

OGRE_ASSUME_NONNULL_BEGIN

class wxSlider;
class wxTextCtrl;

struct EditingScope
{
    bool &editing;

    EditingScope( bool &bValue ) : editing( bValue ) { editing = true; }
    ~EditingScope() { editing = false; }
};

struct SliderTextWidget
{
    // If one is nullptr, then the other one also is.
    wxSlider *ogre_nullable   slider;
    wxTextCtrl *ogre_nullable text;

    void fromSlider();
    void fromText();
};

namespace CoordinateConvention
{
    enum CoordinateConvention
    {
        xUp,
        yUp,
        zUp,
        NumCoordinateConventions
    };
}

extern const Ogre::Quaternion kCoordConventions[CoordinateConvention::NumCoordinateConventions];

OGRE_ASSUME_NONNULL_END
