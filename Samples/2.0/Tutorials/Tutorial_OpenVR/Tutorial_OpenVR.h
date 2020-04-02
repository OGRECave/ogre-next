
#ifndef _Demo_Tutorial_OpenVR_H_
#define _Demo_Tutorial_OpenVR_H_

#include "GraphicsSystem.h"

#if __cplusplus <= 199711L
    #ifndef nullptr
        #define OgreDemoNullptrDefined
        #define nullptr (0)
    #endif
#endif
#include "openvr.h"
#if __cplusplus <= 199711L
    #ifdef OgreDemoNullptrDefined
        #undef OgreDemoNullptrDefined
        #undef nullptr
    #endif
#endif


namespace Demo
{
    class NullCompositorListener;
    class OpenVRCompositorListener;

    class Tutorial_OpenVRGraphicsSystem : public GraphicsSystem
    {
        vr::IVRSystem *mHMD;
        std::string mStrDriver;
        std::string mStrDisplay;
        std::string mDeviceModelNumber;
        vr::TrackedDevicePose_t mTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];

        Ogre::CompositorWorkspace   *mVrWorkspace;
        Ogre::TextureGpu            *mVrTexture;
        Ogre::Camera                *mVrCullCamera;

        OpenVRCompositorListener    *mOvrCompositorListener;
        /// Only used if USE_OPEN_VR is defined
        NullCompositorListener      *mNullCompositorListener;

        virtual Ogre::CompositorWorkspace* setupCompositor();

        virtual void setupResources(void);

        static std::string GetTrackedDeviceString( vr::TrackedDeviceIndex_t unDevice,
                                                   vr::TrackedDeviceProperty prop,
                                                   vr::TrackedPropertyError *peError = NULL );
        void initOpenVR(void);
        void initCompositorVR(void);
        void createHiddenAreaMeshVR(void);

    public:
        Tutorial_OpenVRGraphicsSystem( GameState *gameState ) :
            GraphicsSystem( gameState ),
            mHMD( 0 ),
            mVrTexture( 0 ),
            mVrCullCamera( 0 ),
            mOvrCompositorListener( 0 ),
            mNullCompositorListener( 0 )
        {
            memset( mTrackedDevicePose, 0, sizeof (mTrackedDevicePose) );
        }

        virtual void deinitialize(void);

        OpenVRCompositorListener* getOvrCompositorListener(void) { return mOvrCompositorListener; }
    };
}

#endif
