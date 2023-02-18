/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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
#ifndef __Camera_H__
#define __Camera_H__

// Default options
#include "OgrePrerequisites.h"

// Matrices & Vectors
#include "OgreCommon.h"
#include "OgreFrustum.h"
#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class Matrix4;
    class Ray;

    struct VrData
    {
        Matrix4 mHeadToEye[2];
        Matrix4 mProjectionMatrix[2];
        // Matrix4 mLeftToRight;
        Vector3 mLeftToRight;

        void set( const Matrix4 eyeToHead[2], const Matrix4 projectionMatrix[2] )
        {
            for( int i = 0; i < 2; ++i )
            {
                mHeadToEye[i] = eyeToHead[i];
                mProjectionMatrix[i] = projectionMatrix[i];
                mHeadToEye[i] = mHeadToEye[i].inverseAffine();
            }
            mLeftToRight = ( mHeadToEye[0] * eyeToHead[1] ).getTrans();
        }
    };

    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Scene
     *  @{
     */

    /** A viewpoint from which the scene will be rendered.
    @remarks
        OGRE renders scenes from a camera viewpoint into a buffer of
        some sort, normally a window or a texture (a subclass of
        RenderTarget). OGRE cameras support both perspective projection (the default,
        meaning objects get smaller the further away they are) and
        orthographic projection (blueprint-style, no decrease in size
        with distance). Each camera carries with it a style of rendering,
        e.g. full textured, flat shaded, wireframe), field of view,
        rendering distances etc, allowing you to use OGRE to create
        complex multi-window views if required. In addition, more than
        one camera can point at a single render target if required,
        each rendering to a subset of the target, allowing split screen
        and picture-in-picture views.
    @par
        Cameras maintain their own aspect ratios, field of view, and frustum,
        and project co-ordinates into a space measured from -1 to 1 in x and y,
        and 0 to 1 in z. At render time, the camera will be rendering to a
        Viewport which will translate these parametric co-ordinates into real screen
        co-ordinates. Obviously it is advisable that the viewport has the same
        aspect ratio as the camera to avoid distortion (unless you want it!).
    @par
        Starting Ogre 2.x, a Camera must be attached to a SceneNode, using the
        method SceneNode::attachObject. By default the camera is attached to
        the root scene node on creation.
        When this is done the Camera will combine it's own
        position/orientation settings with it's parent SceneNode.
        This is also useful for implementing more complex Camera / object
        relationships i.e. having a camera attached to a world object.
    */
    class _OgreExport Camera : public Frustum
    {
    public:
        /** Listener interface so you can be notified of Camera events.
         */
        class _OgreExport Listener
        {
        public:
            Listener() {}
            virtual ~Listener();

            /// Called prior to the scene being rendered with this camera
            virtual void cameraPreRenderScene( Camera *cam ) { (void)cam; }

            /// Called after the scene has been rendered with this camera
            virtual void cameraPostRenderScene( Camera *cam ) { (void)cam; }

            /// Called when the camera is being destroyed
            virtual void cameraDestroyed( Camera *cam ) { (void)cam; }
        };

        /// Sets how the objects are sorted. This affects both opaque
        /// (performance optimization, rendered front to back) and
        /// transparents (visual correctness, rendered back to front)
        ///
        /// Object sorting is approximate, and some scenes are suited
        /// to different modes depending on the objects' geometric properties
        ///
        /// See https://forums.ogre3d.org/viewtopic.php?f=2&t=94090 for examples
        ///
        /// Please note that RenderQueue::addRenderable quantizes the final depth value.
        /// Therefore if two objects are numerically very close, the chosen mode may not
        /// make too much of a difference.
        enum CameraSortMode
        {
            /// Sort objects by distance to camera. i.e.
            ///     cameraPos.distance( objPos ) - objRadius
            ///
            /// The bigger the object radius, the closer it is considered to be to the camera
            SortModeDistance,
            /// Sort objects by depth i.e.
            ///     objViewSpacePos.z - objRadius
            ///
            /// The bigger the object radius, the closer it is considered to be to the camera
            SortModeDepth,
            /// Same as SortModeDistance, but skips object radius from calculations
            SortModeDistanceRadiusIgnoring,
            /// Same as SortModeDepth, but skips object radius from calculations
            SortModeDepthRadiusIgnoring
        };

    protected:
        /// Scene manager responsible for the scene
        SceneManager *mSceneMgr;

        /// Camera orientation, quaternion style
        Quaternion mOrientation;

        /// Camera position - default (0,0,0)
        Vector3 mPosition;

        /// Derived orientation/position of the camera, including reflection
        mutable Quaternion mDerivedOrientation;
        mutable Vector3    mDerivedPosition;

        /// Real world orientation/position of the camera
        mutable Quaternion mRealOrientation;
        mutable Vector3    mRealPosition;

        /// Whether to yaw around a fixed axis.
        bool mYawFixed;
        /// Fixed axis to yaw around
        Vector3 mYawFixedAxis;

        /// Stored number of visible faces, batches, etc. in the last render
        RenderingMetrics mLastRenderingMetrics;

        VrData *mVrData;

        /// Shared class-level name for Movable type
        static String msMovableType;

        /// SceneNode which this Camera will automatically track
        SceneNode *mAutoTrackTarget;
        /// Tracking offset for fine tuning
        Vector3 mAutoTrackOffset;

        /// Scene LOD factor used to adjust overall LOD
        Real mSceneLodFactor;
        /// Inverted scene LOD factor, can be used by Renderables to adjust their LOD
        Real mSceneLodFactorInv;

        /** Viewing window.
        @remarks
        Generalize camera class for the case, when viewing frustum doesn't cover all viewport.
        */
        Real mWLeft, mWTop, mWRight, mWBottom;
        /// Is viewing window used.
        bool mWindowSet;
        /// Windowed viewport clip planes
        mutable PlaneList mWindowClipPlanes;
        /// Was viewing window changed.
        mutable bool mRecalcWindow;
        /// The last viewport to be added using this camera
        Viewport *mLastViewport;
        /** Whether aspect ratio will automatically be recalculated
            when a viewport changes its size
        */
        bool mAutoAspectRatio;
        /// Custom culling frustum
        Frustum *mCullFrustum;
        /// Whether or not the rendering distance of objects should take effect for this camera
        bool mUseRenderingDistance;
        /// Camera to use for LOD calculation
        const Camera *mLodCamera;

        bool mNeedsDepthClamp;

        /// Whether or not the minimum display size of objects should take effect for this camera
        bool mUseMinPixelSize;
        /// @see Camera::getPixelDisplayRatio
        Real mPixelDisplayRatio;

        float mConstantBiasScale;

        typedef vector<Listener *>::type ListenerList;
        ListenerList                     mListeners;

        static CameraSortMode msDefaultSortMode;

    public:
        /// PUBLIC VARIABLE. This variable can be altered directly.
        /// Changes are reflected immediately.
        CameraSortMode mSortMode;

        /** Sets the default sort mode for all future Camera instances. */
        static void setDefaultSortMode( CameraSortMode sortMode ) { msDefaultSortMode = sortMode; }
        static CameraSortMode getDefaultSortMode() { return msDefaultSortMode; }

    protected:
        // Internal functions for calcs
        bool isViewOutOfDate() const override;
        /// Signal to update frustum information.
        void invalidateFrustum() const override;
        /// Signal to update view information.
        void invalidateView() const override;

        /** Do actual window setting, using parameters set in SetWindow call
        @remarks
            The method will called on demand.
        */
        virtual void setWindowImpl() const;

        /** Helper function for forwardIntersect that intersects rays with canonical plane */
        virtual vector<Vector4>::type getRayForwardIntersect( const Vector3 &anchor, const Vector3 *dir,
                                                              Real planeOffset ) const;

    public:
        /** Standard constructor.
         */
        Camera( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *sm );

        /** Standard destructor.
         */
        ~Camera() override;

        /// Add a listener to this camera
        virtual void addListener( Listener *l );
        /// Remove a listener to this camera
        virtual void removeListener( Listener *l );

        /** Returns a pointer to the SceneManager this camera is rendering through.
         */
        SceneManager *getSceneManager() const;

        /** Sets the camera's position.
         */
        void setPosition( Real x, Real y, Real z );

        /** Sets the camera's position.
         */
        void setPosition( const Vector3 &vec );

        /** Retrieves the camera's position.
         */
        const Vector3 &getPosition() const;

        /** Moves the camera's position by the vector offset provided along world axes.
         */
        void move( const Vector3 &vec );

        /** Moves the camera's position by the vector offset provided along it's own axes (relative to
         * orientation).
         */
        void moveRelative( const Vector3 &vec );

        /** Sets the camera's direction vector.
        @remarks
            Note that the 'up' vector for the camera will automatically be recalculated based on the
            current 'up' vector (i.e. the roll will remain the same).
        */
        void setDirection( Real x, Real y, Real z );

        /** Sets the camera's direction vector.
         */
        void setDirection( const Vector3 &vec );

        /** Gets the camera's direction.
         */
        Vector3 getDirection() const;

        /** Gets the camera's up vector.
         */
        Vector3 getUp() const;

        /** Gets the camera's right vector.
         */
        Vector3 getRight() const;

        /** Points the camera at a location in worldspace.
        @remarks
            This is a helper method to automatically generate the
            direction vector for the camera, based on it's current position
            and the supplied look-at point.
        @param
            targetPoint A vector specifying the look at point.
        */
        void lookAt( const Vector3 &targetPoint );
        /** Points the camera at a location in worldspace.
        @remarks
            This is a helper method to automatically generate the
            direction vector for the camera, based on it's current position
            and the supplied look-at point.
        @param x
            The @c x co-ordinates of the point to look at.
        @param y
            The @c y co-ordinates of the point to look at.
        @param z
            The @c z co-ordinates of the point to look at.
        */
        void lookAt( Real x, Real y, Real z );

        /** Rolls the camera anticlockwise, around its local z axis.
         */
        void roll( const Radian &angle );

        /** Rotates the camera anticlockwise around it's local y axis.
         */
        void yaw( const Radian &angle );

        /** Pitches the camera up/down anticlockwise around it's local z axis.
         */
        void pitch( const Radian &angle );

        /** Rotate the camera around an arbitrary axis.
         */
        void rotate( const Vector3 &axis, const Radian &angle );

        /** Rotate the camera around an arbitrary axis using a Quaternion.
         */
        void rotate( const Quaternion &q );

        /** Tells the camera whether to yaw around it's own local Y axis or a
            fixed axis of choice.
        @remarks
            This method allows you to change the yaw behaviour of the camera
            - by default, the camera yaws around a fixed Y axis. This is
            often what you want - for example if you're making a first-person
            shooter, you really don't want the yaw axis to reflect the local
            camera Y, because this would mean a different yaw axis if the
            player is looking upwards rather than when they are looking
            straight ahead. You can change this behaviour by calling this
            method, which you will want to do if you are making a completely
            free camera like the kind used in a flight simulator.
        @param useFixed
            If @c true, the axis passed in the second parameter will
            always be the yaw axis no matter what the camera orientation.
            If false, the camera yaws around the local Y.
        @param fixedAxis
            The axis to use if the first parameter is true.
        */
        void setFixedYawAxis( bool useFixed, const Vector3 &fixedAxis = Vector3::UNIT_Y );

        /** Returns the camera's current orientation.
         */
        const Quaternion &getOrientation() const;

        /** Sets the camera's orientation.
         */
        void setOrientation( const Quaternion &q );

        /** When a camera is created via SceneManager::createCamera, there are two
            additional parameters.
        @param collectLights
            Tell Ogre to cull lights against this camera. This requires additional
            CPU power. If a camera is not going to be used for a long time (or it
            doesn't need lights, which is what happens with shadow mapping cameras)
            you want this set to false. Otherwise if you need to render and need
            lights, enable this setting.
        @param isCubemap
            Use an alternative algorithm to collect lights in 360° around the camera.
            This is required if the camera is going to be used in passes where
            CompositorPassSceneDef::mCameraCubemapReorient = true;
            Does nothing if collectLights = false
        */
        void setLightCullingVisibility( bool collectLights, bool isCubemap );

        /** Tells the Camera to contact the SceneManager to render from it's viewpoint.
        @param vp The viewport to render to
        @param includeOverlays Whether or not any overlay objects should be included
        @param firstRq
            First RenderQueue ID to render (inclusive)
        @param lastRq
            Last RenderQueue ID to render (exclusive)
        */
        void _cullScenePhase01( Camera *renderCamera, const Camera *lodCamera, Viewport *vp,
                                uint8 firstRq, uint8 lastRq, bool reuseCullData );

        void _renderScenePhase02( const Camera *lodCamera, uint8 firstRq, uint8 lastRq,
                                  bool includeOverlays );

        /** Function for outputting to a stream.
         */
        _OgreExport friend std::ostream &operator<<( std::ostream &o, const Camera &c );

        /** Internal method to notify camera of the visible faces in the last render.
         */
        void _notifyRenderingMetrics( const RenderingMetrics &metrics );

        /** Internal method to retrieve the number of visible faces, batches, etc in the last render.
         */
        const RenderingMetrics &_getRenderingMetrics() const;

        /** Internal method to retrieve the number of visible faces in the last render.
         * @deprecated use Camera::_getRenderingMetrics() instead.
         */
        size_t _getNumRenderedFaces() const;

        /** Internal method to retrieve the number of visible batches in the last render.
         * @deprecated use Camera::_getRenderingMetrics() instead.
         */
        size_t _getNumRenderedBatches() const;

        /** Gets the derived orientation of the camera, including any
            rotation inherited from a node attachment and reflection matrix. */
        const Quaternion &getDerivedOrientation() const;
        /** Gets the derived position of the camera, including any
            translation inherited from a node attachment and reflection matrix. */
        const Vector3 &getDerivedPosition() const;
        /** Gets the derived direction vector of the camera, including any
            rotation inherited from a node attachment and reflection matrix. */
        Vector3 getDerivedDirection() const;
        /** Gets the derived up vector of the camera, including any
            rotation inherited from a node attachment and reflection matrix. */
        Vector3 getDerivedUp() const;
        /** Gets the derived right vector of the camera, including any
            rotation inherited from a node attachment and reflection matrix. */
        Vector3 getDerivedRight() const;

        /// Same as getDerivedPosition, but doesn't check if dirty.
        const Vector3    &_getCachedDerivedPosition() const { return mDerivedPosition; }
        const Quaternion &_getCachedDerivedOrientation() const { return mDerivedOrientation; }

        /** Gets the real world orientation of the camera, including any
            rotation inherited from a node attachment */
        const Quaternion &getRealOrientation() const;
        /** Gets the real world position of the camera, including any
            translation inherited from a node attachment. */
        const Vector3 &getRealPosition() const;
        /** Gets the real world direction vector of the camera, including any
            rotation inherited from a node attachment. */
        Vector3 getRealDirection() const;
        /** Gets the real world up vector of the camera, including any
            rotation inherited from a node attachment. */
        Vector3 getRealUp() const;
        /** Gets the real world right vector of the camera, including any
            rotation inherited from a node attachment. */
        Vector3 getRealRight() const;

        const Vector3    &_getCachedRealPosition() const { return mRealPosition; }
        const Quaternion &_getCachedRealOrientation() const { return mRealOrientation; }

        /** Overridden from Frustum/Renderable */
        void getWorldTransforms( Matrix4 *mat ) const override;

        /** Overridden from MovableObject */
        const String &getMovableType() const override;

        /** Enables / disables automatic tracking of a SceneNode.
        @remarks
            If you enable auto-tracking, this Camera will automatically rotate to
            look at the target SceneNode every frame, no matter how
            it or SceneNode move. This is handy if you want a Camera to be focused on a
            single object or group of objects. Note that by default the Camera looks at the
            origin of the SceneNode, if you want to tweak this, e.g. if the object which is
            attached to this target node is quite big and you want to point the camera at
            a specific point on it, provide a vector in the 'offset' parameter and the
            camera's target point will be adjusted.
        @param enabled If true, the Camera will track the SceneNode supplied as the next
            parameter (cannot be null). If false the camera will cease tracking and will
            remain in it's current orientation.
        @param target Pointer to the SceneNode which this Camera will track. Make sure you don't
            delete this SceneNode before turning off tracking (e.g. SceneManager::clearScene will
            delete it so be careful of this). Can be null if and only if the enabled param is false.
        @param offset If supplied, the camera targets this point in local space of the target node
            instead of the origin of the target node. Good for fine tuning the look at point.
        */
        void setAutoTracking( bool enabled, SceneNode *const target = 0,
                              const Vector3 &offset = Vector3::ZERO );

        /** Sets the level-of-detail factor for this Camera.
        @remarks
            This method can be used to influence the overall level of detail of the scenes
            rendered using this camera. Various elements of the scene have level-of-detail
            reductions to improve rendering speed at distance; this method allows you
            to hint to those elements that you would like to adjust the level of detail that
            they would normally use (up or down).
        @par
            The most common use for this method is to reduce the overall level of detail used
            for a secondary camera used for sub viewports like rear-view mirrors etc.
            Note that scene elements are at liberty to ignore this setting if they choose,
            this is merely a hint.
        @param factor The factor to apply to the usual level of detail calculation. Higher
            values increase the detail, so 2.0 doubles the normal detail and 0.5 halves it.
        */
        void setLodBias( Real factor = 1.0 );

        /** Returns the level-of-detail bias factor currently applied to this camera.
        @remarks
            See Camera::setLodBias for more details.
        */
        Real getLodBias() const;

        /** Set a pointer to the camera which should be used to determine
            LOD settings.
        @remarks
            Sometimes you don't want the LOD of a render to be based on the camera
            that's doing the rendering, you want it to be based on a different
            camera. A good example is when rendering shadow maps, since they will
            be viewed from the perspective of another camera. Therefore this method
            lets you associate a different camera instance to use to determine the LOD.
        @par
            To revert the camera to determining LOD based on itself, call this method with
            a pointer to itself.
        */
        virtual void setLodCamera( const Camera *lodCam );

        /** Get a pointer to the camera which should be used to determine
            LOD settings.
        @remarks
            If setLodCamera hasn't been called with a different camera, this
            method will return 'this'.
        */
        virtual const Camera *getLodCamera() const;

        /** Gets a world space ray as cast from the camera through a viewport position.
        @param screenx, screeny The x and y position at which the ray should intersect the viewport,
            in normalised screen coordinates [0,1]
        */
        Ray getCameraToViewportRay( Real screenx, Real screeny ) const;
        /** Gets a world space ray as cast from the camera through a viewport position.
        @param screenx, screeny The x and y position at which the ray should intersect the viewport,
            in normalised screen coordinates [0,1]
        @param outRay Ray instance to populate with result
        */
        void getCameraToViewportRay( Real screenx, Real screeny, Ray *outRay ) const;

        /** Gets a world-space list of planes enclosing a volume based on a viewport
            rectangle.
        @remarks
            Can be useful for populating a PlaneBoundedVolumeListSceneQuery, e.g.
            for a rubber-band selection.
        @param screenLeft, screenTop, screenRight, screenBottom The bounds of the
            on-screen rectangle, expressed in normalised screen coordinates [0,1]
        @param includeFarPlane If true, the volume is truncated by the camera far plane,
            by default it is left open-ended
        */
        PlaneBoundedVolume getCameraToViewportBoxVolume( Real screenLeft, Real screenTop,
                                                         Real screenRight, Real screenBottom,
                                                         bool includeFarPlane = false );

        /** Gets a world-space list of planes enclosing a volume based on a viewport
            rectangle.
        @remarks
            Can be useful for populating a PlaneBoundedVolumeListSceneQuery, e.g.
            for a rubber-band selection.
        @param screenLeft, screenTop, screenRight, screenBottom The bounds of the
            on-screen rectangle, expressed in normalised screen coordinates [0,1]
        @param outVolume The plane list to populate with the result
        @param includeFarPlane If true, the volume is truncated by the camera far plane,
            by default it is left open-ended
        */
        void getCameraToViewportBoxVolume( Real screenLeft, Real screenTop, Real screenRight,
                                           Real screenBottom, PlaneBoundedVolume *outVolume,
                                           bool includeFarPlane = false );

        /** Internal method for OGRE to use for LOD calculations. */
        Real _getLodBiasInverse() const;

        /** Internal method used by OGRE to update auto-tracking cameras. */
        void _autoTrack();

        /** Sets the viewing window inside of viewport.
        @remarks
            This method can be used to set a subset of the viewport as the rendering
            target.
        @param left Relative to Viewport - 0 corresponds to left edge, 1 - to right edge (default - 0).
        @param top Relative to Viewport - 0 corresponds to top edge, 1 - to bottom edge (default - 0).
        @param right Relative to Viewport - 0 corresponds to left edge, 1 - to right edge (default - 1).
        @param bottom Relative to Viewport - 0 corresponds to top edge, 1 - to bottom edge (default - 1).
        */
        virtual void setWindow( Real left, Real top, Real right, Real bottom );
        /// Cancel view window.
        virtual void resetWindow();
        /// Returns if a viewport window is being used
        virtual bool isWindowSet() const { return mWindowSet; }
        /// Gets the window clip planes, only applicable if isWindowSet == true
        const PlaneList &getWindowPlanes() const;

        /** Get the auto tracking target for this camera, if any. */
        SceneNode *getAutoTrackTarget() const { return mAutoTrackTarget; }
        /** Get the auto tracking offset for this camera, if it is auto tracking. */
        const Vector3 &getAutoTrackOffset() const { return mAutoTrackOffset; }

        /** Get the last viewport which was attached to this camera.
        @note This is not guaranteed to be the only viewport which is
            using this camera, just the last once which was created referring
            to it.
        */
        Viewport *getLastViewport() const { return mLastViewport; }
        /** Notifies this camera that a viewport is using it.*/
        void _notifyViewport( Viewport *viewport ) { mLastViewport = viewport; }

        /** If set to true a viewport that owns this frustum will be able to
            recalculate the aspect ratio whenever the frustum is resized.
        @remarks
            You should set this to true only if the frustum / camera is used by
            one viewport at the same time. Otherwise the aspect ratio for other
            viewports may be wrong.
        */
        void setAutoAspectRatio( bool autoratio );

        /** Retrieves if AutoAspectRatio is currently set or not
         */
        bool getAutoAspectRatio() const;

        /** Tells the camera to use a separate Frustum instance to perform culling.
        @remarks
            By calling this method, you can tell the camera to perform culling
            against a different frustum to it's own. This is mostly useful for
            debug cameras that allow you to show the culling behaviour of another
            camera, or a manual frustum instance.
        @param frustum Pointer to a frustum to use; this can either be a manual
            Frustum instance (which you can attach to scene nodes like any other
            MovableObject), or another camera. If you pass 0 to this method it
            reverts the camera to normal behaviour.
        */
        void setCullingFrustum( Frustum *frustum ) { mCullFrustum = frustum; }
        /** Returns the custom culling frustum in use. */
        Frustum *getCullingFrustum() const { return mCullFrustum; }

        /** Sets VR data for calculating left & right eyes
            See OpenVR sample for more info
        @param vrData
            Pointer to valid VrData
            This pointer must remain valid while the Camera is using it.
            We won't free this pointer. It is the developer's responsability to free it
            once no longer in use.
            Multiple cameras can share the same VrData pointer.
            The internal data hold by VrData can be changed withohut having to call setVrData again
        */
        void          setVrData( VrData *vrData );
        const VrData *getVrData() const { return mVrData; }

        Matrix4 getVrViewMatrix( size_t eyeIdx ) const;
        Matrix4 getVrProjectionMatrix( size_t eyeIdx ) const;

        /** Forward projects frustum rays to find forward intersection with plane.
        @remarks
            Forward projection may lead to intersections at infinity.
        */
        virtual void forwardIntersect( const Plane           &worldPlane,
                                       vector<Vector4>::type *intersect3d ) const;

        /// @copydoc Frustum::isVisible(const AxisAlignedBox&, FrustumPlane*) const
        bool isVisible( const AxisAlignedBox &bound, FrustumPlane *culledBy = 0 ) const override;
        /// @copydoc Frustum::isVisible(const Sphere&, FrustumPlane*) const
        bool isVisible( const Sphere &bound, FrustumPlane *culledBy = 0 ) const override;
        /// @copydoc Frustum::isVisible(const Vector3&, FrustumPlane*) const
        bool isVisible( const Vector3 &vert, FrustumPlane *culledBy = 0 ) const override;
        /// @copydoc Frustum::getWorldSpaceCorners
        const Vector3 *getWorldSpaceCorners() const override;
        /// @copydoc Frustum::getFrustumPlane
        const Plane &getFrustumPlane( unsigned short plane ) const override;
        /// @copydoc Frustum::projectSphere
        bool projectSphere( const Sphere &sphere, Real *left, Real *top, Real *right,
                            Real *bottom ) const override;
        /// @copydoc Frustum::getNearClipDistance
        Real getNearClipDistance() const override;
        /// @copydoc Frustum::getFarClipDistance
        Real getFarClipDistance() const override;
        /// @copydoc Frustum::getViewMatrix
        const Matrix4 &getViewMatrix() const override;
        /** Specialised version of getViewMatrix allowing caller to differentiate
            whether the custom culling frustum should be allowed or not.
        @remarks
            The default behaviour of the standard getViewMatrix is to delegate to
            the alternate culling frustum, if it is set. This is expected when
            performing CPU calculations, but the final rendering must be performed
            using the real view matrix in order to display the correct debug view.
        */
        const Matrix4 &getViewMatrix( bool ownFrustumOnly ) const;
        /** Set whether this camera should use the 'rendering distance' on
            objects to exclude distant objects from the final image. The
            default behaviour is to use it.
        @param use True to use the rendering distance, false not to.
        */
        virtual void setUseRenderingDistance( bool use ) { mUseRenderingDistance = use; }
        /** Get whether this camera should use the 'rendering distance' on
            objects to exclude distant objects from the final image.
        */
        virtual bool getUseRenderingDistance() const { return mUseRenderingDistance; }

        /** Synchronise core camera settings with another.
        @remarks
            Copies the position, orientation, clip distances, projection type,
            FOV, focal length and aspect ratio from another camera. Other settings like query flags,
            reflection etc are preserved.
        */
        virtual void synchroniseBaseSettingsWith( const Camera *cam );

        /** Get the derived position of this frustum. */
        const Vector3 &getPositionForViewUpdate() const override;
        /** Get the derived orientation of this frustum. */
        const Quaternion &getOrientationForViewUpdate() const override;

        /** @brief Sets whether to use min display size calculations.
            When active, objects that derive from MovableObject whose size on the screen is less then a
           MovableObject::mMinPixelSize will not be rendered.
        */
        void setUseMinPixelSize( bool enable ) { mUseMinPixelSize = enable; }
        /** Returns whether to use min display size calculations
        @see Camera::setUseMinDisplaySize
        */
        bool getUseMinPixelSize() const { return mUseMinPixelSize; }

        void _setNeedsDepthClamp( bool bNeedsDepthClamp );
        bool getNeedsDepthClamp() const { return mNeedsDepthClamp; }

        /** Returns an estimated ratio between a pixel and the display area it represents.
            For orthographic cameras this function returns the amount of meters covered by
            a single pixel along the vertical axis. For perspective cameras the value
            returned is the amount of meters covered by a single pixel per meter distance
            from the camera.
        @note
            This parameter is calculated just before the camera is rendered
        @note
            This parameter is used in min display size calculations.
        */
        Real getPixelDisplayRatio() const { return mPixelDisplayRatio; }

        void  _setConstantBiasScale( const float bias ) { mConstantBiasScale = bias; }
        float _getConstantBiasScale() const { return mConstantBiasScale; }
    };
    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif  // __Camera_H__
