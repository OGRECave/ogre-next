#include "ProjectSettings.h"

#include "OgreConfigFile.h"
#include "OgreHlms.h"
#include "OgreHlmsJson.h"
#include "OgreHlmsManager.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreResourceGroupManager.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "wx/dirdlg.h"
#include "wx/filedlg.h"
#include "wx/msgdlg.h"

#include <fstream>

ProjectSettings::ProjectSettings( wxWindow *parent ) : ProjectSettingsBase( parent ), m_editing( false )
{
}
//-----------------------------------------------------------------------------
void ProjectSettings::OnResourcesAdd( wxCommandEvent &event )
{
    // TODO: Implement OnResourcesAdd
    event.Skip();
    if( m_editing )
        return;
    EditingScope scope( m_editing );
}
//-----------------------------------------------------------------------------
void ProjectSettings::OnResourcesFolderAdd( wxCommandEvent &event )
{
    // TODO: Implement OnResourcesFolderAdd
    event.Skip();
    if( m_editing )
        return;
    EditingScope scope( m_editing );
}
//-----------------------------------------------------------------------------
void ProjectSettings::OnResourcesRemove( wxCommandEvent &event )
{
    // TODO: Implement OnResourcesRemove
    event.Skip();
    if( m_editing )
        return;
    EditingScope scope( m_editing );
}
//-----------------------------------------------------------------------------
void ProjectSettings::OnText( wxCommandEvent &event )
{
    event.Skip();
    if( m_editing )
        return;
    EditingScope scope( m_editing );

    validateSettings();
}
//-----------------------------------------------------------------------------
void ProjectSettings::OnBrowseFolder( wxCommandEvent &event )
{
    event.Skip();

    if( m_editing )
        return;
    EditingScope scope( m_editing );

    wxDirDialog dirPicker( this, wxT( "Pick Relative folder for resources.cfg" ), m_lastOpenDir );

    if( dirPicker.ShowModal() == wxID_OK )
    {
        if( m_relativeFolderBrowseBtn == event.GetEventObject() )
            m_relativeFolderTextCtrl->SetValue( dirPicker.GetPath() );
    }
}
//-----------------------------------------------------------------------------
void ProjectSettings::OnBrowse( wxCommandEvent &event )
{
    event.Skip();
    if( m_editing )
        return;
    EditingScope scope( m_editing );

    wxString title;
    wxString extension;
    if( m_materialFileBrowseBtn == event.GetEventObject() )
    {
        title = wxT( "Material file save location..." );
        extension = wxT( ".material.json" );
    }
    else
    {
        title = wxT( "Project save location..." );
        extension = wxT( ".matproj.json" );
    }

    wxFileDialog openFileDialog( this, title, m_lastOpenDir, wxT( "" ), wxT( "*" ) + extension,
                                 wxFD_SAVE | wxFD_OVERWRITE_PROMPT, wxDefaultPosition );

    if( openFileDialog.ShowModal() == wxID_OK )
    {
        if( m_materialFileBrowseBtn == event.GetEventObject() )
        {
            wxString path = openFileDialog.GetPath();
            if( !path.EndsWith( extension ) )
                path += extension;
            m_materialFileLocationTextCtrl->SetValue( path );
        }
        else
        {
            wxString path = openFileDialog.GetPath();
            if( !path.EndsWith( extension ) )
                path += extension;
            m_projectFileLocationTextCtrl->SetValue( path );
        }
    }

    validateSettings();
}
//-----------------------------------------------------------------------------
void ProjectSettings::OnButtonClick( wxCommandEvent &event )
{
    // TODO: Implement OnButtonClick
    event.Skip();
}
//-----------------------------------------------------------------------------
void ProjectSettings::validateSettings()
{
    bool bValidSettings = true;

    if( m_projectFileLocationTextCtrl->IsEmpty() )
    {
        m_projectFileLocationTextCtrl->SetBackgroundColour( wxColour( 127u, 0, 0 ) );
        bValidSettings = false;
    }
    else
        m_projectFileLocationTextCtrl->SetBackgroundColour( wxNullColour );

    if( m_materialFileLocationTextCtrl->IsEmpty() )
    {
        m_materialFileLocationTextCtrl->SetBackgroundColour( wxColour( 127u, 0, 0 ) );
        bValidSettings = false;
    }
    else
        m_materialFileLocationTextCtrl->SetBackgroundColour( wxNullColour );

    m_okButton->Enable( bValidSettings );
}
//-----------------------------------------------------------------------------
void ProjectSettings::loadProject( Ogre::HlmsManager *hlmsManager )
{
    const Ogre::String relativeFolder = m_relativeFolder.utf8_string();

    for( const wxString &resource : m_resources )
    {
        if( resource.EndsWith( wxT( ".cfg" ) ) )
        {
            // This is a resources.cfg file.
            Ogre::ConfigFile cf;
            cf.load( resource.utf8_string() );

            Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

            Ogre::String secName, typeName, archName;
            while( seci.hasMoreElements() )
            {
                secName = seci.peekNextKey();
                Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();

                if( secName != "Hlms" )
                {
                    Ogre::ConfigFile::SettingsMultiMap::iterator i;
                    for( i = settings->begin(); i != settings->end(); ++i )
                    {
                        typeName = i->first;
                        archName = i->second;

                        if( !archName.empty() && archName.front() == '/' )
                            archName.erase( 0u );

                        Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                            relativeFolder + archName, typeName, secName );
                    }
                }
            }
        }
        else
        {
            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                resource.utf8_string(), "FileSystem",
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
        }
    }

    const Ogre::String materialFileLocation = m_materialDstPath.utf8_string();
    std::ifstream matFile;
    matFile.open( materialFileLocation, std::ios::in | std::ios::binary );

    if( matFile.is_open() )
    {
        std::vector<char> jsonString;
        matFile.seekg( 0, std::ios_base::end );
        const size_t fileSize = (size_t)matFile.tellg();
        matFile.seekg( 0, std::ios_base::beg );
        jsonString.resize( fileSize + 1u );
        matFile.read( jsonString.data(), (std::streamsize)fileSize );
        if( jsonString[fileSize] != '\0' )
            jsonString[fileSize] = '\0';  // Make sure it's null-terminated.

        Ogre::HlmsJson hlmsJson( hlmsManager, nullptr );
        hlmsJson.loadMaterials( materialFileLocation,
                                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                jsonString.data(), "" );
    }
}
//-----------------------------------------------------------------------------
int ProjectSettings::ShowModal()
{
    m_resourcesListBox->Set( m_resources );
    m_relativeFolderTextCtrl->SetValue( m_relativeFolder );
    m_materialFileLocationTextCtrl->SetValue( m_materialDstPath );
    m_projectFileLocationTextCtrl->SetValue( m_projectPath );

    validateSettings();
    return ProjectSettingsBase::ShowModal();
}
//-----------------------------------------------------------------------------
void ProjectSettings::newProject( Ogre::HlmsManager *hlmsManager )
{
    Ogre::ResourceGroupManager &resourceGroupManager = Ogre::ResourceGroupManager::getSingleton();
    const Ogre::StringVector resourceGroups = resourceGroupManager.getResourceGroups();
    for( const Ogre::String &resourceGroup : resourceGroups )
        resourceGroupManager.destroyResourceGroup( resourceGroup );

    for( size_t i = 0u; i < Ogre::HlmsTypes::HLMS_MAX; ++i )
    {
        if( i == Ogre::HlmsTypes::HLMS_PBS || i == Ogre::HlmsTypes::HLMS_UNLIT )
        {
            Ogre::Hlms *hlms = hlmsManager->getHlms( static_cast<Ogre::HlmsTypes>( i ) );
            const Ogre::Hlms::HlmsDatablockMap datablockMap = hlms->getDatablockMap();  // Hard copy.
            for( const auto &keyVal : datablockMap )
            {
                const Ogre::Hlms::DatablockEntry &entry = keyVal.second;
                if( entry.visibleToManager )
                    hlms->destroyDatablock( keyVal.first );
            }
        }
    }

    Ogre::v1::MeshManager::getSingleton().removeAll();
    Ogre::MeshManager::getSingleton().removeAll();
}
//-----------------------------------------------------------------------------
void ProjectSettings::openProject( wxString projectPath, Ogre::HlmsManager *hlmsManager )
{
    projectPath.Replace( "\\", "/" );
    m_projectPath = projectPath;
    m_lastOpenDir = projectPath.SubString( 0u, projectPath.find_last_of( "/" ) );

    std::ifstream projFile;
    projFile.open( projectPath.utf8_string(), std::ios::in | std::ios::binary );

    if( !projFile.is_open() )
    {
        wxMessageBox( wxString::Format( wxT( "Failed to open '%s'." ), projectPath ),
                      wxT( "Error Loading File" ) );
        return;
    }

    std::vector<char> jsonString;
    projFile.seekg( 0, std::ios_base::end );
    const size_t fileSize = (size_t)projFile.tellg();
    projFile.seekg( 0, std::ios_base::beg );
    jsonString.resize( fileSize + 1u );
    projFile.read( jsonString.data(), (std::streamsize)fileSize );
    if( jsonString[fileSize] != '\0' )
        jsonString[fileSize] = '\0';  // Make sure it's null-terminated.

    rapidjson::Document d;
    d.Parse( jsonString.data() );

    if( d.HasParseError() )
    {
        wxMessageBox( wxString::Format( wxT( "Error parsing '%s'.\nError: %s at line %i" ), projectPath,
                                        rapidjson::GetParseError_En( d.GetParseError() ),
                                        std::to_string( d.GetErrorOffset() ) ),
                      wxT( "JSON Parse Error" ) );
        return;
    }

    m_resources.Clear();
    m_relativeFolder.clear();
    m_materialDstPath.clear();

    rapidjson::Value::ConstMemberIterator itor;
    itor = d.FindMember( "resources" );
    if( itor != d.MemberEnd() && itor->value.IsArray() )
    {
        const rapidjson::Value &subobj = itor->value;

        const rapidjson::SizeType numEntries = subobj.Size();
        m_resources.reserve( numEntries );

        for( rapidjson::SizeType i = 0u; i < numEntries; ++i )
        {
            if( subobj[i].IsString() )
            {
                m_resources.push_back( subobj[i].GetString() );
                m_resources.back().Replace( "\\", "/" );
            }
        }
    }

    itor = d.FindMember( "relative_folder" );
    if( itor != d.MemberEnd() && itor->value.IsString() )
        m_relativeFolder = itor->value.GetString();

    // Make relativeFolder always end in "/".
    m_relativeFolder.Replace( "\\", "/" );
    if( !m_relativeFolder.empty() && !m_relativeFolder.ends_with( '/' ) )
        m_relativeFolder += "/";

    itor = d.FindMember( "material_dst_path" );
    if( itor != d.MemberEnd() && itor->value.IsString() )
        m_materialDstPath = itor->value.GetString();

    loadProject( hlmsManager );
}
//-----------------------------------------------------------------------------
void ProjectSettings::openProject( Ogre::HlmsManager *hlmsManager )
{
    validateSettings();

    wxFileDialog openFileDialog( this, wxT( "Open Project" ), m_lastOpenDir, wxT( "" ),
                                 wxT( "*.matproj.json" ), wxFD_OPEN | wxFD_FILE_MUST_EXIST,
                                 wxDefaultPosition );

    if( openFileDialog.ShowModal() == wxID_OK )
        openProject( openFileDialog.GetPath(), hlmsManager );
}
