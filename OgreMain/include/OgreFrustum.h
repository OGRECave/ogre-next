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
#ifndef __Frustum_H__
#define __Frustum_H__

#include "OgrePrerequisites.h"

#include "OgreAxisAlignedBox.h"
#include "OgreMovableObject.h"
#include "OgreRenderable.h"
#include "OgreVertexIndexData.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Math
     *  @{
     */

    /** Specifies perspective (realistic) or orthographic (architectural) projection.
     */
    enum ProjectionType
    {
        PT_ORTHOGRAPHIC,
        PT_PERSPECTIVE
    };

    /** Worldspace clipping planes.
     */
    enum FrustumPlane
    {
        FRUSTUM_PLANE_NEAR = 0,
        FRUSTUM_PLANE_FAR = 1,
        FRUSTUM_PLANE_LEFT = 2,
        FRUSTUM_PLANE_RIGHT = 3,
        FRUSTUM_PLANE_TOP = 4,
        FRUSTUM_PLANE_BOTTOM = 5
    };

    /** Specify how the frustrum extents are represented.
     */
    enum FrustrumExtentsType
    {
        /// Represent extents as positions on the near clip plane
        FET_PROJ_PLANE_POS,
        /// Represet extents as tan of the half angles in radians (ie FoV)
        FET_TAN_HALF_ANGLES
    };

    /** A frustum represents a pyramid, capped at the near and far end which is
        used to represent either a visible area or a projection area. Can be used
        for a number of applications.
    */
    class _OgreExport Frustum : public MovableObject, public Renderable
    {
    protected:
        /// Orthographic or perspective?
        ProjectionType mProjType;

        /// y-direction field-of-view (default 45)
        Radian mFOVy;
        /// Far clip distance - default 10000
        Real mFarDist;
        /// Near clip distance - default 100
        Real mNearDist;
        /// x/y viewport ratio - default 1.3333
        Real mAspect;
        /// Ortho height size (world units)
        Real mOrthoHeight;
        /// Off-axis frustum center offset - default (0.0, 0.0)
        Vector2 mFrustumOffset;
        /// Focal length of frustum (for stereo rendering, defaults to 1.0)
        Real mFocalLength;

        /// The 6 main clipping planes
        mutable Plane mFrustumPlanes[6];

        /// Stored versions of parent orientation / position
        mutable Quaternion mLastParentOrientation;
        mutable Vector3    mLastParentPosition;

        /// Pre-calced standard projection matrix but with render system depth range
        mutable Matrix4 mProjMatrixRSDepth;
        /// Pre-calced standard projection matrix
        mutable Matrix4 mProjMatrix;
        /// Pre-calced view matrix
        mutable Matrix4 mViewMatrix;
        /// Something's changed in the frustum shape?
        mutable bool mRecalcFrustum;
        /// Something re the view pos has changed
        mutable bool mRecalcView;
        /// Something re the frustum planes has changed
        mutable bool mRecalcFrustumPlanes;
        /// Something re the world space corners has changed
        mutable bool mRecalcWorldSpaceCorners;
        /// Something re the vertex data has changed
        mutable bool mRecalcVertexData;
        /// Are we using a custom view matrix?
        bool mCustomViewMatrix;
        /// Are we using a custom projection matrix?
        bool mCustomProjMatrix;
        /// Have the frustum extents been manually set?
        bool mFrustumExtentsManuallySet;
        /// Whether to treat frustum extents as tangents of the angles between the viewing
        /// vector and the edges of the field of view or as positions on the projection plane
        FrustrumExtentsType mFrustrumExtentsType;
        /// Frustum extents
        mutable Real mLeft, mRight, mTop, mBottom;
        /// Frustum orientation mode
        mutable OrientationMode mOrientationMode;

        // Internal functions for calcs
        virtual void calcProjectionParameters( Real &left, Real &right, Real &bottom, Real &top ) const;
        /// Update frustum if out of date
        virtual void updateFrustum() const;
        /// Update view if out of date
        virtual void updateView() const;
        /// Implementation of updateFrustum (called if out of date)
        virtual void updateFrustumImpl() const;
        /// Implementation of updateView (called if out of date)
        virtual void updateViewImpl() const;
        virtual void updateFrustumPlanes() const;
        /// Implementation of updateFrustumPlanes (called if out of date)
        virtual void updateFrustumPlanesImpl() const;
        virtual void updateWorldSpaceCorners() const;
        /// Implementation of updateWorldSpaceCorners (called if out of date)
        virtual void updateWorldSpaceCornersImpl() const;
        virtual void updateVertexData() const;
        virtual bool isViewOutOfDate() const;
        virtual bool isFrustumOutOfDate() const;
        /// Signal to update frustum information.
        virtual void invalidateFrustum() const;
        /// Signal to update view information.
        virtual void invalidateView() const;

        /// Shared class-level name for Movable type
        static String msMovableType;

        mutable AxisAlignedBox mBoundingBox;
        mutable v1::VertexData mVertexData;  // TODO: Remove this
        mutable Vector3        mWorldSpaceCorners[8];

        /// Is this frustum to act as a reflection of itself?
        bool mReflect;
        /// Derived reflection matrix
        mutable Matrix4 mReflectMatrix;
        /// Fixed reflection plane
        mutable Plane mReflectPlane;
        /// Pointer to a reflection plane (automatically updated)
        const MovablePlane *mLinkedReflectPlane;
        /// Record of the last world-space reflection plane info used
        mutable Plane mLastLinkedReflectionPlane;

        /// Is this frustum using an oblique depth projection?
        bool mObliqueDepthProjection;
        /// Fixed oblique projection plane
        mutable Plane mObliqueProjPlane;
        /// Pointer to oblique projection plane (automatically updated)
        const MovablePlane *mLinkedObliqueProjPlane;
        /// Record of the last world-space oblique depth projection plane info used
        mutable Plane mLastLinkedObliqueProjPlane;

    public:
        /// Named constructor
        Frustum( IdType id, ObjectMemoryManager *objectMemoryManager );

        ~Frustum() override;
        /** Sets the Y-dimension Field Of View (FOV) of the frustum.
        @remarks
            Field Of View (FOV) is the angle made between the frustum's position, and the edges
            of the 'screen' onto which the scene is projected. High values (90+ degrees) result in a
        wide-angle, fish-eye kind of view, low values (30- degrees) in a stretched, telescopic kind of
        view. Typical values are between 45 and 60 degrees.
        @par
            This value represents the VERTICAL field-of-view. The horizontal field of view is calculated
        from this depending on the dimensions of the viewport (they will only be the same if the viewport
        is square).
        @note
            Setting the FOV overrides the value supplied for frustum::setNearClipPlane.
         */
        virtual void setFOVy( const Radian &fovy );

        /** Retrieves the frustums Y-dimension Field Of View (FOV).
         */
        virtual const Radian &getFOVy() const;

        /// Returns the terms projectionA and projectionB in .x and .y respectively, which can
        /// be used to reconstruct linear depth from a Z buffer with the following formula:
        ///     linearDepth = projectionParams.y / (fDepth - projectionParams.x);
        Vector2 getProjectionParamsAB() const;

        /** Sets the position of the near clipping plane.
        @remarks
            The position of the near clipping plane is the distance from the frustums position to the
        screen on which the world is projected. The near plane distance, combined with the field-of-view
        and the aspect ratio, determines the size of the viewport through which the world is viewed (in
        world co-ordinates). Note that this world viewport is different to a screen viewport, which has
        it's dimensions expressed in pixels. The frustums viewport should have the same aspect ratio as
        the screen viewport it renders into to avoid distortion.
        @param nearDist
            The distance to the near clipping plane from the frustum in world coordinates.
         */
        virtual void setNearClipDistance( Real nearDist );

        /** Sets the position of the near clipping plane.
         */
        virtual Real getNearClipDistance() const;

        /** Sets the distance to the far clipping plane.
        @remarks
            The view frustum is a pyramid created from the frustum position and the edges of the
        viewport. This method sets the distance for the far end of that pyramid. Different applications
        need different values: e.g. a flight sim needs a much further far clipping plane than a
        first-person shooter. An important point here is that the larger the ratio between near and far
        clipping planes, the lower the accuracy of the Z-buffer used to depth-cue pixels. This is because
        the Z-range is limited to the size of the Z buffer (16 or 32-bit) and the max values must be
        spread over the gap between near and far clip planes. As it happens, you can affect the accuracy
        far more by altering the near distance rather than the far distance, but keep this in mind.
        @param farDist
            The distance to the far clipping plane from the frustum in
            world coordinates.If you specify 0, this means an infinite view
            distance which is useful especially when projecting shadows; but
            be careful not to use a near distance too close.
        */
        virtual void setFarClipDistance( Real farDist );

        /** Retrieves the distance from the frustum to the far clipping plane.
         */
        virtual Real getFarClipDistance() const;

        /** Sets the aspect ratio for the frustum viewport.
        @remarks
            The ratio between the x and y dimensions of the rectangular area visible through the frustum
            is known as aspect ratio: aspect = width / height .
        @par
            The default for most fullscreen windows is 1.3333 - this is also assumed by Ogre unless you
            use this method to state otherwise.
        */
        virtual void setAspectRatio( Real ratio );

        /** Retrieves the current aspect ratio.
         */
        virtual Real getAspectRatio() const;

        /** Sets frustum offsets, used in stereo rendering.
        @remarks
            You can set both horizontal and vertical plane offsets of "eye"; in
            stereo rendering frustum is moved in horizontal plane. To be able to
            render from two "eyes" you'll need two cameras rendering on two
            RenderTargets.
        @par
            The frustum offsets is in world coordinates, and default to (0, 0) - no offsets.
        @param offset
            The horizontal and vertical plane offsets.
        */
        virtual void setFrustumOffset( const Vector2 &offset );

        /** Sets frustum offsets, used in stereo rendering.
        @remarks
            You can set both horizontal and vertical plane offsets of "eye"; in
            stereo rendering frustum is moved in horizontal plane. To be able to
            render from two "eyes" you'll need two cameras rendering on two
            RenderTargets.
        @par
            The frustum offsets is in world coordinates, and default to (0, 0) - no offsets.
        @param horizontal
            The horizontal plane offset.
        @param vertical
            The vertical plane offset.
        */
        virtual void setFrustumOffset( Real horizontal = 0.0, Real vertical = 0.0 );

        /** Retrieves the frustum offsets.
         */
        virtual const Vector2 &getFrustumOffset() const;

        /** Sets frustum focal length (used in stereo rendering).
        @param focalLength
            The distance to the focal plane from the frustum in world coordinates.
        */
        virtual void setFocalLength( Real focalLength = 1.0 );

        /** Returns focal length of frustum.
         */
        virtual Real getFocalLength() const;

        /** Manually set the extents of the frustum.
        @param left, right, top, bottom The position where the side clip planes intersect
            the near clip plane, in eye space OR the tangent of the half angles from the eye to the edges
            of the near clip plane
        @param frustrumExtentsType
            Specifies how the extents are represented. Be default they are the positions on near clip
        plane. However if this is a scene camera it is recommended that you actually use the tangent of
        the half angles. ie Half the FoV angles. This is because Ogre uses this camera setup to calculate
        custom clipping planes for PSSM algorithms, and Forward+ algorithms, but alter the near and far
        clip distances.
            @par
            If you supply extents as positions on the projection plane the near clip distance has already
        been calculated as part of the supplied extents, however if they are tangent of the half angles
        Ogre can apply different near clip distances dynamically.
        */
        virtual void setFrustumExtents( Real left, Real right, Real top, Real bottom,
                                        FrustrumExtentsType frustrumExtentsType = FET_PROJ_PLANE_POS );

        bool getFrustumExtentsManuallySet() const;

        /** Reset the frustum extents to be automatically derived from other params. */
        virtual void resetFrustumExtents();

        /** Get the extents of the frustum in view space.
        @param left, right, top, bottom The position where the side clip planes intersect
            the near clip plane, in eye space OR the tangent of the half angles from the eye to the edges
            of the near clip plane
        @param frustrumExtentsType
            Specifies in what format the extents are returned. See OgreFrustrum::setFrustumExtents for
        more information
        */
        virtual void getFrustumExtents(
            Real &outleft, Real &outright, Real &outtop, Real &outbottom,
            FrustrumExtentsType frustrumExtentsType = FET_PROJ_PLANE_POS ) const;

        /** Gets the depth-adjusted projection matrix for the current rendersystem,
            but one which still conforms to right-hand rules.
        @remarks
            This differs from the rendering-API dependent getProjectionMatrix
            in that it always returns a right-handed projection matrix result
            no matter what rendering API is being used - this is required for
            vertex and fragment programs for example. However, the resulting depth
            range may still vary between render systems since D3D uses [0,1] and
            GL uses [-1,1], and the range must be kept the same between programmable
            and fixed-function pipelines.
        */
        virtual const Matrix4 &getProjectionMatrixWithRSDepth() const;
        /** Gets the normal projection matrix for this frustum, ie the
            projection matrix which conforms to standard right-handed rules and
            uses depth range [-1,+1].
        @remarks
            This differs from the rendering-API dependent getProjectionMatrixRS
            in that it always returns a right-handed projection matrix with depth
            range [-1,+1], result no matter what rendering API is being used - this
            is required for some uniform algebra for example.
        */
        virtual const Matrix4 &getProjectionMatrix() const;

        /** Gets the view matrix for this frustum. Mainly for use by OGRE internally.
         */
        virtual const Matrix4 &getViewMatrix() const;

        /** Calculate a view matrix for this frustum, relative to a potentially dynamic point.
            Mainly for use by OGRE internally when using camera-relative rendering
            for frustums that are not the centre (e.g. texture projection)
        */
        virtual void calcViewMatrixRelative( const Vector3 &relPos, Matrix4 &matToUpdate ) const;

        /** Set whether to use a custom view matrix on this frustum.
        @remarks
            This is an advanced method which allows you to manually set
            the view matrix on this frustum, rather than having it calculate
            itself based on it's position and orientation.
        @note
            After enabling a custom view matrix, the frustum will no longer
            update on its own based on position / orientation changes. You
            are completely responsible for keeping the view matrix up to date.
            The custom matrix will be returned from getViewMatrix.
        @param enable If @c true, the custom view matrix passed as the second
            parameter will be used in preference to an auto calculated one. If
            false, the frustum will revert to auto calculating the view matrix.
        @param viewMatrix The custom view matrix to use, the matrix must be an
            affine matrix.
        @see Frustum::setCustomProjectionMatrix, Matrix4::isAffine
        */
        virtual void setCustomViewMatrix( bool enable, const Matrix4 &viewMatrix = Matrix4::IDENTITY );
        /// Returns whether a custom view matrix is in use
        virtual bool isCustomViewMatrixEnabled() const { return mCustomViewMatrix; }

        /** Set whether to use a custom projection matrix on this frustum.
        @remarks
            This is an advanced method which allows you to manually set
            the projection matrix on this frustum, rather than having it
            calculate itself based on the clipping distances or the field
            of view.

            See Frustum::setCustomViewMatrix
        @note
            After enabling a custom projection matrix, the frustum will no
            longer update on its own based on field of view and near / far
            distance changes. You are completely responsible for keeping the
            projection matrix up to date if those values change. The custom
            matrix will be returned from getProjectionMatrix and derivative
            functions.
            Even when using a custom projection matrix, you should still
            call the methods that update the ner / far distances and field of
            view.
        @param enable
            If @c true, the custom projection matrix passed as the
            second parameter will be used in preference to an auto calculated
            one. If @c false, the frustum will revert to auto calculating the
            projection matrix.
        @param projectionMatrix
            The custom view matrix to use.
        @param alternateDepthRange
            Set this to true if the input projectionMatrix has the depth in range [0; 1]
            instead of the range [-1; 1] that Ogre expects.

            Projection matrices obtained from Oculus SDK's need to set this to true.

            Note that we will internally convert the projection matrix by performing the
            range conversion, so don't expect Frustum::getProjectionMatrix to return you
            exactly the same projection matrix you passed to us!
        */
        virtual void setCustomProjectionMatrix( bool           enable,
                                                const Matrix4 &projectionMatrix = Matrix4::IDENTITY,
                                                bool           alternateDepthRange = false );
        /// Returns whether a custom projection matrix is in use
        virtual bool isCustomProjectionMatrixEnabled() const { return mCustomProjMatrix; }

        /** Retrieves the clipping planes of the frustum (world space).
        @remarks
            The clipping planes are ordered as declared in enumerate constants FrustumPlane.
        */
        const Plane *getFrustumPlanes() const;

        /// Returns the frustum planes, doesn't check if dirty.
        const Plane   *_getCachedFrustumPlanes() const { return mFrustumPlanes; }
        const Vector3 *_getCachedWorldSpaceCorners() const { return mWorldSpaceCorners; }

        /** Retrieves a specified plane of the frustum (world space).
        @remarks
            Gets a reference to one of the planes which make up the frustum frustum,
            e.g. for clipping purposes.
        */
        virtual const Plane &getFrustumPlane( unsigned short plane ) const;

        /** Tests whether the given container is visible in the Frustum.
        @param bound
            Bounding box to be checked (world space).
        @param culledBy
            Optional pointer to an int which will be filled by the plane number which culled
            the box if the result was @c false;
        @return
            If the box was visible, @c true is returned.
        @par
            Otherwise, @c false is returned.
        */
        virtual bool isVisible( const AxisAlignedBox &bound, FrustumPlane *culledBy = 0 ) const;

        /** Tests whether the given container is visible in the Frustum.
        @param bound
            Bounding sphere to be checked (world space).
        @param culledBy
            Optional pointer to an int which will be filled by the plane number which culled
            the box if the result was @c false;
        @return
            If the sphere was visible, @c true is returned.
        @par
            Otherwise, @c false is returned.
        */
        virtual bool isVisible( const Sphere &bound, FrustumPlane *culledBy = 0 ) const;

        /** Tests whether the given vertex is visible in the Frustum.
        @param vert
            Vertex to be checked (world space).
        @param culledBy
            Optional pointer to an int which will be filled by the plane number which culled
            the box if the result was @c false;
        @return
            If the sphere was visible, @c true is returned.
        @par
            Otherwise, @c false is returned.
        */
        virtual bool isVisible( const Vector3 &vert, FrustumPlane *culledBy = 0 ) const;

        /** Overridden from MovableObject */
        const AxisAlignedBox &getBoundingBox() const;

        /** Overridden from MovableObject */
        const String &getMovableType() const override;

        /** Overridden from Renderable */
        void getRenderOperation( v1::RenderOperation &op, bool casterPass ) override;

        /** Overridden from Renderable */
        void getWorldTransforms( Matrix4 *xform ) const override;

        /** Overridden from Renderable */
        Real getSquaredViewDepth( const Camera *cam ) const;

        /** Overridden from Renderable */
        const LightList &getLights() const override;

        void getCustomWorldSpaceCorners(
            ArrayVector3 outCorners[( 8 + ARRAY_PACKED_REALS - 1 ) / ARRAY_PACKED_REALS],
            Real         customFarPlane ) const;

        /** Gets the world space corners of the frustum.
        @remarks
            The corners are ordered as follows: top-right near,
            top-left near, bottom-left near, bottom-right near,
            top-right far, top-left far, bottom-left far, bottom-right far.
        */
        virtual const Vector3 *getWorldSpaceCorners() const;

        /** Sets the type of projection to use (orthographic or perspective). Default is perspective.
         */
        virtual void setProjectionType( ProjectionType pt );

        /** Retrieves info on the type of projection used (orthographic or perspective).
         */
        virtual ProjectionType getProjectionType() const;

        /** Sets the orthographic window settings, for use with orthographic rendering only.
        @note Calling this method will recalculate the aspect ratio, use
            setOrthoWindowHeight or setOrthoWindowWidth alone if you wish to
            preserve the aspect ratio but just fit one or other dimension to a
            particular size.
        @param w
            The width of the view window in world units.
        @param h
            The height of the view window in world units.
        */
        virtual void setOrthoWindow( Real w, Real h );
        /** Sets the orthographic window height, for use with orthographic rendering only.
        @note The width of the window will be calculated from the aspect ratio.
        @param h
            The height of the view window in world units.
        */
        virtual void setOrthoWindowHeight( Real h );
        /** Sets the orthographic window width, for use with orthographic rendering only.
        @note The height of the window will be calculated from the aspect ratio.
        @param w
            The width of the view window in world units.
        */
        virtual void setOrthoWindowWidth( Real w );
        /** Gets the orthographic window height, for use with orthographic rendering only.
         */
        virtual Real getOrthoWindowHeight() const;
        /** Gets the orthographic window width, for use with orthographic rendering only.
        @note This is calculated from the orthographic height and the aspect ratio
        */
        virtual Real getOrthoWindowWidth() const;

        /** Modifies this frustum so it always renders from the reflection of itself through the
            plane specified.
        @remarks
            This is obviously useful for performing planar reflections.
        */
        virtual void enableReflection( const Plane &p );
        /** Modifies this frustum so it always renders from the reflection of itself through the
            plane specified. Note that this version of the method links to a plane
            so that changes to it are picked up automatically. It is important that
            this plane continues to exist whilst this object does; do not destroy
            the plane before the frustum.
        @remarks
            This is obviously useful for performing planar reflections.
        */
        virtual void enableReflection( const MovablePlane *p );

        /** Disables reflection modification previously turned on with enableReflection */
        virtual void disableReflection();

        /// Returns whether this frustum is being reflected
        virtual bool isReflected() const { return mReflect; }
        /// Returns the reflection matrix of the frustum if appropriate
        virtual const Matrix4 &getReflectionMatrix() const { return mReflectMatrix; }
        /// Returns the reflection plane of the frustum if appropriate
        virtual const Plane &getReflectionPlane() const { return mReflectPlane; }

        /** Project a sphere onto the near plane and get the bounding rectangle.
        @param sphere The world-space sphere to project.
        @param left
            Pointers to destination values, these will be completed with
            the normalised device coordinates (in the range {-1,1}).
        @param top
            Pointers to destination values, these will be completed with
            the normalised device coordinates (in the range {-1,1}).
        @param right
            Pointers to destination values, these will be completed with
            the normalised device coordinates (in the range {-1,1}).
        @param bottom
            Pointers to destination values, these will be completed with
            the normalised device coordinates (in the range {-1,1}).
        @return @c true if the sphere was projected to a subset of the near plane,
            @c false if the entire near plane was contained.
        */
        virtual bool projectSphere( const Sphere &sphere, Real *left, Real *top, Real *right,
                                    Real *bottom ) const;

        /** Links the frustum to a custom near clip plane, which can be used
            to clip geometry in a custom manner without using user clip planes.
        @remarks
            There are several applications for clipping a scene arbitrarily by
            a single plane; the most common is when rendering a reflection to
            a texture, and you only want to render geometry that is above the
            water plane (to do otherwise results in artefacts). Whilst it is
            possible to use user clip planes, they are not supported on all
            cards, and sometimes are not hardware accelerated when they are
            available. Instead, where a single clip plane is involved, this
            technique uses a 'fudging' of the near clip plane, which is
            available and fast on all hardware, to perform as the arbitrary
            clip plane. This does change the shape of the frustum, leading
            to some depth buffer loss of precision, but for many of the uses of
            this technique that is not an issue.
        @par
            This version of the method links to a plane, rather than requiring
            a by-value plane definition, and therefore you can
            make changes to the plane (e.g. by moving / rotating the node it is
            attached to) and they will automatically affect this object.
        @note This technique only works for perspective projection.
        @param plane
            The plane to link to to perform the clipping. This plane
            must continue to exist while the camera is linked to it; do not
            destroy it before the frustum.
        */
        virtual void enableCustomNearClipPlane( const MovablePlane *plane );
        /** Links the frustum to a custom near clip plane, which can be used
            to clip geometry in a custom manner without using user clip planes.
        @remarks
            There are several applications for clipping a scene arbitrarily by
            a single plane; the most common is when rendering a reflection to
            a texture, and you only want to render geometry that is above the
            water plane (to do otherwise results in artefacts). Whilst it is
            possible to use user clip planes, they are not supported on all
            cards, and sometimes are not hardware accelerated when they are
            available. Instead, where a single clip plane is involved, this
            technique uses a 'fudging' of the near clip plane, which is
            available and fast on all hardware, to perform as the arbitrary
            clip plane. This does change the shape of the frustum, leading
            to some depth buffer loss of precision, but for many of the uses of
            this technique that is not an issue.
        @note This technique only works for perspective projection.
        @param plane
            The plane to link to to perform the clipping. This plane
            must continue to exist while the camera is linked to it; do not
            destroy it before the frustum.
        */
        virtual void enableCustomNearClipPlane( const Plane &plane );
        /** Disables any custom near clip plane. */
        virtual void disableCustomNearClipPlane();
        /** Is a custom near clip plane in use? */
        virtual bool isCustomNearClipPlaneEnabled() const { return mObliqueDepthProjection; }

        /// Small constant used to reduce far plane projection to avoid inaccuracies
        static const Real INFINITE_FAR_PLANE_ADJUST;

        /** Get the derived position of this frustum. */
        virtual const Vector3 &getPositionForViewUpdate() const;
        /** Get the derived orientation of this frustum. */
        virtual const Quaternion &getOrientationForViewUpdate() const;

        /** Gets a world-space list of planes enclosing the frustum.
         */
        PlaneBoundedVolume getPlaneBoundedVolume();
        /** Set the orientation mode of the frustum. Default is OR_DEGREE_0
        @remarks
            Setting the orientation of a frustum is only supported on
            iOS at this time.  An exception is thrown on other platforms.
        */
        void setOrientationMode( OrientationMode orientationMode );

        /** Get the orientation mode of the frustum.
        @remarks
            Getting the orientation of a frustum is only supported on
            iOS at this time.  An exception is thrown on other platforms.
        */
        OrientationMode getOrientationMode() const;
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif  // __Frustum_H__
