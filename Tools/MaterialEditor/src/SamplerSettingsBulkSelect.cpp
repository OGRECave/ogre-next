#include "SamplerSettingsBulkSelect.h"

SamplerSettingsBulkSelect::SamplerSettingsBulkSelect( wxWindow *parent ) :
    SamplerSettingsBulkSelectBase( parent ),
    m_editing( false )
{
    EditingScope scope( m_editing );
    setupAllCheckboxes();
}
//-----------------------------------------------------------------------------
void SamplerSettingsBulkSelect::setupAllCheckboxes()
{
    const bool bAllBase = m_diffuse->IsChecked() && m_normal->IsChecked() && m_specular->IsChecked() &&
                          m_roughness->IsChecked() && m_emissive->IsChecked();
    const bool bDetailWeights = m_detailWeights->IsChecked();
    const bool bAllDetails = m_detail0Diffuse->IsChecked() && m_detail0Nm->IsChecked() &&
                             m_detail1Diffuse->IsChecked() && m_detail1Nm->IsChecked() &&
                             m_detail2Diffuse->IsChecked() && m_detail2Nm->IsChecked() &&
                             m_detail3Diffuse->IsChecked() && m_detail3Nm->IsChecked();

    m_all->SetValue( bAllBase && bDetailWeights && bAllDetails );
    m_allBase->SetValue( bAllBase );
    m_allDetails->SetValue( bAllDetails );

    const bool bAnyOn =
        m_diffuse->IsChecked() || m_normal->IsChecked() || m_specular->IsChecked() ||
        m_roughness->IsChecked() || m_emissive->IsChecked() || m_detailWeights->IsChecked() ||
        m_detail0Diffuse->IsChecked() || m_detail0Nm->IsChecked() || m_detail1Diffuse->IsChecked() ||
        m_detail1Nm->IsChecked() || m_detail2Diffuse->IsChecked() || m_detail2Nm->IsChecked() ||
        m_detail3Diffuse->IsChecked() || m_detail3Nm->IsChecked();

    m_okButton->Enable( bAnyOn );
}
//-----------------------------------------------------------------------------
void SamplerSettingsBulkSelect::OnCheckbox( wxCommandEvent &event )
{
    event.Skip();

    if( m_editing )
        return;
    EditingScope scope( m_editing );

    if( m_all == event.GetEventObject() )
    {
        const bool bValue = m_all->IsChecked();
        m_allBase->SetValue( bValue );
        m_diffuse->SetValue( bValue );
        m_normal->SetValue( bValue );
        m_specular->SetValue( bValue );
        m_roughness->SetValue( bValue );
        m_emissive->SetValue( bValue );
        m_detailWeights->SetValue( bValue );
        m_allDetails->SetValue( bValue );
        m_detail0Diffuse->SetValue( bValue );
        m_detail0Nm->SetValue( bValue );
        m_detail1Diffuse->SetValue( bValue );
        m_detail1Nm->SetValue( bValue );
        m_detail2Diffuse->SetValue( bValue );
        m_detail2Nm->SetValue( bValue );
        m_detail3Diffuse->SetValue( bValue );
        m_detail3Nm->SetValue( bValue );
    }
    else if( m_allBase == event.GetEventObject() )
    {
        const bool bValue = m_allBase->IsChecked();
        m_diffuse->SetValue( bValue );
        m_normal->SetValue( bValue );
        m_specular->SetValue( bValue );
        m_roughness->SetValue( bValue );
        m_emissive->SetValue( bValue );
    }
    else if( m_allDetails == event.GetEventObject() )
    {
        const bool bValue = m_allDetails->IsChecked();
        m_detail0Diffuse->SetValue( bValue );
        m_detail0Nm->SetValue( bValue );
        m_detail1Diffuse->SetValue( bValue );
        m_detail1Nm->SetValue( bValue );
        m_detail2Diffuse->SetValue( bValue );
        m_detail2Nm->SetValue( bValue );
        m_detail3Diffuse->SetValue( bValue );
        m_detail3Nm->SetValue( bValue );
    }

    setupAllCheckboxes();
}
//-----------------------------------------------------------------------------
bool SamplerSettingsBulkSelect::isSelected( const Ogre::PbsTextureTypes textureTypes ) const
{
    switch( textureTypes )
    {
    case Ogre::PBSM_DIFFUSE:
        return m_diffuse->IsChecked();
    case Ogre::PBSM_NORMAL:
        return m_normal->IsChecked();
    case Ogre::PBSM_SPECULAR:
        return m_specular->IsChecked();
    case Ogre::PBSM_ROUGHNESS:
        return m_roughness->IsChecked();
    case Ogre::PBSM_DETAIL_WEIGHT:
        return m_detailWeights->IsChecked();
    case Ogre::PBSM_DETAIL0:
        return m_detail0Diffuse->IsChecked();
    case Ogre::PBSM_DETAIL1:
        return m_detail1Diffuse->IsChecked();
    case Ogre::PBSM_DETAIL2:
        return m_detail2Diffuse->IsChecked();
    case Ogre::PBSM_DETAIL3:
        return m_detail3Diffuse->IsChecked();
    case Ogre::PBSM_DETAIL0_NM:
        return m_detail0Nm->IsChecked();
    case Ogre::PBSM_DETAIL1_NM:
        return m_detail1Nm->IsChecked();
    case Ogre::PBSM_DETAIL2_NM:
        return m_detail2Nm->IsChecked();
    case Ogre::PBSM_DETAIL3_NM:
        return m_detail3Nm->IsChecked();
    case Ogre::PBSM_EMISSIVE:
        return m_emissive->IsChecked();
    case Ogre::PBSM_REFLECTION:
        return false;
    case Ogre::NUM_PBSM_TEXTURE_TYPES:
        return false;
    }
}
