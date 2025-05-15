#include "LightPanel.h"

#include "MainWindow.h"

#include "Utils/HdrUtils.h"

#include "OgreColourValue.h"
#include "OgreLight.h"
#include "OgreSceneManager.h"
#include "OgreSceneNode.h"

#include "rapidjson/document.h"

struct Preset
{
    const char *name;
    Ogre::ColourValue skyColour;
    Ogre::ColourValue ambUpperHemisphere;
    Ogre::ColourValue ambLowerHemisphere;
    float lightPower[3];
    float exposure;
    float minAutoExposure;
    float maxAutoExposure;
    float bloomThreshold;
    float envmapScale;
};

static const Preset c_presets[] = {
    {
        "Bright, sunny day",
        Ogre::ColourValue( 0.2f, 0.4f, 0.6f ) * 60.0f,  // Sky
        Ogre::ColourValue( 0.3f, 0.50f, 0.7f ) * 4.5f,
        Ogre::ColourValue( 0.6f, 0.45f, 0.3f ) * 2.925f,
        {
            97.0f,      // Sun power
            1.5f, 1.5f  // Lights
        },
        0.0f,   // Exposure
        -1.0f,  // Exposure
        2.5f,   // Exposure
        5.0f,   // Bloom
        16.0f   // Env. map scale
    },
    {
        "Average, slightly hazy day",
        Ogre::ColourValue( 0.2f, 0.4f, 0.6f ) * 32.0f,  // Sky
        Ogre::ColourValue( 0.3f, 0.50f, 0.7f ) * 3.15f,
        Ogre::ColourValue( 0.6f, 0.45f, 0.3f ) * 2.0475f,
        {
            48.0f,      // Sun power
            1.5f, 1.5f  // Lights
        },
        0.0f,   // Exposure
        -2.0f,  // Exposure
        2.5f,   // Exposure
        5.0f,   // Bloom
        8.0f    // Env. map scale
    },
    {
        "Heavy overcast day",
        Ogre::ColourValue( 0.4f, 0.4f, 0.4f ) * 4.5f,  // Sky
        Ogre::ColourValue( 0.5f, 0.5f, 0.5f ) * 0.4f,
        Ogre::ColourValue( 0.5f, 0.5f, 0.5f ) * 0.365625f,
        {
            6.0625f,    // Sun power
            1.5f, 1.5f  // Lights
        },
        0.0f,   // Exposure
        -2.5f,  // Exposure
        1.0f,   // Exposure
        5.0f,   // Bloom
        0.5f    // Env. map scale
    },
    {
        "Gibbous moon night",
        Ogre::ColourValue( 0.27f, 0.3f, 0.6f ) * 0.01831072f,  // Sky
        Ogre::ColourValue( 0.5f, 0.5f, 0.50f ) * 0.003f,
        Ogre::ColourValue( 0.4f, 0.5f, 0.65f ) * 0.00274222f,
        {
            0.0009251f,  // Sun power
            1.5f, 1.5f   // Lights
        },
        0.65f,            // Exposure
        -2.5f,            // Exposure
        3.0f,             // Exposure
        5.0f,             // Bloom
        0.0152587890625f  // Env. map scale
    },
    {
        "Gibbous moon night w/ powerful spotlights",
        Ogre::ColourValue( 0.27f, 0.3f, 0.6f ) * 0.01831072f,  // Sky
        Ogre::ColourValue( 0.5f, 0.5f, 0.50f ) * 0.003f,
        Ogre::ColourValue( 0.4f, 0.5f, 0.65f ) * 0.00274222f,
        {
            0.0009251f,  // Sun power
            6.5f, 6.5f   // Lights
        },
        0.65f,            // Exposure
        -2.5f,            // Exposure
        3.0f,             // Exposure
        5.0f,             // Bloom
        0.0152587890625f  // Env. map scale
    },
    {
        "JJ Abrams style",
        Ogre::ColourValue( 0.2f, 0.4f, 0.6f ) * 6.0f,  // Sky
        Ogre::ColourValue( 0.3f, 0.50f, 0.7f ) * 0.1125f,
        Ogre::ColourValue( 0.6f, 0.45f, 0.3f ) * 0.073125f,
        {
            4.0f,           // Sun power
            17.05f, 17.05f  // Lights
        },
        0.5f,  // Exposure
        1.0f,  // Exposure
        2.5f,  // Exposure
        3.0f,  // Bloom
        1.0f,  // Env. map scale
    },
};

