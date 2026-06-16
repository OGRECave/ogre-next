#include "PbsParametersPanel.h"

#include "MainWindow.h"

#include "OgreColourValue.h"
#include "OgreHlms.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreMovableObject.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreRenderable.h"

#include <wx/colordlg.h>

static const Ogre::PbsBrdf::PbsBrdf kBrdfList[] = { Ogre::PbsBrdf::Default,
                                                    Ogre::PbsBrdf::CookTorrance,
                                                    Ogre::PbsBrdf::BlinnPhong,
                                                    Ogre::PbsBrdf::DefaultUncorrelated,
                                                    Ogre::PbsBrdf::DefaultHasDiffuseFresnel,
                                                    Ogre::PbsBrdf::DefaultSeparateDiffuseFresnel,
                                                    Ogre::PbsBrdf::CookTorranceHasDiffuseFresnel,
                                                    Ogre::PbsBrdf::CookTorranceSeparateDiffuseFresnel,
                                                    Ogre::PbsBrdf::CookTorrance,
                                                    Ogre::PbsBrdf::BlinnPhongHasDiffuseFresnel,
                                                    Ogre::PbsBrdf::BlinnPhongSeparateDiffuseFresnel,
                                                    Ogre::PbsBrdf::BlinnPhong,
                                                    Ogre::PbsBrdf::BlinnPhongLegacyMath,
                                                    Ogre::PbsBrdf::BlinnPhongFullLegacy };

