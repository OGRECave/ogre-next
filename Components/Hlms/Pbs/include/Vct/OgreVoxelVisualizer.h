
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
        void createBuffers(void);

    public:
        VoxelVisualizer( IdType id, ObjectMemoryManager *objectMemoryManager,
                         SceneManager* manager, uint8 renderQueueId );
        virtual ~VoxelVisualizer();

        void setTrackingVoxel( TextureGpu *opacityTex, TextureGpu *texture, bool anyColour );

        //Overrides from MovableObject
        virtual const String& getMovableType(void) const;

        //Overrides from Renderable
        virtual const LightList& getLights(void) const;
        virtual void getRenderOperation( v1::RenderOperation& op, bool casterPass );
        virtual void getWorldTransforms( Matrix4* xform ) const;
        virtual bool getCastsShadows(void) const;
    };
}

#include "OgreHeaderSuffix.h"

#endif
