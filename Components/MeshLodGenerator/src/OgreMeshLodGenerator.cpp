/*
 * -----------------------------------------------------------------------------
 * This source file is part of OGRE-Next
 * (Object-oriented Graphics Rendering Engine)
 * For the latest info, see http://www.ogre3d.org/
 *
 * Copyright (c) 2000-2014 Torus Knot Software Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -----------------------------------------------------------------------------
 */

#include "OgreMeshLodGenerator.h"

#include "OgreLodCollapseCost.h"
#include "OgreLodCollapseCostCurvature.h"
#include "OgreLodCollapseCostOutside.h"
#include "OgreLodCollapseCostProfiler.h"
#include "OgreLodCollapser.h"
#include "OgreLodData.h"
#include "OgreLodInputProvider.h"
#include "OgreLodInputProviderBuffer.h"
#include "OgreLodInputProviderMesh.h"
#include "OgreLodInputProviderMeshV2.h"
#include "OgreLodOutputProvider.h"
#include "OgreLodOutputProviderBuffer.h"
#include "OgreLodOutputProviderCompressedBuffer.h"
#include "OgreLodOutputProviderCompressedMesh.h"
#include "OgreLodOutputProviderMesh.h"
#include "OgreLodOutputProviderMeshV2.h"
#include "OgreLodWorkQueueInjector.h"
#include "OgreLodWorkQueueWorker.h"
#include "OgreMesh.h"
#include "OgreMesh2.h"
#include "OgreStringConverter.h"
#include "OgrePixelCountLodStrategy.h"

namespace Ogre
{
    template <>
    MeshLodGenerator *Singleton<MeshLodGenerator>::msSingleton = 0;
    MeshLodGenerator *MeshLodGenerator::getSingletonPtr() { return msSingleton; }
    MeshLodGenerator &MeshLodGenerator::getSingleton()
    {
        assert( msSingleton );
        return ( *msSingleton );
    }
    void MeshLodGenerator::getAutoconfig( v1::MeshPtr &inMesh, LodConfig &outLodConfig )
    {
        outLodConfig.mesh = inMesh;
        outLodConfig.strategy = PixelCountLodStrategy::getSingletonPtr();
        LodLevel lodLevel;
        lodLevel.reductionMethod = LodLevel::VRM_COLLAPSE_COST;
        Real radius = inMesh->getBoundingSphereRadius();
        for( int i = 2; i < 6; i++ )
        {
            Real i4 = (Real)( i * i * i * i );
            Real i5 = i4 * (Real)i;
            // Distance = pixel count
            // Constant: zoom of the Lod. This could be scaled based on resolution.
            //     Higher constant means first Lod is nearer to camera. Smaller constant means the first
            //     Lod is further away from camera.
            // i4: The stretching. Normally you want to have more Lods in the near, then in far away.
            //     i4 means distance is divided by 16=(2*2*2*2), 81, 256, 625=(5*5*5*5).
            //     if 16 would be smaller, the first Lod would be nearer. if 625 would be bigger, the
            //     last Lod would be further awaay.
            // if you increase 16 and decrease 625, first and Last Lod distance would be smaller.
            lodLevel.distance = 3388608.f / i4;

            // reductionValue = collapse cost
            // Radius: Edges are multiplied by the length, when calculating collapse cost. So as a base
            // value we use radius, which should help in balancing collapse cost to any mesh size. The
            // constant and i5 are playing together. 1/(1/100k*i5) You need to determine the quality of
            // nearest Lod and the furthest away first. I have chosen 1/(1/100k*(2^5)) = 3125 for nearest
            // Lod and 1/(1/100k*(5^5)) = 32 for nearest Lod. if you divide radius by a bigger number, it
            // means smaller reduction. So radius/3125 is very small reduction for nearest Lod. if you
            // divide radius by a smaller number, it means bigger reduction. So radius/32 means
            // aggressive reduction for furthest away lod. current values: 3125, 411, 97, 32
            lodLevel.reductionValue = radius / 100000.f * i5;
            outLodConfig.levels.push_back( lodLevel );
        }
    }

