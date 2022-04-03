
#ifndef _Demo_MyCustomRenderable_H_
#define _Demo_MyCustomRenderable_H_

#include "OgreMovableObject.h"
#include "OgreRenderable.h"

namespace Ogre
{
    class MyCustomRenderable final : public MovableObject, public Renderable
    {
        void createBuffers();

    public:
        MyCustomRenderable( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *manager,
                            uint8 renderQueueId );
        ~MyCustomRenderable() override;

        // Overrides from MovableObject
        const String &getMovableType() const override;

        // Overrides from Renderable
        const LightList &getLights() const override;
        void getRenderOperation( v1::RenderOperation &op, bool casterPass ) override;
        void getWorldTransforms( Matrix4 *xform ) const override;
        bool getCastsShadows() const override;
    };
}  // namespace Ogre

#endif
