
#ifndef _Demo_Tutorial_OpenVR_H_
#define _Demo_Tutorial_OpenVR_H_

#include "GraphicsSystem.h"

#if __cplusplus <= 199711L
    #ifndef nullptr
        #define OgreDemoNullptrDefined
        #define nullptr (0)
    #endif
#endif
#ifdef USE_OPEN_VR
#include "openvr.h"
#endif
#if __cplusplus <= 199711L
    #ifdef OgreDemoNullptrDefined
        #undef OgreDemoNullptrDefined
        #undef nullptr
    #endif
#endif


namespace Demo
{
    class OpenVRCompositorListener;

    class Tutorial_OpenVRGraphicsSystem : public GraphicsSystem
    {
#ifdef USE_OPEN_VR
        vr::IVRSystem *mHMD;
#endif
        std::string mStrDriver;
        std::string mStrDisplay;
        std::string mDeviceModelNumber;
#ifdef USE_OPEN_VR
        vr::TrackedDevicePose_t mTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
#endif

        Ogre::CompositorWorkspace   *mVrWorkspace;
        Ogre::TextureGpu            *mVrTexture;
        Ogre::Camera                *mVrCullCamera;

        OpenVRCompositorListener    *mOvrCompositorListener;

        virtual Ogre::CompositorWorkspace* setupCompositor();

        virtual void setupResources(void);

#ifdef USE_OPEN_VR
        static std::string GetTrackedDeviceString( vr::TrackedDeviceIndex_t unDevice,
                                                   vr::TrackedDeviceProperty prop,
                                                   vr::TrackedPropertyError *peError = NULL );
#endif
        void initOpenVR(void);
        void initCompositorVR(void);
        void createHiddenAreaMeshVR(void);

    public:
        Tutorial_OpenVRGraphicsSystem( GameState *gameState ) :
            GraphicsSystem( gameState ),
#ifdef USE_OPEN_VR
            mHMD( 0 ),
#endif
            mVrTexture( 0 ),
            mVrCullCamera( 0 ),
            mOvrCompositorListener( 0 )
        {
#ifdef USE_OPEN_VR
            memset( mTrackedDevicePose, 0, sizeof (mTrackedDevicePose) );
#endif
        }

        virtual void deinitialize(void);

        OpenVRCompositorListener* getOvrCompositorListener(void) { return mOvrCompositorListener; }
    };
}

#endif
