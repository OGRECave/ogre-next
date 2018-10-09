/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2018 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#ifndef _OgreInternalCubemapProbe_H_
#define _OgreInternalCubemapProbe_H_

#include "OgreMovableObject.h"
#include "OgreRenderable.h"

namespace Ogre
{
    /**
    @class InternalCubemapProbe
        This class is internal and should not be interfaced directly by users.
        The class 'CubemapProbe' in the HlmsPbs component is the public interface.
        The class CubemapProbe will manage this InternalCubemapProbe and its parent SceneNode

        This internal class is required because Forward Clustered lives in OgreMain and
        is what allows multiple cubemaps to be used on screen.
    */
    class _OgreExport InternalCubemapProbe : public MovableObject
    {
    public:
        float mGpuData[7][4];

    public:
        InternalCubemapProbe( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager* manager );
        virtual ~InternalCubemapProbe();

        //Overrides from MovableObject
        virtual const String& getMovableType(void) const;

        /// InternalCubemapProbes only allow
        /// ForwardPlusBase::MinCubemapProbeRq <= queueID < ForwardPlusBase::MaxCubemapProbeRq
        virtual void setRenderQueueGroup( uint8 queueID );
    };

    class _OgreExport InternalCubemapProbeFactory : public MovableObjectFactory
    {
    protected:
        virtual MovableObject* createInstanceImpl( IdType id, ObjectMemoryManager *objectMemoryManager,
                                                   SceneManager *manager,
                                                   const NameValuePairList* params = 0 );
    public:
        InternalCubemapProbeFactory() {}
        virtual ~InternalCubemapProbeFactory() {}

        static String FACTORY_TYPE_NAME;

        const String& getType(void) const;
        void destroyInstance(MovableObject* obj);
    };
}

#endif