constexpr size_t kNumPresets = sizeof( c_presets ) / sizeof( c_presets[0] );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
LightPanel::LightPanel( MainWindow *parent ) :
    LightPanelBase( parent ),
    m_mainWindow( parent ),
    m_masterNode( 0 ),
    m_secondaryNode( 0 ),
    m_lightNodes{},
    m_editing( false )
{
    wxArrayString presets;
    presets.reserve( kNumPresets );
    for( size_t i = 0u; i < kNumPresets; ++i )
        presets.push_back( wxString::FromUTF8( c_presets[i].name ) );
    m_presetChoice->Set( presets );

    const Ogre::Vector3 initialDir = Ogre::Vector3( -1.0f, -1.0f, -1.5f ).normalisedCopy();

    Ogre::ColourValue lightColours[] = {
        Ogre::ColourValue::White,               // Main.
        Ogre::ColourValue( 0.8f, 0.4f, 0.2f ),  // Warm.
        Ogre::ColourValue( 0.2f, 0.4f, 0.8f ),  // Cold.
    };

    Ogre::SceneManager *sceneManager = m_mainWindow->getSceneManager();
    m_masterNode = sceneManager->getRootSceneNode()->createChildSceneNode();
    m_secondaryNode = m_masterNode->createChildSceneNode();

    for( size_t i = 0u; i < 3u; ++i )
    {
        Ogre::Light *light = sceneManager->createLight();
        m_lightNodes[i] = m_secondaryNode->createChildSceneNode();
        m_lightNodes[i]->attachObject( light );

        light->setType( i == 0u ? Ogre::Light::LT_DIRECTIONAL : Ogre::Light::LT_SPOTLIGHT );

        light->setDirection(
            Ogre::Quaternion( Ogre::Degree( 120.0f * Ogre::Real( i ) ), Ogre::Vector3::UNIT_Y ) *
            initialDir );
        light->setDiffuseColour( lightColours[i] );
        light->setSpecularColour( lightColours[i] );
    }

    for( size_t i = 0u; i < 3u; ++i )
    {
        SliderTextWidgetAngle widgets = getEulerSliders( i );
        widgets.text->SetValue( wxT( "0.0" ) );
        widgets.fromText();
    }

    m_presetChoice->SetSelection( 0 );
    setPreset( 0u );

    setCameraRelative( true );
}
//-----------------------------------------------------------------------------
void LightPanel::reorientLights()
{
    double ex = 0.0, ey = 0.0, ez = 0.0;
    m_eulerX->GetValue().ToDouble( &ex );
    m_eulerY->GetValue().ToDouble( &ey );
    m_eulerZ->GetValue().ToDouble( &ez );

    const Ogre::Quaternion qx( Ogre::Degree( Ogre::Real( ex ) ), Ogre::Vector3::UNIT_X );
    const Ogre::Quaternion qy( Ogre::Degree( Ogre::Real( ey ) ), Ogre::Vector3::UNIT_Y );
    const Ogre::Quaternion qz( Ogre::Degree( Ogre::Real( ez ) ), Ogre::Vector3::UNIT_Z );

    Ogre::Quaternion q = qx * qy * qz;
    q.normalise();

    m_secondaryNode->setOrientation( q );
}
//-----------------------------------------------------------------------------
SliderTextWidgetAngle LightPanel::getEulerSliders( size_t idx )
{
    switch( idx )
    {
    default:
    case 0:
        return { m_eulerXSlider, m_eulerX };
    case 1:
        return { m_eulerYSlider, m_eulerY };
    case 2:
        return { m_eulerZSlider, m_eulerZ };
    }
}
//-----------------------------------------------------------------------------
void LightPanel::OnPresetChoice( wxCommandEvent &event )
{
    if( m_editing )
        return;
    EditingScope scope( m_editing );

    setPreset( uint32_t( m_presetChoice->GetSelection() ) );
    event.Skip();
}
//-----------------------------------------------------------------------------
void LightPanel::OnCheckbox( wxCommandEvent &event )
{
    setCameraRelative( m_cameraRelativeCheckbox->IsChecked() );
    event.Skip();
}
//-----------------------------------------------------------------------------
void LightPanel::OnEulerText( wxCommandEvent &event )
{
    event.Skip();

    if( m_editing )
        return;
    EditingScope scope( m_editing );

    for( size_t i = 0u; i < 3u; ++i )
    {
        SliderTextWidgetAngle widgets = getEulerSliders( i );
        if( widgets.text == event.GetEventObject() )
        {
            widgets.fromText();
            reorientLights();
            return;
        }
    }
}
//-----------------------------------------------------------------------------
void LightPanel::OnSlider( wxCommandEvent &event )
{
    event.Skip();

    if( m_editing )
        return;
    EditingScope scope( m_editing );

    for( size_t i = 0u; i < 3u; ++i )
    {
        SliderTextWidgetAngle widgets = getEulerSliders( i );
        if( widgets.slider == event.GetEventObject() )
        {
            widgets.fromSlider();
            reorientLights();
            return;
        }
    }
}
//-----------------------------------------------------------------------------
void LightPanel::OnText( wxCommandEvent &event )
{
    // TODO: Implement OnText
    event.Skip();
}
//-----------------------------------------------------------------------------
void LightPanel::setCoordinateConvention( CoordinateConvention::CoordinateConvention newConvention )
{
    m_masterNode->setOrientation( kCoordConventions[newConvention] );
}
//-----------------------------------------------------------------------------
void LightPanel::setCameraRelative( const bool bCameraRelative )
{
    if( m_editing )
        return;
    EditingScope scope( m_editing );

    m_secondaryNode->getParent()->removeChild( m_secondaryNode );

    Ogre::SceneNode *parentNode = 0;
    if( bCameraRelative )
    {
        parentNode = m_mainWindow->getCameraNode();
    }
    else
    {
        parentNode = m_masterNode;
    }

    m_cameraRelativeCheckbox->SetValue( bCameraRelative );

    parentNode->addChild( m_secondaryNode );
}
//-----------------------------------------------------------------------------
void LightPanel::notifyMeshChanged()
{
    const Ogre::MovableObject *movableObject = m_mainWindow->getActiveObject();
    if( !movableObject )
        return;

    for( size_t i = 1u; i < 3u; ++i )
    {
        OGRE_ASSERT_HIGH( dynamic_cast<Ogre::Light *>( m_lightNodes[i]->getAttachedObject( 0 ) ) );
        Ogre::Light *light = static_cast<Ogre::Light *>( m_lightNodes[i]->getAttachedObject( 0 ) );
        m_lightNodes[i]->setPosition( movableObject->getLocalAabb().mCenter -
                                      light->getDirection() * movableObject->getLocalRadius() * 2.0f );
        light->setAttenuationBasedOnRadius( movableObject->getLocalRadius() * 2.0f, 0.01f );
    }
}
//-----------------------------------------------------------------------------
void LightPanel::setPreset( const uint32_t presetIdx )
{
    OGRE_ASSERT( presetIdx < kNumPresets );

    const Preset &preset = c_presets[presetIdx];

    using namespace Demo;
    HdrUtils::setSkyColour( preset.skyColour, 1.0f, m_mainWindow->getCompositorWorkspace() );
    HdrUtils::setExposure( preset.exposure, preset.minAutoExposure, preset.maxAutoExposure );
    HdrUtils::setBloomThreshold( std::max( preset.bloomThreshold - 2.0f, 0.0f ),
                                 std::max( preset.bloomThreshold, 0.01f ) );

    for( size_t i = 0u; i < 3u; ++i )
    {
        OGRE_ASSERT_HIGH( dynamic_cast<Ogre::Light *>( m_lightNodes[i]->getAttachedObject( 0 ) ) );
        Ogre::Light *light = static_cast<Ogre::Light *>( m_lightNodes[i]->getAttachedObject( 0 ) );
        light->setPowerScale( preset.lightPower[i] );
    }

    Ogre::SceneManager *sceneManager = m_mainWindow->getSceneManager();

    sceneManager->setAmbientLight( preset.ambLowerHemisphere, preset.ambUpperHemisphere,
                                   sceneManager->getAmbientLightHemisphereDir(), preset.envmapScale );
}
//-----------------------------------------------------------------------------
void LightPanel::loadProject( const rapidjson::Document &d )
{
    rapidjson::Value::ConstMemberIterator itor = d.FindMember( "lights" );
    if( itor == d.MemberEnd() || !itor->value.IsObject() )
        return;

    const rapidjson::Value &lightObj = itor->value;

    lightObj.FindMember( "preset" );
    if( itor != lightObj.MemberEnd() && itor->value.IsUint() &&
        itor->value.GetUint() < m_presetChoice->GetCount() )
    {
        m_presetChoice->SetSelection( (int)itor->value.GetUint() );
        setPreset( itor->value.GetUint() );
    }

    lightObj.FindMember( "camera_relative" );
    if( itor != lightObj.MemberEnd() && itor->value.IsBool() )
        setCameraRelative( itor->value.GetBool() );

    lightObj.FindMember( "euler" );
    if( itor != lightObj.MemberEnd() && itor->value.IsArray() && itor->value.Size() == 3u &&
        itor->value[0].IsDouble() && itor->value[1].IsDouble() && itor->value[2].IsDouble() )
    {
        m_eulerX->SetValue( wxString::FromDouble( itor->value[0].GetDouble() ) );
        m_eulerY->SetValue( wxString::FromDouble( itor->value[1].GetDouble() ) );
        m_eulerZ->SetValue( wxString::FromDouble( itor->value[2].GetDouble() ) );

        for( size_t i = 0u; i < 3u; ++i )
        {
            SliderTextWidgetAngle widgets = getEulerSliders( i );
            widgets.fromText();
        }
        reorientLights();
    }
}
//-----------------------------------------------------------------------------
void LightPanel::saveProject( Ogre::String &jsonString )
{
    double ex = 0.0, ey = 0.0, ez = 0.0;
    m_eulerX->GetValue().ToDouble( &ex );
    m_eulerY->GetValue().ToDouble( &ey );
    m_eulerZ->GetValue().ToDouble( &ez );

    jsonString += ",\n	\"lights\" :\n	{";
    jsonString += "\n		\"preset\" : ";
    jsonString += std::to_string( m_presetChoice->GetSelection() );
    jsonString += ",\n		\"camera_relative\" : ";
    jsonString += m_cameraRelativeCheckbox->IsChecked() ? "true" : "false";
    jsonString += ",\n		\"euler\" : [ ";
    jsonString +=
        std::to_string( ex ) + ", " + std::to_string( ey ) + ", " + std::to_string( ez ) + " ]";
    jsonString += "\n	}";
}
