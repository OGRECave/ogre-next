
#include "Vct/OgreVctVoxelizer.h"

#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreSubMesh2.h"

#include "Vao/OgreVertexArrayObject.h"

#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreUavBufferPacked.h"

namespace Ogre
{
    void VctVoxelizer::countBuffersSize( const MeshPtr &mesh, QueuedMesh &queuedMesh )
    {
        const uint16 numSubmeshes = mesh->getNumSubMeshes();

        size_t totalNumVertices = 0u;
        size_t numIndices16 = 0u;
        size_t numIndices32 = 0u;

        for( uint16 subMeshIdx=0; subMeshIdx<numSubmeshes; ++subMeshIdx )
        {
            SubMesh *subMesh = mesh->getSubMesh( subMeshIdx );
            VertexArrayObject *vao = subMesh->mVao[VpNormal].front();

            //Count the total number of indices and vertices
            size_t vertexStart = 0u;
            size_t numVertices = vao->getBaseVertexBuffer()->getNumElements();

            IndexBufferPacked *indexBuffer = vao->getIndexBuffer();
            if( indexBuffer )
            {
                if( indexBuffer->getIndexType() == IndexBufferPacked::IT_16BIT )
                    numIndices16 += vao->getPrimitiveCount();
                else
                    numIndices32 += vao->getPrimitiveCount();

                totalNumVertices += vao->getBaseVertexBuffer()->getNumElements();
            }
            else
            {
                vertexStart = vao->getPrimitiveStart();
                numVertices = vao->getPrimitiveCount();

                totalNumVertices += vao->getPrimitiveCount();
            }

            //Request to download the vertex buffer(s) to CPU (it will be mapped soon)
            VertexElementSemanticFullArray semanticsToDownload;
            semanticsToDownload.push_back( VES_POSITION );
            semanticsToDownload.push_back( VES_NORMAL );
            semanticsToDownload.push_back( VES_TEXTURE_COORDINATES );
            queuedMesh.submeshes[subMeshIdx].downloadHelper.queueDownload( vao, semanticsToDownload,
                                                                           vertexStart, numVertices );
        }

        if( queuedMesh.bCompressed )
            mNumVerticesCompressed += static_cast<uint32>( totalNumVertices );
        else
            mNumVerticesUncompressed += static_cast<uint32>( totalNumVertices );

        mNumIndices16 += static_cast<uint32>( numIndices16 );
        mNumIndices32 += static_cast<uint32>( numIndices32 );
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::convertMeshUncompressed( const MeshPtr &mesh, QueuedMesh &queuedMesh,
                                                MappedBuffers &mappedBuffers )
    {
        const uint16 numSubmeshes = mesh->getNumSubMeshes();

        for( uint16 subMeshIdx=0; subMeshIdx<numSubmeshes; ++subMeshIdx )
        {
            SubMesh *subMesh = mesh->getSubMesh( subMeshIdx );
            VertexArrayObject *vao = subMesh->mVao[VpNormal].front();

            size_t vertexStart = 0u;
            size_t numVertices = vao->getBaseVertexBuffer()->getNumElements();

            IndexBufferPacked *indexBuffer = vao->getIndexBuffer();
            if( indexBuffer )
            {
                if( indexBuffer->getIndexType() == IndexBufferPacked::IT_16BIT )
                {
                    indexBuffer->copyTo( mIndexBuffer16, vao->getPrimitiveStart(),
                                         vao->getPrimitiveCount() );
                    mappedBuffers.index16BufferOffset += vao->getPrimitiveCount();
                }
                else
                {
                    indexBuffer->copyTo( mIndexBuffer32, vao->getPrimitiveStart(),
                                         vao->getPrimitiveCount() );
                    mappedBuffers.index32BufferOffset += vao->getPrimitiveCount();
                }
            }
            else
            {
                vertexStart = vao->getPrimitiveStart();
                numVertices = vao->getPrimitiveCount();
            }

            float * RESTRICT_ALIAS uncVertexBuffer = mappedBuffers.uncompressedVertexBuffer;

            VertexBufferDownloadHelper &downloadHelper = queuedMesh.submeshes[subMeshIdx].downloadHelper;
            const VertexBufferDownloadHelper::DownloadData *downloadData =
                    downloadHelper.getDownloadData().begin();

            VertexElement2 dummy( VET_FLOAT1, VES_TEXTURE_COORDINATES );
            VertexElement2 origElements[3] =
            {
                downloadData[0].origElements ? *downloadData[0].origElements : dummy,
                downloadData[1].origElements ? *downloadData[1].origElements : dummy,
                downloadData[2].origElements ? *downloadData[2].origElements : dummy,
            };

            //Map the buffers we started downloading in countBuffersSize
            uint8 const * srcData[3];
            downloadHelper.map( srcData );

            for( size_t vertexIdx=0; vertexIdx<numVertices; ++vertexIdx )
            {
                Vector4 pos = downloadHelper.getVector4( srcData[0] + downloadData[0].srcOffset,
                                                         origElements[0] );
                Vector3 normal( Vector3::UNIT_Y );
                Vector2 uv( Vector2::ZERO );

                if( srcData[1] )
                {
                    normal = downloadHelper.getNormal( srcData[1] + downloadData[1].srcOffset,
                                                       origElements[1] );
                }
                if( srcData[2] )
                {
                    uv = downloadHelper.getVector4( srcData[2] + downloadData[2].srcOffset,
                                                    origElements[2] ).xy();
                }

                *uncVertexBuffer++ = static_cast<float>( pos.x );
                *uncVertexBuffer++ = static_cast<float>( pos.y );
                *uncVertexBuffer++ = static_cast<float>( pos.z );

                *uncVertexBuffer++ = static_cast<float>( normal.x );
                *uncVertexBuffer++ = static_cast<float>( normal.y );
                *uncVertexBuffer++ = static_cast<float>( normal.z );

                *uncVertexBuffer++ = static_cast<float>( uv.x );
                *uncVertexBuffer++ = static_cast<float>( uv.y );

                srcData[0] += downloadData[0].srcBytesPerVertex;
                srcData[1] += downloadData[1].srcBytesPerVertex;
                srcData[2] += downloadData[2].srcBytesPerVertex;
            }

            downloadHelper.unmap();
        }
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::addItem( Item *item, bool bCompressed )
    {
        const MeshPtr &mesh = item->getMesh();

        if( !bCompressed )
        {
            //Force no compression, even if the entry was already there
            QueuedMesh &queuedMesh = mMeshesV2[mesh];
            queuedMesh.bCompressed = false;
            queuedMesh.submeshes.resize( mesh->getNumSubMeshes() );
        }
        else
        {
            //We can only request with compression if the entry wasn't already there
            MeshPtrMap::iterator itor = mMeshesV2.find( mesh );
            if( itor == mMeshesV2.end() )
            {
                QueuedMesh queuedMesh;
                queuedMesh.bCompressed = true;
                queuedMesh.submeshes.resize( mesh->getNumSubMeshes() );
                mMeshesV2[mesh] = queuedMesh;
            }
        }

        mItems.push_back( item );
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::build()
    {
        mNumVerticesCompressed      = 0;
        mNumVerticesUncompressed    = 0;
        mNumIndices16 = 0;
        mNumIndices32 = 0;

        MeshPtrMap::iterator itor = mMeshesV2.begin();
        MeshPtrMap::iterator end  = mMeshesV2.end();

        while( itor != end )
        {
            countBuffersSize( itor->first, itor->second );
            ++itor;
        }

        MappedBuffers mappedBuffers;
        FreeOnDestructor uncompressedVb( OGRE_MALLOC_SIMD( mNumVerticesUncompressed * sizeof(float) * 8u,
                                                           MEMCATEGORY_GEOMETRY ) );
        mappedBuffers.uncompressedVertexBuffer = reinterpret_cast<float*>( uncompressedVb.ptr );
        mappedBuffers.index16BufferOffset = 0u;
        mappedBuffers.index32BufferOffset = 0u;

        itor = mMeshesV2.begin();
        end  = mMeshesV2.end();

        while( itor != end )
        {
            convertMeshUncompressed( itor->first, itor->second, mappedBuffers );
            ++itor;
        }
    }
}
