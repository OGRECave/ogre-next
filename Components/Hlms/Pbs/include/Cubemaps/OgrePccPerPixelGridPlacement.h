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

#ifndef _OgrePccPerPixelGridPlacement_H_
#define _OgrePccPerPixelGridPlacement_H_

#include "OgreHlmsPbsPrerequisites.h"

#include "Cubemaps/OgreParallaxCorrectedCubemapAuto.h"

#include "Math/Simple/OgreAabb.h"
#include "OgrePixelFormatGpu.h"

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
    class _OgreHlmsPbsExport PccPerPixelGridPlacement : public ParallaxCorrectedCubemapAutoListener
    {
        uint32 mNumProbes[3];
        Aabb   mFullRegion;

        Vector3 mOverlap;
        Vector3 mSnapDeviationError;
        Vector3 mSnapSidesDeviationErrorMin;
        Vector3 mSnapSidesDeviationErrorMax;

        ParallaxCorrectedCubemapAuto *mPcc;

        /// We may need more than one in case we're using DPM instead of cubemap arrays
        FastArray<AsyncTextureTicket *> mAsyncTicket;

        /// We store everything (mip N of all probes) contiguously in mDownloadedImages
        uint8 *mDownloadedImages;

        void allocateImages();
        void deallocateImages();

        TextureBox getImagesBox() const;

        /** Snaps the new bounds of the new probe to match mFullRegion if the relative
            difference between the two is smaller than mSnapDeviationError.

            In code it performs the following:

            @code
                deviation = abs( newProbeAreaMin - mFullRegion.getMinimum() )
                relativeDev.x = deviation.x / mFullRegion.getMinimum().x;
                if( relativeDev <= mSnapDeviationError.x )
                    newProbeAreaMin.x = mFullRegion.getMinimum().x;
            @endcode

            Performs this test for both min and max and
            for each axis component (x,y,z) individually
        @param inOutNewProbeAreaMin [in/out]
        @param inOutNewProbeAreaMax [in/out]
        */
        void snapToFullRegion( Vector3 &inOutNewProbeAreaMin, Vector3 &inOutNewProbeAreaMax );

        /** Very similar to PccPerPixelGridPlacement::snapToFullRegion. However this one
            only works on the corner edges of the grid.
        @param probeIdx
        @param inOutNewProbeAreaMin
        @param inOutNewProbeAreaMax
        */
        void snapToSides( size_t probeIdx, Vector3 &inOutNewProbeAreaMin,
                          Vector3 &inOutNewProbeAreaMax );
        void processProbeDepth( TextureBox box, size_t probeIdx, size_t sliceIdx );

        Vector3 getProbeNormalizedCenter( size_t probeIdx ) const;

    public:
        PccPerPixelGridPlacement();
        ~PccPerPixelGridPlacement() override;

        void setParallaxCorrectedCubemapAuto( ParallaxCorrectedCubemapAuto *pcc );
        ParallaxCorrectedCubemapAuto *getParallaxCorrectedCubemap() { return mPcc; }

        /** Sets the number of probes in each axis.
            Thus the total number of probes will be numProbes[0] * numProbes[1] * numProbes[2]
        @param numProbes
        */
        void          setNumProbes( uint32 numProbes[3] );
        const uint32 *getNumProbes() const { return mNumProbes; }

        /** PccPerPixelGridPlacement needs, as guidance, the maximum region it will be occupying.

            It could be a room, a house. We will then distribute the probes homegeneously
            then analyze average depth, and try to match the shape of the probes to match
            the visible geometry from the probe's perspective.
        @param fullRegion
        */
        void        setFullRegion( const Aabb &fullRegion );
        const Aabb &getFullRegion() const { return mFullRegion; }

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
        void           setOverlap( const Vector3 &overlap );
        const Vector3 &getOverlap() const { return mOverlap; }

        /** When the probes have a different size than what they should have, but they're almost
            the same as mFullRegion (they can be different due to precision issues, small objects
            being on camera); it may be desirable to make them "snap" back to mFullRegion;
            particularly to avoid artifacts (missing reflections).

            If the distance between the new shape and the mFullRegion is smaller than relativeError
            (in %) then we snap the shape's bound back to mFullRegion.
            For more info, see snapToFullRegion.

            @see    PccPerPixelGridPlacement::snapToFullRegion
        @param relativeError
            Valid range: [0; inf)
            Use a very large value to always snap
            0 to disable snapping in this axis
        */
        void           setSnapDeviationError( const Vector3 &relativeError );
        const Vector3 &getSnapDeviationError() const { return mSnapDeviationError; }

        /** Very similar to PccPerPixelGridPlacement::setSnapDeviationError but more specific; thus
            allowing for much bigger error margins to fix glitches without causing major distortions

            If a wall is outside the probe's shape, it won't have reflections. It may be quite common
            that if setFullRegion() defines a rectangular room, then the produced shapes may
            end up smaller than the room; thus the walls, ceiling and floor won't have reflections.

            Thus this setting allows to defining relative errors for probes that are in the corners.

            In a 2D version, if the layout is like the following:

            @code
                                 max
                     -----------
                    | A | B | C |
                    | D | E | F |
                    | G | H | I |
                     -----------
                  min
            @endcode

            Then G will be snapped against the left corner if the distance to this corner is
            <= snapSidesDeviationErrorMin.x (in percentage)
            and A will also snapped against the bottom corner using snapSidesDeviationErrorMin.y

            While C will snapped against the right corner using snapSidesDeviationErrorMax.x
            and against the top corner using snapSidesDeviationErrorMax.y

            B will only be snapped against the top corner using snapSidesDeviationErrorMax.y

            Likewise, E won't be snapped because it's not in any corner or edge.
        @remarks
            Why shouldn't always snap?
            Consider a room with a hallway, or a non-rectangular room with column:

            @code
                // Room with a hallway:
                     -----------------------
                    | A | B | C  ___________|
                    | D | E | F |
                    | G | H | I |
                     -----------

                // Non-rectangular with a column:
                     -------
                    | A | B |___
                    | D | E | F |
                    | G | H | I |
                     -----------
            @endcode

            In these cases, we don't want C to snap, because the reflections when using C's probe
            will look unnecessarily distorted. Thus we only want to snap if the error is small
            enough, else it should be safe to assume there is no wall at that location.

            It's not always easy. If the probe is too big and the hallway/column is halfway
            through the probe, then we could only fix this by hand by an artist, or by adding
            more probes (which can consume significantly more resources)

        @param snapSidesDeviationErrorMin
            Valid range: [0; inf)
            Use a very large value to always snap
            0 to disable snapping against this corner
        @param snapSidesDeviationErrorMax
            Valid range: [0; inf)
            Use a very large value to always snap
            0 to disable snapping against this corner
         */
        void setSnapSides( const Vector3 &snapSidesDeviationErrorMin,
                           const Vector3 &snapSidesDeviationErrorMax );

        const Vector3 &getSnapSidesDeviationErrorMin() const;
        const Vector3 &getSnapSidesDeviationErrorMax() const;

        /// Returns numProbes[0] * numProbes[1] * numProbes[2]
        uint32 getMaxNumProbes() const;

        /** Issues GPU commands to render into all probes and obtain depth from it.

            See PccPerPixelGridPlacement::buildEnd
        @param resolution
            Cubemap resolution. Beware you can easily run out of memory!
        @param camera
        @param pixelFormat
        @param camNear
        @param camFar
        */
        void buildStart( uint32 resolution, Camera *camera,
                         PixelFormatGpu pixelFormat = PFG_RGBA8_UNORM_SRGB, float camNear = 0.5f,
                         float camFar = 500.0f );

        /** Finishes placing all probes and renders to them again, this time with the modified
            auto-generated shape sizes.

            Must be called after buildStart. This function reads from GPU memory, thus we must
            wait for our GPU commands issued in buildStart to finish. If you have something
            else to do, you may want to insert your code between those buildStart and buildEnd
            calls in order to maximize CPU/GPU parallelism.

            See PccPerPixelGridPlacement::buildStart
         */
        void buildEnd();

        /// ParallaxCorrectedCubemapAutoListener overloads
        void preCopyRenderTargetToCubemap( TextureGpu *renderTarget, uint32 cubemapArrayIdx ) override;
    };
}  // namespace Ogre

#endif
