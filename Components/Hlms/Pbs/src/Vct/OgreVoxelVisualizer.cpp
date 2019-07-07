/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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

#include "Vct/OgreVoxelVisualizer.h"

#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include "OgreSceneManager.h"

#include "OgreMaterialManager.h"
#include "OgreTechnique.h"
#include "OgrePass.h"

namespace Ogre
{
    VoxelVisualizer::VoxelVisualizer( IdType id, ObjectMemoryManager *objectMemoryManager,
                                      SceneManager *manager, uint8 renderQueueId ) :
        MovableObject( id, objectMemoryManager, manager, renderQueueId ),
        Renderable()
    {
        Aabb aabb( Vector3( 0.5f ), Vector3( 0.5f ) );
        mObjectData.mLocalAabb->setFromAabb( aabb, mObjectData.mIndex );
        mObjectData.mWorldAabb->setFromAabb( aabb, mObjectData.mIndex );
        mObjectData.mLocalRadius[mObjectData.mIndex] = aabb.getRadius();
        mObjectData.mWorldRadius[mObjectData.mIndex] = aabb.getRadius();

        createBuffers();

        mRenderables.push_back( this );

        setCastShadows( false );
    }
    //-----------------------------------------------------------------------------------
    VoxelVisualizer::~VoxelVisualizer()
    {
        VaoManager *vaoManager = mManager->getDestinationRenderSystem()->getVaoManager();

        VertexArrayObjectArray::const_iterator itor = mVaoPerLod[0].begin();
        VertexArrayObjectArray::const_iterator end  = mVaoPerLod[0].end();
        while( itor != end )
            vaoManager->destroyVertexArrayObject( *itor++ );

        String matName;
        matName = "VCT/VoxelVisualizer" + StringConverter::toString( getId() );
        MaterialManager::getSingleton().remove( matName );
        matName = "VCT/VoxelVisualizer/SeparateOpacity" + StringConverter::toString( getId() );
        MaterialManager::getSingleton().remove( matName );
    }
    //-----------------------------------------------------------------------------------
    void VoxelVisualizer::createBuffers(void)
    {
        VaoManager *vaoManager = mManager->getDestinationRenderSystem()->getVaoManager();

        VertexBufferPackedVec vertexBuffers;
        Ogre::VertexArrayObject *vao = vaoManager->createVertexArrayObject( vertexBuffers, 0,
                                                                            OT_TRIANGLE_STRIP );

        mVaoPerLod[0].push_back( vao );
        mVaoPerLod[1].push_back( vao );
    }
    //-----------------------------------------------------------------------------------
    void VoxelVisualizer::setTrackingVoxel( TextureGpu *opacityTex, TextureGpu *texture, bool anyColour )
    {
        String matName = "VCT/VoxelVisualizer";
        if( anyColour )
            matName += "/OpacityAnyColour";
        else
        {
            if( opacityTex != texture )
                matName += "/SeparateOpacity";
        }

        const size_t stringNameBaseSize = matName.size();
        matName += StringConverter::toString( getId() );
        MaterialPtr mat = MaterialManager::getSingleton().getByName(
                              matName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME ).
                          staticCast<Material>();
        if( mat.isNull() )
        {
            MaterialPtr baseMat = MaterialManager::getSingleton().load(
                                      matName.substr( 0u, stringNameBaseSize ),
                                      ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME ).
                                  staticCast<Material>();
            mat = baseMat->clone( matName );
            mat->load();
        }

        setMaterial( mat );

        const uint32 width  = texture->getWidth();
        const uint32 height = texture->getHeight();
        const uint32 depth  = texture->getDepth();

        VertexArrayObject *vao = mVaoPerLod[0].back();
        vao->setPrimitiveRange( 0u, width * height * depth * 16u );

        Pass *pass = mMaterial->getTechnique( 0 )->getPass( 0 );
        GpuProgramParametersSharedPtr vsParams = pass->getVertexProgramParameters();
        vsParams->setNamedConstant( "vertexBase", uint32( 0u ) );

        uint32 voxelResolution[4] = { width, height, depth, 1u };
        vsParams->setNamedConstant( "voxelResolution", voxelResolution, 1u, 3u );

        if( !anyColour )
        {
            pass->getTextureUnitState( 0 )->setTexture( opacityTex );
            if( opacityTex != texture )
                pass->getTextureUnitState( 1 )->setTexture( texture );
        }
        else
        {
            pass->getTextureUnitState( 0 )->setTexture( texture );
        }

        Aabb aabb( Vector3( width, height, depth ) * 0.5f, Vector3( width, height, depth ) * 0.5f );
        mObjectData.mLocalAabb->setFromAabb( aabb, mObjectData.mIndex );
        mObjectData.mWorldAabb->setFromAabb( aabb, mObjectData.mIndex );
        mObjectData.mLocalRadius[mObjectData.mIndex] = aabb.getRadius();
        mObjectData.mWorldRadius[mObjectData.mIndex] = aabb.getRadius();
    }
    //-----------------------------------------------------------------------------------
    const String& VoxelVisualizer::getMovableType(void) const
    {
        return BLANKSTRING;
    }
    //-----------------------------------------------------------------------------------
    const LightList& VoxelVisualizer::getLights(void) const
    {
        return this->queryLights(); //Return the data from our MovableObject base class.
    }
    //-----------------------------------------------------------------------------------
    void VoxelVisualizer::getRenderOperation( v1::RenderOperation& op , bool casterPass )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                        "VoxelVisualizer do not implement getRenderOperation."
                        " You've put a v2 object in "
                        "the wrong RenderQueue ID (which is set to be compatible with "
                        "v1::Entity). Do not mix v2 and v1 objects",
                        "VoxelVisualizer::getRenderOperation" );
    }
    //-----------------------------------------------------------------------------------
    void VoxelVisualizer::getWorldTransforms( Matrix4* xform ) const
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                        "VoxelVisualizer do not implement getWorldTransforms."
                        " You've put a v2 object in "
                        "the wrong RenderQueue ID (which is set to be compatible with "
                        "v1::Entity). Do not mix v2 and v1 objects",
                        "VoxelVisualizer::getRenderOperation" );
    }
    //-----------------------------------------------------------------------------------
    bool VoxelVisualizer::getCastsShadows(void) const
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                        "VoxelVisualizer do not implement getCastsShadows."
                        " You've put a v2 object in "
                        "the wrong RenderQueue ID (which is set to be compatible with "
                        "v1::Entity). Do not mix v2 and v1 objects",
                        "VoxelVisualizer::getRenderOperation" );
    }
}