    void MeshLodGenerator::getAutoconfig( MeshPtr &inMesh, LodConfig &outLodConfig )
    {
        // Identical heuristic to the v1 overload above -- only the bounding-sphere
        // source and which LodConfig field gets set differ.
        outLodConfig.meshV2 = inMesh;
        outLodConfig.strategy = PixelCountLodStrategy::getSingletonPtr();
        LodLevel lodLevel;
        lodLevel.reductionMethod = LodLevel::VRM_COLLAPSE_COST;
        Real radius = inMesh->getBoundingSphereRadius();
        for( int i = 2; i < 6; i++ )
        {
            Real i4 = (Real)( i * i * i * i );
            Real i5 = i4 * (Real)i;
            lodLevel.distance = 3388608.f / i4;
            lodLevel.reductionValue = radius / 100000.f * i5;
            outLodConfig.levels.push_back( lodLevel );
        }
    }

    void MeshLodGenerator::generateAutoconfiguredLodLevels( v1::MeshPtr &mesh )
    {
        LodConfig lodConfig;
        getAutoconfig( mesh, lodConfig );
        generateLodLevels( lodConfig );
    }

    void MeshLodGenerator::_configureMeshLodUsage( const LodConfig &lodConfig )
    {
        bool edgeListWasBuilt = lodConfig.mesh->isEdgeListBuilt();
        lodConfig.mesh->freeEdgeList();
        lodConfig.mesh->setLodStrategyName( lodConfig.strategy->getName() );
        v1::MeshLodUsage usage;
        size_t n = 0;
        lodConfig.mesh->_setLodInfo( uint16( lodConfig.levels.size() + 1u ) );  // add Lod levels
        for( size_t i = 0; i < lodConfig.levels.size(); i++ )
        {
            // Record usages. First Lod usage is the mesh itself.

            // Skip lods, which have the same amount of vertices. No buffer generated for them.
            if( !lodConfig.levels[i].outSkipped )
            {
                usage.userValue = lodConfig.levels[i].distance;
                usage.value = lodConfig.strategy->transformUserValue( usage.userValue );
                usage.edgeData = NULL;
                usage.manualMesh.reset();
                usage.manualName = lodConfig.levels[i].manualMeshName;
                lodConfig.mesh->_setLodUsage( uint16( ++n ), usage );
            }
        }
        // Remove skipped Lod levels
        lodConfig.mesh->_setLodInfo( uint16( n + 1u ) );
        if( edgeListWasBuilt )
            lodConfig.mesh->buildEdgeList();
    }

    void MeshLodGenerator::_configureMeshLodUsageV2( const LodConfig &lodConfig )
    {
        // v2 equivalent of _configureMeshLodUsage() above. There is no edge list or
        // MeshLodUsage record to maintain on v2 Mesh -- only Mesh::mLodValues, read
        // directly off the strategy-transformed distance/pixel value, one entry per
        // *kept* (non-outSkipped) level, in the same order LodOutputProviderMeshV2
        // pushed VAOs onto each SubMesh::mVao[...].
        Mesh::LodValueArray lodValues;
        lodValues.reserve( lodConfig.levels.size() + 1u );

        // Placeholder for the base (LOD0) entry. We deliberately do NOT use
        // lodConfig.strategy->getBaseValue() here: it is not guaranteed to share the
        // same numeric convention as transformUserValue()'s output for every
        // strategy (e.g. for a pixel-count-style strategy, "more detail" may mean a
        // *larger* raw value, the opposite of distance-style strategies), and lodSet()
        // (OgreLodStrategyPrivate.inl) requires this array strictly ascending via
        // std::lower_bound. Filled in below once the first real threshold is known.
        lodValues.push_back( Real( 0 ) );

        for( size_t i = 0; i < lodConfig.levels.size(); ++i )
        {
            if( !lodConfig.levels[i].outSkipped )
            {
                lodValues.push_back(
                    lodConfig.strategy->transformUserValue( lodConfig.levels[i].distance ) );
            }
        }

        if( lodValues.size() > 1u )
        {
            // Duplicating the first real threshold into slot 0 is correct regardless
            // of the strategy's value convention: lower_bound() behaves identically
            // for any query value below this threshold whether slot 0 holds a true
            // minimum or an exact copy of slot 1, and equality trivially satisfies
            // the ascending requirement.
            lodValues[0] = lodValues[1];
        }
        else
        {
            // Degenerate case: every level was outSkipped, so there are no generated
            // LOD levels at all (mVao[VpNormal] never grew past LOD-0). Fall back to
            // getBaseValue() here since there is no other threshold to copy from --
            // this entry will never actually be compared against a second one.
            lodValues[0] = lodConfig.strategy->getBaseValue();
        }

#if OGRE_DEBUG_MODE
        for( size_t i = 1u; i < lodValues.size(); ++i )
        {
            if( lodValues[i - 1u] > lodValues[i] )
            {
                Ogre::String dump;
                for( size_t j = 0; j < lodValues.size(); ++j )
                    dump += Ogre::StringConverter::toString( lodValues[j] ) + " ";
                LogManager::getSingleton().logMessage(
                    "ERROR: MeshLodGenerator::_configureMeshLodUsageV2: lodValues not "
                    "ascending: " +
                    dump );
            }
        }
#endif

        lodConfig.meshV2->setLodStrategyName( lodConfig.strategy->getName() );
        lodConfig.meshV2->_setLodValues( lodValues );
    }

