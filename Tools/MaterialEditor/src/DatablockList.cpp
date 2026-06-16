#include "DatablockList.h"

#include "MainWindow.h"

#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgreRoot.h"

#include <wx/tokenzr.h>

DatablockList::DatablockList( MainWindow *parent ) :
    DatablockListBase( parent ),
    m_mainWindow( parent ),
    m_editing( false ),
    m_activeWouldBeFiltered( false )
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
    m_mainWindow->getUndoSystem().pushUndoStateMaterialSelect();
    m_mainWindow->setActiveDatablock( datablock, false );

    if( m_activeWouldBeFiltered )
        populateFromDatabase( false );

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

    m_activeWouldBeFiltered = false;

    Ogre::HlmsDatablock *activeDatablock = m_mainWindow->getActiveDatablock();
    int activeDatablockIdx = -1;

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
        if( ( hlmsTypes[i] == Ogre::HLMS_PBS && !bAllowPbs ) ||
            ( hlmsTypes[i] == Ogre::HLMS_UNLIT && !bAllowUnlit ) )
        {
            if( activeDatablock && activeDatablock->getCreator()->getType() == hlmsTypes[i] )
            {
                // Active datablock must always be shown (and selected!) regardless of filters.
                activeDatablockIdx = int( entriesInList.size() );
                entriesInList.push_back( *activeDatablock->getNameStr() );
                m_activeWouldBeFiltered = true;
            }
            continue;
        }

        const Ogre::Hlms::HlmsDatablockMap &datablockMap =
            hlmsManager->getHlms( hlmsTypes[i] )->getDatablockMap();

        for( const auto &keyVal : datablockMap )
        {
            const Ogre::Hlms::DatablockEntry &entry = keyVal.second;
            const bool bFilteredIn =
                !activeMeshOnly || meshDatablocks.find( entry.datablock ) != meshDatablocks.end();
            if( entry.visibleToManager && ( bFilteredIn || entry.datablock == activeDatablock ) )
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

                if( entry.datablock == activeDatablock )
                {
                    // Active datablock must always be shown (and selected!) regardless of filters.
                    activeDatablockIdx = int( entriesInList.size() );
                    if( !bFound || !bFilteredIn )
                        m_activeWouldBeFiltered = true;
                    bFound = true;
                }

                if( bFound )
                    entriesInList.push_back( materialName );
            }
        }
    }
    // TODO? Use wxTreeListCtrl / wxImageList to display preview thumbnails.
    m_datablockList->Set( entriesInList );
    m_datablockList->SetSelection( activeDatablockIdx );
}
//-----------------------------------------------------------------------------
void DatablockList::notifyDatablockSelectChanged()
{
    if( m_activeWouldBeFiltered )
    {
        // We need to rebuild the list (messes up scrolling though).
        populateFromDatabase( false );
    }
    else
    {
        Ogre::HlmsDatablock *activeDatablock = m_mainWindow->getActiveDatablock();
        if( !activeDatablock )
            m_datablockList->SetSelection( -1 );
        else
        {
            const int sel =
                m_datablockList->FindString( wxString::FromUTF8( *activeDatablock->getNameStr() ) );
            if( sel >= 0 )
                m_datablockList->SetSelection( sel );
            else
                populateFromDatabase( false );
        }
    }
}
