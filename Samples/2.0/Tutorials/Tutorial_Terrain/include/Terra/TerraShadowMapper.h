
#ifndef _OgreTerraShadowMapper_H_
#define _OgreTerraShadowMapper_H_

#include "OgrePrerequisites.h"
#include "OgreMovableObject.h"
#include "OgreShaderParams.h"

#include "Terra/TerrainCell.h"

namespace Ogre
{
    typedef TextureGpu* CompositorChannel;

    struct TerraSharedResources;

    class ShadowMapper
    {
        Ogre::TextureGpu    *m_heightMapTex;

        ConstBufferPacked   *m_shadowStarts;
        ConstBufferPacked   *m_shadowPerGroupData;
        CompositorWorkspace *m_shadowWorkspace;
        TextureGpu          *m_shadowMapTex;
        TextureGpu          *m_tmpGaussianFilterTex;
        HlmsComputeJob      *m_shadowJob;
        ShaderParams::Param *m_jobParamDelta;
        ShaderParams::Param *m_jobParamXYStep;
        ShaderParams::Param *m_jobParamIsStep;
        ShaderParams::Param *m_jobParamHeightDelta;

        IdType m_terraId;
        bool m_minimizeMemoryConsumption;
        TerraSharedResources *m_sharedResources;

        //Ogre stuff
        SceneManager            *m_sceneManager;
        CompositorManager2      *m_compositorManager;

        static inline size_t getStartsPtrCount( int32 *starts, int32 *startsBase );

        /** Gets how many steps are needed in Bresenham's algorithm to reach certain height,
            given its dx / dy ratio where:
                dx = abs( x1 - x0 );
                dy = abs( y1 - y0 );
            and Bresenham is drawn in ranges [x0; x1) and [y0; y1)
        @param y
            Height to reach
        @param fStep
            (dx * 0.5f) / dy;
        @return
            Number of X iterations needed to reach the the pixel at height 'y'
            The returned value is at position (retVal; y)
            which means (retVal-1; y-1) is true unless y = 0;
        */
        static inline int32 getXStepsNeededToReachY( uint32 y, float fStep );

        /** Calculates the value of the error at position x = xIterationsToSkip from
            Bresenham's algorithm.
        @remarks
            We use this function so we can start Bresenham from '0' but resuming as
            if we wouldn't be starting from 0.
        @param xIterationsToSkip
            The X position in which we want the error.
        @param dx
            delta.x
        @param dy
            delta.y
        @return
            The error at position (xIterationsToSkip; y)
        */
        static inline float getErrorAfterXsteps( uint32 xIterationsToSkip, float dx, float dy );

        static void setGaussianFilterParams( HlmsComputeJob *job, uint8 kernelRadius,
                                             float gaussianDeviationFactor=0.5f );

        void createCompositorWorkspace();
        void destroyCompositorWorkspace();

    public:
        ShadowMapper( SceneManager *sceneManager, CompositorManager2 *compositorManager );
        ~ShadowMapper();

        /** Sets the parameter of the gaussian filter we apply to the shadow map.
        @param kernelRadius
            Kernel radius. Must be an even number.
        @param gaussianDeviationFactor
            Expressed in terms of gaussianDeviation = kernelRadius * gaussianDeviationFactor
        */
        void setGaussianFilterParams( uint8 kernelRadius, float gaussianDeviationFactor=0.5f );

        /// Don't call this function directly
        ///
        /// @see Terra::setSharedResources
        void _setSharedResources( TerraSharedResources *sharedResources );

        /**
            When false, we will keep around a temporary texture required for updating shadows
            This value is recommended if lighting direction will be changing frequently
            (e.g. once per frame, every two frames, every four frames, etc)

            When true, we will create the temporary texture to update whenever the shadows need
            to be updated, and destroy it immediately afterwards, freeing memory ASAP.
            This value is recommended if lighting direction changes rarely (e.g. during level
            load, when user manually changes time of day)

        @remarks
            This setting can be toggled at any time.

            If you have multiple Terras, even if you need this setting to be false
            you can still save a lot of memory by using Terra::setSharedResources so that
            this temporary texture is shared across all Terras.

            If this setting is true and you are using TerraSharedResources,
            you must still call TerraSharedResources::freeAllMemory to actually
            free the memory once you're over updating all Terras

        @param bMinimizeMemoryConsumption
            True to minimize memory consumption but makes updating light direction slower.
            False to increase memory consumption but being able to change lighting frequently.
        */
        void setMinimizeMemoryConsumption( bool bMinimizeMemoryConsumption );

        void createShadowMap( IdType id, TextureGpu *heightMapTex );
        void destroyShadowMap(void);
        void updateShadowMap( const Vector3 &lightDir, const Vector2 &xzDimensions, float heightScale );

        void fillUavDataForCompositorChannel( TextureGpu **outChannel ) const;

        Ogre::TextureGpu* getShadowMapTex(void) const           { return m_shadowMapTex; }
    };
}

#endif