    MeshLodGenerator::MeshLodGenerator() : mWQWorker( NULL ), mWQInjector( NULL ) {}

    MeshLodGenerator::~MeshLodGenerator()
    {
        delete mWQWorker;
        delete mWQInjector;
    }
    void MeshLodGenerator::_resolveComponents( LodConfig &lodConfig, LodCollapseCostPtr &cost,
                                               LodDataPtr &data, LodInputProviderPtr &input,
                                               LodOutputProviderPtr &output, LodCollapserPtr &collapser )
    {
        if( !cost )
        {
            cost = LodCollapseCostPtr( new LodCollapseCostCurvature );
            if( lodConfig.advanced.outsideWeight != 0 )
            {
                cost = LodCollapseCostPtr( new LodCollapseCostOutside(
                    cost, lodConfig.advanced.outsideWeight, lodConfig.advanced.outsideWalkAngle ) );
            }
            if( !lodConfig.advanced.profile.empty() )
            {
                cost = LodCollapseCostPtr(
                    new LodCollapseCostProfiler( lodConfig.advanced.profile, cost ) );
            }
        }
        if( !data )
        {
            data = LodDataPtr( new LodData() );
        }
        if( !collapser )
        {
            collapser = LodCollapserPtr( new LodCollapser() );
        }
        if( lodConfig.advanced.useBackgroundQueue )
        {
            // V2 meshes are not (yet) supported on the background queue path -- the
            // existing LodInputProviderBuffer/LodOutputProvider*Buffer classes are
            // built around v1::Mesh's WorkQueue-friendly buffer-copy contract. A v2
            // background-queue variant is explicitly out of scope for this change;
            // generateLodLevels() rejects useBackgroundQueue + isV2() before reaching
            // here (see below) so this branch is unreachable for v2 configs today.
            if( !input )
            {
                input = LodInputProviderPtr( new LodInputProviderBuffer( lodConfig.mesh ) );
            }
            if( !output )
            {
                if( lodConfig.advanced.useCompression )
                {
                    output =
                        LodOutputProviderPtr( new LodOutputProviderCompressedBuffer( lodConfig.mesh ) );
                }
                else
                {
                    output = LodOutputProviderPtr( new LodOutputProviderBuffer( lodConfig.mesh ) );
                }
            }
        }
        else
        {
            if( !input )
            {
                if( lodConfig.isV2() )
                    input = LodInputProviderPtr( new LodInputProviderMeshV2( lodConfig.meshV2 ) );
                else
                    input = LodInputProviderPtr( new LodInputProviderMesh( lodConfig.mesh ) );
            }
            if( !output )
            {
                if( lodConfig.isV2() )
                {
                    // Note: compressed output (LodOutputProviderCompressedMesh) has no
                    // v2 equivalent yet -- v2's Vao-based storage doesn't have the same
                    // "shared faces with frame shifting" representation that mechanism
                    // was built around. useCompression is silently ignored for v2 for
                    // now; flagged here rather than in the PR description alone so it
                    // isn't missed during review.
                    output = LodOutputProviderPtr( new LodOutputProviderMeshV2( lodConfig.meshV2 ) );
                }
                else if( lodConfig.advanced.useCompression )
                {
                    output =
                        LodOutputProviderPtr( new LodOutputProviderCompressedMesh( lodConfig.mesh ) );
                }
                else
                {
                    output = LodOutputProviderPtr( new LodOutputProviderMesh( lodConfig.mesh ) );
                }
            }
        }
    }
    void MeshLodGenerator::_process( LodConfig &lodConfig, LodCollapseCost *cost, LodData *data,
                                     LodInputProvider *input, LodOutputProvider *output,
                                     LodCollapser *collapser )
    {
        input->initData( data );
        data->mUseVertexNormals = data->mUseVertexNormals && lodConfig.advanced.useVertexNormals;
        cost->initCollapseCosts( data );
        output->prepare( data );
        computeLods( lodConfig, data, cost, output, collapser );
        output->finalize( data );
        if( !lodConfig.advanced.useBackgroundQueue )
        {
            // This will be processed in LodWorkQueueInjector if we use background queue.
            output->inject();
            if( lodConfig.isV2() )
                _configureMeshLodUsageV2( lodConfig );
            else
                _configureMeshLodUsage( lodConfig );
            // lodConfig.mesh->buildEdgeList();
        }
    }
    void MeshLodGenerator::generateLodLevels( LodConfig &lodConfig, LodCollapseCostPtr cost,
                                              LodDataPtr data, LodInputProviderPtr input,
                                              LodOutputProviderPtr output, LodCollapserPtr collapser )
    {
        if( lodConfig.isV2() && lodConfig.advanced.useBackgroundQueue )
        {
            // See the comment in _resolveComponents(): the WorkQueue-based providers
            // are built around v1::Mesh's buffer-copy contract and have no v2
            // equivalent yet. Fail loudly here instead of silently falling through to
            // a v1-only code path with a null lodConfig.mesh.
            OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                         "useBackgroundQueue is not yet supported for v2 meshes.",
                         "MeshLodGenerator::generateLodLevels" );
        }

