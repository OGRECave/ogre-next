#include "PbsTexturePanel.h"

#include "MainWindow.h"
#include "TextureSelect.h"

#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreRenderSystem.h"
#include "OgreRoot.h"
#include "OgreTextureFilters.h"
#include "OgreTextureGpu.h"
#include "OgreTextureGpuManager.h"

// If user selects "NormalMap.png" as both diffuse and normal map; they need to be loaded very
// differently (and have 2 duplicates). Address this issue by *always* appending "_nm" to normal maps.
static const char *getAliasSuffix( const uint32_t textureFilters )
{
    using namespace Ogre;
    if( textureFilters & TextureFilter::TypePrepareForNormalMapping )
        return "_nm";

    if( textureFilters & TextureFilter::TypeLeaveChannelR )
        return "_r";

    return "";
}

PbsTexturePanel::PbsTexturePanel( MainWindow *parent ) :
    PbsTexturePanelBase( parent ),
    m_mainWindow( parent ),
    m_reflectionMap( 0 ),
    m_editing( false ),
    m_ignoreUndo( false ),
    m_undoMouseUp( false )
{
    m_units[Ogre::PBSM_DIFFUSE] = { m_diffuseMapBtn, m_diffuseMapSpin };
    m_units[Ogre::PBSM_NORMAL] = { m_normalMapBtn, m_normalMapSpin };
    m_units[Ogre::PBSM_SPECULAR] = { m_specularMapBtn, m_specularMapSpin };
    m_units[Ogre::PBSM_ROUGHNESS] = { m_roughnessMapBtn, m_roughnessMapSpin };
    m_units[Ogre::PBSM_DETAIL_WEIGHT] = { m_detailWeightMapBtn, m_detailWeightMapSpin };
    m_units[Ogre::PBSM_DETAIL0] = { m_detailMapBtn0, m_detailMapSpin0 };
    m_units[Ogre::PBSM_DETAIL1] = { m_detailMapBtn1, m_detailMapSpin1 };
    m_units[Ogre::PBSM_DETAIL2] = { m_detailMapBtn2, m_detailMapSpin2 };
    m_units[Ogre::PBSM_DETAIL3] = { m_detailMapBtn3, m_detailMapSpin3 };
    m_units[Ogre::PBSM_DETAIL0_NM] = { m_detailNmMapBtn0, m_detailNmMapSpin0 };
    m_units[Ogre::PBSM_DETAIL1_NM] = { m_detailNmMapBtn1, m_detailNmMapSpin1 };
    m_units[Ogre::PBSM_DETAIL2_NM] = { m_detailNmMapBtn2, m_detailNmMapSpin2 };
    m_units[Ogre::PBSM_DETAIL3_NM] = { m_detailNmMapBtn3, m_detailNmMapSpin3 };
    m_units[Ogre::PBSM_EMISSIVE] = { m_emissiveMapBtn, m_emissiveMapSpin };
    m_units[Ogre::PBSM_REFLECTION] = {};

    OGRE_ASSERT( m_detailMapBlendMode0->GetCount() == Ogre::NUM_PBSM_BLEND_MODES );
    OGRE_ASSERT( m_detailMapBlendMode1->GetCount() == Ogre::NUM_PBSM_BLEND_MODES );
    OGRE_ASSERT( m_detailMapBlendMode2->GetCount() == Ogre::NUM_PBSM_BLEND_MODES );
    OGRE_ASSERT( m_detailMapBlendMode3->GetCount() == Ogre::NUM_PBSM_BLEND_MODES );

    Ogre::TextureGpuManager *textureManager =
        parent->getRoot()->getRenderSystem()->getTextureGpuManager();
    m_reflectionMap = textureManager->createOrRetrieveTexture(
        "SaintPetersBasilica.dds", "## INTERNAL SaintPetersBasilica.dds",
        Ogre::GpuPageOutStrategy::Discard, Ogre::CommonTextureTypes::EnvMap,
        Ogre::ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME );
    m_reflectionMap->scheduleTransitionTo( Ogre::GpuResidency::Resident );

    refreshFromDatablockAndApplyEnvMap();
}
//-----------------------------------------------------------------------------
Ogre::PbsTextureTypes PbsTexturePanel::findFrom( wxButton *button ) const
{
    for( size_t i = 0u; i < Ogre::NUM_PBSM_TEXTURE_TYPES; ++i )
    {
        if( m_units[i].mapButton == button )
            return static_cast<Ogre::PbsTextureTypes>( i );
    }
    return Ogre::NUM_PBSM_TEXTURE_TYPES;
}
//-----------------------------------------------------------------------------
wxChoice *PbsTexturePanel::getBlendMode( const Ogre::PbsTextureTypes textureTypes )
{
    switch( textureTypes )
    {
    case Ogre::PBSM_DIFFUSE:
    case Ogre::PBSM_SPECULAR:
    case Ogre::PBSM_ROUGHNESS:
    case Ogre::PBSM_DETAIL_WEIGHT:
    case Ogre::PBSM_DETAIL0_NM:
    case Ogre::PBSM_DETAIL1_NM:
    case Ogre::PBSM_DETAIL2_NM:
    case Ogre::PBSM_DETAIL3_NM:
    case Ogre::PBSM_EMISSIVE:
    case Ogre::PBSM_REFLECTION:
    case Ogre::NUM_PBSM_TEXTURE_TYPES:
    case Ogre::PBSM_NORMAL:
        return 0;
    case Ogre::PBSM_DETAIL0:
        return m_detailMapBlendMode0;
    case Ogre::PBSM_DETAIL1:
        return m_detailMapBlendMode1;
    case Ogre::PBSM_DETAIL2:
        return m_detailMapBlendMode2;
    case Ogre::PBSM_DETAIL3:
        return m_detailMapBlendMode3;
    }
}
//-----------------------------------------------------------------------------
SliderTextWidget PbsTexturePanel::getStrengthSliderWidgets( const Ogre::PbsTextureTypes textureTypes )
{
    switch( textureTypes )
    {
    case Ogre::PBSM_DIFFUSE:
    case Ogre::PBSM_SPECULAR:
    case Ogre::PBSM_ROUGHNESS:
    case Ogre::PBSM_DETAIL_WEIGHT:
    case Ogre::PBSM_EMISSIVE:
    case Ogre::PBSM_REFLECTION:
    case Ogre::NUM_PBSM_TEXTURE_TYPES:
        return {};
    case Ogre::PBSM_NORMAL:
        return { m_normalMapSlider, m_normalMapTextCtrl };
    case Ogre::PBSM_DETAIL0:
        return { m_detailMapSlider0, m_detailMapTextCtrl0 };
    case Ogre::PBSM_DETAIL1:
        return { m_detailMapSlider1, m_detailMapTextCtrl1 };
    case Ogre::PBSM_DETAIL2:
        return { m_detailMapSlider2, m_detailMapTextCtrl2 };
    case Ogre::PBSM_DETAIL3:
        return { m_detailMapSlider3, m_detailMapTextCtrl3 };
    case Ogre::PBSM_DETAIL0_NM:
        return { m_detailNmMapSlider0, m_detailNmMapTextCtrl0 };
    case Ogre::PBSM_DETAIL1_NM:
        return { m_detailNmMapSlider1, m_detailNmMapTextCtrl1 };
    case Ogre::PBSM_DETAIL2_NM:
        return { m_detailNmMapSlider2, m_detailNmMapTextCtrl2 };
    case Ogre::PBSM_DETAIL3_NM:
        return { m_detailNmMapSlider3, m_detailNmMapTextCtrl3 };
    }
}
//-----------------------------------------------------------------------------
PbsTexturePanel::DetailMapOffsetScale PbsTexturePanel::getDetailMapOffsetScale(
    const Ogre::PbsTextureTypes textureTypes )
{
    switch( textureTypes )
    {
    case Ogre::PBSM_DIFFUSE:
    case Ogre::PBSM_SPECULAR:
    case Ogre::PBSM_ROUGHNESS:
    case Ogre::PBSM_DETAIL_WEIGHT:
    case Ogre::PBSM_DETAIL0_NM:
    case Ogre::PBSM_DETAIL1_NM:
    case Ogre::PBSM_DETAIL2_NM:
    case Ogre::PBSM_DETAIL3_NM:
    case Ogre::PBSM_EMISSIVE:
    case Ogre::PBSM_REFLECTION:
    case Ogre::NUM_PBSM_TEXTURE_TYPES:
    case Ogre::PBSM_NORMAL:
        return {};
    case Ogre::PBSM_DETAIL0:
        return { { m_detailMapXSlider0, m_detailMapXTextCtrl0 },
                 { m_detailMapYSlider0, m_detailMapYTextCtrl0 },
                 { m_detailMapWSlider0, m_detailMapWTextCtrl0 },
                 { m_detailMapHSlider0, m_detailMapHTextCtrl0 } };
    case Ogre::PBSM_DETAIL1:
        return { { m_detailMapXSlider1, m_detailMapXTextCtrl1 },
                 { m_detailMapYSlider1, m_detailMapYTextCtrl1 },
                 { m_detailMapWSlider1, m_detailMapWTextCtrl1 },
                 { m_detailMapHSlider1, m_detailMapHTextCtrl1 } };
    case Ogre::PBSM_DETAIL2:
        return { { m_detailMapXSlider2, m_detailMapXTextCtrl2 },
                 { m_detailMapYSlider2, m_detailMapYTextCtrl2 },
                 { m_detailMapWSlider2, m_detailMapWTextCtrl2 },
                 { m_detailMapHSlider2, m_detailMapHTextCtrl2 } };
    case Ogre::PBSM_DETAIL3:
        return { { m_detailMapXSlider3, m_detailMapXTextCtrl3 },
                 { m_detailMapYSlider3, m_detailMapYTextCtrl3 },
                 { m_detailMapWSlider3, m_detailMapWTextCtrl3 },
                 { m_detailMapHSlider3, m_detailMapHTextCtrl3 } };
    }
}
#if 0
//-----------------------------------------------------------------------------
void PbsTexturePanel::converTo( Ogre::TextureGpu *texture, wxImage &outImage )
{
    const uint32_t targetHeight = 32u;
    const uint32_t origHeight = texture->getHeight();
    uint8_t bestMip = 0u;

    const uint8_t numMips = texture->getNumMipmaps();
    for( uint8_t mipLevel = 0u; mipLevel < numMips; ++mipLevel )
    {
        if( targetHeight <= std::max( 1u, origHeight >> mipLevel ) )
            bestMip = mipLevel;
    }

    Ogre::Image2 image;
    image.convertFromTexture( texture, bestMip, bestMip );
    if( targetHeight != image.getWidth() || targetHeight != image.getHeight() )
        image.resize( targetHeight, targetHeight, Ogre::Image2::FILTER_NEAREST );

    outImage.Create( targetHeight, targetHeight, false );
    const Ogre::TextureBox srcData = image.getData( 0u );
    for( uint32_t y = 0u; y < srcData.height; ++y )
    {
        uint8_t const *srcRaw = reinterpret_cast<const uint8_t *>( srcData.at( 0u, y, 0u ) );
        for( uint32_t x = 0u; x < srcData.width; ++x )
        {
            outImage.SetRGB( int( x ), int( y ), srcRaw[x * 4u + 0u], srcRaw[x * 4u + 1u],
                             srcRaw[x * 4u + 2u] );
        }
    }
}
#endif
//-----------------------------------------------------------------------------
void PbsTexturePanel::OnTextureChangeButton( wxCommandEvent &event )
{
    if( m_editing )
        return;
    EditingScope scope( m_editing );

    event.Skip();

    Ogre::HlmsDatablock *baseDatablock = m_mainWindow->getActiveDatablock();
    if( !baseDatablock || baseDatablock->getCreator()->getType() != Ogre::HLMS_PBS )
        return;

    TextureSelect textureSelect( this );
    textureSelect.ShowModal();

    const TextureSelect::ResourceEntry *chosenEntry = textureSelect.getChosenEntry();
    if( chosenEntry && !chosenEntry->name.empty() && !chosenEntry->resourceGroup.empty() )
    {
        m_mainWindow->getUndoSystem().pushUndoState( baseDatablock );

        OGRE_ASSERT_HIGH( dynamic_cast<Ogre::HlmsPbsDatablock *>( baseDatablock ) );
        Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock *>( baseDatablock );

        using namespace Ogre;
        OGRE_ASSERT_HIGH( dynamic_cast<wxButton *>( event.GetEventObject() ) );
        const PbsTextureTypes textureType =
            findFrom( static_cast<wxButton *>( event.GetEventObject() ) );

        OGRE_ASSERT_LOW( textureType != NUM_PBSM_TEXTURE_TYPES );

        uint32_t textureFlags = TextureFlags::AutomaticBatching;

        if( datablock->suggestUsingSRGB( textureType ) )
            textureFlags |= TextureFlags::PrefersLoadingFromFileAsSRGB;

        TextureTypes::TextureTypes internalTextureType = TextureTypes::Type2D;
        if( textureType == PBSM_REFLECTION )
        {
            internalTextureType = TextureTypes::TypeCube;
            textureFlags &= static_cast<uint32_t>( ~TextureFlags::AutomaticBatching );
        }

        uint32_t filters = TextureFilter::TypeGenerateDefaultMipmaps;
        filters |= datablock->suggestFiltersForType( textureType );

        TextureGpuManager *textureManager =
            datablock->getCreator()->getRenderSystem()->getTextureGpuManager();

        TextureGpu *texture = textureManager->createOrRetrieveTexture(
            chosenEntry->name, chosenEntry->name + getAliasSuffix( filters ),
            GpuPageOutStrategy::Discard, textureFlags, internalTextureType, chosenEntry->resourceGroup,
            filters );

        datablock->setTexture( textureType, texture, datablock->getSamplerblock( textureType ) );

        // texture->scheduleTransitionTo( Ogre::GpuResidency::Resident );
        // texture->waitForData();

        m_units[textureType].mapButton->SetLabel( chosenEntry->name );

        /*wxImage img;
        converTo( texture, img );
        m_units[textureType].mapBmp->SetBitmap( wxBitmapBundle( img ) );*/
    }
    else if( chosenEntry )
    {
        m_mainWindow->getUndoSystem().pushUndoState( baseDatablock );

        using namespace Ogre;
        OGRE_ASSERT_HIGH( dynamic_cast<wxButton *>( event.GetEventObject() ) );
        const PbsTextureTypes textureType =
            findFrom( static_cast<wxButton *>( event.GetEventObject() ) );

        OGRE_ASSERT_HIGH( dynamic_cast<Ogre::HlmsPbsDatablock *>( baseDatablock ) );
        Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock *>( baseDatablock );

        datablock->setTexture( textureType, (TextureGpu *)nullptr,
                               datablock->getSamplerblock( textureType ) );
        m_units[textureType].mapButton->SetLabel( wxString() );
    }
}
//-----------------------------------------------------------------------------
void PbsTexturePanel::OnSpinCtrl( wxSpinEvent &event )
{
    if( m_editing )
        return;
    EditingScope scope( m_editing );

    event.Skip();

    // TODO: Hook up UV sets without crashing the editor every time the mesh doesn't have the UV sets.
}
//-----------------------------------------------------------------------------
void PbsTexturePanel::OnSlider( wxCommandEvent &event )
{
    if( m_editing )
        return;
    EditingScope scope( m_editing );

    event.Skip();

    Ogre::HlmsDatablock *baseDatablock = m_mainWindow->getActiveDatablock();
    if( !baseDatablock || baseDatablock->getCreator()->getType() != Ogre::HLMS_PBS )
        return;

    for( size_t i = 0u; i < Ogre::NUM_PBSM_TEXTURE_TYPES; ++i )
    {
        const Ogre::PbsTextureTypes textureType = static_cast<Ogre::PbsTextureTypes>( i );

        SliderTextWidget widget = getStrengthSliderWidgets( textureType );
        if( widget.slider && widget.slider == event.GetEventObject() )
        {
            widget.fromSlider();

            double val;
            if( widget.text->GetValue().ToDouble( &val ) )
            {
                if( !m_ignoreUndo )
                {
                    m_mainWindow->getUndoSystem().pushUndoState( baseDatablock );
                    m_ignoreUndo = true;
                }
                if( m_undoMouseUp )
                {
                    m_ignoreUndo = false;
                    m_undoMouseUp = false;
                }

                OGRE_ASSERT_HIGH( dynamic_cast<Ogre::HlmsPbsDatablock *>( baseDatablock ) );
                Ogre::HlmsPbsDatablock *datablock =
                    static_cast<Ogre::HlmsPbsDatablock *>( baseDatablock );

                if( i == Ogre::PBSM_NORMAL )
                    datablock->setNormalMapWeight( Ogre::Real( val ) );
                else if( i >= Ogre::PBSM_DETAIL0 && i <= Ogre::PBSM_DETAIL3 )
                {
                    datablock->setDetailMapWeight( uint8_t( i - Ogre::PBSM_DETAIL0 ),
                                                   Ogre::Real( val ) );
                }
            }

            return;
        }

        DetailMapOffsetScale offsetScale = getDetailMapOffsetScale( textureType );
        if( offsetScale.x.slider )
        {
            if( offsetScale.x.slider == event.GetEventObject() ||
                offsetScale.y.slider == event.GetEventObject() ||
                offsetScale.w.slider == event.GetEventObject() ||
                offsetScale.h.slider == event.GetEventObject() )
            {
                if( !m_ignoreUndo )
                {
                    m_mainWindow->getUndoSystem().pushUndoState( baseDatablock );
                    m_ignoreUndo = true;
                }
                if( m_undoMouseUp )
                {
                    m_ignoreUndo = false;
                    m_undoMouseUp = false;
                }

                offsetScale.x.fromSlider();
                offsetScale.y.fromSlider();
                offsetScale.w.fromSlider();
                offsetScale.h.fromSlider();

                OGRE_ASSERT_HIGH( dynamic_cast<Ogre::HlmsPbsDatablock *>( baseDatablock ) );
                Ogre::HlmsPbsDatablock *datablock =
                    static_cast<Ogre::HlmsPbsDatablock *>( baseDatablock );

                Ogre::Vector4 value(
                    datablock->getDetailMapOffsetScale( uint8_t( i - Ogre::PBSM_DETAIL0 ) ) );

                double val;
                if( offsetScale.x.text->GetValue().ToDouble( &val ) )
                    value.x = Ogre::Real( val );
                if( offsetScale.y.text->GetValue().ToDouble( &val ) )
                    value.y = Ogre::Real( val );
                if( offsetScale.w.text->GetValue().ToDouble( &val ) )
                    value.z = Ogre::Real( val );
                if( offsetScale.h.text->GetValue().ToDouble( &val ) )
                    value.w = Ogre::Real( val );
                datablock->setDetailMapOffsetScale( uint8_t( i - Ogre::PBSM_DETAIL0 ), value );
                return;
            }
        }
    }
}
//-----------------------------------------------------------------------------
void PbsTexturePanel::OnText( wxCommandEvent &event )
{
    if( m_editing )
        return;
    EditingScope scope( m_editing );

    event.Skip();

    Ogre::HlmsDatablock *baseDatablock = m_mainWindow->getActiveDatablock();
    if( !baseDatablock || baseDatablock->getCreator()->getType() != Ogre::HLMS_PBS )
        return;

    for( size_t i = 0u; i < Ogre::NUM_PBSM_TEXTURE_TYPES; ++i )
    {
        const Ogre::PbsTextureTypes textureType = static_cast<Ogre::PbsTextureTypes>( i );

        SliderTextWidget widget = getStrengthSliderWidgets( textureType );
        if( widget.text && widget.text == event.GetEventObject() )
        {
            widget.fromText();

            double val;
            if( widget.text->GetValue().ToDouble( &val ) )
            {
                m_mainWindow->getUndoSystem().pushUndoState( baseDatablock );

                OGRE_ASSERT_HIGH( dynamic_cast<Ogre::HlmsPbsDatablock *>( baseDatablock ) );
                Ogre::HlmsPbsDatablock *datablock =
                    static_cast<Ogre::HlmsPbsDatablock *>( baseDatablock );

                if( i == Ogre::PBSM_NORMAL )
                    datablock->setNormalMapWeight( Ogre::Real( val ) );
                else if( i >= Ogre::PBSM_DETAIL0 && i <= Ogre::PBSM_DETAIL3 )
                {
                    datablock->setDetailMapWeight( uint8_t( i - Ogre::PBSM_DETAIL0 ),
                                                   Ogre::Real( val ) );
                }
            }

            return;
        }

        DetailMapOffsetScale offsetScale = getDetailMapOffsetScale( textureType );
        if( offsetScale.x.text )
        {
            if( offsetScale.x.text == event.GetEventObject() ||
                offsetScale.y.text == event.GetEventObject() ||
                offsetScale.w.text == event.GetEventObject() ||
                offsetScale.h.text == event.GetEventObject() )
            {
                m_mainWindow->getUndoSystem().pushUndoState( baseDatablock );

                offsetScale.x.fromText();
                offsetScale.y.fromText();
                offsetScale.w.fromText();
                offsetScale.h.fromText();

                OGRE_ASSERT_HIGH( dynamic_cast<Ogre::HlmsPbsDatablock *>( baseDatablock ) );
                Ogre::HlmsPbsDatablock *datablock =
                    static_cast<Ogre::HlmsPbsDatablock *>( baseDatablock );

                Ogre::Vector4 value(
                    datablock->getDetailMapOffsetScale( uint8_t( i - Ogre::PBSM_DETAIL0 ) ) );

                double val;
                if( offsetScale.x.text->GetValue().ToDouble( &val ) )
                    value.x = Ogre::Real( val );
                if( offsetScale.y.text->GetValue().ToDouble( &val ) )
                    value.y = Ogre::Real( val );
                if( offsetScale.w.text->GetValue().ToDouble( &val ) )
                    value.z = Ogre::Real( val );
                if( offsetScale.h.text->GetValue().ToDouble( &val ) )
                    value.w = Ogre::Real( val );
                datablock->setDetailMapOffsetScale( uint8_t( i - Ogre::PBSM_DETAIL0 ), value );
                return;
            }
        }
    }
}
//-----------------------------------------------------------------------------
void PbsTexturePanel::OnBlendModeChoice( wxCommandEvent &event )
{
    if( m_editing )
        return;
    EditingScope scope( m_editing );

    event.Skip();

    Ogre::HlmsDatablock *baseDatablock = m_mainWindow->getActiveDatablock();
    if( !baseDatablock || baseDatablock->getCreator()->getType() != Ogre::HLMS_PBS )
        return;

    for( uint8_t i = Ogre::PBSM_DETAIL0; i <= Ogre::PBSM_DETAIL3; ++i )
    {
        m_mainWindow->getUndoSystem().pushUndoState( baseDatablock );

        OGRE_ASSERT_HIGH( dynamic_cast<Ogre::HlmsPbsDatablock *>( baseDatablock ) );
        Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock *>( baseDatablock );

        wxChoice *choice = getBlendMode( static_cast<Ogre::PbsTextureTypes>( i ) );
        datablock->setDetailMapBlendMode( i - Ogre::PBSM_DETAIL0,
                                          Ogre::PbsBlendModes( choice->GetSelection() ) );
    }
}
//-----------------------------------------------------------------------------
void PbsTexturePanel::OnCollapsiblePaneChanged( wxCollapsiblePaneEvent &event )
{
    this->Layout();
    event.Skip();
}
//-----------------------------------------------------------------------------
void PbsTexturePanel::UndoMouseUp( wxMouseEvent &event )
{
    event.Skip();
    m_undoMouseUp = true;
}
//-----------------------------------------------------------------------------
void PbsTexturePanel::UndoKeyUp( wxKeyEvent &event )
{
    event.Skip();
    m_ignoreUndo = false;
}
//-----------------------------------------------------------------------------
void PbsTexturePanel::UndoKillFocus( wxFocusEvent &event )
{
    event.Skip();
    m_ignoreUndo = false;
}
//-----------------------------------------------------------------------------
void PbsTexturePanel::refreshFromDatablockAndApplyEnvMap()
{
    OGRE_ASSERT_LOW( !m_editing );
    EditingScope scope( m_editing );

    Ogre::HlmsDatablock *datablockBase = m_mainWindow->getActiveDatablock();
    if( !datablockBase || datablockBase->getCreator()->getType() != Ogre::HLMS_PBS )
    {
        this->Disable();
        return;
    }

    this->Enable();

    applyEnvMapToDatablock( datablockBase );

    OGRE_ASSERT_HIGH( dynamic_cast<const Ogre::HlmsPbsDatablock *>( datablockBase ) );
    const Ogre::HlmsPbsDatablock *datablock =
        static_cast<const Ogre::HlmsPbsDatablock *>( datablockBase );

    for( size_t i = 0u; i < Ogre::NUM_PBSM_TEXTURE_TYPES; ++i )
    {
        if( !m_units[i].mapButton )
            continue;

        const Ogre::PbsTextureTypes textureType = static_cast<Ogre::PbsTextureTypes>( i );

        Ogre::TextureGpu *texture = datablock->getTexture( textureType );
        if( texture )
            m_units[i].mapButton->SetLabel( wxString::FromUTF8( texture->getNameStr() ) );
        else
            m_units[i].mapButton->SetLabel( wxString() );

        m_units[i].mapSpin->SetValue( datablock->getTextureUvSource( textureType ) );

        SliderTextWidget widget = getStrengthSliderWidgets( textureType );
        if( widget.text )
        {
            Ogre::Real value = 1.0f;
            switch( textureType )
            {
            case Ogre::PBSM_DIFFUSE:
            case Ogre::PBSM_SPECULAR:
            case Ogre::PBSM_ROUGHNESS:
            case Ogre::PBSM_DETAIL_WEIGHT:
            case Ogre::PBSM_EMISSIVE:
            case Ogre::PBSM_REFLECTION:
            case Ogre::NUM_PBSM_TEXTURE_TYPES:
                break;
            case Ogre::PBSM_NORMAL:
                value = datablock->getNormalMapWeight();
                break;
            case Ogre::PBSM_DETAIL0:
            case Ogre::PBSM_DETAIL1:
            case Ogre::PBSM_DETAIL2:
            case Ogre::PBSM_DETAIL3:
                value = datablock->getDetailMapWeight( uint8_t( i - Ogre::PBSM_DETAIL0 ) );
                break;
            case Ogre::PBSM_DETAIL0_NM:
            case Ogre::PBSM_DETAIL1_NM:
            case Ogre::PBSM_DETAIL2_NM:
            case Ogre::PBSM_DETAIL3_NM:
                value = datablock->getDetailNormalWeight( uint8_t( i - Ogre::PBSM_DETAIL0_NM ) );
                break;
            }

            widget.text->SetValue( wxString::Format( wxT( "%.05lf" ), value ) );
            widget.fromText();
        }

        DetailMapOffsetScale offsetScale = getDetailMapOffsetScale( textureType );
        if( offsetScale.x.text )
        {
            Ogre::Vector4 value( 0.0f, 0.0f, 1.0f, 1.0f );
            switch( textureType )
            {
            case Ogre::PBSM_DIFFUSE:
            case Ogre::PBSM_SPECULAR:
            case Ogre::PBSM_ROUGHNESS:
            case Ogre::PBSM_DETAIL_WEIGHT:
            case Ogre::PBSM_EMISSIVE:
            case Ogre::PBSM_REFLECTION:
            case Ogre::NUM_PBSM_TEXTURE_TYPES:
            case Ogre::PBSM_NORMAL:
            case Ogre::PBSM_DETAIL0_NM:
            case Ogre::PBSM_DETAIL1_NM:
            case Ogre::PBSM_DETAIL2_NM:
            case Ogre::PBSM_DETAIL3_NM:
                break;
            case Ogre::PBSM_DETAIL0:
            case Ogre::PBSM_DETAIL1:
            case Ogre::PBSM_DETAIL2:
            case Ogre::PBSM_DETAIL3:
                value = datablock->getDetailMapOffsetScale( uint8_t( i - Ogre::PBSM_DETAIL0 ) );
                break;
            }

            offsetScale.x.text->SetValue( wxString::Format( wxT( "%.05lf" ), value.x ) );
            offsetScale.y.text->SetValue( wxString::Format( wxT( "%.05lf" ), value.y ) );
            offsetScale.w.text->SetValue( wxString::Format( wxT( "%.05lf" ), value.z ) );
            offsetScale.h.text->SetValue( wxString::Format( wxT( "%.05lf" ), value.w ) );
            offsetScale.x.fromText();
            offsetScale.y.fromText();
            offsetScale.w.fromText();
            offsetScale.h.fromText();
        }
    }

    for( uint8_t i = Ogre::PBSM_DETAIL0; i <= Ogre::PBSM_DETAIL3; ++i )
    {
        wxChoice *choice = getBlendMode( static_cast<Ogre::PbsTextureTypes>( i ) );
        choice->SetSelection( datablock->getDetailMapBlendMode( i - Ogre::PBSM_DETAIL0 ) );
    }
}
//-----------------------------------------------------------------------------
void PbsTexturePanel::notifyMeshChanged()
{
    const Ogre::MovableObject *movableObject = m_mainWindow->getActiveObject();
    if( !movableObject )
        return;

    for( const Ogre::Renderable *renderable : movableObject->mRenderables )
        applyEnvMapToDatablock( renderable->getDatablock() );
}
//-----------------------------------------------------------------------------
void PbsTexturePanel::applyEnvMapToDatablock( Ogre::HlmsDatablock *baseDatablock )
{
    if( !baseDatablock || baseDatablock->getCreator()->getType() != Ogre::HLMS_PBS )
        return;

    OGRE_ASSERT_HIGH( dynamic_cast<Ogre::HlmsPbsDatablock *>( baseDatablock ) );
    Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock *>( baseDatablock );

    if( !datablock->getTexture( Ogre::PBSM_REFLECTION ) )
    {
        datablock->setTexture( Ogre::PBSM_REFLECTION, m_reflectionMap,
                               datablock->getSamplerblock( Ogre::PBSM_REFLECTION ) );

        static_cast<Ogre::HlmsPbs *>( datablock->getCreator() )
            ->_notifyIblSpecMipmap( m_reflectionMap->getNumMipmaps() );
    }
}
//-----------------------------------------------------------------------------
void PbsTexturePanel::unsetEnvMapFromAllDatablocks()
{
    Ogre::Hlms *hlms = m_mainWindow->getRoot()->getHlmsManager()->getHlms( Ogre::HLMS_PBS );

    const Ogre::Hlms::HlmsDatablockMap &datablockMap = hlms->getDatablockMap();

    for( const auto &keyVal : datablockMap )
    {
        const Ogre::Hlms::DatablockEntry &entry = keyVal.second;
        OGRE_ASSERT_HIGH( dynamic_cast<Ogre::HlmsPbsDatablock *>( entry.datablock ) );
        Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock *>( entry.datablock );

        if( datablock->getTexture( Ogre::PBSM_REFLECTION ) == m_reflectionMap )
        {
            datablock->setTexture( Ogre::PBSM_REFLECTION, (Ogre::TextureGpu *)nullptr,
                                   datablock->getSamplerblock( Ogre::PBSM_REFLECTION ) );
        }
    }
}
