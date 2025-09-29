/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2021 Torus Knot Software Ltd

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

#include "Terra/Terra.h"

#include "Terra/Hlms/OgreHlmsTerraDatablock.h"

#include "Compositor/OgreCompositorChannel.h"
#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "OgreCamera.h"
#include "OgreDepthBuffer.h"
#include "OgreImage2.h"
#include "OgreMaterialManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreSceneManager.h"
#include "OgreStagingTexture.h"
#include "OgreTechnique.h"
#include "OgreTextureGpuManager.h"
#include "Terra/Hlms/OgreHlmsTerra.h"
#include "Terra/TerraShadowMapper.h"

namespace Ogre
{
    inline Vector3 ZupToYup( Vector3 value )
    {
        std::swap( value.y, value.z );
        value.z = -value.z;
        return value;
    }

    inline Ogre::Vector3 YupToZup( Ogre::Vector3 value )
    {
        std::swap( value.y, value.z );
        value.y = -value.y;
        return value;
    }

    /*inline Ogre::Quaternion ZupToYup( Ogre::Quaternion value )
    {
        return value * Ogre::Quaternion( Ogre::Radian( Ogre::Math::HALF_PI ), Ogre::Vector3::UNIT_X );
    }*/

    Terra::Terra( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *sceneManager,
                  uint8 renderQueueId, CompositorManager2 *compositorManager, Camera *camera,
                  bool zUp ) :
        MovableObject( id, objectMemoryManager, sceneManager, renderQueueId ),
        m_width( 0u ),
        m_depth( 0u ),
        m_depthWidthRatio( 1.0f ),
        m_skirtSize( 10.0f ),
        m_invWidth( 1.0f ),
        m_invDepth( 1.0f ),
        m_zUp( zUp ),
        m_xzDimensions( Vector2::UNIT_SCALE ),
        m_xzInvDimensions( Vector2::UNIT_SCALE ),
        m_xzRelativeSize( Vector2::UNIT_SCALE ),
        m_height( 1.0f ),
        m_heightUnormScaled( 1.0f ),
        m_terrainOrigin( Vector3::ZERO ),
        m_basePixelDimension( 256u ),
        m_currentCell( 0u ),
        m_heightMapTex( 0 ),
        m_normalMapTex( 0 ),
        m_prevLightDir( Vector3::ZERO ),
        m_shadowMapper( 0 ),
        m_sharedResources( 0 ),
        m_compositorManager( compositorManager ),
        m_camera( camera ),
        mHlmsTerraIndex( std::numeric_limits<uint32>::max() )
    {
    }
    //-----------------------------------------------------------------------------------
    Terra::~Terra()
    {
        if( !m_terrainCells[0].empty() && m_terrainCells[0].back().getDatablock() )
        {
            HlmsDatablock *datablock = m_terrainCells[0].back().getDatablock();
            OGRE_ASSERT_HIGH( dynamic_cast<HlmsTerra *>( datablock->getCreator() ) );
            HlmsTerra *hlms = static_cast<HlmsTerra *>( datablock->getCreator() );
            hlms->_unlinkTerra( this );
        }

        if( m_shadowMapper )
        {
            m_shadowMapper->destroyShadowMap();
            delete m_shadowMapper;
            m_shadowMapper = 0;
        }
        destroyNormalTexture();
        destroyHeightmapTexture();
        m_terrainCells[0].clear();
        m_terrainCells[1].clear();
    }
    //-----------------------------------------------------------------------------------
    Vector3 Terra::fromYUp( Vector3 value ) const
    {
        if( m_zUp )
            return YupToZup( value );
        return value;
    }
    //-----------------------------------------------------------------------------------
    Vector3 Terra::fromYUpSignPreserving( Vector3 value ) const
    {
        if( m_zUp )
            std::swap( value.y, value.z );
        return value;
    }
    //-----------------------------------------------------------------------------------
    Vector3 Terra::toYUp( Vector3 value ) const
    {
        if( m_zUp )
            return ZupToYup( value );
        return value;
    }
    //-----------------------------------------------------------------------------------
    Vector3 Terra::toYUpSignPreserving( Vector3 value ) const
    {
        if( m_zUp )
            std::swap( value.y, value.z );
        return value;
    }
    //-----------------------------------------------------------------------------------
    void Terra::destroyHeightmapTexture()
    {
        if( m_heightMapTex )
        {
            TextureGpuManager *textureManager =
                mManager->getDestinationRenderSystem()->getTextureGpuManager();
            textureManager->destroyTexture( m_heightMapTex );
            m_heightMapTex = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void Terra::createHeightmapTexture( const Ogre::Image2 &image, const String &imageName )
    {
        destroyHeightmapTexture();

        if( image.getPixelFormat() != PFG_R8_UNORM && image.getPixelFormat() != PFG_R16_UNORM &&
            image.getPixelFormat() != PFG_R32_FLOAT )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Texture " + imageName + "must be greyscale 8 bpp, 16 bpp, or 32-bit Float",
                         "Terra::createHeightmapTexture" );
        }

        // const uint8 numMipmaps = image.getNumMipmaps();

        TextureGpuManager *textureManager =
            mManager->getDestinationRenderSystem()->getTextureGpuManager();
        m_heightUnormScaled = m_height;
        PixelFormatGpu pixelFormat = image.getPixelFormat();
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
        // Many Android GPUs don't support PFG_R16_UNORM so we scale it by hand
        if( pixelFormat == PFG_R16_UNORM &&
            !textureManager->checkSupport( PFG_R16_UNORM, TextureTypes::Type2D, 0 ) )
        {
            pixelFormat = PFG_R16_UINT;
            m_heightUnormScaled /= 65535.0f;
        }
#endif

        m_heightMapTex = textureManager->createTexture(
            "HeightMapTex" + StringConverter::toString( getId() ), GpuPageOutStrategy::SaveToSystemRam,
            TextureFlags::ManualTexture, TextureTypes::Type2D );
        m_heightMapTex->setResolution( image.getWidth(), image.getHeight() );
        m_heightMapTex->setPixelFormat( pixelFormat );
        m_heightMapTex->scheduleTransitionTo( GpuResidency::Resident );

        StagingTexture *stagingTexture = textureManager->getStagingTexture(
            image.getWidth(), image.getHeight(), 1u, 1u, pixelFormat );
        stagingTexture->startMapRegion();
        TextureBox texBox =
            stagingTexture->mapRegion( image.getWidth(), image.getHeight(), 1u, 1u, pixelFormat );

        // for( uint8 mip=0; mip<numMipmaps; ++mip )
        texBox.copyFrom( image.getData( 0 ) );
        stagingTexture->stopMapRegion();
        stagingTexture->upload( texBox, m_heightMapTex, 0, 0, 0 );
        textureManager->removeStagingTexture( stagingTexture );
        stagingTexture = 0;
    }
    //-----------------------------------------------------------------------------------
    void Terra::createHeightmap( Image2 &image, const String &imageName, bool bMinimizeMemoryConsumption,
                                 bool bLowResShadow )
    {
        m_width = image.getWidth();
        m_depth = image.getHeight();
        m_depthWidthRatio = float( m_depth ) / float( m_width );
        m_invWidth = 1.0f / float( m_width );
        m_invDepth = 1.0f / float( m_depth );

        // image.generateMipmaps( false, Image::FILTER_NEAREST );

        createHeightmapTexture( image, imageName );

        m_heightMap.resize( m_width * m_depth );

        float fBpp = (float)( PixelFormatGpuUtils::getBytesPerPixel( image.getPixelFormat() ) << 3u );
        const float maxValue = powf( 2.0f, fBpp ) - 1.0f;
        const float invMaxValue = 1.0f / maxValue;

        if( image.getPixelFormat() == PFG_R8_UNORM )
        {
            for( uint32 y = 0; y < m_depth; ++y )
            {
                TextureBox texBox = image.getData( 0 );
                const uint8 *RESTRICT_ALIAS data =
                    reinterpret_cast<uint8 * RESTRICT_ALIAS>( texBox.at( 0, y, 0 ) );
                for( uint32 x = 0; x < m_width; ++x )
                    m_heightMap[y * m_width + x] = ( data[x] * invMaxValue ) * m_height;
            }
        }
        else if( image.getPixelFormat() == PFG_R16_UNORM )
        {
            TextureBox texBox = image.getData( 0 );
            for( uint32 y = 0; y < m_depth; ++y )
            {
                const uint16 *RESTRICT_ALIAS data =
                    reinterpret_cast<uint16 * RESTRICT_ALIAS>( texBox.at( 0, y, 0 ) );
                for( uint32 x = 0; x < m_width; ++x )
                    m_heightMap[y * m_width + x] = ( data[x] * invMaxValue ) * m_height;
            }
        }
        else if( image.getPixelFormat() == PFG_R32_FLOAT )
        {
            TextureBox texBox = image.getData( 0 );
            for( uint32 y = 0; y < m_depth; ++y )
            {
                const float *RESTRICT_ALIAS data =
                    reinterpret_cast<float * RESTRICT_ALIAS>( texBox.at( 0, y, 0 ) );
                for( uint32 x = 0; x < m_width; ++x )
                    m_heightMap[y * m_width + x] = data[x] * m_height;
            }
        }

        m_xzRelativeSize =
            m_xzDimensions / Vector2( static_cast<Real>( m_width ), static_cast<Real>( m_depth ) );

        createNormalTexture();

        m_prevLightDir = Vector3::ZERO;

        delete m_shadowMapper;
        m_shadowMapper = new ShadowMapper( mManager, m_compositorManager );
        m_shadowMapper->_setSharedResources( m_sharedResources );
        m_shadowMapper->setMinimizeMemoryConsumption( bMinimizeMemoryConsumption );
        m_shadowMapper->createShadowMap( getId(), m_heightMapTex, bLowResShadow );

        calculateOptimumSkirtSize();
    }
    //-----------------------------------------------------------------------------------
    void Terra::createNormalTexture()
    {
        destroyNormalTexture();

        TextureGpuManager *textureManager =
            mManager->getDestinationRenderSystem()->getTextureGpuManager();
        m_normalMapTex = textureManager->createTexture(
            "NormalMapTex_" + StringConverter::toString( getId() ), GpuPageOutStrategy::SaveToSystemRam,
            TextureFlags::ManualTexture, TextureTypes::Type2D );
        m_normalMapTex->setResolution( m_heightMapTex->getWidth(), m_heightMapTex->getHeight() );
        m_normalMapTex->setNumMipmaps( PixelFormatGpuUtils::getMaxMipmapCount(
            m_normalMapTex->getWidth(), m_normalMapTex->getHeight() ) );
        if( textureManager->checkSupport(
                PFG_R10G10B10A2_UNORM, TextureTypes::Type2D,
                TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps ) )
        {
            m_normalMapTex->setPixelFormat( PFG_R10G10B10A2_UNORM );
        }
        else
        {
            m_normalMapTex->setPixelFormat( PFG_RGBA8_UNORM );
        }
        m_normalMapTex->scheduleTransitionTo( GpuResidency::Resident );

        Ogre::TextureGpu *tmpRtt = TerraSharedResources::getTempTexture(
            "TMP NormalMapTex_", getId(), m_sharedResources, TerraSharedResources::TmpNormalMap,
            m_normalMapTex, TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps );

        MaterialPtr normalMapperMat =
            std::static_pointer_cast<Material>( MaterialManager::getSingleton().load(
                "Terra/GpuNormalMapper", ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME ) );
        Pass *pass = normalMapperMat->getTechnique( 0 )->getPass( 0 );
        TextureUnitState *texUnit = pass->getTextureUnitState( 0 );
        texUnit->setTexture( m_heightMapTex );

        // Normalize vScale for better precision in the shader math
        const Vector3 vScale =
            Vector3( m_xzRelativeSize.x, m_heightUnormScaled, m_xzRelativeSize.y ).normalisedCopy();

        GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();
        psParams->setNamedConstant(
            "heightMapResolution",
            Vector4( static_cast<Real>( m_width ), static_cast<Real>( m_depth ), 1, 1 ) );
        psParams->setNamedConstant( "vScale", vScale );

        CompositorChannelVec finalTargetChannels( 1, CompositorChannel() );
        finalTargetChannels[0] = tmpRtt;

        Camera *dummyCamera = mManager->createCamera( "TerraDummyCamera" );

        const IdString workspaceName = m_heightMapTex->getPixelFormat() == PFG_R16_UINT
                                           ? "Terra/GpuNormalMapperWorkspaceU16"
                                           : "Terra/GpuNormalMapperWorkspace";
        CompositorWorkspace *workspace = m_compositorManager->addWorkspace(
            mManager, finalTargetChannels, dummyCamera, workspaceName, false );
        workspace->_beginUpdate( true );
        workspace->_update();
        workspace->_endUpdate( true );

        m_compositorManager->removeWorkspace( workspace );
        mManager->destroyCamera( dummyCamera );

        for( uint8 i = 0u; i < m_normalMapTex->getNumMipmaps(); ++i )
        {
            tmpRtt->copyTo( m_normalMapTex, m_normalMapTex->getEmptyBox( i ), i,
                            tmpRtt->getEmptyBox( i ), i );
        }

        TerraSharedResources::destroyTempTexture( m_sharedResources, tmpRtt );
    }
    //-----------------------------------------------------------------------------------
    void Terra::destroyNormalTexture()
    {
        if( m_normalMapTex )
        {
            TextureGpuManager *textureManager =
                mManager->getDestinationRenderSystem()->getTextureGpuManager();
            textureManager->destroyTexture( m_normalMapTex );
            m_normalMapTex = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void Terra::calculateOptimumSkirtSize()
    {
        m_skirtSize = std::numeric_limits<float>::max();

        const uint32 basePixelDimension = m_basePixelDimension;
        const uint32 vertPixelDimension =
            static_cast<uint32>( float( m_basePixelDimension ) * m_depthWidthRatio );

        for( size_t y = vertPixelDimension - 1u; y < m_depth - 1u; y += vertPixelDimension )
        {
            const size_t ny = y + 1u;

            bool allEqualInLine = true;
            float minHeight = m_heightMap[y * m_width];
            for( size_t x = 0; x < m_width; ++x )
            {
                const float minValue =
                    std::min( m_heightMap[y * m_width + x], m_heightMap[ny * m_width + x] );
                minHeight = std::min( minValue, minHeight );
                allEqualInLine &= m_heightMap[y * m_width + x] == m_heightMap[ny * m_width + x];
            }

            if( !allEqualInLine )
                m_skirtSize = std::min( minHeight, m_skirtSize );
        }

        for( size_t x = basePixelDimension - 1u; x < m_width - 1u; x += basePixelDimension )
        {
            const size_t nx = x + 1u;

            bool allEqualInLine = true;
            float minHeight = m_heightMap[x];
            for( size_t y = 0; y < m_depth; ++y )
            {
                const float minValue =
                    std::min( m_heightMap[y * m_width + x], m_heightMap[y * m_width + nx] );
                minHeight = std::min( minValue, minHeight );
                allEqualInLine &= m_heightMap[y * m_width + x] == m_heightMap[y * m_width + nx];
            }

            if( !allEqualInLine )
                m_skirtSize = std::min( minHeight, m_skirtSize );
        }

        m_skirtSize /= m_height;

        // Many Android GPUs don't support PFG_R16_UNORM so we scale it by hand
        if( m_heightMapTex->getPixelFormat() == PFG_R16_UINT )
            m_skirtSize *= 65535.0f;
    }
    //-----------------------------------------------------------------------------------
    void Terra::createTerrainCells()
    {
        {
            // Find out how many TerrainCells we need. I think this might be
            // solved analitically with a power series. But my math is rusty.
            const uint32 basePixelDimension = m_basePixelDimension;
            const uint32 vertPixelDimension =
                static_cast<uint32>( m_basePixelDimension * m_depthWidthRatio );
            const uint32 maxPixelDimension = std::max( basePixelDimension, vertPixelDimension );
            const uint32 maxRes = std::max( m_width, m_depth );

            uint32 numCells = 16u;  // 4x4
            uint32 accumDim = 0u;
            uint32 iteration = 1u;
            while( accumDim < maxRes )
            {
                numCells += 12u;  // 4x4 - 2x2
                accumDim += maxPixelDimension * ( 1u << iteration );
                ++iteration;
            }

            numCells += 12u;
            accumDim += maxPixelDimension * ( 1u << iteration );
            ++iteration;

            for( size_t i = 0u; i < 2u; ++i )
            {
                m_terrainCells[i].clear();
                m_terrainCells[i].resize( numCells, TerrainCell( this ) );
            }
        }

        VaoManager *vaoManager = mManager->getDestinationRenderSystem()->getVaoManager();

        for( size_t i = 0u; i < 2u; ++i )
        {
            std::vector<TerrainCell>::iterator itor = m_terrainCells[i].begin();
            std::vector<TerrainCell>::iterator endt = m_terrainCells[i].end();

            const std::vector<TerrainCell>::iterator begin = itor;

            while( itor != endt )
            {
                itor->initialize( vaoManager, ( itor - begin ) >= 16u );
                ++itor;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    inline GridPoint Terra::worldToGrid( const Vector3 &vPos ) const
    {
        GridPoint retVal;
        const float fWidth = static_cast<float>( m_width );
        const float fDepth = static_cast<float>( m_depth );

        const float fX = floorf( ( ( vPos.x - m_terrainOrigin.x ) * m_xzInvDimensions.x ) * fWidth );
        const float fZ = floorf( ( ( vPos.z - m_terrainOrigin.z ) * m_xzInvDimensions.y ) * fDepth );
        retVal.x = static_cast<int32>( fX );
        retVal.z = static_cast<int32>( fZ );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    inline Vector2 Terra::gridToWorld( const GridPoint &gPos ) const
    {
        Vector2 retVal;
        const float fWidth = static_cast<float>( m_width );
        const float fDepth = static_cast<float>( m_depth );

        retVal.x = ( float( gPos.x ) / fWidth ) * m_xzDimensions.x + m_terrainOrigin.x;
        retVal.y = ( float( gPos.z ) / fDepth ) * m_xzDimensions.y + m_terrainOrigin.z;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    bool Terra::isVisible( const GridPoint &gPos, const GridPoint &gSize ) const
    {
        if( gPos.x >= static_cast<int32>( m_width ) || gPos.z >= static_cast<int32>( m_depth ) ||
            gPos.x + gSize.x <= 0 || gPos.z + gSize.z <= 0 )
        {
            // Outside terrain bounds.
            return false;
        }

        //        return true;

        const Vector2 cellPos = gridToWorld( gPos );
        const Vector2 cellSize( Real( gSize.x + 1 ) * m_xzRelativeSize.x,
                                Real( gSize.z + 1 ) * m_xzRelativeSize.y );

        const Vector3 vHalfSizeYUp = Vector3( cellSize.x, m_height, cellSize.y ) * 0.5f;
        const Vector3 vCenter =
            fromYUp( Vector3( cellPos.x, m_terrainOrigin.y, cellPos.y ) + vHalfSizeYUp );
        const Vector3 vHalfSize = fromYUpSignPreserving( vHalfSizeYUp );

        for( unsigned i = 0; i < 6u; ++i )
        {
            // Skip far plane if view frustum is infinite
            if( i == FRUSTUM_PLANE_FAR && m_camera->getFarClipDistance() == 0 )
                continue;

            Plane::Side side = m_camera->getFrustumPlane( (uint16)i ).getSide( vCenter, vHalfSize );

            // We only need one negative match to know the obj is outside the frustum
            if( side == Plane::NEGATIVE_SIDE )
                return false;
        }

        return true;
    }
    //-----------------------------------------------------------------------------------
    void Terra::addRenderable( const GridPoint &gridPos, const GridPoint &cellSize, uint32 lodLevel )
    {
        TerrainCell *cell = &m_terrainCells[0][m_currentCell++];
        cell->setOrigin( gridPos,  //
                         static_cast<uint32>( cellSize.x ), static_cast<uint32>( cellSize.z ),
                         lodLevel );
        m_collectedCells[0].push_back( cell );
    }
    //-----------------------------------------------------------------------------------
    void Terra::optimizeCellsAndAdd()
    {
        // Keep iterating until m_collectedCells[0] stops shrinking
        size_t numCollectedCells = std::numeric_limits<size_t>::max();
        while( numCollectedCells != m_collectedCells[0].size() )
        {
            numCollectedCells = m_collectedCells[0].size();

            if( m_collectedCells[0].size() > 1 )
            {
                m_collectedCells[1].clear();

                std::vector<TerrainCell *>::const_iterator itor = m_collectedCells[0].begin();
                std::vector<TerrainCell *>::const_iterator end = m_collectedCells[0].end();

                while( end - itor >= 2u )
                {
                    TerrainCell *currCell = *itor;
                    TerrainCell *nextCell = *( itor + 1 );

                    m_collectedCells[1].push_back( currCell );
                    if( currCell->merge( nextCell ) )
                        itor += 2;
                    else
                        ++itor;
                }

                while( itor != end )
                    m_collectedCells[1].push_back( *itor++ );

                m_collectedCells[1].swap( m_collectedCells[0] );
            }
        }

        std::vector<TerrainCell *>::const_iterator itor = m_collectedCells[0].begin();
        std::vector<TerrainCell *>::const_iterator end = m_collectedCells[0].end();
        while( itor != end )
            mRenderables.push_back( *itor++ );

        m_collectedCells[0].clear();
    }
    //-----------------------------------------------------------------------------------
    void Terra::setSharedResources( TerraSharedResources *sharedResources )
    {
        m_sharedResources = sharedResources;
        if( m_shadowMapper )
            m_shadowMapper->_setSharedResources( sharedResources );
    }
    //-----------------------------------------------------------------------------------
    void Terra::update( const Vector3 &lightDir, float lightEpsilon )
    {
        const float lightCosAngleChange =
            Math::Clamp( (float)m_prevLightDir.dotProduct( lightDir.normalisedCopy() ), -1.0f, 1.0f );
        if( lightCosAngleChange <= ( 1.0f - lightEpsilon ) )
        {
            m_shadowMapper->updateShadowMap( toYUp( lightDir ), m_xzDimensions, m_height );
            m_prevLightDir = lightDir.normalisedCopy();
        }
        // m_shadowMapper->updateShadowMap( Vector3::UNIT_X, m_xzDimensions, m_height );
        // m_shadowMapper->updateShadowMap( Vector3(2048,0,1024), m_xzDimensions, m_height );
        // m_shadowMapper->updateShadowMap( Vector3(1,0,0.1), m_xzDimensions, m_height );
        // m_shadowMapper->updateShadowMap( Vector3::UNIT_Y, m_xzDimensions, m_height ); //Check! Does
        // NAN

        mRenderables.clear();
        m_currentCell = 0;

        const Vector3 camPos = toYUp( m_camera->getDerivedPosition() );

        const int32 basePixelDimension = static_cast<int32>( m_basePixelDimension );
        const int32 vertPixelDimension =
            static_cast<int32>( float( m_basePixelDimension ) * m_depthWidthRatio );

        GridPoint cellSize;
        cellSize.x = basePixelDimension;
        cellSize.z = vertPixelDimension;

        // Quantize the camera position to basePixelDimension steps
        GridPoint camCenter = worldToGrid( camPos );
        camCenter.x = ( camCenter.x / basePixelDimension ) * basePixelDimension;
        camCenter.z = ( camCenter.z / vertPixelDimension ) * vertPixelDimension;

        uint32 currentLod = 0;

        //        camCenter.x = 64;
        //        camCenter.z = 64;

        // LOD 0: Add full 4x4 grid
        for( int32 z = -2; z < 2; ++z )
        {
            for( int32 x = -2; x < 2; ++x )
            {
                GridPoint pos = camCenter;
                pos.x += x * cellSize.x;
                pos.z += z * cellSize.z;

                if( isVisible( pos, cellSize ) )
                    addRenderable( pos, cellSize, currentLod );
            }
        }

        optimizeCellsAndAdd();

        m_currentCell = 16u;  // The first 16 cells don't use skirts.

        const uint32 maxRes = std::max( m_width, m_depth );
        // TODO: When we're too far (outside the terrain), just display a 4x4 grid or something like
        // that.

        size_t numObjectsAdded = std::numeric_limits<size_t>::max();
        // LOD n: Add 4x4 grid, ignore 2x2 center (which
        // is the same as saying the borders of the grid)
        while( numObjectsAdded != m_currentCell ||
               ( mRenderables.empty() && ( 1u << currentLod ) <= maxRes ) )
        {
            numObjectsAdded = m_currentCell;

            cellSize.x <<= 1u;
            cellSize.z <<= 1u;
            ++currentLod;

            // Row 0
            {
                const int32 z = 1;
                for( int32 x = -2; x < 2; ++x )
                {
                    GridPoint pos = camCenter;
                    pos.x += x * cellSize.x;
                    pos.z += z * cellSize.z;

                    if( isVisible( pos, cellSize ) )
                        addRenderable( pos, cellSize, currentLod );
                }
            }
            // Row 3
            {
                const int32 z = -2;
                for( int32 x = -2; x < 2; ++x )
                {
                    GridPoint pos = camCenter;
                    pos.x += x * cellSize.x;
                    pos.z += z * cellSize.z;

                    if( isVisible( pos, cellSize ) )
                        addRenderable( pos, cellSize, currentLod );
                }
            }
            // Cells [0, 1] & [0, 2];
            {
                const int32 x = -2;
                for( int32 z = -1; z < 1; ++z )
                {
                    GridPoint pos = camCenter;
                    pos.x += x * cellSize.x;
                    pos.z += z * cellSize.z;

                    if( isVisible( pos, cellSize ) )
                        addRenderable( pos, cellSize, currentLod );
                }
            }
            // Cells [3, 1] & [3, 2];
            {
                const int32 x = 1;
                for( int32 z = -1; z < 1; ++z )
                {
                    GridPoint pos = camCenter;
                    pos.x += x * cellSize.x;
                    pos.z += z * cellSize.z;

                    if( isVisible( pos, cellSize ) )
                        addRenderable( pos, cellSize, currentLod );
                }
            }

            optimizeCellsAndAdd();
        }
    }
    //-----------------------------------------------------------------------------------
    void Terra::load( const String &texName, const Vector3 &center, const Vector3 &dimensions,
                      bool bMinimizeMemoryConsumption, bool bLowResShadow )
    {
        Ogre::Image2 image;
        image.load( texName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

        load( image, center, dimensions, bMinimizeMemoryConsumption, bLowResShadow, texName );
    }
    //-----------------------------------------------------------------------------------
    void Terra::load( Image2 &image, Vector3 center, Vector3 dimensions, bool bMinimizeMemoryConsumption,
                      bool bLowResShadow, const String &imageName )
    {
        // Use sign-preserving because origin in XZ plane is always from
        // bottom-left to top-right.
        // If we use toYUp, we'll start from top-right and go up and right
        m_terrainOrigin = toYUpSignPreserving( center - dimensions * 0.5f );
        center = toYUp( center );
        dimensions = toYUpSignPreserving( dimensions );
        m_xzDimensions = Vector2( dimensions.x, dimensions.z );
        m_xzInvDimensions = 1.0f / m_xzDimensions;
        m_height = dimensions.y;
        m_basePixelDimension = 64u;
        createHeightmap( image, imageName, bMinimizeMemoryConsumption, bLowResShadow );
        createTerrainCells();
    }
    //-----------------------------------------------------------------------------------
    void Terra::setBasePixelDimension( uint32 basePixelDimension )
    {
        m_basePixelDimension = basePixelDimension;
        if( !m_heightMap.empty() && !m_terrainCells->empty() )
        {
            HlmsDatablock *datablock = m_terrainCells[0].back().getDatablock();

            calculateOptimumSkirtSize();
            createTerrainCells();

            if( datablock )
            {
                for( size_t i = 0u; i < 2u; ++i )
                {
                    for( TerrainCell &terrainCell : m_terrainCells[i] )
                        terrainCell.setDatablock( datablock );
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    bool Terra::getHeightAt( Vector3 &vPosArg ) const
    {
        bool retVal = false;

        Vector3 vPos = toYUp( vPosArg );

        GridPoint pos2D = worldToGrid( vPos );

        const int32 iWidth = static_cast<int32>( m_width );
        const int32 iDepth = static_cast<int32>( m_depth );

        if( pos2D.x >= 0 && pos2D.x < iWidth - 1 && pos2D.z >= 0 && pos2D.z < iDepth - 1 )
        {
            const Vector2 vPos2D = gridToWorld( pos2D );

            const float dx = ( vPos.x - vPos2D.x ) * float( m_width ) * m_xzInvDimensions.x;
            const float dz = ( vPos.z - vPos2D.y ) * float( m_depth ) * m_xzInvDimensions.y;

            float a, b, c;
            const float h00 = m_heightMap[size_t( pos2D.z * iWidth + pos2D.x )];
            const float h11 = m_heightMap[size_t( ( pos2D.z + 1 ) * iWidth + pos2D.x + 1 )];

            c = h00;
            if( dx < dz )
            {
                // Plane eq: y = ax + bz + c
                // x=0 z=0 -> c		= h00
                // x=0 z=1 -> b + c	= h01 -> b = h01 - c
                // x=1 z=1 -> a + b + c  = h11 -> a = h11 - b - c
                const float h01 = m_heightMap[size_t( ( pos2D.z + 1 ) * iWidth + pos2D.x )];

                b = h01 - c;
                a = h11 - b - c;
            }
            else
            {
                // Plane eq: y = ax + bz + c
                // x=0 z=0 -> c		= h00
                // x=1 z=0 -> a + c	= h10 -> a = h10 - c
                // x=1 z=1 -> a + b + c  = h11 -> b = h11 - a - c
                const float h10 = m_heightMap[size_t( pos2D.z * iWidth + pos2D.x + 1 )];

                a = h10 - c;
                b = h11 - a - c;
            }

            vPos.y = a * dx + b * dz + c + m_terrainOrigin.y;
            retVal = true;
        }

        vPosArg = fromYUp( vPos );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void Terra::setDatablock( HlmsDatablock *datablock )
    {
        if( !datablock && !m_terrainCells[0].empty() && m_terrainCells[0].back().getDatablock() )
        {
            // Unsetting the datablock. We have no way of unlinking later on. Do it now
            HlmsDatablock *oldDatablock = m_terrainCells[0].back().getDatablock();
            OGRE_ASSERT_HIGH( dynamic_cast<HlmsTerra *>( oldDatablock->getCreator() ) );
            HlmsTerra *hlms = static_cast<HlmsTerra *>( oldDatablock->getCreator() );
            hlms->_unlinkTerra( this );
        }

        for( size_t i = 0u; i < 2u; ++i )
        {
            std::vector<TerrainCell>::iterator itor = m_terrainCells[i].begin();
            std::vector<TerrainCell>::iterator end = m_terrainCells[i].end();

            while( itor != end )
            {
                itor->setDatablock( datablock );
                ++itor;
            }
        }

        if( mHlmsTerraIndex == std::numeric_limits<uint32>::max() )
        {
            OGRE_ASSERT_HIGH( dynamic_cast<HlmsTerra *>( datablock->getCreator() ) );
            HlmsTerra *hlms = static_cast<HlmsTerra *>( datablock->getCreator() );
            hlms->_linkTerra( this );
        }
    }
    //-----------------------------------------------------------------------------------
    Ogre::TextureGpu *Terra::_getShadowMapTex() const { return m_shadowMapper->getShadowMapTex(); }
    //-----------------------------------------------------------------------------------
    Vector3 Terra::getTerrainOrigin() const { return fromYUpSignPreserving( m_terrainOrigin ); }
    //-----------------------------------------------------------------------------------
    Vector2 Terra::getTerrainXZCenter() const
    {
        return Vector2( m_terrainOrigin.x + m_xzDimensions.x * 0.5f,
                        m_terrainOrigin.z + m_xzDimensions.y * 0.5f );
    }
    //-----------------------------------------------------------------------------------
    const String &Terra::getMovableType() const
    {
        static const String movType = "Terra";
        return movType;
    }
    //-----------------------------------------------------------------------------------
    void Terra::_swapSavedState()
    {
        m_terrainCells[0].swap( m_terrainCells[1] );
        m_savedState.m_renderables.swap( mRenderables );
        std::swap( m_savedState.m_currentCell, m_currentCell );
        std::swap( m_savedState.m_camera, m_camera );
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    TerraSharedResources::TerraSharedResources() { memset( textures, 0, sizeof( textures ) ); }
    //-----------------------------------------------------------------------------------
    TerraSharedResources::~TerraSharedResources() { freeAllMemory(); }
    //-----------------------------------------------------------------------------------
    void TerraSharedResources::freeAllMemory()
    {
        freeStaticMemory();

        for( size_t i = NumStaticTmpTextures; i < NumTemporaryUsages; ++i )
        {
            if( textures[i] )
            {
                TextureGpuManager *textureManager = textures[i]->getTextureManager();
                textureManager->destroyTexture( textures[i] );
                textures[i] = 0;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void TerraSharedResources::freeStaticMemory()
    {
        for( size_t i = 0u; i < NumStaticTmpTextures; ++i )
        {
            if( textures[i] )
            {
                TextureGpuManager *textureManager = textures[i]->getTextureManager();
                textureManager->destroyTexture( textures[i] );
                textures[i] = 0;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *TerraSharedResources::getTempTexture( const char *texName, IdType id,
                                                      TerraSharedResources *sharedResources,
                                                      TemporaryUsages temporaryUsage,
                                                      TextureGpu *baseTemplate, uint32 flags )
    {
        TextureGpu *tmpRtt = 0;

        if( sharedResources && sharedResources->textures[temporaryUsage] )
            tmpRtt = sharedResources->textures[temporaryUsage];
        else
        {
            TextureGpuManager *textureManager = baseTemplate->getTextureManager();
            tmpRtt = textureManager->createTexture( texName + StringConverter::toString( id ),
                                                    GpuPageOutStrategy::Discard, flags,
                                                    TextureTypes::Type2D );
            tmpRtt->copyParametersFrom( baseTemplate );
            tmpRtt->scheduleTransitionTo( GpuResidency::Resident );
            if( flags & TextureFlags::RenderToTexture )
                tmpRtt->_setDepthBufferDefaults( DepthBuffer::POOL_NO_DEPTH, false, PFG_UNKNOWN );

            if( sharedResources )
                sharedResources->textures[temporaryUsage] = tmpRtt;
        }
        return tmpRtt;
    }
    //-----------------------------------------------------------------------------------
    void TerraSharedResources::destroyTempTexture( TerraSharedResources *sharedResources,
                                                   TextureGpu *tmpRtt )
    {
        if( !sharedResources )
        {
            TextureGpuManager *textureManager = tmpRtt->getTextureManager();
            textureManager->destroyTexture( tmpRtt );
        }
    }
}  // namespace Ogre
