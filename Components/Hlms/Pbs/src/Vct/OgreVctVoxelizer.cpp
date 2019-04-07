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

#include "Vct/OgreVctVoxelizer.h"
#include "Vct/OgreVctMaterial.h"

#include "OgreRenderSystem.h"

#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreSubMesh2.h"

#include "Vao/OgreVertexArrayObject.h"

#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreTexBufferPacked.h"
#include "Vao/OgreUavBufferPacked.h"

#include "Vao/OgreVaoManager.h"

#include "Vao/OgreStagingBuffer.h"

#include "Compute/OgreComputeTools.h"

#include "OgreHlmsCompute.h"
#include "OgreHlmsComputeJob.h"
#include "OgreLwString.h"

#include "OgreTextureGpuManager.h"
#include "OgreStringConverter.h"

#define TODO_deal_no_index_buffer
#define TODO_clear_voxels
#define TODO_add_barrier

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
    VctVoxelizer::VctVoxelizer( IdType id, RenderSystem *renderSystem, HlmsManager *hlmsManager ) :
        IdObject( id ),
        mCpuInstanceBuffer( 0 ),
        mInstanceBuffer( 0 ),
        mVertexBufferCompressed( 0 ),
        mVertexBufferUncompressed( 0 ),
        mIndexBuffer16( 0 ),
        mIndexBuffer32( 0 ),
        mNumVerticesCompressed( 0 ),
        mNumVerticesUncompressed( 0 ),
        mNumIndices16( 0 ),
        mNumIndices32( 0 ),
        mAlbedoVox( 0 ),
        mEmissiveVox( 0 ),
        mNormalVox( 0 ),
        mAccumValVox( 0 ),
        mRenderSystem( renderSystem ),
        mVaoManager( renderSystem->getVaoManager() ),
        mHlmsManager( hlmsManager ),
        mTextureGpuManager( renderSystem->getTextureGpuManager() ),
        mComputeTools( new ComputeTools( hlmsManager->getComputeHlms() ) ),
        mVctMaterial( new VctMaterial( renderSystem->getVaoManager() ) ),
        mWidth( 128u ),
        mHeight( 128u ),
        mDepth( 128u ),
        mAutoRegion( true ),
        mRegionToVoxelize( Aabb::BOX_ZERO ),
        mMaxRegion( Aabb::BOX_INFINITE )
    {
        memset( mComputeJobs, 0, sizeof(mComputeJobs) );
        createComputeJobs();
    }
    //-------------------------------------------------------------------------
    VctVoxelizer::~VctVoxelizer()
    {
        destroyVoxelTextures();
        freeBuffers();
        destroyInstanceBuffers();

        delete mVctMaterial;
        mVctMaterial = 0;

        delete mComputeTools;
        mComputeTools = 0;
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

                ShaderParams &glslShaderParams = mComputeJobs[variant]->getShaderParams( "glsl" );

                uint8 numTexUnits = 1u;
                if( variant & VoxelizerJobSetting::HasDiffuseTex )
                {
                    ShaderParams::Param param;
                    param.name = "diffuseTex";
                    param.setManualValue( static_cast<int32>( numTexUnits ) );
                    glslShaderParams.mParams.push_back( param );
                    glslShaderParams.setDirty();
                    ++numTexUnits;
                }
                if( variant & VoxelizerJobSetting::HasEmissiveTex )
                {
                    ShaderParams::Param param;
                    param.name = "emissiveTex";
                    param.setManualValue( static_cast<int32>( numTexUnits ) );
                    glslShaderParams.mParams.push_back( param );
                    glslShaderParams.setDirty();
                    ++numTexUnits;
                }
                mComputeJobs[variant]->setNumTexUnits( numTexUnits );

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

        uint32 totalNumVertices = 0u;
        uint32 totalNumIndices16 = 0u;
        uint32 totalNumIndices32 = 0u;

        for( uint16 subMeshIdx=0; subMeshIdx<numSubmeshes; ++subMeshIdx )
        {
            SubMesh *subMesh = mesh->getSubMesh( subMeshIdx );
            VertexArrayObject *vao = subMesh->mVao[VpNormal].front();

            //Count the total number of indices and vertices
            size_t vertexStart = 0u;
            size_t numVertices = vao->getBaseVertexBuffer()->getNumElements();

            uint32 vbOffset = totalNumVertices + (queuedMesh.bCompressed ? mNumVerticesCompressed :
                                                                           mNumVerticesUncompressed);
            uint32 ibOffset = 0;
            uint32 numIndices = 0;

            bool uses32bitIndices = false;

            IndexBufferPacked *indexBuffer = vao->getIndexBuffer();
            if( indexBuffer )
            {
                numIndices = vao->getPrimitiveCount();
                if( indexBuffer->getIndexType() == IndexBufferPacked::IT_16BIT )
                {
                    uses32bitIndices = false;
                    ibOffset = mNumIndices16 + totalNumIndices16;
                    totalNumIndices16 += numIndices;
                }
                else
                {
                    ibOffset = mNumIndices32 + totalNumIndices32;
                    totalNumIndices32 += numIndices;
                }

                totalNumVertices += vao->getBaseVertexBuffer()->getNumElements();
            }
            else
            {
                vertexStart = vao->getPrimitiveStart();
                numVertices = vao->getPrimitiveCount();
                numIndices = 0; TODO_deal_no_index_buffer;

                totalNumVertices += vao->getPrimitiveCount();
            }

            TODO_deal_no_index_buffer;

            queuedMesh.submeshes[subMeshIdx].vbOffset = vbOffset;
            queuedMesh.submeshes[subMeshIdx].ibOffset = ibOffset;
            queuedMesh.submeshes[subMeshIdx].numIndices = numIndices;

            //Request to download the vertex buffer(s) to CPU (it will be mapped soon)
            VertexElementSemanticFullArray semanticsToDownload;
            semanticsToDownload.push_back( VES_POSITION );
            semanticsToDownload.push_back( VES_NORMAL );
            semanticsToDownload.push_back( VES_TEXTURE_COORDINATES );
            queuedMesh.submeshes[subMeshIdx].downloadHelper.queueDownload( vao, semanticsToDownload,
                                                                           vertexStart, numVertices );
        }

        if( queuedMesh.bCompressed )
            mNumVerticesCompressed += totalNumVertices;
        else
            mNumVerticesUncompressed += totalNumVertices;

        mNumIndices16 += totalNumIndices16;
        mNumIndices32 += totalNumIndices32;
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
        mappedBuffers.index16BufferOffset = 0u;
        mappedBuffers.index32BufferOffset = 0u;

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_LOW
        const float *uncompressedVertexBufferStart = mappedBuffers.uncompressedVertexBuffer;
#endif

        itor = mMeshesV2.begin();
        end  = mMeshesV2.end();

        while( itor != end )
        {
            convertMeshUncompressed( itor->first, itor->second, mappedBuffers );
            ++itor;
        }

        OGRE_ASSERT_LOW( (size_t)(mappedBuffers.uncompressedVertexBuffer -
                                  uncompressedVertexBufferStart) <=
                         mNumVerticesUncompressed * sizeof(float) * 8u );

        mVertexBufferUncompressed = mVaoManager->createUavBuffer( mNumVerticesUncompressed,
                                                                  sizeof(float) * 8u,
                                                                  0, 0, false );
        vbUncomprStagingBuffer->unmap( StagingBuffer::Destination(
                                           mVertexBufferUncompressed, 0u, 0u,
                                           mVertexBufferUncompressed->getTotalSizeBytes() ) );

        vbUncomprStagingBuffer->removeReferenceCount();
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::createVoxelTextures(void)
    {
        if( mAlbedoVox &&
            mAlbedoVox->getWidth() == mWidth &&
            mAlbedoVox->getHeight() == mHeight &&
            mAlbedoVox->getDepth() == mDepth )
        {
            mAccumValVox->scheduleTransitionTo( GpuResidency::Resident );
            return;
        }

        if( !mAlbedoVox )
        {
            mAlbedoVox = mTextureGpuManager->createTexture( "VctVoxelizer" +
                                                            StringConverter::toString( getId() ) +
                                                            "/Albedo",
                                                            GpuPageOutStrategy::Discard,
                                                            TextureFlags::Uav, TextureTypes::Type3D );
            mEmissiveVox = mTextureGpuManager->createTexture( "VctVoxelizer" +
                                                              StringConverter::toString( getId() ) +
                                                              "/Emissive",
                                                              GpuPageOutStrategy::Discard,
                                                              TextureFlags::Uav, TextureTypes::Type3D );
            mNormalVox = mTextureGpuManager->createTexture( "VctVoxelizer" +
                                                            StringConverter::toString( getId() ) +
                                                            "/Normal",
                                                            GpuPageOutStrategy::Discard,
                                                            TextureFlags::Uav, TextureTypes::Type3D );
            mAccumValVox = mTextureGpuManager->createTexture( "VctVoxelizer" +
                                                              StringConverter::toString( getId() ) +
                                                              "/AccumVal",
                                                              GpuPageOutStrategy::Discard,
                                                              TextureFlags::NotTexture|TextureFlags::Uav,
                                                              TextureTypes::Type3D );
        }

        TextureGpu *textures[4] = { mAlbedoVox, mEmissiveVox, mNormalVox, mAccumValVox };
        for( size_t i=0; i<sizeof(textures) / sizeof(textures[0]); ++i )
            textures[i]->scheduleTransitionTo( GpuResidency::OnStorage );

        mAlbedoVox->setPixelFormat( PFG_RGBA8_UNORM );
        mEmissiveVox->setPixelFormat( PFG_RGBA8_UNORM );
        mNormalVox->setPixelFormat( PFG_R10G10B10A2_UNORM );
        mAccumValVox->setPixelFormat( PFG_R16_UINT );

        for( size_t i=0; i<sizeof(textures) / sizeof(textures[0]); ++i )
        {
            textures[i]->setResolution( mWidth, mHeight, mDepth );
            textures[i]->setNumMipmaps( 1u );
            textures[i]->scheduleTransitionTo( GpuResidency::Resident );
        }
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::destroyVoxelTextures(void)
    {
        if( mAlbedoVox )
        {
            mTextureGpuManager->destroyTexture( mAlbedoVox );
            mTextureGpuManager->destroyTexture( mEmissiveVox );
            mTextureGpuManager->destroyTexture( mNormalVox );
            mTextureGpuManager->destroyTexture( mAccumValVox );

            mAlbedoVox = 0;
            mEmissiveVox = 0;
            mNormalVox = 0;
            mAccumValVox = 0;
        }
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::autoCalculateRegion()
    {
        if( !mAutoRegion )
            return;

        mRegionToVoxelize = Aabb::BOX_NULL;

        ItemArray::const_iterator itor = mItems.begin();
        ItemArray::const_iterator end  = mItems.end();

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

        mRegionToVoxelize.setExtents( minAabb, maxAabb );
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::placeItemsInBuckets()
    {
        ItemArray::const_iterator itor = mItems.begin();
        ItemArray::const_iterator end  = mItems.end();

        while( itor != end )
        {
            Item *item = *itor;
            MeshPtrMap::const_iterator itMesh = mMeshesV2.find( item->getMesh() );

            const size_t numSubItems = item->getNumSubItems();
            for( size_t i=0; i<numSubItems; ++i )
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

                if( convResult.diffuseTex )
                    variant |= VoxelizerJobSetting::HasDiffuseTex;
                if( convResult.emissiveTex )
                    variant |= VoxelizerJobSetting::HasEmissiveTex;
                if( convResult.diffuseTex && convResult.diffuseTex == convResult.emissiveTex )
                    variant |= VoxelizerJobSetting::EmissiveIsDiffuseTex;

                bucket.job              = mComputeJobs[variant];
                bucket.materialBuffer   = convResult.constBuffer;
                bucket.diffuseTex       = convResult.diffuseTex;
                bucket.emissiveTex      = convResult.emissiveTex;
                bucket.vertexBuffer     = itMesh->second.bCompressed ? mVertexBufferCompressed :
                                                                       mVertexBufferUncompressed;
                bucket.indexBuffer      = (variant & VoxelizerJobSetting::Index32bit) ? mIndexBuffer32 :
                                                                                        mIndexBuffer16;

                QueuedInstance queuedInstance;
                queuedInstance.movableObject = item;
                queuedInstance.vertexBufferStart    = itMesh->second.submeshes[i].vbOffset;
                queuedInstance.indexBufferStart     = itMesh->second.submeshes[i].ibOffset;
                queuedInstance.numIndices           = itMesh->second.submeshes[i].numIndices;
                queuedInstance.materialIdx          = convResult.slotIdx;
                mBuckets[bucket].push_back( queuedInstance );
            }

            ++itor;
        }
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::createInstanceBuffers(void)
    {
        const size_t instanceCount = mItems.size();
        const size_t bytesNeeded = instanceCount * mOctants.size() * sizeof(float) * 4u * 6u;

        if( !mInstanceBuffer || bytesNeeded > mInstanceBuffer->getTotalSizeBytes() )
        {
            destroyInstanceBuffers();
            mInstanceBuffer = mVaoManager->createTexBuffer( PF_FLOAT32_RGBA, bytesNeeded,
                                                            BT_DEFAULT
                                                            /*BT_DYNAMIC_DEFAULT*/, 0, false );
            mCpuInstanceBuffer = reinterpret_cast<float*>( OGRE_MALLOC_SIMD( bytesNeeded,
                                                                             MEMCATEGORY_GENERAL ) );
        }
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::destroyInstanceBuffers(void)
    {
        if( mInstanceBuffer )
        {
            mVaoManager->destroyTexBuffer( mInstanceBuffer );
            mInstanceBuffer = 0;

            OGRE_FREE_SIMD( mCpuInstanceBuffer, MEMCATEGORY_GENERAL );
            mCpuInstanceBuffer = 0;
        }
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::fillInstanceBuffers(void)
    {
        createInstanceBuffers();

//        float * RESTRICT_ALIAS instanceBuffer =
//                reinterpret_cast<float*>( mInstanceBuffer->map( 0, mInstanceBuffer->getNumElements() ) );
        float * RESTRICT_ALIAS instanceBuffer = reinterpret_cast<float*>( mCpuInstanceBuffer );
        const float *instanceBufferStart = instanceBuffer;
        FastArray<Octant>::const_iterator itor = mOctants.begin();
        FastArray<Octant>::const_iterator end  = mOctants.end();

        while( itor != end )
        {
            const Aabb octantAabb = itor->region;
            VoxelizerBucketMap::const_iterator itBucket = mBuckets.begin();
            VoxelizerBucketMap::const_iterator enBucket = mBuckets.end();

            while( itBucket != enBucket )
            {
                FastArray<QueuedInstance>::const_iterator itQueuedInst = itBucket->second.begin();
                FastArray<QueuedInstance>::const_iterator enQueuedInst = itBucket->second.end();

                while( itQueuedInst != enQueuedInst )
                {
                    const QueuedInstance &instance = *itQueuedInst;
                    Aabb worldAabb = instance.movableObject->getWorldAabb();

                    //Perform culling against this octant.
                    if( octantAabb.intersects( worldAabb ) )
                    {
                        const Matrix4 &fullTransform =
                                instance.movableObject->_getParentNodeFullTransform();
                        for( size_t i=0; i<12u; ++i )
                            *instanceBuffer++ = static_cast<float>( fullTransform[0][i] );

                        *instanceBuffer++ = worldAabb.mCenter.x;
                        *instanceBuffer++ = worldAabb.mCenter.y;
                        *instanceBuffer++ = worldAabb.mCenter.z;
                        *instanceBuffer++ = 0.0f;

                        #define AS_U32PTR( x ) reinterpret_cast<uint32*RESTRICT_ALIAS>(x)

                        *instanceBuffer++ = worldAabb.mHalfSize.x;
                        *instanceBuffer++ = worldAabb.mHalfSize.y;
                        *instanceBuffer++ = worldAabb.mHalfSize.z;
                        *AS_U32PTR( instanceBuffer ) = instance.materialIdx;        ++instanceBuffer;

                        *AS_U32PTR( instanceBuffer ) = instance.vertexBufferStart;  ++instanceBuffer;
                        *AS_U32PTR( instanceBuffer ) = instance.indexBufferStart;   ++instanceBuffer;
                        *AS_U32PTR( instanceBuffer ) = instance.numIndices;         ++instanceBuffer;
                        *instanceBuffer++ = 0.0f;

                        #undef AS_U32PTR
                    }

                    ++itQueuedInst;
                }

                ++itBucket;
            }

            ++itor;
        }

        OGRE_ASSERT_LOW( (size_t)(instanceBuffer - instanceBufferStart) * sizeof(float) <=
                         mInstanceBuffer->getTotalSizeBytes() );
//        mInstanceBuffer->unmap( UO_UNMAP_ALL );
        mInstanceBuffer->upload( mCpuInstanceBuffer, 0u, mInstanceBuffer->getNumElements() );
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
        octant.width    = mWidth / numOctantsX;
        octant.height   = mHeight / numOctantsY;
        octant.depth    = mDepth / numOctantsZ;

        const Vector3 voxelOrigin = mRegionToVoxelize.getMinimum();
        const Vector3 voxelCellSize = mRegionToVoxelize.getSize() /
                                      Vector3( numOctantsX, numOctantsY, numOctantsZ );

        for( uint32 x=0u; x<numOctantsX; ++x )
        {
            octant.x = x * octant.width;
            for( uint32 y=0u; y<numOctantsY; ++y )
            {
                octant.y = y * octant.height;
                for( uint32 z=0u; z<numOctantsZ; ++z )
                {
                    octant.z = z * octant.depth;

                    Vector3 octantOrigin = Vector3( octant.x, octant.y, octant.z ) * voxelCellSize;
                    octantOrigin += voxelOrigin;
                    octant.region.setExtents( octantOrigin, octantOrigin + voxelCellSize );
                    mOctants.push_back( octant );
                }
            }
        }
    }
    //-------------------------------------------------------------------------
    void VctVoxelizer::build(void)
    {
        OGRE_ASSERT_LOW( !mOctants.empty() );

        if( mItems.empty() )
        {
            TODO_clear_voxels;
        }

        buildMeshBuffers();

        createVoxelTextures();

        placeItemsInBuckets();

        fillInstanceBuffers();

        for( size_t i=0; i<sizeof(mComputeJobs) / sizeof(mComputeJobs[0]); ++i )
        {
            const bool compressedVf = (i & VoxelizerJobSetting::CompressedVertexFormat) != 0;
            const bool hasIndices32 = (i & VoxelizerJobSetting::Index32bit) != 0;

            DescriptorSetUav::BufferSlot bufferSlot( DescriptorSetUav::BufferSlot::makeEmpty() );
            bufferSlot.buffer = compressedVf ? mVertexBufferCompressed : mVertexBufferUncompressed;
            mComputeJobs[i]->_setUavBuffer( 0, bufferSlot );
            bufferSlot.buffer = hasIndices32 ? mIndexBuffer32 : mIndexBuffer16;
            mComputeJobs[i]->_setUavBuffer( 1, bufferSlot );

            DescriptorSetUav::TextureSlot uavSlot( DescriptorSetUav::TextureSlot::makeEmpty() );
            uavSlot.access = ResourceAccess::ReadWrite;

            uavSlot.texture     = mAlbedoVox;
            uavSlot.pixelFormat = mAlbedoVox->getPixelFormat();
            mComputeJobs[i]->_setUavTexture( 2, uavSlot );

            uavSlot.texture     = mNormalVox;
            uavSlot.pixelFormat = mNormalVox->getPixelFormat();
            mComputeJobs[i]->_setUavTexture( 3, uavSlot );

            uavSlot.texture     = mEmissiveVox;
            uavSlot.pixelFormat = mEmissiveVox->getPixelFormat();
            mComputeJobs[i]->_setUavTexture( 4, uavSlot );

            uavSlot.texture     = mAccumValVox;
            uavSlot.pixelFormat = mAccumValVox->getPixelFormat();
            mComputeJobs[i]->_setUavTexture( 5, uavSlot );

            DescriptorSetTexture2::BufferSlot texBufSlot(DescriptorSetTexture2::BufferSlot::makeEmpty());
            texBufSlot.buffer = mInstanceBuffer;
            mComputeJobs[i]->setTexBuffer( 0, texBufSlot );
        }

        mRenderSystem->endRenderPassDescriptor();

        HlmsCompute *hlmsCompute = mHlmsManager->getComputeHlms();

        float fClearValue[4];
        uint32 uClearValue[4];
        memset( fClearValue, 0, sizeof(fClearValue) );
        memset( uClearValue, 0, sizeof(uClearValue) );
        mComputeTools->clearUavFloat( mAlbedoVox, fClearValue );
        mComputeTools->clearUavFloat( mEmissiveVox, fClearValue );
        mComputeTools->clearUavFloat( mNormalVox, fClearValue );
        mComputeTools->clearUavUint( mAccumValVox, uClearValue );

        TODO_add_barrier;

        const uint32 *threadsPerGroup = mComputeJobs[0]->getThreadsPerGroup();

        const Vector3 voxelOrigin = mRegionToVoxelize.getMinimum();

        ShaderParams::Param paramInstanceRange;
        ShaderParams::Param paramVoxelOrigin;
        ShaderParams::Param paramVoxelCellSize;

        paramInstanceRange.name	= "instanceStart_instanceEnd";
        paramVoxelOrigin.name	= "voxelOrigin";
        paramVoxelCellSize.name	= "voxelCellSize";

        paramVoxelCellSize.setManualValue( mRegionToVoxelize.getSize() /
                                           Vector3( mWidth, mHeight, mDepth ) );

        uint32 instanceStart = 0;

        FastArray<Octant>::const_iterator itor = mOctants.begin();
        FastArray<Octant>::const_iterator end  = mOctants.end();

        while( itor != end )
        {
            const Octant &octant = *itor;
            VoxelizerBucketMap::const_iterator itBucket = mBuckets.begin();
            VoxelizerBucketMap::const_iterator enBucket = mBuckets.end();

            while( itBucket != enBucket )
            {
                const VoxelizerBucket &bucket = itBucket->first;
                bucket.job->setNumThreadGroups( octant.width / threadsPerGroup[0],
                                                octant.height / threadsPerGroup[1],
                                                octant.depth / threadsPerGroup[2] );

                uint32 texUnit = 1u;

                DescriptorSetTexture2::TextureSlot
                        texSlot( DescriptorSetTexture2::TextureSlot::makeEmpty() );
                if( bucket.diffuseTex )
                {
                    texSlot.texture = bucket.diffuseTex;
                    bucket.job->setTexture( texUnit, texSlot );
                    ++texUnit;
                }
                if( bucket.emissiveTex )
                {
                    texSlot.texture = bucket.emissiveTex;
                    bucket.job->setTexture( texUnit, texSlot );
                    ++texUnit;
                }

                uint32 numInstancesInBucket = static_cast<uint32 >( itBucket->second.size() );
                uint32 instanceRange[2] = { instanceStart, instanceStart + numInstancesInBucket };

                paramInstanceRange.setManualValue( instanceRange, 2u );
                paramVoxelOrigin.setManualValue( voxelOrigin );

                ShaderParams &shaderParams = bucket.job->getShaderParams( "default" );
                shaderParams.mParams.clear();
                shaderParams.mParams.push_back( paramInstanceRange );
                shaderParams.mParams.push_back( paramVoxelOrigin );
                shaderParams.mParams.push_back( paramVoxelCellSize );
                shaderParams.setDirty();

                hlmsCompute->dispatch( bucket.job, 0, 0 );

                instanceStart += numInstancesInBucket;

                ++itBucket;
            }

            ++itor;
        }

        //This texture is no longer needed, it's not used for the injection phase. Save memory.
        mAccumValVox->scheduleTransitionTo( GpuResidency::OnStorage );
    }
}
