///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "MaterialEditorBaseAutogen.h"

///////////////////////////////////////////////////////////////////////////

MainWindowBase::MainWindowBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );


	this->Centre( wxBOTH );
}

MainWindowBase::~MainWindowBase()
{
}

PbsParametersPanelBase::PbsParametersPanelBase( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* rootLayout;
	rootLayout = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* pbsSizer;
	pbsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("PBS Settings") ), wxHORIZONTAL );

	wxString m_workflowChoiceChoices[] = { _("Specular"), _("Specular as Fresnel"), _("Metallic") };
	int m_workflowChoiceNChoices = sizeof( m_workflowChoiceChoices ) / sizeof( wxString );
	m_workflowChoice = new wxChoice( pbsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_workflowChoiceNChoices, m_workflowChoiceChoices, 0 );
	m_workflowChoice->SetSelection( 0 );
	pbsSizer->Add( m_workflowChoice, 0, wxALL, 5 );

	wxString m_brdfChoiceChoices[] = { _("Default"), _("CookTorrance"), _("BlinnPhong"), _("DefaultUncorrelated"), _("DefaultHasDiffuseFresnel"), _("DefaultSeparateDiffuseFresnel"), _("CookTorranceHasDiffuseFresnel"), _("CookTorranceSeparateDiffuseFresnel"), _("CookTorrance"), _("BlinnPhongHasDiffuseFresnel"), _("BlinnPhongSeparateDiffuseFresnel"), _("BlinnPhong"), _("BlinnPhongLegacyMath"), _("BlinnPhongFullLegacy") };
	int m_brdfChoiceNChoices = sizeof( m_brdfChoiceChoices ) / sizeof( wxString );
	m_brdfChoice = new wxChoice( pbsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_brdfChoiceNChoices, m_brdfChoiceChoices, 0 );
	m_brdfChoice->SetSelection( 0 );
	pbsSizer->Add( m_brdfChoice, 0, wxALL, 5 );


	rootLayout->Add( pbsSizer, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* diffuseSizer;
	diffuseSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Diffuse") ), wxVERTICAL );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );

	m_diffuseR = new wxTextCtrl( diffuseSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer6->Add( m_diffuseR, 1, wxALL, 5 );

	m_diffuseG = new wxTextCtrl( diffuseSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer6->Add( m_diffuseG, 1, wxALL, 5 );

	m_diffuseB = new wxTextCtrl( diffuseSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer6->Add( m_diffuseB, 1, wxALL, 5 );


	diffuseSizer->Add( bSizer6, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_diffuseRGBA = new wxTextCtrl( diffuseSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_diffuseRGBA, 1, wxALL, 5 );

	m_buttonDiffuse = new wxButton( diffuseSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_buttonDiffuse, 1, wxALL|wxEXPAND, 5 );


	diffuseSizer->Add( bSizer7, 1, wxEXPAND, 5 );


	rootLayout->Add( diffuseSizer, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* specularSizer;
	specularSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Specular") ), wxVERTICAL );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	m_specularR = new wxTextCtrl( specularSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer8->Add( m_specularR, 1, wxALL, 5 );

	m_specularG = new wxTextCtrl( specularSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer8->Add( m_specularG, 1, wxALL, 5 );

	m_specularB = new wxTextCtrl( specularSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer8->Add( m_specularB, 1, wxALL, 5 );


	specularSizer->Add( bSizer8, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	m_specularRGBA = new wxTextCtrl( specularSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_specularRGBA, 1, wxALL, 5 );

	m_buttonSpecular = new wxButton( specularSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_buttonSpecular, 1, wxALL|wxEXPAND, 5 );


	specularSizer->Add( bSizer9, 1, wxEXPAND, 5 );


	rootLayout->Add( specularSizer, 0, wxEXPAND, 5 );

	m_fresnelSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Fresnel") ), wxVERTICAL );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_fresnelR = new wxTextCtrl( m_fresnelSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer2->Add( m_fresnelR, 1, wxALL, 5 );

	m_fresnelG = new wxTextCtrl( m_fresnelSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer2->Add( m_fresnelG, 1, wxALL, 5 );

	m_fresnelB = new wxTextCtrl( m_fresnelSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer2->Add( m_fresnelB, 1, wxALL, 5 );


	m_fresnelSizer->Add( bSizer2, 1, wxEXPAND, 0 );

	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxHORIZONTAL );

	m_fresnelRGBA = new wxTextCtrl( m_fresnelSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer21->Add( m_fresnelRGBA, 1, wxALL, 5 );

	m_buttonFresnel = new wxButton( m_fresnelSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer21->Add( m_buttonFresnel, 1, wxALL|wxEXPAND, 5 );


	m_fresnelSizer->Add( bSizer21, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_fresnelColouredCheckbox = new wxCheckBox( m_fresnelSizer->GetStaticBox(), wxID_ANY, _("Coloured"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_fresnelColouredCheckbox, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_fresnelSlider = new wxSlider( m_fresnelSizer->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer3->Add( m_fresnelSlider, 1, wxALL, 5 );


	m_fresnelSizer->Add( bSizer3, 1, wxEXPAND, 5 );


	rootLayout->Add( m_fresnelSizer, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* rougnessSizer;
	rougnessSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Roughness") ), wxHORIZONTAL );

	m_roughnessSlider = new wxSlider( rougnessSizer->GetStaticBox(), wxID_ANY, 50, 2, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	rougnessSizer->Add( m_roughnessSlider, 1, wxALL|wxEXPAND, 5 );

	m_roughness = new wxTextCtrl( rougnessSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	rougnessSizer->Add( m_roughness, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	rootLayout->Add( rougnessSizer, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* transparencySizer;
	transparencySizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Transparency") ), wxVERTICAL );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	m_transparencySlider = new wxSlider( transparencySizer->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer4->Add( m_transparencySlider, 1, wxALL|wxEXPAND, 5 );

	m_transparency = new wxTextCtrl( transparencySizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer4->Add( m_transparency, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	transparencySizer->Add( bSizer4, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	wxString m_transparencyModeChoiceChoices[] = { _("None"), _("Transparent"), _("Fade"), _("Refractive") };
	int m_transparencyModeChoiceNChoices = sizeof( m_transparencyModeChoiceChoices ) / sizeof( wxString );
	m_transparencyModeChoice = new wxChoice( transparencySizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_transparencyModeChoiceNChoices, m_transparencyModeChoiceChoices, 0 );
	m_transparencyModeChoice->SetSelection( 0 );
	bSizer5->Add( m_transparencyModeChoice, 0, wxALL, 5 );

	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxVERTICAL );

	m_alphaFromTexCheckbox = new wxCheckBox( transparencySizer->GetStaticBox(), wxID_ANY, _("Use Alpha from Textures"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer15->Add( m_alphaFromTexCheckbox, 0, wxALL, 5 );

	m_alphaHashCheckbox = new wxCheckBox( transparencySizer->GetStaticBox(), wxID_ANY, _("Alpha Hashing"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer15->Add( m_alphaHashCheckbox, 0, wxALL, 5 );


	bSizer5->Add( bSizer15, 1, wxEXPAND, 5 );


	transparencySizer->Add( bSizer5, 1, wxEXPAND, 5 );

	m_submeshListBox = new wxListBox( transparencySizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_EXTENDED|wxLB_MULTIPLE|wxLB_NEEDED_SB );
	m_submeshListBox->SetMinSize( wxSize( -1,400 ) );

	transparencySizer->Add( m_submeshListBox, 1, wxALL|wxEXPAND, 5 );


	rootLayout->Add( transparencySizer, 0, wxEXPAND, 5 );


	this->SetSizer( rootLayout );
	this->Layout();

	// Connect Events
	m_workflowChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PbsParametersPanelBase::OnWorkflowChange ), NULL, this );
	m_brdfChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PbsParametersPanelBase::OnSettingDirty ), NULL, this );
	m_diffuseR->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsParametersPanelBase::OnColourText ), NULL, this );
	m_diffuseG->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsParametersPanelBase::OnColourText ), NULL, this );
	m_diffuseB->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsParametersPanelBase::OnColourText ), NULL, this );
	m_diffuseRGBA->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsParametersPanelBase::OnColourHtml ), NULL, this );
	m_buttonDiffuse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PbsParametersPanelBase::OnColourButton ), NULL, this );
	m_specularR->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsParametersPanelBase::OnColourText ), NULL, this );
	m_specularG->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsParametersPanelBase::OnColourText ), NULL, this );
	m_specularB->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsParametersPanelBase::OnColourText ), NULL, this );
	m_specularRGBA->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsParametersPanelBase::OnColourHtml ), NULL, this );
	m_buttonSpecular->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PbsParametersPanelBase::OnColourButton ), NULL, this );
	m_fresnelR->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsParametersPanelBase::OnColourText ), NULL, this );
	m_fresnelG->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsParametersPanelBase::OnColourText ), NULL, this );
	m_fresnelB->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsParametersPanelBase::OnColourText ), NULL, this );
	m_fresnelRGBA->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsParametersPanelBase::OnColourHtml ), NULL, this );
	m_buttonFresnel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PbsParametersPanelBase::OnColourButton ), NULL, this );
	m_fresnelColouredCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PbsParametersPanelBase::OnCheckbox ), NULL, this );
	m_fresnelSlider->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsParametersPanelBase::OnSlider ), NULL, this );
	m_roughnessSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( PbsParametersPanelBase::OnSlider ), NULL, this );
	m_roughnessSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( PbsParametersPanelBase::OnSlider ), NULL, this );
	m_roughnessSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( PbsParametersPanelBase::OnSlider ), NULL, this );
	m_roughnessSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( PbsParametersPanelBase::OnSlider ), NULL, this );
	m_roughnessSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( PbsParametersPanelBase::OnSlider ), NULL, this );
	m_roughnessSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( PbsParametersPanelBase::OnSlider ), NULL, this );
	m_roughnessSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( PbsParametersPanelBase::OnSlider ), NULL, this );
	m_roughnessSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( PbsParametersPanelBase::OnSlider ), NULL, this );
	m_roughnessSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( PbsParametersPanelBase::OnSlider ), NULL, this );
	m_roughnessSlider->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsParametersPanelBase::OnSlider ), NULL, this );
	m_roughness->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsParametersPanelBase::OnSliderText ), NULL, this );
	m_transparencySlider->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsParametersPanelBase::OnSlider ), NULL, this );
	m_transparency->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsParametersPanelBase::OnSliderText ), NULL, this );
	m_transparencyModeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PbsParametersPanelBase::OnSettingDirty ), NULL, this );
	m_alphaFromTexCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PbsParametersPanelBase::OnCheckbox ), NULL, this );
	m_alphaHashCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PbsParametersPanelBase::OnCheckbox ), NULL, this );
	m_submeshListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( PbsParametersPanelBase::OnSubMeshApply ), NULL, this );
}

