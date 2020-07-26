
#ifndef _Demo_NullCompositorListener_H_
#define _Demo_NullCompositorListener_H_

#include "Compositor/OgreCompositorWorkspaceListener.h"
#include "OgreFrameListener.h"

#include "OgreCamera.h"
#include "OgreMatrix4.h"

namespace Demo
{
    class NullCompositorListener : public Ogre::FrameListener, public Ogre::CompositorWorkspaceListener
    {
    public:
    protected:
        Ogre::TextureGpu *mVrTexture;
        Ogre::Root *mRoot;
        Ogre::RenderSystem *mRenderSystem;

        Ogre::CompositorWorkspace *mWorkspace;

        Ogre::VrData mVrData;
        Ogre::Camera *mCamera;
        Ogre::Camera *mVrCullCamera;
        Ogre::Vector3 mCullCameraOffset;

        Ogre::Real mLastCamNear;
        Ogre::Real mLastCamFar;
        bool mMustSyncAtEndOfFrame;

        void syncCullCamera( void );
        void syncCameraProjection( bool bForceUpdate );

    public:
        NullCompositorListener( Ogre::TextureGpu *vrTexture, Ogre::Root *root,
                                Ogre::CompositorWorkspace *workspace, Ogre::Camera *camera,
                                Ogre::Camera *cullCamera );
        virtual ~NullCompositorListener();

        virtual bool frameStarted( const Ogre::FrameEvent &evt );
        virtual bool frameRenderingQueued( const Ogre::FrameEvent &evt );
        virtual bool frameEnded( const Ogre::FrameEvent &evt );

        virtual void workspacePreUpdate( Ogre::CompositorWorkspace *workspace );
        virtual void passPreExecute( Ogre::CompositorPass *pass );

        virtual void passSceneAfterShadowMaps( Ogre::CompositorPassScene *pass );
        virtual void passSceneAfterFrustumCulling( Ogre::CompositorPassScene *pass );

        /** Returns true if the current waiting mode can update the camera immediately,
            false if it must wait until the end of the frame.
        @remarks
            VrWaitingMode::BeforeSceneGraph and earlier always returns true.

            See NullCompositorListener::setGlitchFree
        */
        bool canSyncCameraTransformImmediately( void ) const;
    };
}  // namespace Demo

#endif
