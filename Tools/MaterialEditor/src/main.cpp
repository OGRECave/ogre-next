#define __WXWIDGETSAPP_H

#include <wx/cmdline.h>  //for wxCmdLineParser
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/wx.h>

#include "MainWindow.h"

#include "CmdSettings.h"

class wxWidgetsApp : public wxApp
{
    CmdSettings parseCmdLine() const;

    void setWorkingDirectory();

public:
    wxWidgetsApp();
    ~wxWidgetsApp() override;
    bool OnInit() override;
};

DECLARE_APP( wxWidgetsApp )
IMPLEMENT_APP( wxWidgetsApp )

wxWidgetsApp::wxWidgetsApp()
{
}

wxWidgetsApp::~wxWidgetsApp()
{
}

bool wxWidgetsApp::OnInit()
{
    const CmdSettings cmdSettings = parseCmdLine();
    setWorkingDirectory();

    MainWindow *window = new MainWindow( 0, cmdSettings );
    window->Show();
    SetTopWindow( window );
    return true;
}

// Returns True if we shouldn't close
CmdSettings wxWidgetsApp::parseCmdLine() const
{
    CmdSettings retVal;
    retVal.resoucesCfgPath = wxT( "" );
    retVal.setupRenderSystems = false;

    // argc, argv
    wxCmdLineEntryDesc cmdLineDesc[3];
    cmdLineDesc[0].kind = wxCMD_LINE_SWITCH;
    cmdLineDesc[0].shortName = "setup";
    cmdLineDesc[0].longName = NULL;
    cmdLineDesc[0].description = "Configure OGRE's RenderSystem (D3D, OGL, Vulkan).";
    cmdLineDesc[0].type = wxCMD_LINE_VAL_STRING;
    cmdLineDesc[0].flags = wxCMD_LINE_PARAM_OPTIONAL;

    cmdLineDesc[1].kind = wxCMD_LINE_OPTION;
    cmdLineDesc[1].shortName = "r";
    cmdLineDesc[1].longName = "resources";
    cmdLineDesc[1].description = "Specify resources.cfg to load on startup.";
    cmdLineDesc[1].type = wxCMD_LINE_VAL_STRING;
    cmdLineDesc[1].flags = wxCMD_LINE_PARAM_OPTIONAL;

    cmdLineDesc[2].kind = wxCMD_LINE_NONE;

    // gets the passed media files from cmd line
    wxCmdLineParser parser( cmdLineDesc, argc, argv );

    if( !parser.Parse() )
    {
        wxString value;

        if( parser.Found( wxT( "setup" ) ) )
            retVal.setupRenderSystems = true;

        if( parser.Found( wxT( "resources" ), &value ) )
            retVal.resoucesCfgPath = value;
    }

    return retVal;
}

// Sets the working directory to the local path of the executable, so that loading
// plugins.cfg & co. doesn't fail.
void wxWidgetsApp::setWorkingDirectory()
{
    wxFileName fullAppPath( wxStandardPaths::Get().GetExecutablePath() );
#ifdef __WXMSW__
    SetCurrentDirectory( fullAppPath.GetPath() );
#else
    // This ought to work in Unix, but I didn't test it, otherwise try setenv().
    chdir( fullAppPath.GetPath().mb_str() );

    // Most Ogre materials assume floating point to use radix point, not comma.
    // Prevent awfull number truncation in non-US systems
    //(I love standarization...).
    setlocale( LC_NUMERIC, "C" );
#endif
}
