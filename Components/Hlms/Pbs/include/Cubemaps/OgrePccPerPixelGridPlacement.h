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

#ifndef _OgrePccPerPixelGridPlacement_H_
#define _OgrePccPerPixelGridPlacement_H_

#include "OgreHlmsPbsPrerequisites.h"

#include "OgrePixelFormatGpu.h"

#include "Math/Simple/OgreAabb.h"

namespace Ogre
{
    class CompositorWorkspaceDef;

    /**
    @class PccPerPixelGridPlacement
        Placing multiple PCC probes by hand can be difficult and even error prone.

        This class aims at helping developers and artists by automatically finding ideal probe
        shape sizes that adjust to underlying geometry in the scene by only providing the
        overall enclosing AABB of the scene that needs to be covered by PCC.

        Note that once probes have been rendered and their shapes set, you can safely destroy
        this PccPerPixelGridPlacement instance; and even modify or remove probes you don't like
        or that feel excessive.

    @remarks
        The ParallaxCorrectedCubemapAuto pointer is externally owned and must be provided
        via PccPerPixelGridPlacement::setParallaxCorrectedCubemapAuto, before calling build.

        The compositor from which ParallaxCorrectedCubemapAuto is based on MUST have the
        depth compressed in the alpha channel.

        See samples on how they use PccDepthCompressor and see
        Media/2.0/scripts/Compositors/LocalCubemaps.compositor

        See Samples/2.0/ApiUsage/PccPerPixelGridPlacement for usage
    */
    class _OgreHlmsPbsExport PccPerPixelGridPlacement
    {
        uint32 mNumProbes[3];
        Aabb mFullRegion;

        Vector3 mOverlap;

        ParallaxCorrectedCubemapAuto *mPcc;

        FastArray<AsyncTextureTicket *> mAsyncTicket;

        void processProbeDepth( TextureBox box, size_t probeIdx, size_t sliceIdx );

        Vector3 getProbeNormalizedCenter( size_t probeIdx ) const;

    public:
        PccPerPixelGridPlacement();
        ~PccPerPixelGridPlacement();

        void setParallaxCorrectedCubemapAuto( ParallaxCorrectedCubemapAuto *pcc );

        ParallaxCorrectedCubemapAuto *getParallaxCorrectedCubemap() { return mPcc; }

        /** Sets the number of probes in each axis.
            Thus the total number of probes will be numProbes[0] * numProbes[1] * numProbes[2]
        @param numProbes
        */
        void setNumProbes( uint32 numProbes[3] );

        /** PccPerPixelGridPlacement needs, as guidance, the maximum region it will be occupying.

            It could be a room, a house. We will then distribute the probes homegeneously
            then analyze average depth, and try to match the shape of the probes to match
            the visible geometry from the probe's perspective.
        @param fullRegion
        */
        void setFullRegion( const Aabb &fullRegion );

        /** PccPerPixelGridPlacement will subdivide in mNumProbes[i] probes along each axis,
            creating a 3D grid.

            Overlap is disabled with a value of 1.0

            Without overlap, there will be harsh outlines at the borders between the probes.
            Adding some overlap allows a smooth blending between them.

            Higher overlap results in smoother blending, but also makes each probe's area coverage
            bigger, which reduces runtime performance.

            Vaules lower than 1.0 make the areas smaller, leaving gaps between each probe.
            This often is undesired unless it's a building with clearly separated rooms at
            homogeneous distances (which is often unnatural).
        @param overlap
            Valid range: (0; inf)
            Default: 1.5
        */
        void setOverlap( const Vector3 &overlap );

        /// Returns numProbes[0] * numProbes[1] * numProbes[2]
        uint32 getMaxNumProbes() const;

        void buildStart( uint32 resolution, Camera *camera,
                         PixelFormatGpu pixelFormat = PFG_RGBA8_UNORM_SRGB, float camNear = 0.5f,
                         float camFar = 500.0f );
        void buildEnd( void );
    };
}  // namespace Ogre

#endif
