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
#ifndef _OgreParallaxCorrectedCubemapAuto_H_
#define _OgreParallaxCorrectedCubemapAuto_H_

#include "OgreHlmsPbsPrerequisites.h"

#include "Cubemaps/OgreParallaxCorrectedCubemapBase.h"

#include "Compositor/OgreCompositorWorkspaceListener.h"
#include "OgreFrameListener.h"
#include "OgreGpuProgramParams.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class _OgreHlmsPbsExport ParallaxCorrectedCubemapAutoListener
    {
    public:
        virtual ~ParallaxCorrectedCubemapAutoListener();
        /// Called when the probe is done rendering to the temporary cubemap, and is now about
        /// to be copied to the cubemap array (or about to be converted to DPM 2D array texture)
        virtual void preCopyRenderTargetToCubemap( TextureGpu *renderTarget, uint32 cubemapArrayIdx );
    };

    /**
    @class ParallaxCorrectedCubemapAuto
        Per-Pixel reflection probes.

        Per-Pixel reflection probes are much easier to handle, they're more flexible and powerful.
        However they require a modern GPU with Cubemap Arrays.
        Forward Clustered also must be active.

        ParallaxCorrectedCubemapAuto supports having more CubemapProbes than the maximum number
        of probes you pass to ParallaxCorrectedCubemapAuto::setEnabled.
        However only 'maxNumProbes' probes can be active at the same time.
        This is all done thanks to _acquireTextureSlot & _releaseTextureSlot.

        To activate a probe, call CubemapProbe::initWorkspace.
        To deactivate it, call CubemapProbe::destroyWorkspace.
    */
    class _OgreHlmsPbsExport ParallaxCorrectedCubemapAuto : public ParallaxCorrectedCubemapBase,
                                                            public FrameListener
    {
        CubemapProbeVec mDirtyProbes;

        /// This variable should be updated every frame and often represents the camera position,
        /// but it can also be used set to other things like the player's character position.
    public:
        Vector3 mTrackedPosition;

    private:
        TextureGpu *mRenderTarget;
        TextureGpu *mIblTarget;

        vector<uint64>::type mReservedSlotBitset;

        ParallaxCorrectedCubemapAutoListener *mListener;

        void updateSceneGraph();
        /// Probes with a large number of iterations will blow up our memory consumption
        /// because too many commands will be queued up before flushing the command buffer
        /// (e.g. a probe with 32 iterations 5 shadow maps per face will consume
        ///     6 faces * 6 pass_scene * 32 iterations = 1152
        /// 1152x the number of commands.
        /// This function will flush with each iteration so that memory consumption doesn't
        /// skyrocket. Furthermore this allows GPU & CPU to work in parallel.
        /// This function must NOT be called after the Compositor started updating workspaces.
        void updateExpensiveCollectedDirtyProbes( uint16 iterationThreshold );
        void updateRender();

    public:
        ParallaxCorrectedCubemapAuto( IdType id, Root *root, SceneManager *sceneManager,
                                      const CompositorWorkspaceDef *probeWorkspaceDef );
        ~ParallaxCorrectedCubemapAuto() override;

        void destroyProbe( CubemapProbe *probe ) override;

        void setListener( ParallaxCorrectedCubemapAutoListener *listener ) { mListener = listener; }
        ParallaxCorrectedCubemapAutoListener *getListener() const { return mListener; }

        TextureGpu *_acquireTextureSlot( uint16 &outTexSlot ) override;
        void        _releaseTextureSlot( TextureGpu *texture, uint32 texSlot ) override;

        TextureGpu *findTmpRtt( const TextureGpu *baseParams ) override;
        void        releaseTmpRtt( const TextureGpu *tmpRtt ) override;

        TextureGpu *findIbl( const TextureGpu *baseParams ) override;
        void        releaseIbl( const TextureGpu *tmpRtt ) override;

        void _copyRenderTargetToCubemap( uint32 cubemapArrayIdx ) override;

        /** Will update both mTrackedPosition with appropiate settings
            every time it's called. Must be called every time the camera changes.

            This information is used to know which nearby non-static probes need
            to be updated every frame.
        @remarks
            You don't *have to* use a camera, which is why mTrackedPosition is a public variables.
            Sometimes you want something else to be used as reference for probe
            update (e.g. character's position instead of the camera). This is up to you.
        @param trackedCamera
            Camera whose settings to use as reference. We will not keep a reference to this pointer.
        */
        void setUpdatedTrackedDataFromCamera( Camera *trackedCamera );

        /** Enables/disables this ParallaxCorrectedCubemapAuto system.
            It will (de)allocate some resources, thus it may cause stalls.
            If you need to temporarily pause the system (or toggle at high frequency)
            use mPaused instead (it's a public variable).
        @param bEnabled
            True to enable. False to disable. When false, the rest of the arguments are ignored.
        @param width
            Unlike the manual system, in PCC Auto all probes must use the same resolution
        @param height
            Unlike the manual system, in PCC Auto all probes must use the same resolution
        @param maxNumProbes
            Max number of probes. Necessary. On AMD GCN cards, this should be a power of 2
            otherwise the driver will internally round it up (and waste memory)
        @param pixelFormat
            PixelFormatGpu for the cubemap
        */
        void setEnabled( bool bEnabled, uint32 width, uint32 height, uint32 maxNumProbes,
                         PixelFormatGpu pixelFormat );
        bool getEnabled() const;

        /** Whether we should use Dual Paraboloid Mapping with 2D Array instead of Cubemap Arrays
        @remarks
            DPM is lower quality than cubemap arrays. However not all GPUs support cubemap arrays,
            most notably iOS GPUs before A11 chips and DX10 level HW.

            When cubemap arrays are not supported, this setting is always forced on.<br/>
            When cubemap arrays are supported, it's up to you. The most likely reason you want to
            use DPM overy cubemap arrays is to see how it looks like on unsupported platforms.

            You cannot toggle this setting after enabling the system. You must call
            ParallaxCorrectedCubemapAuto::setEnabled( false, ... ); to switch this.
        @param useDpm2DArray
            True to use DPM, cubemap arrays otherwise. Default: False unless GPU does not support
            cubemap arrays
        */
        void setUseDpm2DArray( bool useDpm2DArray );

        /// By default the probes will be constructed when the user enters the vecinity
        /// of non-static probes, and whenever a static probe is dirty.
        /// This can cause noticeable stalls. Use this function to regenerate them all
        /// at once (i.e. at loading time)
        /// This function also uses a memory-friendly way of updating the probes.
        void updateAllDirtyProbes() override;

        // CompositorWorkspaceListener overloads
        void allWorkspacesBeforeBeginUpdate() override;
        void allWorkspacesBeginUpdate() override;

        void passPreExecute( CompositorPass *pass ) override;

        // FrameListener overloads
        bool frameStarted( const FrameEvent &evt ) override;
    };

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
