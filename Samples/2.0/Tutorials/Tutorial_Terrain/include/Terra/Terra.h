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

#ifndef _OgreTerra_H_
#define _OgreTerra_H_

#include "OgrePrerequisites.h"

#include "OgreMovableObject.h"
#include "OgreShaderParams.h"

#include "Terra/TerrainCell.h"

namespace Ogre
{
    struct GridPoint
    {
        int32 x;
        int32 z;
    };

    struct GridDirection
    {
        int x;
        int z;
    };

    class ShadowMapper;
    struct TerraSharedResources;

    /**
    @brief The Terra class
        Internally Terra operates in Y-up space so input and outputs may
        be converted to/from the correct spaces based on setting, unless
        explicitly stated to be always Y-up by documentation.
    */
    class Terra : public MovableObject
    {
        friend class TerrainCell;

        struct SavedState
        {
            RenderableArray m_renderables;
            size_t          m_currentCell;
            Camera const *  m_camera;
        };

        std::vector<float> m_heightMap;
        uint32             m_width;
        uint32             m_depth;  // PNG's Height
        float              m_depthWidthRatio;
        float              m_skirtSize;  // Already unorm scaled
        float              m_invWidth;
        float              m_invDepth;

        bool m_zUp;

        Vector2 m_xzDimensions;
        Vector2 m_xzInvDimensions;
        Vector2 m_xzRelativeSize;  // m_xzDimensions / [m_width, m_height]
        float   m_height;
        float   m_heightUnormScaled;  // m_height / 1 or m_height / 65535
        Vector3 m_terrainOrigin;
        uint32  m_basePixelDimension;

        /// 0 is currently in use
        /// 1 is SavedState
        std::vector<TerrainCell> m_terrainCells[2];
        /// 0 & 1 are for tmp use
        std::vector<TerrainCell *> m_collectedCells[2];
        size_t                     m_currentCell;

        Ogre::TextureGpu *m_heightMapTex;
        Ogre::TextureGpu *m_normalMapTex;

        Vector3       m_prevLightDir;
        ShadowMapper *m_shadowMapper;

        TerraSharedResources *m_sharedResources;

        /// When rendering shadows we want to override the data calculated by update
        /// but only temporarily, for later restoring it.
        SavedState m_savedState;

        // Ogre stuff
        CompositorManager2 *m_compositorManager;
        Camera const *      m_camera;

        /// Converts value from Y-up to whatever the user up vector is (see m_zUp)
        inline Vector3 fromYUp( Vector3 value ) const;
        /// Same as fromYUp, but preserves original sign. Needed when value is a scale
        inline Vector3 fromYUpSignPreserving( Vector3 value ) const;
        /// Converts value from user up vector to Y-up
        inline Vector3 toYUp( Vector3 value ) const;
        /// Same as toYUp, but preserves original sign. Needed when value is a scale
        inline Vector3 toYUpSignPreserving( Vector3 value ) const;

    public:
        uint32 mHlmsTerraIndex;

    protected:
        void destroyHeightmapTexture();

        /// Creates the Ogre texture based on the image data.
        /// Called by @see createHeightmap
        void createHeightmapTexture( const Image2 &image, const String &imageName );

        /// Calls createHeightmapTexture, loads image data to our CPU-side buffers
        void createHeightmap( Image2 &image, const String &imageName, bool bMinimizeMemoryConsumption,
                              bool bLowResShadow );

        void createNormalTexture();
        void destroyNormalTexture();

        ///	Automatically calculates the optimum skirt size (no gaps with
        /// lowest overdraw possible).
        ///	This is done by taking the heighest delta between two adjacent
        /// pixels in a 4x4 block.
        ///	This calculation may not be perfect, as the block search should
        /// get bigger for higher LODs.
        void calculateOptimumSkirtSize();

        void createTerrainCells();

        inline GridPoint worldToGrid( const Vector3 &vPos ) const;
        inline Vector2   gridToWorld( const GridPoint &gPos ) const;

        bool isVisible( const GridPoint &gPos, const GridPoint &gSize ) const;

        void addRenderable( const GridPoint &gridPos, const GridPoint &cellSize, uint32 lodLevel );

        void optimizeCellsAndAdd();

    public:
        Terra( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *sceneManager,
               uint8 renderQueueId, CompositorManager2 *compositorManager, Camera *camera, bool zUp );
        ~Terra() override;

        /// Sets shared resources for minimizing memory consumption wasted on temporary
        /// resources when you have more than one Terra.
        ///
        /// This function is only useful if you load/have multiple Terras at once.
        ///
        /// @see    TerraSharedResources
        void setSharedResources( TerraSharedResources *sharedResources );

        /// How low should the skirt be. Normally you should let this value untouched and let
        /// calculateOptimumSkirtSize do its thing for best performance/quality ratio.
        ///
        /// However if your height values are unconventional (i.e. artificial, non-natural) and you
        /// need to look the terrain from the "outside" (rather than being inside the terrain),
        /// you may have to tweak this value manually.
        ///
        /// This value should be between min height and max height of the heightmap.
        ///
        /// A value of 0.0 will give you the biggest skirt and fix all skirt-related issues.
        /// Note however, this may have a *tremendous* GPU performance impact.
        void  setCustomSkirtMinHeight( const float skirtMinHeight ) { m_skirtSize = skirtMinHeight; }
        float getCustomSkirtMinHeight() const { return m_skirtSize; }

