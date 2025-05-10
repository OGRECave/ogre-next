#include "MeshList.h"

#include "MainWindow.h"

#include "OgreMesh2.h"
#include "OgreMeshManager2.h"

#include <wx/tokenzr.h>

MeshList::MeshList( MainWindow *parent ) :
    MeshListBase( parent ),
    m_mainWindow( parent ),
    m_editing( false )
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
        m_mainWindow->setActiveMesh( m_meshes[idx].name, m_meshes[idx].resourceGroup );
        event.Skip( false );
    }
    else
    {
        event.Skip();
    }
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
    const wxString filterStr = m_searchCtrl->GetValue();
    wxStringTokenizer tokenizer( filterStr, " " );

    Ogre::ResourceGroupManager &resourceGroupMgr = Ogre::ResourceGroupManager::getSingleton();
    const Ogre::StringVector groups = resourceGroupMgr.getResourceGroups();

    wxArrayString entriesInList;

    for( const Ogre::String &groupName : groups )
    {
        const Ogre::FileInfoListPtr fileInfoList =
            resourceGroupMgr.findResourceFileInfo( groupName, "*.mesh" );

        for( const Ogre::FileInfo &fileInfo : *fileInfoList )
        {
            wxString meshName = groupName + "/" + fileInfo.filename;

            bool bFound = true;
            while( tokenizer.HasMoreTokens() )
            {
                wxString tokenStr = tokenizer.GetNextToken();
                bFound &= meshName.Find( tokenStr ) != wxNOT_FOUND;
            }
            tokenizer.Reinit( filterStr );

            if( bFound )
            {
                m_meshes.push_back( { fileInfo.filename, groupName } );
                entriesInList.push_back( meshName );
            }
        }
    }

    // TODO? Use wxTreeListCtrl / wxImageList to display preview thumbnails.
    m_meshList->Set( entriesInList );
}
