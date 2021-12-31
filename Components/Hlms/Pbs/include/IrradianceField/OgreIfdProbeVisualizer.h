
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
        void createBuffers();

    public:
        IfdProbeVisualizer( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *manager,
                            uint8 renderQueueId );
        ~IfdProbeVisualizer() override;

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
        const String &getMovableType() const override;

        // Overrides from Renderable
        const LightList &getLights() const override;
        void             getRenderOperation( v1::RenderOperation &op, bool casterPass ) override;
        void             getWorldTransforms( Matrix4 *xform ) const override;
        bool             getCastsShadows() const override;
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
