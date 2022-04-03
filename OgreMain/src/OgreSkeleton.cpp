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
#include "OgreStableHeaders.h"

#include "OgreSkeleton.h"

#include "OgreAnimationState.h"
#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreOldBone.h"
#include "OgreOldSkeletonManager.h"
#include "OgreSkeletonSerializer.h"
// Just for logging
#include "OgreAnimationTrack.h"
#include "OgreKeyFrame.h"
#include "OgreStringConverter.h"

#include <fstream>

namespace Ogre
{
    namespace v1
    {
        //---------------------------------------------------------------------
        Skeleton::Skeleton() :
            Resource(),
            mBlendState( ANIMBLEND_AVERAGE ),
            mNextAutoHandle( 0 ),
            mManualBonesDirty( false )
        {
        }
        //---------------------------------------------------------------------
        Skeleton::Skeleton( ResourceManager *creator, const String &name, ResourceHandle handle,
                            const String &group, bool isManual, ManualResourceLoader *loader ) :
            Resource( creator, name, handle, group, isManual, loader ),
            mBlendState( ANIMBLEND_AVERAGE ),
            mNextAutoHandle( 0 )
        // set animation blending to weighted, not cumulative
        {
            if( createParamDictionary( "Skeleton" ) )
            {
                // no custom params
            }
        }
        //---------------------------------------------------------------------
        Skeleton::~Skeleton()
        {
            // have to call this here rather than in Resource destructor
            // since calling virtual methods in base destructors causes crash
            unload();
        }
        //---------------------------------------------------------------------
        void Skeleton::loadImpl()
        {
            SkeletonSerializer serializer;
            LogManager::getSingleton().logMessage( "Skeleton: Loading " + mName );

            DataStreamPtr stream =
                ResourceGroupManager::getSingleton().openResource( mName, mGroup, true, this );

            serializer.importSkeleton( stream, this );

            // Load any linked skeletons
            LinkedSkeletonAnimSourceList::iterator i;
            for( i = mLinkedSkeletonAnimSourceList.begin(); i != mLinkedSkeletonAnimSourceList.end();
                 ++i )
            {
                i->pSkeleton = std::static_pointer_cast<Skeleton>(
                    OldSkeletonManager::getSingleton().load( i->skeletonName, mGroup ) );
            }
        }
        //---------------------------------------------------------------------
        void Skeleton::unloadImpl()
        {
            // destroy bones
            BoneList::iterator i;
            for( i = mBoneList.begin(); i != mBoneList.end(); ++i )
            {
                OGRE_DELETE *i;
            }
            mBoneList.clear();
            mBoneListByName.clear();
            mRootBones.clear();
            mManualBones.clear();
            mManualBonesDirty = false;

            // Destroy animations
            AnimationList::iterator ai;
            for( ai = mAnimationsList.begin(); ai != mAnimationsList.end(); ++ai )
            {
                OGRE_DELETE ai->second;
            }
            mAnimationsList.clear();

            // Remove all linked skeletons
            mLinkedSkeletonAnimSourceList.clear();
        }
        //---------------------------------------------------------------------
        OldBone *Skeleton::createBone()
        {
            // use autohandle
            return createBone( mNextAutoHandle++ );
        }
        //---------------------------------------------------------------------
        OldBone *Skeleton::createBone( const String &name )
        {
            return createBone( name, mNextAutoHandle++ );
        }
        //---------------------------------------------------------------------
        OldBone *Skeleton::createBone( unsigned short handle )
        {
            if( handle >= OGRE_MAX_NUM_BONES )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Exceeded the maximum number of bones per skeleton.",
                             "Skeleton::createBone" );
            }
            // Check handle not used
            if( handle < mBoneList.size() && mBoneList[handle] != NULL )
            {
                OGRE_EXCEPT(
                    Exception::ERR_DUPLICATE_ITEM,
                    "A bone with the handle " + StringConverter::toString( handle ) + " already exists",
                    "Skeleton::createBone" );
            }
            OldBone *ret = OGRE_NEW OldBone( handle, this );
            assert( mBoneListByName.find( ret->getName() ) == mBoneListByName.end() );
            if( mBoneList.size() <= handle )
            {
                mBoneList.resize( handle + 1 );
            }
            mBoneList[handle] = ret;
            mBoneListByName[ret->getName()] = ret;
            return ret;
        }
        //---------------------------------------------------------------------
        OldBone *Skeleton::createBone( const String &name, unsigned short handle )
        {
            if( handle >= OGRE_MAX_NUM_BONES )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Exceeded the maximum number of bones per skeleton.",
                             "Skeleton::createBone" );
            }
            // Check handle not used
            if( handle < mBoneList.size() && mBoneList[handle] != NULL )
            {
                OGRE_EXCEPT(
                    Exception::ERR_DUPLICATE_ITEM,
                    "A bone with the handle " + StringConverter::toString( handle ) + " already exists",
                    "Skeleton::createBone" );
            }
            // Check name not used
            if( mBoneListByName.find( name ) != mBoneListByName.end() )
            {
                OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                             "A bone with the name " + name + " already exists",
                             "Skeleton::createBone" );
            }
            OldBone *ret = OGRE_NEW OldBone( name, handle, this );
            if( mBoneList.size() <= handle )
            {
                mBoneList.resize( handle + 1 );
            }
            mBoneList[handle] = ret;
            mBoneListByName[name] = ret;
            return ret;
        }

        //---------------------------------------------------------------------
        OldBone *Skeleton::getRootBone() const
        {
            if( mRootBones.empty() )
            {
                deriveRootBone();
            }

            return mRootBones[0];
        }
        //---------------------------------------------------------------------
        void Skeleton::setAnimationState( const AnimationStateSet &animSet )
        {
            /*
            Algorithm:
              1. Reset all bone positions
              2. Iterate per AnimationState, if enabled get Animation and call Animation::apply
            */

            // Reset bones
            reset();

            Real weightFactor = 1.0f;
            if( mBlendState == ANIMBLEND_AVERAGE )
            {
                // Derive total weights so we can rebalance if > 1.0f
                Real totalWeights = 0.0f;
                ConstEnabledAnimationStateIterator stateIt = animSet.getEnabledAnimationStateIterator();
                while( stateIt.hasMoreElements() )
                {
                    const AnimationState *animState = stateIt.getNext();
                    // Make sure we have an anim to match implementation
                    const LinkedSkeletonAnimationSource *linked = 0;
                    if( _getAnimationImpl( animState->getAnimationName(), &linked ) )
                    {
                        totalWeights += animState->getWeight();
                    }
                }

                // Allow < 1.0f, allows fade out of all anims if required
                if( totalWeights > 1.0f )
                {
                    weightFactor = 1.0f / totalWeights;
                }
            }

            // Per enabled animation state
            ConstEnabledAnimationStateIterator stateIt = animSet.getEnabledAnimationStateIterator();
            while( stateIt.hasMoreElements() )
            {
                const AnimationState *animState = stateIt.getNext();
                const LinkedSkeletonAnimationSource *linked = 0;
                Animation *anim = _getAnimationImpl( animState->getAnimationName(), &linked );
                // tolerate state entries for animations we're not aware of
                if( anim )
                {
                    if( animState->hasBlendMask() )
                    {
                        anim->apply( this, animState->getTimePosition(),
                                     animState->getWeight() * weightFactor, animState->getBlendMask(),
                                     linked ? linked->scale : 1.0f );
                    }
                    else
                    {
                        anim->apply( this, animState->getTimePosition(),
                                     animState->getWeight() * weightFactor,
                                     linked ? linked->scale : 1.0f );
                    }
                }
            }
        }
        //---------------------------------------------------------------------
        void Skeleton::setBindingPose()
        {
            // Update the derived transforms
            _updateTransforms();

            BoneList::iterator i;
            for( i = mBoneList.begin(); i != mBoneList.end(); ++i )
            {
                ( *i )->setBindingPose();
            }
        }
        //---------------------------------------------------------------------
        void Skeleton::reset( bool resetManualBones )
        {
            BoneList::iterator i;
            for( i = mBoneList.begin(); i != mBoneList.end(); ++i )
            {
                if( !( *i )->isManuallyControlled() || resetManualBones )
                    ( *i )->reset();
            }
        }
        //---------------------------------------------------------------------
        Animation *Skeleton::createAnimation( const String &name, Real length )
        {
            // Check name not used
            if( mAnimationsList.find( name ) != mAnimationsList.end() )
            {
                OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                             "An animation with the name " + name + " already exists",
                             "Skeleton::createAnimation" );
            }

            Animation *ret = OGRE_NEW Animation( name, length );
            ret->_notifyContainer( this );

            // Add to list
            mAnimationsList[name] = ret;

            return ret;
        }
        //---------------------------------------------------------------------
        Animation *Skeleton::getAnimation( const String &name,
                                           const LinkedSkeletonAnimationSource **linker ) const
        {
            Animation *ret = _getAnimationImpl( name, linker );
            if( !ret )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "No animation entry found named " + name,
                             "Skeleton::getAnimation" );
            }

            return ret;
        }
        //---------------------------------------------------------------------
        Animation *Skeleton::getAnimation( const String &name ) const { return getAnimation( name, 0 ); }
        //---------------------------------------------------------------------
        bool Skeleton::hasAnimation( const String &name ) const
        {
            return _getAnimationImpl( name ) != 0;
        }
        //---------------------------------------------------------------------
        Animation *Skeleton::_getAnimationImpl( const String &name,
                                                const LinkedSkeletonAnimationSource **linker ) const
        {
            Animation *ret = 0;
            AnimationList::const_iterator i = mAnimationsList.find( name );

            if( i == mAnimationsList.end() )
            {
                LinkedSkeletonAnimSourceList::const_iterator it;
                for( it = mLinkedSkeletonAnimSourceList.begin();
                     it != mLinkedSkeletonAnimSourceList.end() && !ret; ++it )
                {
                    if( it->pSkeleton )
                    {
                        ret = it->pSkeleton->_getAnimationImpl( name );
                        if( ret && linker )
                        {
                            *linker = &( *it );
                        }
                    }
                }
            }
            else
            {
                if( linker )
                    *linker = 0;
                ret = i->second;
            }

            return ret;
        }
        //---------------------------------------------------------------------
        void Skeleton::removeAnimation( const String &name )
        {
            AnimationList::iterator i = mAnimationsList.find( name );

            if( i == mAnimationsList.end() )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "No animation entry found named " + name,
                             "Skeleton::getAnimation" );
            }

            OGRE_DELETE i->second;

            mAnimationsList.erase( i );
        }
        //-----------------------------------------------------------------------
        void Skeleton::_initAnimationState( AnimationStateSet *animSet )
        {
            animSet->removeAllAnimationStates();

            AnimationList::iterator i;
            for( i = mAnimationsList.begin(); i != mAnimationsList.end(); ++i )
            {
                Animation *anim = i->second;
                // Create animation at time index 0, default params mean this has weight 1 and is
                // disabled
                const String &animName = anim->getName();
                animSet->createAnimationState( animName, 0.0, anim->getLength() );
            }

            // Also iterate over linked animation
            LinkedSkeletonAnimSourceList::iterator li;
            for( li = mLinkedSkeletonAnimSourceList.begin(); li != mLinkedSkeletonAnimSourceList.end();
                 ++li )
            {
                if( li->pSkeleton )
                {
                    li->pSkeleton->_refreshAnimationState( animSet );
                }
            }
        }
        //-----------------------------------------------------------------------
        void Skeleton::_refreshAnimationState( AnimationStateSet *animSet )
        {
            // Merge in any new animations
            AnimationList::iterator i;
            for( i = mAnimationsList.begin(); i != mAnimationsList.end(); ++i )
            {
                Animation *anim = i->second;
                // Create animation at time index 0, default params mean this has weight 1 and is
                // disabled
                const String &animName = anim->getName();
                if( !animSet->hasAnimationState( animName ) )
                {
                    animSet->createAnimationState( animName, 0.0, anim->getLength() );
                }
                else
                {
                    // Update length incase changed
                    AnimationState *animState = animSet->getAnimationState( animName );
                    animState->setLength( anim->getLength() );
                    animState->setTimePosition(
                        std::min( anim->getLength(), animState->getTimePosition() ) );
                }
            }
            // Also iterate over linked animation
            LinkedSkeletonAnimSourceList::iterator li;
            for( li = mLinkedSkeletonAnimSourceList.begin(); li != mLinkedSkeletonAnimSourceList.end();
                 ++li )
            {
                if( li->pSkeleton )
                {
                    li->pSkeleton->_refreshAnimationState( animSet );
                }
            }
        }
        //-----------------------------------------------------------------------
        void Skeleton::_notifyManualBonesDirty() { mManualBonesDirty = true; }
        //-----------------------------------------------------------------------
        void Skeleton::_notifyManualBoneStateChange( OldBone *bone )
        {
            if( bone->isManuallyControlled() )
                mManualBones.insert( bone );
            else
                mManualBones.erase( bone );
        }
        //-----------------------------------------------------------------------
        unsigned short Skeleton::getNumBones() const { return (unsigned short)mBoneList.size(); }
        //-----------------------------------------------------------------------
        void Skeleton::_getBoneMatrices( Matrix4 *pMatrices )
        {
            // Update derived transforms
            _updateTransforms();

            /*
                Calculating the bone matrices
                -----------------------------
                Now that we have the derived scaling factors, orientations & positions in the
                OldBone nodes, we have to compute the Matrix4 to apply to the vertices of a mesh.
                Because any modification of a vertex has to be relative to the bone, we must
                first reverse transform by the OldBone's original derived position/orientation/scale,
                then transform by the new derived position/orientation/scale.
                Also note we combine scale as equivalent axes, no shearing.
            */

            BoneList::const_iterator i, boneend;
            boneend = mBoneList.end();
            for( i = mBoneList.begin(); i != boneend; ++i )
            {
                OldBone *pBone = *i;
                pBone->_getOffsetTransform( *pMatrices );
                pMatrices++;
            }
        }
        //---------------------------------------------------------------------
        unsigned short Skeleton::getNumAnimations() const
        {
            return (unsigned short)mAnimationsList.size();
        }
        //---------------------------------------------------------------------
        Animation *Skeleton::getAnimation( unsigned short index ) const
        {
            // If you hit this assert, then the index is out of bounds.
            assert( index < mAnimationsList.size() );

            AnimationList::const_iterator i = mAnimationsList.begin();

            std::advance( i, index );

            return i->second;
        }
        //---------------------------------------------------------------------
        OldBone *Skeleton::getBone( unsigned short handle ) const
        {
            assert( handle < mBoneList.size() && "Index out of bounds" );
            return mBoneList[handle];
        }
        //---------------------------------------------------------------------
        OldBone *Skeleton::getBone( const String &name ) const
        {
            BoneListByName::const_iterator i = mBoneListByName.find( name );

            if( i == mBoneListByName.end() )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "OldBone named '" + name + "' not found.",
                             "Skeleton::getBone" );
            }

            return i->second;
        }
        //---------------------------------------------------------------------
        bool Skeleton::hasBone( const String &name ) const
        {
            return mBoneListByName.find( name ) != mBoneListByName.end();
        }
        //---------------------------------------------------------------------
        void Skeleton::deriveRootBone() const
        {
            // Start at the first bone and work up
            if( mBoneList.empty() )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Cannot derive root bone as this "
                             "skeleton has no bones!",
                             "Skeleton::deriveRootBone" );
            }

            mRootBones.clear();

            BoneList::const_iterator i;
            BoneList::const_iterator iend = mBoneList.end();
            for( i = mBoneList.begin(); i != iend; ++i )
            {
                OldBone *currentBone = *i;
                if( currentBone->getParent() == 0 )
                {
                    // This is a root
                    mRootBones.push_back( currentBone );
                }
            }
        }
        //---------------------------------------------------------------------
        void Skeleton::_dumpContents( const String &filename )
        {
            std::ofstream of;

            Quaternion q;
            Radian angle;
            Vector3 axis;
            of.open( filename.c_str() );

            of << "-= Debug output of skeleton " << mName << " =-" << std::endl << std::endl;
            of << "== Bones ==" << std::endl;
            of << "Number of bones: " << (unsigned int)mBoneList.size() << std::endl;

            BoneList::iterator bi;
            for( bi = mBoneList.begin(); bi != mBoneList.end(); ++bi )
            {
                OldBone *bone = *bi;

                of << "-- OldBone " << bone->getHandle() << " --" << std::endl;
                of << "Position: " << bone->getPosition();
                q = bone->getOrientation();
                of << "Rotation: " << q;
                q.ToAngleAxis( angle, axis );
                of << " = " << angle.valueRadians() << " radians around axis " << axis << std::endl
                   << std::endl;
            }

            of << "== Animations ==" << std::endl;
            of << "Number of animations: " << (unsigned int)mAnimationsList.size() << std::endl;

            AnimationList::iterator ai;
            for( ai = mAnimationsList.begin(); ai != mAnimationsList.end(); ++ai )
            {
                Animation *anim = ai->second;

                of << "-- Animation '" << anim->getName() << "' (length " << anim->getLength() << ") --"
                   << std::endl;
                of << "Number of tracks: " << anim->getNumNodeTracks() << std::endl;

                for( unsigned short ti = 0; ti < anim->getNumNodeTracks(); ++ti )
                {
                    OldNodeAnimationTrack *track = anim->getOldNodeTrack( ti );
                    of << "  -- AnimationTrack " << ti << " --" << std::endl;
                    of << "  Affects bone: " << ( (OldBone *)track->getAssociatedNode() )->getHandle()
                       << std::endl;
                    of << "  Number of keyframes: " << track->getNumKeyFrames() << std::endl;

                    for( unsigned short ki = 0; ki < track->getNumKeyFrames(); ++ki )
                    {
                        TransformKeyFrame *key = track->getNodeKeyFrame( ki );
                        of << "    -- KeyFrame " << ki << " --" << std::endl;
                        of << "    Time index: " << key->getTime();
                        of << "    Translation: " << key->getTranslate() << std::endl;
                        q = key->getRotation();
                        of << "    Rotation: " << q;
                        q.ToAngleAxis( angle, axis );
                        of << " = " << angle.valueRadians() << " radians around axis " << axis
                           << std::endl;
                    }
                }
            }
        }
        //---------------------------------------------------------------------
        SkeletonAnimationBlendMode Skeleton::getBlendMode() const { return mBlendState; }
        //---------------------------------------------------------------------
        void Skeleton::setBlendMode( SkeletonAnimationBlendMode state ) { mBlendState = state; }
        //---------------------------------------------------------------------
        Skeleton::BoneIterator Skeleton::getRootBoneIterator()
        {
            if( mRootBones.empty() )
            {
                deriveRootBone();
            }
            return BoneIterator( mRootBones.begin(), mRootBones.end() );
        }
        //---------------------------------------------------------------------
        Skeleton::BoneIterator Skeleton::getBoneIterator()
        {
            return BoneIterator( mBoneList.begin(), mBoneList.end() );
        }
        //---------------------------------------------------------------------
        Skeleton::ConstBoneIterator Skeleton::getBoneIteratorConst() const
        {
            return ConstBoneIterator( mBoneList.begin(), mBoneList.end() );
        }
        //---------------------------------------------------------------------
        void Skeleton::_updateTransforms()
        {
            BoneList::iterator i, iend;
            iend = mRootBones.end();
            for( i = mRootBones.begin(); i != iend; ++i )
            {
                ( *i )->_update( true, false );
            }
            mManualBonesDirty = false;
        }
        //---------------------------------------------------------------------
        void Skeleton::optimiseAllAnimations( bool preservingIdentityNodeTracks )
        {
            AnimationList::iterator ai, aiend;
            aiend = mAnimationsList.end();

            if( !preservingIdentityNodeTracks )
            {
                Animation::TrackHandleList tracksToDestroy;

                // Assume all node tracks are identity
                ushort numBones = getNumBones();
                for( ushort h = 0; h < numBones; ++h )
                {
                    tracksToDestroy.insert( h );
                }

                // Collect identity node tracks for all animations
                for( ai = mAnimationsList.begin(); ai != aiend; ++ai )
                {
                    ai->second->_collectIdentityOldNodeTracks( tracksToDestroy );
                }

                // Destroy identity node tracks
                for( ai = mAnimationsList.begin(); ai != aiend; ++ai )
                {
                    ai->second->_destroyOldNodeTracks( tracksToDestroy );
                }
            }

            for( ai = mAnimationsList.begin(); ai != aiend; ++ai )
            {
                // Don't discard identity node tracks here
                ai->second->optimise( false );
            }
        }
        //---------------------------------------------------------------------
        void Skeleton::addLinkedSkeletonAnimationSource( const String &skelName, Real scale )
        {
            // Check not already linked
            LinkedSkeletonAnimSourceList::iterator i;
            for( i = mLinkedSkeletonAnimSourceList.begin(); i != mLinkedSkeletonAnimSourceList.end();
                 ++i )
            {
                if( skelName == i->skeletonName )
                    return;  // don't bother
            }

            if( isLoaded() )
            {
                // Load immediately
                SkeletonPtr skelPtr = std::static_pointer_cast<Skeleton>(
                    OldSkeletonManager::getSingleton().load( skelName, mGroup ) );
                mLinkedSkeletonAnimSourceList.push_back(
                    LinkedSkeletonAnimationSource( skelName, scale, skelPtr ) );
            }
            else
            {
                // Load later
                mLinkedSkeletonAnimSourceList.push_back(
                    LinkedSkeletonAnimationSource( skelName, scale ) );
            }
        }
        //---------------------------------------------------------------------
        void Skeleton::removeAllLinkedSkeletonAnimationSources()
        {
            mLinkedSkeletonAnimSourceList.clear();
        }
        //---------------------------------------------------------------------
        Skeleton::LinkedSkeletonAnimSourceIterator Skeleton::getLinkedSkeletonAnimationSourceIterator()
            const
        {
            return LinkedSkeletonAnimSourceIterator( mLinkedSkeletonAnimSourceList.begin(),
                                                     mLinkedSkeletonAnimSourceList.end() );
        }
        //---------------------------------------------------------------------
        struct DeltaTransform
        {
            Vector3 translate;
            Quaternion rotate;
            Vector3 scale;
            bool isIdentity;
        };
        void Skeleton::_mergeSkeletonAnimations( const Skeleton *src, const BoneHandleMap &boneHandleMap,
                                                 const StringVector &animations )
        {
            ushort handle;

            ushort numSrcBones = src->getNumBones();
            ushort numDstBones = this->getNumBones();

            if( boneHandleMap.size() != numSrcBones )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Number of bones in the bone handle map must equal to "
                             "number of bones in the source skeleton.",
                             "Skeleton::_mergeSkeletonAnimations" );
            }

            bool existsMissingBone = false;

            // Check source skeleton structures compatible with ourself (that means
            // identically bones with identical handles, and with same hierarchy, but
            // not necessary to have same number of bones and bone names).
            for( handle = 0; handle < numSrcBones; ++handle )
            {
                const OldBone *srcBone = src->getBone( handle );
                ushort dstHandle = boneHandleMap[handle];

                // Does it exists in target skeleton?
                if( dstHandle < numDstBones )
                {
                    OldBone *destBone = this->getBone( dstHandle );

                    // Check both bones have identical parent, or both are root bone.
                    const OldBone *srcParent = static_cast<OldBone *>( srcBone->getParent() );
                    OldBone *destParent = static_cast<OldBone *>( destBone->getParent() );
                    if( ( srcParent || destParent ) &&
                        ( !srcParent || !destParent ||
                          boneHandleMap[srcParent->getHandle()] != destParent->getHandle() ) )
                    {
                        OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                     "Source skeleton incompatible with this skeleton: "
                                     "difference hierarchy between bone '" +
                                         srcBone->getName() + "' and '" + destBone->getName() + "'.",
                                     "Skeleton::_mergeSkeletonAnimations" );
                    }
                }
                else
                {
                    existsMissingBone = true;
                }
            }

            // Clone bones if need
            if( existsMissingBone )
            {
                // Create missing bones
                for( handle = 0; handle < numSrcBones; ++handle )
                {
                    const OldBone *srcBone = src->getBone( handle );
                    ushort dstHandle = boneHandleMap[handle];

                    // The bone is missing in target skeleton?
                    if( dstHandle >= numDstBones )
                    {
                        OldBone *dstBone = this->createBone( srcBone->getName(), dstHandle );
                        // Sets initial transform
                        dstBone->setPosition( srcBone->getInitialPosition() );
                        dstBone->setOrientation( srcBone->getInitialOrientation() );
                        dstBone->setScale( srcBone->getInitialScale() );
                        dstBone->setInitialState();
                    }
                }

                // Link new bones to parent
                for( handle = 0; handle < numSrcBones; ++handle )
                {
                    const OldBone *srcBone = src->getBone( handle );
                    ushort dstHandle = boneHandleMap[handle];

                    // Is new bone?
                    if( dstHandle >= numDstBones )
                    {
                        const OldBone *srcParent = static_cast<OldBone *>( srcBone->getParent() );
                        if( srcParent )
                        {
                            OldBone *destParent = this->getBone( boneHandleMap[srcParent->getHandle()] );
                            OldBone *dstBone = this->getBone( dstHandle );
                            destParent->addChild( dstBone );
                        }
                    }
                }

                // Derive root bones in case it was changed
                this->deriveRootBone();

                // Reset binding pose for new bones
                this->reset( true );
                this->setBindingPose();
            }

            //
            // We need to adapt animations from source to target skeleton, but since source
            // and target skeleton bones bind transform might difference, so we need to alter
            // keyframes in source to suit to target skeleton.
            //
            // For any given animation time, formula:
            //
            //      LocalTransform = BindTransform * KeyFrame;
            //      DerivedTransform = ParentDerivedTransform * LocalTransform
            //
            // And all derived transforms should be keep identically after adapt to
            // target skeleton, Then:
            //
            //      DestDerivedTransform == SrcDerivedTransform
            //      DestParentDerivedTransform == SrcParentDerivedTransform
            // ==>
            //      DestLocalTransform = SrcLocalTransform
            // ==>
            //      DestBindTransform * DestKeyFrame = SrcBindTransform * SrcKeyFrame
            // ==>
            //      DestKeyFrame = inverse(DestBindTransform) * SrcBindTransform * SrcKeyFrame
            //
            // We define (inverse(DestBindTransform) * SrcBindTransform) as 'delta-transform' here.
            //

            // Calculate delta-transforms for all source bones.
            vector<DeltaTransform>::type deltaTransforms( numSrcBones );
            for( handle = 0; handle < numSrcBones; ++handle )
            {
                const OldBone *srcBone = src->getBone( handle );
                DeltaTransform &deltaTransform = deltaTransforms[handle];
                ushort dstHandle = boneHandleMap[handle];

                if( dstHandle < numDstBones )
                {
                    // Common bone, calculate delta-transform

                    OldBone *dstBone = this->getBone( dstHandle );

                    deltaTransform.translate =
                        srcBone->getInitialPosition() - dstBone->getInitialPosition();
                    deltaTransform.rotate =
                        dstBone->getInitialOrientation().Inverse() * srcBone->getInitialOrientation();
                    deltaTransform.scale = srcBone->getInitialScale() / dstBone->getInitialScale();

                    // Check whether or not delta-transform is identity
                    const Real tolerance = 1e-3f;
                    Vector3 axis;
                    Radian angle;
                    deltaTransform.rotate.ToAngleAxis( angle, axis );
                    deltaTransform.isIdentity =
                        deltaTransform.translate.positionEquals( Vector3::ZERO, tolerance ) &&
                        deltaTransform.scale.positionEquals( Vector3::UNIT_SCALE, tolerance ) &&
                        Math::RealEqual( angle.valueRadians(), 0.0f, tolerance );
                }
                else
                {
                    // New bone, the delta-transform is identity

                    deltaTransform.translate = Vector3::ZERO;
                    deltaTransform.rotate = Quaternion::IDENTITY;
                    deltaTransform.scale = Vector3::UNIT_SCALE;
                    deltaTransform.isIdentity = true;
                }
            }

            // Now copy animations

            ushort numAnimations;
            if( animations.empty() )
                numAnimations = src->getNumAnimations();
            else
                numAnimations = static_cast<ushort>( animations.size() );
            for( ushort i = 0; i < numAnimations; ++i )
            {
                const Animation *srcAnimation;
                if( animations.empty() )
                {
                    // Get animation of source skeleton by the given index
                    srcAnimation = src->getAnimation( i );
                }
                else
                {
                    // Get animation of source skeleton by the given name
                    const LinkedSkeletonAnimationSource *linker;
                    srcAnimation = src->_getAnimationImpl( animations[i], &linker );
                    if( !srcAnimation || linker )
                    {
                        OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                     "No animation entry found named " + animations[i],
                                     "Skeleton::_mergeSkeletonAnimations" );
                    }
                }

                // Create target animation
                Animation *dstAnimation =
                    this->createAnimation( srcAnimation->getName(), srcAnimation->getLength() );

                // Copy interpolation modes
                dstAnimation->setInterpolationMode( srcAnimation->getInterpolationMode() );
                dstAnimation->setRotationInterpolationMode(
                    srcAnimation->getRotationInterpolationMode() );

                // Copy track for each bone
                for( handle = 0; handle < numSrcBones; ++handle )
                {
                    const DeltaTransform &deltaTransform = deltaTransforms[handle];
                    ushort dstHandle = boneHandleMap[handle];

                    if( srcAnimation->hasOldNodeTrack( handle ) )
                    {
                        // Clone track from source animation

                        const OldNodeAnimationTrack *srcTrack = srcAnimation->getOldNodeTrack( handle );
                        OldNodeAnimationTrack *dstTrack =
                            dstAnimation->createOldNodeTrack( dstHandle, this->getBone( dstHandle ) );
                        dstTrack->setUseShortestRotationPath( srcTrack->getUseShortestRotationPath() );

                        ushort numKeyFrames = srcTrack->getNumKeyFrames();
                        for( ushort k = 0; k < numKeyFrames; ++k )
                        {
                            const TransformKeyFrame *srcKeyFrame = srcTrack->getNodeKeyFrame( k );
                            TransformKeyFrame *dstKeyFrame =
                                dstTrack->createNodeKeyFrame( srcKeyFrame->getTime() );

                            // Adjust keyframes to match target binding pose
                            if( deltaTransform.isIdentity )
                            {
                                dstKeyFrame->setTranslate( srcKeyFrame->getTranslate() );
                                dstKeyFrame->setRotation( srcKeyFrame->getRotation() );
                                dstKeyFrame->setScale( srcKeyFrame->getScale() );
                            }
                            else
                            {
                                dstKeyFrame->setTranslate( deltaTransform.translate +
                                                           srcKeyFrame->getTranslate() );
                                dstKeyFrame->setRotation( deltaTransform.rotate *
                                                          srcKeyFrame->getRotation() );
                                dstKeyFrame->setScale( deltaTransform.scale * srcKeyFrame->getScale() );
                            }
                        }
                    }
                    else if( !deltaTransform.isIdentity )
                    {
                        // Create 'static' track for this bone

                        OldNodeAnimationTrack *dstTrack =
                            dstAnimation->createOldNodeTrack( dstHandle, this->getBone( dstHandle ) );
                        TransformKeyFrame *dstKeyFrame;

                        dstKeyFrame = dstTrack->createNodeKeyFrame( 0 );
                        dstKeyFrame->setTranslate( deltaTransform.translate );
                        dstKeyFrame->setRotation( deltaTransform.rotate );
                        dstKeyFrame->setScale( deltaTransform.scale );

                        dstKeyFrame = dstTrack->createNodeKeyFrame( dstAnimation->getLength() );
                        dstKeyFrame->setTranslate( deltaTransform.translate );
                        dstKeyFrame->setRotation( deltaTransform.rotate );
                        dstKeyFrame->setScale( deltaTransform.scale );
                    }
                }
            }
        }
        //---------------------------------------------------------------------
        void Skeleton::_buildMapBoneByHandle( const Skeleton *src, BoneHandleMap &boneHandleMap ) const
        {
            ushort numSrcBones = src->getNumBones();
            boneHandleMap.resize( numSrcBones );

            for( ushort handle = 0; handle < numSrcBones; ++handle )
            {
                boneHandleMap[handle] = handle;
            }
        }
        //---------------------------------------------------------------------
        void Skeleton::_buildMapBoneByName( const Skeleton *src, BoneHandleMap &boneHandleMap ) const
        {
            ushort numSrcBones = src->getNumBones();
            boneHandleMap.resize( numSrcBones );

            ushort newBoneHandle = this->getNumBones();
            for( ushort handle = 0; handle < numSrcBones; ++handle )
            {
                const OldBone *srcBone = src->getBone( handle );
                BoneListByName::const_iterator i = this->mBoneListByName.find( srcBone->getName() );
                if( i == mBoneListByName.end() )
                    boneHandleMap[handle] = newBoneHandle++;
                else
                    boneHandleMap[handle] = i->second->getHandle();
            }
        }

        size_t Skeleton::calculateSize() const
        {
            size_t memSize = 0;
            memSize += sizeof( SkeletonAnimationBlendMode );
            memSize += mBoneList.size() * sizeof( OldBone );
            memSize += mRootBones.size() * sizeof( OldBone );
            memSize += mBoneListByName.size() * ( sizeof( String ) + sizeof( OldBone * ) );
            memSize += mAnimationsList.size() * ( sizeof( String ) + sizeof( Animation * ) );
            memSize += mManualBones.size() * sizeof( OldBone * );
            memSize += mLinkedSkeletonAnimSourceList.size() * sizeof( LinkedSkeletonAnimationSource );
            memSize += sizeof( bool );

            return memSize;
        }
    }  // namespace v1
}  // namespace Ogre