PbsParametersPanelBase::~PbsParametersPanelBase()
{
}

PbsTexturePanelBase::PbsTexturePanelBase( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* scrollRootLayout;
	scrollRootLayout = new wxBoxSizer( wxVERTICAL );

	m_scrolledWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_scrolledWindow->SetScrollRate( 5, 5 );
	wxBoxSizer* rootLayout;
	rootLayout = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer7;
	sbSizer7 = new wxStaticBoxSizer( new wxStaticBox( m_scrolledWindow, wxID_ANY, _("Diffuse Map") ), wxHORIZONTAL );

	m_diffuseMapBtn = new wxButton( sbSizer7->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer7->Add( m_diffuseMapBtn, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_diffuseMapSpin = new wxSpinCtrl( sbSizer7->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0 );
	sbSizer7->Add( m_diffuseMapSpin, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	rootLayout->Add( sbSizer7, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer71;
	sbSizer71 = new wxStaticBoxSizer( new wxStaticBox( m_scrolledWindow, wxID_ANY, _("Normal Map") ), wxVERTICAL );

	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxHORIZONTAL );

	m_normalMapBtn = new wxButton( sbSizer71->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer17->Add( m_normalMapBtn, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_normalMapSpin = new wxSpinCtrl( sbSizer71->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0 );
	bSizer17->Add( m_normalMapSpin, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	sbSizer71->Add( bSizer17, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer18;
	bSizer18 = new wxBoxSizer( wxHORIZONTAL );

	m_normalMapSlider = new wxSlider( sbSizer71->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer18->Add( m_normalMapSlider, 1, wxALL, 5 );

	m_normalMapTextCtrl = new wxTextCtrl( sbSizer71->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_normalMapTextCtrl->SetMinSize( wxSize( 48,-1 ) );

	bSizer18->Add( m_normalMapTextCtrl, 0, wxALL, 5 );


	sbSizer71->Add( bSizer18, 1, wxEXPAND, 5 );


	rootLayout->Add( sbSizer71, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer72;
	sbSizer72 = new wxStaticBoxSizer( new wxStaticBox( m_scrolledWindow, wxID_ANY, _("Specular Map") ), wxHORIZONTAL );

	m_specularMapBtn = new wxButton( sbSizer72->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer72->Add( m_specularMapBtn, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_specularMapSpin = new wxSpinCtrl( sbSizer72->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0 );
	sbSizer72->Add( m_specularMapSpin, 0, wxALL, 5 );


	rootLayout->Add( sbSizer72, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer722;
	sbSizer722 = new wxStaticBoxSizer( new wxStaticBox( m_scrolledWindow, wxID_ANY, _("Roughness Map") ), wxHORIZONTAL );

	m_roughnessMapBtn = new wxButton( sbSizer722->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer722->Add( m_roughnessMapBtn, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_roughnessMapSpin = new wxSpinCtrl( sbSizer722->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0 );
	sbSizer722->Add( m_roughnessMapSpin, 0, wxALL, 5 );


	rootLayout->Add( sbSizer722, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer723;
	sbSizer723 = new wxStaticBoxSizer( new wxStaticBox( m_scrolledWindow, wxID_ANY, _("Emissive Map") ), wxHORIZONTAL );

	m_emissiveMapBtn = new wxButton( sbSizer723->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer723->Add( m_emissiveMapBtn, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_emissiveMapSpin = new wxSpinCtrl( sbSizer723->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0 );
	sbSizer723->Add( m_emissiveMapSpin, 0, wxALL, 5 );


	rootLayout->Add( sbSizer723, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer721;
	sbSizer721 = new wxStaticBoxSizer( new wxStaticBox( m_scrolledWindow, wxID_ANY, _("Detail Weight Map") ), wxHORIZONTAL );

	m_detailWeightMapBtn = new wxButton( sbSizer721->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer721->Add( m_detailWeightMapBtn, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_detailWeightMapSpin = new wxSpinCtrl( sbSizer721->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0 );
	sbSizer721->Add( m_detailWeightMapSpin, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	rootLayout->Add( sbSizer721, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer711;
	sbSizer711 = new wxStaticBoxSizer( new wxStaticBox( m_scrolledWindow, wxID_ANY, _("Detail Map 0") ), wxVERTICAL );

	wxStaticBoxSizer* sbSizer16;
	sbSizer16 = new wxStaticBoxSizer( new wxStaticBox( sbSizer711->GetStaticBox(), wxID_ANY, _("Diffuse 0") ), wxVERTICAL );

	wxBoxSizer* bSizer171;
	bSizer171 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapBtn0 = new wxButton( sbSizer16->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer171->Add( m_detailMapBtn0, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_detailMapSpin0 = new wxSpinCtrl( sbSizer16->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0 );
	bSizer171->Add( m_detailMapSpin0, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	sbSizer16->Add( bSizer171, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer181;
	bSizer181 = new wxBoxSizer( wxHORIZONTAL );

	wxString m_detailMapBlendMode0Choices[] = { _("Normal"), _("Normal Premul"), _("Add"), _("Subtract"), _("Multiply"), _("Multiply 2x"), _("Screen"), _("Overlay"), _("Lighten"), _("Darken"), _("Grain Extract"), _("Grain Merge"), _("Difference") };
	int m_detailMapBlendMode0NChoices = sizeof( m_detailMapBlendMode0Choices ) / sizeof( wxString );
	m_detailMapBlendMode0 = new wxChoice( sbSizer16->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_detailMapBlendMode0NChoices, m_detailMapBlendMode0Choices, 0 );
	m_detailMapBlendMode0->SetSelection( 0 );
	bSizer181->Add( m_detailMapBlendMode0, 0, wxALL, 5 );

	m_detailMapSlider0 = new wxSlider( sbSizer16->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer181->Add( m_detailMapSlider0, 1, wxALL, 5 );

	m_detailMapTextCtrl0 = new wxTextCtrl( sbSizer16->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_detailMapTextCtrl0->SetMinSize( wxSize( 48,-1 ) );

	bSizer181->Add( m_detailMapTextCtrl0, 0, wxALL, 5 );


	sbSizer16->Add( bSizer181, 1, wxEXPAND, 5 );


	sbSizer711->Add( sbSizer16, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer161;
	sbSizer161 = new wxStaticBoxSizer( new wxStaticBox( sbSizer711->GetStaticBox(), wxID_ANY, _("Normal 0") ), wxVERTICAL );

	wxBoxSizer* bSizer1712;
	bSizer1712 = new wxBoxSizer( wxHORIZONTAL );

	m_detailNmMapBtn0 = new wxButton( sbSizer161->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1712->Add( m_detailNmMapBtn0, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_detailNmMapSpin0 = new wxSpinCtrl( sbSizer161->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0 );
	bSizer1712->Add( m_detailNmMapSpin0, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	sbSizer161->Add( bSizer1712, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer1812;
	bSizer1812 = new wxBoxSizer( wxHORIZONTAL );

	m_detailNmMapSlider0 = new wxSlider( sbSizer161->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer1812->Add( m_detailNmMapSlider0, 1, wxALL, 5 );

	m_detailNmMapTextCtrl0 = new wxTextCtrl( sbSizer161->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_detailNmMapTextCtrl0->SetMinSize( wxSize( 48,-1 ) );

	bSizer1812->Add( m_detailNmMapTextCtrl0, 0, wxALL, 5 );


	sbSizer161->Add( bSizer1812, 1, wxEXPAND, 5 );


	sbSizer711->Add( sbSizer161, 0, wxEXPAND, 5 );

	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 2, 2, 0, 0 );

	wxBoxSizer* bSizer29;
	bSizer29 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapXSlider0 = new wxSlider( sbSizer711->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer29->Add( m_detailMapXSlider0, 1, wxALL, 5 );

	m_detailMapXTextCtrl0 = new wxTextCtrl( sbSizer711->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer29->Add( m_detailMapXTextCtrl0, 0, wxALL, 5 );


	gSizer1->Add( bSizer29, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer291;
	bSizer291 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapYSlider0 = new wxSlider( sbSizer711->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer291->Add( m_detailMapYSlider0, 1, wxALL, 5 );

	m_detailMapYTextCtrl0 = new wxTextCtrl( sbSizer711->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer291->Add( m_detailMapYTextCtrl0, 0, wxALL, 5 );


	gSizer1->Add( bSizer291, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer292;
	bSizer292 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapWSlider0 = new wxSlider( sbSizer711->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer292->Add( m_detailMapWSlider0, 1, wxALL, 5 );

	m_detailMapWTextCtrl0 = new wxTextCtrl( sbSizer711->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer292->Add( m_detailMapWTextCtrl0, 0, wxALL, 5 );


	gSizer1->Add( bSizer292, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer293;
	bSizer293 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapHSlider0 = new wxSlider( sbSizer711->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer293->Add( m_detailMapHSlider0, 1, wxALL, 5 );

	m_detailMapHTextCtrl0 = new wxTextCtrl( sbSizer711->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer293->Add( m_detailMapHTextCtrl0, 0, wxALL, 5 );


	gSizer1->Add( bSizer293, 1, wxEXPAND, 5 );


	sbSizer711->Add( gSizer1, 0, wxEXPAND, 5 );


	rootLayout->Add( sbSizer711, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer7111;
	sbSizer7111 = new wxStaticBoxSizer( new wxStaticBox( m_scrolledWindow, wxID_ANY, _("Detail Map 1") ), wxVERTICAL );

	wxStaticBoxSizer* sbSizer162;
	sbSizer162 = new wxStaticBoxSizer( new wxStaticBox( sbSizer7111->GetStaticBox(), wxID_ANY, _("Diffuse 1") ), wxVERTICAL );

	wxBoxSizer* bSizer1711;
	bSizer1711 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapBtn1 = new wxButton( sbSizer162->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1711->Add( m_detailMapBtn1, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_detailMapSpin1 = new wxSpinCtrl( sbSizer162->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0 );
	bSizer1711->Add( m_detailMapSpin1, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	sbSizer162->Add( bSizer1711, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer1811;
	bSizer1811 = new wxBoxSizer( wxHORIZONTAL );

	wxString m_detailMapBlendMode1Choices[] = { _("Normal"), _("Normal Premul"), _("Add"), _("Subtract"), _("Multiply"), _("Multiply 2x"), _("Screen"), _("Overlay"), _("Lighten"), _("Darken"), _("Grain Extract"), _("Grain Merge"), _("Difference") };
	int m_detailMapBlendMode1NChoices = sizeof( m_detailMapBlendMode1Choices ) / sizeof( wxString );
	m_detailMapBlendMode1 = new wxChoice( sbSizer162->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_detailMapBlendMode1NChoices, m_detailMapBlendMode1Choices, 0 );
	m_detailMapBlendMode1->SetSelection( 0 );
	bSizer1811->Add( m_detailMapBlendMode1, 0, wxALL, 5 );

	m_detailMapSlider1 = new wxSlider( sbSizer162->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer1811->Add( m_detailMapSlider1, 1, wxALL, 5 );

	m_detailMapTextCtrl1 = new wxTextCtrl( sbSizer162->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_detailMapTextCtrl1->SetMinSize( wxSize( 48,-1 ) );

	bSizer1811->Add( m_detailMapTextCtrl1, 0, wxALL, 5 );


	sbSizer162->Add( bSizer1811, 1, wxEXPAND, 5 );


	sbSizer7111->Add( sbSizer162, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer1611;
	sbSizer1611 = new wxStaticBoxSizer( new wxStaticBox( sbSizer7111->GetStaticBox(), wxID_ANY, _("Normal 1") ), wxVERTICAL );

	wxBoxSizer* bSizer17121;
	bSizer17121 = new wxBoxSizer( wxHORIZONTAL );

	m_detailNmMapBtn1 = new wxButton( sbSizer1611->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer17121->Add( m_detailNmMapBtn1, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_detailNmMapSpin1 = new wxSpinCtrl( sbSizer1611->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0 );
	bSizer17121->Add( m_detailNmMapSpin1, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	sbSizer1611->Add( bSizer17121, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer18121;
	bSizer18121 = new wxBoxSizer( wxHORIZONTAL );

	m_detailNmMapSlider1 = new wxSlider( sbSizer1611->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer18121->Add( m_detailNmMapSlider1, 1, wxALL, 5 );

	m_detailNmMapTextCtrl1 = new wxTextCtrl( sbSizer1611->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_detailNmMapTextCtrl1->SetMinSize( wxSize( 48,-1 ) );

	bSizer18121->Add( m_detailNmMapTextCtrl1, 0, wxALL, 5 );


	sbSizer1611->Add( bSizer18121, 1, wxEXPAND, 5 );


	sbSizer7111->Add( sbSizer1611, 0, wxEXPAND, 5 );

	wxGridSizer* gSizer11;
	gSizer11 = new wxGridSizer( 2, 2, 0, 0 );

	wxBoxSizer* bSizer294;
	bSizer294 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapXSlider1 = new wxSlider( sbSizer7111->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer294->Add( m_detailMapXSlider1, 1, wxALL, 5 );

	m_detailMapXTextCtrl1 = new wxTextCtrl( sbSizer7111->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer294->Add( m_detailMapXTextCtrl1, 0, wxALL, 5 );


	gSizer11->Add( bSizer294, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer2911;
	bSizer2911 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapYSlider1 = new wxSlider( sbSizer7111->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer2911->Add( m_detailMapYSlider1, 1, wxALL, 5 );

	m_detailMapYTextCtrl1 = new wxTextCtrl( sbSizer7111->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer2911->Add( m_detailMapYTextCtrl1, 0, wxALL, 5 );


	gSizer11->Add( bSizer2911, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer2921;
	bSizer2921 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapWSlider1 = new wxSlider( sbSizer7111->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer2921->Add( m_detailMapWSlider1, 1, wxALL, 5 );

	m_detailMapWTextCtrl1 = new wxTextCtrl( sbSizer7111->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer2921->Add( m_detailMapWTextCtrl1, 0, wxALL, 5 );


	gSizer11->Add( bSizer2921, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer2931;
	bSizer2931 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapHSlider1 = new wxSlider( sbSizer7111->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer2931->Add( m_detailMapHSlider1, 1, wxALL, 5 );

	m_detailMapHTextCtrl1 = new wxTextCtrl( sbSizer7111->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer2931->Add( m_detailMapHTextCtrl1, 0, wxALL, 5 );


	gSizer11->Add( bSizer2931, 1, wxEXPAND, 5 );


	sbSizer7111->Add( gSizer11, 0, wxEXPAND, 5 );


	rootLayout->Add( sbSizer7111, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer71111;
	sbSizer71111 = new wxStaticBoxSizer( new wxStaticBox( m_scrolledWindow, wxID_ANY, _("Detail Map 2") ), wxVERTICAL );

	wxStaticBoxSizer* sbSizer1621;
	sbSizer1621 = new wxStaticBoxSizer( new wxStaticBox( sbSizer71111->GetStaticBox(), wxID_ANY, _("Diffuse 2") ), wxVERTICAL );

	wxBoxSizer* bSizer17111;
	bSizer17111 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapBtn2 = new wxButton( sbSizer1621->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer17111->Add( m_detailMapBtn2, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_detailMapSpin2 = new wxSpinCtrl( sbSizer1621->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0 );
	bSizer17111->Add( m_detailMapSpin2, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	sbSizer1621->Add( bSizer17111, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer18111;
	bSizer18111 = new wxBoxSizer( wxHORIZONTAL );

	wxString m_detailMapBlendMode2Choices[] = { _("Normal"), _("Normal Premul"), _("Add"), _("Subtract"), _("Multiply"), _("Multiply 2x"), _("Screen"), _("Overlay"), _("Lighten"), _("Darken"), _("Grain Extract"), _("Grain Merge"), _("Difference") };
	int m_detailMapBlendMode2NChoices = sizeof( m_detailMapBlendMode2Choices ) / sizeof( wxString );
	m_detailMapBlendMode2 = new wxChoice( sbSizer1621->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_detailMapBlendMode2NChoices, m_detailMapBlendMode2Choices, 0 );
	m_detailMapBlendMode2->SetSelection( 0 );
	bSizer18111->Add( m_detailMapBlendMode2, 0, wxALL, 5 );

	m_detailMapSlider2 = new wxSlider( sbSizer1621->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer18111->Add( m_detailMapSlider2, 1, wxALL, 5 );

	m_detailMapTextCtrl2 = new wxTextCtrl( sbSizer1621->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_detailMapTextCtrl2->SetMinSize( wxSize( 48,-1 ) );

	bSizer18111->Add( m_detailMapTextCtrl2, 0, wxALL, 5 );


	sbSizer1621->Add( bSizer18111, 1, wxEXPAND, 5 );


	sbSizer71111->Add( sbSizer1621, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer16111;
	sbSizer16111 = new wxStaticBoxSizer( new wxStaticBox( sbSizer71111->GetStaticBox(), wxID_ANY, _("Normal 2") ), wxVERTICAL );

	wxBoxSizer* bSizer171211;
	bSizer171211 = new wxBoxSizer( wxHORIZONTAL );

	m_detailNmMapBtn2 = new wxButton( sbSizer16111->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer171211->Add( m_detailNmMapBtn2, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_detailNmMapSpin2 = new wxSpinCtrl( sbSizer16111->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0 );
	bSizer171211->Add( m_detailNmMapSpin2, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	sbSizer16111->Add( bSizer171211, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer181211;
	bSizer181211 = new wxBoxSizer( wxHORIZONTAL );

	m_detailNmMapSlider2 = new wxSlider( sbSizer16111->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer181211->Add( m_detailNmMapSlider2, 1, wxALL, 5 );

	m_detailNmMapTextCtrl2 = new wxTextCtrl( sbSizer16111->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_detailNmMapTextCtrl2->SetMinSize( wxSize( 48,-1 ) );

	bSizer181211->Add( m_detailNmMapTextCtrl2, 0, wxALL, 5 );


	sbSizer16111->Add( bSizer181211, 1, wxEXPAND, 5 );


	sbSizer71111->Add( sbSizer16111, 0, wxEXPAND, 5 );

	wxGridSizer* gSizer111;
	gSizer111 = new wxGridSizer( 2, 2, 0, 0 );

	wxBoxSizer* bSizer2941;
	bSizer2941 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapXSlider2 = new wxSlider( sbSizer71111->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer2941->Add( m_detailMapXSlider2, 1, wxALL, 5 );

	m_detailMapXTextCtrl2 = new wxTextCtrl( sbSizer71111->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer2941->Add( m_detailMapXTextCtrl2, 0, wxALL, 5 );


	gSizer111->Add( bSizer2941, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer29111;
	bSizer29111 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapYSlider2 = new wxSlider( sbSizer71111->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer29111->Add( m_detailMapYSlider2, 1, wxALL, 5 );

	m_detailMapYTextCtrl2 = new wxTextCtrl( sbSizer71111->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer29111->Add( m_detailMapYTextCtrl2, 0, wxALL, 5 );


	gSizer111->Add( bSizer29111, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer29211;
	bSizer29211 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapWSlider2 = new wxSlider( sbSizer71111->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer29211->Add( m_detailMapWSlider2, 1, wxALL, 5 );

	m_detailMapWTextCtrl2 = new wxTextCtrl( sbSizer71111->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer29211->Add( m_detailMapWTextCtrl2, 0, wxALL, 5 );


	gSizer111->Add( bSizer29211, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer29311;
	bSizer29311 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapHSlider2 = new wxSlider( sbSizer71111->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer29311->Add( m_detailMapHSlider2, 1, wxALL, 5 );

	m_detailMapHTextCtrl2 = new wxTextCtrl( sbSizer71111->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer29311->Add( m_detailMapHTextCtrl2, 0, wxALL, 5 );


	gSizer111->Add( bSizer29311, 1, wxEXPAND, 5 );


	sbSizer71111->Add( gSizer111, 0, wxEXPAND, 5 );


	rootLayout->Add( sbSizer71111, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer71112;
	sbSizer71112 = new wxStaticBoxSizer( new wxStaticBox( m_scrolledWindow, wxID_ANY, _("Detail Map 3") ), wxVERTICAL );

	wxStaticBoxSizer* sbSizer16211;
	sbSizer16211 = new wxStaticBoxSizer( new wxStaticBox( sbSizer71112->GetStaticBox(), wxID_ANY, _("Diffuse 3") ), wxVERTICAL );

	wxBoxSizer* bSizer171111;
	bSizer171111 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapBtn3 = new wxButton( sbSizer16211->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer171111->Add( m_detailMapBtn3, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_detailMapSpin3 = new wxSpinCtrl( sbSizer16211->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0 );
	bSizer171111->Add( m_detailMapSpin3, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	sbSizer16211->Add( bSizer171111, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer181111;
	bSizer181111 = new wxBoxSizer( wxHORIZONTAL );

	wxString m_detailMapBlendMode3Choices[] = { _("Normal"), _("Normal Premul"), _("Add"), _("Subtract"), _("Multiply"), _("Multiply 2x"), _("Screen"), _("Overlay"), _("Lighten"), _("Darken"), _("Grain Extract"), _("Grain Merge"), _("Difference") };
	int m_detailMapBlendMode3NChoices = sizeof( m_detailMapBlendMode3Choices ) / sizeof( wxString );
	m_detailMapBlendMode3 = new wxChoice( sbSizer16211->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_detailMapBlendMode3NChoices, m_detailMapBlendMode3Choices, 0 );
	m_detailMapBlendMode3->SetSelection( 0 );
	bSizer181111->Add( m_detailMapBlendMode3, 0, wxALL, 5 );

	m_detailMapSlider3 = new wxSlider( sbSizer16211->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer181111->Add( m_detailMapSlider3, 1, wxALL, 5 );

	m_detailMapTextCtrl3 = new wxTextCtrl( sbSizer16211->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_detailMapTextCtrl3->SetMinSize( wxSize( 48,-1 ) );

	bSizer181111->Add( m_detailMapTextCtrl3, 0, wxALL, 5 );


	sbSizer16211->Add( bSizer181111, 1, wxEXPAND, 5 );


	sbSizer71112->Add( sbSizer16211, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer161111;
	sbSizer161111 = new wxStaticBoxSizer( new wxStaticBox( sbSizer71112->GetStaticBox(), wxID_ANY, _("Normal 3") ), wxVERTICAL );

	wxBoxSizer* bSizer1712111;
	bSizer1712111 = new wxBoxSizer( wxHORIZONTAL );

	m_detailNmMapBtn3 = new wxButton( sbSizer161111->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1712111->Add( m_detailNmMapBtn3, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_detailNmMapSpin3 = new wxSpinCtrl( sbSizer161111->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0 );
	bSizer1712111->Add( m_detailNmMapSpin3, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	sbSizer161111->Add( bSizer1712111, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer1812111;
	bSizer1812111 = new wxBoxSizer( wxHORIZONTAL );

	m_detailNmMapSlider3 = new wxSlider( sbSizer161111->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer1812111->Add( m_detailNmMapSlider3, 1, wxALL, 5 );

	m_detailNmMapTextCtrl3 = new wxTextCtrl( sbSizer161111->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_detailNmMapTextCtrl3->SetMinSize( wxSize( 48,-1 ) );

	bSizer1812111->Add( m_detailNmMapTextCtrl3, 0, wxALL, 5 );


	sbSizer161111->Add( bSizer1812111, 1, wxEXPAND, 5 );


	sbSizer71112->Add( sbSizer161111, 0, wxEXPAND, 5 );

	wxGridSizer* gSizer112;
	gSizer112 = new wxGridSizer( 2, 2, 0, 0 );

	wxBoxSizer* bSizer2942;
	bSizer2942 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapXSlider3 = new wxSlider( sbSizer71112->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer2942->Add( m_detailMapXSlider3, 1, wxALL, 5 );

	m_detailMapXTextCtrl3 = new wxTextCtrl( sbSizer71112->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer2942->Add( m_detailMapXTextCtrl3, 0, wxALL, 5 );


	gSizer112->Add( bSizer2942, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer29112;
	bSizer29112 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapYSlider3 = new wxSlider( sbSizer71112->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer29112->Add( m_detailMapYSlider3, 1, wxALL, 5 );

	m_detailMapYTextCtrl3 = new wxTextCtrl( sbSizer71112->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer29112->Add( m_detailMapYTextCtrl3, 0, wxALL, 5 );


	gSizer112->Add( bSizer29112, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer29212;
	bSizer29212 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapWSlider3 = new wxSlider( sbSizer71112->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer29212->Add( m_detailMapWSlider3, 1, wxALL, 5 );

	m_detailMapWTextCtrl3 = new wxTextCtrl( sbSizer71112->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer29212->Add( m_detailMapWTextCtrl3, 0, wxALL, 5 );


	gSizer112->Add( bSizer29212, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer29312;
	bSizer29312 = new wxBoxSizer( wxHORIZONTAL );

	m_detailMapHSlider3 = new wxSlider( sbSizer71112->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer29312->Add( m_detailMapHSlider3, 1, wxALL, 5 );

	m_detailMapHTextCtrl3 = new wxTextCtrl( sbSizer71112->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer29312->Add( m_detailMapHTextCtrl3, 0, wxALL, 5 );


	gSizer112->Add( bSizer29312, 1, wxEXPAND, 5 );


	sbSizer71112->Add( gSizer112, 0, wxEXPAND, 5 );


	rootLayout->Add( sbSizer71112, 0, wxEXPAND, 5 );


	m_scrolledWindow->SetSizer( rootLayout );
	m_scrolledWindow->Layout();
	rootLayout->Fit( m_scrolledWindow );
	scrollRootLayout->Add( m_scrolledWindow, 1, wxEXPAND | wxALL, 5 );


	this->SetSizer( scrollRootLayout );
	this->Layout();

	// Connect Events
	m_diffuseMapBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PbsTexturePanelBase::OnTextureChangeButton ), NULL, this );
	m_diffuseMapSpin->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( PbsTexturePanelBase::OnSpinCtrl ), NULL, this );
	m_normalMapBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PbsTexturePanelBase::OnTextureChangeButton ), NULL, this );
	m_normalMapSpin->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( PbsTexturePanelBase::OnSpinCtrl ), NULL, this );
	m_normalMapSlider->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_normalMapTextCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_specularMapBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PbsTexturePanelBase::OnTextureChangeButton ), NULL, this );
	m_specularMapSpin->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( PbsTexturePanelBase::OnSpinCtrl ), NULL, this );
	m_roughnessMapBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PbsTexturePanelBase::OnTextureChangeButton ), NULL, this );
	m_roughnessMapSpin->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( PbsTexturePanelBase::OnSpinCtrl ), NULL, this );
	m_emissiveMapBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PbsTexturePanelBase::OnTextureChangeButton ), NULL, this );
	m_emissiveMapSpin->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( PbsTexturePanelBase::OnSpinCtrl ), NULL, this );
	m_detailWeightMapBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PbsTexturePanelBase::OnTextureChangeButton ), NULL, this );
	m_detailWeightMapSpin->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( PbsTexturePanelBase::OnSpinCtrl ), NULL, this );
	m_detailMapBtn0->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PbsTexturePanelBase::OnTextureChangeButton ), NULL, this );
	m_detailMapSpin0->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( PbsTexturePanelBase::OnSpinCtrl ), NULL, this );
	m_detailMapBlendMode0->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PbsTexturePanelBase::OnBlendModeChoice ), NULL, this );
	m_detailMapSlider0->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapTextCtrl0->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailNmMapBtn0->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PbsTexturePanelBase::OnTextureChangeButton ), NULL, this );
	m_detailNmMapSpin0->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( PbsTexturePanelBase::OnSpinCtrl ), NULL, this );
	m_detailNmMapSlider0->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailNmMapTextCtrl0->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapXSlider0->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapXTextCtrl0->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapYSlider0->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapYTextCtrl0->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapWSlider0->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapWTextCtrl0->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapHSlider0->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapHTextCtrl0->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapBtn1->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PbsTexturePanelBase::OnTextureChangeButton ), NULL, this );
	m_detailMapSpin1->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( PbsTexturePanelBase::OnSpinCtrl ), NULL, this );
	m_detailMapBlendMode1->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PbsTexturePanelBase::OnBlendModeChoice ), NULL, this );
	m_detailMapSlider1->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapTextCtrl1->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailNmMapBtn1->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PbsTexturePanelBase::OnTextureChangeButton ), NULL, this );
	m_detailNmMapSpin1->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( PbsTexturePanelBase::OnSpinCtrl ), NULL, this );
	m_detailNmMapSlider1->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailNmMapTextCtrl1->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapXSlider1->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapXTextCtrl1->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapYSlider1->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapYTextCtrl1->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapWSlider1->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapWTextCtrl1->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapHSlider1->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapHTextCtrl1->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapBtn2->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PbsTexturePanelBase::OnTextureChangeButton ), NULL, this );
	m_detailMapSpin2->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( PbsTexturePanelBase::OnSpinCtrl ), NULL, this );
	m_detailMapBlendMode2->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PbsTexturePanelBase::OnBlendModeChoice ), NULL, this );
	m_detailMapSlider2->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapTextCtrl2->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailNmMapBtn2->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PbsTexturePanelBase::OnTextureChangeButton ), NULL, this );
	m_detailNmMapSpin2->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( PbsTexturePanelBase::OnSpinCtrl ), NULL, this );
	m_detailNmMapSlider2->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailNmMapTextCtrl2->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapXSlider2->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapXTextCtrl2->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapYSlider2->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapYTextCtrl2->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapWSlider2->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapWTextCtrl2->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapHSlider2->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapHTextCtrl2->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapBtn3->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PbsTexturePanelBase::OnTextureChangeButton ), NULL, this );
	m_detailMapSpin3->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( PbsTexturePanelBase::OnSpinCtrl ), NULL, this );
	m_detailMapBlendMode3->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PbsTexturePanelBase::OnBlendModeChoice ), NULL, this );
	m_detailMapSlider3->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapTextCtrl3->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailNmMapBtn3->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PbsTexturePanelBase::OnTextureChangeButton ), NULL, this );
	m_detailNmMapSpin3->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( PbsTexturePanelBase::OnSpinCtrl ), NULL, this );
	m_detailNmMapSlider3->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailNmMapTextCtrl3->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapXSlider3->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapXTextCtrl3->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapYSlider3->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapYTextCtrl3->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapWSlider3->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapWTextCtrl3->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
	m_detailMapHSlider3->Connect( wxEVT_SLIDER, wxCommandEventHandler( PbsTexturePanelBase::OnSlider ), NULL, this );
	m_detailMapHTextCtrl3->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PbsTexturePanelBase::OnText ), NULL, this );
}

PbsTexturePanelBase::~PbsTexturePanelBase()
{
}

DatablockListBase::DatablockListBase( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* rootLayout;
	rootLayout = new wxBoxSizer( wxVERTICAL );

	m_datablockList = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	rootLayout->Add( m_datablockList, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxHORIZONTAL );

	m_pbsCheckbox = new wxCheckBox( this, wxID_ANY, _("PBS"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pbsCheckbox->SetValue(true);
	bSizer12->Add( m_pbsCheckbox, 0, wxALL, 5 );

	m_unlitCheckbox = new wxCheckBox( this, wxID_ANY, _("Unlit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unlitCheckbox->SetValue(true);
	bSizer12->Add( m_unlitCheckbox, 0, wxALL, 5 );


	rootLayout->Add( bSizer12, 0, wxEXPAND, 5 );

	m_searchCtrl = new wxSearchCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifndef __WXMAC__
	m_searchCtrl->ShowSearchButton( true );
	#endif
	m_searchCtrl->ShowCancelButton( false );
	rootLayout->Add( m_searchCtrl, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( rootLayout );
	this->Layout();

	// Connect Events
	m_datablockList->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DatablockListBase::OnDatablockSelect ), NULL, this );
	m_pbsCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DatablockListBase::OnCheckbox ), NULL, this );
	m_unlitCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DatablockListBase::OnCheckbox ), NULL, this );
	m_searchCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DatablockListBase::OnSearchText ), NULL, this );
}

DatablockListBase::~DatablockListBase()
{
}

MeshListBase::MeshListBase( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* rootLayout;
	rootLayout = new wxBoxSizer( wxVERTICAL );

	m_meshList = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	rootLayout->Add( m_meshList, 1, wxALL|wxEXPAND, 5 );

	m_searchCtrl = new wxSearchCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifndef __WXMAC__
	m_searchCtrl->ShowSearchButton( true );
	#endif
	m_searchCtrl->ShowCancelButton( false );
	rootLayout->Add( m_searchCtrl, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( rootLayout );
	this->Layout();

	// Connect Events
	m_meshList->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( MeshListBase::OnMeshSelect ), NULL, this );
	m_searchCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( MeshListBase::OnSearchText ), NULL, this );
}

MeshListBase::~MeshListBase()
{
}

TextureSelectBase::TextureSelectBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* rootLayout;
	rootLayout = new wxBoxSizer( wxVERTICAL );

	m_textureList = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	rootLayout->Add( m_textureList, 1, wxALL|wxEXPAND, 5 );

	m_searchCtrl = new wxSearchCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifndef __WXMAC__
	m_searchCtrl->ShowSearchButton( true );
	#endif
	m_searchCtrl->ShowCancelButton( false );
	rootLayout->Add( m_searchCtrl, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer44;
	bSizer44 = new wxBoxSizer( wxHORIZONTAL );

	m_cancelButton = new wxButton( this, wxID_ANY, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer44->Add( m_cancelButton, 1, wxALL, 5 );

	m_okButton = new wxButton( this, wxID_ANY, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer44->Add( m_okButton, 1, wxALL, 5 );


	rootLayout->Add( bSizer44, 0, wxEXPAND, 5 );


	this->SetSizer( rootLayout );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_textureList->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( TextureSelectBase::OnTextureSelect ), NULL, this );
	m_searchCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( TextureSelectBase::OnSearchText ), NULL, this );
	m_cancelButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TextureSelectBase::OnButtonClick ), NULL, this );
	m_okButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TextureSelectBase::OnButtonClick ), NULL, this );
}

TextureSelectBase::~TextureSelectBase()
{
}
