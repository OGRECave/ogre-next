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

	wxStaticBoxSizer* fresnelSizer;
	fresnelSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Fresnel") ), wxVERTICAL );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_fresnelR = new wxTextCtrl( fresnelSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer2->Add( m_fresnelR, 1, wxALL, 5 );

	m_fresnelG = new wxTextCtrl( fresnelSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer2->Add( m_fresnelG, 1, wxALL, 5 );

	m_fresnelB = new wxTextCtrl( fresnelSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 48,-1 ), 0 );
	bSizer2->Add( m_fresnelB, 1, wxALL, 5 );


	fresnelSizer->Add( bSizer2, 1, wxEXPAND, 0 );

	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxHORIZONTAL );

	m_fresnelRGBA = new wxTextCtrl( fresnelSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer21->Add( m_fresnelRGBA, 1, wxALL, 5 );

	m_buttonFresnel = new wxButton( fresnelSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer21->Add( m_buttonFresnel, 1, wxALL|wxEXPAND, 5 );


	fresnelSizer->Add( bSizer21, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_fresnelColouredCheckbox = new wxCheckBox( fresnelSizer->GetStaticBox(), wxID_ANY, _("Coloured"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_fresnelColouredCheckbox, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_fresnelSlider = new wxSlider( fresnelSizer->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer3->Add( m_fresnelSlider, 1, wxALL, 5 );


	fresnelSizer->Add( bSizer3, 1, wxEXPAND, 5 );


	rootLayout->Add( fresnelSizer, 0, wxEXPAND, 5 );

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

	wxString m_transparencyModeChoiceChoices[] = { _("None"), _("Transparent"), _("Fade"), _("Refractive"), wxEmptyString };
	int m_transparencyModeChoiceNChoices = sizeof( m_transparencyModeChoiceChoices ) / sizeof( wxString );
	m_transparencyModeChoice = new wxChoice( transparencySizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_transparencyModeChoiceNChoices, m_transparencyModeChoiceChoices, 0 );
	m_transparencyModeChoice->SetSelection( 0 );
	bSizer5->Add( m_transparencyModeChoice, 0, wxALL, 5 );

	m_alphaFromTexCheckbox = new wxCheckBox( transparencySizer->GetStaticBox(), wxID_ANY, _("Use Alpha from Textures"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_alphaFromTexCheckbox, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_alphaHashCheckbox = new wxCheckBox( transparencySizer->GetStaticBox(), wxID_ANY, _("Alpha Hashing"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_alphaHashCheckbox, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	transparencySizer->Add( bSizer5, 1, wxEXPAND, 5 );


	rootLayout->Add( transparencySizer, 0, wxEXPAND, 5 );


	this->SetSizer( rootLayout );
	this->Layout();

	// Connect Events
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
	m_transparencyModeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PbsParametersPanelBase::OnTransparencyMode ), NULL, this );
	m_alphaFromTexCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PbsParametersPanelBase::OnCheckbox ), NULL, this );
	m_alphaHashCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PbsParametersPanelBase::OnCheckbox ), NULL, this );
}

PbsParametersPanelBase::~PbsParametersPanelBase()
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
