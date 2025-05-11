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
#include <wx/spinctrl.h>
#include <wx/scrolwin.h>
#include <wx/srchctrl.h>
#include <wx/dialog.h>

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
/// Class PbsTexturePanelBase
///////////////////////////////////////////////////////////////////////////////
class PbsTexturePanelBase : public wxPanel
{
	private:

	protected:
		wxScrolledWindow* m_scrolledWindow;
		wxButton* m_diffuseMapBtn;
		wxSpinCtrl* m_diffuseMapSpin;
		wxButton* m_normalMapBtn;
		wxSpinCtrl* m_normalMapSpin;
		wxSlider* m_normalMapSlider;
		wxTextCtrl* m_normalMapTextCtrl;
		wxButton* m_specularMapBtn;
		wxSpinCtrl* m_specularMapSpin;
		wxButton* m_roughnessMapBtn;
		wxSpinCtrl* m_roughnessMapSpin;
		wxButton* m_emissiveMapBtn;
		wxSpinCtrl* m_emissiveMapSpin;
		wxButton* m_detailWeightMapBtn;
		wxSpinCtrl* m_detailWeightMapSpin;
		wxButton* m_detailMapBtn0;
		wxSpinCtrl* m_detailMapSpin0;
		wxChoice* m_detailMapBlendMode0;
		wxSlider* m_detailMapSlider0;
		wxTextCtrl* m_detailMapTextCtrl0;
		wxButton* m_detailNmMapBtn0;
		wxSpinCtrl* m_detailNmMapSpin0;
		wxSlider* m_detailNmMapSlider0;
		wxTextCtrl* m_detailNmMapTextCtrl0;
		wxSlider* m_detailMapXSlider0;
		wxTextCtrl* m_detailMapXTextCtrl0;
		wxSlider* m_detailMapYSlider0;
		wxTextCtrl* m_detailMapYTextCtrl0;
		wxSlider* m_detailMapWSlider0;
		wxTextCtrl* m_detailMapWTextCtrl0;
		wxSlider* m_detailMapHSlider0;
		wxTextCtrl* m_detailMapHTextCtrl0;
		wxButton* m_detailMapBtn1;
		wxSpinCtrl* m_detailMapSpin1;
		wxChoice* m_detailMapBlendMode1;
		wxSlider* m_detailMapSlider1;
		wxTextCtrl* m_detailMapTextCtrl1;
		wxButton* m_detailNmMapBtn1;
		wxSpinCtrl* m_detailNmMapSpin1;
		wxSlider* m_detailNmMapSlider1;
		wxTextCtrl* m_detailNmMapTextCtrl1;
		wxSlider* m_detailMapXSlider1;
		wxTextCtrl* m_detailMapXTextCtrl1;
		wxSlider* m_detailMapYSlider1;
		wxTextCtrl* m_detailMapYTextCtrl1;
		wxSlider* m_detailMapWSlider1;
		wxTextCtrl* m_detailMapWTextCtrl1;
		wxSlider* m_detailMapHSlider1;
		wxTextCtrl* m_detailMapHTextCtrl1;
		wxButton* m_detailMapBtn2;
		wxSpinCtrl* m_detailMapSpin2;
		wxChoice* m_detailMapBlendMode2;
		wxSlider* m_detailMapSlider2;
		wxTextCtrl* m_detailMapTextCtrl2;
		wxButton* m_detailNmMapBtn2;
		wxSpinCtrl* m_detailNmMapSpin2;
		wxSlider* m_detailNmMapSlider2;
		wxTextCtrl* m_detailNmMapTextCtrl2;
		wxSlider* m_detailMapXSlider2;
		wxTextCtrl* m_detailMapXTextCtrl2;
		wxSlider* m_detailMapYSlider2;
		wxTextCtrl* m_detailMapYTextCtrl2;
		wxSlider* m_detailMapWSlider2;
		wxTextCtrl* m_detailMapWTextCtrl2;
		wxSlider* m_detailMapHSlider2;
		wxTextCtrl* m_detailMapHTextCtrl2;
		wxButton* m_detailMapBtn3;
		wxSpinCtrl* m_detailMapSpin3;
		wxChoice* m_detailMapBlendMode3;
		wxSlider* m_detailMapSlider3;
		wxTextCtrl* m_detailMapTextCtrl3;
		wxButton* m_detailNmMapBtn3;
		wxSpinCtrl* m_detailNmMapSpin3;
		wxSlider* m_detailNmMapSlider3;
		wxTextCtrl* m_detailNmMapTextCtrl3;
		wxSlider* m_detailMapXSlider3;
		wxTextCtrl* m_detailMapXTextCtrl3;
		wxSlider* m_detailMapYSlider3;
		wxTextCtrl* m_detailMapYTextCtrl3;
		wxSlider* m_detailMapWSlider3;
		wxTextCtrl* m_detailMapWTextCtrl3;
		wxSlider* m_detailMapHSlider3;
		wxTextCtrl* m_detailMapHTextCtrl3;

		// Virtual event handlers, override them in your derived class
		virtual void OnTextureChangeButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSpinCtrl( wxSpinEvent& event ) { event.Skip(); }
		virtual void OnSlider( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBlendModeChoice( wxCommandEvent& event ) { event.Skip(); }


	public:

		PbsTexturePanelBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,1700 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PbsTexturePanelBase();

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

///////////////////////////////////////////////////////////////////////////////
/// Class TextureSelectBase
///////////////////////////////////////////////////////////////////////////////
class TextureSelectBase : public wxDialog
{
	private:

	protected:
		wxListBox* m_textureList;
		wxSearchCtrl* m_searchCtrl;
		wxButton* m_cancelButton;
		wxButton* m_okButton;

		// Virtual event handlers, override them in your derived class
		virtual void OnTextureSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSearchText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		TextureSelectBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Select Texture"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 600,600 ), long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER|wxSYSTEM_MENU );

		~TextureSelectBase();

};

