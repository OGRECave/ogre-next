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
#ifndef _OgreVctVoxelizerSourceBase_H_
#define _OgreVctVoxelizerSourceBase_H_

#include "OgreHlmsPbsPrerequisites.h"

#include "Math/Simple/OgreAabb.h"
#include "OgreId.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class VoxelVisualizer;

    /**
    @class VctVoxelizerSourceBase
        This class contains shared functionality between voxelizer; and
        is used by VctLighting to source its voxel data to generate GI
    */
    class _OgreHlmsPbsExport VctVoxelizerSourceBase : public IdObject
    {
    public:
        enum DebugVisualizationMode
        {
            DebugVisualizationAlbedo,
            DebugVisualizationNormal,
            DebugVisualizationEmissive,
            DebugVisualizationNone
        };

    protected:
        TextureGpu *mAlbedoVox;
        TextureGpu *mEmissiveVox;
        TextureGpu *mNormalVox;
        TextureGpu *mAccumValVox;

        RenderSystem      *mRenderSystem;
        VaoManager        *mVaoManager;
        HlmsManager       *mHlmsManager;
        TextureGpuManager *mTextureGpuManager;

        DebugVisualizationMode mDebugVisualizationMode;
        VoxelVisualizer       *mDebugVoxelVisualizer;

        uint32 mWidth;
        uint32 mHeight;
        uint32 mDepth;

        Aabb mRegionToVoxelize;

        virtual void destroyVoxelTextures();
        void         setTextureToDebugVisualizer();

    public:
        VctVoxelizerSourceBase( IdType id, RenderSystem *renderSystem, HlmsManager *hlmsManager );
        virtual ~VctVoxelizerSourceBase();

        void setDebugVisualization( VctVoxelizerSourceBase::DebugVisualizationMode mode,
                                    SceneManager                                  *sceneManager );
        VctVoxelizerSourceBase::DebugVisualizationMode getDebugVisualizationMode() const;

        Vector3 getVoxelOrigin() const;
        Vector3 getVoxelCellSize() const;
        Vector3 getVoxelSize() const;
        Vector3 getVoxelResolution() const;

        TextureGpu *getAlbedoVox() { return mAlbedoVox; }
        TextureGpu *getNormalVox() { return mNormalVox; }
        TextureGpu *getEmissiveVox() { return mEmissiveVox; }

        TextureGpuManager *getTextureGpuManager();
        RenderSystem      *getRenderSystem();
        HlmsManager       *getHlmsManager();
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
