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
#ifndef _OgreVctLighting_H_
#define _OgreVctLighting_H_

#include "OgreHlmsPbsPrerequisites.h"

#include "OgreId.h"
#include "OgreResourceTransition.h"
#include "OgreShaderParams.h"
#include "OgreTextureGpuListener.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class VctVoxelizerSourceBase;
    class VoxelVisualizer;
    struct ShaderVctLight;

    class _OgreHlmsPbsExport VctLighting : public IdObject, public TextureGpuListener
    {
    public:
        static const uint16 msDistanceThresholdCustomParam;

    protected:
        /// When mAnisotropic == false, mLightVoxel[0] contains all the mips.
        ///
        /// When mAnisotropic == true, mLightVoxel[0] contains mip 0.
        ///  * mLightVoxel[1] contains mipmaps in -X and +X
        ///  * mLightVoxel[2] contains mipmaps in -Y and +Y
        ///  * mLightVoxel[3] contains mipmaps in -Z and +Z
        ///
        /// The negative axis is in
        ///     [0; mLightVoxel[1]->getWidth() / 2)
        /// and the positive axis is in
        ///     [mLightVoxel[1]->getWidth() / 2; mLightVoxel[1]->getWidth())
        ///
        /// We don't put all mips in the same texture (i.e. making mLightVoxel[2]) because we
        /// would need mLightVoxel[1] to be of resolution:
        ///     mLightVoxel[1].width  = mLightVoxel[0].width
        ///     mLightVoxel[1].height = mLightVoxel[0].height / 2
        ///     mLightVoxel[1].depth  = mLightVoxel[0].depth * 1.5
        ///
        /// Since most GPUs out there only support up to 2048 resolution in any axis,
        /// we wouldn't be able to support anisotropic mips for high resolution voxels.
        /// But more importantly, we would waste 1/4th of memory (actually 1/2 of memory
        /// because GPUs like GCN round memory consumption to the next power of 2).
        TextureGpu             *mLightVoxel[4];
        HlmsSamplerblock const *mSamplerblockTrilinear;

        VctVoxelizerSourceBase *mVoxelizer;
        bool                    mVoxelizerTexturesChanged;
        bool                    mVoxelizerListenersRemoved;

        HlmsComputeJob    *mLightInjectionJob;
        ConstBufferPacked *mLightsConstBuffer;

        /// Anisotropic mipmap generation consists of 2 main steps:
        ///
        /// Step 1 takes mLightVoxel[0] and computes
        /// mLightVoxel[1].mip[0], mLightVoxel[2].mip[0] & mLightVoxel[3].mip[0]
        ///
        /// Step 2 takes mLightVoxel[i].mip[n] and computes mLightVoxel[i].mip[n+1]
        /// where i is in range [1; 3] and n is the number of mipmaps in those textures.
        HlmsComputeJob             *mAnisoGeneratorStep0;
        FastArray<HlmsComputeJob *> mAnisoGeneratorStep1;

        HlmsComputeJob *mLightVctBounceInject;
        TextureGpu     *mLightBounce;

        float mBakingMultiplier;
        float mInvBakingMultiplier;

        float mUpperHemisphere[3];
        float mLowerHemisphere[3];

        float mDefaultLightDistThreshold;
        bool  mAnisotropic;

        /// When we do multiple bounces, cascades can be used to improve accuracy
        FastArray<VctLighting *> mExtraCascades;

        ShaderParams::Param *mNumLights;
        ShaderParams::Param *mRayMarchStepSize;
        ShaderParams::Param *mVoxelCellSize;
        ShaderParams::Param *mDirCorrectionRatioThinWallCounter;
        ShaderParams::Param *mInvVoxelResolution;
        ShaderParams        *mShaderParams;

        typedef vector<ShaderParams::Param>::type ParamVec;
        ParamVec                                  mLocalBounceShaderParams;
        ShaderParams::Param                      *mBounceVoxelCellSize;
        ShaderParams::Param                      *mBounceInvVoxelResolution;
        ShaderParams::Param                      *mBounceIterationDampening;
        ShaderParams::Param                      *mBounceStartBiasInvBiasCascadeMaxLod;
        ShaderParams::Param *mBounceFromPreviousProbeToNext;  ///< Used when cascades > 1
        ShaderParams        *mBounceShaderParams;

        ResourceTransitionArray mResourceTransitions;

    public:
        /** When roughness is close to 0.02, specular cone tracing becomes path tracing.
            This is very slow. However we can greatly speed it up by skipping gaps of empty
            voxels.

            We use the alpha (opacity) component of the higher mips to approximate
            the SDF (Signed Distance Field) and thus know how much to skip. This is
            theoretically wrong, but not very wrong because the mips are very close to
            its true SDF representation thus in it works practice.

            Some of these formulas have been empirically tuned to match a good
            performance/quality ratio

            Once the roughness is higher, this formula starts hurting quality (produces
            noticeable artifacts) and thus we disable it.

            This formula has tweakable parameters to leverage performance vs quality

            Recommended range is [0; 1] where 1 is high quality and 0 is high performance
            (artifacts may appear).

            However you can go outside that range.
        @remarks
            When resolution is <= 32; we completely disable this hack as it only hurts
            performance (fetching the opacity is more expensive than skipping pixels)

            PUBLIC VARIABLE. This variable can be altered directly.
            Changes are reflected immediately.
        */
        float mSpecularSdfQuality;

        /** Sets the intensity/brightness of the GI. e.g. to make the GI 2x brighter, set it to 2.0
            To make the GI darker, set it to 0.5

            Default value is 1.0

            Valid range is (0; inf)

            @remark	 PUBLIC VARIABLE. This variable can be altered directly.
                     Changes are reflected immediately.
        */
        float mMultiplier;

    protected:
        VoxelVisualizer *mDebugVoxelVisualizer;

        ShaderParams::Param *addLocalBounceShaderParam( const char *name );

        void restoreSwappedTextures();

        float addLight( ShaderVctLight *RESTRICT_ALIAS vctLight, Light *light,
                        const Vector3 &voxelOrigin, const Vector3 &invVoxelSize );

        void createTextures();
        void destroyTextures();
        void checkTextures();
        void setupBounceTextures();
        void setupGlslTextureUnits();

        void generateAnisotropicMips();

        void runBounce( uint32 bounceIteration );

    public:
        VctLighting( IdType id, VctVoxelizerSourceBase *voxelizer, bool bAnisotropic );
        ~VctLighting() override;

        /// Used by VctCascadedVoxelizer. By having extra cascade info, we can
        /// calculate multiple bounces with extra info
        ///
        /// This function calls mExtraCascades.reserve
        void reserveExtraCascades( size_t numExtraCascades );

        /// Used by VctCascadedVoxelizer. By having extra cascade info, we can
        /// calculate multiple bounces with extra info
        void addCascade( VctLighting *cascade );

        /** This function allows VctLighting::update to pass numBounces > 0 as argument.
            Note however, that multiple bounces requires creating another RGBA32_UNORM texture
            of the same resolution as the voxel texture.

            This can cause increase memory consumption.
        @remarks
            It is valid to call setAllowMultipleBounces( false ) right after calling
            update( sceneManager, numBounces > 0 ) in order to release the extra memory.

            However remember to call setAllowMultipleBounces( true ) before calling update()
            again with numBounces > 0.
        @param bAllowMultipleBounces
            True to allow multiple bounces, and consume more memory.
            False to no longer allow multiple bounces, and release memory.
        */
        void setAllowMultipleBounces( bool bAllowMultipleBounces );
        bool getAllowMultipleBounces() const;

        /** Sets baking multiplier for HDR rendering.

            Internally the lighting data is stored in RGBA8_UNORM_sRGB, which is not enough
            for HDR. More precise formats would allow for native HDR, however it's memory cost
            and bandwidth could be prohibtive.

            Hence bakingMult is used to bring down the lighting data to usable levels without
            saturation (however beware areas with very low lighting conditions may round to 0).

            During rendering, we use 1.0 / bakingMult to bring back the values its original range.

            For LDR rendering, you probably would want to set this value to 1.0.
        @remarks
            This value will be ignored if VctLighting::update is not called after setting this value
            or if VctLighting::update gets called again but autoMultiplier = true

            This function is different from VctLighting::mMultiplier.
            That variable controls the GI strength/brightness.
            This function only controls precision and accuracy.
        @param bakingMult
            Value to multiply against GI lighting during baking.
            Use values >= 1 when all your lights are too dim,
            but could saturate quickly if that's not the case

            Values <= 1 make when your lights are very bright,
            but can cause low light to become 0 (too dark)

            Changes to this value take effect after calling VctLighting::update
            and autoMultiplier must be set to false
        */
        void  setBakingMultiplier( float bakingMult );
        float getBakingMultiplier() const { return mBakingMultiplier; }

        /// If you've set setBakingMultiplier but haven't yet called VctLighting::update
        /// with autoMultiplier = false, this function returns the baking multiplier that
        /// is currently in use (beware of floating point accuracy differences)
        float getCurrentBakingMultiplier() const { return 1.0f / mInvBakingMultiplier; }

        /**
        @param sceneManager
        @param numBounces
            Number of GI bounces. This value must be 0 if getAllowMultipleBounces() == false
        @param thinWallCounter
            Shadows are calculated by raymarching towards the light source. However sometimes
            the ray 'may go through' a wall due to how bilinear interpolation works.

            Bilinear interpolation can produce nicer soft shaddows, but it can also cause
            this light leaking from behind a wall.

            Increase this value (e.g. to 2.0f) to fight light leaking.
            This should generally (over-)darken the scene

            Lower values will lighten the scene and allow more light leaking

            Note that thinWallCounter can *not* fight all sources of light leaking,
            thus increasing it to ridiculous high values may not yield any benefit.
        @param autoMultiplier
            Whether we should calculate the ideal multiplier based on lights on scene.
            See VctLighting::setMultiplier
        @param rayMarchStepScale
            Scale for the ray march step size. A value < 1.0f makes little sense
            and will trigger an assert.

            Bigger values means the shadow raymarching during light injection
            pass is faster, but may cause glitches if too high (areas that
            are supposed to be shadowed won't be shadowed)
        @param lightMask
        */
        void update( SceneManager *sceneManager, uint32 numBounces, float thinWallCounter = 1.0f,
                     bool autoMultiplier = true, float rayMarchStepScale = 1.0f,
                     uint32 lightMask = 0xffffffff );

        /// When VctImageVoxelizer::buildRelative is called; voxelizer's textures
        /// (albedo, normal, emissive) may be swapped for a copy.
        ///
        /// This function notifies us that buildRelative to update some of our references
        void resetTexturesFromBuildRelative();

        bool needsAmbientHemisphere() const;

        size_t getNumCascades() const { return mExtraCascades.size() + 1u; }

        size_t getConstBufferSize() const;

        void fillConstBufferData( const Matrix4 &viewMatrix, float *RESTRICT_ALIAS passBufferPtr ) const;

        bool shouldEnableSpecularSdfQuality() const;

        void setDebugVisualization( bool bShow, SceneManager *sceneManager );
        bool getDebugVisualizationMode() const;

        /** Toggles anisotropic mips.

            Anisotropic mips provide much higher quality and generally lower light leaking.
            However it costs a bit more performance, and increases memory consumption.

            Normally regular mipmaps of 3D textures increase memory consumption by 1/7
            Anisotropic mipmaps of 3D textures increase memory consumption by 6/7

             + Isotropic 256x256x256 RGBA8_UNORM = 256x256x256x4 * (1+1/7) = 64 MB * 1.143 =  73.14MB
             + Anisotrop 256x256x256 RGBA8_UNORM = 256x256x256x4 * (1+6/7) = 64 MB * 1.857 = 118.85MB

        @remarks
            After changing this setting, VctLighting::update
            *must* be called again to repopulate the light data.
        */
        void setAnisotropic( bool bAnisotropic );
        bool isAnisotropic() const { return mAnisotropic; }

        /** Extremely similar version of SceneManager::setAmbientLight
            In fact the hemisphereDir parameter is shared and set in SceneManager::setAmbientLight

            Setting upperHemisphere = lowerHemisphere can cause shader recompilations.

            These values are used when cone tracing reaches the end of the probe without hitting
            anything, which usually means the sky must be visible.
        @param upperHemisphere
            upperHemisphere should be set to the sky colour, which is usually set to a value *much*
            brighter than the ambient light (i.e. use the clear colour instead of the ambient colour)
        @param lowerHemisphere
            lowerHemisphere should be set to the ground colour
        */
        void setAmbient( const ColourValue &upperHemisphere, const ColourValue &lowerHemisphere );

        TextureGpu **getLightVoxelTextures() { return mLightVoxel; }
        TextureGpu **getLightVoxelTextures( const size_t cascadeIdx );
        uint32       getNumVoxelTextures() const { return mAnisotropic ? 4u : 1u; }

        const HlmsSamplerblock *getBindTrilinearSamplerblock() { return mSamplerblockTrilinear; }

        const VctVoxelizerSourceBase *getVoxelizer() const { return mVoxelizer; }

        // TextureGpuListener overloads
        void notifyTextureChanged( TextureGpu *texture, TextureGpuListener::Reason reason,
                                   void *extraData ) override;
        bool shouldStayLoaded( TextureGpu *texture ) override { return false; }
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
