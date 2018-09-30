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
#include "OgreStableHeaders.h"

#include "OgreInternalCubemapProbe.h"
#include "OgreForwardPlusBase.h"

#include "OgreSceneManager.h"

namespace Ogre
{
    InternalCubemapProbe::InternalCubemapProbe(
            IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *manager ) :
        MovableObject( id, objectMemoryManager, manager, ForwardPlusBase::MinCubemapProbeRq )
    {
        memset( mGpuData, 0, sizeof(mGpuData) );

        //NOTE: For performance reasons, ForwardClustered::collectLightForSlice ignores
        //mLocalAabb & mWorldAabb and assumes its local AABB is this aabb we set as
        //default. To change its shape, use node scaling
        Aabb aabb( Vector3::ZERO, Vector3::UNIT_SCALE * 0.5f );
        mObjectData.mLocalAabb->setFromAabb( aabb, mObjectData.mIndex );
        mObjectData.mWorldAabb->setFromAabb( aabb, mObjectData.mIndex );
        const float radius = aabb.getRadius();
        mObjectData.mLocalRadius[mObjectData.mIndex] = radius;
        mObjectData.mWorldRadius[mObjectData.mIndex] = radius;

        //Disable shadow casting by default. Otherwise it's a waste or resources
        setCastShadows( false );
    }
    //-----------------------------------------------------------------------------------
    InternalCubemapProbe::~InternalCubemapProbe()
    {
    }
    //-----------------------------------------------------------------------------------
    const String& InternalCubemapProbe::getMovableType(void) const
    {
        return InternalCubemapProbeFactory::FACTORY_TYPE_NAME;
    }
    //-----------------------------------------------------------------------------------
    void InternalCubemapProbe::setRenderQueueGroup(uint8 queueID)
    {
        assert( queueID >= ForwardPlusBase::MinCubemapProbeRq &&
                queueID <= ForwardPlusBase::MaxCubemapProbeRq &&
                "RenderQueue IDs > 128 are reserved for other Forward+ objects" );
        MovableObject::setRenderQueueGroup( queueID );
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    String InternalCubemapProbeFactory::FACTORY_TYPE_NAME = "InternalCubemapProbe";
    //-----------------------------------------------------------------------
    const String& InternalCubemapProbeFactory::getType(void) const
    {
     return FACTORY_TYPE_NAME;
    }
    //-----------------------------------------------------------------------
    MovableObject* InternalCubemapProbeFactory::createInstanceImpl( IdType id,
                                                  ObjectMemoryManager *objectMemoryManager,
                                                  SceneManager *manager,
                                                  const NameValuePairList* params )
    {
        InternalCubemapProbe* internalCubemapProbe =
                OGRE_NEW InternalCubemapProbe( id, objectMemoryManager, manager );
        return internalCubemapProbe;
    }
    //-----------------------------------------------------------------------
    void InternalCubemapProbeFactory::destroyInstance( MovableObject* obj)
    {
        OGRE_DELETE obj;
    }
}
