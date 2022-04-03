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

#ifndef __Animation_H__
#define __Animation_H__

#include "OgrePrerequisites.h"

#include "OgreAnimationState.h"
#include "OgreAnimationTrack.h"
#include "OgreIteratorWrappers.h"

#include "ogrestd/set.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    namespace v1
    {
        /** \addtogroup Core
         *  @{
         */

        /** \addtogroup Animation
         *  @{
         */

        class Animation;

        /** An animation container interface, which allows generic access to sibling animations.
         @remarks
            Because Animation instances can be held by different kinds of classes, and
            there are sometimes instances when you need to reference other Animation
            instances within the same container, this class allows generic access to
            named animations within that container, whatever it may be.
        */
        class _OgreExport AnimationContainer
        {
        public:
            virtual ~AnimationContainer();

            /** Gets the number of animations in this container. */
            virtual unsigned short getNumAnimations() const = 0;

            /** Retrieve an animation by index.  */
            virtual Animation *getAnimation( unsigned short index ) const = 0;

            /** Retrieve an animation by name. */
            virtual Animation *getAnimation( const String &name ) const = 0;

            /** Create a new animation with a given length owned by this container. */
            virtual Animation *createAnimation( const String &name, Real length ) = 0;

            /** Returns whether this object contains the named animation. */
            virtual bool hasAnimation( const String &name ) const = 0;

            /** Removes an Animation from this container. */
            virtual void removeAnimation( const String &name ) = 0;
        };
        /** An animation sequence.
        @remarks
            This class defines the interface for a sequence of animation, whether that
            be animation of a mesh, a path along a spline, or possibly more than one
            type of animation in one. An animation is made up of many 'tracks', which are
            the more specific types of animation.
        @par
            You should not create these animations directly. They will be created via a parent
            object which owns the animation, e.g. Skeleton.
        */
        class _OgreExport Animation : public OgreAllocatedObj
        {
        public:
            /** The types of animation interpolation available. */
            enum InterpolationMode
            {
                /** Values are interpolated along straight lines. */
                IM_LINEAR,
                /** Values are interpolated along a spline, resulting in smoother changes in direction.
                 */
                IM_SPLINE
            };

            /** The types of rotational interpolation available. */
            enum RotationInterpolationMode
            {
                /** Values are interpolated linearly. This is faster but does not
                    necessarily give a completely accurate result.
                */
                RIM_LINEAR,
                /** Values are interpolated spherically. This is more accurate but
                    has a higher cost.
                */
                RIM_SPHERICAL
            };
            /** You should not use this constructor directly, use the parent object such as Skeleton
            instead.
            @param name The name of the animation, should be unique within it's parent (e.g. Skeleton)
            @param length The length of the animation in seconds.
            */
            Animation( const String &name, Real length );
            virtual ~Animation();

            /** Gets the name of this animation. */
            const String &getName() const;

            /** Gets the total length of the animation. */
            Real getLength() const;

            /** Sets the length of the animation.
            @note Changing the length of an animation may invalidate existing AnimationState
                instances which will need to be recreated.
            */
            void setLength( Real len );

            /** Creates a NodeAnimationTrack for animating a OldNode.
            @param handle Handle to give the track, used for accessing the track later.
                Must be unique within this Animation.
            */
            NodeAnimationTrack *createNodeTrack();

            /** Creates a OldNodeAnimationTrack for animating a OldOldNode.
            @param handle Handle to give the track, used for accessing the track later.
                Must be unique within this Animation.
            */
            OldNodeAnimationTrack *createOldNodeTrack( unsigned short handle );

            /** Creates a NumericAnimationTrack for animating any numeric value.
            @param handle Handle to give the track, used for accessing the track later.
                Must be unique within this Animation.
            */
            NumericAnimationTrack *createNumericTrack( unsigned short handle );

            /** Creates a VertexAnimationTrack for animating vertex position data.
            @param handle Handle to give the track, used for accessing the track later.
                Must be unique within this Animation, and is used to identify the target. For example
                when applied to a Mesh, the handle must reference the index of the geometry being
                modified; 0 for the shared geometry, and 1+ for SubMesh geometry with the same index-1.
            @param animType Either morph or pose animation,
            */
            VertexAnimationTrack *createVertexTrack( unsigned short      handle,
                                                     VertexAnimationType animType );

            /** Creates a new AnimationTrack automatically associated with a OldNode.
            @remarks
                This method creates a standard AnimationTrack, but also associates it with a
                target Node which will receive all keyframe effects.
            @param handle Numeric handle to give the track, used for accessing the track later.
                Must be unique within this Animation.
            @param node A pointer to the OldNode object which will be affected by this track
            */
            NodeAnimationTrack *createNodeTrack( Node *node );

            /** Creates a new AnimationTrack automatically associated with a OldNode.
            @remarks
                This method creates a standard AnimationTrack, but also associates it with a
                target OldNode which will receive all keyframe effects.
            @param handle Numeric handle to give the track, used for accessing the track later.
                Must be unique within this Animation.
            @param node A pointer to the OldNode object which will be affected by this track
            */
            OldNodeAnimationTrack *createOldNodeTrack( unsigned short handle, OldNode *node );

            /** Creates a NumericAnimationTrack and associates it with an animable.
            @param handle Handle to give the track, used for accessing the track later.
            @param anim Animable object link
                Must be unique within this Animation.
            */
            NumericAnimationTrack *createNumericTrack( unsigned short          handle,
                                                       const AnimableValuePtr &anim );

            /** Creates a VertexAnimationTrack and associates it with VertexData.
            @param handle Handle to give the track, used for accessing the track later.
            @param data VertexData object link
            @param animType The animation type
                Must be unique within this Animation.
            */
            VertexAnimationTrack *createVertexTrack( unsigned short handle, VertexData *data,
                                                     VertexAnimationType animType );

            /** Gets the number of NodeAnimationTrack objects contained in this animation. */
            size_t getNumNodeTracks() const;

            /** Gets a node track by it's handle. */
            NodeAnimationTrack *getNodeTrack( size_t handle ) const;

            /** Gets the number of OldNodeAnimationTrack objects contained in this animation. */
            size_t getNumOldNodeTracks() const;

            /** Gets a node track by it's handle. */
            OldNodeAnimationTrack *getOldNodeTrack( unsigned short handle ) const;

            /** Does a track exist with the given handle? */
            bool hasOldNodeTrack( unsigned short handle ) const;

            /** Gets the number of NumericAnimationTrack objects contained in this animation. */
            unsigned short getNumNumericTracks() const;

            /** Gets a numeric track by it's handle. */
            NumericAnimationTrack *getNumericTrack( unsigned short handle ) const;

            /** Does a track exist with the given handle? */
            bool hasNumericTrack( unsigned short handle ) const;

            /** Gets the number of VertexAnimationTrack objects contained in this animation. */
            unsigned short getNumVertexTracks() const;

            /** Gets a Vertex track by it's handle. */
            VertexAnimationTrack *getVertexTrack( unsigned short handle ) const;

            /** Does a track exist with the given handle? */
            bool hasVertexTrack( unsigned short handle ) const;

            /** Destroys the node track with the given handle. */
            void destroyNodeTrack( unsigned short handle );

            /** Destroys the node track with the given handle. */
            void destroyOldNodeTrack( unsigned short handle );

            /** Destroys the numeric track with the given handle. */
            void destroyNumericTrack( unsigned short handle );

            /** Destroys the Vertex track with the given handle. */
            void destroyVertexTrack( unsigned short handle );

            /** Removes and destroys all tracks making up this animation. */
            void destroyAllTracks();

            /** Removes and destroys all tracks making up this animation. */
            void destroyAllNodeTracks();
            void destroyAllOldNodeTracks();
            /** Removes and destroys all tracks making up this animation. */
            void destroyAllNumericTracks();
            /** Removes and destroys all tracks making up this animation. */
            void destroyAllVertexTracks();

            /** Applies an animation given a specific time point and weight.
            @remarks
                Where you have associated animation tracks with objects, you can easily apply
                an animation to those objects by calling this method.
            @param timePos The time position in the animation to apply.
            @param weight The influence to give to this track, 1.0 for full influence, less to blend with
              other animations.
            @param scale The scale to apply to translations and scalings, useful for
                adapting an animation to a different size target.
            */
            void apply( Real timePos, Real weight = 1.0, Real scale = 1.0f );

            /** Applies all node tracks given a specific time point and weight to the specified node.
            @remarks
                It does not consider the actual node tracks are attached to.
                As such, it resembles the apply method for a given skeleton (see below).
            @param timePos The time position in the animation to apply.
            @param weight The influence to give to this track, 1.0 for full influence, less to blend with
              other animations.
            @param scale The scale to apply to translations and scalings, useful for
                adapting an animation to a different size target.
            */
            void applyToNode( OldNode *node, Real timePos, Real weight = 1.0, Real scale = 1.0f );

            /** Applies all node tracks given a specific time point and weight to a given skeleton.
            @remarks
                Where you have associated animation tracks with OldNode objects, you can easily apply
                an animation to those nodes by calling this method.
            @param timePos The time position in the animation to apply.
            @param weight The influence to give to this track, 1.0 for full influence, less to blend with
                other animations.
            @param scale The scale to apply to translations and scalings, useful for
                adapting an animation to a different size target.
            */
            void apply( Skeleton *skeleton, Real timePos, Real weight = 1.0, Real scale = 1.0f );

            /** Applies all node tracks given a specific time point and weight to a given skeleton.
            @remarks
                Where you have associated animation tracks with OldNode objects, you can easily apply
                an animation to those nodes by calling this method.
            @param timePos The time position in the animation to apply.
            @param weight The influence to give to this track, 1.0 for full influence, less to blend with
                other animations.
            @param blendMask The influence array defining additional per bone weights. These will
                be modulated with the weight factor.
            @param scale The scale to apply to translations and scalings, useful for
                adapting an animation to a different size target.
            */
            void apply( Skeleton *skeleton, Real timePos, float weight,
                        const AnimationState::BoneBlendMask *blendMask, Real scale );

            /** Applies all vertex tracks given a specific time point and weight to a given entity.
            @param entity The Entity to which this animation should be applied
            @param timePos The time position in the animation to apply.
            @param weight The weight at which the animation should be applied
                (only affects pose animation)
            @param software Whether to populate the software morph vertex data
            @param hardware Whether to populate the hardware morph vertex data
            */
            void apply( Entity *entity, Real timePos, Real weight, bool software, bool hardware );

            /** Applies all numeric tracks given a specific time point and weight to the specified
            animable value.
            @remarks
                It does not applies to actual attached animable values but rather uses all tracks for a
            single animable value.
            @param timePos The time position in the animation to apply.
            @param weight The influence to give to this track, 1.0 for full influence, less to blend with
              other animations.
            @param scale The scale to apply to translations and scalings, useful for
                adapting an animation to a different size target.
            */
            void applyToAnimable( const AnimableValuePtr &anim, Real timePos, Real weight = 1.0,
                                  Real scale = 1.0f );

            /** Applies all vertex tracks given a specific time point and weight to the specified vertex
            data.
            @remarks
                It does not apply to the actual attached vertex data but rather uses all tracks for a
            given vertex data.
            @param timePos The time position in the animation to apply.
            @param weight The influence to give to this track, 1.0 for full influence, less to blend with
              other animations.
            */
            void applyToVertexData( VertexData *data, Real timePos, Real weight = 1.0 );

            /** Tells the animation how to interpolate between keyframes.
            @remarks
                By default, animations normally interpolate linearly between keyframes. This is
                fast, but when animations include quick changes in direction it can look a little
                unnatural because directions change instantly at keyframes. An alternative is to
                tell the animation to interpolate along a spline, which is more expensive in terms
                of calculation time, but looks smoother because major changes in direction are
                distributed around the keyframes rather than just at the keyframe.
            @par
                You can also change the default animation behaviour by calling
                Animation::setDefaultInterpolationMode.
            */
            void setInterpolationMode( InterpolationMode im );

            /** Gets the current interpolation mode of this animation.
            @remarks
                See setInterpolationMode for more info.
            */
            InterpolationMode getInterpolationMode() const;
            /** Tells the animation how to interpolate rotations.
            @remarks
                By default, animations interpolate linearly between rotations. This
                is fast but not necessarily completely accurate. If you want more
                accurate interpolation, use spherical interpolation, but be aware
                that it will incur a higher cost.
            @par
                You can also change the default rotation behaviour by calling
                Animation::setDefaultRotationInterpolationMode.
            */
            void setRotationInterpolationMode( RotationInterpolationMode im );

            /** Gets the current rotation interpolation mode of this animation.
            @remarks
                See setRotationInterpolationMode for more info.
            */
            RotationInterpolationMode getRotationInterpolationMode() const;

            // Methods for setting the defaults
            /** Sets the default animation interpolation mode.
            @remarks
                Every animation created after this option is set will have the new interpolation
                mode specified. You can also change the mode per animation by calling the
                setInterpolationMode method on the instance in question.
            */
            static void setDefaultInterpolationMode( InterpolationMode im );

            /** Gets the default interpolation mode for all animations. */
            static InterpolationMode getDefaultInterpolationMode();

            /** Sets the default rotation interpolation mode.
            @remarks
                Every animation created after this option is set will have the new interpolation
                mode specified. You can also change the mode per animation by calling the
                setInterpolationMode method on the instance in question.
            */
            static void setDefaultRotationInterpolationMode( RotationInterpolationMode im );

            /** Gets the default rotation interpolation mode for all animations. */
            static RotationInterpolationMode getDefaultRotationInterpolationMode();

            typedef vector<NodeAnimationTrack *>::type NodeTrackList;
            typedef ConstVectorIterator<NodeTrackList> NodeTrackIterator;

            typedef map<unsigned short, OldNodeAnimationTrack *>::type OldNodeTrackList;
            typedef ConstMapIterator<OldNodeTrackList>                 OldNodeTrackIterator;

            typedef map<unsigned short, NumericAnimationTrack *>::type NumericTrackList;
            typedef ConstMapIterator<NumericTrackList>                 NumericTrackIterator;

            typedef map<unsigned short, VertexAnimationTrack *>::type VertexTrackList;
            typedef ConstMapIterator<VertexTrackList>                 VertexTrackIterator;

            /// Fast access to NON-UPDATEABLE node track list
            const NodeTrackList &_getNodeTrackList() const;
            /// Fast access to NON-UPDATEABLE OldNode track list
            const OldNodeTrackList &_getOldNodeTrackList() const;

            /// Get non-updateable iterator over node tracks
            NodeTrackIterator getNodeTrackIterator() const
            {
                return NodeTrackIterator( mNodeTrackList.begin(), mNodeTrackList.end() );
            }

            /// Get non-updateable iterator over node tracks
            OldNodeTrackIterator getOldNodeTrackIterator() const
            {
                return OldNodeTrackIterator( mOldNodeTrackList.begin(), mOldNodeTrackList.end() );
            }

            /// Fast access to NON-UPDATEABLE numeric track list
            const NumericTrackList &_getNumericTrackList() const;

            /// Get non-updateable iterator over node tracks
            NumericTrackIterator getNumericTrackIterator() const
            {
                return NumericTrackIterator( mNumericTrackList.begin(), mNumericTrackList.end() );
            }

            /// Fast access to NON-UPDATEABLE Vertex track list
            const VertexTrackList &_getVertexTrackList() const;

            /// Get non-updateable iterator over node tracks
            VertexTrackIterator getVertexTrackIterator() const
            {
                return VertexTrackIterator( mVertexTrackList.begin(), mVertexTrackList.end() );
            }

            /** Optimise an animation by removing unnecessary tracks and keyframes.
            @remarks
                When you export an animation, it is possible that certain tracks
                have been keyframed but actually don't include anything useful - the
                keyframes include no transformation. These tracks can be completely
                eliminated from the animation and thus speed up the animation.
                In addition, if several keyframes in a row have the same value,
                then they are just adding overhead and can be removed.
            @note
                Since track-less and identity track has difference behavior for
                accumulate animation blending if corresponding track presenting at
                other animation that is non-identity, and in normally this method
                didn't known about the situation of other animation, it can't deciding
                whether or not discards identity tracks. So there have a parameter
                allow you choose what you want, in case you aren't sure how to do that,
                you should use Skeleton::optimiseAllAnimations instead.
            @param
                discardIdentityNodeTracks If true, discard identity node tracks.
            */
            void optimise( bool discardIdentityNodeTracks = true );

            /// A list of track handles
            typedef set<ushort>::type TrackHandleList;

            /** Internal method for collecting identity node tracks.
            @remarks
                This method remove non-identity node tracks form the track handle list.
            @param
                tracks A list of track handle of non-identity node tracks, where this
                method will remove non-identity node track handles.
            */
            void _collectIdentityOldNodeTracks( TrackHandleList &tracks ) const;

            /** Internal method for destroy given node tracks.
             */
            void _destroyOldNodeTracks( const TrackHandleList &tracks );

            /** Clone this animation.
            @note
                The pointer returned from this method is the only one recorded,
                thus it is up to the caller to arrange for the deletion of this
                object.
            */
            Animation *clone( const String &newName ) const;

            /** Internal method used to tell the animation that keyframe list has been
                changed, which may cause it to rebuild some internal data */
            void _keyFrameListChanged() { mKeyFrameTimesDirty = true; }

            /** Internal method used to convert time position to time index object.
            @note
                The time index returns by this function are associated with state of
                the animation object, if the animation object altered (e.g. create/remove
                keyframe or track), all related time index will invalidated.
            @param timePos The time position.
            @return The time index object which contains wrapped time position (in
                relation to the whole animation sequence) and lower bound index of
                global keyframe time list.
            */
            TimeIndex _getTimeIndex( Real timePos ) const;

            /** Sets a base keyframe which for the skeletal / pose keyframes
                in this animation.
            @remarks
                Skeletal and pose animation keyframes are expressed as deltas from a
                given base state. By default, that is the binding setup of the skeleton,
                or the object space mesh positions for pose animation. However, sometimes
                it is useful for animators to create animations with a different starting
                pose, because that's more convenient, and the animation is designed to
                simply be added to the existing animation state and not globally averaged
                with other animations (this is always the case with pose animations, but
                is activated for skeletal animations via ANIMBLEND_CUMULATIVE).
            @par
                In order for this to work, the keyframes need to be 're-based' against
                this new starting state, for example by treating the first keyframe as
                the reference point (and therefore representing no change). This can
                be achieved by applying the inverse of this reference keyframe against
                all other keyframes. Since this fundamentally changes the animation,
                this method just marks the animation as requiring this rebase, which
                is performed at the next Animation 'apply' call. This is to allow the
                Animation to be re-saved with this flag set, but without having altered
                the keyframes yet, so no data is lost unintentionally. If you wish to
                save the animation after the adjustment has taken place, you can
                (@see _applyBaseKeyFrame)
            @param useBaseKeyFrame Whether a base keyframe should be used
            @param keyframeTime The time corresponding to the base keyframe, if any
            @param baseAnimName Optionally a different base animation (must contain the same tracks)
            */
            void setUseBaseKeyFrame( bool useBaseKeyFrame, Real keyframeTime = 0.0f,
                                     const String &baseAnimName = BLANKSTRING );
            /** Whether a base keyframe is being used for this Animation. */
            bool getUseBaseKeyFrame() const;
            /** If a base keyframe is being used, the time of that keyframe. */
            Real getBaseKeyFrameTime() const;
            /** If a base keyframe is being used, the Animation that provides that keyframe. */
            const String &getBaseKeyFrameAnimationName() const;

            /// Internal method to adjust keyframes relative to a base keyframe (@see setUseBaseKeyFrame)
            /// */
            void _applyBaseKeyFrame();

            void _notifyContainer( AnimationContainer *c );
            /** Retrieve the container of this animation. */
            AnimationContainer *getContainer();

        protected:
            /// Node tracks, indexed by handle
            NodeTrackList mNodeTrackList;
            /// OldNode tracks, indexed by handle
            OldNodeTrackList mOldNodeTrackList;
            /// Numeric tracks, indexed by handle
            NumericTrackList mNumericTrackList;
            /// Vertex tracks, indexed by handle
            VertexTrackList mVertexTrackList;
            String          mName;

            Real mLength;

            InterpolationMode         mInterpolationMode;
            RotationInterpolationMode mRotationInterpolationMode;

            static InterpolationMode         msDefaultInterpolationMode;
            static RotationInterpolationMode msDefaultRotationInterpolationMode;

            /// Global keyframe time list used to search global keyframe index.
            typedef vector<Real>::type KeyFrameTimeList;
            mutable KeyFrameTimeList   mKeyFrameTimes;
            /// Dirty flag indicate that keyframe time list need to rebuild
            mutable bool mKeyFrameTimesDirty;

            bool                mUseBaseKeyFrame;
            Real                mBaseKeyFrameTime;
            String              mBaseKeyFrameAnimationName;
            AnimationContainer *mContainer;

            void optimiseOldNodeTracks( bool discardIdentityTracks );
            void optimiseVertexTracks();

            /// Internal method to build global keyframe time list
            void buildKeyFrameTimeList() const;
        };

        /** @} */
        /** @} */
    }  // namespace v1
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif  // __Animation_H__
