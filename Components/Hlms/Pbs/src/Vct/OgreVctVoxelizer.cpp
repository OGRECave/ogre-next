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

#include "Vct/OgreVctVoxelizer.h"

#include "Vct/OgreVctMaterial.h"
#include "Vct/OgreVoxelVisualizer.h"

#include "Compute/OgreComputeTools.h"
#include "OgreHlmsCompute.h"
#include "OgreHlmsComputeJob.h"
#include "OgreItem.h"
#include "OgreLogManager.h"
#include "OgreLwString.h"
#include "OgreMaterial.h"
#include "OgreMaterialManager.h"
#include "OgreMesh2.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreProfiler.h"
#include "OgreRenderSystem.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreStringConverter.h"
#include "OgreSubMesh2.h"
#include "OgreTextureGpuManager.h"
#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreReadOnlyBufferPacked.h"
#include "Vao/OgreStagingBuffer.h"
#include "Vao/OgreUavBufferPacked.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#define TODO_deal_no_index_buffer

namespace Ogre
{
    static const size_t c_numVctProperties = 4u;
    static const size_t c_numAabCalcProperties = 2u;

    struct VctVoxelizerProp
    {
        static const IdString HasDiffuseTex;
        static const IdString HasEmissiveTex;
        static const IdString Index32bit;
        static const IdString CompressedVertexFormat;

        static const IdString *AllProps[c_numVctProperties];
    };

    const IdString VctVoxelizerProp::HasDiffuseTex = IdString( "has_diffuse_tex" );
    const IdString VctVoxelizerProp::HasEmissiveTex = IdString( "has_emissive_tex" );
    const IdString VctVoxelizerProp::Index32bit = IdString( "index_32bit" );
    const IdString VctVoxelizerProp::CompressedVertexFormat = IdString( "compressed_vertex_format" );

