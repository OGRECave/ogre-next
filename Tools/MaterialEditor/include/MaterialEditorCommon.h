
#pragma once

#include "OgreHlmsSamplerblock.h"

#include <wx/slider.h>
#include <wx/textctrl.h>

OGRE_ASSUME_NONNULL_BEGIN

class wxSlider;
class wxTextCtrl;

class LightPanel;

struct EditingScope
{
    bool &editing;

    EditingScope( bool &bValue ) : editing( bValue ) { editing = true; }
    ~EditingScope() { editing = false; }
};

template <int32_t MULTIPLIER>
struct SliderTextWidgetTemplate
{
    // If one is nullptr, then the other one also is.
    wxSlider *ogre_nullable   slider;
    wxTextCtrl *ogre_nullable text;

    void fromSlider()
    {
        const double val = double( slider->GetValue() );
        text->SetValue( wxString::Format( wxT( "%.05lf" ), val / ( double( MULTIPLIER ) / 100.0 ) ) );
    }
    void fromText()
    {
        double val = 0;
        if( text->GetValue().ToDouble( &val ) )
            slider->SetValue( int( std::round( val * ( double( MULTIPLIER ) / 100.0 ) ) ) );
    }
};

typedef SliderTextWidgetTemplate<10000> SliderTextWidget;
typedef SliderTextWidgetTemplate<100>   SliderTextWidgetAngle;

namespace CoordinateConvention
{
    enum CoordinateConvention
    {
        xUp,
        yUp,
        zUp,
        NumCoordinateConventions
    };
}

extern const Ogre::Quaternion kCoordConventions[CoordinateConvention::NumCoordinateConventions];

struct MeshEntry
{
    Ogre::String name;
    Ogre::String resourceGroup;

    bool operator==( const MeshEntry &other ) const
    {
        return this->name == other.name && this->resourceGroup == other.resourceGroup;
    }
};

struct NamedSampler;
typedef std::vector<NamedSampler> NamedSamplerVec;

struct NamedSampler
{
    wxString               name;
    Ogre::HlmsSamplerblock samplerblock;

    /// Performs Linear O(N) search.
    static NamedSamplerVec::iterator find( NamedSamplerVec              &namedSamplers,
                                           const Ogre::HlmsSamplerblock &samplerblock )
    {
        NamedSamplerVec::iterator itor = namedSamplers.begin();
        NamedSamplerVec::iterator endt = namedSamplers.end();

        while( itor != endt && itor->samplerblock != samplerblock )
            ++itor;
        return itor;
    }

    static NamedSamplerVec::const_iterator find( const NamedSamplerVec        &namedSamplers,
                                                 const Ogre::HlmsSamplerblock &samplerblock )
    {
        NamedSamplerVec::const_iterator itor = namedSamplers.begin();
        NamedSamplerVec::const_iterator endt = namedSamplers.end();

        while( itor != endt && itor->samplerblock != samplerblock )
            ++itor;
        return itor;
    }

    static wxString autogenNameFrom( const Ogre::HlmsSamplerblock &samplerblock );

    static void addTemplate( NamedSamplerVec              &namedSamplers,
                             const Ogre::HlmsSamplerblock &samplerblock );
};

OGRE_ASSUME_NONNULL_END