        /** Must be called every frame so we can check the camera's position
            (passed in the constructor) and update our visible batches (and LODs)
            We also update the shadow map if the light direction changed.
        @param lightDir
            Light direction for computing the shadow map.
        @param lightEpsilon
            Epsilon to consider how different light must be from previous
            call to recompute the shadow map.
            Interesting values are in the range [0; 2], but any value is accepted.
        @par
            Large epsilons will reduce the frequency in which the light is updated,
            improving performance (e.g. only compute the shadow map when needed)
        @par
            Use an epsilon of <= 0 to force recalculation every frame. This is
            useful to prevent heterogeneity between frames (reduce stutter) if
            you intend to update the light slightly every frame.
        */
        void update( const Vector3 &lightDir, float lightEpsilon = 1e-6f );

        /**
        @brief load
        @param texName
        @param center
        @param dimensions
        @param bMinimizeMemoryConsumption
            See ShadowMapper::setMinimizeMemoryConsumption
        @param bLowResShadow
            See ShadowMapper::createShadowMap
        */
        void load( const String &texName, const Vector3 &center, const Vector3 &dimensions,
                   bool bMinimizeMemoryConsumption, bool bLowResShadow );
        void load( Image2 &image, Vector3 center, Vector3 dimensions, bool bMinimizeMemoryConsumption,
                   bool bLowResShadow, const String &imageName = BLANKSTRING );

        /** Lower values makes LOD very aggressive. Higher values less aggressive.
        @param basePixelDimension
            Must be power of 2.
        */
        void setBasePixelDimension( uint32 basePixelDimension );

        /** Gets the interpolated height at the given location.
            If outside the bounds, it leaves the height untouched.
        @param vPos
            Y-up:
                [in] XZ position, Y for default height.
                [out] Y height, or default Y (from input) if outside terrain bounds.
            Z-up
                [in] XY position, Z for default height.
                [out] Z height, or default Z (from input) if outside terrain bounds.
        @return
            True if Y (or Z for Z-up) component was changed
        */
        bool getHeightAt( Vector3 &vPos ) const;

        /// load must already have been called.
        void setDatablock( HlmsDatablock *datablock );

        // MovableObject overloads
        const String &getMovableType() const override;

        /// Swaps current state with a saved one. Useful for rendering shadow maps
        void _swapSavedState();

        const Camera *getCamera() const { return m_camera; }
        void          setCamera( const Camera *camera ) { m_camera = camera; }

        bool isZUp() const { return m_zUp; }

        const ShadowMapper *getShadowMapper() const { return m_shadowMapper; }

        Ogre::TextureGpu *getHeightMapTex() const { return m_heightMapTex; }
        Ogre::TextureGpu *getNormalMapTex() const { return m_normalMapTex; }
        TextureGpu *      _getShadowMapTex() const;

        // These are always in Y-up space
        const Vector2 &getXZDimensions() const { return m_xzDimensions; }
        const Vector2 &getXZInvDimensions() const { return m_xzInvDimensions; }
        float          getHeight() const { return m_height; }
        const Vector3 &getTerrainOriginRaw() const { return m_terrainOrigin; }

        /// Return value is in client-space (i.e. could be y- or z-up)
        Vector3 getTerrainOrigin() const;

        // Always in Y-up space
        Vector2 getTerrainXZCenter() const;
    };

    /** Terra during creation requires a number of temporary surfaces that are used then discarded.
        These resources can be shared by all the Terra 'islands' since as long as they have the
        same resolution and format.

        Originally, we were creating them and destroying them immediately. But because
        GPUs are async and drivers don't discard resources immediately, it was causing
        out of memory conditions.

        Once all N Terras are constructed, memory should be freed.

        Usage:

        @code
            // At level load:
            TerraSharedResources *sharedResources = new TerraSharedResources();
            for( i < numTerras )
            {
                terra[i]->setSharedResources( sharedResources );
                terra[i]->load( "Heightmap.png" );
            }

            // Free memory that is only used during Terra::load()
            sharedResources->freeStaticMemory();

            // Every frame
            for( i < numTerras )
                terra[i]->update( lightDir );

            // On shutdown:
            for( i < numTerras )
                delete terra[i];
            delete sharedResources;
        @endcode
    */
    struct TerraSharedResources
    {
        enum TemporaryUsages
        {
            TmpNormalMap,
            NumStaticTmpTextures,
            TmpShadows = NumStaticTmpTextures,
            NumTemporaryUsages
        };

        TextureGpu *textures[NumTemporaryUsages];

        TerraSharedResources();
        ~TerraSharedResources();

        /// Destroys all textures in the cache
        void freeAllMemory();

        /// Destroys all textures that are only used during heightmap load
        void freeStaticMemory();

        /**
        @brief getTempTexture
            Retrieves a cached texture to be shared with all Terras.
            If no such texture exists, creates it.

            If sharedResources is a nullptr (i.e. no sharing) then we create a temporary
            texture that will be freed by TerraSharedResources::destroyTempTexture

            If sharedResources is not nullptr, then the texture will be freed much later on
            either by TerraSharedResources::freeStaticMemory or the destructor
        @param terra
        @param sharedResources
            Can be nullptr
        @param temporaryUsage
            Type of texture to use
        @param baseTemplate
            A TextureGpu that will be used for basis of constructing our temporary RTT
        @return
            A valid ptr
        */
        static TextureGpu *getTempTexture( const char *texName, IdType id,
                                           TerraSharedResources *sharedResources,
                                           TemporaryUsages temporaryUsage, TextureGpu *baseTemplate,
                                           uint32 flags );
        /**
        @brief destroyTempTexture
            Destroys a texture created by TerraSharedResources::getTempTexture ONLY IF sharedResources
            is nullptr
        @param sharedResources
        @param tmpRtt
        */
        static void destroyTempTexture( TerraSharedResources *sharedResources, TextureGpu *tmpRtt );
    };
}  // namespace Ogre

#endif
