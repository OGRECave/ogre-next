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

#include "IrradianceField/OgreIfdProbeVisualizer.h"

#include "IrradianceField/OgreIrradianceField.h"

#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include "OgreSceneManager.h"

#include "OgreMaterialManager.h"
#include "OgrePass.h"
#include "OgreTechnique.h"

namespace Ogre
{
    IfdProbeVisualizer::IfdProbeVisualizer( IdType id, ObjectMemoryManager *objectMemoryManager,
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
    IfdProbeVisualizer::~IfdProbeVisualizer()
    {
        VaoManager *vaoManager = mManager->getDestinationRenderSystem()->getVaoManager();

        VertexArrayObjectArray::const_iterator itor = mVaoPerLod[0].begin();
        VertexArrayObjectArray::const_iterator endt = mVaoPerLod[0].end();
        while( itor != endt )
            vaoManager->destroyVertexArrayObject( *itor++ );

        String matName;
        matName = "IFD/ProbeVisualizer" + StringConverter::toString( getId() );
        MaterialManager::getSingleton().remove( matName );
    }
    //-----------------------------------------------------------------------------------
    void IfdProbeVisualizer::createBuffers( void )
    {
        VaoManager *vaoManager = mManager->getDestinationRenderSystem()->getVaoManager();

        VertexBufferPackedVec vertexBuffers;
        Ogre::VertexArrayObject *vao =
            vaoManager->createVertexArrayObject( vertexBuffers, 0, OT_TRIANGLE_STRIP );

        mVaoPerLod[0].push_back( vao );
        mVaoPerLod[1].push_back( vao );
    }
    //-----------------------------------------------------------------------------------
    void IfdProbeVisualizer::setTrackingIfd( const IrradianceFieldSettings &ifSettings,
                                             const Vector3 &fieldSize, uint8 resolution,
                                             TextureGpu *ifdTex, const Vector2 &rangeMult,
                                             uint8_t tessellation )
    {
        OGRE_ASSERT_LOW( tessellation > 2u && tessellation < 17u );

        String matName = "IFD/ProbeVisualizer";

        const size_t stringNameBaseSize = matName.size();
        matName += StringConverter::toString( getId() );
        MaterialPtr mat = MaterialManager::getSingleton()
                              .getByName( matName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME )
                              .staticCast<Material>();
        if( mat.isNull() )
        {
            MaterialPtr baseMat = MaterialManager::getSingleton()
                                      .load( matName.substr( 0u, stringNameBaseSize ),
                                             ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME )
                                      .staticCast<Material>();
            mat = baseMat->clone( matName );
            mat->load();
        }

        setMaterial( mat );

        Pass *pass = mMaterial->getTechnique( 0 )->getPass( 0 );
        GpuProgramParametersSharedPtr vsParams = pass->getVertexProgramParameters();
        vsParams->setNamedConstant( "vertexBase", uint32( 0u ) );

        const uint32 bandPower = tessellation;  // 2^bandPower = Total Points in a band.
        const uint32 bandPoints = 1u << tessellation;
        const uint32 bandMask = bandPoints - 2u;
        const uint32 sectionsInBand = ( bandPoints / 2u ) - 1u;
        const uint32 totalPoints = sectionsInBand * bandPoints;
        const float sectionArc = Math::TWO_PI / sectionsInBand;

        const uint bandMaskPower[3] = { bandMask, bandPower, totalPoints };
        const Vector2 sectionsBandArc( static_cast<float>( sectionsInBand ), sectionArc );

        Vector3 sphereSize( fieldSize / ifSettings.getNumProbes3f() );
        Real minRatio = Ogre::min( Ogre::min( sphereSize.x, sphereSize.y ), sphereSize.z );
        Vector3 aspectRatio = minRatio / sphereSize;

        vsParams->setNamedConstant( "bandMaskPower", bandMaskPower, 1u, 3u );
        vsParams->setNamedConstant( "sectionsBandArc", sectionsBandArc );
        vsParams->setNamedConstant( "numProbes", ifSettings.mNumProbes, 1u, 3u );
        vsParams->setNamedConstant( "aspectRatioFixer", aspectRatio );

        VertexArrayObject *vao = mVaoPerLod[0].back();
        vao->setPrimitiveRange( 0u, ( totalPoints + 2u ) * ifSettings.getTotalNumProbes() );

        GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();

        const float fColourFullWidth = static_cast<float>( ifdTex->getWidth() );
        const float fColourFullHeight = static_cast<float>( ifdTex->getHeight() );
        Vector4 allParams( resolution, fColourFullWidth, 1.0f / fColourFullWidth,
                           1.0f / fColourFullHeight );
        psParams->setNamedConstant( "allParams", allParams );
        psParams->setNamedConstant( "rangeMult", rangeMult );

        pass->getTextureUnitState( 0 )->setTexture( ifdTex );

        Aabb aabb( ifSettings.getNumProbes3f() * 0.5f, ifSettings.getNumProbes3f() * 0.5f );
        mObjectData.mLocalAabb->setFromAabb( aabb, mObjectData.mIndex );
        mObjectData.mWorldAabb->setFromAabb( aabb, mObjectData.mIndex );
        mObjectData.mLocalRadius[mObjectData.mIndex] = aabb.getRadius();
        mObjectData.mWorldRadius[mObjectData.mIndex] = aabb.getRadius();
    }
    //-----------------------------------------------------------------------------------
    const String &IfdProbeVisualizer::getMovableType( void ) const { return BLANKSTRING; }
    //-----------------------------------------------------------------------------------
    const LightList &IfdProbeVisualizer::getLights( void ) const
    {
        return this->queryLights();  // Return the data from our MovableObject base class.
    }
    //-----------------------------------------------------------------------------------
    void IfdProbeVisualizer::getRenderOperation( v1::RenderOperation &op, bool casterPass )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "IfdProbeVisualizer do not implement getRenderOperation."
                     " You've put a v2 object in "
                     "the wrong RenderQueue ID (which is set to be compatible with "
                     "v1::Entity). Do not mix v2 and v1 objects",
                     "IfdProbeVisualizer::getRenderOperation" );
    }
    //-----------------------------------------------------------------------------------
    void IfdProbeVisualizer::getWorldTransforms( Matrix4 *xform ) const
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "IfdProbeVisualizer do not implement getWorldTransforms."
                     " You've put a v2 object in "
                     "the wrong RenderQueue ID (which is set to be compatible with "
                     "v1::Entity). Do not mix v2 and v1 objects",
                     "IfdProbeVisualizer::getRenderOperation" );
    }
    //-----------------------------------------------------------------------------------
    bool IfdProbeVisualizer::getCastsShadows( void ) const
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "IfdProbeVisualizer do not implement getCastsShadows."
                     " You've put a v2 object in "
                     "the wrong RenderQueue ID (which is set to be compatible with "
                     "v1::Entity). Do not mix v2 and v1 objects",
                     "IfdProbeVisualizer::getRenderOperation" );
    }
}  // namespace Ogre
