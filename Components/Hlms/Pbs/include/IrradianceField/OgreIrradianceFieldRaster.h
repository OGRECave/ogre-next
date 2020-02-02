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

#ifndef _OgreIrradianceFieldRaster_H_
#define _OgreIrradianceFieldRaster_H_

#include "OgreHlmsPbsPrerequisites.h"

#include "Compositor/OgreCompositorWorkspaceListener.h"
#include "OgreVector3.h"
#include "OgrePixelFormatGpu.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /**
    @class IrradianceFieldRaster
        Implements IrradianceField, but generating results via cubemap rasterization

        GPUs are normally slow for rasterization at row resolution thus not well suited
        (hence raytracing or voxelization are better suited) but currently voxelization
        either requires a lot of memory or a slow pre-build process, hence
        a raster-based approach is less invasive and in many cases more practical.

    @remarks
        Q: Why can't we reuse the cubemaps from PCC? (Parallax Corrected Cubemaps)

        A: Because:

            1. We require to convert the depth buffer of each side into a cubemap.
               We could reuse the depth compression though. It's not accurate information
               but it may be good enough
            2. Resolution needs to be low (e.g. 16x16 or so). Workaroundeable if
               we sample higher mips.
            3. PCC provides no guarantees the cubemaps are aligned in a perfect 3D grid.
               PccPerPixelGridPlacement generates the cubemaps in a grid alignment, but
               the camera are not guaranteed to be at the center of each probe

        In short, it may be possible, but there are several issues to workaround,
        hence it is just easier to render the information we need
    */
    class _OgreHlmsPbsExport IrradianceFieldRaster : public CompositorWorkspaceListener
    {
        IrradianceField *mCreator;

        TextureGpu *mCubemap;
        TextureGpu *mDepthCubemap;
        CompositorWorkspace *mRenderWorkspace;
        CompositorWorkspace *mConvertToIfdWorkspace;

        IdString mWorkspaceName;
        PixelFormatGpu mPixelFormat;
        float mCameraNear;
        float mCameraFar;

        Vector3 mFieldOrigin;
        Vector3 mFieldSize;

        Camera *mCamera;
        Pass *mDepthBufferToCubemapPass;

        void createWorkspace( void );
        void destroyWorkspace( void );

        Vector3 getProbeCenter( size_t probeIdx ) const;
        void renderProbes( uint32 probesPerFrame );

    public:
        IrradianceFieldRaster( IrradianceField *creator );
        virtual ~IrradianceFieldRaster();

        /// Needed
        virtual void passPreExecute( CompositorPass *pass );
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
