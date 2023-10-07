
#ifndef Demo_Hlms01MyHlmsListener_H
#define Demo_Hlms01MyHlmsListener_H

#include "OgreHlmsListener.h"

#include "OgreColourValue.h"
#include "OgreHlms.h"

namespace Ogre
{
    class MyHlmsListener final : public HlmsListener
    {
    public:
        bool mEnableEffect;
        ColourValue mMyCustomColour;

        MyHlmsListener() : mEnableEffect( true ), mMyCustomColour( Ogre::ColourValue::Red ) {}

        void preparePassHash( const CompositorShadowNode *shadowNode, bool casterPass,
                              bool dualParaboloid, SceneManager *sceneManager, Hlms *hlms ) override
        {
            if( casterPass )
                return;

            // Note that every time the Hlms encounters a new property (or it is set to a new
            // value it hasn't seen before) a new shader will likely be compiled.
            if( mEnableEffect )
                hlms->_setProperty( Hlms::kNoTid, "my_custom_colour_enabled", 1 );
        }

        uint32 getPassBufferSize( const CompositorShadowNode *shadowNode, bool casterPass,
                                  bool dualParaboloid, SceneManager *sceneManager ) const override
        {
            if( casterPass )
                return 0u;  // Do not add anything to the caster pass.

            if( !mEnableEffect )
                return 0u;

            // We add one float4, so we must add sizeof( float ) * 4 = 16 bytes.
            return 16u;
        }

        float *preparePassBuffer( const CompositorShadowNode *shadowNode, bool casterPass,
                                  bool dualParaboloid, SceneManager *sceneManager,
                                  float *passBufferPtr ) override
        {
            if( casterPass )
            {
                // Do NOT fill anything during the caster pass.
                // Because we returned 0 in getPassBufferSize, trying to write now
                // would be a buffer overflow!
                return passBufferPtr;
            }

            if( !mEnableEffect )
                return passBufferPtr;

            // We must fill the buffers in the GPU from here so the shader can use.
            // This value will be the same for all objects using PBS (because we
            // added this listener only to HlmsPbs).
            //
            // It is our shader templates in
            // Samples/Media/2.0/scripts/materials/Tutorial_Hlms01_Customization
            // that make use of these values.

            // float4 myCustomColour;
            *passBufferPtr++ = static_cast<float>( mMyCustomColour.r );
            *passBufferPtr++ = static_cast<float>( mMyCustomColour.g );
            *passBufferPtr++ = static_cast<float>( mMyCustomColour.b );
            *passBufferPtr++ = static_cast<float>( mMyCustomColour.a );

            return passBufferPtr;
        }
    };
}  // namespace Ogre

#endif
