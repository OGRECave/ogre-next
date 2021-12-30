
#ifndef _Ogre_VoxelVisualizer_H_
#define _Ogre_VoxelVisualizer_H_

#include "OgreHlmsPbsPrerequisites.h"

#include "OgreMovableObject.h"
#include "OgreRenderable.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class _OgreHlmsPbsExport VoxelVisualizer : public MovableObject, public Renderable
    {
        void createBuffers();

    public:
        VoxelVisualizer( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *manager,
                         uint8 renderQueueId );
        ~VoxelVisualizer() override;

        void setTrackingVoxel( TextureGpu *opacityTex, TextureGpu *texture, bool anyColour );

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
