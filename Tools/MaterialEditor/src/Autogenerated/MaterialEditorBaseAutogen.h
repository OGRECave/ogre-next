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
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/checkbox.h>
#include <wx/slider.h>
#include <wx/listbox.h>
#include <wx/panel.h>
#include <wx/srchctrl.h>

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
		wxChoice* m_workflowChoice;
		wxChoice* m_brdfChoice;
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
		wxStaticBoxSizer* m_fresnelSizer;
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
		wxListBox* m_submeshListBox;

		// Virtual event handlers, override them in your derived class
		virtual void OnWorkflowChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSettingDirty( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnColourText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnColourHtml( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnColourButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCheckbox( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSlider( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSlider( wxScrollEvent& event ) { event.Skip(); }
		virtual void OnSliderText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSubMeshApply( wxCommandEvent& event ) { event.Skip(); }


	public:

		PbsParametersPanelBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,779 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PbsParametersPanelBase();

};

///////////////////////////////////////////////////////////////////////////////
/// Class DatablockListBase
///////////////////////////////////////////////////////////////////////////////
class DatablockListBase : public wxPanel
{
	private:

	protected:
		wxListBox* m_datablockList;
		wxCheckBox* m_pbsCheckbox;
		wxCheckBox* m_unlitCheckbox;
		wxSearchCtrl* m_searchCtrl;

		// Virtual event handlers, override them in your derived class
		virtual void OnDatablockSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCheckbox( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSearchText( wxCommandEvent& event ) { event.Skip(); }


	public:

		DatablockListBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~DatablockListBase();

};

///////////////////////////////////////////////////////////////////////////////
/// Class MeshListBase
///////////////////////////////////////////////////////////////////////////////
class MeshListBase : public wxPanel
{
	private:

	protected:
		wxListBox* m_meshList;
		wxSearchCtrl* m_searchCtrl;

		// Virtual event handlers, override them in your derived class
		virtual void OnMeshSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSearchText( wxCommandEvent& event ) { event.Skip(); }


	public:

		MeshListBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~MeshListBase();

};

