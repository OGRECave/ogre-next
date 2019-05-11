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
#ifndef _OgreVctLighting_H_
#define _OgreVctLighting_H_

#include "OgreHlmsPbsPrerequisites.h"
#include "OgreId.h"
#include "OgreShaderParams.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class VctVoxelizer;
    struct ShaderVctLight;

    class _OgreHlmsPbsExport VctLighting : public IdObject
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
        TextureGpu              *mLightVoxel[4];
        HlmsSamplerblock const  *mSamplerblockTrilinear;
        VctVoxelizer    *mVoxelizer;

        HlmsComputeJob      *mLightInjectionJob;
        ConstBufferPacked   *mLightsConstBuffer;

        /// Anisotropic mipmap generation consists of 2 main steps:
        ///
        /// Step 1 takes mLightVoxel[0] and computes
        /// mLightVoxel[1].mip[0], mLightVoxel[2].mip[0] & mLightVoxel[3].mip[0]
        ///
        /// Step 2 takes mLightVoxel[i].mip[n] and computes mLightVoxel[i].mip[n+1]
        /// where i is in range [1; 3] and n is the number of mipmaps in those textures.
        HlmsComputeJob              *mAnisoGeneratorStep0;
        FastArray<HlmsComputeJob*>  mAnisoGeneratorStep1;

        float mDefaultLightDistThreshold;
        bool    mAnisotropic;

        ShaderParams::Param *mNumLights;
        ShaderParams::Param *mRayMarchStepSize;
        ShaderParams::Param *mVoxelCellSize;
        ShaderParams::Param *mInvVoxelResolution;
        ShaderParams        *mShaderParams;

    public:
        /** Bias/distance in units (i.e. if your engine is in meters, then this value
            is in meters) along the geometric normal at which to start cone tracing.

            If this value is 0, then VCT results may resemble direct lighting rather
            than indirect lighting due to self-illumination.

            @remark	 PUBLIC VARIABLE. This variable can be altered directly.
                     Changes are reflected immediately.
        */
        float   mNormalBias;

    protected:
        void addLight( ShaderVctLight * RESTRICT_ALIAS vctLight, Light *light,
                       const Vector3 &voxelOrigin, const Vector3 &invVoxelRes );

        void createTextures();
        void destroyTextures();

        void generateAnisotropicMips(void);

    public:
        VctLighting( IdType id, VctVoxelizer *voxelizer );
        ~VctLighting();

        /**
        @param sceneManager
        @param rayMarchStepScale
            Scale for the ray march step size. A value < 1.0f makes little sense
            and will trigger an assert.

            Bigger values means the shadow raymarching during light injection
            pass is faster, but may cause glitches if too high (areas that
            are supposed to be shadowed won't be shadowed)
        @param lightMask
        */
        void update( SceneManager *sceneManager, float rayMarchStepScale=1.0f,
                     uint32 lightMask=0xffffffff );

        size_t getConstBufferSize(void) const;

        void fillConstBufferData( const Matrix4 &viewMatrix,
                                  float * RESTRICT_ALIAS passBufferPtr ) const;

        TextureGpu** getLightVoxelTextures(void)            { return mLightVoxel; }
        const HlmsSamplerblock* getBindTrilinearSamplerblock(void)
                                                            { return mSamplerblockTrilinear; }
    };
}

#include "OgreHeaderSuffix.h"

#endif
