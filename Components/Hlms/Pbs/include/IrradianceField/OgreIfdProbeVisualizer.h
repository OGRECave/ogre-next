
#ifndef _Ogre_IfdProbeVisualizer_H_
#define _Ogre_IfdProbeVisualizer_H_

#include "OgreHlmsPbsPrerequisites.h"

#include "OgreMovableObject.h"
#include "OgreRenderable.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    struct IrradianceFieldSettings;

    class _OgreHlmsPbsExport IfdProbeVisualizer : public MovableObject, public Renderable
    {
        void createBuffers( void );

    public:
        IfdProbeVisualizer( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *manager,
                            uint8 renderQueueId );
        virtual ~IfdProbeVisualizer();

        /**
        @param ifSettings
            See IrradianceField::mSettings
        @param fieldSize
        @param resolution
            Either ifSettings.mIrradianceResolution or ifSettings.mDepthProbeResolution
        @param ifdTex
            Texture to visualize. Could be colour (irradiance) or depth
        @param tessellation
            Value in range [3; 16]
            Note this value increases exponentially:
                tessellation = 3u -> 24 vertices (per sphere)
                tessellation = 4u -> 112 vertices
                tessellation = 5u -> 480 vertices
                tessellation = 6u -> 1984 vertices
                tessellation = 7u -> 8064 vertices
                tessellation = 8u -> 32512 vertices
                tessellation = 9u -> 130560 vertices
                tessellation = 16u -> 2.147.418.112 vertices
        */
        void setTrackingIfd( const IrradianceFieldSettings &ifSettings, const Vector3 &fieldSize,
                             uint8 resolution, TextureGpu *ifdTex, const Vector2 &rangeMult,
                             uint8_t tessellation );

        // Overrides from MovableObject
        virtual const String &getMovableType( void ) const;

        // Overrides from Renderable
        virtual const LightList &getLights( void ) const;
        virtual void getRenderOperation( v1::RenderOperation &op, bool casterPass );
        virtual void getWorldTransforms( Matrix4 *xform ) const;
        virtual bool getCastsShadows( void ) const;
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
