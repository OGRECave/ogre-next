
#ifndef _Demo_OpenVRCompositorListener_H_
#define _Demo_OpenVRCompositorListener_H_

#include "Compositor/OgreCompositorWorkspaceListener.h"
#include "OgreFrameListener.h"

#include "OgreCamera.h"
#include "OgreMatrix4.h"

#include "openvr.h"

namespace Demo
{
    namespace VrWaitingMode
    {
        /** OpenVR's WaitGetPoses blocks until 3ms before VSync/start of the frame. Valve
            calls this 'running start'.

            Theoretically this reduces GPU bubbles, and helps a lot with meeting VSync one time.

            However... it's extremely difficult to get right.

            So: WaitGetPoses will block until 3ms before VSync. And once it returns,
            *we have 3ms to submit our graphics commands to the GPU*
            If the CPU takes longer than 3ms, VSync will be missed and performance is going
            to go down really bad, even if the system is perfectly capable of sustaining 90hz.

            Thus we want to call WaitGetPoses as late as possible, but not late enough that after
            it returns we have 3ms with nothing to do.

            You can think of the situation like a glass of water with 3ms capacity. You don't
            want the glass to be empty all the time because that's a wait of a precious container
            (of precious CPU cycles), but you absolutely don't want the glass to overflow and spill
            water.

            Therefore WHEN we call WaitGetPoses depends on how long the work we have left to
            do will take. And that depends on:
                1. How fast the CPU is
                2. How expensive the scene is

            Therefore there is no one true location on when to call this function.
            And that's why we can dynamically shift where it is called.

            This enum is ordered. i.e. VrWaitingMode::AfterSwap calls WaitGetPoses as early
            as possible, while VrWaitingMode::AfterFrustumCulling calls it as late as possible.

            More info about running start in GDC 2015 Advanced VR Rendering with Valve's Alex Vlachos
            https://youtu.be/ya8vKZRBXdw?t=1143
         */
        enum VrWaitingMode
        {
            /// Call WaitGetPoses right after swapping the render targets,
            /// i.e. at the end of the frame.
            /// This ensures the next frame has all the controller and head data up to date,
            /// but unless rendering is very simple, it's likely to run very slow.
            AfterSwap,
            /// Calls this before SceneManager::updateSceneGraph. This ensures the head data
            /// is up to date, but your code will probably have read controller location
            /// and thus is not up to date. You may want to add custom code to correct that in
            /// the listener. It is not late yet.
            BeforeSceneGraph,
            /// Calls this after SceneManager::updateSceneGraph. Modifying any SceneNode after this
            /// call must be followed by a call to Node::_getFullTransformUpdated so that their
            /// derived transform is actually updated
            AfterSceneGraph,
            /// Calls this before generating shadow maps.
            ///
            /// If the compositor workspace is complex, modifying the camera now may result in
            /// graphical artifacts if those passes use the camera. How severe depends on what
            /// you're doing.
            ///
            /// See VrWaitingMode::AfterSceneGraph notes, which are relevant.
            BeforeShadowmaps,
            /// Calls this before Frustum Culling.
            /// Modifying the camera now may result in a few minor graphical artifacts, as the shadow
            /// maps have already been processed using the old camera position, thus a few minor
            /// shadowing artifacts may occur (depends on how much the camera moves/rotates between
            /// frames).
            ///
            /// See VrWaitingMode::BeforeShadowmaps & AfterSceneGraph notes.
            /// See OpenVRCompositorListener::canSyncCameraTransformImmediately
            BeforeFrustumCulling,
            /// Calls this after Frustum Culling. Modifying the camera now may result in more
            /// graphical artifacts, as frustum culling will have been performed using an old camera
            /// transform (which can be countered by increasing AABB sizes, thus being more
            /// conservative in culling) and Forward+ has also already collected lights with the wrong
            /// camera transform.
            ///
            /// See VrWaitingMode::BeforeFrustumCulling notes.
            /// See OpenVRCompositorListener::canSyncCameraTransformImmediately
            AfterFrustumCulling,
            NumVrWaitingModes
        };
    }  // namespace VrWaitingMode