void PbsParametersPanel::ColourWidgets::fromRgbaText()
{
    Ogre::ColourValue c;
    Ogre::ColourValue srgb;

    for( size_t i = 0u; i < 3u; ++i )
    {
        wxString value = rgbaText[i]->GetValue();
        double fValue = 0.0f;
        value.ToDouble( &fValue );
        c.ptr()[i] = float( fValue );
        srgb.ptr()[i] = Ogre::PixelFormatGpuUtils::toSRGB( c.ptr()[i] );
    }

    wxColour wxCol( srgb.getAsABGR() );

    rgbaHtml->SetValue( wxString::Format( wxT( "%06x" ), srgb.getAsRGBA() >> 8u ) );
    buttonPicker->SetBackgroundColour( wxCol );
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::ColourWidgets::fromButton( const uint32_t rgba )
{
    buttonPicker->SetBackgroundColour( wxColour( rgba ) );
    Ogre::ColourValue srgb;
    srgb.setAsABGR( rgba );

    rgbaHtml->SetValue( wxString::Format( wxT( "%06x" ), srgb.getAsRGBA() >> 8u ) );

    for( size_t i = 0u; i < 3u; ++i )
    {
        const float linearC = Ogre::PixelFormatGpuUtils::fromSRGB( srgb.ptr()[i] );
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

    for( size_t i = 0u; i < 3u; ++i )
    {
        const float linearC = Ogre::PixelFormatGpuUtils::fromSRGB( srgb.ptr()[i] );
        rgbaText[i]->SetValue( wxString::Format( wxT( "%.05lf" ), linearC ) );
    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
PbsParametersPanel::PbsParametersPanel( MainWindow *parent ) :
    PbsParametersPanelBase( parent ),
    m_mainWindow( parent ),
    m_editing( false ),
    m_datablockDirty( DirtyStateUpToDate ),
    m_ignoreUndo( false ),
    m_undoMouseUp( false )
{
    OGRE_ASSERT( m_brdfChoice->GetCount() == sizeof( kBrdfList ) / sizeof( kBrdfList[0] ) );
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
        return { { m_diffuseR, m_diffuseG, m_diffuseB }, m_diffuseRGBA, m_buttonDiffuse };
    case ColourSection::Specular:
        return { { m_specularR, m_specularG, m_specularB }, m_specularRGBA, m_buttonSpecular };
    case ColourSection::Fresnel:
        return { { m_fresnelR, m_fresnelG, m_fresnelB }, m_fresnelRGBA, m_buttonFresnel };
    }
}
//-----------------------------------------------------------------------------
SliderTextWidget PbsParametersPanel::getSliderWidgets( PbsSliders::PbsSliders slider )
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
    const bool bIsMetallic =
        m_workflowChoice->GetSelection() == Ogre::HlmsPbsDatablock::MetallicWorkflow;
    const bool bIsChecked = m_fresnelColouredCheckbox->IsChecked() && !bIsMetallic;

    m_fresnelColouredCheckbox->Enable( !bIsMetallic );
    if( bIsMetallic )
        m_fresnelSizer->GetStaticBox()->SetLabel( _( "Metalness" ) );
    else
        m_fresnelSizer->GetStaticBox()->SetLabel( _( "Fresnel" ) );

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
            m_datablockDirty = DirtyStateDirty;
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
            m_datablockDirty = DirtyStateDirty;
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
        for( size_t j = 0u; j < 3u; ++j )
        {
            if( widgets.rgbaText[j] == channelText )
            {
                widgets.fromRgbaText();
                m_datablockDirty = DirtyStateDirty;
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

    m_datablockDirty = DirtyStateDirty;
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

            m_datablockDirty = DirtyStateDirtyFromSlider;
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

            m_datablockDirty = DirtyStateDirty;
            event.Skip( false );
            return;
        }
    }

    event.Skip();
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::OnWorkflowChange( wxCommandEvent &event )
{
    if( m_editing )
        return;

    EditingScope scope( m_editing );

    syncFresnelCheckbox();
    m_datablockDirty = DirtyStateDirty;
    event.Skip();
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::OnSettingDirty( wxCommandEvent &event )
{
    m_datablockDirty = DirtyStateDirty;
    event.Skip();
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::OnSubMeshApply( wxCommandEvent &event )
{
    if( m_editing )
        return;

    EditingScope scope( m_editing );

    Ogre::MovableObject *movableObject = m_mainWindow->getActiveObject();
    Ogre::HlmsDatablock *datablock = m_mainWindow->getActiveDatablock();
    if( !movableObject || !datablock )
        return;

    const Ogre::String &resourceGroup = m_mainWindow->getActiveMeshResourceGroup();

    m_mainWindow->getUndoSystem().pushUndoStateMaterialAssign( m_mainWindow->getActiveItem(),
                                                               m_mainWindow->getActiveEntity() );

    int submeshIdx = 0;
    for( Ogre::Renderable *renderable : movableObject->mRenderables )
    {
        if( m_submeshListBox->IsSelected( submeshIdx ) )
        {
            renderable->setDatablock( datablock );
        }
        else if( renderable->getDatablock() == datablock )
        {
            // Restore the original datablock the mesh says.
            renderable->setDatablockOrMaterialName(
                m_mainWindow->getOriginalMaterialNameForActiveObject( size_t( submeshIdx ) ),
                resourceGroup );

            // We ARE the original datablock. Cannot deselect.
            if( renderable->getDatablock() == datablock )
                m_submeshListBox->SetSelection( submeshIdx, true );
        }

        ++submeshIdx;
    }
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::UndoMouseUp( wxMouseEvent &event )
{
    event.Skip();
    m_undoMouseUp = true;
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::UndoKeyUp( wxKeyEvent &event )
{
    event.Skip();
    m_ignoreUndo = false;
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::UndoKillFocus( wxFocusEvent &event )
{
    event.Skip();
    m_ignoreUndo = false;
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::syncDatablockFromUI()
{
    if( !m_datablockDirty )
        return;

    Ogre::HlmsDatablock *datablock = m_mainWindow->getActiveDatablock();
    if( !datablock )
        return;

    if( !m_ignoreUndo || m_datablockDirty != DirtyStateDirtyFromSlider )
    {
        m_mainWindow->getUndoSystem().pushUndoState( datablock );
        m_ignoreUndo = m_datablockDirty == DirtyStateDirtyFromSlider;
    }

    if( m_undoMouseUp )
    {
        m_ignoreUndo = false;
        m_undoMouseUp = false;
    }

    OGRE_ASSERT_HIGH( dynamic_cast<Ogre::HlmsPbsDatablock *>( datablock ) );
    Ogre::HlmsPbsDatablock *pbsDatablock = static_cast<Ogre::HlmsPbsDatablock *>( datablock );

    pbsDatablock->setWorkflow( Ogre::HlmsPbsDatablock::Workflows( m_workflowChoice->GetSelection() ) );

    OGRE_ASSERT_LOW( (unsigned)m_brdfChoice->GetSelection() <
                     sizeof( kBrdfList ) / sizeof( kBrdfList[0] ) );
    pbsDatablock->setBrdf( kBrdfList[m_brdfChoice->GetSelection()] );

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
        if( pbsDatablock->getWorkflow() != Ogre::HlmsPbsDatablock::MetallicWorkflow )
        {
            pbsDatablock->setFresnel( col, m_fresnelColouredCheckbox->IsChecked() );
        }
        else
        {
            pbsDatablock->setMetalness( col.x );
        }
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

    if( m_alphaHashCheckbox->IsChecked() )
    {
        // Make sure A2C is enabled if Alpha Hashing is being used.
        const Ogre::HlmsBlendblock *blendblock = pbsDatablock->getBlendblock();
        if( blendblock->mAlphaToCoverage == Ogre::HlmsBlendblock::A2cDisabled )
        {
            Ogre::HlmsBlendblock newBlendblock = *blendblock;
            newBlendblock.mAlphaToCoverage = Ogre::HlmsBlendblock::A2cEnabledMsaaOnly;
            pbsDatablock->setBlendblock( newBlendblock, false, false );
        }
    }
    pbsDatablock->setAlphaHashing( m_alphaHashCheckbox->IsChecked() );

    m_datablockDirty = DirtyStateUpToDate;
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::refreshFromDatablock()
{
    OGRE_ASSERT_LOW( !m_editing );
    EditingScope scope( m_editing );

    const Ogre::HlmsDatablock *datablock = m_mainWindow->getActiveDatablock();
    if( !datablock || datablock->getCreator()->getType() != Ogre::HLMS_PBS )
    {
        this->Disable();
        return;
    }

    this->Enable();

    OGRE_ASSERT_HIGH( dynamic_cast<const Ogre::HlmsPbsDatablock *>( datablock ) );
    const Ogre::HlmsPbsDatablock *pbsDatablock =
        static_cast<const Ogre::HlmsPbsDatablock *>( datablock );

    m_workflowChoice->SetSelection( pbsDatablock->getWorkflow() );

    for( size_t i = 0u; i < sizeof( kBrdfList ) / sizeof( kBrdfList[0] ); ++i )
    {
        if( pbsDatablock->getBrdf() == kBrdfList[i] )
        {
            m_brdfChoice->SetSelection( (int)i );
            break;
        }
    }

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

    m_datablockDirty = DirtyStateUpToDate;
    m_ignoreUndo = false;
}
//-----------------------------------------------------------------------------
void PbsParametersPanel::refreshSubMeshList()
{
    OGRE_ASSERT_LOW( !m_editing );
    EditingScope scope( m_editing );

    Ogre::MovableObject *movableObject = m_mainWindow->getActiveObject();
    Ogre::HlmsDatablock *datablock = m_mainWindow->getActiveDatablock();

    if( !movableObject || !datablock )
    {
        m_submeshListBox->Clear();
        return;
    }

    const size_t numSubMeshes = movableObject->mRenderables.size();
    if( numSubMeshes != size_t( m_submeshListBox->GetCount() ) )
    {
        m_submeshListBox->Clear();
        wxArrayString entries;
        entries.reserve( numSubMeshes );
        for( size_t i = 0u; i < numSubMeshes; ++i )
            entries.push_back( wxString::Format( "Submesh %zu", i ) );
        m_submeshListBox->Set( entries );
    }

    int idx = 0;
    for( const Ogre::Renderable *renderable : movableObject->mRenderables )
        m_submeshListBox->SetSelection( idx++, renderable->getDatablock() == datablock );
}
