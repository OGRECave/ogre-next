///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/frame.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/slider.h>
#include <wx/choice.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class MainWindowBase
///////////////////////////////////////////////////////////////////////////////
class MainWindowBase : public wxFrame
{
	private:

	protected:

	public:

		MainWindowBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("OgreNext Material Editor"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 1280,720 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

		~MainWindowBase();

};

///////////////////////////////////////////////////////////////////////////////
/// Class PbsParametersPanelBase
///////////////////////////////////////////////////////////////////////////////
class PbsParametersPanelBase : public wxPanel
{
	private:

	protected:
		wxTextCtrl* m_diffuseR;
		wxTextCtrl* m_diffuseG;
		wxTextCtrl* m_diffuseB;
		wxTextCtrl* m_diffuseRGBA;
		wxButton* m_buttonDiffuse;
		wxTextCtrl* m_specularR;
		wxTextCtrl* m_specularG;
		wxTextCtrl* m_specularB;
		wxTextCtrl* m_specularRGBA;
		wxButton* m_buttonSpecular;
		wxTextCtrl* m_fresnelR;
		wxTextCtrl* m_fresnelG;
		wxTextCtrl* m_fresnelB;
		wxTextCtrl* m_fresnelRGBA;
		wxButton* m_buttonFresnel;
		wxCheckBox* m_fresnelColouredCheckbox;
		wxSlider* m_fresnelSlider;
		wxSlider* m_roughnessSlider;
		wxTextCtrl* m_roughness;
		wxSlider* m_transparencySlider;
		wxTextCtrl* m_transparency;
		wxChoice* m_transparencyModeChoice;
		wxCheckBox* m_alphaFromTexCheckbox;
		wxCheckBox* m_alphaHashCheckbox;

		// Virtual event handlers, override them in your derived class
		virtual void OnColourText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnColourHtml( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnColourButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCheckbox( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSlider( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSlider( wxScrollEvent& event ) { event.Skip(); }
		virtual void OnSliderText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTransparencyMode( wxCommandEvent& event ) { event.Skip(); }


	public:

		PbsParametersPanelBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,535 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PbsParametersPanelBase();

};

