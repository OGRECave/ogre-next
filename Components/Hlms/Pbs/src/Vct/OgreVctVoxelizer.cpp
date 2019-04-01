
#include "Vct/OgreVctVoxelizer.h"

#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreSubMesh2.h"

#include "Vao/OgreVertexArrayObject.h"

#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreUavBufferPacked.h"

#include "Vao/OgreVaoManager.h"

#include "Vao/OgreStagingBuffer.h"

#include "OgreHlmsCompute.h"
#include "OgreHlmsComputeJob.h"
#include "OgreLwString.h"

namespace Ogre
{
    static const size_t c_numVctProperties = 5u;

    struct VctVoxelizerProp
    {
        static const IdString HasDiffuseTex;
        static const IdString HasEmissiveTex;
        static const IdString EmissiveIsDiffuseTex;
        static const IdString Index32bit;
        static const IdString CompressedVertexFormat;

        static const IdString* AllProps[c_numVctProperties];
    };

    const IdString VctVoxelizerProp::HasDiffuseTex          = IdString( "has_diffuse_tex" );
    const IdString VctVoxelizerProp::HasEmissiveTex         = IdString( "has_emissive_tex" );
    const IdString VctVoxelizerProp::EmissiveIsDiffuseTex   = IdString( "emissive_is_diffuse_tex" );
    const IdString VctVoxelizerProp::Index32bit             = IdString( "index_32bit" );
    const IdString VctVoxelizerProp::CompressedVertexFormat = IdString( "compressed_vertex_format" );

    const IdString* VctVoxelizerProp::AllProps[c_numVctProperties] =
    {
        &VctVoxelizerProp::HasDiffuseTex,
        &VctVoxelizerProp::HasEmissiveTex,
        &VctVoxelizerProp::EmissiveIsDiffuseTex,
        &VctVoxelizerProp::Index32bit,
        &VctVoxelizerProp::CompressedVertexFormat,
    };
    //-------------------------------------------------------------------------
    VctVoxelizer::VctVoxelizer( VaoManager *vaoManager, HlmsManager *hlmsManager ) :
        mVaoManager( vaoManager ),
        mHlmsManager( hlmsManager )
    {
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::createComputeJobs()
    {
        HlmsCompute *hlmsCompute = mHlmsManager->getComputeHlms();

        HlmsComputeJob *voxelizerJob = hlmsCompute->findComputeJobNoThrow( "VCT/Voxelizer" );

#if OGRE_NO_JSON
        OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                     "To use VctVoxelizer, Ogre must be build with JSON support "
                     "and you must include the resources bundled at "
                     "Samples/Media/VCT",
                     "VctVoxelizer::createComputeJobs" );
#endif
        if( !voxelizerJob )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "To use VctVoxelizer, you must include the resources bundled at "
                         "Samples/Media/VCT\n"
                         "Could not find VCT/Voxelizer",
                         "VctVoxelizer::createComputeJobs" );
        }

        const size_t numVariants = 1u << c_numVctProperties;

        char tmpBuffer[128];
        LwString jobName( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

        for( size_t variant=0u; variant<numVariants; ++variant )
        {
            jobName.clear();
            jobName.a( "VCT/Voxelizer/", static_cast<uint32>( variant ) );

            mComputeJobs[variant] = hlmsCompute->findComputeJobNoThrow( jobName.c_str() );

            if( !mComputeJobs[variant] )
            {
                mComputeJobs[variant] = voxelizerJob->clone( jobName.c_str() );

                for( size_t property=0; property<c_numVctProperties; ++property )
                {
                    const int32 propValue = variant & (1u << property) ? 1 : 0;
                    voxelizerJob->setProperty( *VctVoxelizerProp::AllProps[property], propValue );
                }
            }
        }
    }
    //-------------------------------------------------------------------------
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
    void VctVoxelizer::freeBuffers(void)
    {
        if( mIndexBuffer16 && mIndexBuffer16->getNumElements() != mNumIndices16 )
        {
            mVaoManager->destroyUavBuffer( mIndexBuffer16 );
            mIndexBuffer16 = 0;
        }
        if( mIndexBuffer32 && mIndexBuffer32->getNumElements() != mNumIndices32 )
        {
            mVaoManager->destroyUavBuffer( mIndexBuffer32 );
            mIndexBuffer32 = 0;
        }

        if( mVertexBufferCompressed )
        {
            mVaoManager->destroyUavBuffer( mVertexBufferCompressed );
            mVertexBufferCompressed = 0;
        }

        if( mVertexBufferUncompressed )
        {
            mVaoManager->destroyUavBuffer( mVertexBufferUncompressed );
            mVertexBufferUncompressed = 0;
        }
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::buildMeshBuffers(void)
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

        freeBuffers();

        if( mNumIndices16 )
            mIndexBuffer16 = mVaoManager->createUavBuffer( mNumIndices16, sizeof(uint16), 0, 0, false );
        if( mNumIndices32 )
            mIndexBuffer32 = mVaoManager->createUavBuffer( mNumIndices32, sizeof(uint32), 0, 0, false );

        StagingBuffer *vbUncomprStagingBuffer =
                mVaoManager->getStagingBuffer( mNumVerticesUncompressed * sizeof(float) * 8u, true );

        MappedBuffers mappedBuffers;
        mappedBuffers.uncompressedVertexBuffer =
                reinterpret_cast<float*>( vbUncomprStagingBuffer->map( mNumVerticesUncompressed *
                                                                       sizeof(float) * 8u ) );

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

        mVertexBufferUncompressed = mVaoManager->createUavBuffer( mNumVerticesUncompressed,
                                                                  sizeof(float) * 8u,
                                                                  0, 0, false );
        vbUncomprStagingBuffer->unmap( StagingBuffer::Destination(
                                           mVertexBufferUncompressed, 0u, 0u,
                                           mVertexBufferUncompressed->getTotalSizeBytes() ) );

        vbUncomprStagingBuffer->removeReferenceCount();
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::build(void)
    {
        buildMeshBuffers();
    }
}