        // If we don't have generated Lod levels, we can use _generateManualLodLevels.
        bool hasGeneratedLevels = false;
        for( size_t i = 0; i < lodConfig.levels.size(); i++ )
        {
            if( lodConfig.levels[i].manualMeshName.empty() )
            {
                hasGeneratedLevels = true;
                break;
            }
        }
        if( hasGeneratedLevels || ( LodWorkQueueInjector::getSingletonPtr() &&
                                    LodWorkQueueInjector::getSingletonPtr()->getInjectorListener() ) )
        {
            _resolveComponents( lodConfig, cost, data, input, output, collapser );
            if( lodConfig.advanced.useBackgroundQueue )
            {
                _initWorkQueue();
                LodWorkQueueWorker::getSingleton().addRequestToQueue( lodConfig, cost, data, input,
                                                                      output, collapser );
            }
            else
            {
                _process( lodConfig, cost.get(), data.get(), input.get(), output.get(),
                          collapser.get() );
            }
        }
        else
        {
            _generateManualLodLevels( lodConfig );
        }

        if( lodConfig.isV2() )
        {
            // Deliberately NOT calling lodConfig.meshV2->prepareForShadowMapping()
            // here (unlike the v1 branch below). That triggers
            // VertexShadowMapHelper::optimizeForShadowMapping() when
            // Mesh::msOptimizeForShadowMapping is enabled (likely the default),
            // which rebuilds mVao[VpShadow] from scratch independently of
            // mVao[VpNormal] -- a code path this change has not audited, and one
            // that risks breaking the shared-VAO invariant
            // (mVao[VpNormal][0] == mVao[VpShadow][0]) that
            // SubMesh::destroyShadowMappingVaos() relies on to safely skip
            // double-destroying buffers on mesh teardown. LodOutputProviderMeshV2::
            // bakeLodLevel() already keeps mVao[VpShadow] correctly in lockstep
            // with mVao[VpNormal] for the common (shared-buffers) case, which is
            // exactly the case that safety check handles correctly. Callers who
            // specifically need independent/optimized shadow Vaos can call
            // Mesh::prepareForShadowMapping(true) themselves afterwards -- see the
            // WARNING LodOutputProviderMeshV2::bakeLodLevel() logs for that case.
        }
        else
        {
            lodConfig.mesh->prepareForShadowMapping( false );
        }
    }

    void MeshLodGenerator::computeLods( LodConfig &lodConfig, LodData *data, LodCollapseCost *cost,
                                        LodOutputProvider *output, LodCollapser *collapser )
    {
        int lodID = 0;
        size_t lastBakeVertexCount = data->mVertexList.size();
        for( unsigned short curLod = 0; curLod < lodConfig.levels.size(); curLod++ )
        {
            if( !lodConfig.levels[curLod].manualMeshName.empty() )
            {
                // Manual Lod level
                lodConfig.levels[curLod].outSkipped =
                    ( curLod != 0 && lodConfig.levels[curLod].manualMeshName ==
                                         lodConfig.levels[curLod - 1].manualMeshName );
                lodConfig.levels[curLod].outUniqueVertexCount = 0;
                lastBakeVertexCount = std::numeric_limits<size_t>::max();
                if( !lodConfig.levels[curLod].outSkipped )
                {
                    output->bakeManualLodLevel( data, lodConfig.levels[curLod].manualMeshName, lodID++ );
                }
            }
            else
            {
                size_t vertexCountLimit;
                Real collapseCostLimit;
                calcLodVertexCount( lodConfig.levels[curLod], data->mVertexList.size(), vertexCountLimit,
                                    collapseCostLimit );
                collapser->collapse( data, cost, output, static_cast<int>( vertexCountLimit ),
                                     collapseCostLimit );
                size_t vertexCount = data->mCollapseCostHeap.size();
                lodConfig.levels[curLod].outUniqueVertexCount = vertexCount;
                lodConfig.levels[curLod].outSkipped = ( vertexCount == lastBakeVertexCount );
                if( !lodConfig.levels[curLod].outSkipped )
                {
                    lastBakeVertexCount = vertexCount;
                    output->bakeLodLevel( data, lodID++ );
                }
            }
        }
    }
    void MeshLodGenerator::calcLodVertexCount( const LodLevel &lodLevel, size_t uniqueVertexCount,
                                               size_t &outVertexCountLimit, Real &outCollapseCostLimit )
    {
        switch( lodLevel.reductionMethod )
        {
        case LodLevel::VRM_PROPORTIONAL:
            outCollapseCostLimit = LodData::NEVER_COLLAPSE_COST;
            outVertexCountLimit =
                uniqueVertexCount - (size_t)( (Real)uniqueVertexCount * lodLevel.reductionValue );
            break;

        case LodLevel::VRM_CONSTANT:
            outCollapseCostLimit = LodData::NEVER_COLLAPSE_COST;
            outVertexCountLimit = (size_t)lodLevel.reductionValue;
            if( outVertexCountLimit < uniqueVertexCount )
            {
                outVertexCountLimit = uniqueVertexCount - outVertexCountLimit;
            }
            else
            {
                outVertexCountLimit = 0;
            }
            break;

        case LodLevel::VRM_COLLAPSE_COST:
            outCollapseCostLimit = lodLevel.reductionValue;
            outVertexCountLimit = 0;
            break;

        default:
            OgreAssert( 0, "" );
            outCollapseCostLimit = LodData::NEVER_COLLAPSE_COST;
            outVertexCountLimit = uniqueVertexCount;
            break;
        }
    }

    void MeshLodGenerator::_generateManualLodLevels( LodConfig &lodConfig )
    {
        if( lodConfig.isV2() )
        {
            // Manual LOD levels are not supported for v2 meshes -- see the comment in
            // LodOutputProviderMeshV2::bakeManualLodLevel. This path is v1-only
            // (constructs a LodOutputProviderMesh directly below), so fail loudly
            // here rather than passing a null v1::MeshPtr into it.
            OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                         "All-manual Lod level configs are not supported for v2 meshes.",
                         "MeshLodGenerator::_generateManualLodLevels" );
        }

        LodOutputProviderMesh output( lodConfig.mesh );
        output.prepare( NULL );
        for( unsigned short curLod = 0; curLod < lodConfig.levels.size(); curLod++ )
        {
            OgreAssert( !lodConfig.levels[curLod].manualMeshName.empty(),
                        "Only manual Lod levels are supported! Call generateLodLevels() instead!" );
            lodConfig.levels[curLod].outSkipped = false;
            lodConfig.levels[curLod].outUniqueVertexCount = 0;
            output.bakeManualLodLevel( NULL, lodConfig.levels[curLod].manualMeshName, curLod );
        }
        output.finalize( NULL );
        output.inject();
        _configureMeshLodUsage( lodConfig );
    }

    void MeshLodGenerator::_initWorkQueue()
    {
        if( !LodWorkQueueWorker::getSingletonPtr() )
        {
            mWQWorker = new LodWorkQueueWorker();
        }
        if( !LodWorkQueueInjector::getSingletonPtr() )
        {
            mWQInjector = new LodWorkQueueInjector();
        }
    }

}  // namespace Ogre