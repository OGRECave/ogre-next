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
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/frame.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/slider.h>
#include <wx/listbox.h>
#include <wx/panel.h>
#include <wx/spinctrl.h>
#include <wx/collpane.h>
#include <wx/stattext.h>
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
		enum
		{
			wxID_MENUCAMCENTERMESH = 6000,
			wxID_MENUCAMERAORIGIN,
			wxID_MENUCAMERAORIGINCENTERY,
			wxID_MENUCOORDINATE_X_UP,
			wxID_MENUCOORDINATE_Y_UP,
			wxID_MENUCOORDINATE_Z_UP,
		};

		wxMenu* m_menuView;

		// Virtual event handlers, override them in your derived class
		virtual void OnMenuSelection( wxCommandEvent& event ) { event.Skip(); }


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
		virtual void UndoMouseUp( wxMouseEvent& event ) { event.Skip(); }
		virtual void UndoKeyUp( wxKeyEvent& event ) { event.Skip(); }
		virtual void UndoKillFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnSlider( wxCommandEvent& event ) { event.Skip(); }
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
		wxButton* m_samplerBulkChangeButton;
		wxChoice* m_diffuseSamplerChoice;
		wxChoice* m_normalSamplerChoice;
		wxChoice* m_specularSamplerChoice;
		wxChoice* m_roughnessSamplerChoice;
		wxChoice* m_emissiveSamplerChoice;
		wxChoice* m_detailWeightSamplerChoice;
		wxChoice* m_detail0DifSamplerChoice;
		wxChoice* m_detail0NmSamplerChoice;
		wxChoice* m_detail1DifSamplerChoice;
		wxChoice* m_detail1NmSamplerChoice;
		wxChoice* m_detail2DifSamplerChoice;
		wxChoice* m_detail2NmSamplerChoice;
		wxChoice* m_detail3DifSamplerChoice;
		wxChoice* m_detail3NmSamplerChoice;

		// Virtual event handlers, override them in your derived class
		virtual void OnTextureChangeButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSpinCtrl( wxSpinEvent& event ) { event.Skip(); }
		virtual void UndoMouseUp( wxMouseEvent& event ) { event.Skip(); }
		virtual void UndoKeyUp( wxKeyEvent& event ) { event.Skip(); }
		virtual void UndoKillFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnSlider( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCollapsiblePaneChanged( wxCollapsiblePaneEvent& event ) { event.Skip(); }
		virtual void OnBlendModeChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBulkSamplerChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSamplerblockChoice( wxCommandEvent& event ) { event.Skip(); }


	public:

		PbsTexturePanelBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,800 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PbsTexturePanelBase();

};

///////////////////////////////////////////////////////////////////////////////
/// Class LightPanelBase
///////////////////////////////////////////////////////////////////////////////
class LightPanelBase : public wxPanel
{
	private:

	protected:
		wxChoice* m_presetChoice;
		wxCheckBox* m_cameraRelativeCheckbox;
		wxSlider* m_eulerXSlider;
		wxTextCtrl* m_eulerX;
		wxSlider* m_eulerYSlider;
		wxTextCtrl* m_eulerY;
		wxSlider* m_eulerZSlider;
		wxTextCtrl* m_eulerZ;
		wxSlider* m_envStrengthSlider;
		wxTextCtrl* m_envStrengthTextCtrl;

