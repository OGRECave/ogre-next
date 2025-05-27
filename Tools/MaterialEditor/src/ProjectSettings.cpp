#include "ProjectSettings.h"

#include "LightPanel.h"

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
    m_groupedDatablocks.insert( { "EXCLUDED", {} } );
}
//-----------------------------------------------------------------------------
void ProjectSettings::generateDefaultSamplerTemplates()
{
    Ogre::HlmsSamplerblock samplerblock;
    samplerblock.mMaxAnisotropy = 16.0f;

    for( size_t i = Ogre::TFO_NONE; i <= Ogre::TFO_ANISOTROPIC; ++i )
    {
        samplerblock.setFiltering( Ogre::TextureFilterOptions( i ) );
        for( size_t j = 0u; j < 2u; ++j )
        {
            samplerblock.setAddressingMode( j == 0u ? Ogre::TAM_WRAP : Ogre::TAM_CLAMP );
            m_samplerTemplates.push_back(
                { NamedSampler::autogenNameFrom( samplerblock ), samplerblock } );
        }
    }
}
//-----------------------------------------------------------------------------
void ProjectSettings::OnResourcesAdd( wxCommandEvent &event )
{
    event.Skip();
    if( m_editing )
        return;
    EditingScope scope( m_editing );

    wxFileDialog openFileDialog( this, wxT( "Add resources.cfg" ), m_lastOpenDir, wxT( "" ),
                                 wxT( "*.cfg" ), wxFD_OPEN | wxFD_FILE_MUST_EXIST, wxDefaultPosition );

    if( openFileDialog.ShowModal() == wxID_OK )
    {
        const wxString fullpath = openFileDialog.GetPath();
        m_lastOpenDir = fullpath.SubString( 0u, fullpath.find_last_of( "/" ) );
        m_resourcesListBox->Append( fullpath );

        if( m_relativeFolderTextCtrl->IsEmpty() )
            m_relativeFolderTextCtrl->SetValue( m_lastOpenDir );
    }
}
//-----------------------------------------------------------------------------
void ProjectSettings::OnResourcesFolderAdd( wxCommandEvent &event )
{
    event.Skip();
    if( m_editing )
        return;
    EditingScope scope( m_editing );

    wxDirDialog dirPicker( this, wxT( "Add folder" ), m_lastOpenDir );

    if( dirPicker.ShowModal() == wxID_OK )
    {
        const wxString fullpath = dirPicker.GetPath();
        m_lastOpenDir = fullpath.SubString( 0u, fullpath.find_last_of( "/" ) );
        m_resourcesListBox->Append( fullpath );
    }
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
    event.Skip();
    syncSettingsFromUI();
}
//-----------------------------------------------------------------------------
bool ProjectSettings::validateSettings()
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
    return bValidSettings;
}
//-----------------------------------------------------------------------------
void ProjectSettings::syncSettingsFromUI()
{
    if( !validateSettings() )
        return;

    m_resources.clear();
    const unsigned int numItems = m_resourcesListBox->GetCount();
    m_resources.reserve( (size_t)numItems );
    for( unsigned int i = 0u; i < numItems; ++i )
        m_resources.push_back( m_resourcesListBox->GetString( i ) );

    m_relativeFolder = m_relativeFolderTextCtrl->GetValue();
    m_materialDstPath = m_materialFileLocationTextCtrl->GetValue();
    if( m_materialDstPath.EndsWith( ".material.json" ) )
        m_materialDstPath.resize( m_materialDstPath.size() - ( sizeof( ".material.json" ) - 1u ) );
    m_projectPath = m_projectFileLocationTextCtrl->GetValue();
}
//-----------------------------------------------------------------------------
void ProjectSettings::loadProject( Ogre::HlmsManager *hlmsManager )
{
    m_samplerTemplates.clear();
    generateDefaultSamplerTemplates();

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
                        m_resourceLocations.push_back( { relativeFolder + archName, secName } );
                    }
                }
            }
        }
        else
        {
            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                resource.utf8_string(), "FileSystem",
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
            m_resourceLocations.push_back(
                { resource.utf8_string(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME } );
        }
    }

    const char *suffix[] = { "_pbs.material.json", "_unlit.material.json" };
    for( size_t i = 0u; i < 2u; ++i )
    {
        const Ogre::String materialFileLocation = m_materialDstPath.utf8_string() + suffix[i];
        std::ifstream matFile;
        matFile.open( materialFileLocation, std::ios::binary | std::ios::in );

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
}
//-----------------------------------------------------------------------------
void ProjectSettings::saveProject( Ogre::HlmsManager *hlmsManager, LightPanel &lightPanel )
{
    Ogre::String jsonString;

    jsonString += "{";
    jsonString += "\n	\"relative_folder\" : \"" + m_relativeFolder;
    jsonString += "\",\n	\"material_dst_path\" : \"" + m_materialDstPath;
    jsonString +=
        "\",\n	\"resources\" :"
        "\n	[";
    for( const wxString &resource : m_resources )
    {
        jsonString += "\n		\"" + resource.utf8_string();
        jsonString += "\",";
    }
    if( !m_resources.empty() )
        jsonString.pop_back();  // Remove last trailing comma.
    jsonString += "\n	]";

    jsonString += ",\n	\"category_groups\" :\n	{";
    for( const auto &groups : m_groupedDatablocks )
    {
        jsonString += "\n		\"";
        jsonString += groups.first;
        jsonString += "\" : [";
        for( const Ogre::String &datablockName : groups.second )
        {
            jsonString += "\"";
            jsonString += datablockName;
            jsonString += "\", ";
        }
        if( !groups.second.empty() )
        {
            // Remove last trailing comma & space.
            jsonString.pop_back();
            jsonString.pop_back();
        }
        jsonString += "],";
    }
    if( !m_groupedDatablocks.empty() )
        jsonString.pop_back();  // Remove last trailing comma.
    jsonString += "\n	}";

    lightPanel.saveProject( jsonString );

    jsonString += "\n}";

    std::ofstream projFile;
    projFile.open( m_projectPath.utf8_string(), std::ios::binary | std::ios::out );

    if( !projFile.is_open() )
    {
        wxMessageBox( wxString::Format( wxT( "Failed to save to '%s'." ), m_projectPath ),
                      wxT( "Error Saving File" ), wxOK | wxICON_ERROR | wxCENTRE );
        return;
    }

    projFile.write( jsonString.data(), std::streamsize( jsonString.size() ) );

    hlmsManager->saveMaterials(  //
        Ogre::HLMS_PBS, m_materialDstPath.utf8_string() + "_pbs.material.json", this, "", true );
    hlmsManager->saveMaterials(
        Ogre::HLMS_UNLIT, m_materialDstPath.utf8_string() + "_unlit.material.json", this, "", true );
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
    m_groupedDatablocks.insert( { "EXCLUDED", {} } );

    Ogre::v1::MeshManager::getSingleton().removeAll();
    Ogre::MeshManager::getSingleton().removeAll();

    Ogre::ResourceGroupManager &resourceGroupManager = Ogre::ResourceGroupManager::getSingleton();
    for( const ResourceLocation &location : m_resourceLocations )
        resourceGroupManager.removeResourceLocation( location.name, location.group );

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

    loadProject( hlmsManager );
}
//-----------------------------------------------------------------------------
void ProjectSettings::openProject( wxString projectPath, Ogre::HlmsManager *hlmsManager,
                                   LightPanel &lightPanel )
{
    projectPath.Replace( "\\", "/" );
    m_projectPath = projectPath;
    m_lastOpenDir = projectPath.SubString( 0u, projectPath.find_last_of( "/" ) );

    std::ifstream projFile;
    projFile.open( projectPath.utf8_string(), std::ios::binary | std::ios::in );

    if( !projFile.is_open() )
    {
        wxMessageBox( wxString::Format( wxT( "Failed to open '%s'." ), projectPath ),
                      wxT( "Error Loading File" ), wxOK | wxICON_ERROR | wxCENTRE );
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
        wxMessageBox(
            wxString::Format( wxT( "Error parsing '%s'.\nError: %s at line %zu" ), projectPath,
                              rapidjson::GetParseError_En( d.GetParseError() ), d.GetErrorOffset() ),
            wxT( "JSON Parse Error" ), wxOK | wxICON_ERROR | wxCENTRE );
        return;
    }

    m_resources.Clear();
    m_relativeFolder.clear();
    m_materialDstPath.clear();
    m_groupedDatablocks.clear();

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

    itor = d.FindMember( "category_groups" );
    if( itor != d.MemberEnd() && itor->value.IsObject() )
    {
        const rapidjson::Value &groups = itor->value;

        itor = groups.MemberBegin();
        rapidjson::Value::ConstMemberIterator endt = groups.MemberEnd();

        while( itor != endt )
        {
            if( itor->name.IsString() && itor->value.IsArray() )
            {
                std::set<Ogre::String> &datablocksInGroup =
                    m_groupedDatablocks.insert( { itor->name.GetString(), {} } ).first->second;
                const rapidjson::SizeType numEntries = itor->value.Size();
                for( rapidjson::SizeType i = 0u; i < numEntries; ++i )
                {
                    if( itor->value[i].IsString() )
                        datablocksInGroup.insert( itor->value[i].GetString() );
                }
            }
            ++itor;
        }
    }

    lightPanel.loadProject( d );

    newProject( hlmsManager );
}
//-----------------------------------------------------------------------------
wxString ProjectSettings::openProjectModal()
{
    wxFileDialog openFileDialog( this, wxT( "Open Project" ), m_lastOpenDir, wxT( "" ),
                                 wxT( "*.matproj.json" ), wxFD_OPEN | wxFD_FILE_MUST_EXIST,
                                 wxDefaultPosition );

    if( openFileDialog.ShowModal() == wxID_OK )
        return openFileDialog.GetPath();
    return "";
}
//-----------------------------------------------------------------------------
void ProjectSettings::loadInternalSettings( const std::string &rwFolder )
{
    const std::string fullpath = rwFolder + "ProjectSettings.json";
    std::ifstream file;
    file.open( fullpath, std::ios::binary | std::ios::in );

    if( !file.is_open() )
    {
        fprintf( stderr, "Could not load settings from '%s'", fullpath.c_str() );
        return;
    }

    std::vector<char> jsonString;
    file.seekg( 0, std::ios_base::end );
    const size_t fileSize = (size_t)file.tellg();
    file.seekg( 0, std::ios_base::beg );
    jsonString.resize( fileSize + 1u );
    file.read( jsonString.data(), (std::streamsize)fileSize );
    if( jsonString[fileSize] != '\0' )
        jsonString[fileSize] = '\0';  // Make sure it's null-terminated.

    rapidjson::Document d;
    d.Parse( jsonString.data() );

    if( d.HasParseError() )
    {
        wxMessageBox( wxString::Format( wxT( "Error parsing '%s'.\nError: %s at line %i" ), fullpath,
                                        rapidjson::GetParseError_En( d.GetParseError() ),
                                        std::to_string( d.GetErrorOffset() ) ),
                      wxT( "JSON Parse Error" ), wxOK | wxICON_ERROR | wxCENTRE );
        return;
    }

    rapidjson::Value::ConstMemberIterator itor;
    itor = d.FindMember( "last_open_dir" );
    if( itor != d.MemberEnd() && itor->value.IsString() )
        m_lastOpenDir = wxString::FromUTF8( itor->value.GetString() );
}
//-----------------------------------------------------------------------------
void ProjectSettings::saveInternalSettings( const std::string &rwFolder )
{
    Ogre::String jsonString;

    jsonString += "{";
    jsonString += "\n	\"last_open_dir\" : \"" + m_lastOpenDir;
    jsonString += "\"\n}";

    std::ofstream file;
    file.open( rwFolder + "ProjectSettings.json", std::ios::binary | std::ios::out );

    if( !file.is_open() )
    {
        wxMessageBox( wxString::Format( wxT( "Could not write internal settings to '%s'" ),
                                        ( rwFolder + "ProjectSettings.json" ) ),
                      wxT( "Internal Error" ), wxOK | wxICON_ERROR | wxCENTRE );
        return;
    }

    file.write( jsonString.data(), std::streamsize( jsonString.size() ) );
}
//-----------------------------------------------------------------------------
void ProjectSettings::addToCategoryGroup( const Ogre::String &groupName,
                                          const Ogre::String &datablockName )
{
    m_groupedDatablocks[groupName].insert( datablockName );
}
//-----------------------------------------------------------------------------
void ProjectSettings::removeFromCategoryGroup( const Ogre::String &groupName,
                                               const Ogre::String &datablockName )
{
    m_groupedDatablocks[groupName].erase( datablockName );
}
//-----------------------------------------------------------------------------
bool ProjectSettings::isDatablockGroupless( const Ogre::String &name ) const
{
    for( const auto &group : m_groupedDatablocks )
    {
        if( group.second.find( name ) != group.second.end() )
            return false;
    }
    return true;
}
//-----------------------------------------------------------------------------
bool ProjectSettings::canSaveDatablock( const Ogre::HlmsDatablock *datablock ) const
{
    const Ogre::String *name = datablock->getNameStr();

    if( m_currentGroupSaving.empty() )
    {
        if( !name )
            return true;
        return isDatablockGroupless( *datablock->getNameStr() );
    }

    if( !name )
        return false;

    GroupedDatablockMap::const_iterator itor = m_groupedDatablocks.find( m_currentGroupSaving );
    if( itor == m_groupedDatablocks.end() )
        return false;

    return itor->second.find( *name ) != itor->second.end();
}
