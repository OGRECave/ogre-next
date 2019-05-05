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
        TextureGpu              *mLightVoxel[6];
        HlmsSamplerblock const  *mSamplerblockTrilinear;
        VctVoxelizer    *mVoxelizer;

        HlmsComputeJob      *mLightInjectionJob;
        ConstBufferPacked   *mLightsConstBuffer;

        float mDefaultLightDistThreshold;

        ShaderParams::Param *mNumLights;
        ShaderParams::Param *mRayMarchStepSize;
        ShaderParams::Param *mVoxelCellSize;
        ShaderParams::Param *mInvVoxelResolution;
        ShaderParams        *mShaderParams;

        void addLight( ShaderVctLight * RESTRICT_ALIAS vctLight, Light *light,
                       const Vector3 &voxelOrigin, const Vector3 &invVoxelRes );

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
