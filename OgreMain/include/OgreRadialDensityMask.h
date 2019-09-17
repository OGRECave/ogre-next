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
#ifndef _OgreRadialDensityMask_H_
#define _OgreRadialDensityMask_H_

#include "OgrePrerequisites.h"

#include "OgreGpuProgram.h"
#include "OgreShaderParams.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */

    /**
    @class RadialDensityMaskVrGenerator
        Manages a full screen stereo rectangle that masks rectangles in the periphery and then
        reconstruct these blocks. This allows us to shade at different rates e.g. 1x2, 2x2, 4x4

        It's not as efficient or its quality as good as Variable Rate Shading (because VRS
        shades at lower frequency but it still evaluates the depth buffer at full resolution),
        but it is compatible with all HW.

        Used as an optimization for VR.
    */
    class _OgreExport RadialDensityMask : public MovableAlloc
    {
        Rectangle2D *mRectangle;

        float mRadius[3];
        Vector2 mLeftEyeCenter;
        Vector2 mRightEyeCenter;

        bool mDirty;
        int32 mLastVpWidth;
        int32 mLastVpHeight;
        GpuProgramParametersSharedPtr mPsParams;

        HlmsComputeJob *mReconstructJob;
        ConstBufferPacked *mJobParams;

        static void setEyeCenter( Real *outEyeCenter, Vector2 inEyeCenterClipSpace, const Viewport &vp );

    public:
        RadialDensityMask( SceneManager *sceneManager, const float radius[3], HlmsManager *hlmsManager );
        ~RadialDensityMask();

        void update( Viewport *viewports );

        enum RdmQuality
        {
            /// Cheap, pixelated and grainy
            RdmLow,
            /// Slower than low but much better results.
            /// Can result in sharp text but still aliasing, shimmering/grain
            RdmMedium,
            /// Slowest, but best results.
            /// Can result in blurry text but smoother graphics
            /// (i.e. less aliasing and shimmering/grain)
            RdmHigh
        };

        /** Sets the quality of the reconstruction filter for half resolution rendering.
            Quarter and Sixteenth resolution rendering is always reconstructed using low quality.
        @param quality
        */
        void setQuality( RdmQuality quality );

        /** Sets the center of the eye. Each eye is relative to its own viewport. e.g.
            the default for both eyes is Vector2( 0, 0 ) and not Vector2( -0.5, 0 ) for the
            left eye and Vector2( 0.5, 0 ) for the right eye
        @param leftEyeCenter
            In clip space, i.e. in range [-1; 1]
        @param rightEyeCenter
            In clip space, i.e. in range [-1; 1]
        */
        void setEyesCenter( const Vector2 &leftEyeCenter, const Vector2 &rightEyeCenter );

        /** Changes existing set of radiuses
        @param radius
            For best performance, do not change the value of mRadius[0]
        */
        void setNewRadius( const float radius[3] );
        const float *getRadius( void ) const { return mRadius; }

        Rectangle2D *getRectangle( void ) const { return mRectangle; }
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
