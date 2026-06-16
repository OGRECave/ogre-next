#include "CategoryGrouping.h"

#include "MainWindow.h"
#include "ProjectSettings.h"

#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgreRoot.h"

#include <wx/tokenzr.h>

CategoryGrouping::CategoryGrouping( MainWindow *parent ) :
    CategoryGroupingBase( parent ),
    m_mainWindow( parent ),
    m_editing( false )
{
    wxArrayString groups;
    const GroupedDatablockMap &groupedDatablocks =
        m_mainWindow->getProjectSettings().getGroupedDatablocks();
    groups.reserve( groupedDatablocks.size() );
    for( const auto &group : groupedDatablocks )
        groups.push_back( wxString::FromUTF8( group.first ) );
    m_groupChoice->Set( groups );

    m_currGroup = groupedDatablocks.begin()->first;
    m_groupChoice->SetSelection( 0 );

    refreshGroupless();
    refreshGrouped();
}
//-----------------------------------------------------------------------------
void CategoryGrouping::OnGroupChoice( wxCommandEvent &event )
{
    // TODO: Implement OnGroupChoice
}
//-----------------------------------------------------------------------------
void CategoryGrouping::OnButtonClick( wxCommandEvent &event )
{
    if( m_editing )
        return;

    EditingScope scope( m_editing );

    if( m_addButton == event.GetEventObject() )
        addSelectedToGroup();
    else if( m_removeButton == event.GetEventObject() )
        removeSelectedFromGroup();
}
//-----------------------------------------------------------------------------
void CategoryGrouping::OnListBoxDClick( wxCommandEvent &event )
{
    if( m_grouplessList == event.GetEventObject() )
        addSelectedToGroup();
    else
        removeSelectedFromGroup();
}
//-----------------------------------------------------------------------------
void CategoryGrouping::OnSearchText( wxCommandEvent &event )
{
    if( m_editing )
        return;

    EditingScope scope( m_editing );

    if( m_grouplessSearch == event.GetEventObject() )
        refreshGroupless();
    else
        refreshGrouped();
}
//-----------------------------------------------------------------------------
void CategoryGrouping::addSelectedToGroup()
{
    wxArrayInt selections;
    m_grouplessList->GetSelections( selections );
    for( const int isel : selections )
    {
        const unsigned sel = unsigned( isel );
        if( sel < m_grouplessEntries.size() )
        {
            m_mainWindow->getProjectSettings().addToCategoryGroup( m_currGroup,
                                                                   m_grouplessEntries[sel] );
            m_groupEntries.push_back( m_grouplessEntries[sel] );
            m_groupList->Append( m_grouplessList->GetString( sel ) );
        }
    }

    refreshGroupless();
}
//-----------------------------------------------------------------------------
void CategoryGrouping::removeSelectedFromGroup()
{
    wxArrayInt selections;
    m_groupList->GetSelections( selections );
    for( const int isel : selections )
    {
        const unsigned sel = unsigned( isel );
        if( sel < m_groupEntries.size() )
        {
            m_mainWindow->getProjectSettings().removeFromCategoryGroup( m_currGroup,
                                                                        m_groupEntries[sel] );
            m_grouplessEntries.push_back( m_groupEntries[sel] );
            m_grouplessList->Append( m_groupList->GetString( sel ) );
        }
    }

    refreshGrouped();
}
//-----------------------------------------------------------------------------
void CategoryGrouping::refreshGroupless()
{
    const wxString filterStr = m_grouplessSearch->GetValue().Lower();
    wxStringTokenizer tokenizer( filterStr, " " );

    wxArrayString entriesInList;
    m_grouplessEntries.clear();

    const Ogre::HlmsTypes hlmsTypes[] = { Ogre::HLMS_PBS, Ogre::HLMS_UNLIT };

    const ProjectSettings &projectSettings = m_mainWindow->getProjectSettings();

    Ogre::Root *root = m_mainWindow->getRoot();
    Ogre::HlmsManager *hlmsManager = root->getHlmsManager();

    for( size_t i = 0u; i < sizeof( hlmsTypes ) / sizeof( hlmsTypes[0] ); ++i )
    {
        const Ogre::Hlms::HlmsDatablockMap &datablockMap =
            hlmsManager->getHlms( hlmsTypes[i] )->getDatablockMap();

        for( const auto &keyVal : datablockMap )
        {
            const Ogre::Hlms::DatablockEntry &entry = keyVal.second;
            if( entry.visibleToManager && projectSettings.isDatablockGroupless( entry.name ) )
            {
                const wxString materialName = wxString::FromUTF8( entry.name );
                const wxString needle = materialName.Lower();

                bool bFound = true;
                while( tokenizer.HasMoreTokens() )
                {
                    wxString tokenStr = tokenizer.GetNextToken();
                    bFound &= needle.Find( tokenStr ) != wxNOT_FOUND;
                }
                tokenizer.Reinit( filterStr );

                if( bFound )
                {
                    m_grouplessEntries.push_back( entry.name );
                    entriesInList.push_back( materialName );
                }
            }
        }
    }

    m_grouplessList->Set( entriesInList );
}
//-----------------------------------------------------------------------------
void CategoryGrouping::refreshGrouped()
{
    const wxString filterStr = m_groupSearch->GetValue().Lower();
    wxStringTokenizer tokenizer( filterStr, " " );

    wxArrayString entriesInList;
    m_groupEntries.clear();

    const GroupedDatablockMap &groupedDatablocks =
        m_mainWindow->getProjectSettings().getGroupedDatablocks();

    GroupedDatablockMap::const_iterator itGroup = groupedDatablocks.find( m_currGroup );
    if( itGroup != groupedDatablocks.end() )
    {
        for( const Ogre::String &datablockName : itGroup->second )
        {
            const wxString materialName = wxString::FromUTF8( datablockName );
            const wxString needle = materialName.Lower();

            bool bFound = true;
            while( tokenizer.HasMoreTokens() )
            {
                wxString tokenStr = tokenizer.GetNextToken();
                bFound &= needle.Find( tokenStr ) != wxNOT_FOUND;
            }
            tokenizer.Reinit( filterStr );

            if( bFound )
            {
                m_groupEntries.push_back( datablockName );
                entriesInList.push_back( materialName );
            }
        }
    }

    m_groupList->Set( entriesInList );
}
