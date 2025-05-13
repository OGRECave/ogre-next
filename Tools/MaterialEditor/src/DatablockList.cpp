#include "DatablockList.h"

#include "MainWindow.h"

#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgreRoot.h"

#include <wx/tokenzr.h>

DatablockList::DatablockList( MainWindow *parent ) :
    DatablockListBase( parent ),
    m_mainWindow( parent ),
    m_editing( false )
{
    populateFromDatabase();
}
//-----------------------------------------------------------------------------
void DatablockList::OnDatablockSelect( wxCommandEvent &event )
{
    if( m_editing )
        return;
    EditingScope scope( m_editing );

    Ogre::Root *root = m_mainWindow->getRoot();
    Ogre::HlmsManager *hlmsManager = root->getHlmsManager();

    wxString materialName = m_datablockList->GetString( (unsigned int)m_datablockList->GetSelection() );

    Ogre::HlmsDatablock *datablock = hlmsManager->getDatablockNoDefault( materialName.utf8_string() );
    m_mainWindow->setActiveDatablock( datablock );
    event.Skip( false );
}
//-----------------------------------------------------------------------------
void DatablockList::OnCheckbox( wxCommandEvent &event )
{
    if( m_editing )
        return;
    EditingScope scope( m_editing );
    populateFromDatabase();
    event.Skip( false );
}
//-----------------------------------------------------------------------------
void DatablockList::OnSearchText( wxCommandEvent &event )
{
    if( m_editing )
        return;
    EditingScope scope( m_editing );
    populateFromDatabase();
    event.Skip( false );
}
//-----------------------------------------------------------------------------
void DatablockList::populateFromDatabase( bool bReasonActiveMeshChanged )
{
    const bool activeMeshOnly = m_activeMeshOnlyCheckbox->IsChecked();
    if( bReasonActiveMeshChanged && !activeMeshOnly )
        return;

    std::set<Ogre::HlmsDatablock *> meshDatablocks;
    const Ogre::MovableObject *movableObject = m_mainWindow->getActiveObject();
    if( movableObject && activeMeshOnly )
    {
        for( const Ogre::Renderable *renderable : movableObject->mRenderables )
            meshDatablocks.insert( renderable->getDatablock() );
    }

    Ogre::Root *root = m_mainWindow->getRoot();
    Ogre::HlmsManager *hlmsManager = root->getHlmsManager();

    const wxString filterStr = m_searchCtrl->GetValue().Lower();
    wxStringTokenizer tokenizer( filterStr, " " );

    const bool bAllowPbs = m_pbsCheckbox->IsChecked();
    const bool bAllowUnlit = m_unlitCheckbox->IsChecked();

    wxArrayString entriesInList;

    const Ogre::HlmsTypes hlmsTypes[] = { Ogre::HLMS_PBS, Ogre::HLMS_UNLIT };

    for( size_t i = 0u; i < sizeof( hlmsTypes ) / sizeof( hlmsTypes[0] ); ++i )
    {
        if( hlmsTypes[i] == Ogre::HLMS_PBS && !bAllowPbs )
            continue;
        if( hlmsTypes[i] == Ogre::HLMS_UNLIT && !bAllowUnlit )
            continue;

        const Ogre::Hlms::HlmsDatablockMap &datablockMap =
            hlmsManager->getHlms( hlmsTypes[i] )->getDatablockMap();

        for( const auto &keyVal : datablockMap )
        {
            const Ogre::Hlms::DatablockEntry &entry = keyVal.second;
            if( entry.visibleToManager &&
                ( !activeMeshOnly || meshDatablocks.find( entry.datablock ) != meshDatablocks.end() ) )
            {
                const wxString materialName = entry.name;
                const wxString needle = materialName.Lower();

                bool bFound = true;
                while( tokenizer.HasMoreTokens() )
                {
                    wxString tokenStr = tokenizer.GetNextToken();
                    bFound &= needle.Find( tokenStr ) != wxNOT_FOUND;
                }
                tokenizer.Reinit( filterStr );

                if( bFound )
                    entriesInList.push_back( materialName );
            }
        }
    }
    // TODO? Use wxTreeListCtrl / wxImageList to display preview thumbnails.
    m_datablockList->Set( entriesInList );
}
