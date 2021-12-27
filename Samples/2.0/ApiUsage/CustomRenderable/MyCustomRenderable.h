
#ifndef _Demo_MyCustomRenderable_H_
#define _Demo_MyCustomRenderable_H_

#include "OgreMovableObject.h"
#include "OgreRenderable.h"

namespace Ogre
{
    class MyCustomRenderable : public MovableObject, public Renderable
    {
        void createBuffers();

    public:
        MyCustomRenderable( IdType id, ObjectMemoryManager *objectMemoryManager,
                            SceneManager* manager, uint8 renderQueueId );
        virtual ~MyCustomRenderable();

        //Overrides from MovableObject
        virtual const String& getMovableType() const;

        //Overrides from Renderable
        virtual const LightList& getLights() const;
        virtual void getRenderOperation( v1::RenderOperation& op, bool casterPass );
        virtual void getWorldTransforms( Matrix4* xform ) const;
        virtual bool getCastsShadows() const;
    };
}

#endif