    const IdString *VctVoxelizerProp::AllProps[c_numVctProperties] = {
        &VctVoxelizerProp::Index32bit,
        &VctVoxelizerProp::CompressedVertexFormat,
        &VctVoxelizerProp::HasDiffuseTex,
        &VctVoxelizerProp::HasEmissiveTex,
    };
    //-------------------------------------------------------------------------
    VctVoxelizer::VctVoxelizer( IdType id, RenderSystem *renderSystem, HlmsManager *hlmsManager,
                                bool correctAreaLightShadows ) :
        VctVoxelizerSourceBase( id, renderSystem, hlmsManager ),
        mAabbWorldSpaceJob( 0 ),
        mTotalNumInstances( 0 ),
        mCpuInstanceBuffer( 0 ),
        mInstanceBuffer( 0 ),
        mInstanceBufferAsTex( 0 ),
        mVertexBufferCompressed( 0 ),
        mVertexBufferUncompressed( 0 ),
        mIndexBuffer16( 0 ),
        mIndexBuffer32( 0 ),
        mNumUncompressedPartSubMeshes16( 0 ),
        mNumUncompressedPartSubMeshes32( 0 ),
        mNumCompressedPartSubMeshes16( 0 ),
        mNumCompressedPartSubMeshes32( 0 ),
        mGpuPartitionedSubMeshes( 0 ),
        mMeshAabb( 0 ),
        mNeedsAlbedoMipmaps( correctAreaLightShadows ),
        mNeedsAllMipmaps( false ),
        mNumVerticesCompressed( 0 ),
        mNumVerticesUncompressed( 0 ),
        mNumIndices16( 0 ),
        mNumIndices32( 0 ),
        mDefaultIndexCountSplit( 2001u
                                 /*std::numeric_limits<uint32>::max()*/ ),
        mComputeTools( new ComputeTools( hlmsManager->getComputeHlms() ) ),
        mVctMaterial( new VctMaterial( id, renderSystem->getVaoManager(),
                                       Root::getSingleton().getCompositorManager2(),
                                       renderSystem->getTextureGpuManager() ) ),
        mAutoRegion( true ),
        mMaxRegion( Aabb::BOX_INFINITE )
    {
        memset( mComputeJobs, 0, sizeof( mComputeJobs ) );
        memset( mAabbCalculator, 0, sizeof( mAabbCalculator ) );
        createComputeJobs();
    }
    //-------------------------------------------------------------------------
    VctVoxelizer::~VctVoxelizer()
    {
        setDebugVisualization( DebugVisualizationNone, 0 );
        destroyVoxelTextures();
        freeBuffers( true );
        destroyInstanceBuffers();

        delete mVctMaterial;
        mVctMaterial = 0;

        delete mComputeTools;
        mComputeTools = 0;
    }
    //-------------------------------------------------------------------------
    bool VctVoxelizer::adjustIndexOffsets16( uint32 &indexStart, uint32 &numIndices )
    {
        bool adjustedIndexStart = false;
        if( indexStart & 0x01 )
        {
            adjustedIndexStart = true;
            indexStart -= 1u;
            ++numIndices;
        }
        numIndices = static_cast<uint32>( alignToNextMultiple( numIndices, 2u ) );
        return adjustedIndexStart;
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

        size_t numVariants = 1u << c_numVctProperties;

        char tmpBuffer[128];
        LwString jobName( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

        for( size_t variant = 0u; variant < numVariants; ++variant )
        {
            jobName.clear();
            jobName.a( "VCT/Voxelizer/", static_cast<uint32>( variant ) );

            mComputeJobs[variant] = hlmsCompute->findComputeJobNoThrow( jobName.c_str() );

            if( !mComputeJobs[variant] )
            {
                mComputeJobs[variant] = voxelizerJob->clone( jobName.c_str() );

                ShaderParams &glslShaderParams = mComputeJobs[variant]->getShaderParams( "glsl" );

                uint8 numTexUnits = 1u;
                if( variant &
                    ( VoxelizerJobSetting::HasDiffuseTex | VoxelizerJobSetting::HasEmissiveTex ) )
                {
                    ShaderParams::Param param;
                    param.name = "texturePool";
                    param.setManualValue( static_cast<int32>(
                        numTexUnits + mComputeJobs[variant]->_getRawGlTexSlotStart() ) );
                    glslShaderParams.mParams.push_back( param );
                    glslShaderParams.setDirty();
                    ++numTexUnits;
                }
                mComputeJobs[variant]->setNumTexUnits( numTexUnits );

                for( size_t property = 0; property < c_numVctProperties; ++property )
                {
                    const int32 propValue = variant & ( 1u << property ) ? 1 : 0;
                    mComputeJobs[variant]->setProperty( *VctVoxelizerProp::AllProps[property],
                                                        propValue );
                }
            }
        }

        HlmsComputeJob *aabbCalc = hlmsCompute->findComputeJob( "VCT/AabbCalculator" );

        const RenderSystemCapabilities *caps = mRenderSystem->getCapabilities();
        aabbCalc->setThreadsPerGroup( caps->getMaxThreadsPerThreadgroupAxis()[0], 1u, 1u );

        numVariants = 1u << c_numAabCalcProperties;
        for( size_t variant = 0u; variant < numVariants; ++variant )
        {
            jobName.clear();
            jobName.a( "VCT/AabbCalculator/", static_cast<uint32>( variant ) );

            mAabbCalculator[variant] = hlmsCompute->findComputeJobNoThrow( jobName.c_str() );

            if( !mAabbCalculator[variant] )
            {
                mAabbCalculator[variant] = aabbCalc->clone( jobName.c_str() );

                for( size_t property = 0; property < c_numAabCalcProperties; ++property )
                {
                    const int32 propValue = variant & ( 1u << property ) ? 1 : 0;
                    mAabbCalculator[variant]->setProperty( *VctVoxelizerProp::AllProps[property],
                                                           propValue );
                }
            }
        }

        mAabbWorldSpaceJob = hlmsCompute->findComputeJob( "VCT/AabbWorldSpace" );
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::clearComputeJobResources( bool calculatorDataOnly )
    {
        // Do not leave dangling pointers when destroying buffers, even if we later set them
        // with a new pointer (if malloc reuses an address and the jobs weren't cleared, we're screwed)
        if( !calculatorDataOnly )
        {
            for( size_t i = 0; i < sizeof( mComputeJobs ) / sizeof( mComputeJobs[0] ); ++i )
            {
                mComputeJobs[i]->clearUavBuffers();
                mComputeJobs[i]->clearTexBuffers();
            }
        }

        {
            const size_t numVariants = 1u << c_numAabCalcProperties;

            for( size_t i = 0; i < numVariants; ++i )
            {
                mAabbCalculator[i]->clearUavBuffers();
                mAabbCalculator[i]->clearTexBuffers();
            }
        }

        mAabbWorldSpaceJob->clearTexBuffers();
        mAabbWorldSpaceJob->clearUavBuffers();
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::countBuffersSize( const MeshPtr &mesh, QueuedMesh &queuedMesh )
    {
        const unsigned numSubmeshes = mesh->getNumSubMeshes();

        uint32 totalNumVertices = 0u;
        uint32 totalNumIndices16 = 0u;
        uint32 totalNumIndices32 = 0u;

        for( unsigned subMeshIdx = 0; subMeshIdx < numSubmeshes; ++subMeshIdx )
        {
            SubMesh *subMesh = mesh->getSubMesh( subMeshIdx );
            VertexArrayObject *vao = subMesh->mVao[VpNormal].front();

            // Count the total number of indices and vertices
            size_t vertexStart = 0u;
            size_t numVertices = vao->getBaseVertexBuffer()->getNumElements();

            uint32 vbOffset = totalNumVertices + ( queuedMesh.bCompressed ? mNumVerticesCompressed
                                                                          : mNumVerticesUncompressed );
            uint32 ibOffset = 0;
            uint32 numIndices = 0;

            uint32 *partSubMeshIdx = 0;

            // bool uses32bitIndices = false;

            IndexBufferPacked *indexBuffer = vao->getIndexBuffer();
            if( indexBuffer )
            {
                numIndices = vao->getPrimitiveCount();
                if( indexBuffer->getIndexType() == IndexBufferPacked::IT_16BIT )
                {
                    ibOffset = mNumIndices16 + totalNumIndices16;
                    totalNumIndices16 += alignToNextMultiple( numIndices, 4u );

                    partSubMeshIdx = queuedMesh.bCompressed ? &mNumCompressedPartSubMeshes16
                                                            : &mNumUncompressedPartSubMeshes16;
                }
                else
                {
                    partSubMeshIdx = queuedMesh.bCompressed ? &mNumCompressedPartSubMeshes32
                                                            : &mNumUncompressedPartSubMeshes32;
                    ibOffset = mNumIndices32 + totalNumIndices32;
                    totalNumIndices32 += numIndices;
                }

                totalNumVertices += vao->getBaseVertexBuffer()->getNumElements();
            }
            else
            {
                vertexStart = vao->getPrimitiveStart();
                numVertices = vao->getPrimitiveCount();
                numIndices = 0;
                TODO_deal_no_index_buffer;

                totalNumVertices += vao->getPrimitiveCount();
            }

            TODO_deal_no_index_buffer;

            // If the mesh has a lot of triangles, the voxelizer will test N triangles for every WxHxD
            // voxel, which can be very inefficient. By partitioning the submeshes and calculating
            // their AABBs, we can perform broadphase culling and skip a lot of triangles
            const uint32 numPartitions =
                queuedMesh.indexCountSplit == std::numeric_limits<uint32>::max()
                    ? 1u
                    : static_cast<uint32>(
                          alignToNextMultiple( numIndices, queuedMesh.indexCountSplit ) /
                          queuedMesh.indexCountSplit );
            queuedMesh.submeshes[subMeshIdx].partSubMeshes.resize( numPartitions );

            for( uint32 partition = 0u; partition < numPartitions; ++partition )
            {
                PartitionedSubMesh &partSubMesh =
                    queuedMesh.submeshes[subMeshIdx].partSubMeshes[partition];
                partSubMesh.vbOffset = vbOffset;
                partSubMesh.ibOffset = ibOffset + queuedMesh.indexCountSplit * partition;
                partSubMesh.numIndices = std::min( numIndices - queuedMesh.indexCountSplit * partition,
                                                   queuedMesh.indexCountSplit );
                partSubMesh.aabbSubMeshIdx = *partSubMeshIdx;
                *partSubMeshIdx += 1u;
            }

#ifdef STREAM_DOWNLOAD
            // Request to download the vertex buffer(s) to CPU (it will be mapped soon)
            VertexElementSemanticFullArray semanticsToDownload;
            semanticsToDownload.push_back( VES_POSITION );
            semanticsToDownload.push_back( VES_NORMAL );
            semanticsToDownload.push_back( VES_TEXTURE_COORDINATES );
            queuedMesh.submeshes[subMeshIdx].downloadHelper.queueDownload( vao, semanticsToDownload,
                                                                           vertexStart, numVertices );
#else
            queuedMesh.submeshes[subMeshIdx].downloadVertexStart = vertexStart;
            queuedMesh.submeshes[subMeshIdx].downloadNumVertices = numVertices;
#endif
        }

        if( queuedMesh.bCompressed )
            mNumVerticesCompressed += totalNumVertices;
        else
            mNumVerticesUncompressed += totalNumVertices;

        mNumIndices16 += totalNumIndices16;
        mNumIndices32 += totalNumIndices32;
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::prepareAabbCalculatorMeshData()
    {
        OgreProfile( "VctVoxelizer::prepareAabbCalculatorMeshData" );

        destroyAabbCalculatorMeshData();
        const size_t totalNumMeshes = mNumUncompressedPartSubMeshes16 + mNumUncompressedPartSubMeshes32 +
                                      mNumCompressedPartSubMeshes16 + mNumCompressedPartSubMeshes32;
        mMeshAabb = mVaoManager->createUavBuffer( totalNumMeshes, sizeof( float ) * 4u * 2u,
                                                  BB_FLAG_READONLY, 0, false );

        PartitionedSubMesh *partitionedSubMeshGpu = reinterpret_cast<PartitionedSubMesh *>(
            OGRE_MALLOC_SIMD( totalNumMeshes * sizeof( PartitionedSubMesh ), MEMCATEGORY_GEOMETRY ) );
        FreeOnDestructor partitionedSubMeshGpuPtr( partitionedSubMeshGpu );

        const size_t numVariants = 1u << c_numAabCalcProperties;

        size_t submeshStarts[numVariants];
        submeshStarts[0] = 0u;                                                  // 16-bit uncompressed
        submeshStarts[1] = submeshStarts[0] + mNumUncompressedPartSubMeshes16;  // 32-bit uncompressed
        submeshStarts[2] = submeshStarts[1] + mNumUncompressedPartSubMeshes32;  // 16-bit compressed
        submeshStarts[3] = submeshStarts[2] + mNumCompressedPartSubMeshes16;    // 32-bit compressed

        PartitionedSubMesh *partitionedSubMeshGpuPtrs[numVariants] = {
            partitionedSubMeshGpu + submeshStarts[0], partitionedSubMeshGpu + submeshStarts[1],
            partitionedSubMeshGpu + submeshStarts[2], partitionedSubMeshGpu + submeshStarts[3]
        };

        MeshPtrMap::iterator itor = mMeshesV2.begin();
        MeshPtrMap::iterator end = mMeshesV2.end();

        while( itor != end )
        {
            const Mesh *mesh = itor->first.get();
            QueuedMesh &queuedMesh = itor->second;
            const size_t numSubMeshes = queuedMesh.submeshes.size();
            for( size_t i = 0u; i < numSubMeshes; ++i )
            {
                VertexArrayObject *vao = mesh->getSubMesh( (uint16)( i ) )->mVao[VpNormal].front();
                IndexBufferPacked *indexBuffer = vao->getIndexBuffer();
                const bool is16bit = indexBuffer->getIndexType() == IndexBufferPacked::IT_16BIT;

                FastArray<PartitionedSubMesh>::iterator itPartSub =
                    queuedMesh.submeshes[i].partSubMeshes.begin();
                FastArray<PartitionedSubMesh>::iterator enPartSub =
                    queuedMesh.submeshes[i].partSubMeshes.end();

                while( itPartSub != enPartSub )
                {
                    size_t variantIdx = is16bit ? 0u : 1u;
                    if( queuedMesh.bCompressed )
                        variantIdx += 2u;

                    itPartSub->aabbSubMeshIdx += submeshStarts[variantIdx];

                    partitionedSubMeshGpuPtrs[variantIdx]->vbOffset = itPartSub->vbOffset;
                    partitionedSubMeshGpuPtrs[variantIdx]->ibOffset = itPartSub->ibOffset;
                    partitionedSubMeshGpuPtrs[variantIdx]->numIndices = itPartSub->numIndices;
                    // VCT/AabbWorldSpace compute shader needs to read aabbSubMeshIdx from
                    // InstanceBuffer::meshData.w & ~0x80000000u; not from inMeshAabb which is
                    // what we're filling right now. So set it to 0 to avoid confussion
                    partitionedSubMeshGpuPtrs[variantIdx]->aabbSubMeshIdx = 0;

                    // const bool needsAabbCalc = numSubMeshes != 1u ||
                    //                           queuedMesh.submeshes[i].partSubMeshes.size()
                    //                           != 1u;
                    // if( needsAabbCalc )
                    //    partitionedSubMeshGpu->numIndices |= 0x80000000;

                    ++itPartSub;
                    ++partitionedSubMeshGpuPtrs[variantIdx];
                }
            }
            ++itor;
        }

        OGRE_ASSERT_LOW( (size_t)( partitionedSubMeshGpuPtrs[0] - partitionedSubMeshGpu ) ==
                         mNumUncompressedPartSubMeshes16 );
        OGRE_ASSERT_LOW( (size_t)( partitionedSubMeshGpuPtrs[1] - partitionedSubMeshGpuPtrs[0] ) ==
                         mNumUncompressedPartSubMeshes32 );
        OGRE_ASSERT_LOW( (size_t)( partitionedSubMeshGpuPtrs[2] - partitionedSubMeshGpuPtrs[1] ) ==
                         mNumCompressedPartSubMeshes16 );
        OGRE_ASSERT_LOW( (size_t)( partitionedSubMeshGpuPtrs[3] - partitionedSubMeshGpuPtrs[2] ) ==
                         mNumCompressedPartSubMeshes32 );

        mGpuPartitionedSubMeshes =
            mVaoManager->createTexBuffer( PFG_RGBA32_UINT, totalNumMeshes * sizeof( PartitionedSubMesh ),
                                          BT_DEFAULT, partitionedSubMeshGpuPtr.ptr, false );
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::destroyAabbCalculatorMeshData()
    {
        // if( mGpuMeshDataDirty )
        if( mGpuPartitionedSubMeshes )
        {
            mVaoManager->destroyTexBuffer( mGpuPartitionedSubMeshes );
            mGpuPartitionedSubMeshes = 0;
        }
        if( mMeshAabb )
        {
            mVaoManager->destroyUavBuffer( mMeshAabb );
            mMeshAabb = 0;
        }

        clearComputeJobResources( true );
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::convertMeshUncompressed( const MeshPtr &mesh, QueuedMesh &queuedMesh,
                                                MappedBuffers &mappedBuffers )
    {
        OgreProfile( "VctVoxelizer::convertMeshUncompressed" );

        const unsigned numSubmeshes = mesh->getNumSubMeshes();

        for( unsigned subMeshIdx = 0; subMeshIdx < numSubmeshes; ++subMeshIdx )
        {
            SubMesh *subMesh = mesh->getSubMesh( subMeshIdx );
            VertexArrayObject *vao = subMesh->mVao[VpNormal].front();

            size_t numVertices = vao->getBaseVertexBuffer()->getNumElements();

            IndexBufferPacked *indexBuffer = vao->getIndexBuffer();
            if( indexBuffer )
            {
                if( indexBuffer->getIndexType() == IndexBufferPacked::IT_16BIT )
                {
                    uint32 indexStart = vao->getPrimitiveStart();
                    uint32 numIndices = vao->getPrimitiveCount();
                    indexBuffer->copyTo( mIndexBuffer16, mappedBuffers.index16BufferOffset >> 1u,
                                         indexStart, numIndices );
                    mappedBuffers.index16BufferOffset += alignToNextMultiple( numIndices, 4u );
                }
                else
                {
                    indexBuffer->copyTo( mIndexBuffer32, mappedBuffers.index32BufferOffset,
                                         vao->getPrimitiveStart(), vao->getPrimitiveCount() );
                    mappedBuffers.index32BufferOffset += vao->getPrimitiveCount();
                }
            }
            else
            {
                // vertexStart = vao->getPrimitiveStart();
                numVertices = vao->getPrimitiveCount();
            }

            float *RESTRICT_ALIAS uncVertexBuffer = mappedBuffers.uncompressedVertexBuffer;

#ifdef STREAM_DOWNLOAD
            VertexBufferDownloadHelper &downloadHelper = queuedMesh.submeshes[subMeshIdx].downloadHelper;
#else
            VertexBufferDownloadHelper downloadHelper;
            {
                VertexElementSemanticFullArray semanticsToDownload;
                semanticsToDownload.push_back( VES_POSITION );
                semanticsToDownload.push_back( VES_NORMAL );
                semanticsToDownload.push_back( VES_TEXTURE_COORDINATES );

                downloadHelper.queueDownload( vao, semanticsToDownload,
                                              queuedMesh.submeshes[subMeshIdx].downloadVertexStart,
                                              queuedMesh.submeshes[subMeshIdx].downloadNumVertices );
            }
#endif
            const VertexBufferDownloadHelper::DownloadData *downloadData =
                downloadHelper.getDownloadData().data();

            VertexElement2 dummy( VET_FLOAT1, VES_TEXTURE_COORDINATES );
            VertexElement2 origElements[3] = {
                downloadData[0].origElements ? *downloadData[0].origElements : dummy,
                downloadData[1].origElements ? *downloadData[1].origElements : dummy,
                downloadData[2].origElements ? *downloadData[2].origElements : dummy,
            };

            // Map the buffers we started downloading in countBuffersSize
            uint8 const *srcData[3];
            downloadHelper.map( srcData );

            for( size_t vertexIdx = 0; vertexIdx < numVertices; ++vertexIdx )
            {
                Vector4 pos =
                    downloadHelper.getVector4( srcData[0] + downloadData[0].srcOffset, origElements[0] );
                Vector3 normal( Vector3::UNIT_Y );
                Vector2 uv( Vector2::ZERO );

                if( srcData[1] )
                {
                    normal = downloadHelper.getNormal( srcData[1] + downloadData[1].srcOffset,
                                                       origElements[1] );
                }
                if( srcData[2] )
                {
                    uv = downloadHelper
                             .getVector4( srcData[2] + downloadData[2].srcOffset, origElements[2] )
                             .xy();
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

            mappedBuffers.uncompressedVertexBuffer = uncVertexBuffer;

            downloadHelper.unmap();
        }
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::addItem( Item *item, bool bCompressed, uint32 indexCountSplit )
    {
        const MeshPtr &mesh = item->getMesh();

        bool bCanAdd = true;

        if( indexCountSplit == 0u )
            indexCountSplit = mDefaultIndexCountSplit;
        if( indexCountSplit != std::numeric_limits<uint32>::max() )
            indexCountSplit = static_cast<uint32>( alignToNextMultiple( indexCountSplit, 3u ) );

        MeshPtrMap::iterator itor = mMeshesV2.find( mesh );

        const bool isNewEntry = itor == mMeshesV2.end();

        if( isNewEntry )
        {
            const unsigned numSubMeshes = mesh->getNumSubMeshes();
            for( unsigned i = 0u; i < numSubMeshes && bCanAdd; ++i )
            {
                VertexArrayObject *vao = mesh->getSubMesh( i )->mVao[VpNormal].front();
                if( !vao->getIndexBuffer() )
                {
                    bCanAdd = false;
                    LogManager::getSingleton().logMessage(
                        "WARNING: Mesh '" + mesh->getName() +
                            "' contains geometry without index buffers. This is currently not "
                            "implemented and cannot be added to VctVoxelizer. GI may not look as "
                            "expected",
                        LML_CRITICAL );
                }
            }
        }

        if( bCanAdd )
        {
            if( !bCompressed )
            {
                // Force no compression, even if the entry was already there
                QueuedMesh &queuedMesh = mMeshesV2[mesh];
                // const bool wasCompressed = queuedMesh.bCompressed;
                queuedMesh.bCompressed = false;

                if( isNewEntry )
                {
                    queuedMesh.numItems = 0u;
                    queuedMesh.indexCountSplit = indexCountSplit;
                    queuedMesh.submeshes.resize( mesh->getNumSubMeshes() );
                }

                ++queuedMesh.numItems;
            }
            else
            {
                // We can only request with compression if the entry wasn't already there
                if( isNewEntry )
                {
                    QueuedMesh queuedMesh;
                    queuedMesh.numItems = 0u;
                    queuedMesh.bCompressed = true;
                    queuedMesh.indexCountSplit = indexCountSplit;
                    queuedMesh.submeshes.resize( mesh->getNumSubMeshes() );

                    mMeshesV2[mesh] = queuedMesh;
                }
                else
                {
                    ++itor->second.numItems;
                }
            }

            mItems.push_back( item );
        }
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::removeItem( Item *item )
    {
        ItemArray::iterator itor = std::find( mItems.begin(), mItems.end(), item );
        if( itor == mItems.end() )
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "", "VctVoxelizer::removeItem" );

        const MeshPtr &mesh = item->getMesh();
        MeshPtrMap::iterator itMesh = mMeshesV2.find( mesh );
        if( itMesh == mMeshesV2.end() )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "The item was in our records but its mesh wasn't! This should be "
                         "impossible. Was the mesh ptr of the item altered manually?",
                         "VctVoxelizer::removeItem" );
        }
        --itMesh->second.numItems;
        if( !itMesh->second.numItems )
            mMeshesV2.erase( mesh );

        efficientVectorRemove( mItems, itor );
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::removeAllItems()
    {
        mItems.clear();
        mMeshesV2.clear();
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::freeBuffers( bool bForceFree )
    {
        if( mIndexBuffer16 &&
            ( bForceFree || mIndexBuffer16->getNumElements() != ( mNumIndices16 + 1u ) >> 1u ) )
        {
            mVaoManager->destroyUavBuffer( mIndexBuffer16 );
            mIndexBuffer16 = 0;
        }
        if( mIndexBuffer32 && ( bForceFree || mIndexBuffer32->getNumElements() != mNumIndices32 ) )
        {
            mVaoManager->destroyUavBuffer( mIndexBuffer32 );
            mIndexBuffer32 = 0;
        }

        if( mVertexBufferCompressed &&
            ( bForceFree || mVertexBufferCompressed->getNumElements() != mNumVerticesCompressed ) )
        {
            mVaoManager->destroyUavBuffer( mVertexBufferCompressed );
            mVertexBufferCompressed = 0;
        }

        if( mVertexBufferUncompressed &&
            ( bForceFree || mVertexBufferUncompressed->getNumElements() != mNumVerticesUncompressed ) )
        {
            mVaoManager->destroyUavBuffer( mVertexBufferUncompressed );
            mVertexBufferUncompressed = 0;
        }

        if( bForceFree )
            destroyAabbCalculatorMeshData();

        clearComputeJobResources( false );
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::buildMeshBuffers()
    {
        OgreProfile( "VctVoxelizer::buildMeshBuffers" );

        mNumVerticesCompressed = 0;
        mNumVerticesUncompressed = 0;
        mNumIndices16 = 0;
        mNumIndices32 = 0;

        mNumUncompressedPartSubMeshes16 = 0;
        mNumUncompressedPartSubMeshes32 = 0;
        mNumCompressedPartSubMeshes16 = 0;
        mNumCompressedPartSubMeshes32 = 0;

        {
            OgreProfile( "VctVoxelizer::countBuffersSize aggregated" );
            MeshPtrMap::iterator itor = mMeshesV2.begin();
            MeshPtrMap::iterator end = mMeshesV2.end();

            while( itor != end )
            {
                countBuffersSize( itor->first, itor->second );
                ++itor;
            }
        }

        freeBuffers( false );

        if( mNumIndices16 && !mIndexBuffer16 )
        {
            // D3D11 does not support 2-byte strides, so we create 4-byte buffers
            // and halve the number of indices (rounding up)
            mIndexBuffer16 = mVaoManager->createUavBuffer( ( mNumIndices16 + 1u ) >> 1u,
                                                           sizeof( uint32 ), 0, 0, false );
        }
        if( mNumIndices32 && !mIndexBuffer32 )
            mIndexBuffer32 =
                mVaoManager->createUavBuffer( mNumIndices32, sizeof( uint32 ), 0, 0, false );

        StagingBuffer *vbUncomprStagingBuffer =
            mVaoManager->getStagingBuffer( mNumVerticesUncompressed * sizeof( float ) * 8u, true );

        MappedBuffers mappedBuffers;
        mappedBuffers.uncompressedVertexBuffer = reinterpret_cast<float *>(
            vbUncomprStagingBuffer->map( mNumVerticesUncompressed * sizeof( float ) * 8u ) );
        mappedBuffers.index16BufferOffset = 0u;
        mappedBuffers.index32BufferOffset = 0u;

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_LOW
        const float *uncompressedVertexBufferStart = mappedBuffers.uncompressedVertexBuffer;
#endif

        {
            OgreProfile( "VctVoxelizer::convertMeshUncompressed aggregated" );
            MeshPtrMap::iterator itor = mMeshesV2.begin();
            MeshPtrMap::iterator end = mMeshesV2.end();

            while( itor != end )
            {
                convertMeshUncompressed( itor->first, itor->second, mappedBuffers );
                ++itor;
            }
        }

        OGRE_ASSERT_LOW(
            (size_t)( mappedBuffers.uncompressedVertexBuffer - uncompressedVertexBufferStart ) <=
            mNumVerticesUncompressed * sizeof( float ) * 8u );

        if( mNumVerticesUncompressed && !mVertexBufferUncompressed )
        {
            mVertexBufferUncompressed = mVaoManager->createUavBuffer(
                mNumVerticesUncompressed, sizeof( float ) * 8u, 0, 0, false );
        }
        vbUncomprStagingBuffer->unmap( StagingBuffer::Destination(
            mVertexBufferUncompressed, 0u, 0u, mVertexBufferUncompressed->getTotalSizeBytes() ) );

        vbUncomprStagingBuffer->removeReferenceCount();

        prepareAabbCalculatorMeshData();
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::createVoxelTextures()
    {
        if( mAlbedoVox && mAlbedoVox->getWidth() == mWidth && mAlbedoVox->getHeight() == mHeight &&
            mAlbedoVox->getDepth() == mDepth )
        {
            mAccumValVox->scheduleTransitionTo( GpuResidency::Resident );
            return;
        }

        const bool hasTypedUavs = mRenderSystem->getCapabilities()->hasCapability( RSC_TYPED_UAV_LOADS );

        if( !mAlbedoVox )
        {
            uint32 texFlags = TextureFlags::Uav;
            if( !hasTypedUavs )
                texFlags |= TextureFlags::Reinterpretable;

            if( mNeedsAlbedoMipmaps || mNeedsAllMipmaps )
                texFlags |= TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps;

            mAlbedoVox = mTextureGpuManager->createTexture(
                "VctVoxelizer" + StringConverter::toString( getId() ) + "/Albedo",
                GpuPageOutStrategy::Discard, texFlags, TextureTypes::Type3D );

            if( !mNeedsAllMipmaps )
                texFlags &= ~uint32( TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps );

            mEmissiveVox = mTextureGpuManager->createTexture(
                "VctVoxelizer" + StringConverter::toString( getId() ) + "/Emissive",
                GpuPageOutStrategy::Discard, texFlags, TextureTypes::Type3D );
            mNormalVox = mTextureGpuManager->createTexture(
                "VctVoxelizer" + StringConverter::toString( getId() ) + "/Normal",
                GpuPageOutStrategy::Discard, texFlags, TextureTypes::Type3D );

            texFlags &= ~uint32( TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps );

            mAccumValVox = mTextureGpuManager->createTexture(
                "VctVoxelizer" + StringConverter::toString( getId() ) + "/AccumVal",
                GpuPageOutStrategy::Discard, TextureFlags::NotTexture | texFlags, TextureTypes::Type3D );
        }

        TextureGpu *textures[4] = { mAlbedoVox, mEmissiveVox, mNormalVox, mAccumValVox };
        for( size_t i = 0; i < sizeof( textures ) / sizeof( textures[0] ); ++i )
            textures[i]->scheduleTransitionTo( GpuResidency::OnStorage );

        mAlbedoVox->setPixelFormat( PFG_RGBA8_UNORM );
        mEmissiveVox->setPixelFormat( PFG_RGBA8_UNORM );
        mNormalVox->setPixelFormat( PFG_R10G10B10A2_UNORM );
        if( hasTypedUavs )
            mAccumValVox->setPixelFormat( PFG_R16_UINT );
        else
            mAccumValVox->setPixelFormat( PFG_R32_UINT );

        const uint8 numMipmaps = PixelFormatGpuUtils::getMaxMipmapCount( mWidth, mHeight, mDepth );

        for( size_t i = 0; i < sizeof( textures ) / sizeof( textures[0] ); ++i )
        {
            if( textures[i] != mAccumValVox || hasTypedUavs )
                textures[i]->setResolution( mWidth, mHeight, mDepth );
            else
                textures[i]->setResolution( mWidth >> 1u, mHeight, mDepth );
            if( ( ( mNeedsAlbedoMipmaps && i == 0u ) || mNeedsAllMipmaps ) && i < 3u )
                textures[i]->setNumMipmaps( numMipmaps );
            else
                textures[i]->setNumMipmaps( 1u );
            textures[i]->scheduleTransitionTo( GpuResidency::Resident );
        }

        if( mDebugVoxelVisualizer )
        {
            setTextureToDebugVisualizer();
            mDebugVoxelVisualizer->setVisible( true );
        }
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::setRegionToVoxelize( bool autoRegion, const Aabb &regionToVoxelize,
                                            const Aabb &maxRegion )
    {
        mAutoRegion = autoRegion;
        mRegionToVoxelize = regionToVoxelize;
        mMaxRegion = maxRegion;
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::autoCalculateRegion()
    {
        if( !mAutoRegion )
            return;

        mRegionToVoxelize = Aabb::BOX_NULL;

        ItemArray::const_iterator itor = mItems.begin();
        ItemArray::const_iterator end = mItems.end();

        while( itor != end )
        {
            Item *item = *itor;
            mRegionToVoxelize.merge( item->getWorldAabb() );
            ++itor;
        }

        Vector3 minAabb = mRegionToVoxelize.getMinimum();
        Vector3 maxAabb = mRegionToVoxelize.getMaximum();

        minAabb.makeCeil( mMaxRegion.getMinimum() );
        maxAabb.makeFloor( mMaxRegion.getMaximum() );

        if( minAabb.x > maxAabb.x || minAabb.y > maxAabb.y || minAabb.z > maxAabb.z )
        {
            LogManager::getSingleton().logMessage(
                "WARNING: VctVoxelizer::autoCalculateRegion could not calculate a valid bound! GI won't "
                "be available or won't look as expected",
                LML_CRITICAL );
            mRegionToVoxelize.setExtents( Ogre::Vector3::ZERO, Ogre::Vector3::ZERO );
        }
        else
        {
            mRegionToVoxelize.setExtents( minAabb, maxAabb );
        }
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::placeItemsInBuckets()
    {
        OgreProfile( "VctVoxelizer::placeItemsInBuckets" );

        mBuckets.clear();

        ItemArray::const_iterator itor = mItems.begin();
        ItemArray::const_iterator end = mItems.end();

        while( itor != end )
        {
            Item *item = *itor;
            MeshPtrMap::const_iterator itMesh = mMeshesV2.find( item->getMesh() );

            const size_t numSubItems = item->getNumSubItems();
            for( size_t i = 0; i < numSubItems; ++i )
            {
                VoxelizerBucket bucket;
                uint32 variant = 0;
                if( itMesh->second.bCompressed )
                    variant |= VoxelizerJobSetting::CompressedVertexFormat;

                SubItem *subItem = item->getSubItem( i );

                VertexArrayObject *vao = subItem->getSubMesh()->mVao[VpNormal].front();
                if( vao->getIndexBuffer() )
                {
                    if( vao->getIndexBuffer()->getIndexType() == IndexBufferPacked::IT_32BIT )
                        variant |= VoxelizerJobSetting::Index32bit;
                }
                else
                {
                    TODO_deal_no_index_buffer;
                }

                HlmsDatablock *datablock = subItem->getDatablock();
                VctMaterial::DatablockConversionResult convResult =
                    mVctMaterial->addDatablock( datablock );

                if( convResult.hasDiffuseTex() )
                    variant |= VoxelizerJobSetting::HasDiffuseTex;
                if( convResult.hasEmissiveTex() )
                    variant |= VoxelizerJobSetting::HasEmissiveTex;

                bucket.job = mComputeJobs[variant];
                bucket.materialBuffer = convResult.constBuffer;
                bucket.needsTexPool = convResult.hasDiffuseTex() || convResult.hasEmissiveTex();
                bucket.vertexBuffer =
                    itMesh->second.bCompressed ? mVertexBufferCompressed : mVertexBufferUncompressed;
                bucket.indexBuffer =
                    ( variant & VoxelizerJobSetting::Index32bit ) ? mIndexBuffer32 : mIndexBuffer16;

                QueuedInstance queuedInstance;
                queuedInstance.movableObject = item;
                queuedInstance.materialIdx = convResult.slotIdx;

                const size_t numPartitions = itMesh->second.submeshes[i].partSubMeshes.size();
                for( size_t j = 0u; j < numPartitions; ++j )
                {
                    const PartitionedSubMesh &partSubMesh = itMesh->second.submeshes[i].partSubMeshes[j];
                    queuedInstance.vertexBufferStart = partSubMesh.vbOffset;
                    queuedInstance.indexBufferStart = partSubMesh.ibOffset;
                    queuedInstance.numIndices = partSubMesh.numIndices;
                    queuedInstance.aabbSubMeshIdx = partSubMesh.aabbSubMeshIdx;
                    queuedInstance.needsAabbUpdate = numPartitions != 1u || numSubItems != 1u;
                    mBuckets[bucket].queuedInst.push_back( queuedInstance );
                }
            }

            ++itor;
        }
    }
    //-------------------------------------------------------------------------
    size_t VctVoxelizer::countSubMeshPartitionsIn( Item *item ) const
    {
        size_t numSubMeshPartitions = 0;
        MeshPtrMap::const_iterator itMesh = mMeshesV2.find( item->getMesh() );
        OGRE_ASSERT_MEDIUM( itMesh != mMeshesV2.end() );

        QueuedSubMeshArray::const_iterator itor = itMesh->second.submeshes.begin();
        QueuedSubMeshArray::const_iterator end = itMesh->second.submeshes.end();

        while( itor != end )
        {
            numSubMeshPartitions += itor->partSubMeshes.size();
            ++itor;
        }

        return numSubMeshPartitions;
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::createInstanceBuffers()
    {
        size_t instanceCount = 0;
        ItemArray::const_iterator itor = mItems.begin();
        ItemArray::const_iterator end = mItems.end();

        while( itor != end )
        {
            instanceCount += countSubMeshPartitionsIn( *itor );
            ++itor;
        }

        const size_t structStride = sizeof( float ) * 4u * 6u;
        const size_t elementCount = alignToNextMultiple<size_t>(
            instanceCount * mOctants.size(), mAabbWorldSpaceJob->getThreadsPerGroupX() );

        if( !mInstanceBuffer || ( elementCount * structStride ) > mInstanceBuffer->getTotalSizeBytes() )
        {
            destroyInstanceBuffers();
            mInstanceBuffer = mVaoManager->createUavBuffer( elementCount, structStride,
                                                            BB_FLAG_UAV | BB_FLAG_READONLY, 0, false );
            mCpuInstanceBuffer = reinterpret_cast<float *>(
                OGRE_MALLOC_SIMD( elementCount * structStride, MEMCATEGORY_GENERAL ) );
            mInstanceBufferAsTex = mInstanceBuffer->getAsReadOnlyBufferView();
        }
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::destroyInstanceBuffers()
    {
        if( mInstanceBuffer )
        {
            mVaoManager->destroyUavBuffer( mInstanceBuffer );
            mInstanceBuffer = 0;
            mInstanceBufferAsTex = 0;

            OGRE_FREE_SIMD( mCpuInstanceBuffer, MEMCATEGORY_GENERAL );
            mCpuInstanceBuffer = 0;
        }

        clearComputeJobResources( false );
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::fillInstanceBuffers()
    {
        OgreProfile( "VctVoxelizer::fillInstanceBuffers" );

        createInstanceBuffers();

        //        float * RESTRICT_ALIAS instanceBuffer =
        //                reinterpret_cast<float*>( mInstanceBuffer->map( 0,
        //                mInstanceBuffer->getNumElements() ) );
        float *RESTRICT_ALIAS instanceBuffer = reinterpret_cast<float *>( mCpuInstanceBuffer );
        const float *instanceBufferStart = instanceBuffer;
        const FastArray<Octant>::const_iterator begin = mOctants.begin();
        FastArray<Octant>::const_iterator itor = begin;
        FastArray<Octant>::const_iterator end = mOctants.end();

        while( itor != end )
        {
            const Aabb octantAabb = itor->region;
            VoxelizerBucketMap::iterator itBucket = mBuckets.begin();
            VoxelizerBucketMap::iterator enBucket = mBuckets.end();

            while( itBucket != enBucket )
            {
                uint32 numInstancesAfterCulling = 0u;
                FastArray<QueuedInstance>::const_iterator itQueuedInst =
                    itBucket->second.queuedInst.begin();
                FastArray<QueuedInstance>::const_iterator enQueuedInst =
                    itBucket->second.queuedInst.end();

                while( itQueuedInst != enQueuedInst )
                {
                    const QueuedInstance &instance = *itQueuedInst;
                    Aabb worldAabb = instance.movableObject->getWorldAabb();

                    // Perform culling against this octant.
                    if( octantAabb.intersects( worldAabb ) )
                    {
                        const Matrix4 &fullTransform =
                            instance.movableObject->_getParentNodeFullTransform();
                        for( size_t i = 0; i < 12u; ++i )
                            *instanceBuffer++ = static_cast<float>( fullTransform[0][i] );

                        *instanceBuffer++ = worldAabb.mCenter.x;
                        *instanceBuffer++ = worldAabb.mCenter.y;
                        *instanceBuffer++ = worldAabb.mCenter.z;
                        *instanceBuffer++ = 0.0f;

#define AS_U32PTR( x ) reinterpret_cast<uint32 * RESTRICT_ALIAS>( x )

                        *instanceBuffer++ = worldAabb.mHalfSize.x;
                        *instanceBuffer++ = worldAabb.mHalfSize.y;
                        *instanceBuffer++ = worldAabb.mHalfSize.z;
                        *AS_U32PTR( instanceBuffer ) = instance.materialIdx;
                        ++instanceBuffer;

                        uint32 aabbSubMeshIdx = instance.aabbSubMeshIdx;
                        if( instance.needsAabbUpdate )
                            aabbSubMeshIdx |= 0x80000000;

                        *AS_U32PTR( instanceBuffer ) = instance.vertexBufferStart;
                        ++instanceBuffer;
                        *AS_U32PTR( instanceBuffer ) = instance.indexBufferStart;
                        ++instanceBuffer;
                        *AS_U32PTR( instanceBuffer ) = instance.numIndices;
                        ++instanceBuffer;
                        *AS_U32PTR( instanceBuffer ) = aabbSubMeshIdx;
                        ++instanceBuffer;

#undef AS_U32PTR

                        ++numInstancesAfterCulling;
                    }

                    ++itQueuedInst;
                }

                if( numInstancesAfterCulling > 0u )
                {
                    const uint32 octantIdx = static_cast<uint32>( itor - begin );
                    itBucket->second.numInstancesAfterCulling[octantIdx] = numInstancesAfterCulling;
                }

                ++itBucket;
            }

            ++itor;
        }

        OGRE_ASSERT_LOW( (size_t)( instanceBuffer - instanceBufferStart ) * sizeof( float ) <=
                         mInstanceBuffer->getTotalSizeBytes() );

        mTotalNumInstances =
            static_cast<uint32>( ( instanceBuffer - instanceBufferStart ) / ( 4u * ( 3u + 3u ) ) );

        // Fill the remaining bytes with 0 so that mAabbWorldSpaceJob ignores those
        memset( instanceBuffer, 0,
                mInstanceBuffer->getTotalSizeBytes() -
                    ( static_cast<size_t>( instanceBuffer - instanceBufferStart ) * sizeof( float ) ) );
        //        mInstanceBuffer->unmap( UO_UNMAP_ALL );
        mInstanceBuffer->upload( mCpuInstanceBuffer, 0u, mInstanceBuffer->getNumElements() );
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::computeMeshAabbs()
    {
        OgreProfile( "VctVoxelizer::computeMeshAabbs" );
        HlmsCompute *hlmsCompute = mHlmsManager->getComputeHlms();

        const size_t numVariants = 1u << c_numAabCalcProperties;

        OGRE_STATIC_ASSERT( sizeof( mAabbCalculator ) / sizeof( mAabbCalculator[0] ) == numVariants );

        const uint32 numMeshes[numVariants] = { mNumUncompressedPartSubMeshes16,
                                                mNumUncompressedPartSubMeshes32,
                                                mNumCompressedPartSubMeshes16,
                                                mNumCompressedPartSubMeshes32 };

        ShaderParams::Param paramMeshRange;
        paramMeshRange.name = "meshStart_meshEnd";

        uint32 meshStart = 0u;

        OgreProfileGpuBegin( "VCT Mesh AABB calculation" );

        for( size_t i = 0; i < numVariants; ++i )
        {
            if( numMeshes[i] == 0u )
                continue;

            const bool compressedVf = ( i & VoxelizerJobSetting::CompressedVertexFormat ) != 0;
            const bool hasIndices32 = ( i & VoxelizerJobSetting::Index32bit ) != 0;

            DescriptorSetUav::BufferSlot bufferSlot( DescriptorSetUav::BufferSlot::makeEmpty() );
            bufferSlot.buffer = compressedVf ? mVertexBufferCompressed : mVertexBufferUncompressed;
            bufferSlot.access = ResourceAccess::Read;
            mAabbCalculator[i]->_setUavBuffer( 0, bufferSlot );
            bufferSlot.buffer = hasIndices32 ? mIndexBuffer32 : mIndexBuffer16;
            bufferSlot.access = ResourceAccess::Read;
            mAabbCalculator[i]->_setUavBuffer( 1, bufferSlot );
            bufferSlot.buffer = mMeshAabb;
            bufferSlot.access = ResourceAccess::Write;
            mAabbCalculator[i]->_setUavBuffer( 2, bufferSlot );

            DescriptorSetTexture2::BufferSlot texBufSlot(
                DescriptorSetTexture2::BufferSlot::makeEmpty() );
            texBufSlot.buffer = mGpuPartitionedSubMeshes;
            mAabbCalculator[i]->setTexBuffer( 0, texBufSlot );

            uint32 meshRange[2] = { meshStart, meshStart + numMeshes[i] };

            paramMeshRange.setManualValue( meshRange, 2u );

            ShaderParams &shaderParams = mAabbCalculator[i]->getShaderParams( "default" );
            shaderParams.mParams.clear();
            shaderParams.mParams.push_back( paramMeshRange );
            shaderParams.setDirty();

            mAabbCalculator[i]->analyzeBarriers( mResourceTransitions );
            mRenderSystem->executeResourceTransition( mResourceTransitions );
            hlmsCompute->dispatch( mAabbCalculator[i], 0, 0 );
            meshStart += numMeshes[i];
        }

        OgreProfileGpuEnd( "VCT Mesh AABB calculation" );

        DescriptorSetUav::BufferSlot bufferSlot( DescriptorSetUav::BufferSlot::makeEmpty() );
        bufferSlot.buffer = mInstanceBuffer;
        bufferSlot.access = ResourceAccess::ReadWrite;
        mAabbWorldSpaceJob->_setUavBuffer( 0, bufferSlot );

        DescriptorSetTexture2::BufferSlot texBufSlot( DescriptorSetTexture2::BufferSlot::makeEmpty() );
        texBufSlot.buffer = mMeshAabb->getAsReadOnlyBufferView();
        mAabbWorldSpaceJob->setTexBuffer( 0, texBufSlot );

        const uint32 threadsPerGroupX = mAabbWorldSpaceJob->getThreadsPerGroupX();
        mAabbWorldSpaceJob->setNumThreadGroups(
            ( mTotalNumInstances + threadsPerGroupX - 1u ) / threadsPerGroupX, 1u, 1u );

        OgreProfileGpuBegin( "VCT AABB local to world space conversion" );
        mAabbWorldSpaceJob->analyzeBarriers( mResourceTransitions );
        mRenderSystem->executeResourceTransition( mResourceTransitions );
        hlmsCompute->dispatch( mAabbWorldSpaceJob, 0, 0 );
        OgreProfileGpuEnd( "VCT AABB local to world space conversion" );
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::dividideOctants( uint32 numOctantsX, uint32 numOctantsY, uint32 numOctantsZ )
    {
        mOctants.clear();
        mOctants.reserve( numOctantsX * numOctantsY * numOctantsZ );

        OGRE_ASSERT_LOW( mWidth % numOctantsX == 0 );
        OGRE_ASSERT_LOW( mHeight % numOctantsY == 0 );
        OGRE_ASSERT_LOW( mDepth % numOctantsZ == 0 );

        Octant octant;
        octant.width = mWidth / numOctantsX;
        octant.height = mHeight / numOctantsY;
        octant.depth = mDepth / numOctantsZ;

        const Vector3 voxelOrigin = mRegionToVoxelize.getMinimum();
        const Vector3 voxelCellSize = mRegionToVoxelize.getSize() /
                                      Vector3( (Real)numOctantsX, (Real)numOctantsY, (Real)numOctantsZ );

        for( uint32 x = 0u; x < numOctantsX; ++x )
        {
            octant.x = x * octant.width;
            for( uint32 y = 0u; y < numOctantsY; ++y )
            {
                octant.y = y * octant.height;
                for( uint32 z = 0u; z < numOctantsZ; ++z )
                {
                    octant.z = z * octant.depth;

                    Vector3 octantOrigin = Vector3( (Real)x, (Real)y, (Real)z ) * voxelCellSize;
                    octantOrigin += voxelOrigin;
                    octant.region.setExtents( octantOrigin, octantOrigin + voxelCellSize );
                    mOctants.push_back( octant );
                }
            }
        }
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::clearVoxels()
    {
        OgreProfileGpuBegin( "VCT Voxelization Clear" );
        float fClearValue[4];
        uint32 uClearValue[4];
        float fClearNormals[4];
        memset( fClearValue, 0, sizeof( fClearValue ) );
        memset( uClearValue, 0, sizeof( uClearValue ) );
        fClearNormals[0] = 0.5f;
        fClearNormals[1] = 0.5f;
        fClearNormals[2] = 0.5f;
        fClearNormals[3] = 0.0f;

        mResourceTransitions.clear();
        mComputeTools->prepareForUavClear( mResourceTransitions, mAlbedoVox );
        mComputeTools->prepareForUavClear( mResourceTransitions, mEmissiveVox );
        mComputeTools->prepareForUavClear( mResourceTransitions, mNormalVox );
        mComputeTools->prepareForUavClear( mResourceTransitions, mAccumValVox );
        mRenderSystem->executeResourceTransition( mResourceTransitions );

        mComputeTools->clearUavFloat( mAlbedoVox, fClearValue );
        mComputeTools->clearUavFloat( mEmissiveVox, fClearValue );
        mComputeTools->clearUavFloat( mNormalVox, fClearNormals );
        mComputeTools->clearUavUint( mAccumValVox, uClearValue );
        OgreProfileGpuEnd( "VCT Voxelization Clear" );
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::setResolution( uint32 width, uint32 height, uint32 depth )
    {
        destroyVoxelTextures();
        mWidth = width;
        mHeight = height;
        mDepth = depth;
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::build( SceneManager *sceneManager )
    {
        OgreProfile( "VctVoxelizer::build" );
        OgreProfileGpuBegin( "VCT build" );

        OGRE_ASSERT_LOW( !mOctants.empty() );

        mRenderSystem->endRenderPassDescriptor();

        if( mItems.empty() )
        {
            createVoxelTextures();
            clearVoxels();
            return;
        }

        buildMeshBuffers();

        createVoxelTextures();

        mVctMaterial->initTempResources( sceneManager );
        placeItemsInBuckets();
        mVctMaterial->destroyTempResources();

        fillInstanceBuffers();

        computeMeshAabbs();

        const bool hasTypedUavs = mRenderSystem->getCapabilities()->hasCapability( RSC_TYPED_UAV_LOADS );

        for( size_t i = 0; i < sizeof( mComputeJobs ) / sizeof( mComputeJobs[0] ); ++i )
        {
            const bool compressedVf = ( i & VoxelizerJobSetting::CompressedVertexFormat ) != 0;
            const bool hasIndices32 = ( i & VoxelizerJobSetting::Index32bit ) != 0;

            DescriptorSetUav::BufferSlot bufferSlot( DescriptorSetUav::BufferSlot::makeEmpty() );
            bufferSlot.buffer = compressedVf ? mVertexBufferCompressed : mVertexBufferUncompressed;
            bufferSlot.access = ResourceAccess::Read;
            mComputeJobs[i]->_setUavBuffer( 0, bufferSlot );
            bufferSlot.buffer = hasIndices32 ? mIndexBuffer32 : mIndexBuffer16;
            bufferSlot.access = ResourceAccess::Read;
            mComputeJobs[i]->_setUavBuffer( 1, bufferSlot );

            DescriptorSetUav::TextureSlot uavSlot( DescriptorSetUav::TextureSlot::makeEmpty() );
            uavSlot.access = ResourceAccess::ReadWrite;

            uavSlot.texture = mAlbedoVox;
            if( hasTypedUavs )
                uavSlot.pixelFormat = mAlbedoVox->getPixelFormat();
            else
                uavSlot.pixelFormat = PFG_R32_UINT;
            uavSlot.access = ResourceAccess::ReadWrite;
            mComputeJobs[i]->_setUavTexture( 2, uavSlot );

            uavSlot.texture = mNormalVox;
            if( hasTypedUavs )
                uavSlot.pixelFormat = mNormalVox->getPixelFormat();
            else
                uavSlot.pixelFormat = PFG_R32_UINT;
            uavSlot.access = ResourceAccess::ReadWrite;
            mComputeJobs[i]->_setUavTexture( 3, uavSlot );

            uavSlot.texture = mEmissiveVox;
            if( hasTypedUavs )
                uavSlot.pixelFormat = mEmissiveVox->getPixelFormat();
            else
                uavSlot.pixelFormat = PFG_R32_UINT;
            uavSlot.access = ResourceAccess::ReadWrite;
            mComputeJobs[i]->_setUavTexture( 4, uavSlot );

            uavSlot.texture = mAccumValVox;
            uavSlot.pixelFormat = mAccumValVox->getPixelFormat();
            uavSlot.access = ResourceAccess::ReadWrite;
            mComputeJobs[i]->_setUavTexture( 5, uavSlot );

            DescriptorSetTexture2::BufferSlot texBufSlot(
                DescriptorSetTexture2::BufferSlot::makeEmpty() );
            texBufSlot.buffer = mInstanceBufferAsTex;
            mComputeJobs[i]->setTexBuffer( 0, texBufSlot );
        }

        HlmsCompute *hlmsCompute = mHlmsManager->getComputeHlms();
        clearVoxels();

        const uint32 *threadsPerGroup = mComputeJobs[0]->getThreadsPerGroup();

        ShaderParams::Param paramInstanceRange;
        ShaderParams::Param paramVoxelOrigin;
        ShaderParams::Param paramVoxelCellSize;
        ShaderParams::Param paramVoxelPixelOrigin;

        paramInstanceRange.name = "instanceStart_instanceEnd";
        paramVoxelOrigin.name = "voxelOrigin";
        paramVoxelCellSize.name = "voxelCellSize";
        paramVoxelPixelOrigin.name = "voxelPixelOrigin";

        paramVoxelCellSize.setManualValue( getVoxelCellSize() );

        uint32 instanceStart = 0;

        OgreProfileGpuBegin( "VCT Voxelization Jobs" );

        const FastArray<Octant>::const_iterator begin = mOctants.begin();
        FastArray<Octant>::const_iterator itor = begin;
        FastArray<Octant>::const_iterator end = mOctants.end();

        while( itor != end )
        {
            const Octant &octant = *itor;
            VoxelizerBucketMap::const_iterator itBucket = mBuckets.begin();
            VoxelizerBucketMap::const_iterator enBucket = mBuckets.end();

            while( itBucket != enBucket )
            {
                const uint32 octantIdx = static_cast<uint32>( itor - begin );
                const BucketData::InstancesPerOctantIdxMap::const_iterator itNumInstances =
                    itBucket->second.numInstancesAfterCulling.find( octantIdx );

                if( itNumInstances != itBucket->second.numInstancesAfterCulling.end() )
                {
                    const VoxelizerBucket &bucket = itBucket->first;
                    bucket.job->setNumThreadGroups( std::max( 1u, octant.width / threadsPerGroup[0] ),
                                                    std::max( 1u, octant.height / threadsPerGroup[1] ),
                                                    std::max( 1u, octant.depth / threadsPerGroup[2] ) );

                    bucket.job->setConstBuffer( 0, bucket.materialBuffer );

                    uint8 texUnit = 1u;

                    DescriptorSetTexture2::TextureSlot texSlot(
                        DescriptorSetTexture2::TextureSlot::makeEmpty() );
                    if( bucket.needsTexPool )
                    {
                        texSlot.texture = mVctMaterial->getTexturePool();
                        HlmsSamplerblock samplerblock;
                        samplerblock.setAddressingMode( TAM_WRAP );
                        bucket.job->setTexture( texUnit, texSlot, &samplerblock );
                        ++texUnit;
                    }

                    const uint32 numInstancesInBucket = itNumInstances->second;
                    const uint32 instanceRange[2] = { instanceStart,
                                                      instanceStart + numInstancesInBucket };
                    const uint32 voxelPixelOrigin[3] = { octant.x, octant.y, octant.z };

                    paramInstanceRange.setManualValue( instanceRange, 2u );
                    paramVoxelOrigin.setManualValue( octant.region.getMinimum() );
                    paramVoxelPixelOrigin.setManualValue( voxelPixelOrigin, 3u );

                    ShaderParams &shaderParams = bucket.job->getShaderParams( "default" );
                    shaderParams.mParams.clear();
                    shaderParams.mParams.push_back( paramInstanceRange );
                    shaderParams.mParams.push_back( paramVoxelOrigin );
                    shaderParams.mParams.push_back( paramVoxelCellSize );
                    shaderParams.mParams.push_back( paramVoxelPixelOrigin );
                    shaderParams.setDirty();

                    bucket.job->analyzeBarriers( mResourceTransitions );
                    mRenderSystem->executeResourceTransition( mResourceTransitions );
                    hlmsCompute->dispatch( bucket.job, 0, 0 );

                    instanceStart += numInstancesInBucket;
                }

                ++itBucket;
            }

            ++itor;
        }

        OgreProfileGpuEnd( "VCT Voxelization Jobs" );

        // This texture is no longer needed, it's not used for the injection phase. Save memory.
        mAccumValVox->scheduleTransitionTo( GpuResidency::OnStorage );

        if( mNeedsAlbedoMipmaps || mNeedsAllMipmaps )
            mAlbedoVox->_autogenerateMipmaps();
        if( mNeedsAllMipmaps )
        {
            mEmissiveVox->_autogenerateMipmaps();
            mNormalVox->_autogenerateMipmaps();
        }

        OgreProfileGpuEnd( "VCT build" );
    }
}  // namespace Ogre
