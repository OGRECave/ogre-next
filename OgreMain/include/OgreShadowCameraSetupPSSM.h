/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd
Copyright (c) 2006 Matthias Fink, netAllied GmbH <matthias.fink@web.de>

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
#ifndef __ShadowCameraSetupPSSM_H__
#define __ShadowCameraSetupPSSM_H__

#include "OgrePrerequisites.h"

#include "OgreShadowCameraSetupConcentric.h"
#include "OgreShadowCameraSetupFocused.h"

#include "ogrestd/vector.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Scene
     *  @{
     */
    /** Parallel Split Shadow Map (PSSM) shadow camera setup.
    @remarks
        A PSSM shadow system uses multiple shadow maps per light and maps each
        texture into a region of space, progressing away from the camera. As such
        it is most appropriate for directional light setups. This particular version
        also uses LiSPSM projection for each split to maximise the quality.
    @note
        Because PSSM uses multiple shadow maps per light, you will need to increase
        the number of shadow textures available (via SceneManager) to match the
        number of shadow maps required (default is 3 per light).
    */
    class _OgreExport PSSMShadowCameraSetup : public FocusedShadowCameraSetup
    {
    public:
        typedef vector<Real>::type SplitPointList;

    protected:
        uint32         mSplitCount;
        uint32         mNumStableSplits;
        SplitPointList mSplitPoints;
        SplitPointList mSplitBlendPoints;
        Real           mSplitFadePoint;
        Real           mSplitPadding;

        mutable size_t mCurrentIteration;

        ConcentricShadowCamera mConcentricShadowCamera;

    public:
        /// Constructor, defaults to 3 splits
        PSSMShadowCameraSetup();
        ~PSSMShadowCameraSetup() override;

        /** Calculate a new splitting scheme.
        @param splitCount The number of splits to use
        @param nearDist The near plane to use for the first split
        @param farDist The far plane to use for the last split
        @param lambda Factor to use to reduce the split size
        @param blend Factor to use to reduce the split seams
        @param fade Factor to use to fade out the last split
        */
        void calculateSplitPoints( uint splitCount, Real nearDist, Real farDist,
                                   Real lambda = Real( 0.95 ), Real blend = Real( 0.125 ),
                                   Real fade = Real( 0.313 ) );

        /** Manually configure a new splitting scheme.
        @param newSplitPoints A list which is splitCount + 1 entries long, containing the
            split points. The first value is the near point, the last value is the
            far point, and each value in between is both a far point of the previous
            split, and a near point for the next one.
        @param blend Factor to use to reduce the split seams.
        @param fade Factor to use to fade out the last split.
        */
        void setSplitPoints( const SplitPointList &newSplitPoints, Real blend = Real( 0.125 ),
                             Real fade = Real( 0.313 ) );

        /** Set the LiSPSM optimal adjust factor for a given split (call after
            configuring splits).
        */
        void setOptimalAdjustFactor( size_t splitIndex, Real factor );

        /** Set the padding factor to apply to the near & far distances when matching up
            splits to one another, to avoid 'cracks'.
        */
        void setSplitPadding( Real pad ) { mSplitPadding = pad; }

        /** Get the padding factor to apply to the near & far distances when matching up
            splits to one another, to avoid 'cracks'.
        */
        Real getSplitPadding() const { return mSplitPadding; }
        /// Get the number of splits.
        uint getSplitCount() const { return mSplitCount; }

        /// PSSM tends to be very unstable to camera rotation changes. Rotate the camera around
        /// and the shadow mapping artifacts keep changing.
        ///
        /// setNumStableSplits allows you to fix that problem; by switching to ConcentricShadowCamera
        /// for the first N splits you specify; while the rest of the splits will use
        /// FocusedShadowCameraSetup.
        ///
        /// We achieve rotation stability by sacrificing overall quality. Using ConcentricShadowCamera
        /// on higher splits means sacrificing exponentially a lot more quality (and even performance);
        /// thus the recommended values are numStableSplits = 1 or numStableSplits = 2
        ///
        /// The default is numStableSplits = 0 which disables the feature
        void   setNumStableSplits( uint32 numStableSplits ) { mNumStableSplits = numStableSplits; }
        uint32 getNumStableSplits() const { return mNumStableSplits; }

        /// Returns a LiSPSM shadow camera with PSSM splits base on iteration.
        void getShadowCamera( const Ogre::SceneManager *sm, const Ogre::Camera *cam,
                              const Ogre::Light *light, Ogre::Camera *texCam, size_t iteration,
                              const Vector2 &viewportRealSize ) const override;

        /// Returns the calculated split points.
        inline const SplitPointList &getSplitPoints() const { return mSplitPoints; }

        /// Returns the calculated split blend points.
        inline const SplitPointList &getSplitBlendPoints() const { return mSplitBlendPoints; }

        /// Returns the calculated split fade point.
        inline const Real &getSplitFadePoint() const { return mSplitFadePoint; }
    };
    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
