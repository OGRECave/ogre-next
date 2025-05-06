#include "PbsParametersPanel.h"

#include "OgreColourValue.h"
#include "OgrePixelFormatGpuUtils.h"

#include <wx/colordlg.h>

void PbsParametersPanel::ColourWidgets::fromRgbaText()
{
    Ogre::ColourValue c;
    Ogre::ColourValue srgb;

    for( size_t i = 0u; i < 4u; ++i )
    {
        wxString value = rgbaText[i]->GetValue();
        double fValue = 0.0f;
        value.ToDouble( &fValue );
        c.ptr()[i] = float( fValue );
        srgb.ptr()[i] = i == 3u ? srgb.ptr()[i] : Ogre::PixelFormatGpuUtils::toSRGB( c.ptr()[i] );
    }

    wxColour wxCol( srgb.getAsABGR() );

    rgbaHtml->SetValue( wxString::Format( wxT( "%08x" ), srgb.getAsRGBA() ) );
    buttonPicker->SetBackgroundColour( wxCol );
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::ColourWidgets::fromButton( const uint32_t rgba )
{
    buttonPicker->SetBackgroundColour( wxColour( rgba ) );
    Ogre::ColourValue srgb;
    srgb.setAsABGR( rgba );

    rgbaHtml->SetValue( wxString::Format( wxT( "%08x" ), srgb.getAsRGBA() ) );

    for( size_t i = 0u; i < 4u; ++i )
    {
        const float linearC =
            i == 3u ? srgb.ptr()[i] : Ogre::PixelFormatGpuUtils::fromSRGB( srgb.ptr()[i] );
        rgbaText[i]->SetValue( wxString::Format( wxT( "%.05lf" ), linearC ) );
    }
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::ColourWidgets::fromHtml()
{
    wxString value = rgbaHtml->GetValue();
    unsigned long ulongVal = 0xffffffff;
    value.ToULong( &ulongVal, 16 );

    if( value.size() <= 6u )
    {
        ulongVal <<= 8u;  // Value is RGB. Alpha is not being specified. Make it 1.0
        ulongVal |= 0xFF;
    }

    Ogre::ColourValue srgb;
    srgb.setAsRGBA( uint32_t( ulongVal ) );

    buttonPicker->SetBackgroundColour( wxColour( srgb.getAsABGR() ) );

    for( size_t i = 0u; i < 4u; ++i )
    {
        const float linearC =
            i == 3u ? srgb.ptr()[i] : Ogre::PixelFormatGpuUtils::fromSRGB( srgb.ptr()[i] );
        rgbaText[i]->SetValue( wxString::Format( wxT( "%.05lf" ), linearC ) );
    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
PbsParametersPanel::PbsParametersPanel( wxWindow *parent ) :
    PbsParametersPanelBase( parent ),
    m_editing( false )
{
}
//-----------------------------------------------------------------------------
PbsParametersPanel::ColourWidgets PbsParametersPanel::getColourWidgets(
    ColourSection::ColourSection section )
{
    switch( section )
    {
    case ColourSection::NumColourSection:
    case ColourSection::Diffuse:
        return { { m_diffuseR, m_diffuseG, m_diffuseB, m_diffuseA }, m_diffuseRGBA, m_buttonDiffuse };
    case ColourSection::Specular:
        return { { m_specularR, m_specularG, m_specularB, m_specularA },
                 m_specularRGBA,
                 m_buttonSpecular };
    case ColourSection::Fresnel:
        return { { m_fresnelR, m_fresnelG, m_fresnelB, m_fresnelA }, m_fresnelRGBA, m_buttonFresnel };
    }
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::OnColourHtml( wxCommandEvent &event )
{
    if( m_editing )
        return;

    EditingScope scope( m_editing );

    wxTextCtrl *textCtrl = static_cast<wxTextCtrl *>( event.GetEventObject() );

    for( size_t i = 0u; i < ColourSection::NumColourSection; ++i )
    {
        ColourWidgets widgets = getColourWidgets( ColourSection::ColourSection( i ) );
        if( widgets.rgbaHtml == textCtrl )
        {
            widgets.fromHtml();
            event.Skip( false );
            return;
        }
    }

    event.Skip();
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::OnColourButton( wxCommandEvent &event )
{
    if( m_editing )
        return;

    EditingScope scope( m_editing );

    wxColourDialog pickerDlg( this );

    if( pickerDlg.ShowModal() != wxID_OK )
    {
        event.Skip();
        return;
    }

    wxButton *pickerButton = static_cast<wxButton *>( event.GetEventObject() );

    for( size_t i = 0u; i < ColourSection::NumColourSection; ++i )
    {
        ColourWidgets widgets = getColourWidgets( ColourSection::ColourSection( i ) );
        if( widgets.buttonPicker == pickerButton )
        {
            const uint32_t rgba = pickerDlg.GetColourData().GetColour().GetRGBA();
            widgets.fromButton( rgba );
            event.Skip( false );
            return;
        }
    }

    event.Skip();
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::OnColourText( wxCommandEvent &event )
{
    if( m_editing )
        return;

    EditingScope scope( m_editing );

    wxTextCtrl *channelText = static_cast<wxTextCtrl *>( event.GetEventObject() );

    for( size_t i = 0u; i < ColourSection::NumColourSection; ++i )
    {
        ColourWidgets widgets = getColourWidgets( ColourSection::ColourSection( i ) );
        for( size_t j = 0u; j < 4u; ++j )
        {
            if( widgets.rgbaText[j] == channelText )
            {
                widgets.fromRgbaText();
                event.Skip( false );
                return;
            }
        }
    }

    event.Skip();
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::OnCheckBox( wxCommandEvent &event )
{
    // TODO: Implement OnCheckBox
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::OnSlider( wxCommandEvent &event )
{
    // TODO: Implement OnSlider
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::OnSliderText( wxCommandEvent &event )
{
    // TODO: Implement OnSliderText
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::OnTransparencyMode( wxCommandEvent &event )
{
    // TODO: Implement OnTransparencyMode
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::OnCheckbox( wxCommandEvent &event )
{
    // TODO: Implement OnCheckbox
}
