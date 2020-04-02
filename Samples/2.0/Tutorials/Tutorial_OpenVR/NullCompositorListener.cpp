
#include "NullCompositorListener.h"

#include "Compositor/Pass/OgreCompositorPass.h"
#include "Compositor/Pass/OgreCompositorPassDef.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassScene.h"
#include "OgreRenderSystem.h"
#include "OgreTextureGpu.h"

#include "Compositor/OgreCompositorWorkspace.h"

namespace Demo
{
    NullCompositorListener::NullCompositorListener( Ogre::TextureGpu *vrTexture, Ogre::Root *root,
                                                    Ogre::CompositorWorkspace *workspace,
                                                    Ogre::Camera *camera, Ogre::Camera *cullCamera ) :
        mVrTexture( vrTexture ),
        mRoot( root ),
        mRenderSystem( root->getRenderSystem() ),
        mWorkspace( workspace ),
        mCamera( camera ),
        mVrCullCamera( cullCamera ),
        mCullCameraOffset( Ogre::Vector3::ZERO ),
        mLastCamNear( 0 ),
        mLastCamFar( 0 ),
        mMustSyncAtEndOfFrame( false )
    {
        memset( &mVrData, 0, sizeof( mVrData ) );

        mCamera->setVrData( &mVrData );
        syncCameraProjection( true );

        mRoot->addFrameListener( this );
        mWorkspace->addListener( this );
    }
    //-------------------------------------------------------------------------
    NullCompositorListener::~NullCompositorListener()
    {
        if( mCamera )
            mCamera->setVrData( 0 );

        mWorkspace->removeListener( this );
        mRoot->removeFrameListener( this );
    }
    //-------------------------------------------------------------------------
    void NullCompositorListener::syncCullCamera( void )
    {
        const Ogre::Quaternion derivedRot = mCamera->getDerivedOrientation();
        Ogre::Vector3 camPos = mCamera->getDerivedPosition();
        mVrCullCamera->setOrientation( derivedRot );
        mVrCullCamera->setPosition( camPos + derivedRot * mCullCameraOffset );
    }
    //-------------------------------------------------------------------------
    void NullCompositorListener::syncCameraProjection( bool bForceUpdate )
    {
        const Ogre::Real camNear = mCamera->getNearClipDistance();
        const Ogre::Real camFar = mCamera->getFarClipDistance();

        if( mLastCamNear != camNear || mLastCamFar != camFar || bForceUpdate )
        {
            Ogre::Matrix4 eyeToHead[2] = { Ogre::Matrix4::IDENTITY, Ogre::Matrix4::IDENTITY };
            Ogre::Matrix4 projectionMatrixRS[2] = { mCamera->getProjectionMatrixWithRSDepth(),
                                                    mCamera->getProjectionMatrixWithRSDepth() };

            mVrData.set( eyeToHead, projectionMatrixRS );
            mLastCamNear = camNear;
            mLastCamFar = camFar;

            mCullCameraOffset = Ogre::Vector3::ZERO;

            mVrCullCamera->setNearClipDistance( camNear );
            mVrCullCamera->setFarClipDistance( camFar );
            mVrCullCamera->setFOVy( mCamera->getFOVy() );
        }
    }
    //-------------------------------------------------------------------------
    bool NullCompositorListener::frameStarted( const Ogre::FrameEvent &evt ) { return true; }
    //-------------------------------------------------------------------------
    bool NullCompositorListener::frameRenderingQueued( const Ogre::FrameEvent &evt )
    {
        mRenderSystem->flushCommands();
        return true;
    }
    //-------------------------------------------------------------------------
    bool NullCompositorListener::frameEnded( const Ogre::FrameEvent &evt )
    {
        syncCameraProjection( false );
        syncCullCamera();
        return true;
    }
    //-------------------------------------------------------------------------
    void NullCompositorListener::workspacePreUpdate( Ogre::CompositorWorkspace *workspace ) {}
    //-------------------------------------------------------------------------
    void NullCompositorListener::passPreExecute( Ogre::CompositorPass *pass ) {}
    //-------------------------------------------------------------------------
    void NullCompositorListener::passSceneAfterShadowMaps( Ogre::CompositorPassScene *pass ) {}
    //-------------------------------------------------------------------------
    void NullCompositorListener::passSceneAfterFrustumCulling( Ogre::CompositorPassScene *pass ) {}
    //-------------------------------------------------------------------------
    bool NullCompositorListener::canSyncCameraTransformImmediately( void ) const { return true; }
}  // namespace Demo
