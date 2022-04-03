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

#ifndef __SkeletonAnimation_H__
#define __SkeletonAnimation_H__

#include "OgreSkeletonTrack.h"

#include "OgreIdString.h"
#include "OgreRawPtr.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
#if defined( __GNUC__ ) && !defined( __clang__ )
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wignored-attributes"
#endif

    class SkeletonAnimationDef;
    class SkeletonInstance;

    typedef vector<KeyFrameRigVec::const_iterator>::type KnownKeyFramesVec;

    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Animation
     *  @{
     */

    /// Represents the instance of a Skeletal animation based on its definition
    class _OgreExport SkeletonAnimation : public OgreAllocatedObj
    {
        SkeletonAnimationDef const *mDefinition;

    protected:
        RawSimdUniquePtr<ArrayReal, MEMCATEGORY_ANIMATION> mBoneWeights;
        Real                                               mCurrentFrame;

    public:
        Real                     mFrameRate;  // Playback framerate
        Real                     mWeight;
        FastArray<size_t> const *mSlotStarts;  // One per parent depth level
        bool                     mLoop;
        bool                     mEnabled;
        SkeletonInstance        *mOwner;

    protected:
        IdString mName;

        /// One per track
        KnownKeyFramesVec mLastKnownKeyFrames;

    public:
        SkeletonAnimation( const SkeletonAnimationDef *definition, const FastArray<size_t> *slotStarts,
                           SkeletonInstance *owner );

        /// Internal function that initializes a lot of structures that can't be done in the
        /// constructor due to how SkeletonInstance is created/pushed in a vector.
        /// If you're not an Ogre dev, don't call this directly.
        void _initialize();

        /** Shifts the values of mBoneWeights to new locations because the bones' mIndex
            may have changed.
            Needed when our BoneMemoryManager performs a cleanup or similar memory change.
        @param oldSlotStarts
            Array with the contents old contents of SkeletonInstance::mSlotStarts, one
            entry per node hirearchy depth level
        */
        void _boneMemoryRebased( const FastArray<size_t> &oldSlotStarts );

        /** Plays the animation forward (or backwards if negative)
        @param time
            Time to advance, in seconds
        */
        void addTime( Real time ) { addFrame( time * mFrameRate ); }

        /** Plays the animation forward (or backwards if negative)
        @param frames
            Frames to advance, in frames
        */
        void addFrame( Real frames );

        /** Sets the animation to a particular time.
        @param time
            Time to set to, in seconds
        */
        void setTime( Real time ) { setFrame( time * mFrameRate ); }

        /** Sets the animation to a particular frame.
        @param frames
            Frame to set to, in frames
        */
        void setFrame( Real frame );

        /// Gets the current animation time, in seconds. Prefer using getCurrentFrame
        Real getCurrentTime() const { return mCurrentFrame / mFrameRate; }

        /// Gets the current animation frame, in frames.
        Real getCurrentFrame() const { return mCurrentFrame; }

        /// Gets the frame count.
        Real getNumFrames() const;

        /// Gets animation length, in seconds.
        Real getDuration() const;

        IdString getName() const { return mName; }

        /** Loop setting. Looped animations will wrap back to zero when reaching the animation length
            or go back to the animation length if playing backwards.
            Non-looped animations will stop at the animation length (or at 0 if backwards) but won't
            be disabled.
        */
        void setLoop( bool bLoop ) { mLoop = bLoop; }

        /** Returns current loop setting. @See setLoop.
         */
        bool getLoop() const { return mLoop; }

        /** Sets the per-bone weight to a particular bone. Useful for fine control
            over animation strength on a set of nodes (i.e. an arm)
        @remarks
            By default all bone weights are set to 1.0
        @param boneName
            The name of the bone to set. If this animation doesn't affect that bone (or the
            name is invalid) this function does nothing.
        @param weight
            Weight to apply to this particular bone. Note that the animation multiplies this
            value against the global mWeight to obtain the final weight.
            Normal range is between [0; 1] but not necessarily.
        */
        void setBoneWeight( IdString boneName, Real weight );

        /** Gets the current per-bone weight of a particular bone.
        @param boneName
            The name of the bone to get. If this animation doesn't affect that bone (or the
            name is invalid) this function returns 0.
        @return
            The weight of the specified bone. 0 if not found.
        */
        Real getBoneWeight( IdString boneName ) const;

        /** Gets a pointer current per-bone weight of a particular bone. Useful if you intend
            to have read/write access to this value very often.
        @remarks
            !!! EXTREMELY IMPORTANT !!!
            If *any* skeleton instance (that shares the same SkeletonDef) is destroyed,
            the returned value may be invalidated!

            If returnPtr is the return value to bone[0], do not assume that returnPtr+1
            affects bone[1] or even any other bone. Doing so the behavior is underfined
            and most likely you could be affecting the contents of other SkeletonInstances.
        @param boneName
            The name of the bone to get. If this animation doesn't affect that bone (or the
            name is invalid) this function returns a null pointer.
        @return
            The pointer to the bone weight of the specified bone. Null pointer if not found.
        */
        Real *getBoneWeightPtr( IdString boneName );

        /** Given all the bones this animation uses, sets the weight of these on _other_ animations

            The use case is very specific: Imagine a 3rd person shooter. Normally animations get
            blended together either additively or cummulative (e.g. to smoothly transition from walk
            to idle, from idle to run, from run to cover, etc)

            However certain animations, such as Reload, need to _override_ all other animations but
            only on a particular set of bones.

            Whether the character is idle, walking or running; we want the reload animation to
            play at 100% weight (on torso, arms and hands), while the walk/idle/run animations still
            also play at 100% weight on bones unaffected by the reload (like the legs).

            Example code:

            @code
                // When starting reload
                reloadAnim->setOverrideBoneWeightsOnActiveAnimations( 0.0f, false );
                reloadAnim->setEnabled( true );

                // When starting reload is over
                reloadAnim->setEnabled( false );
                reloadAnim->setOverrideBoneWeightsOnActiveAnimations( 1.0f, false );

                // To smoothly fade in/out while allowing per-bone granularity:
                // fadeOutFactor = 0 means we're fully faded in
                // fadeOutFactor = 1 means we're faded out entirely
                reloadAnim->setOverrideBoneWeightsOnActiveAnimations( fadeOutFactor, true );

                // To smoothly fade in/out while overriding all bones:
                reloadAnim->setOverrideBoneWeightsOnActiveAnimations( fadeOutFactor, false );
            @endcode

            For this function to have any usefulness, the animation from Maya/Blender/etc
            needs to have been exported with only animation tracks on bones that are modified
            (i.e. the exporter should not create dummy nodes resetting to default pose on
            unanimated bones)

            If you're using [blender2ogre](https://github.com/OGRECave/blender2ogre), make sure
            to tick "Only Keyframed Bones"

        @remarks
            This overload works only on currently active animations.
            To override all (active and inactive) animations, use setOverrideBoneWeightsOnAllAnimations

            Avoid calling this function unnecessarily (e.g. don't call it every frame if weight
            value did not change). It's not super expensive, but it is not free either.

            Any custom per-bone weight you set on other animations
            (e.g. by calling other->setBoneWeight) will be overwritten.
        @param constantWeight
            A constant weight to apply to all bone weights in other animations
            Should be in range [0; 1]
        @param bPerBone
            When false, all other animations are set to constantWeight
            When true, each bone in other animations are set to:

                Math::lerp( 1.0f - boneWeight, constantWeight, boneWeight );

            This allows you to selectively avoid overriding certain bones while also
            smoothly fade in/out animations (i.e. gives you finer granularity control)

            When true, the math operation we perform boils down to:

            @code
                for each affectedBone in this_animation
                    for each otherAnim in parent->getAnimations()
                        finalWeight = lerp( 1.0 - affectedBone->weight,
                                            constantWeight, affectedBone->weight );
                        otherAnim->setBoneWeight( affectedBone->name, finalWeight );
            @endcode
        */
        void setOverrideBoneWeightsOnActiveAnimations( const Real constantWeight,
                                                       const bool bPerBone = false );

        /// @see SkeletonAnimation::setOverrideBoneWeightsOnActiveAnimations
        void setOverrideBoneWeightsOnAllAnimations( const Real constantWeight,
                                                    const bool bPerBone = false );

        /// Enables or disables this animation. A disabled animation won't be processed at all.
        void setEnabled( bool bEnable );
        bool getEnabled() const { return mEnabled; }

        void _applyAnimation( const TransformArray &boneTransforms );

        void _swapBoneWeightsUniquePtr(
            RawSimdUniquePtr<ArrayReal, MEMCATEGORY_ANIMATION> &inOutBoneWeights );

        const SkeletonAnimationDef *getDefinition() const { return mDefinition; }
    };

    /** @} */
    /** @} */

#if defined( __GNUC__ ) && !defined( __clang__ )
#    pragma GCC diagnostic pop
#endif
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
