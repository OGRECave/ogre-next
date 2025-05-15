
#pragma once

#include "OgrePrerequisites.h"

#include <wx/slider.h>
#include <wx/textctrl.h>

OGRE_ASSUME_NONNULL_BEGIN

class wxSlider;
class wxTextCtrl;

class LightPanel;

struct EditingScope
{
    bool &editing;

    EditingScope( bool &bValue ) : editing( bValue ) { editing = true; }
    ~EditingScope() { editing = false; }
};

template <int32_t MULTIPLIER>
struct SliderTextWidgetTemplate
{
    // If one is nullptr, then the other one also is.
    wxSlider *ogre_nullable   slider;
    wxTextCtrl *ogre_nullable text;

    void fromSlider()
    {
        const double val = double( slider->GetValue() );
        text->SetValue( wxString::Format( wxT( "%.05lf" ), val / ( double( MULTIPLIER ) / 100.0 ) ) );
    }
    void fromText()
    {
        double val = 0;
        if( text->GetValue().ToDouble( &val ) )
            slider->SetValue( int( std::round( val * ( double( MULTIPLIER ) / 100.0 ) ) ) );
    }
};

typedef SliderTextWidgetTemplate<10000> SliderTextWidget;
typedef SliderTextWidgetTemplate<100>   SliderTextWidgetAngle;

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
