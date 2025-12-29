#include "MeshList.h"

#include "MainWindow.h"

#include "OgreMesh2.h"
#include "OgreMeshManager2.h"

#include <wx/tokenzr.h>

MeshList::MeshList( MainWindow *parent ) :
    MeshListBase( parent ),
    m_mainWindow( parent ),
    m_editing( false ),
    m_activeWouldBeFiltered( false )
{
    populateFromDatabase();
}
//-----------------------------------------------------------------------------
void MeshList::OnMeshSelect( wxCommandEvent &event )
{
    if( m_editing )
        return;
    EditingScope scope( m_editing );

    const size_t idx = size_t( m_meshList->GetSelection() );

    if( idx < m_meshes.size() )
    {
        m_mainWindow->getUndoSystem().pushUndoMeshSelect();
        m_mainWindow->setActiveMesh( m_meshes[idx].name, m_meshes[idx].resourceGroup, false );

        if( m_activeWouldBeFiltered )
            populateFromDatabase();
    }

    event.Skip();
}
//-----------------------------------------------------------------------------
void MeshList::OnSearchText( wxCommandEvent &event )
{
    if( m_editing )
        return;
    EditingScope scope( m_editing );
    populateFromDatabase();
    event.Skip( false );
}
//-----------------------------------------------------------------------------
void MeshList::populateFromDatabase()
{
    m_activeWouldBeFiltered = false;
    const MeshEntry activeMesh = m_mainWindow->getActiveMeshName();
    int activeMeshIdx = -1;

    const wxString filterStr = m_searchCtrl->GetValue().Lower();
    wxStringTokenizer tokenizer( filterStr, " " );

    Ogre::ResourceGroupManager &resourceGroupMgr = Ogre::ResourceGroupManager::getSingleton();
    const Ogre::StringVector groups = resourceGroupMgr.getResourceGroups();

    m_meshes.clear();
    wxArrayString entriesInList;

    for( const Ogre::String &groupName : groups )
    {
        const Ogre::FileInfoListPtr fileInfoList =
            resourceGroupMgr.findResourceFileInfo( groupName, "*.mesh" );

        for( const Ogre::FileInfo &fileInfo : *fileInfoList )
        {
            const wxString meshName = groupName + "/" + fileInfo.filename;
            const wxString needle = meshName.Lower();

            bool bFound = true;
            while( tokenizer.HasMoreTokens() )
            {
                wxString tokenStr = tokenizer.GetNextToken();
                bFound &= needle.Find( tokenStr ) != wxNOT_FOUND;
            }
            tokenizer.Reinit( filterStr );

            if( groupName == activeMesh.resourceGroup && fileInfo.filename == activeMesh.name )
            {
                // Active mesh must always be shown (and selected!) regardless of filters.
                activeMeshIdx = int( entriesInList.size() );
                if( !bFound )
                    m_activeWouldBeFiltered = true;
                bFound = true;
            }

            if( bFound )
            {
                m_meshes.push_back( { fileInfo.filename, groupName } );
                entriesInList.push_back( meshName );
            }
        }
    }

    // TODO? Use wxTreeListCtrl / wxImageList to display preview thumbnails.
    m_meshList->Set( entriesInList );
    m_meshList->SetSelection( activeMeshIdx );
}
//-----------------------------------------------------------------------------
void MeshList::notifyMeshSelectChanged()
{
    if( m_activeWouldBeFiltered )
    {
        // We need to rebuild the list (messes up scrolling though).
        populateFromDatabase();
    }
    else
    {
        if( !m_mainWindow->getActiveObject() )
            m_meshList->SetSelection( -1 );
        else
        {
            MeshEntry meshEntry = m_mainWindow->getActiveMeshName();

            std::vector<MeshEntry>::const_iterator itor =
                std::find( m_meshes.begin(), m_meshes.end(), meshEntry );
            if( itor != m_meshes.end() )
                m_meshList->SetSelection( int( itor - m_meshes.begin() ) );
            else
                populateFromDatabase();
        }
    }
}
