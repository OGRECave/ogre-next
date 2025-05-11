
#include "MaterialEditorCommon.h"

#include <wx/slider.h>
#include <wx/textctrl.h>

//-----------------------------------------------------------------------------
void SliderTextWidget::fromSlider()
{
    const double val = double( slider->GetValue() );
    text->SetValue( wxString::Format( wxT( "%.05lf" ), val / 100.0 ) );
}
//-----------------------------------------------------------------------------
void SliderTextWidget::fromText()
{
    double val = 0;
    if( text->GetValue().ToDouble( &val ) )
        slider->SetValue( int( std::round( val * 100.0 ) ) );
}
