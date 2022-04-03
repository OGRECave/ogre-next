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

#ifndef __AnimationTrack_H__
#define __AnimationTrack_H__

#include "OgrePrerequisites.h"

#include "OgrePose.h"
#include "OgreRotationalSpline.h"
#include "OgreSimpleSpline.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    namespace v1
    {
        class VertexPoseKeyFrame;
        class KeyFrame;

        /** \addtogroup Core
         *  @{
         */
        /** \addtogroup Animation
         *  @{
         */
        /** Time index object used to search keyframe at the given position.
         */
        class _OgreExport TimeIndex
        {
        protected:
            /** The time position (in relation to the whole animation sequence)
             */
            Real mTimePos;
            /** The global keyframe index (in relation to the whole animation sequence)
                that used to convert to local keyframe index, or INVALID_KEY_INDEX which
                means global keyframe index unavailable, and then slight slow method will
                used to search local keyframe index.
            */
            uint mKeyIndex;

            /** Indicate it's an invalid global keyframe index.
             */
            static const uint INVALID_KEY_INDEX = (uint)-1;

        public:
            /** Construct time index object by the given time position.
             */
            TimeIndex( Real timePos ) : mTimePos( timePos ), mKeyIndex( INVALID_KEY_INDEX ) {}

            /** Construct time index object by the given time position and
                global keyframe index.
            @note In normally, you don't need to use this constructor directly, use
                Animation::_getTimeIndex instead.
            */
            TimeIndex( Real timePos, uint keyIndex ) : mTimePos( timePos ), mKeyIndex( keyIndex ) {}

            bool hasKeyIndex() const { return mKeyIndex != INVALID_KEY_INDEX; }

            Real getTimePos() const { return mTimePos; }

            uint getKeyIndex() const { return mKeyIndex; }
        };

        /** A 'track' in an animation sequence, i.e. a sequence of keyframes which affect a
            certain type of animable object.
        @remarks
            This class is intended as a base for more complete classes which will actually
            animate specific types of object, e.g. a bone in a skeleton to affect
            skeletal animation. An animation will likely include multiple tracks each of which
            can be made up of many KeyFrame instances. Note that the use of tracks allows each animable
            object to have it's own number of keyframes, i.e. you do not have to have the
            maximum number of keyframes for all animable objects just to cope with the most
            animated one.
        @remarks
            Since the most common animable object is a OldNode, there are options in this class for
        associating the track with a OldNode which will receive keyframe updates automatically when the
        'apply' method is called.
        @remarks
            By default rotation is done using shortest-path algorithm.
            It is possible to change this behaviour using
            setUseShortestRotationPath() method.
        */
        class _OgreExport AnimationTrack : public OgreAllocatedObj
        {
        public:
            /** Listener allowing you to override certain behaviour of a track,
                for example to drive animation procedurally.
            */
            class _OgreExport Listener
            {
            public:
                virtual ~Listener();

                /** Get an interpolated keyframe for this track at the given time.
                @return true if the KeyFrame was populated, false if not.
                */
                virtual bool getInterpolatedKeyFrame( const AnimationTrack *t,
                                                      const TimeIndex &timeIndex, KeyFrame *kf ) = 0;
            };

            /// Constructor
            AnimationTrack( Animation *parent, unsigned short handle );

            virtual ~AnimationTrack();

            /** Get the handle associated with this track. */
            unsigned short getHandle() const { return mHandle; }

            /** Returns the number of keyframes in this animation. */
            virtual unsigned short getNumKeyFrames() const;

            /** Returns the KeyFrame at the specified index. */
            virtual KeyFrame *getKeyFrame( unsigned short index ) const;

            /** Gets the 2 KeyFrame objects which are active at the time given, and the blend value
            between them.
            @remarks
                At any point in time  in an animation, there are either 1 or 2 keyframes which are
            'active', 1 if the time index is exactly on a keyframe, 2 at all other times i.e. the
            keyframe before and the keyframe after.
            @par
                This method returns those keyframes given a time index, and also returns a parametric
                value indicating the value of 't' representing where the time index falls between them.
                E.g. if it returns 0, the time index is exactly on keyFrame1, if it returns 0.5 it is
                half way between keyFrame1 and keyFrame2 etc.
            @param timeIndex The time index.
            @param keyFrame1 Pointer to a KeyFrame pointer which will receive the pointer to the
                keyframe just before or at this time index.
            @param keyFrame2 Pointer to a KeyFrame pointer which will receive the pointer to the
                keyframe just after this time index.
            @param firstKeyIndex Pointer to an unsigned short which, if supplied, will receive the
                index of the 'from' keyframe in case the caller needs it.
            @return Parametric value indicating how far along the gap between the 2 keyframes the
            timeIndex value is, e.g. 0.0 for exactly at 1, 0.25 for a quarter etc. By definition the
            range of this value is:  0.0 <= returnValue < 1.0 .
            */
            virtual Real getKeyFramesAtTime( const TimeIndex &timeIndex, KeyFrame **keyFrame1,
                                             KeyFrame      **keyFrame2,
                                             unsigned short *firstKeyIndex = 0 ) const;

            /** Creates a new KeyFrame and adds it to this animation at the given time index.
            @remarks
                It is better to create KeyFrames in time order. Creating them out of order can result
                in expensive reordering processing. Note that a KeyFrame at time index 0.0 is always
            created for you, so you don't need to create this one, just access it using getKeyFrame(0);
            @param timePos The time from which this KeyFrame will apply.
            */
            virtual KeyFrame *createKeyFrame( Real timePos );

            /** Removes a KeyFrame by it's index. */
            virtual void removeKeyFrame( unsigned short index );

            /** Removes all the KeyFrames from this track. */
            virtual void removeAllKeyFrames();

            /** Gets a KeyFrame object which contains the interpolated transforms at the time index
            specified.
            @remarks
                The KeyFrame objects held by this class are transformation snapshots at
                discrete points in time. Normally however, you want to interpolate between these
                keyframes to produce smooth movement, and this method allows you to do this easily.
                In animation terminology this is called 'tweening'.
            @param timeIndex The time (in relation to the whole animation sequence)
            @param kf Keyframe object to store results
            */
            virtual void getInterpolatedKeyFrame( const TimeIndex &timeIndex, KeyFrame *kf ) const = 0;

            /** Applies an animation track to the designated target.
            @param timeIndex The time position in the animation to apply.
            @param weight The influence to give to this track, 1.0 for full influence, less to blend with
              other animations.
            @param scale The scale to apply to translations and scalings, useful for
                adapting an animation to a different size target.
            */
            virtual void apply( const TimeIndex &timeIndex, Real weight = 1.0, Real scale = 1.0f ) = 0;

            /** Internal method used to tell the track that keyframe data has been
                changed, which may cause it to rebuild some internal data. */
            virtual void _keyFrameDataChanged() const {}

            /** Method to determine if this track has any KeyFrames which are
            doing anything useful - can be used to determine if this track
            can be optimised out.
            */
            virtual bool hasNonZeroKeyFrames() const { return true; }

            /** Optimise the current track by removing any duplicate keyframes. */
            virtual void optimise() {}

            /** Internal method to collect keyframe times, in unique, ordered format. */
            virtual void _collectKeyFrameTimes( vector<Real>::type &keyFrameTimes );

            /** Internal method to build keyframe time index map to translate global lower
                bound index to local lower bound index. */
            virtual void _buildKeyFrameIndexMap( const vector<Real>::type &keyFrameTimes );

            /** Internal method to re-base the keyframes relative to a given keyframe. */
            virtual void _applyBaseKeyFrame( const KeyFrame *base );

            /** Set a listener for this track. */
            virtual void setListener( Listener *l ) { mListener = l; }

            /** Returns the parent Animation object for this track. */
            Animation *getParent() const { return mParent; }

        protected:
            typedef vector<KeyFrame *>::type KeyFrameList;
            KeyFrameList                     mKeyFrames;
            Animation                       *mParent;
            unsigned short                   mHandle;
            Listener                        *mListener;

            /// Map used to translate global keyframe time lower bound index to local lower bound index
            typedef vector<ushort>::type KeyFrameIndexMap;
            KeyFrameIndexMap             mKeyFrameIndexMap;

            /// Create a keyframe implementation - must be overridden
            virtual KeyFrame *createKeyFrameImpl( Real time ) = 0;

            /// Internal method for clone implementation
            virtual void populateClone( AnimationTrack *clone ) const;
        };

        /** Specialised AnimationTrack for dealing with generic animable values.
         */
        class _OgreExport NumericAnimationTrack : public AnimationTrack
        {
        public:
            /// Constructor
            NumericAnimationTrack( Animation *parent, unsigned short handle );
            /// Constructor, associates with an AnimableValue
            NumericAnimationTrack( Animation *parent, unsigned short handle, AnimableValuePtr &target );

            /** Creates a new KeyFrame and adds it to this animation at the given time index.
            @remarks
                It is better to create KeyFrames in time order. Creating them out of order can result
                in expensive reordering processing. Note that a KeyFrame at time index 0.0 is always
            created for you, so you don't need to create this one, just access it using getKeyFrame(0);
            @param timePos The time from which this KeyFrame will apply.
            */
            virtual NumericKeyFrame *createNumericKeyFrame( Real timePos );

            /// @copydoc AnimationTrack::getInterpolatedKeyFrame
            void getInterpolatedKeyFrame( const TimeIndex &timeIndex, KeyFrame *kf ) const override;

            /// @copydoc AnimationTrack::apply
            void apply( const TimeIndex &timeIndex, Real weight = 1.0, Real scale = 1.0f ) override;

            /** Applies an animation track to a given animable value.
            @param anim The AnimableValue to which to apply the animation
            @param timeIndex The time position in the animation to apply.
            @param weight The influence to give to this track, 1.0 for full influence, less to blend with
              other animations.
            @param scale The scale to apply to translations and scalings, useful for
                adapting an animation to a different size target.
            */
            void applyToAnimable( const AnimableValuePtr &anim, const TimeIndex &timeIndex,
                                  Real weight = 1.0, Real scale = 1.0f );

            /** Returns a pointer to the associated animable object (if any). */
            virtual const AnimableValuePtr &getAssociatedAnimable() const;

            /** Sets the associated animable object which will be automatically
                affected by calls to 'apply'. */
            virtual void setAssociatedAnimable( const AnimableValuePtr &val );

            /** Returns the KeyFrame at the specified index. */
            NumericKeyFrame *getNumericKeyFrame( unsigned short index ) const;

            /** Clone this track (internal use only) */
            NumericAnimationTrack *_clone( Animation *newParent ) const;

        protected:
            /// Target to animate
            AnimableValuePtr mTargetAnim;

            /// @copydoc AnimationTrack::createKeyFrameImpl
            KeyFrame *createKeyFrameImpl( Real time ) override;
        };

        /** Specialised AnimationTrack for dealing with node transforms.
         */
        class _OgreExport NodeAnimationTrack : public AnimationTrack
        {
        public:
            /// Constructor
            NodeAnimationTrack( Animation *parent, unsigned short handle );
            /// Constructor, associates with a OldNode
            NodeAnimationTrack( Animation *parent, unsigned short handle, Node *targetNode );
            /// Destructor
            ~NodeAnimationTrack() override;
            /** Creates a new KeyFrame and adds it to this animation at the given time index.
            @remarks
                It is better to create KeyFrames in time order. Creating them out of order can result
                in expensive reordering processing. Note that a KeyFrame at time index 0.0 is always
            created for you, so you don't need to create this one, just access it using getKeyFrame(0);
            @param timePos The time from which this KeyFrame will apply.
            */
            virtual TransformKeyFrame *createNodeKeyFrame( Real timePos );

            /** Get the initial state of the node for this track
            @remarks
                Node tracks are based from an initial state, which the node is set to every frame before
            the node animation is applied See NodeAnimationTrack::getInitialState for more information
            */
            void getInitialState( Vector3 *outPosition, Quaternion *outOrientation, Vector3 *outScale );

            /** Set the initial state of the node for this animation track
            @remarks
                Node tracks are based from an initial state, which the node is set to every frame before
            the node animation is applied Most 3D model exporters base each animation frame from a 0
            transform, which is the defaults. However in some situations you may want to change this.
            @param position     The initial position to set
            @param orientation  The initial orientation to set
            @param scale        The initial scale to set
            */
            void setInitialState( const Vector3 &position, const Quaternion &orientation,
                                  const Vector3 &scale );

            /** Resets the the target node to the initial state
            @remarks
                This is called everyframe before the animation is applied. @See setInitialState
            */
            void resetNodeToInitialState();

            /** Returns a pointer to the associated OldNode object (if any). */
            Node *getAssociatedNode() const;

            /** Sets the associated OldNode object which will be automatically affected by calls to
             * 'apply'. */
            void setAssociatedNode( Node *node );

            /** As the 'apply' method but applies to a specified OldNode instead of associated node. */
            void applyToNode( Node *node, const TimeIndex &timeIndex, Real weight = 1.0,
                              Real scale = 1.0f );

            /** Sets the method of rotation calculation */
            virtual void setUseShortestRotationPath( bool useShortestPath );

            /** Gets the method of rotation calculation */
            virtual bool getUseShortestRotationPath() const;

            /// @copydoc AnimationTrack::getInterpolatedKeyFrame
            void getInterpolatedKeyFrame( const TimeIndex &timeIndex, KeyFrame *kf ) const override;

            /// @copydoc AnimationTrack::apply
            void apply( const TimeIndex &timeIndex, Real weight = 1.0, Real scale = 1.0f ) override;

            /// @copydoc AnimationTrack::_keyFrameDataChanged
            void _keyFrameDataChanged() const override;

            /** Returns the KeyFrame at the specified index. */
            virtual TransformKeyFrame *getNodeKeyFrame( unsigned short index ) const;

            /** Method to determine if this track has any KeyFrames which are
                doing anything useful - can be used to determine if this track
                can be optimised out.
            */
            bool hasNonZeroKeyFrames() const override;

            /** Optimise the current track by removing any duplicate keyframes. */
            void optimise() override;

            /** Clone this track (internal use only) */
            NodeAnimationTrack *_clone( Animation *newParent ) const;

            void _applyBaseKeyFrame( const KeyFrame *base ) override;

        protected:
            /// Specialised keyframe creation
            KeyFrame *createKeyFrameImpl( Real time ) override;
            // Flag indicating we need to rebuild the splines next time
            virtual void buildInterpolationSplines() const;

            // Struct for store splines, allocate on demand for better memory footprint
            struct Splines
            {
                SimpleSpline     positionSpline;
                SimpleSpline     scaleSpline;
                RotationalSpline rotationSpline;
            };

            Node *mTargetNode;

            /// OldNodeAnimationTrack delegated intial transform information to the OldNode.
            /// That was the wrong place. The right place is in the root of the animation system.
            /// In the case of skeletal animation that would've been the Skeleton. Since this
            /// new NodeAnimationTrack is only used as a tool to animate individual nodes (i.e.
            /// cameras, a few lights) and rarely more than one track is applied to the same Node,
            /// we are now the ones holding this information.
            Vector3    mInitialPosition;
            Quaternion mInitialOrientation;
            Vector3    mInitialScale;

            // Prebuilt splines, must be mutable since lazy-update in const method
            mutable Splines *mSplines;
            mutable bool     mSplineBuildNeeded;
            /// Defines if rotation is done using shortest path
            mutable bool mUseShortestRotationPath;
        };

        /** Specialised AnimationTrack for dealing with node transforms.
         */
        class _OgreExport OldNodeAnimationTrack : public AnimationTrack
        {
        public:
            /// Constructor
            OldNodeAnimationTrack( Animation *parent, unsigned short handle );
            /// Constructor, associates with a OldNode
            OldNodeAnimationTrack( Animation *parent, unsigned short handle, OldNode *targetNode );
            /// Destructor
            ~OldNodeAnimationTrack() override;
            /** Creates a new KeyFrame and adds it to this animation at the given time index.
            @remarks
                It is better to create KeyFrames in time order. Creating them out of order can result
                in expensive reordering processing. Note that a KeyFrame at time index 0.0 is always
            created for you, so you don't need to create this one, just access it using getKeyFrame(0);
            @param timePos The time from which this KeyFrame will apply.
            */
            virtual TransformKeyFrame *createNodeKeyFrame( Real timePos );
            /** Returns a pointer to the associated OldNode object (if any). */
            OldNode *getAssociatedNode() const;

            /** Sets the associated OldNode object which will be automatically affected by calls to
             * 'apply'. */
            void setAssociatedNode( OldNode *node );

            /** As the 'apply' method but applies to a specified OldNode instead of associated node. */
            void applyToNode( OldNode *node, const TimeIndex &timeIndex, Real weight = 1.0,
                              Real scale = 1.0f );

            /** Sets the method of rotation calculation */
            virtual void setUseShortestRotationPath( bool useShortestPath );

            /** Gets the method of rotation calculation */
            virtual bool getUseShortestRotationPath() const;

            /// @copydoc AnimationTrack::getInterpolatedKeyFrame
            void getInterpolatedKeyFrame( const TimeIndex &timeIndex, KeyFrame *kf ) const override;

            /// @copydoc AnimationTrack::apply
            void apply( const TimeIndex &timeIndex, Real weight = 1.0, Real scale = 1.0f ) override;

            /// @copydoc AnimationTrack::_keyFrameDataChanged
            void _keyFrameDataChanged() const override;

            /** Returns the KeyFrame at the specified index. */
            virtual TransformKeyFrame *getNodeKeyFrame( unsigned short index ) const;

            /** Method to determine if this track has any KeyFrames which are
                doing anything useful - can be used to determine if this track
                can be optimised out.
            */
            bool hasNonZeroKeyFrames() const override;

            /** Optimise the current track by removing any duplicate keyframes. */
            void optimise() override;

            /** Clone this track (internal use only) */
            OldNodeAnimationTrack *_clone( Animation *newParent ) const;

            void _applyBaseKeyFrame( const KeyFrame *base ) override;

        protected:
            /// Specialised keyframe creation
            KeyFrame *createKeyFrameImpl( Real time ) override;
            // Flag indicating we need to rebuild the splines next time
            virtual void buildInterpolationSplines() const;

            // Struct for store splines, allocate on demand for better memory footprint
            struct Splines
            {
                SimpleSpline     positionSpline;
                SimpleSpline     scaleSpline;
                RotationalSpline rotationSpline;
            };

            OldNode *mTargetNode;
            // Prebuilt splines, must be mutable since lazy-update in const method
            mutable Splines *mSplines;
            mutable bool     mSplineBuildNeeded;
            /// Defines if rotation is done using shortest path
            mutable bool mUseShortestRotationPath;
        };

        /** Type of vertex animation.
            Vertex animation comes in 2 types, morph and pose. The reason
            for the 2 types is that we have 2 different potential goals - to encapsulate
            a complete, flowing morph animation with multiple keyframes (a typical animation,
            but implemented by having snapshots of the vertex data at each keyframe),
            or to represent a single pose change, for example a facial expression.
            Whilst both could in fact be implemented using the same system, we choose
            to separate them since the requirements and limitations of each are quite
            different.
        @par
            Morph animation is a simple approach where we have a whole series of
            snapshots of vertex data which must be interpolated, e.g. a running
            animation implemented as morph targets. Because this is based on simple
            snapshots, it's quite fast to use when animating an entire mesh because
            it's a simple linear change between keyframes. However, this simplistic
            approach does not support blending between multiple morph animations.
            If you need animation blending, you are advised to use skeletal animation
            for full-mesh animation, and pose animation for animation of subsets of
            meshes or where skeletal animation doesn't fit - for example facial animation.
            For animating in a vertex shader, morph animation is quite simple and
            just requires the 2 vertex buffers (one the original position buffer)
            of absolute position data, and an interpolation factor. Each track in
            a morph animation references a unique set of vertex data.
        @par
            Pose animation is more complex. Like morph animation each track references
            a single unique set of vertex data, but unlike morph animation, each
            keyframe references 1 or more 'poses', each with an influence level.
            A pose is a series of offsets to the base vertex data, and may be sparse - ie it
            may not reference every vertex. Because they're offsets, they can be
            blended - both within a track and between animations. This set of features
            is very well suited to facial animation.
        @par
            For example, let's say you modelled a face (one set of vertex data), and
            defined a set of poses which represented the various phonetic positions
            of the face. You could then define an animation called 'SayHello', containing
            a single track which referenced the face vertex data, and which included
            a series of keyframes, each of which referenced one or more of the facial
            positions at different influence levels - the combination of which over
            time made the face form the shapes required to say the word 'hello'. Since
            the poses are only stored once, but can be referenced may times in
            many animations, this is a very powerful way to build up a speech system.
        @par
            The downside of pose animation is that it can be more difficult to set up.
            Also, since it uses more buffers (one for the base data, and one for each
            active pose), if you're animating in hardware using vertex shaders you need
            to keep an eye on how many poses you're blending at once. You define a
            maximum supported number in your vertex program definition, see the
            includes_pose_animation material script entry.
        @par
            So, by partitioning the vertex animation approaches into 2, we keep the
            simple morph technique easy to use, whilst still allowing all
            the powerful techniques to be used. Note that morph animation cannot
            be blended with other types of vertex animation (pose animation or other
            morph animation); pose animation can be blended with other pose animation
            though, and both types can be combined with skeletal animation. Also note
            that all morph animation can be expressed as pose animation, but not vice
            versa.
        */
        enum VertexAnimationType
        {
            /// No animation
            VAT_NONE = 0,
            /// Morph animation is made up of many interpolated snapshot keyframes
            VAT_MORPH = 1,
            /// Pose animation is made up of a single delta pose keyframe
            VAT_POSE = 2
        };

        /** Specialised AnimationTrack for dealing with changing vertex position information.
        @see VertexAnimationType
        */
        class _OgreExport VertexAnimationTrack : public AnimationTrack
        {
        public:
            /** The target animation mode */
            enum TargetMode
            {
                /// Interpolate vertex positions in software
                TM_SOFTWARE,
                /** Bind keyframe 1 to position, and keyframe 2 to a texture coordinate
                    for interpolation in hardware */
                TM_HARDWARE
            };
            /// Constructor
            VertexAnimationTrack( Animation *parent, unsigned short handle,
                                  VertexAnimationType animType );
            /// Constructor, associates with target VertexData and temp buffer (for software)
            VertexAnimationTrack( Animation *parent, unsigned short handle, VertexAnimationType animType,
                                  VertexData *targetData, TargetMode target = TM_SOFTWARE );

            /** Get the type of vertex animation we're performing. */
            VertexAnimationType getAnimationType() const { return mAnimationType; }

            /** Whether the vertex animation (if present) includes normals */
            bool getVertexAnimationIncludesNormals() const;

            /** Creates a new morph KeyFrame and adds it to this animation at the given time index.
            @remarks
            It is better to create KeyFrames in time order. Creating them out of order can result
            in expensive reordering processing. Note that a KeyFrame at time index 0.0 is always created
            for you, so you don't need to create this one, just access it using getKeyFrame(0);
            @param timePos The time from which this KeyFrame will apply.
            */
            virtual VertexMorphKeyFrame *createVertexMorphKeyFrame( Real timePos );

            /** Creates the single pose KeyFrame and adds it to this animation.
             */
            virtual VertexPoseKeyFrame *createVertexPoseKeyFrame( Real timePos );

            /** @copydoc AnimationTrack::getInterpolatedKeyFrame
             */
            void getInterpolatedKeyFrame( const TimeIndex &timeIndex, KeyFrame *kf ) const override;

            /// @copydoc AnimationTrack::apply
            void apply( const TimeIndex &timeIndex, Real weight = 1.0, Real scale = 1.0f ) override;

            /** As the 'apply' method but applies to specified VertexData instead of
                associated data. */
            virtual void applyToVertexData( VertexData *data, const TimeIndex &timeIndex,
                                            Real weight = 1.0, const PoseList *poseList = 0 );

            /** Returns the morph KeyFrame at the specified index. */
            VertexMorphKeyFrame *getVertexMorphKeyFrame( unsigned short index ) const;

            /** Returns the pose KeyFrame at the specified index. */
            VertexPoseKeyFrame *getVertexPoseKeyFrame( unsigned short index ) const;

            /** Sets the associated VertexData which this track will update. */
            void setAssociatedVertexData( VertexData *data ) { mTargetVertexData = data; }
            /** Gets the associated VertexData which this track will update. */
            VertexData *getAssociatedVertexData() const { return mTargetVertexData; }

            /// Set the target mode
            void setTargetMode( TargetMode m ) { mTargetMode = m; }
            /// Get the target mode
            TargetMode getTargetMode() const { return mTargetMode; }

            /** Method to determine if this track has any KeyFrames which are
            doing anything useful - can be used to determine if this track
            can be optimised out.
            */
            bool hasNonZeroKeyFrames() const override;

            /** Optimise the current track by removing any duplicate keyframes. */
            void optimise() override;

            /** Clone this track (internal use only) */
            VertexAnimationTrack *_clone( Animation *newParent ) const;

            void _applyBaseKeyFrame( const KeyFrame *base ) override;

        protected:
            /// Animation type
            VertexAnimationType mAnimationType;
            /// Target to animate
            VertexData *mTargetVertexData;
            /// Mode to apply
            TargetMode mTargetMode;

            /// @copydoc AnimationTrack::createKeyFrameImpl
            KeyFrame *createKeyFrameImpl( Real time ) override;

            /// Utility method for applying pose animation
            void applyPoseToVertexData( const Pose *pose, VertexData *data, Real influence );
        };
        /** @} */
        /** @} */
    }  // namespace v1
}  // namespace Ogre
#include "OgreHeaderSuffix.h"

#endif
