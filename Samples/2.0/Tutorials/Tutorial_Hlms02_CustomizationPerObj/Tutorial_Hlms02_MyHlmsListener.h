
#ifndef Demo_Hlms02MyHlmsListener_H
#define Demo_Hlms02MyHlmsListener_H

#include "OgreHlmsListener.h"

#include "OgreColourValue.h"
#include "OgreHlms.h"

namespace Ogre
{
    class MyHlmsListener final : public HlmsListener
    {
    public:
        bool mEnableEffect;
        Vector3 mWind;
        ColourValue mMyCustomColour;

        MyHlmsListener() :
            mEnableEffect( true ),
            mWind( Ogre::Vector3::ZERO ),
            mMyCustomColour( Ogre::ColourValue::Red )
        {
        }

        void preparePassHash( const CompositorShadowNode *shadowNode, bool casterPass,
                              bool dualParaboloid, SceneManager *sceneManager, Hlms *hlms ) override
        {
            // Note that every time the Hlms encounters a new property (or it is set to a new
            // value it hasn't seen before) a new shader will likely be compiled.
            if( mEnableEffect )
                hlms->_setProperty( Hlms::kNoTid, "my_customizations_enabled", 1 );
        }

        uint32 getPassBufferSize( const CompositorShadowNode *shadowNode, bool casterPass,
                                  bool dualParaboloid, SceneManager *sceneManager ) const override
        {
            if( !mEnableEffect )
                return 0u;

            // We add one float + 3 padding, so we must add sizeof( float ) * 4 = 16 bytes.
            // Wind must be applied to both shadow caster pass and normal pass.
            // (or else the shadow won't have wind)
            uint32 bytesToAdd = 16u;

            if( !casterPass )
                bytesToAdd += 16u;  // Add one float4, so sizeof( float ) * 4 = 16 bytes.

            return bytesToAdd;
        }

        float *preparePassBuffer( const CompositorShadowNode *shadowNode, bool casterPass,
                                  bool dualParaboloid, SceneManager *sceneManager,
                                  float *passBufferPtr ) override
        {
            // We must write EXACTLY the same number of bytes we requested in getPassBufferSize().
            // And that MUST match with the PassBuffer declaration in the shaders.
            //
            // Doing otheriwse would be either a buffer overflow or OgreNext placing data in the
            // wrong variables.

            if( !mEnableEffect )
                return passBufferPtr;

            *passBufferPtr++ = static_cast<float>( mWind.x );
            *passBufferPtr++ = static_cast<float>( mWind.y );
            *passBufferPtr++ = static_cast<float>( mWind.z );
            *passBufferPtr++ = static_cast<float>( 0.0f );

            // We must fill the buffers in the GPU from here so the shader can use.
            // This value will be the same for all objects using PBS (because we
            // added this listener only to HlmsPbs).
            //
            // It is our shader templates in
            // Samples/Media/2.0/scripts/materials/Tutorial_Hlms02_CustomizationPerObj
            // that make use of these values.

            if( !casterPass )
            {
                // float4 myCustomColour;
                *passBufferPtr++ = static_cast<float>( mMyCustomColour.r );
                *passBufferPtr++ = static_cast<float>( mMyCustomColour.g );
                *passBufferPtr++ = static_cast<float>( mMyCustomColour.b );
                *passBufferPtr++ = static_cast<float>( mMyCustomColour.a );
            }

            return passBufferPtr;
        }
    };
}  // namespace Ogre

#endif
