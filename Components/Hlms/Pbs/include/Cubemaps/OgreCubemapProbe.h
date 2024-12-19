/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

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
#ifndef _OgreCubemapProbe_H_
#define _OgreCubemapProbe_H_

#include "OgreHlmsPbsPrerequisites.h"

#include "Compositor/OgreCompositorChannel.h"
#include "Math/Simple/OgreAabb.h"
#include "OgreIdString.h"
#include "OgreTextureGpu.h"
#include "OgreVector3.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class _OgreHlmsPbsExport CubemapProbe : public OgreAllocatedObj
    {
        friend class ParallaxCorrectedCubemapBase;
        friend class ParallaxCorrectedCubemap;
        friend class ParallaxCorrectedCubemapAuto;

        /// Where to position the camera while constructing the probe.
        Vector3 mProbeCameraPos;
        /// When the camera enters this area, the probe is collected for blending.
        Aabb mArea;
        /// Value between [0; 1] per axis. At 1, the inner region matches mArea (the outer region)
        Vector3 mAreaInnerRegion;
        /// Orientationt. These are not AABBs, but rather OBB (oriented bounding boxes).
        Matrix3 mOrientation;
        Matrix3 mInvOrientation;
        /// The general shape this probe is supposed to represent.
        Aabb mProbeShape;

        TextureGpu       *mTexture;
        uint16            mCubemapArrayIdx;
        SampleDescription mSampleDescription;

        IdString             mWorkspaceDefName;
        CompositorWorkspace *mClearWorkspace;
        CompositorWorkspace *mWorkspace;
        Camera              *mCamera;

        ParallaxCorrectedCubemapBase *mCreator;

        InternalCubemapProbe *mInternalProbe;

        ConstBufferPacked *mConstBufferForManualProbes;
        uint32             mNumDatablockUsers;

        uint16 mPriority;

        /// False if it should be updated every frame. True if only updated when dirty
        bool mStatic;
        /// False if it's static and its camera should not take part in global lighting culling
        bool mCullLights;

    public:
        /// While disabled, this probe won't be updated (even if dirty) and won't be considered
        /// for blending (i.e. won't be used at all).
        bool mEnabled;
        /// True if we must re-render to update the texture's contents. False when we don't.
        bool mDirty;
        /// Number of iterations. The more iterations, the more light bounces and
        /// light reflections we can capture (i.e. mirror of mirrors), but it will
        /// take longer to rebuild the probe.
        /// Default value is 32.
        /// For non-static probes, you should set this value to 1 for performance.
        uint16 mNumIterations;

        /// Mask to group probes. This probe will only be updated (even if dirty) and
        /// blended if mMask & system->mMask is non-zero.
        /// Useful for example for probes to be used during the day while other probes to
        /// be used during the night; thus you only want one group to be active at the same
        /// time.
        /// Or if you have per room probes, but during a panoramic shot where many rooms
        /// are in sight, and you want a more "global" probe.
        /// Defaults to 0xffffffff
        uint32 mMask;

        void destroyWorkspace();

    protected:
        void destroyTexture();

        void acquireTextureAuto();
        void releaseTextureAuto();
        void createInternalProbe();
        void destroyInternalProbe();
        void switchInternalProbeStaticValue();
        void syncInternalProbe();

        void restoreFromClearScene( SceneNode *rootNode );

    public:
        CubemapProbe( ParallaxCorrectedCubemapBase *creator );
        ~CubemapProbe();

        void _releaseManualHardwareResources();
        void _restoreManualHardwareResources();

        /**
        @remarks
            When this CubemapProbe belongs to ParallaxCorrectedCubemapAuto,
            all parameters except isStatic are ignored!
            This call is still required.
        @param width
        @param height
        @param pf
        @param isStatic
            Set to False if it should be updated every frame. True if only updated when dirty
        @param sampleDesc
        @param useManual
            Set to true if you plan on using thie probe for manually rendering, so we keep
            mipmaps at the probe level. User is responsible for supplying a workspace
            definition that will generate mipmaps though!
        */
        void setTextureParams( uint32 width, uint32 height, bool useManual = false,
                               PixelFormatGpu pf = PFG_RGBA8_UNORM_SRGB, bool isStatic = true,
                               SampleDescription sampleDesc = SampleDescription() );

        /** Initializes the workspace so we can actually render to the cubemap.
            You must call setTextureParams first.
        @param mipmapsExecutionMask
            When ParallaxCorrectedCubemapAuto needs to use DPM via 2D Array (see
            ParallaxCorrectedCubemapAuto::setUseDpm2DArray), you will most likely *NOT* want
            to execute the mipmap pass in the workspace you provide, as the system will
            automatically generate mipmaps for you in the 2D Array instead.
            We will be using this value to skip that pass.
            We'll be calling:
            @code
                compositorManager->addWorkspace( ..., executionMask = ~mipmapsExecutionMask);
            @endcode
            But only when ParallaxCorrectedCubemapAuto::getUseDpm2DArray is true.
        @param workspaceDefOverride
            Pass a null IdString() to use the default workspace definition passed to
            ParallaxCorrectedCubemap.
            This value allows you to override it with a different workspace definition.
        */
        void initWorkspace( float cameraNear = 0.5f, float cameraFar = 500.0f,
                            IdString                    workspaceDefOverride = IdString(),
                            const CompositorChannelVec &additionalChannels = CompositorChannelVec(),
                            uint8                       executionMask = 0xFF );
        bool isInitialized() const;

        /** Sets cubemap probe's parameters.
        @param cameraPos
            Specify where the camera will be positioned.
        @param area
            When the camera enters this area, the probe is collected for blending.
        @param areaInnerRegion
            A value in range [0; 1]. It indicates a % of the OBB's size that will have smooth
            interpolation with other probes. When region = 1.0; stepping outside the OBB's results
            in a lighting "pop". The smaller the value, the smoother the transition, but at the cost
            of quality & precision while inside the OBB (as results get mixed up with other probes').
            The value is per axis.
        @param orientation
            The orientation of the AABB that makes it an OBB.
            This orientation is applied to both probeShape AND area.
            Skewing and shearing is not tested. May or may not work.
        @param probeShape
            Note AABB is actually an OBB (Oriented Bounding Box). See orientation parameter.
            The OBB should closely match the shape of the environment around it. The better it fits,
            the more accurate the reflections.
        */
        void set( const Vector3 &cameraPos, const Aabb &area, const Vector3 &areaInnerRegion,
                  const Matrix3 &orientation, const Aabb &probeShape );

        /** Set to False if it should be updated every frame. True if only updated when dirty
        @remarks
            This call is not cheap.
        */
        void setStatic( bool isStatic );
        bool getStatic() const { return mStatic; }

        /** Set to False if it's static and its camera should not take part in global lighting culling
       @remarks
           call it outside before renderOneFrame()
       */
        void setCullLights( bool b ) { mCullLights = b; }
        bool getCullLights() const { return mCullLights; }

        /** When two probes overlap, you may want one probe to have particularly more influence
            than the others. Use this value to decrease/increase the weight when blending the probes.
        @remarks
            This value is only useful for Per-Pixel cubemaps
        @param priority
            A value in range [1; 65535]
            A higher value means the probe should have a stronger influence over the others.
        */
        void     setPriority( uint16 priority );
        uint16_t getPriority() const;

        Aabb getAreaLS() const { return Aabb( Vector3::ZERO, mArea.mHalfSize ); }

        /** Gets the Normalized Distance Function.
        @param posLS
            Position, in local space (relative to this probe)
        @return
            Interpretation:
                <=0 means we're inside the inner range, or in its border.
                Range (0; 1) we're between inner and outer range.
                >=1 means we're outside the object.
        */
        Real getNDF( const Vector3 &posLS ) const;

        void _prepareForRendering();
        void _clearCubemap();
        void _updateRender();

        const Vector3 &getProbeCameraPos() const { return mProbeCameraPos; }
        const Aabb    &getArea() const { return mArea; }
        const Vector3 &getAreaInnerRegion() const { return mAreaInnerRegion; }
        const Matrix3 &getOrientation() const { return mOrientation; }
        const Matrix3 &getInvOrientation() const { return mInvOrientation; }
        const Aabb    &getProbeShape() const { return mProbeShape; }

        CompositorWorkspace *getWorkspace() const { return mWorkspace; }

        TextureGpu *getInternalTexture() const { return mTexture; }

        void _addReference();
        void _removeReference();

        const SceneNode *getInternalCubemapProbeSceneNode() const;

        uint16 getInternalSliceToArrayTexture() const { return mCubemapArrayIdx; }

        ConstBufferPacked *getConstBufferForManualProbes() { return mConstBufferForManualProbes; }

        ParallaxCorrectedCubemapBase *getCreator() { return mCreator; }
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
