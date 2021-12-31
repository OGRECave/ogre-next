
#ifndef _Demo_Tutorial_OpenVR_H_
#define _Demo_Tutorial_OpenVR_H_

#include "GraphicsSystem.h"

#include "openvr.h"

namespace Demo
{
    class NullCompositorListener;
    class OpenVRCompositorListener;

    class Tutorial_OpenVRGraphicsSystem final : public GraphicsSystem
    {
        vr::IVRSystem *mHMD;
        std::string mStrDriver;
        std::string mStrDisplay;
        std::string mDeviceModelNumber;
        vr::TrackedDevicePose_t mTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];

        Ogre::CompositorWorkspace *mVrWorkspace;
        Ogre::TextureGpu *mVrTexture;
        Ogre::Camera *mVrCullCamera;

        OpenVRCompositorListener *mOvrCompositorListener;
        /// Only used if USE_OPEN_VR is defined
        NullCompositorListener *mNullCompositorListener;

        Ogre::CompositorWorkspace *setupCompositor() override;

        void setupResources() override;

        static std::string GetTrackedDeviceString( vr::TrackedDeviceIndex_t unDevice,
                                                   vr::TrackedDeviceProperty prop,
                                                   vr::TrackedPropertyError *peError = NULL );
        void initOpenVR();
        void initCompositorVR();
        void createHiddenAreaMeshVR();

    public:
        Tutorial_OpenVRGraphicsSystem( GameState *gameState ) :
            GraphicsSystem( gameState ),
            mHMD( 0 ),
            mVrTexture( 0 ),
            mVrCullCamera( 0 ),
            mOvrCompositorListener( 0 ),
            mNullCompositorListener( 0 )
        {
            memset( mTrackedDevicePose, 0, sizeof( mTrackedDevicePose ) );
        }

        void deinitialize() override;

        OpenVRCompositorListener *getOvrCompositorListener() { return mOvrCompositorListener; }
    };
}  // namespace Demo

#endif