    class OpenVRCompositorListener : public Ogre::FrameListener, public Ogre::CompositorWorkspaceListener
    {
    public:
    protected:
        vr::IVRSystem *mHMD;
        vr::IVRCompositor *mVrCompositor;

        Ogre::TextureGpu *mVrTexture;
        Ogre::Root *mRoot;
        Ogre::RenderSystem *mRenderSystem;

        Ogre::CompositorWorkspace *mWorkspace;

        int mValidPoseCount;
        vr::TrackedDevicePose_t mTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
        Ogre::Matrix4 mDevicePose[vr::k_unMaxTrackedDeviceCount];
        vr::ETextureType mApiTextureType;
        Ogre::VrData mVrData;
        Ogre::Camera *mCamera;
        Ogre::Camera *mVrCullCamera;
        Ogre::Vector3 mCullCameraOffset;

        VrWaitingMode::VrWaitingMode mWaitingMode;
        VrWaitingMode::VrWaitingMode mFirstGlitchFreeMode;
        Ogre::Real mLastCamNear;
        Ogre::Real mLastCamFar;
        bool mMustSyncAtEndOfFrame;

        static Ogre::Matrix4 convertSteamVRMatrixToMatrix4( vr::HmdMatrix34_t matPose );
        static Ogre::Matrix4 convertSteamVRMatrixToMatrix4( vr::HmdMatrix44_t matPose );
        void updateHmdTrackingPose();

        void syncCullCamera();
        void syncCamera();
        void syncCameraProjection( bool bForceUpdate );

    public:
        OpenVRCompositorListener( vr::IVRSystem *hmd, vr::IVRCompositor *vrCompositor,
                                  Ogre::TextureGpu *vrTexture, Ogre::Root *root,
                                  Ogre::CompositorWorkspace *workspace, Ogre::Camera *camera,
                                  Ogre::Camera *cullCamera );
        ~OpenVRCompositorListener() override;

        bool frameStarted( const Ogre::FrameEvent &evt ) override;
        bool frameRenderingQueued( const Ogre::FrameEvent &evt ) override;
        bool frameEnded( const Ogre::FrameEvent &evt ) override;

        void workspacePreUpdate( Ogre::CompositorWorkspace *workspace ) override;
        void passPreExecute( Ogre::CompositorPass *pass ) override;

        void passSceneAfterShadowMaps( Ogre::CompositorPassScene *pass ) override;
        void passSceneAfterFrustumCulling( Ogre::CompositorPassScene *pass ) override;

        /// See VrWaitingMode::VrWaitingMode
        void setWaitingMode( VrWaitingMode::VrWaitingMode waitingMode );
        VrWaitingMode::VrWaitingMode getWaitingMode() { return mWaitingMode; }

        /** When operating in VrWaitingMode::AfterSceneGraph or later, there's a chance
            graphical artifacts appear if the camera transform is immediately changed after
            calling WaitGetPoses instead of waiting for the next frame.

            This is a question of whether you want to prioritize latency over graphical
            artifacts, or an artifact-free rendering in exchange for up to one frame of
            latency. The severity and frequency of artifacts may vastly depend on the game.

            Because the graphical artifacts get more severe with each mode, you may e.g.
            find acceptable to tolerate artifacts in VrWaitingMode::BeforeFrustumCulling,
            but not in VrWaitingMode::AfterFrustumCulling
        @param glitchFree
            First stage to consider glitch-free. e.g. calling:
                setGlitchFree( VrWaitingMode::AfterSwap );
            Means that all waiting modes must behave glitch-free, while calling:
                setGlitchFree( VrWaitingMode::BeforeSceneGraph );
            Means that only AfterSwap is allowed to have glitches, while:
                setGlitchFree( VrWaitingMode::NumVrWaitingModes );
            Means that any waiting mode is allowed to have glitches
        */
        void setGlitchFree( VrWaitingMode::VrWaitingMode firstGlitchFreeMode );

        /** Returns true if the current waiting mode can update the camera immediately,
            false if it must wait until the end of the frame.
        @remarks
            VrWaitingMode::BeforeSceneGraph and earlier always returns true.

            See OpenVRCompositorListener::setGlitchFree
        */
        bool canSyncCameraTransformImmediately() const;
    };
}  // namespace Demo

#endif
