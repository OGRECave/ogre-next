#include "PbsParametersPanel.h"

#include "MainWindow.h"

#include "OgreColourValue.h"
#include "OgreHlmsPbsDatablock.h"
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
void PbsParametersPanel::SliderTextWidget::fromSlider()
{
    const double val = double( slider->GetValue() );
    text->SetValue( wxString::Format( wxT( "%.05lf" ), val / 100.0 ) );
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::SliderTextWidget::fromText()
{
    double val = 0;
    if( text->GetValue().ToDouble( &val ) )
        slider->SetValue( int( std::round( val * 100.0 ) ) );
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
PbsParametersPanel::PbsParametersPanel( MainWindow *parent ) :
    PbsParametersPanelBase( parent ),
    m_mainWindow( parent ),
    m_editing( false )
{
    getColourWidgets( ColourSection::Specular ).rgbaText[3]->Disable();
    getColourWidgets( ColourSection::Fresnel ).rgbaText[3]->Disable();
    refreshFromDatablock();
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
PbsParametersPanel::SliderTextWidget PbsParametersPanel::getSliderWidgets(
    PbsSliders::PbsSliders slider )
{
    switch( slider )
    {
    case PbsSliders::Fresnel:
    case PbsSliders::NumPbsSliders:
        return { m_fresnelSlider, m_fresnelR };
    case PbsSliders::Roughness:
        return { m_roughnessSlider, m_roughness };
    case PbsSliders::Transparency:
        return { m_transparencySlider, m_transparency };
    }
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::syncFresnelCheckbox()
{
    const bool bIsChecked = m_fresnelColouredCheckbox->IsChecked();
    m_fresnelG->Enable( bIsChecked );
    m_fresnelB->Enable( bIsChecked );
    m_fresnelRGBA->Enable( bIsChecked );
    m_buttonFresnel->Enable( bIsChecked );
    m_fresnelSlider->Enable( !bIsChecked );
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
void PbsParametersPanel::OnCheckbox( wxCommandEvent &event )
{
    if( m_editing )
        return;

    EditingScope scope( m_editing );

    const wxObject *obj = event.GetEventObject();
    if( obj == m_fresnelColouredCheckbox )
        syncFresnelCheckbox();
    event.Skip();
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::OnSlider( wxCommandEvent &event )
{
    if( m_editing )
        return;

    EditingScope scope( m_editing );

    const wxObject *obj = event.GetEventObject();

    for( size_t i = 0u; i < PbsSliders::NumPbsSliders; ++i )
    {
        SliderTextWidget widgets = getSliderWidgets( PbsSliders::PbsSliders( i ) );
        if( widgets.slider == obj )
        {
            widgets.fromSlider();

            if( i == PbsSliders::Fresnel )
                getColourWidgets( ColourSection::Fresnel ).fromRgbaText();

            event.Skip( false );
            return;
        }
    }

    event.Skip();
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::OnSliderText( wxCommandEvent &event )
{
    if( m_editing )
        return;

    EditingScope scope( m_editing );

    const wxObject *obj = event.GetEventObject();

    for( size_t i = 0u; i < PbsSliders::NumPbsSliders; ++i )
    {
        SliderTextWidget widgets = getSliderWidgets( PbsSliders::PbsSliders( i ) );
        if( widgets.text == obj )
        {
            widgets.fromText();

            if( i == PbsSliders::Fresnel )
                getColourWidgets( ColourSection::Fresnel ).fromRgbaText();

            event.Skip( false );
            return;
        }
    }

    event.Skip();
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::OnTransparencyMode( wxCommandEvent &event )
{
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::syncDatablockFromUI()
{
    Ogre::HlmsDatablock *datablock = m_mainWindow->getActiveDatablock();
    if( !datablock )
        return;

    OGRE_ASSERT_HIGH( dynamic_cast<Ogre::HlmsPbsDatablock *>( datablock ) );
    Ogre::HlmsPbsDatablock *pbsDatablock = static_cast<Ogre::HlmsPbsDatablock *>( datablock );

    ColourWidgets colourWidgets[ColourSection::NumColourSection] = {
        getColourWidgets( ColourSection::Diffuse ),
        getColourWidgets( ColourSection::Specular ),
        getColourWidgets( ColourSection::Fresnel ),
    };

    {
        Ogre::Vector3 col( pbsDatablock->getDiffuse() );
        for( size_t i = 0u; i < 3u; ++i )
        {
            double val;
            if( colourWidgets[ColourSection::Diffuse].rgbaText[i]->GetValue().ToDouble( &val ) )
                col[i] = Ogre::Real( val );
        }
        pbsDatablock->setDiffuse( col );
    }
    {
        Ogre::Vector3 col( pbsDatablock->getSpecular() );
        for( size_t i = 0u; i < 3u; ++i )
        {
            double val;
            if( colourWidgets[ColourSection::Specular].rgbaText[i]->GetValue().ToDouble( &val ) )
                col[i] = Ogre::Real( val );
        }
        pbsDatablock->setSpecular( col );
    }
    {
        Ogre::Vector3 col( pbsDatablock->getFresnel() );
        for( size_t i = 0u; i < 3u; ++i )
        {
            double val;
            if( colourWidgets[ColourSection::Fresnel].rgbaText[i]->GetValue().ToDouble( &val ) )
                col[i] = Ogre::Real( val );
        }
        pbsDatablock->setFresnel( col, m_fresnelColouredCheckbox->IsChecked() );
    }

    wxTextCtrl *textCtrl[PbsSliders::NumPbsSliders] = {
        getSliderWidgets( PbsSliders::Fresnel ).text,
        getSliderWidgets( PbsSliders::Roughness ).text,
        getSliderWidgets( PbsSliders::Transparency ).text,
    };

    double val;
    if( textCtrl[PbsSliders::Roughness]->GetValue().ToDouble( &val ) )
        pbsDatablock->setRoughness( float( val ) );
    if( textCtrl[PbsSliders::Transparency]->GetValue().ToDouble( &val ) )
    {
        pbsDatablock->setTransparency( float( val ), pbsDatablock->getTransparencyMode(),
                                       m_alphaFromTexCheckbox->IsChecked() );
    }

    pbsDatablock->setTransparency(
        pbsDatablock->getTransparency(),
        Ogre::HlmsPbsDatablock::TransparencyModes( m_transparencyModeChoice->GetSelection() ),
        m_alphaFromTexCheckbox->IsChecked() );
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::refreshFromDatablock()
{
    OGRE_ASSERT_LOW( !m_editing );
    EditingScope scope( m_editing );

    const Ogre::HlmsDatablock *datablock = m_mainWindow->getActiveDatablock();
    if( !datablock )
    {
        this->Disable();
        return;
    }

    this->Enable();

    OGRE_ASSERT_HIGH( dynamic_cast<const Ogre::HlmsPbsDatablock *>( datablock ) );
    const Ogre::HlmsPbsDatablock *pbsDatablock =
        static_cast<const Ogre::HlmsPbsDatablock *>( datablock );

    ColourWidgets colourWidgets[ColourSection::NumColourSection] = {
        getColourWidgets( ColourSection::Diffuse ),
        getColourWidgets( ColourSection::Specular ),
        getColourWidgets( ColourSection::Fresnel ),
    };

    {
        const Ogre::Vector3 diffuseCol = pbsDatablock->getDiffuse();
        for( size_t i = 0u; i < 3u; ++i )
        {
            colourWidgets[ColourSection::Diffuse].rgbaText[i]->SetValue(
                wxString::Format( wxT( "%.05lf" ), diffuseCol.ptr()[i] ) );
        }
        colourWidgets[ColourSection::Diffuse].fromRgbaText();
    }
    {
        const Ogre::Vector3 specularCol = pbsDatablock->getSpecular();
        for( size_t i = 0u; i < 3u; ++i )
        {
            colourWidgets[ColourSection::Specular].rgbaText[i]->SetValue(
                wxString::Format( wxT( "%.05lf" ), specularCol.ptr()[i] ) );
        }
        colourWidgets[ColourSection::Specular].fromRgbaText();
    }
    {
        const Ogre::Vector3 fresnelCol = pbsDatablock->getFresnel();
        for( size_t i = 0u; i < 3u; ++i )
        {
            colourWidgets[ColourSection::Fresnel].rgbaText[i]->SetValue(
                wxString::Format( wxT( "%.05lf" ), fresnelCol.ptr()[i] ) );
        }
        colourWidgets[ColourSection::Fresnel].fromRgbaText();
    }

    m_fresnelColouredCheckbox->SetValue( pbsDatablock->hasSeparateFresnel() );
    syncFresnelCheckbox();

    SliderTextWidget sliderWidgets[PbsSliders::NumPbsSliders] = {
        getSliderWidgets( PbsSliders::Fresnel ),
        getSliderWidgets( PbsSliders::Roughness ),
        getSliderWidgets( PbsSliders::Transparency ),
    };

    sliderWidgets[PbsSliders::Roughness].text->SetValue(
        wxString::Format( wxT( "%.05lf" ), pbsDatablock->getRoughness() ) );
    sliderWidgets[PbsSliders::Transparency].text->SetValue(
        wxString::Format( wxT( "%.05lf" ), pbsDatablock->getTransparency() ) );

    for( size_t i = 0u; i < PbsSliders::NumPbsSliders; ++i )
        sliderWidgets[i].fromText();

    m_alphaFromTexCheckbox->SetValue( pbsDatablock->getUseAlphaFromTextures() );
    m_alphaHashCheckbox->SetValue( pbsDatablock->getAlphaHashing() );
    m_transparencyModeChoice->SetSelection( pbsDatablock->getTransparencyMode() );
}
