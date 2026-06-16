#include "SamplerSettings.h"

#include "ProjectSettings.h"

#include "OgreHlmsJson.h"

#include <float.h>

SamplerSettings::SamplerSettings( wxWindow *parent, ProjectSettings &projectSettings,
                                  const Ogre::HlmsSamplerblock &selectedSampler ) :
    SamplerSettingsBase( parent ),
    m_projectSettings( projectSettings ),
    m_editing( false )
{
    const NamedSamplerVec &samplers = m_projectSettings.getSamplerTemplates();
    wxArrayString samplerNames;
    samplerNames.reserve( samplers.size() + 1u );
    samplerNames.push_back( wxT( "Custom" ) );
    for( const NamedSampler &sampler : samplers )
        samplerNames.push_back( sampler.name );
    m_presetsChoice->Set( samplerNames );
    m_presetsChoice->SetSelection( 0 );

    refreshFromSamplerblock( selectedSampler );
}
//-----------------------------------------------------------------------------
void SamplerSettings::OnPresetsChoice( wxCommandEvent &event )
{
    if( m_editing )
        return;

    // EditingScope scope( m_editing ); DON'T. We'll do it in refreshFromSamplerblock().

    const NamedSamplerVec &samplers = m_projectSettings.getSamplerTemplates();
    if( m_presetsChoice->GetSelection() > 0 )
    {
        const size_t presetIdx = size_t( m_presetsChoice->GetSelection() );
        if( presetIdx < samplers.size() )
            refreshFromSamplerblock( samplers[presetIdx].samplerblock, false );
    }
}
//-----------------------------------------------------------------------------
void SamplerSettings::OnText( wxCommandEvent &event )
{
    event.Skip();

    if( m_editing )
        return;
    EditingScope scope( m_editing );

    OGRE_ASSERT_HIGH( dynamic_cast<wxTextCtrl *>( event.GetEventObject() ) );
    wxTextCtrl *textCtrl = static_cast<wxTextCtrl *>( event.GetEventObject() );

    if( m_maxAnisotropyTextCtrl == textCtrl )
        validateAnisotropy();
    else
    {
        double value;
        if( textCtrl->GetValue().ToDouble( &value ) )
            textCtrl->SetBackgroundColour( wxNullColour );
        else
            textCtrl->SetBackgroundColour( wxColour( 255, 0, 0 ) );
    }

    m_nameTextCtrl->SetValue( NamedSampler::autogenNameFrom( getSamplerblockFromUI() ) );
}
//-----------------------------------------------------------------------------
void SamplerSettings::OnFilterChoice( wxCommandEvent &event )
{
    event.Skip();

    if( m_editing )
        return;
    EditingScope scope( m_editing );

    validateAnisotropy();

    m_nameTextCtrl->SetValue( NamedSampler::autogenNameFrom( getSamplerblockFromUI() ) );
}
//-----------------------------------------------------------------------------
void SamplerSettings::OnAddressingModeChoice( wxCommandEvent &event )
{
    event.Skip();

    if( m_editing )
        return;
    EditingScope scope( m_editing );

    const bool bHasBorderColour = hasBorderColour();

    m_borderTextCtrlR->Enable( bHasBorderColour );
    m_borderTextCtrlG->Enable( bHasBorderColour );
    m_borderTextCtrlB->Enable( bHasBorderColour );
    m_borderTextCtrlA->Enable( bHasBorderColour );

    m_nameTextCtrl->SetValue( NamedSampler::autogenNameFrom( getSamplerblockFromUI() ) );
}
//-----------------------------------------------------------------------------
void SamplerSettings::OnCompareFunctionChoice( wxCommandEvent &event )
{
    event.Skip();

    if( m_editing )
        return;

    m_nameTextCtrl->SetValue( NamedSampler::autogenNameFrom( getSamplerblockFromUI() ) );
}
//-----------------------------------------------------------------------------
void SamplerSettings::autogenName()
{
    const wxString name = NamedSampler::autogenNameFrom( getSamplerblockFromUI() );
    m_nameTextCtrl->SetValue( name );
}
//-----------------------------------------------------------------------------
bool SamplerSettings::hasAnisotropy() const
{
    return ( m_filterMin->GetSelection() == 2 || m_filterMag->GetSelection() == 2 ||
             m_filterMip->GetSelection() == 2 ) &&
           m_filterMip->GetSelection() != 3;  // Can't have aniso without mipmaps.
}
//-----------------------------------------------------------------------------
void SamplerSettings::validateAnisotropy()
{
    const bool bHasAnisotropy = hasAnisotropy();
    m_maxAnisotropyTextCtrl->Enable( bHasAnisotropy );

    double maxAnisotropy = 0.0;
    m_maxAnisotropyTextCtrl->GetValue().ToDouble( &maxAnisotropy );
    if( bHasAnisotropy )
    {
        if( maxAnisotropy > 1.0 )
            m_maxAnisotropyTextCtrl->SetBackgroundColour( wxNullColour );
        else
            m_maxAnisotropyTextCtrl->SetBackgroundColour( wxColour( 255, 0, 0 ) );
    }
    else
    {
        /*if( maxAnisotropy == 1.0 )
            m_maxAnisotropyTextCtrl->SetBackgroundColour( wxNullColour );
        else
            m_maxAnisotropyTextCtrl->SetBackgroundColour( wxColour( 255, 0, 0 ) );*/
        m_maxAnisotropyTextCtrl->SetBackgroundColour( wxNullColour );
    }
}
//-----------------------------------------------------------------------------
bool SamplerSettings::hasBorderColour() const
{
    return m_addressUChoice->GetSelection() == 3 || m_addressVChoice->GetSelection() == 3 ||
           m_addressWChoice->GetSelection() == 3;
}
//-----------------------------------------------------------------------------
Ogre::FilterOptions SamplerSettings::getFilter( wxChoice *choice )
{
    switch( choice->GetSelection() )
    {
    case 0:
        return Ogre::FO_POINT;
    default:
    case 1:
        return Ogre::FO_LINEAR;
    case 2:
        return Ogre::FO_ANISOTROPIC;
    case 3:
        return Ogre::FO_NONE;
    }
}
//-----------------------------------------------------------------------------
Ogre::TextureAddressingMode SamplerSettings::getAddressMode( wxChoice *choice )
{
    switch( choice->GetSelection() )
    {
    case 0:
        return Ogre::TAM_WRAP;
    case 1:
        return Ogre::TAM_MIRROR;
    default:
    case 2:
        return Ogre::TAM_CLAMP;
    case 3:
        return Ogre::TAM_BORDER;
    }
}
//-----------------------------------------------------------------------------
void SamplerSettings::syncFilter( wxChoice *choice, Ogre::FilterOptions filter )
{
    switch( filter )
    {
    case Ogre::FO_NONE:
        choice->SetSelection( 3 );
        break;
    case Ogre::FO_POINT:
        choice->SetSelection( 0 );
        break;
    case Ogre::FO_LINEAR:
        choice->SetSelection( 1 );
        break;
    case Ogre::FO_ANISOTROPIC:
        choice->SetSelection( 2 );
        break;
    }
}
//-----------------------------------------------------------------------------
void SamplerSettings::syncAddressMode( wxChoice *choice, Ogre::TextureAddressingMode addressingMode )
{
    switch( addressingMode )
    {
    case Ogre::TAM_WRAP:
        choice->SetSelection( 0 );
        break;
    case Ogre::TAM_MIRROR:
        choice->SetSelection( 1 );
        break;
    case Ogre::TAM_UNKNOWN:
    case Ogre::TAM_CLAMP:
        choice->SetSelection( 2 );
        break;
    case Ogre::TAM_BORDER:
        choice->SetSelection( 3 );
        break;
    }
}
//-----------------------------------------------------------------------------
void SamplerSettings::refreshFromSamplerblock( const Ogre::HlmsSamplerblock &sampler,
                                               const bool bChangePreset )
{
    OGRE_ASSERT_LOW( !m_editing );
    EditingScope scope( m_editing );

    if( bChangePreset )
        m_presetsChoice->SetSelection( 0 );

    m_nameTextCtrl->SetValue( NamedSampler::autogenNameFrom( sampler ) );

    syncFilter( m_filterMin, sampler.mMinFilter );
    syncFilter( m_filterMag, sampler.mMagFilter );
    syncFilter( m_filterMip, sampler.mMipFilter );

    syncAddressMode( m_addressUChoice, sampler.mU );
    syncAddressMode( m_addressVChoice, sampler.mV );
    syncAddressMode( m_addressWChoice, sampler.mW );

    m_maxAnisotropyTextCtrl->SetValue( wxString::Format( "%g", sampler.mMaxAnisotropy ) );
    m_mipLodBiasTextCtrl->SetValue( wxString::Format( "%g", sampler.mMipLodBias ) );
    m_minLodTextCtrl->SetValue( wxString::Format( "%g", sampler.mMinLod ) );
    m_maxLodTextCtrl->SetValue( wxString::Format( "%g", sampler.mMaxLod ) );

    switch( sampler.mCompareFunction )
    {
    case Ogre::CMPF_ALWAYS_FAIL:
        m_compareFunctionChoice->SetSelection( 1 );
        break;
    case Ogre::CMPF_ALWAYS_PASS:
        m_compareFunctionChoice->SetSelection( 2 );
        break;
    case Ogre::CMPF_LESS:
        m_compareFunctionChoice->SetSelection( 3 );
        break;
    case Ogre::CMPF_LESS_EQUAL:
        m_compareFunctionChoice->SetSelection( 4 );
        break;
    case Ogre::CMPF_EQUAL:
        m_compareFunctionChoice->SetSelection( 5 );
        break;
    case Ogre::CMPF_NOT_EQUAL:
        m_compareFunctionChoice->SetSelection( 6 );
        break;
    case Ogre::CMPF_GREATER_EQUAL:
        m_compareFunctionChoice->SetSelection( 7 );
        break;
    case Ogre::CMPF_GREATER:
        m_compareFunctionChoice->SetSelection( 8 );
        break;
    case Ogre::NUM_COMPARE_FUNCTIONS:
        m_compareFunctionChoice->SetSelection( 0 );
        break;
    }

    m_borderTextCtrlR->SetValue( wxString::Format( "%g", sampler.mBorderColour.r ) );
    m_borderTextCtrlG->SetValue( wxString::Format( "%g", sampler.mBorderColour.g ) );
    m_borderTextCtrlB->SetValue( wxString::Format( "%g", sampler.mBorderColour.b ) );
    m_borderTextCtrlA->SetValue( wxString::Format( "%g", sampler.mBorderColour.a ) );

    validateAnisotropy();
}
//-----------------------------------------------------------------------------
Ogre::HlmsSamplerblock SamplerSettings::getSamplerblockFromUI() const
{
    Ogre::HlmsSamplerblock sampler;

    sampler.mMinFilter = getFilter( m_filterMin );
    sampler.mMagFilter = getFilter( m_filterMag );
    sampler.mMipFilter = getFilter( m_filterMip );

    sampler.mU = getAddressMode( m_addressUChoice );
    sampler.mV = getAddressMode( m_addressVChoice );
    sampler.mW = getAddressMode( m_addressWChoice );

    double value;
    if( m_mipLodBiasTextCtrl->GetValue().ToDouble( &value ) )
        sampler.mMipLodBias = Ogre::Real( value );
    if( m_minLodTextCtrl->GetValue().ToDouble( &value ) )
    {
        sampler.mMinLod = float( value );
        if( sampler.mMinLod <= -FLT_MAX * 0.5f )
            sampler.mMinLod = -FLT_MAX;
    }
    if( m_maxLodTextCtrl->GetValue().ToDouble( &value ) )
    {
        sampler.mMaxLod = float( value );
        if( sampler.mMaxLod >= FLT_MAX * 0.5f )
            sampler.mMaxLod = FLT_MAX;
    }

    if( hasAnisotropy() )
    {
        sampler.mMaxAnisotropy = 16.0f;
        if( m_maxAnisotropyTextCtrl->GetValue().ToDouble( &value ) )
            sampler.mMaxAnisotropy = float( value );
    }

    switch( m_compareFunctionChoice->GetSelection() )
    {
    default:
    case 0:
        sampler.mCompareFunction = Ogre::NUM_COMPARE_FUNCTIONS;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        sampler.mCompareFunction =
            static_cast<Ogre::CompareFunction>( m_compareFunctionChoice->GetSelection() - 1 );
        break;
    }

    if( hasBorderColour() )
    {
        if( m_borderTextCtrlR->GetValue().ToDouble( &value ) )
            sampler.mBorderColour.r = Ogre::Real( value );
        if( m_borderTextCtrlG->GetValue().ToDouble( &value ) )
            sampler.mBorderColour.g = Ogre::Real( value );
        if( m_borderTextCtrlB->GetValue().ToDouble( &value ) )
            sampler.mBorderColour.b = Ogre::Real( value );
        if( m_borderTextCtrlA->GetValue().ToDouble( &value ) )
            sampler.mBorderColour.a = Ogre::Real( value );
    }

    return sampler;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static const char kFilterLetters[Ogre::FO_ANISOTROPIC + 1u] = {
    'N',
    'P',
    'L',
    'A',
};
static const char kTAMLetters[4u] = {
    'W',
    'M',
    'C',
    'B',
};

wxString NamedSampler::autogenNameFrom( const Ogre::HlmsSamplerblock &sampler )
{
    wxString name;

    using namespace Ogre;

    if( sampler.mMinFilter == FO_POINT && sampler.mMagFilter == FO_POINT )
    {
        name = wxT( "Nearest" );
        if( sampler.mMipFilter == FO_LINEAR )
            name += wxT( "_LinearMip" );
        else if( sampler.mMipFilter == FO_ANISOTROPIC )
            name += wxT( "_AnisoMip" );
    }
    else if( sampler.mMinFilter == FO_LINEAR && sampler.mMagFilter == FO_LINEAR )
    {
        name = wxT( "Bilinear" );
        if( sampler.mMipFilter == FO_LINEAR )
            name = wxT( "Trilinear" );
        else if( sampler.mMipFilter == FO_ANISOTROPIC )
            name += wxT( "_AnisoMip" );
    }
    else if( sampler.mMinFilter == FO_ANISOTROPIC && sampler.mMagFilter == FO_ANISOTROPIC &&
             sampler.mMipFilter == FO_ANISOTROPIC )
    {
        name = wxT( "Anisotropic" );
    }
    else
    {
        name += kFilterLetters[sampler.mMinFilter];
        name += kFilterLetters[sampler.mMagFilter];
        if( sampler.mMipFilter != FO_NONE )
            name += kFilterLetters[sampler.mMipFilter];
    }

    if( sampler.mMipFilter == FO_NONE )
        name += wxT( "_NoMip" );

    if( sampler.mU == TAM_WRAP && sampler.mV == TAM_WRAP && sampler.mW == TAM_WRAP )
        name += wxT( "_Wrap" );
    else if( sampler.mU == TAM_WRAP && sampler.mV == TAM_WRAP && sampler.mW == TAM_CLAMP )
        name += wxT( "_Wrap (Clamp W)" );
    else if( sampler.mU == TAM_CLAMP && sampler.mV == TAM_CLAMP && sampler.mW == TAM_CLAMP )
        name += wxT( "_Clamp" );
    else if( sampler.mU == TAM_CLAMP && sampler.mV == TAM_CLAMP && sampler.mW == TAM_WRAP )
        name += wxT( "_Clamp (Wrap W)" );
    else if( sampler.mU == TAM_BORDER && sampler.mV == TAM_BORDER && sampler.mW == TAM_BORDER )
        name += wxT( "_Border" );
    else if( sampler.mU == TAM_BORDER && sampler.mV == TAM_BORDER && sampler.mW == TAM_CLAMP )
        name += wxT( "_Border (Clamp W)" );
    else if( sampler.mU == TAM_BORDER && sampler.mV == TAM_BORDER && sampler.mW == TAM_WRAP )
        name += wxT( "_Border (Wrap W)" );
    else if( sampler.mU == TAM_MIRROR && sampler.mV == TAM_MIRROR && sampler.mW == TAM_MIRROR )
        name += wxT( "_Mirror" );
    else if( sampler.mU == TAM_MIRROR && sampler.mV == TAM_MIRROR && sampler.mW == TAM_CLAMP )
        name += wxT( "_Mirror (Clamp W)" );
    else if( sampler.mU == TAM_MIRROR && sampler.mV == TAM_MIRROR && sampler.mW == TAM_WRAP )
        name += wxT( "_Mirror (Wrap W)" );
    else
    {
        name += wxT( '_' );
        name += kTAMLetters[sampler.mU];
        name += kTAMLetters[sampler.mV];
        name += kTAMLetters[sampler.mW];
    }

    if( sampler.mMinFilter == FO_ANISOTROPIC || sampler.mMagFilter == FO_ANISOTROPIC ||
        sampler.mMipFilter == FO_ANISOTROPIC )
    {
        if( float( int32( sampler.mMaxAnisotropy ) ) == sampler.mMaxAnisotropy )
            name += wxString::Format( "_Ax%i", int32( sampler.mMaxAnisotropy ) );
        else
            name += wxString::Format( "_Ax%f", sampler.mMaxAnisotropy );
    }

    if( sampler.mMipLodBias != 0.0f )
        name += wxString::Format( "_lod=%f", sampler.mMipLodBias );

    if( sampler.mMinLod > -FLT_MAX * 0.5f )
        name += wxString::Format( "_minlod=%f", sampler.mMinLod );

    if( sampler.mMaxLod < FLT_MAX * 0.5f )
        name += wxString::Format( "_maxlod=%f", sampler.mMaxLod );

    if( sampler.mU == TAM_BORDER || sampler.mV == TAM_BORDER || sampler.mW == TAM_BORDER )
        name += wxString::Format( "_border=%08x", sampler.mBorderColour.getAsRGBA() );

    if( sampler.mCompareFunction != NUM_COMPARE_FUNCTIONS )
    {
        Ogre::String cmpStr;
        HlmsJson::toQuotedStr( sampler.mCompareFunction, cmpStr );
        // Remove the quotes.
        cmpStr.pop_back();
        cmpStr.erase( cmpStr.begin() );
        name += wxT( "_cmp=" );
        name += wxString::FromUTF8( cmpStr );
    }

    return name;
}
//-----------------------------------------------------------------------------
void NamedSampler::addTemplate( NamedSamplerVec &namedSamplers,
                                const Ogre::HlmsSamplerblock &samplerblock )
{
    NamedSamplerVec::iterator itor = find( namedSamplers, samplerblock );

    if( itor == namedSamplers.end() )
        namedSamplers.push_back( { autogenNameFrom( samplerblock ), samplerblock } );
}
