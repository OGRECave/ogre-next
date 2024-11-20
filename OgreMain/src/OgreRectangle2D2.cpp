/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#include "OgreRectangle2D2.h"

#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "Vao/OgreStagingBuffer.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

namespace Ogre
{
    Rectangle2D::Rectangle2D( IdType id, ObjectMemoryManager *objectMemoryManager,
                              SceneManager *manager ) :
        MovableObject( id, objectMemoryManager, manager, 10u ),
        Renderable(),
        mChanged( true ),
        mGeometryFlags( 0 )
    {
        // Always visible
        Aabb aabb( Aabb::BOX_INFINITE );
        mObjectData.mLocalAabb->setFromAabb( aabb, mObjectData.mIndex );
        mObjectData.mWorldAabb->setFromAabb( aabb, mObjectData.mIndex );
        mObjectData.mLocalRadius[mObjectData.mIndex] = std::numeric_limits<Real>::max();
        mObjectData.mWorldRadius[mObjectData.mIndex] = std::numeric_limits<Real>::max();

        setCastShadows( false );
        setUseIdentityView( true );
        setUseIdentityProjection( true );

        mRenderables.push_back( this );
    }
    //-----------------------------------------------------------------------------------
    Rectangle2D::~Rectangle2D() { _releaseManualHardwareResources(); }
    //-----------------------------------------------------------------------------------
    void Rectangle2D::_releaseManualHardwareResources()
    {
        VaoManager *vaoManager = mManager->getDestinationRenderSystem()->getVaoManager();

        VertexArrayObjectArray::const_iterator itor = mVaoPerLod[0].begin();
        VertexArrayObjectArray::const_iterator endr = mVaoPerLod[0].end();
        while( itor != endr )
        {
            VertexArrayObject *vao = *itor;

            const VertexBufferPackedVec &vertexBuffers = vao->getVertexBuffers();
            VertexBufferPackedVec::const_iterator itBuffers = vertexBuffers.begin();
            VertexBufferPackedVec::const_iterator enBuffers = vertexBuffers.end();

            while( itBuffers != enBuffers )
            {
                if( ( *itBuffers )->getMappingState() != MS_UNMAPPED )
                    ( *itBuffers )->unmap( UO_UNMAP_ALL );
                vaoManager->destroyVertexBuffer( *itBuffers );
                ++itBuffers;
            }

            if( vao->getIndexBuffer() )
                vaoManager->destroyIndexBuffer( vao->getIndexBuffer() );
            vaoManager->destroyVertexArrayObject( vao );

            ++itor;
        }
        mVaoPerLod->clear();
    }
    //-----------------------------------------------------------------------------------
    void Rectangle2D::_restoreManualHardwareResources() { createBuffers(); }
    //-----------------------------------------------------------------------------------
    bool Rectangle2D::isQuad() const { return ( mGeometryFlags & GeometryFlagQuad ) != 0u; }
    //-----------------------------------------------------------------------------------
    bool Rectangle2D::isStereo() const { return ( mGeometryFlags & GeometryFlagStereo ) != 0u; }
    //-----------------------------------------------------------------------------------
    bool Rectangle2D::hasNormals() const { return ( mGeometryFlags & GeometryFlagNormals ) != 0u; }
    //-----------------------------------------------------------------------------------
    BufferType Rectangle2D::getBufferType() const
    {
        return static_cast<BufferType>( ( mGeometryFlags & GeometryFlagReserved0 ) >> 3u );
    }
    //-----------------------------------------------------------------------------------
    bool Rectangle2D::isHollowFullscreenRect() const
    {
        return ( mGeometryFlags & GeometryFlagHollowFsRect ) != 0u;
    }
    //-----------------------------------------------------------------------------------
    uint32 Rectangle2D::calculateNumVertices() const
    {
        uint32 numVertices = isQuad() ? 4u : 3u;
        if( isHollowFullscreenRect() )
            numVertices = 3u * 4u;
        if( isStereo() )
            numVertices = numVertices << 1u;
        return numVertices;
    }
    //-----------------------------------------------------------------------------------
    void Rectangle2D::createBuffers()
    {
        VaoManager *vaoManager = mManager->getDestinationRenderSystem()->getVaoManager();

        // Create the vertex buffer

        const uint32 numVertices = calculateNumVertices();

        // Vertex declaration
        VertexElement2Vec vertexElements;
        vertexElements.push_back( VertexElement2( VET_FLOAT2, VES_POSITION ) );
        if( !isHollowFullscreenRect() )
            vertexElements.push_back( VertexElement2( VET_FLOAT2, VES_TEXTURE_COORDINATES ) );
        if( isQuad() )
            vertexElements.push_back( VertexElement2( VET_FLOAT3, VES_NORMAL ) );

        const uint32 bytesPerVertex = VaoManager::calculateVertexSize( vertexElements );

        const BufferType bufferType = getBufferType();

        float *vertexData = reinterpret_cast<float *>( OGRE_MALLOC_SIMD(
            sizeof( float ) * bytesPerVertex * numVertices, Ogre::MEMCATEGORY_GEOMETRY ) );
        FreeOnDestructor dataPtr( vertexData );
        if( !isHollowFullscreenRect() )
            fillBuffer( vertexData, numVertices );
        else
            fillHollowFsRect( vertexData, numVertices );

        Ogre::VertexBufferPacked *vertexBuffer = 0;
        // Create the actual vertex buffer.
        vertexBuffer =
            vaoManager->createVertexBuffer( vertexElements, numVertices, bufferType, vertexData, false );

        const OperationType opType = isHollowFullscreenRect() ? OT_TRIANGLE_LIST : OT_TRIANGLE_STRIP;

        // Now the Vao. We'll just use one vertex buffer source (multi-source not working yet)
        VertexBufferPackedVec vertexBuffers;
        vertexBuffers.push_back( vertexBuffer );
        Ogre::VertexArrayObject *vao = vaoManager->createVertexArrayObject( vertexBuffers, 0, opType );

        mVaoPerLod[0].push_back( vao );
        mVaoPerLod[1].push_back( vao );
        mChanged = true;
    }
    //-----------------------------------------------------------------------------------
    void Rectangle2D::fillHollowFsRect( float *RESTRICT_ALIAS vertexData, size_t maxElements )
    {
        const float c_veryLargeValue = 65000.0f;
        // const float c_veryLargeValue = 1.0f;
        const float radius = static_cast<float>( getHollowRectRadius() );

        Vector2 posCenter[2] = { mPosition, mSize };

        const size_t numIterations = isStereo() ? 2u : 1u;
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_LOW
        const float *vertexDataStart = vertexData;
#endif
        for( size_t i = 0u; i < numIterations; ++i )
        {
            posCenter[i].y = -posCenter[i].y;

            // 1st upper right quad
            *vertexData++ = radius + posCenter[i].x;
            *vertexData++ = 1.0f;

            *vertexData++ = -c_veryLargeValue;
            *vertexData++ = radius + posCenter[i].y;

            *vertexData++ = radius + posCenter[i].x;
            *vertexData++ = radius + posCenter[i].y;

            // 2nd lower right quad
            *vertexData++ = 1.0f;
            *vertexData++ = -radius + posCenter[i].y;

            *vertexData++ = radius + posCenter[i].x;
            *vertexData++ = c_veryLargeValue;

            *vertexData++ = radius + posCenter[i].x;
            *vertexData++ = -radius + posCenter[i].y;

            // 3rd lower left quad
            *vertexData++ = -radius + posCenter[i].x;
            *vertexData++ = -1.0f;

            *vertexData++ = c_veryLargeValue;
            *vertexData++ = -radius + posCenter[i].y;

            *vertexData++ = -radius + posCenter[i].x;
            *vertexData++ = -radius + posCenter[i].y;

            // 4th upper left quad
            *vertexData++ = -1.0f;
            *vertexData++ = radius + posCenter[i].y;

            *vertexData++ = -radius + posCenter[i].x;
            *vertexData++ = -c_veryLargeValue;

            *vertexData++ = -radius + posCenter[i].x;
            *vertexData++ = radius + posCenter[i].y;
        }

        OGRE_ASSERT_LOW( (size_t)( vertexData - vertexDataStart ) == maxElements * 2u );
    }
    //-----------------------------------------------------------------------------------
    void Rectangle2D::fillBuffer( float *RESTRICT_ALIAS vertexData, size_t maxElements )
    {
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_LOW
        const float *vertexDataStart = vertexData;
#endif
        const size_t numIterations = isStereo() ? 2u : 1u;
        const bool bHasNormals = hasNormals();

        for( size_t i = 0u; i < numIterations; ++i )
        {
            // Bottom left
            *vertexData++ = static_cast<float>( mPosition.x );
            *vertexData++ = static_cast<float>( mPosition.y );
            *vertexData++ = 0.0f;
            *vertexData++ = isQuad() ? 1.0f : 2.0f;
            if( bHasNormals )
            {
                for( size_t j = 0u; j < 3u; ++j )
                    *vertexData++ = mNormals[0][j];
            }

            // Top left
            *vertexData++ = static_cast<float>( mPosition.x );
            *vertexData++ = static_cast<float>( mPosition.y + mSize.y );
            *vertexData++ = 0.0f;
            *vertexData++ = 0.0f;
            if( bHasNormals )
            {
                for( size_t j = 0u; j < 3u; ++j )
                    *vertexData++ = mNormals[1][j];
            }

            // Bottom right
            *vertexData++ = static_cast<float>( mPosition.x + mSize.x );
            *vertexData++ = static_cast<float>( mPosition.y );
            *vertexData++ = isQuad() ? 1.0f : 2.0f;
            *vertexData++ = isQuad() ? 1.0f : 2.0f;
            if( bHasNormals )
            {
                for( size_t j = 0u; j < 3u; ++j )
                    *vertexData++ = mNormals[2][j];
            }

            if( isQuad() )
            {
                // Top right
                *vertexData++ = static_cast<float>( mPosition.x + mSize.x );
                *vertexData++ = static_cast<float>( mPosition.y + mSize.y );
                *vertexData++ = 1.0f;
                *vertexData++ = 0.0f;
                if( bHasNormals )
                {
                    for( size_t j = 0u; j < 3u; ++j )
                        *vertexData++ = mNormals[3][j];
                }
            }
        }

        OGRE_ASSERT_LOW( (size_t)( vertexData - vertexDataStart ) ==
                         maxElements * ( hasNormals() ? 7u : 4u ) );
    }
    //-----------------------------------------------------------------------------------
    void Rectangle2D::setGeometry( const Vector2 &pos, const Vector2 &size )
    {
        OGRE_ASSERT_MEDIUM( getBufferType() != BT_IMMUTABLE || mVaoPerLod[0].empty() );
        mPosition = pos;
        mSize = size;
        mChanged = true;
    }
    //-----------------------------------------------------------------------------------
    void Rectangle2D::setNormals( const Vector3 &upperLeft, const Vector3 &bottomLeft,
                                  const Vector3 &upperRight, const Vector3 &bottomRight )
    {
        OGRE_ASSERT_MEDIUM( getBufferType() != BT_IMMUTABLE || mVaoPerLod[0].empty() );
        OGRE_ASSERT_MEDIUM( hasNormals() || mVaoPerLod[0].empty() );
        mNormals[CornerBottomLeft] = bottomLeft;
        mNormals[CornerUpperLeft] = upperLeft;
        mNormals[CornerBottomRight] = bottomRight;
        mNormals[CornerUpperRight] = upperRight;
        mChanged = true;
    }
    //-----------------------------------------------------------------------------------
    void Rectangle2D::setHollowRectRadius( Real radius )
    {
        OGRE_ASSERT_MEDIUM( getBufferType() != BT_IMMUTABLE || mVaoPerLod[0].empty() );
        OGRE_ASSERT_MEDIUM( isHollowFullscreenRect() || mVaoPerLod[0].empty() );
        mNormals[0].x = radius;
        mChanged = true;
    }
    //-----------------------------------------------------------------------------------
    void Rectangle2D::initialize( BufferType bufferType, uint32 geometryFlags )
    {
        OGRE_ASSERT_LOW( mVaoPerLod[0].empty() && "Can only call Rectangle2D::initialize once!" );
        mGeometryFlags = geometryFlags;
        mGeometryFlags |= ( geometryFlags & GeometryFlagReserved0 ) | ( (uint32)bufferType << 3u );
        OGRE_ASSERT_LOW( !( hasNormals() && isHollowFullscreenRect() ) &&
                         "Can't set normals and hollow FS rect at the same time!" );
        createBuffers();
        mChanged = false;
    }
    //-----------------------------------------------------------------------------------
    void Rectangle2D::update()
    {
        if( !mChanged )
            return;

        VertexBufferPacked *vertexBuffer = mVaoPerLod[0].back()->getVertexBuffers().back();
        VaoManager *vaoManager = mManager->getDestinationRenderSystem()->getVaoManager();
        const size_t numTotalBytes = vertexBuffer->getTotalSizeBytes();

        const bool bIsDynamic = vertexBuffer->getBufferType() != BT_DEFAULT;

        float *RESTRICT_ALIAS vertexData = 0;
        StagingBuffer *stagingBuffer = 0;
        if( bIsDynamic )
        {
            vertexData = reinterpret_cast<float * RESTRICT_ALIAS>(
                vertexBuffer->map( 0u, vertexBuffer->getNumElements() ) );
        }
        else
        {
            stagingBuffer = vaoManager->getStagingBuffer( vertexBuffer->getTotalSizeBytes(), true );
            vertexData = reinterpret_cast<float * RESTRICT_ALIAS>( stagingBuffer->map( numTotalBytes ) );
        }

        if( !isHollowFullscreenRect() )
            fillBuffer( vertexData, vertexBuffer->getNumElements() );
        else
            fillHollowFsRect( vertexData, vertexBuffer->getNumElements() );

        if( bIsDynamic )
            vertexBuffer->unmap( UO_KEEP_PERSISTENT );
        else
        {
            stagingBuffer->unmap( StagingBuffer::Destination( vertexBuffer, 0u, 0u, numTotalBytes ) );
            stagingBuffer->removeReferenceCount();
            stagingBuffer = 0;
        }

        mChanged = false;
    }
    //-----------------------------------------------------------------------------------
    const String &Rectangle2D::getMovableType() const { return Rectangle2DFactory::FACTORY_TYPE_NAME; }
    //-----------------------------------------------------------------------------------
    const LightList &Rectangle2D::getLights() const
    {
        return this->queryLights();  // Return the data from our MovableObject base class.
    }
    //-----------------------------------------------------------------------------------
    void Rectangle2D::getRenderOperation( v1::RenderOperation &op, bool casterPass )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "Rectangle2D do not implement getRenderOperation."
                     " You've put a v2 object in "
                     "the wrong RenderQueue ID (which is set to be compatible with "
                     "v1::Entity). Do not mix v2 and v1 objects",
                     "Rectangle2D::getRenderOperation" );
    }
    //-----------------------------------------------------------------------------------
    void Rectangle2D::getWorldTransforms( Matrix4 *xform ) const
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "Rectangle2D do not implement getWorldTransforms."
                     " You've put a v2 object in "
                     "the wrong RenderQueue ID (which is set to be compatible with "
                     "v1::Entity). Do not mix v2 and v1 objects",
                     "Rectangle2D::getRenderOperation" );
    }
    //-----------------------------------------------------------------------------------
    bool Rectangle2D::getCastsShadows() const
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "Rectangle2D do not implement getCastsShadows."
                     " You've put a v2 object in "
                     "the wrong RenderQueue ID (which is set to be compatible with "
                     "v1::Entity). Do not mix v2 and v1 objects",
                     "Rectangle2D::getRenderOperation" );
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    String Rectangle2DFactory::FACTORY_TYPE_NAME = "Rectangle2Dv2";
    //-----------------------------------------------------------------------------------
    const String &Rectangle2DFactory::getType() const { return FACTORY_TYPE_NAME; }
    //-----------------------------------------------------------------------------------
    MovableObject *Rectangle2DFactory::createInstanceImpl( IdType id,
                                                           ObjectMemoryManager *objectMemoryManager,
                                                           SceneManager *manager,
                                                           const NameValuePairList *params )
    {
        return OGRE_NEW Rectangle2D( id, objectMemoryManager, manager );
    }
    //-----------------------------------------------------------------------------------
    void Rectangle2DFactory::destroyInstance( MovableObject *obj ) { OGRE_DELETE obj; }
}  // namespace Ogre