		// Virtual event handlers, override them in your derived class
		virtual void OnPresetChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCheckbox( wxCommandEvent& event ) { event.Skip(); }
		virtual void UndoMouseUp( wxMouseEvent& event ) { event.Skip(); }
		virtual void UndoKeyUp( wxKeyEvent& event ) { event.Skip(); }
		virtual void UndoKillFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnSlider( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEulerText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnText( wxCommandEvent& event ) { event.Skip(); }


	public:

		LightPanelBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~LightPanelBase();

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
		wxCheckBox* m_activeMeshOnlyCheckbox;
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

///////////////////////////////////////////////////////////////////////////////
/// Class ProjectSettingsBase
///////////////////////////////////////////////////////////////////////////////
class ProjectSettingsBase : public wxDialog
{
	private:

	protected:
		wxListBox* m_resourcesListBox;
		wxButton* m_resourcesAddBtn;
		wxButton* m_resourcesFolderAddBtn;
		wxButton* m_resourcesRemoveBtn;
		wxTextCtrl* m_relativeFolderTextCtrl;
		wxButton* m_relativeFolderBrowseBtn;
		wxTextCtrl* m_materialFileLocationTextCtrl;
		wxButton* m_materialFileBrowseBtn;
		wxCheckBox* m_deleteAllOtherMaterials;
		wxTextCtrl* m_projectFileLocationTextCtrl;
		wxButton* m_projectFileBrowseBtn;
		wxButton* m_okButton;

		// Virtual event handlers, override them in your derived class
		virtual void OnResourcesAdd( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnResourcesFolderAdd( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnResourcesRemove( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowseFolder( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowse( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		ProjectSettingsBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Project Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 800,600 ), long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER|wxSYSTEM_MENU );

		~ProjectSettingsBase();

};

///////////////////////////////////////////////////////////////////////////////
/// Class SamplerSettingsBase
///////////////////////////////////////////////////////////////////////////////
class SamplerSettingsBase : public wxDialog
{
	private:

	protected:
		wxChoice* m_presetsChoice;
		wxTextCtrl* m_nameTextCtrl;
		wxFlexGridSizer* fgSizer1;
		wxChoice* m_filterMin;
		wxChoice* m_filterMag;
		wxChoice* m_filterMip;
		wxTextCtrl* m_maxAnisotropyTextCtrl;
		wxChoice* m_addressUChoice;
		wxChoice* m_addressVChoice;
		wxChoice* m_addressWChoice;
		wxTextCtrl* m_mipLodBiasTextCtrl;
		wxTextCtrl* m_minLodTextCtrl;
		wxTextCtrl* m_maxLodTextCtrl;
		wxChoice* m_compareFunctionChoice;
		wxTextCtrl* m_borderTextCtrlR;
		wxTextCtrl* m_borderTextCtrlG;
		wxTextCtrl* m_borderTextCtrlB;
		wxTextCtrl* m_borderTextCtrlA;

		// Virtual event handlers, override them in your derived class
		virtual void OnPresetsChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFilterChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddressingModeChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCompareFunctionChoice( wxCommandEvent& event ) { event.Skip(); }


	public:

		SamplerSettingsBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Sampler Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER|wxSYSTEM_MENU );

		~SamplerSettingsBase();

};

///////////////////////////////////////////////////////////////////////////////
/// Class SamplerSettingsBulkSelectBase
///////////////////////////////////////////////////////////////////////////////
class SamplerSettingsBulkSelectBase : public wxDialog
{
	private:

	protected:
		wxCheckBox* m_all;
		wxCheckBox* m_allBase;
		wxCheckBox* m_diffuse;
		wxCheckBox* m_normal;
		wxCheckBox* m_specular;
		wxCheckBox* m_roughness;
		wxCheckBox* m_emissive;
		wxCheckBox* m_detailWeights;
		wxCheckBox* m_allDetails;
		wxCheckBox* m_detail0Diffuse;
		wxCheckBox* m_detail0Nm;
		wxCheckBox* m_detail1Diffuse;
		wxCheckBox* m_detail1Nm;
		wxCheckBox* m_detail2Diffuse;
		wxCheckBox* m_detail2Nm;
		wxCheckBox* m_detail3Diffuse;
		wxCheckBox* m_detail3Nm;
		wxButton* m_okButton;

		// Virtual event handlers, override them in your derived class
		virtual void OnCheckbox( wxCommandEvent& event ) { event.Skip(); }


	public:

		SamplerSettingsBulkSelectBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Select slots to apply same samplerblock"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE );

		~SamplerSettingsBulkSelectBase();

};

