#include "TextureSelect.h"

#include "OgreCodec.h"
#include "OgreResourceGroupManager.h"

#include <wx/tokenzr.h>

TextureSelect::TextureSelect( wxWindow *parent ) :
    TextureSelectBase( parent ),
    m_editing( false ),
    m_chosenEntry( UINT32_MAX )
{
    populateFromDatabase();
}
//-----------------------------------------------------------------------------
void TextureSelect::saveSelection()
{
    const uint32_t sel = (uint32_t)this->m_textureList->GetSelection();
    if( sel < m_resources.size() )
        m_chosenEntry = sel;
}
//-----------------------------------------------------------------------------
void TextureSelect::OnTextureSelect( wxCommandEvent &event )
{
    if( m_editing )
        return;
    EditingScope scope( m_editing );

    saveSelection();
    this->EndModal( wxID_OK );
    event.Skip();
}
//-----------------------------------------------------------------------------
void TextureSelect::OnSearchText( wxCommandEvent &event )
{
    if( m_editing )
        return;
    EditingScope scope( m_editing );
    populateFromDatabase();
    event.Skip();
}
//-----------------------------------------------------------------------------
void TextureSelect::OnButtonClick( wxCommandEvent &event )
{
    if( m_editing )
        return;
    EditingScope scope( m_editing );

    if( event.GetEventObject() == m_okButton )
        saveSelection();
    else
        m_chosenEntry = UINT32_MAX;
    this->EndModal( wxID_OK );
    event.Skip();
}
//-----------------------------------------------------------------------------
void TextureSelect::populateFromDatabase()
{
    const Ogre::StringVector extensions = Ogre::Codec::getExtensions();

    const wxString filterStr = m_searchCtrl->GetValue().Lower();
    wxStringTokenizer tokenizer( filterStr, " " );

    Ogre::ResourceGroupManager &resourceGroupMgr = Ogre::ResourceGroupManager::getSingleton();
    const Ogre::StringVector groups = resourceGroupMgr.getResourceGroups();

    m_resources.clear();
    wxArrayString entriesInList;

    {
        m_resources.push_back( { "", "" } );
        entriesInList.push_back( wxT( "[Remove Texture]" ) );
    }

    for( const Ogre::String &groupName : groups )
    {
        for( const Ogre::String &ext : extensions )
        {
            const Ogre::FileInfoListPtr fileInfoList =
                resourceGroupMgr.findResourceFileInfo( groupName, "*." + ext );

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

                if( bFound )
                {
                    m_resources.push_back( { fileInfo.filename, groupName } );
                    entriesInList.push_back( meshName );
                }
            }
        }
    }

    // TODO? Use wxTreeListCtrl / wxImageList to display preview thumbnails.
    m_textureList->Set( entriesInList );
}
//-----------------------------------------------------------------------------
const TextureSelect::ResourceEntry *TextureSelect::getChosenEntry() const
{
    if( m_chosenEntry < m_resources.size() )
        return &m_resources[m_chosenEntry];
    return 0;
}
