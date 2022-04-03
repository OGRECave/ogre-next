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

#include "Vct/OgreVctImageVoxelizer.h"

#include "Vct/OgreVctVoxelizer.h"
#include "Vct/OgreVoxelVisualizer.h"
#include "Vct/OgreVoxelizedMeshCache.h"

#include "Compute/OgreComputeTools.h"
#include "OgreHlmsCompute.h"
#include "OgreHlmsComputeJob.h"
#include "OgreItem.h"
#include "OgreLogManager.h"
#include "OgreMesh2.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreRenderSystem.h"
#include "OgreSceneManager.h"
#include "OgreStringConverter.h"
#include "OgreSubMesh2.h"
#include "OgreTextureBox.h"
#include "OgreTextureGpuManager.h"
#include "Vao/OgreReadOnlyBufferPacked.h"
#include "Vao/OgreUavBufferPacked.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include "OgreProfiler.h"

namespace Ogre
{
    static const uint8 c_reservedTexSlots = 1u;

    VctImageVoxelizer::VctImageVoxelizer( IdType id, RenderSystem *renderSystem,
                                          HlmsManager *hlmsManager, VoxelizedMeshCache *meshCache,
                                          bool correctAreaLightShadows ) :
        VctVoxelizerSourceBase( id, renderSystem, hlmsManager ),
        mMeshCache( meshCache ),
        mItemOrderDirty( false ),
        mTexMeshesPerBatch( 0u ),
        mImageVoxelizerJob( 0 ),
        mPartialClearJob( 0 ),
        mComputeTools( new ComputeTools( hlmsManager->getComputeHlms() ) ),
        mFullBuildDone( false ),
        mNeedsAlbedoMipmaps( correctAreaLightShadows ),
        mAutoRegion( true ),
        mMaxRegion( Aabb::BOX_INFINITE ),
        mCpuInstanceBuffer( 0 ),
        mInstanceBuffer( 0 ),
        mAlbedoVoxAlt( 0 ),
        mEmissiveVoxAlt( 0 ),
        mNormalVoxAlt( 0 )
    {
        createComputeJobs();
    }
    //-------------------------------------------------------------------------
    VctImageVoxelizer::~VctImageVoxelizer()
    {
        setDebugVisualization( DebugVisualizationNone, 0 );
        destroyInstanceBuffers();
        destroyVoxelTextures();
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::createComputeJobs()
    {
        HlmsCompute *hlmsCompute = mHlmsManager->getComputeHlms();
#if OGRE_NO_JSON
        OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                     "To use VctImageVoxelizer, Ogre must be build with JSON support "
                     "and you must include the resources bundled at "
                     "Samples/Media/VCT",
                     "VctImageVoxelizer::createComputeJobs" );
#endif
        mImageVoxelizerJob = hlmsCompute->findComputeJobNoThrow( "VCT/ImageVoxelizer" );

        if( !mImageVoxelizerJob )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "To use VctImageVoxelizer, you must include the resources bundled at "
                         "Samples/Media/VCT\n"
                         "Could not find VCT/ImageVoxelizer",
                         "VctImageVoxelizer::createComputeJobs" );
        }

        if( mVaoManager->readOnlyIsTexBuffer() )
            mImageVoxelizerJob->setGlTexSlotStart( 4u );

        mPartialClearJob = hlmsCompute->findComputeJobNoThrow( "VCT/VoxelPartialClear" );

        if( !mPartialClearJob )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "To use VctImageVoxelizer, you must include the resources bundled at "
                         "Samples/Media/VCT\n"
                         "Could not find VCT/VoxelPartialClear",
                         "VctImageVoxelizer::createComputeJobs" );
        }

        const RenderSystemCapabilities *caps = mHlmsManager->getRenderSystem()->getCapabilities();

        uint32 maxTexturesInCompute = caps->getNumTexturesInTextureDescriptor( NumShaderTypes ) -
                                      ( c_reservedTexSlots + mImageVoxelizerJob->getGlTexSlotStart() );

        const bool bIsD3D11 = hlmsCompute->getShaderSyntax() == HlmsBaseProp::Hlsl;

        // Discretize to avoid shader variant explosion and keep number of empty slots under control(*)
        // Given API + GPU support we should either land in 15 (GL) or 30 (GL/Metal/Vk/D3D11)
        // If maxTexturesInCompute < 9; that should not happen or it's a broken GPU
        //
        // Also our compute shaders assume units slots are uint8, so we have to clamp <= 255
        //
        // (*) This keeps CPU overhead in check. Also D3D11 can't do dynamic texture selection
        // thus the resulting shader ends up being an if-ladder. Therefore too many textures
        // can hurt GPU performance a lot in that API (also compiler will fail to unroll).
        if( maxTexturesInCompute >= 30u && !bIsD3D11 )
            maxTexturesInCompute = 30u;
        else if( maxTexturesInCompute >= 15u && !bIsD3D11 )
            maxTexturesInCompute = 15u;
        else if( maxTexturesInCompute >= 12u )
            maxTexturesInCompute = 12u;
        else if( maxTexturesInCompute >= 9u )
            maxTexturesInCompute = 9u;

        // Force it to be multiple of 3 (otherwise we get crashes)
        maxTexturesInCompute = ( maxTexturesInCompute / 3u ) * 3u;

        mImageVoxelizerJob->setProperty( "tex_meshes_per_batch",
                                         static_cast<int32>( maxTexturesInCompute ) );
        mTexMeshesPerBatch = maxTexturesInCompute;

        mImageVoxelizerJob->setNumTexUnits(
            static_cast<uint8>( mTexMeshesPerBatch + c_reservedTexSlots ) );

        // Samplers need to be set only once for all texture slots that need it
        HlmsSamplerblock const *trilinearSampler = 0;
        {
            HlmsSamplerblock refSampler;
            refSampler.setAddressingMode( TAM_CLAMP );
            refSampler.setFiltering( TFO_TRILINEAR );
            trilinearSampler = mHlmsManager->getSamplerblock( refSampler );
        }

        const bool bSetSampler = !caps->hasCapability( RSC_SEPARATE_SAMPLERS_FROM_TEXTURES );

        if( !bSetSampler )
        {
            mImageVoxelizerJob->setNumSamplerUnits( c_reservedTexSlots + 1u );
        }

        const uint32 texMeshesPerBatch = mTexMeshesPerBatch;

        const uint32 samplersToSet = bSetSampler ? texMeshesPerBatch : 1u;
        for( size_t i = 0u; i < samplersToSet; ++i )
        {
            mHlmsManager->addReference( trilinearSampler );
            mImageVoxelizerJob->_setSamplerblock( static_cast<uint8>( i + c_reservedTexSlots ),
                                                  trilinearSampler );
        }

        mHlmsManager->destroySamplerblock( trilinearSampler );

        if( mMeshCache->mGlslTexUnits.size() != texMeshesPerBatch )
        {
            ShaderParams &glslShaderParams = mImageVoxelizerJob->getShaderParams( "glsl" );
            ShaderParams::Param *meshTexturesParam = glslShaderParams.findParameter( "meshTextures" );
            mMeshCache->mGlslTexUnits.resizePOD( texMeshesPerBatch );
            for( size_t i = 0u; i < texMeshesPerBatch; ++i )
                mMeshCache->mGlslTexUnits[i] = static_cast<int32>( i + c_reservedTexSlots );
            meshTexturesParam->setManualValueEx(
                mMeshCache->mGlslTexUnits.begin(),
                static_cast<uint32>( mMeshCache->mGlslTexUnits.size() ) );
        }
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::clearComputeJobResources()
    {
        // Do not leave dangling pointers when destroying buffers, even if we later set them
        // with a new pointer (if malloc reuses an address and the jobs weren't cleared, we're screwed)
        mImageVoxelizerJob->clearUavBuffers();
        mImageVoxelizerJob->clearTexBuffers();
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::addItem( Item *item )
    {
        if( !mItems.empty() && mItems.back() != item )
            mItemOrderDirty = true;
        mItems.push_back( item );

        mFullBuildDone = false;
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::removeItem( Item *item )
    {
        ItemArray::iterator itor = std::find( mItems.begin(), mItems.end(), item );
        if( itor == mItems.end() )
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "", "VctImageVoxelizer::removeItem" );

        if( mItemOrderDirty )
            efficientVectorRemove( mItems, itor );
        else
            mItems.erase( itor );

        mFullBuildDone = false;
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::removeAllItems()
    {
        mItems.clear();
        mItemOrderDirty = false;
        mFullBuildDone = false;
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::restoreSwappedVoxelTextures()
    {
        if( mAlbedoVox && mAlbedoVoxAlt )
        {
            const IdString mainTexName =
                "VctImageVoxelizer" + StringConverter::toString( getId() ) + "/Albedo";
            if( mAlbedoVox->getName() != mainTexName )
            {
                std::swap( mAlbedoVox, mAlbedoVoxAlt );
                std::swap( mEmissiveVox, mEmissiveVoxAlt );
                std::swap( mNormalVox, mNormalVoxAlt );
            }
        }
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::createVoxelTextures()
    {
        if( mAlbedoVox )
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

            if( mNeedsAlbedoMipmaps )
                texFlags |= TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps;

            mAlbedoVox = mTextureGpuManager->createTexture(
                "VctImageVoxelizer" + StringConverter::toString( getId() ) + "/Albedo",
                GpuPageOutStrategy::Discard, texFlags, TextureTypes::Type3D );

            texFlags &= ~(uint32)( TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps );

            mEmissiveVox = mTextureGpuManager->createTexture(
                "VctImageVoxelizer" + StringConverter::toString( getId() ) + "/Emissive",
                GpuPageOutStrategy::Discard, texFlags, TextureTypes::Type3D );
            mNormalVox = mTextureGpuManager->createTexture(
                "VctImageVoxelizer" + StringConverter::toString( getId() ) + "/Normal",
                GpuPageOutStrategy::Discard, texFlags, TextureTypes::Type3D );
            mAccumValVox = mTextureGpuManager->createTexture(
                "VctImageVoxelizer" + StringConverter::toString( getId() ) + "/AccumVal",
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
            textures[i]->setNumMipmaps( mNeedsAlbedoMipmaps && i == 0u ? numMipmaps : 1u );
            textures[i]->scheduleTransitionTo( GpuResidency::Resident );
        }
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::createAltVoxelTextures()
    {
        if( mAlbedoVoxAlt )
            return;

        const bool hasTypedUavs = mRenderSystem->getCapabilities()->hasCapability( RSC_TYPED_UAV_LOADS );

        uint32 texFlags = TextureFlags::Uav;
        if( !hasTypedUavs )
            texFlags |= TextureFlags::Reinterpretable;

        if( mNeedsAlbedoMipmaps )
            texFlags |= TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps;

        mAlbedoVoxAlt = mTextureGpuManager->createTexture(
            "VctImageVoxelizer" + StringConverter::toString( getId() ) + "/AlbedoALT",
            GpuPageOutStrategy::Discard, texFlags, TextureTypes::Type3D );

        texFlags &= ~(uint32)( TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps );

        mEmissiveVoxAlt = mTextureGpuManager->createTexture(
            "VctImageVoxelizer" + StringConverter::toString( getId() ) + "/EmissiveALT",
            GpuPageOutStrategy::Discard, texFlags, TextureTypes::Type3D );
        mNormalVoxAlt = mTextureGpuManager->createTexture(
            "VctImageVoxelizer" + StringConverter::toString( getId() ) + "/NormalALT",
            GpuPageOutStrategy::Discard, texFlags, TextureTypes::Type3D );

        mAlbedoVoxAlt->copyParametersFrom( mAlbedoVox );
        mEmissiveVoxAlt->copyParametersFrom( mEmissiveVox );
        mNormalVoxAlt->copyParametersFrom( mNormalVox );

        mAlbedoVoxAlt->scheduleTransitionTo( GpuResidency::Resident );
        mEmissiveVoxAlt->scheduleTransitionTo( GpuResidency::Resident );
        mNormalVoxAlt->scheduleTransitionTo( GpuResidency::Resident );
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::setVoxelTexturesToJobs()
    {
        if( mDebugVoxelVisualizer )
        {
            setTextureToDebugVisualizer();
            mDebugVoxelVisualizer->setVisible( true );
        }

        const bool hasTypedUavs = mRenderSystem->getCapabilities()->hasCapability( RSC_TYPED_UAV_LOADS );

        DescriptorSetUav::TextureSlot uavSlot( DescriptorSetUav::TextureSlot::makeEmpty() );
        uavSlot.access = ResourceAccess::ReadWrite;

        uavSlot.texture = mAlbedoVox;
        if( hasTypedUavs )
            uavSlot.pixelFormat = mAlbedoVox->getPixelFormat();
        else
            uavSlot.pixelFormat = PFG_R32_UINT;
        uavSlot.access = ResourceAccess::ReadWrite;
        mImageVoxelizerJob->_setUavTexture( 0u, uavSlot );

        uavSlot.texture = mNormalVox;
        if( hasTypedUavs )
            uavSlot.pixelFormat = mNormalVox->getPixelFormat();
        else
            uavSlot.pixelFormat = PFG_R32_UINT;
        uavSlot.access = ResourceAccess::ReadWrite;
        mImageVoxelizerJob->_setUavTexture( 1u, uavSlot );

        uavSlot.texture = mEmissiveVox;
        if( hasTypedUavs )
            uavSlot.pixelFormat = mEmissiveVox->getPixelFormat();
        else
            uavSlot.pixelFormat = PFG_R32_UINT;
        uavSlot.access = ResourceAccess::ReadWrite;
        mImageVoxelizerJob->_setUavTexture( 2u, uavSlot );

        uavSlot.texture = mAccumValVox;
        uavSlot.pixelFormat = mAccumValVox->getPixelFormat();
        uavSlot.access = ResourceAccess::ReadWrite;
        mImageVoxelizerJob->_setUavTexture( 3u, uavSlot );
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::destroyVoxelTextures()
    {
        if( mAlbedoVoxAlt )
        {
            mTextureGpuManager->destroyTexture( mAlbedoVoxAlt );
            mTextureGpuManager->destroyTexture( mEmissiveVoxAlt );
            mTextureGpuManager->destroyTexture( mNormalVoxAlt );

            mAlbedoVoxAlt = 0;
            mEmissiveVoxAlt = 0;
            mNormalVoxAlt = 0;
        }

        VctVoxelizerSourceBase::destroyVoxelTextures();
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::createInstanceBuffers()
    {
        const size_t structStride = sizeof( float ) * 4u * 5u;
        const size_t elementCount = mItems.size() * mOctants.size();

        if( !mInstanceBuffer || ( elementCount * structStride ) > mInstanceBuffer->getTotalSizeBytes() )
        {
            destroyInstanceBuffers();
            mInstanceBuffer = mVaoManager->createUavBuffer( elementCount, structStride,
                                                            BB_FLAG_UAV | BB_FLAG_READONLY, 0, false );
            mCpuInstanceBuffer = reinterpret_cast<float *>(
                OGRE_MALLOC_SIMD( elementCount * structStride, MEMCATEGORY_GENERAL ) );
        }

        // Always set because we may not be the only VctImageVoxelizer using the same job
        DescriptorSetTexture2::BufferSlot bufferSlot( DescriptorSetTexture2::BufferSlot::makeEmpty() );
        bufferSlot.buffer = mInstanceBuffer->getAsReadOnlyBufferView();
        mImageVoxelizerJob->setTexBuffer( 0u, bufferSlot );
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::destroyInstanceBuffers()
    {
        if( mInstanceBuffer )
        {
            mVaoManager->destroyUavBuffer( mInstanceBuffer );
            mInstanceBuffer = 0;

            OGRE_FREE_SIMD( mCpuInstanceBuffer, MEMCATEGORY_GENERAL );
            mCpuInstanceBuffer = 0;
        }

        clearComputeJobResources();
    }
    //-------------------------------------------------------------------------
    inline bool OrderItemsByMesh( const Item *_left, const Item *_right )
    {
        // Handles are deterministic unlike Mesh ptrs (unless ResourceGroupManager is threaded)
        return _left->getMesh()->getHandle() < _right->getMesh()->getHandle();
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::fillInstanceBuffers( SceneManager *sceneManager )
    {
        OgreProfile( "VctImageVoxelizer::fillInstanceBuffers" );

        createInstanceBuffers();

        {
            float *RESTRICT_ALIAS instanceBuffer = mCpuInstanceBuffer;

            const size_t numItems = mItems.size();
            const size_t numOctants = mOctants.size();

            for( size_t i = 0u; i < numOctants; ++i )
                mOctants[i].instanceBuffer = instanceBuffer + i * numItems * 4u * 5u;
        }

        if( mItemOrderDirty )
        {
            std::sort( mItems.begin(), mItems.end(), OrderItemsByMesh );
            mItemOrderDirty = false;
        }

        mBatches.clear();

        Mesh *lastMesh = 0;
        uint32 numSeenMeshes = 0u;
        uint32 textureIdx = uint32_t( -3 );
        const uint32 texMeshesPerBatch = mTexMeshesPerBatch;
        BatchInstances *batchInstances = 0;
        const Vector3 voxelCellSize = getVoxelCellSize();
        Vector3 meshResolution( Vector3::UNIT_SCALE );

        const size_t numOctants = mOctants.size();
        Octant *octants = mOctants.begin();

        ItemArray::const_iterator itItem = mItems.begin();
        ItemArray::const_iterator enItem = mItems.end();
        while( itItem != enItem )
        {
            const Item *item = *itItem;

            const Aabb worldAabb = item->getWorldAabb();

            const Node *parentNode = item->getParentNode();

            const Vector3 itemPos = parentNode->_getDerivedPosition();
            const Quaternion itemRot = parentNode->_getDerivedOrientation();
            const Vector3 itemScale = parentNode->_getDerivedScale();
            const Aabb localAabb = item->getLocalAabb();  // Must be local, not world

            Matrix4 worldToUvw;
            worldToUvw.makeInverseTransform( itemPos + itemRot * ( localAabb.getMinimum() * itemScale ),
                                             itemScale * localAabb.getSize(), itemRot );

            if( lastMesh != item->getMesh().get() )
            {
                const VoxelizedMeshCache::VoxelizedMesh &cacheEntry = mMeshCache->addMeshToCache(
                    item->getMesh(), sceneManager, mRenderSystem, mHlmsManager, item );

                // New texture idx!
                lastMesh = item->getMesh().get();
                ++numSeenMeshes;

                textureIdx += 3u;
                if( textureIdx + 3u > texMeshesPerBatch || textureIdx == 0u )
                {
                    // We've run out of texture units! This is a batch breaker!
                    // (we may also be here if this is the first batch)
                    textureIdx = 0u;

                    mBatches.push_back( Batch() );
                    const uint32 offset = static_cast<uint32>( itItem - mItems.begin() );
                    mBatches.back().instances.resize( numOctants, BatchInstances( offset ) );
                    batchInstances = mBatches.back().instances.begin();
                }

                mBatches.back().textures.push_back( cacheEntry.albedoVox );
                mBatches.back().textures.push_back( cacheEntry.normalVox );
                mBatches.back().textures.push_back( cacheEntry.emissiveVox );

                meshResolution = Vector3( (Real)cacheEntry.albedoVox->getWidth(),
                                          (Real)cacheEntry.albedoVox->getHeight(),
                                          (Real)cacheEntry.albedoVox->getDepthOrSlices() );
            }

            const Vector3 invMeshCellSize = meshResolution / worldAabb.getSize();
            const Vector3 ratio3 = voxelCellSize * invMeshCellSize;
            const float ratio = std::min( ratio3.x, std::min( ratio3.y, ratio3.z ) );

            const float rawLodLevel = Math::Log2( ratio );

            // Bias by -0.5f because it produces far better results (it's too blocky otherwise)
            // We can't bias more because otherwise holes appear as we skip entire pixels
            const float lodLevel = std::max( rawLodLevel - 0.5f, 0.0f );

            // The values in alphaExponent are arbitrary (i.e. empirically obtained).
            //
            // The problem is when e.g. interpolating a 64x64x64 LOD voxelized mesh
            // into a 128x128x128 scene; the result tends to become very "fat" (i.e.
            // 1 block of the voxelized mesh is worth 2x2 blocks of scene mesh)
            //
            // To thin out the result we perform pow( instanceAlbedo.w, alphaExponent )
            //
            // There is a strong correlation between lod level and wall (wrong) thickness
            // thus we use LOD to derive alphaExponent.
            //
            // rawLodLevel contains the LOD level we would have to sample if we had
            // more resolution available in the mesh, i.e. if LOD 0 of a mesh is 64x64x64
            // then LOD -1 would be 128x128x128
            //
            // We clamp at -5 to prevent this value from exploding (it's an exponent!)
            // x3.0f is arbitrary; and we clamp at 1 to avoid thickening instanceAlbedo.w
            // instead of thinning it
            //
            // We then multiply it by 0.5 to produce some thinning, since it looks better
            // (maintains overall brightness more even)
            const float alphaExponent =
                std::max( lodLevel - std::max( rawLodLevel, -5.0f ) * 3.0f, 1.0f ) * 0.5f;

            for( size_t i = 0u; i < numOctants; ++i )
            {
                const Aabb octantAabb = octants[i].region;

                if( octantAabb.intersects( worldAabb ) )
                {
                    float *RESTRICT_ALIAS instanceBuffer = octants[i].instanceBuffer;

                    for( size_t j = 0; j < 12u; ++j )
                        *instanceBuffer++ = static_cast<float>( worldToUvw[0][j] );

#define AS_U32PTR( x ) reinterpret_cast<uint32 * RESTRICT_ALIAS>( x )
                    *instanceBuffer++ = worldAabb.mCenter.x;
                    *instanceBuffer++ = worldAabb.mCenter.y;
                    *instanceBuffer++ = worldAabb.mCenter.z;
                    *AS_U32PTR( instanceBuffer ) = ( textureIdx % texMeshesPerBatch ) |
                                                   ( uint32( alphaExponent * 10000.0f ) << 8u );
                    ++instanceBuffer;

                    *instanceBuffer++ = worldAabb.mHalfSize.x;
                    *instanceBuffer++ = worldAabb.mHalfSize.y;
                    *instanceBuffer++ = worldAabb.mHalfSize.z;
                    *instanceBuffer++ = lodLevel;
#undef AS_U32PTR

                    octants[i].instanceBuffer = instanceBuffer;
                    ++batchInstances[i].numInstances;

                    OGRE_ASSERT_MEDIUM( size_t( instanceBuffer - mCpuInstanceBuffer ) *
                                            sizeof( float ) <=
                                        mInstanceBuffer->getTotalSizeBytes() );
                }
            }

            ++itItem;
        }

        mInstanceBuffer->upload( mCpuInstanceBuffer, 0u, mInstanceBuffer->getNumElements() );

        const RenderSystemCapabilities *caps = mHlmsManager->getRenderSystem()->getCapabilities();
        const bool bSetSampler = !caps->hasCapability( RSC_SEPARATE_SAMPLERS_FROM_TEXTURES );

        // If we have too few meshes, put dummies in unused slots
        // Note: numSeenMeshes * 3u >= texMeshesPerBatch is possible
        DescriptorSetTexture2::TextureSlot texSlot( DescriptorSetTexture2::TextureSlot::makeEmpty() );
        texSlot.texture = mMeshCache->getBlankEmissive();
        for( size_t i = numSeenMeshes * 3u; i < texMeshesPerBatch; ++i )
        {
            mImageVoxelizerJob->setTexture( static_cast<uint8>( c_reservedTexSlots + i ), texSlot, 0,
                                            bSetSampler );
        }
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::setRegionToVoxelize( bool autoRegion, const Aabb &regionToVoxelize,
                                                 const Aabb &maxRegion )
    {
        mAutoRegion = autoRegion;
        mRegionToVoxelize = regionToVoxelize;
        mMaxRegion = maxRegion;
        mOctants.clear();
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::autoCalculateRegion()
    {
        if( !mAutoRegion )
            return;

        mRegionToVoxelize = Aabb::BOX_NULL;

        ItemArray::const_iterator itor = mItems.begin();
        ItemArray::const_iterator endt = mItems.end();

        while( itor != endt )
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
    void VctImageVoxelizer::dividideOctants( uint32 numOctantsX, uint32 numOctantsY, uint32 numOctantsZ )
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
        const Vector3 octantCellSize =
            mRegionToVoxelize.getSize() /
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

                    Vector3 octantOrigin = Vector3( (Real)x, (Real)y, (Real)z ) * octantCellSize;
                    octantOrigin += voxelOrigin;
                    octant.region.setExtents( octantOrigin, octantOrigin + octantCellSize );
                    mOctants.push_back( octant );
                }
            }
        }
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::clearVoxels( const bool bPartialClear )
    {
        OgreProfileGpuBegin( "IVCT Voxelization Clear" );
        mRenderSystem->debugAnnotationPush( "IVCT Voxelization Clear" );
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
        if( !bPartialClear )
        {
            mComputeTools->prepareForUavClear( mResourceTransitions, mAlbedoVox );
            mComputeTools->prepareForUavClear( mResourceTransitions, mEmissiveVox );
            mComputeTools->prepareForUavClear( mResourceTransitions, mNormalVox );
        }
        else
        {
            DescriptorSetUav::TextureSlot uavSlot( DescriptorSetUav::TextureSlot::makeEmpty() );
            uavSlot.access = ResourceAccess::Write;

            uavSlot.texture = mAlbedoVox;
            mPartialClearJob->_setUavTexture( 0u, uavSlot );
            uavSlot.texture = mEmissiveVox;
            mPartialClearJob->_setUavTexture( 1u, uavSlot );
            uavSlot.texture = mNormalVox;
            mPartialClearJob->_setUavTexture( 2u, uavSlot );

            mPartialClearJob->analyzeBarriers( mResourceTransitions, false );
        }
        mComputeTools->prepareForUavClear( mResourceTransitions, mAccumValVox );
        mRenderSystem->executeResourceTransition( mResourceTransitions );

        if( !bPartialClear )
        {
            mComputeTools->clearUavFloat( mAlbedoVox, fClearValue );
            mComputeTools->clearUavFloat( mEmissiveVox, fClearValue );
            mComputeTools->clearUavFloat( mNormalVox, fClearNormals );
        }
        else
        {
            ShaderParams &shaderParams = mPartialClearJob->getShaderParams( "default" );

            ShaderParams::Param paramStartOffset;
            ShaderParams::Param paramPixelsToClear;

            paramStartOffset.name = "startOffset";
            paramPixelsToClear.name = "pixelsToClear";

            FastArray<Octant>::const_iterator itor = mOctants.begin();
            FastArray<Octant>::const_iterator endt = mOctants.end();

            while( itor != endt )
            {
                const Octant &octant = *itor;

                if( octant.diffAxis == 0u )
                    mPartialClearJob->setThreadsPerGroup( 8u, 8u, 1u );
                else if( octant.diffAxis == 1u )
                    mPartialClearJob->setThreadsPerGroup( 8u, 1u, 8u );
                else  // if( octant.diffAxis == 2u )
                    mPartialClearJob->setThreadsPerGroup( 1u, 8u, 8u );

                paramStartOffset.setManualValue( &octant.x, 3u );
                paramPixelsToClear.setManualValue( &octant.width, 3u );

                const uint32 *threadsPerGroup = mPartialClearJob->getThreadsPerGroup();

                mPartialClearJob->setNumThreadGroups(
                    static_cast<uint32>( alignToNextMultiple( octant.width, threadsPerGroup[0] ) ) /
                        threadsPerGroup[0],
                    static_cast<uint32>( alignToNextMultiple( octant.height, threadsPerGroup[1] ) ) /
                        threadsPerGroup[1],
                    static_cast<uint32>( alignToNextMultiple( octant.depth, threadsPerGroup[2] ) ) /
                        threadsPerGroup[2] );

                shaderParams.mParams.clear();
                shaderParams.mParams.push_back( paramStartOffset );
                shaderParams.mParams.push_back( paramPixelsToClear );
                shaderParams.setDirty();

                HlmsCompute *hlmsCompute = mHlmsManager->getComputeHlms();
                hlmsCompute->dispatch( mPartialClearJob, 0, 0 );
                ++itor;
            }
        }
        mComputeTools->clearUavUint( mAccumValVox, uClearValue );
        mRenderSystem->debugAnnotationPop();
        OgreProfileGpuEnd( "IVCT Voxelization Clear" );
    }
    //-------------------------------------------------------------------------
    /**
    @brief VctImageVoxelizer::setOffsetOctants
        When displacing a scene VCT, we often only do it by a single row of 1 x height x depth
        Or maybe two rows or more if the camera jumped too much.

        We may generate up to 3 octants into mOctants

        That way we only recalculate the new rows, not the whole thing
    */
    void VctImageVoxelizer::setOffsetOctants( const int32 diffX, const int32 diffY, const int32 diffZ )
    {
        mOctants.clear();
        if( diffZ != 0 )
        {
            Octant octant;

            octant.x = 0u;
            octant.y = 0u;
            octant.width = mWidth;
            octant.height = mHeight;

            octant.diffAxis = 0u;

            if( diffZ > 0 )
            {
                octant.z = mDepth - static_cast<uint32>( diffZ );
                octant.depth = static_cast<uint32>( diffZ );
            }
            else
            {
                octant.z = 0u;
                octant.depth = static_cast<uint32>( -diffZ );
            }

            const Vector3 voxelCellSize = getVoxelCellSize();
            const Vector3 voxelOrigin = getVoxelOrigin();
            octant.region.setExtents(
                voxelOrigin + voxelCellSize * Vector3( (Real)octant.x, (Real)octant.y, (Real)octant.z ),
                voxelOrigin + voxelCellSize * Vector3( Real( octant.x + octant.width ),
                                                       Real( octant.y + octant.height ),
                                                       Real( octant.z + octant.depth ) ) );

            mOctants.push_back( octant );
        }
        if( diffY != 0 )
        {
            Octant octant;

            octant.x = 0u;
            octant.z = 0u;
            octant.width = mWidth;
            octant.depth = mDepth;

            octant.diffAxis = 1u;

            if( diffY > 0 )
            {
                octant.y = mHeight - static_cast<uint32>( diffY );
                octant.height = static_cast<uint32>( diffY );
            }
            else
            {
                octant.y = 0u;
                octant.height = static_cast<uint32>( -diffY );
            }

            FastArray<Octant>::const_iterator itor = mOctants.begin();
            FastArray<Octant>::const_iterator endt = mOctants.end();

            while( itor != endt )
            {
                if( itor->z == 0u )
                    octant.z += itor->depth;
                octant.depth -= itor->depth;
                ++itor;
            }

            const Vector3 voxelCellSize = getVoxelCellSize();
            const Vector3 voxelOrigin = getVoxelOrigin();
            octant.region.setExtents(
                voxelOrigin + voxelCellSize * Vector3( (Real)octant.x, (Real)octant.y, (Real)octant.z ),
                voxelOrigin + voxelCellSize * Vector3( Real( octant.x + octant.width ),
                                                       Real( octant.y + octant.height ),
                                                       Real( octant.z + octant.depth ) ) );

            mOctants.push_back( octant );
        }
        if( diffX != 0 )
        {
            Octant octant;

            octant.y = 0u;
            octant.z = 0u;
            octant.height = mHeight;
            octant.depth = mDepth;

            octant.diffAxis = 2u;

            if( diffX > 0 )
            {
                octant.x = mWidth - static_cast<uint32>( diffX );
                octant.width = static_cast<uint32>( diffX );
            }
            else
            {
                octant.x = 0u;
                octant.width = static_cast<uint32>( -diffX );
            }

            FastArray<Octant>::const_iterator itor = mOctants.begin();
            FastArray<Octant>::const_iterator endt = mOctants.end();

            while( itor != endt )
            {
                if( itor->diffAxis == 1u )
                {
                    if( itor->y == 0u )
                        octant.y += itor->height;
                    octant.height -= itor->height;
                }
                else  // if( octant.diffAxis == 0u )
                {
                    if( itor->z == 0u )
                        octant.z += itor->depth;
                    octant.depth -= itor->depth;
                }
                ++itor;
            }

            const Vector3 voxelCellSize = getVoxelCellSize();
            const Vector3 voxelOrigin = getVoxelOrigin();
            octant.region.setExtents(
                voxelOrigin + voxelCellSize * Vector3( (Real)octant.x, (Real)octant.y, (Real)octant.z ),
                voxelOrigin + voxelCellSize * Vector3( Real( octant.x + octant.width ),
                                                       Real( octant.y + octant.height ),
                                                       Real( octant.z + octant.depth ) ) );

            mOctants.push_back( octant );
        }
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::setSceneResolution( uint32 width, uint32 height, uint32 depth )
    {
        destroyVoxelTextures();
        mWidth = width;
        mHeight = height;
        mDepth = depth;
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::build( SceneManager *sceneManager )
    {
        OgreProfile( "VctImageVoxelizer::build" );
        OgreProfileGpuBegin( "IVCT build" );

        OGRE_ASSERT_LOW( !mOctants.empty() );

        mRenderSystem->endRenderPassDescriptor();

        if( mItems.empty() )
        {
            createVoxelTextures();
            clearVoxels( false );
            return;
        }

        mRenderSystem->debugAnnotationPush( "VctImageVoxelizer::build" );

        createVoxelTextures();
        setVoxelTexturesToJobs();

        fillInstanceBuffers( sceneManager );

        HlmsCompute *hlmsCompute = mHlmsManager->getComputeHlms();
        clearVoxels( false );

        ShaderParams::Param paramInstanceRange;
        ShaderParams::Param paramVoxelOrigin;
        ShaderParams::Param paramVoxelCellSize;
        ShaderParams::Param paramVoxelPixelOrigin;

        paramInstanceRange.name = "instanceStart_instanceEnd";
        paramVoxelOrigin.name = "voxelOrigin";
        paramVoxelCellSize.name = "voxelCellSize";
        paramVoxelPixelOrigin.name = "voxelPixelOrigin";

        paramVoxelCellSize.setManualValue( getVoxelCellSize() );

        OgreProfileGpuBegin( "IVCT Image Voxelization Jobs" );

        const uint32 *threadsPerGroup = mImageVoxelizerJob->getThreadsPerGroup();

        const size_t numOctants = mOctants.size();

        mImageVoxelizerJob->setThreadsPerGroup( 4u, 4u, 4u );

        if( mImageVoxelizerJob->getProperty( "check_out_of_bounds" ) != 0 )
            mImageVoxelizerJob->setProperty( "check_out_of_bounds", 0 );

        ShaderParams &shaderParams = mImageVoxelizerJob->getShaderParams( "default" );

        const RenderSystemCapabilities *caps = mHlmsManager->getRenderSystem()->getCapabilities();
        const bool bSetSampler = !caps->hasCapability( RSC_SEPARATE_SAMPLERS_FROM_TEXTURES );

        const size_t numItems = mItems.size();

        BatchArray::const_iterator itBatch = mBatches.begin();
        BatchArray::const_iterator enBatch = mBatches.end();

        while( itBatch != enBatch )
        {
            {
                // texIdx starts at 1 because mInstanceBuffer goes at 0; and whether
                // that consumes a tex slot or not is a rabbit hole
                // (depends on VaoManager::readOnlyIsTexBuffer AND the API used) so we
                // just assume we always consume 1
                size_t texIdx = 1u;
                DescriptorSetTexture2::TextureSlot texSlot(
                    DescriptorSetTexture2::TextureSlot::makeEmpty() );
                FastArray<TextureGpu *>::const_iterator itTex = itBatch->textures.begin();
                FastArray<TextureGpu *>::const_iterator enTex = itBatch->textures.end();

                while( itTex != enTex )
                {
                    texSlot.texture = *itTex;
                    // Sampler is nullptr because it was already set at init
                    mImageVoxelizerJob->setTexture( static_cast<uint8>( texIdx ), texSlot, 0,
                                                    bSetSampler );
                    ++texIdx;
                    ++itTex;
                }
            }

            // Analyze barriers once per batch; since all octants can be run in parallel
            mResourceTransitions.clear();
            for( size_t i = 0u; i < numOctants && mResourceTransitions.empty(); ++i )
            {
                if( itBatch->instances[i].numInstances > 0u )
                {
                    mImageVoxelizerJob->analyzeBarriers( mResourceTransitions );
                    mRenderSystem->executeResourceTransition( mResourceTransitions );
                }
            }

            for( size_t i = 0u; i < numOctants; ++i )
            {
                if( itBatch->instances[i].numInstances > 0u )
                {
                    const Octant &octant = mOctants[i];

                    mImageVoxelizerJob->setNumThreadGroups( octant.width / threadsPerGroup[0],
                                                            octant.height / threadsPerGroup[1],
                                                            octant.depth / threadsPerGroup[2] );

                    const uint32 instanceRange[2] = {
                        itBatch->instances[i].instanceOffset + static_cast<uint32>( i * numItems ),
                        itBatch->instances[i].instanceOffset + itBatch->instances[i].numInstances +
                            static_cast<uint32>( i * numItems )
                    };
                    const uint32 voxelPixelOrigin[3] = { octant.x, octant.y, octant.z };

                    paramInstanceRange.setManualValue( instanceRange, 2u );
                    paramVoxelOrigin.setManualValue( octant.region.getMinimum() );
                    paramVoxelPixelOrigin.setManualValue( voxelPixelOrigin, 3u );

                    shaderParams.mParams.clear();
                    shaderParams.mParams.push_back( paramInstanceRange );
                    shaderParams.mParams.push_back( paramVoxelOrigin );
                    shaderParams.mParams.push_back( paramVoxelCellSize );
                    shaderParams.mParams.push_back( paramVoxelPixelOrigin );
                    shaderParams.setDirty();

                    hlmsCompute->dispatch( mImageVoxelizerJob, 0, 0 );
                }
            }

            ++itBatch;
        }

        OgreProfileGpuEnd( "IVCT Image Voxelization Jobs" );

        // This texture is no longer needed, it's not used for the injection phase. Save memory.
        mAccumValVox->scheduleTransitionTo( GpuResidency::OnStorage );

        if( mNeedsAlbedoMipmaps )
        {
            mRenderSystem->debugAnnotationPush( "Albedo Regular mipmaps" );
            mAlbedoVox->_autogenerateMipmaps();
            mRenderSystem->debugAnnotationPop();
        }

        mFullBuildDone = true;

        mRenderSystem->debugAnnotationPop();
        OgreProfileGpuEnd( "IVCT build" );
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::buildRelative( SceneManager *sceneManager, const int32 diffX,
                                           const int32 diffY, const int32 diffZ,
                                           const uint32 numOctantsX, const uint32 numOctantsY,
                                           const uint32 numOctantsZ )
    {
        if( !mFullBuildDone )
        {
            dividideOctants( numOctantsX, numOctantsY, numOctantsZ );
            build( sceneManager );

            if( mDebugVoxelVisualizer )
            {
                mRenderSystem->endCopyEncoder();
                setTextureToDebugVisualizer();

                Node *visNode = mDebugVoxelVisualizer->getParentNode();
                visNode->setPosition( getVoxelOrigin() );
                visNode->setScale( getVoxelCellSize() );

                // The visualizer is static so force-update its transform manually
                visNode->_getFullTransformUpdated();
                mDebugVoxelVisualizer->getWorldAabbUpdated();
            }

            return;
        }

        OgreProfile( "VctImageVoxelizer::buildRelative" );
        OgreProfileGpuBegin( "IVCT buildRelative" );

        mRenderSystem->endRenderPassDescriptor();

        mRenderSystem->debugAnnotationPush( "VctImageVoxelizer::buildRelative" );

        createVoxelTextures();
        createAltVoxelTextures();
        std::swap( mAlbedoVox, mAlbedoVoxAlt );
        std::swap( mEmissiveVox, mEmissiveVoxAlt );
        std::swap( mNormalVox, mNormalVoxAlt );
        setVoxelTexturesToJobs();

        {
            const TextureBox refBox = mAlbedoVox->getEmptyBox( 0u );
            TextureBox dstBox( refBox );
            TextureBox srcBox( refBox );

            dstBox.x += static_cast<uint32>( std::max( -diffX, 0 ) );
            dstBox.y += static_cast<uint32>( std::max( -diffY, 0 ) );
            dstBox.z += static_cast<uint32>( std::max( -diffZ, 0 ) );
            dstBox.width -= dstBox.x;
            dstBox.height -= dstBox.y;
            dstBox.depth -= dstBox.z;

            srcBox.x += static_cast<uint32>( std::max( diffX, 0 ) );
            srcBox.y += static_cast<uint32>( std::max( diffY, 0 ) );
            srcBox.z += static_cast<uint32>( std::max( diffZ, 0 ) );
            srcBox.width -= srcBox.x;
            srcBox.height -= srcBox.y;
            srcBox.depth -= srcBox.z;

            srcBox.width = std::min( srcBox.width, dstBox.width );
            srcBox.height = std::min( srcBox.height, dstBox.height );
            srcBox.depth = std::min( srcBox.depth, dstBox.depth );
            dstBox.width = srcBox.width;
            dstBox.height = srcBox.height;
            dstBox.depth = srcBox.depth;

            BarrierSolver &solver = mRenderSystem->getBarrierSolver();

            mResourceTransitions.clear();
            solver.resolveTransition( mResourceTransitions, mAlbedoVoxAlt, ResourceLayout::CopySrc,
                                      ResourceAccess::Read, 0u );
            solver.resolveTransition( mResourceTransitions, mEmissiveVoxAlt, ResourceLayout::CopySrc,
                                      ResourceAccess::Read, 0u );
            solver.resolveTransition( mResourceTransitions, mNormalVoxAlt, ResourceLayout::CopySrc,
                                      ResourceAccess::Read, 0u );
            solver.resolveTransition( mResourceTransitions, mAlbedoVox, ResourceLayout::CopyDst,
                                      ResourceAccess::Write, 0u );
            solver.resolveTransition( mResourceTransitions, mEmissiveVox, ResourceLayout::CopyDst,
                                      ResourceAccess::Write, 0u );
            solver.resolveTransition( mResourceTransitions, mNormalVox, ResourceLayout::CopyDst,
                                      ResourceAccess::Write, 0u );

            mRenderSystem->executeResourceTransition( mResourceTransitions );

            mAlbedoVoxAlt->copyTo( mAlbedoVox, dstBox, 0u, srcBox, 0u, false,
                                   CopyEncTransitionMode::AlreadyInLayoutThenManual,
                                   CopyEncTransitionMode::AlreadyInLayoutThenManual );
            mEmissiveVoxAlt->copyTo( mEmissiveVox, dstBox, 0u, srcBox, 0u, false,
                                     CopyEncTransitionMode::AlreadyInLayoutThenManual,
                                     CopyEncTransitionMode::AlreadyInLayoutThenManual );
            mNormalVoxAlt->copyTo( mNormalVox, dstBox, 0u, srcBox, 0u, false,
                                   CopyEncTransitionMode::AlreadyInLayoutThenManual,
                                   CopyEncTransitionMode::AlreadyInLayoutThenManual );
        }

        mOctants.swap( mTmpOctants );
        setOffsetOctants( diffX, diffY, diffZ );

        clearVoxels( true );

        fillInstanceBuffers( sceneManager );

        ShaderParams::Param paramInstanceRange;
        ShaderParams::Param paramVoxelOrigin;
        ShaderParams::Param paramVoxelCellSize;
        ShaderParams::Param paramVoxelPixelOrigin;
        ShaderParams::Param paramPixelsToWrite;

        paramInstanceRange.name = "instanceStart_instanceEnd";
        paramVoxelOrigin.name = "voxelOrigin";
        paramVoxelCellSize.name = "voxelCellSize";
        paramVoxelPixelOrigin.name = "voxelPixelOrigin";
        paramPixelsToWrite.name = "pixelsToWrite";

        paramVoxelCellSize.setManualValue( getVoxelCellSize() );

        OgreProfileGpuBegin( "IVCT Image Voxelization Jobs" );

        const uint32 *threadsPerGroup = mImageVoxelizerJob->getThreadsPerGroup();

        const size_t numOctants = mOctants.size();

        if( mImageVoxelizerJob->getProperty( "check_out_of_bounds" ) == 0 )
            mImageVoxelizerJob->setProperty( "check_out_of_bounds", 1 );

        HlmsCompute *hlmsCompute = mHlmsManager->getComputeHlms();
        ShaderParams &shaderParams = mImageVoxelizerJob->getShaderParams( "default" );

        const RenderSystemCapabilities *caps = mHlmsManager->getRenderSystem()->getCapabilities();
        const bool bSetSampler = !caps->hasCapability( RSC_SEPARATE_SAMPLERS_FROM_TEXTURES );

        const size_t numItems = mItems.size();

        BatchArray::const_iterator itBatch = mBatches.begin();
        BatchArray::const_iterator enBatch = mBatches.end();

        while( itBatch != enBatch )
        {
            {
                // texIdx starts at 1 because mInstanceBuffer goes at 0; and whether
                // that consumes a tex slot or not is a rabbit hole
                // (depends on VaoManager::readOnlyIsTexBuffer AND the API used) so we
                // just assume we always consume 1
                size_t texIdx = 1u;
                DescriptorSetTexture2::TextureSlot texSlot(
                    DescriptorSetTexture2::TextureSlot::makeEmpty() );
                FastArray<TextureGpu *>::const_iterator itTex = itBatch->textures.begin();
                FastArray<TextureGpu *>::const_iterator enTex = itBatch->textures.end();

                while( itTex != enTex )
                {
                    texSlot.texture = *itTex;
                    // Sampler is nullptr because it was already set at init
                    mImageVoxelizerJob->setTexture( static_cast<uint8>( texIdx ), texSlot, 0,
                                                    bSetSampler );
                    ++texIdx;
                    ++itTex;
                }
            }

            for( size_t i = 0u; i < numOctants; ++i )
            {
                if( itBatch->instances[i].numInstances > 0u )
                {
                    const Octant &octant = mOctants[i];

                    if( octant.diffAxis == 0u )
                        mImageVoxelizerJob->setThreadsPerGroup( 8u, 8u, 1u );
                    else if( octant.diffAxis == 1u )
                        mImageVoxelizerJob->setThreadsPerGroup( 8u, 1u, 8u );
                    else  // if( octant.diffAxis == 2u )
                        mImageVoxelizerJob->setThreadsPerGroup( 1u, 8u, 8u );

                    mImageVoxelizerJob->setNumThreadGroups(
                        static_cast<uint32>( alignToNextMultiple( octant.width, threadsPerGroup[0] ) ) /
                            threadsPerGroup[0],
                        static_cast<uint32>( alignToNextMultiple( octant.height, threadsPerGroup[1] ) ) /
                            threadsPerGroup[1],
                        static_cast<uint32>( alignToNextMultiple( octant.depth, threadsPerGroup[2] ) ) /
                            threadsPerGroup[2] );

                    const uint32 instanceRange[2] = {
                        itBatch->instances[i].instanceOffset + static_cast<uint32>( i * numItems ),
                        itBatch->instances[i].instanceOffset + itBatch->instances[i].numInstances +
                            static_cast<uint32>( i * numItems )
                    };
                    const uint32 voxelPixelOrigin[3] = { octant.x, octant.y, octant.z };

                    paramInstanceRange.setManualValue( instanceRange, 2u );
                    paramVoxelOrigin.setManualValue( octant.region.getMinimum() );
                    paramVoxelPixelOrigin.setManualValue( voxelPixelOrigin, 3u );
                    paramPixelsToWrite.setManualValue( &octant.width, 3u );

                    shaderParams.mParams.clear();
                    shaderParams.mParams.push_back( paramInstanceRange );
                    shaderParams.mParams.push_back( paramVoxelOrigin );
                    shaderParams.mParams.push_back( paramVoxelCellSize );
                    shaderParams.mParams.push_back( paramVoxelPixelOrigin );
                    shaderParams.mParams.push_back( paramPixelsToWrite );
                    shaderParams.setDirty();

                    mImageVoxelizerJob->analyzeBarriers( mResourceTransitions );
                    mRenderSystem->executeResourceTransition( mResourceTransitions );
                    hlmsCompute->dispatch( mImageVoxelizerJob, 0, 0 );
                }
            }

            ++itBatch;
        }

        OgreProfileGpuEnd( "IVCT Image Voxelization Jobs" );

        // This texture is no longer needed, it's not used for the injection phase. Save memory.
        mAccumValVox->scheduleTransitionTo( GpuResidency::OnStorage );

        if( mNeedsAlbedoMipmaps )
        {
            mRenderSystem->debugAnnotationPush( "Albedo Regular mipmaps" );
            mAlbedoVox->_autogenerateMipmaps();
            mRenderSystem->debugAnnotationPop();
        }

        mOctants.swap( mTmpOctants );

        if( mDebugVoxelVisualizer )
        {
            mRenderSystem->endCopyEncoder();
            setTextureToDebugVisualizer();

            Node *visNode = mDebugVoxelVisualizer->getParentNode();
            visNode->setPosition( getVoxelOrigin() );
            visNode->setScale( getVoxelCellSize() );

            // The visualizer is static so force-update its transform manually
            visNode->_getFullTransformUpdated();
            mDebugVoxelVisualizer->getWorldAabbUpdated();
        }

        mRenderSystem->debugAnnotationPop();

        OgreProfileGpuEnd( "IVCT buildRelative" );
    }
}  // namespace Ogre
