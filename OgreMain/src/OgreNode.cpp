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

#include "OgreNode.h"

#include "Math/Array/OgreBooleanMask.h"
#include "Math/Array/OgreNodeMemoryManager.h"
#include "OgreCamera.h"
#include "OgreException.h"
#include "OgreMath.h"
#include "OgreStringConverter.h"

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
#    define CACHED_TRANSFORM_OUT_OF_DATE() this->_setCachedTransformOutOfDate()
#else
#    define CACHED_TRANSFORM_OUT_OF_DATE() ( (void)0 )
#endif

namespace Ogre
{
    //-----------------------------------------------------------------------
    Node::Node( IdType id, NodeMemoryManager *nodeMemoryManager, Node *parent ) :
        IdObject( id ),
        mDepthLevel( 0 ),
        mIndestructibleByClearScene( false ),
        mParent( parent ),
        mName( "" ),
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        mCachedTransformOutOfDate( true ),
#endif
        mListener( 0 ),
        mNodeMemoryManager( nodeMemoryManager ),
        mGlobalIndex( std::numeric_limits<size_t>::max() ),
        mParentIndex( std::numeric_limits<size_t>::max() )
    {
        if( mParent )
            mDepthLevel = mParent->mDepthLevel + 1;

        // Will initialize mTransform
        mNodeMemoryManager->nodeCreated( mTransform, mDepthLevel );
        mTransform.mOwner[mTransform.mIndex] = this;
        if( mParent )
            mTransform.mParents[mTransform.mIndex] = mParent;
    }
    //-----------------------------------------------------------------------
    Node::Node( const Transform &transformPtrs ) :
        IdObject( 0 ),
        mDepthLevel( 0 ),
        mParent( 0 ),
        mName( "Dummy Node" ),
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        mCachedTransformOutOfDate( true ),
#endif
        mListener( 0 ),
        mNodeMemoryManager( 0 ),
        mGlobalIndex( std::numeric_limits<size_t>::max() ),
        mParentIndex( std::numeric_limits<size_t>::max() )
    {
        mTransform = transformPtrs;
    }
    //-----------------------------------------------------------------------
    Node::~Node()
    {
        // Call listener (note, only called if there's something to do)
        if( mListener )
        {
            mListener->nodeDestroyed( this );
        }

        // Calling removeAllChildren or mParent->removeChild before nodeDestroyed may
        // result in an enlargement of the array memory buffer or in a cleanup.
        // Either way, the memory manager could end up calling a virtual function;
        // but our vtable is wrong by now. So we destroy our slot first.
        if( mNodeMemoryManager )
            mNodeMemoryManager->nodeDestroyed( mTransform, mDepthLevel );
        mDepthLevel = 0;

        removeAllChildren();
        if( mParent )
        {
            Node *parent = mParent;
            mParent = 0;  // We've already called mNodeMemoryManager->nodeDestroyed.
            parent->removeChild( this );
        }
    }
    //-----------------------------------------------------------------------
    Node *Node::getParent() const { return mParent; }
    //-----------------------------------------------------------------------
    void Node::setIndestructibleByClearScene( bool indestructible )
    {
        mIndestructibleByClearScene = indestructible;
    }
    //-----------------------------------------------------------------------
    bool Node::getIndestructibleByClearScene() const { return mIndestructibleByClearScene; }
    //-----------------------------------------------------------------------
    void Node::migrateTo( NodeMemoryManager *nodeMemoryManager )
    {
        for( Node *child : mChildren )
            child->migrateTo( nodeMemoryManager );

        mNodeMemoryManager->migrateTo( mTransform, mDepthLevel, nodeMemoryManager );
        mNodeMemoryManager = nodeMemoryManager;
        _callMemoryChangeListeners();
    }
    //-----------------------------------------------------------------------
    bool Node::isStatic() const { return mNodeMemoryManager->getMemoryManagerType() == SCENE_STATIC; }
    //-----------------------------------------------------------------------
    bool Node::setStatic( bool bStatic )
    {
        bool retVal = false;
        if( mNodeMemoryManager->getTwin() &&
            ( ( mNodeMemoryManager->getMemoryManagerType() == SCENE_STATIC && !bStatic ) ||
              ( mNodeMemoryManager->getMemoryManagerType() == SCENE_DYNAMIC && bStatic ) ) )
        {
            mNodeMemoryManager->migrateTo( mTransform, mDepthLevel, mNodeMemoryManager->getTwin() );
            mNodeMemoryManager = mNodeMemoryManager->getTwin();
            _callMemoryChangeListeners();
            retVal = true;
        }

        return retVal;
    }
    //-----------------------------------------------------------------------
    void Node::setParent( Node *parent )
    {
        if( mParent != parent )
        {
            mParent = parent;

            // Call listener
            if( mListener )
                mListener->nodeAttached( this );

            size_t oldDepthLevel = mDepthLevel;

            // NodeMemoryManager will set mTransform.mParents to a dummy parent node
            //(as well as transfering the memory)
            mNodeMemoryManager->nodeDettached( mTransform, mDepthLevel );

            mDepthLevel = mParent->mDepthLevel + 1;
            mTransform.mParents[mTransform.mIndex] = parent;

            const bool differentNodeMemoryManager = mParent->mNodeMemoryManager != mNodeMemoryManager;

            if( differentNodeMemoryManager )
            {
                mNodeMemoryManager->migrateToAndAttach( mTransform, mDepthLevel,
                                                        mParent->mNodeMemoryManager );
                mNodeMemoryManager = mParent->mNodeMemoryManager;
            }
            else
            {
                mNodeMemoryManager->nodeAttached( mTransform, mDepthLevel );
            }

            if( oldDepthLevel != mDepthLevel || differentNodeMemoryManager )
            {
                // Propagate the change to our children
                for( Node *child : mChildren )
                    child->parentDepthLevelChanged();

                _callMemoryChangeListeners();
            }
        }
    }
    //-----------------------------------------------------------------------
    void Node::unsetParent()
    {
        if( mParent )
        {
            // Call listener
            if( mListener )
                mListener->nodeDetached( this );

            mParent = 0;

            NodeMemoryManager *defaultNodeMemoryManager =
                getDefaultNodeMemoryManager( mNodeMemoryManager->getMemoryManagerType() );

            const bool differentNodeMemoryManager = defaultNodeMemoryManager != mNodeMemoryManager;

            if( differentNodeMemoryManager )
            {
                mNodeMemoryManager->migrateToAndDetach( mTransform, mDepthLevel,
                                                        defaultNodeMemoryManager );
                mNodeMemoryManager = defaultNodeMemoryManager;
            }
            else
            {
                // NodeMemoryManager will set mTransform.mParents to a dummy parent node
                //(as well as transfering the memory)
                mNodeMemoryManager->nodeDettached( mTransform, mDepthLevel );
            }

            if( mDepthLevel != 0 || differentNodeMemoryManager )
            {
                mDepthLevel = 0;

                // Propagate the change to our children
                for( Node *child : mChildren )
                    child->parentDepthLevelChanged();

                _callMemoryChangeListeners();
            }
        }
    }
    //-----------------------------------------------------------------------
    void Node::parentDepthLevelChanged()
    {
        if( mNodeMemoryManager != mParent->mNodeMemoryManager )
        {
            mNodeMemoryManager->migrateTo( mTransform, mDepthLevel, mParent->mDepthLevel + 1,
                                           mParent->mNodeMemoryManager );
            mNodeMemoryManager = mParent->mNodeMemoryManager;
        }
        else
        {
            mNodeMemoryManager->nodeMoved( mTransform, mDepthLevel, mParent->mDepthLevel + 1 );
        }

        mDepthLevel = mParent->mDepthLevel + 1;

        _callMemoryChangeListeners();

        // Keep propagating changes to our children
        for( Node *child : mChildren )
            child->parentDepthLevelChanged();
    }
    //-----------------------------------------------------------------------
    /*const Matrix4& Node::_getFullTransform() const
    {
        assert( !mCachedTransformOutOfDate );
        return mTransform.mDerivedTransform[mTransform.mIndex];
    }*/
    //-----------------------------------------------------------------------
    const Matrix4 &Node::_getFullTransformUpdated()
    {
        _updateFromParent();
        return mTransform.mDerivedTransform[mTransform.mIndex];
    }
    //-----------------------------------------------------------------------
    void Node::_updateChildren()
    {
        updateFromParentImpl();

        // Call listener (note, this method only called if there's something to do)
        if( mListener )
        {
            mListener->nodeUpdated( this );
        }

        // Keep propagating changes to our children
        for( Node *child : mChildren )
            child->_updateChildren();
    }
    //-----------------------------------------------------------------------
    void Node::_updateFromParent()
    {
        if( mParent )
            mParent->_updateFromParent();

        updateFromParentImpl();

        // Call listener (note, this method only called if there's something to do)
        if( mListener )
        {
            mListener->nodeUpdated( this );
        }
    }
    //-----------------------------------------------------------------------
    void Node::updateFromParentImpl()
    {
#if OGRE_NODE_INHERIT_TRANSFORM
        // determine our transform, without parent part
        ArrayMatrix4 trSoA;
        trSoA.makeTransform( *mTransform.mPosition, *mTransform.mScale, *mTransform.mOrientation );

        for( size_t j = 0; j < ARRAY_PACKED_REALS; ++j )
        {
            const Transform &parentTransform = mTransform.mParents[j]->mTransform;
            const Matrix4 &parentFullTransform =
                parentTransform.mDerivedTransform[parentTransform.mIndex];

            Matrix4 tr;
            trSoA.getAsMatrix4( tr, j );

            if( mTransform.mInheritOrientation[j] &&
                mTransform.mInheritScale[j] )  // everything is inherited
            {
                mTransform.mDerivedTransform[j] = parentFullTransform * tr;
            }
            else if( !mTransform.mInheritOrientation[j] &&
                     !mTransform.mInheritScale[j] )  // only position is inherited
            {
                mTransform.mDerivedTransform[j] = tr;
                mTransform.mDerivedTransform[j].setTrans( tr.getTrans() +
                                                          parentFullTransform.getTrans() );
            }
            else  // shear is inherited together with orientation, controlled by mInheritOrientation
            {
                Ogre::Vector3 parentScale(
                    parentFullTransform.transformDirectionAffine( Vector3::UNIT_X ).length(),
                    parentFullTransform.transformDirectionAffine( Vector3::UNIT_Y ).length(),
                    parentFullTransform.transformDirectionAffine( Vector3::UNIT_Z ).length() );

                assert( mTransform.mInheritOrientation[j] ^ mTransform.mInheritScale[j] );
                mTransform.mDerivedTransform[j] =
                    mTransform.mInheritOrientation[j]
                        ? Matrix4::getScale( 1.0f / parentScale ) * parentFullTransform * tr
                        : Matrix4::getScale( parentScale ) * tr;
            }

            // Decompose full transform to position, orientation and scale, shear is lost here.
            Vector3 pos, scale;
            Quaternion qRot;
            mTransform.mDerivedTransform[j].decomposition( pos, scale, qRot );
            mTransform.mDerivedPosition->setFromVector3( pos, j );
            mTransform.mDerivedScale->setFromVector3( scale, j );
            mTransform.mDerivedOrientation->setFromQuaternion( qRot, j );
        }
#else
        // Retrieve from parents. Unfortunately we need to do SoA -> AoS -> SoA conversion
        ArrayVector3 parentPos, parentScale;
        ArrayQuaternion parentRot;

        for( size_t j = 0; j < ARRAY_PACKED_REALS; ++j )
        {
            Vector3 pos, nodeScale;
            Quaternion qRot;
            const Transform &parentTransform = mTransform.mParents[j]->mTransform;
            parentTransform.mDerivedPosition->getAsVector3( pos, parentTransform.mIndex );
            parentTransform.mDerivedOrientation->getAsQuaternion( qRot, parentTransform.mIndex );
            parentTransform.mDerivedScale->getAsVector3( nodeScale, parentTransform.mIndex );

            parentPos.setFromVector3( pos, j );
            parentRot.setFromQuaternion( qRot, j );
            parentScale.setFromVector3( nodeScale, j );
        }

        parentRot.Cmov4( BooleanMask4::getMask( mTransform.mInheritOrientation ),
                         ArrayQuaternion::IDENTITY );
        parentScale.Cmov4( BooleanMask4::getMask( mTransform.mInheritScale ), ArrayVector3::UNIT_SCALE );

        // Scale own position by parent scale, NB just combine
        // as equivalent axes, no shearing
        *mTransform.mDerivedScale = parentScale * ( *mTransform.mScale );

        // Combine orientation with that of parent
        *mTransform.mDerivedOrientation = parentRot * ( *mTransform.mOrientation );

        // Change position vector based on parent's orientation & scale
        *mTransform.mDerivedPosition = parentRot * ( parentScale * ( *mTransform.mPosition ) );

        // Add altered position vector to parents
        *mTransform.mDerivedPosition += parentPos;

        ArrayMatrix4 derivedTransform;
        derivedTransform.makeTransform( *mTransform.mDerivedPosition, *mTransform.mDerivedScale,
                                        *mTransform.mDerivedOrientation );
        derivedTransform.storeToAoS( mTransform.mDerivedTransform );
#endif
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        for( size_t j = 0; j < ARRAY_PACKED_REALS; ++j )
        {
            if( mTransform.mOwner[j] )
                mTransform.mOwner[j]->mCachedTransformOutOfDate = false;
        }
#endif
    }
    //-----------------------------------------------------------------------
    void Node::updateAllTransforms( const size_t numNodes, Transform t )
    {
        ArrayMatrix4 derivedTransform;
        for( size_t i = 0; i < numNodes; i += ARRAY_PACKED_REALS )
        {
#if OGRE_NODE_INHERIT_TRANSFORM
            // determine our transform, without parent part
            ArrayMatrix4 trSoA;
            trSoA.makeTransform( *t.mPosition, *t.mScale, *t.mOrientation );

            for( size_t j = 0; j < ARRAY_PACKED_REALS; ++j )
            {
                const Transform &parentTransform = t.mParents[j]->mTransform;
                const Matrix4 &parentFullTransform =
                    parentTransform.mDerivedTransform[parentTransform.mIndex];

                Matrix4 tr;
                trSoA.getAsMatrix4( tr, j );

                if( t.mInheritOrientation[j] && t.mInheritScale[j] )  // everything is inherited
                {
                    t.mDerivedTransform[j] = parentFullTransform * tr;
                }
                else if( !t.mInheritOrientation[j] &&
                         !t.mInheritScale[j] )  // only position is inherited
                {
                    t.mDerivedTransform[j] = tr;
                    t.mDerivedTransform[j].setTrans( tr.getTrans() + parentFullTransform.getTrans() );
                }
                else  // shear is inherited together with orientation, controlled by mInheritOrientation
                {
                    Ogre::Vector3 parentScale(
                        parentFullTransform.transformDirectionAffine( Vector3::UNIT_X ).length(),
                        parentFullTransform.transformDirectionAffine( Vector3::UNIT_Y ).length(),
                        parentFullTransform.transformDirectionAffine( Vector3::UNIT_Z ).length() );

                    assert( t.mInheritOrientation[j] ^ t.mInheritScale[j] );
                    t.mDerivedTransform[j] =
                        t.mInheritOrientation[j]
                            ? Matrix4::getScale( 1.0f / parentScale ) * parentFullTransform * tr
                            : Matrix4::getScale( parentScale ) * tr;
                }

                // Decompose full transform to position, orientation and scale, shear is lost here.
                Vector3 pos, scale;
                Quaternion qRot;
                t.mDerivedTransform[j].decomposition( pos, scale, qRot );
                t.mDerivedPosition->setFromVector3( pos, j );
                t.mDerivedScale->setFromVector3( scale, j );
                t.mDerivedOrientation->setFromQuaternion( qRot, j );
            }
#else
            // Retrieve from parents. Unfortunately we need to do SoA -> AoS -> SoA conversion
            ArrayVector3 parentPos, parentScale;
            ArrayQuaternion parentRot;

            for( size_t j = 0; j < ARRAY_PACKED_REALS; ++j )
            {
                Vector3 pos, scale;
                Quaternion qRot;
                const Transform &parentTransform = t.mParents[j]->mTransform;
                parentTransform.mDerivedPosition->getAsVector3( pos, parentTransform.mIndex );
                parentTransform.mDerivedOrientation->getAsQuaternion( qRot, parentTransform.mIndex );
                parentTransform.mDerivedScale->getAsVector3( scale, parentTransform.mIndex );

                parentPos.setFromVector3( pos, j );
                parentRot.setFromQuaternion( qRot, j );
                parentScale.setFromVector3( scale, j );
            }

            // Change position vector based on parent's orientation & scale
            *t.mDerivedPosition = parentRot * ( parentScale * ( *t.mPosition ) );

            // Combine orientation with that of parent
            *t.mDerivedOrientation =
                ArrayQuaternion::Cmov4( parentRot * ( *t.mOrientation ), *t.mOrientation,
                                        BooleanMask4::getMask( t.mInheritOrientation ) );

            // Scale own position by parent scale, NB just combine
            // as equivalent axes, no shearing
            *t.mDerivedScale = ArrayVector3::Cmov4( parentScale * ( *t.mScale ), *t.mScale,
                                                    BooleanMask4::getMask( t.mInheritScale ) );

            // Add altered position vector to parents
            *t.mDerivedPosition += parentPos;

            derivedTransform.makeTransform( *t.mDerivedPosition, *t.mDerivedScale,
                                            *t.mDerivedOrientation );
            derivedTransform.storeToAoS( t.mDerivedTransform );
#endif
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
            for( size_t j = 0; j < ARRAY_PACKED_REALS; ++j )
            {
                if( t.mOwner[j] )
                    t.mOwner[j]->mCachedTransformOutOfDate = false;
            }
#endif

            t.advancePack();
        }
    }
    //-----------------------------------------------------------------------
    Node *Node::createChild( SceneMemoryMgrTypes sceneType, const Vector3 &inTranslate,
                             const Quaternion &inRotate )
    {
        Node *newNode = createChildImpl( sceneType );
        newNode->setPosition( inTranslate );
        newNode->setOrientation( inRotate );

        // createChildImpl must have passed us as parent. It's a special
        // case to improve memory usage (avoid transfering mTransform)
        mChildren.push_back( newNode );
        newNode->mParentIndex = mChildren.size() - 1;

        return newNode;
    }
    //-----------------------------------------------------------------------
    void Node::addChild( Node *child )
    {
        if( child->mParent )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Node ID: " + StringConverter::toString( child->getId() ) + ", named '" +
                             child->getName() + "' already was a child of Node ID: " +
                             StringConverter::toString( child->mParent->getId() ) + ", named '" +
                             child->mParent->getName() + "'.",
                         "Node::addChild" );
        }

        mChildren.push_back( child );
        child->mParentIndex = mChildren.size() - 1;
        child->setParent( this );
    }
    //-----------------------------------------------------------------------
    void Node::removeChild( Node *child )
    {
        // child->getParent() can be null if we're calling this
        // function from the destructor. See ~Node notes.
        assert( ( !child->getParent() || child->getParent() == this ) &&
                "Node says it's not our child" );
        assert( child->mParentIndex < mChildren.size() && "mParentIndex was out of date!!!" );

        if( child->mParentIndex < mChildren.size() )
        {
            NodeVec::iterator itor = mChildren.begin() + static_cast<ptrdiff_t>( child->mParentIndex );

            assert( child == *itor && "mParentIndex was out of date!!!" );

            if( child == *itor )
            {
                itor = efficientVectorRemove( mChildren, itor );
                child->unsetParent();
                child->mParentIndex = std::numeric_limits<size_t>::max();

                // The node that was at the end got swapped and has now a different index
                if( itor != mChildren.end() )
                    ( *itor )->mParentIndex = static_cast<size_t>( itor - mChildren.begin() );
            }
        }
    }
    //-----------------------------------------------------------------------
    void Node::_notifyStaticDirty() const
    {
        for( Node *child : mChildren )
            child->_notifyStaticDirty();
    }
    //-----------------------------------------------------------------------
    Quaternion Node::getOrientation() const
    {
        return mTransform.mOrientation->getAsQuaternion( mTransform.mIndex );
    }
    //-----------------------------------------------------------------------
    void Node::setOrientation( Quaternion q )
    {
        assert( !q.isNaN() && "Invalid orientation supplied as parameter" );
        q.normalise();
        mTransform.mOrientation->setFromQuaternion( q, mTransform.mIndex );
        CACHED_TRANSFORM_OUT_OF_DATE();
    }
    //-----------------------------------------------------------------------
    void Node::setOrientation( Real w, Real x, Real y, Real z )
    {
        setOrientation( Quaternion( w, x, y, z ) );
    }
    //-----------------------------------------------------------------------
    void Node::resetOrientation()
    {
        mTransform.mOrientation->setFromQuaternion( Quaternion::IDENTITY, mTransform.mIndex );
    }

    //-----------------------------------------------------------------------
    void Node::setPosition( const Vector3 &pos )
    {
        assert( !pos.isNaN() && "Invalid vector supplied as parameter" );
        mTransform.mPosition->setFromVector3( pos, mTransform.mIndex );
        CACHED_TRANSFORM_OUT_OF_DATE();
    }
    //-----------------------------------------------------------------------
    void Node::setPosition( Real x, Real y, Real z ) { setPosition( Vector3( x, y, z ) ); }
    //-----------------------------------------------------------------------
    Vector3 Node::getPosition() const { return mTransform.mPosition->getAsVector3( mTransform.mIndex ); }
    //-----------------------------------------------------------------------
    Matrix3 Node::getLocalAxes() const
    {
        Quaternion q;
        mTransform.mOrientation->getAsQuaternion( q, mTransform.mIndex );
        Matrix3 retVal;
        q.ToRotationMatrix( retVal );

        /* Equivalent code (easier to visualize):
        axisX = q.xAxis();
        axisY = q.yAxis();
        axisZ = q.zAxis();

        return Matrix3(axisX.x, axisY.x, axisZ.x,
                       axisX.y, axisY.y, axisZ.y,
                       axisX.z, axisY.z, axisZ.z);*/

        return retVal;
    }

    //-----------------------------------------------------------------------
    void Node::translate( const Vector3 &d, TransformSpace relativeTo )
    {
        Vector3 position;
        mTransform.mPosition->getAsVector3( position, mTransform.mIndex );

        switch( relativeTo )
        {
        case TS_LOCAL:
            // position is relative to parent so transform downwards
            position += mTransform.mOrientation->getAsQuaternion( mTransform.mIndex ) * d;
            break;
        case TS_WORLD:
            // position is relative to parent so transform upwards
            if( mParent )
            {
                position +=
                    ( mParent->_getDerivedOrientation().Inverse() * d ) / mParent->_getDerivedScale();
            }
            else
            {
                position += d;
            }
            break;
        case TS_PARENT:
            position += d;
            break;
        }

        mTransform.mPosition->setFromVector3( position, mTransform.mIndex );
        CACHED_TRANSFORM_OUT_OF_DATE();
    }
    //-----------------------------------------------------------------------
    void Node::translate( Real x, Real y, Real z, TransformSpace relativeTo )
    {
        Vector3 v( x, y, z );
        translate( v, relativeTo );
    }
    //-----------------------------------------------------------------------
    void Node::translate( const Matrix3 &axes, const Vector3 &move, TransformSpace relativeTo )
    {
        Vector3 derived = axes * move;
        translate( derived, relativeTo );
    }
    //-----------------------------------------------------------------------
    void Node::translate( const Matrix3 &axes, Real x, Real y, Real z, TransformSpace relativeTo )
    {
        Vector3 d( x, y, z );
        translate( axes, d, relativeTo );
    }
    //-----------------------------------------------------------------------
    void Node::roll( const Radian &angle, TransformSpace relativeTo )
    {
        rotate( Vector3::UNIT_Z, angle, relativeTo );
    }
    //-----------------------------------------------------------------------
    void Node::pitch( const Radian &angle, TransformSpace relativeTo )
    {
        rotate( Vector3::UNIT_X, angle, relativeTo );
    }
    //-----------------------------------------------------------------------
    void Node::yaw( const Radian &angle, TransformSpace relativeTo )
    {
        rotate( Vector3::UNIT_Y, angle, relativeTo );
    }
    //-----------------------------------------------------------------------
    void Node::rotate( const Vector3 &axis, const Radian &angle, TransformSpace relativeTo )
    {
        Quaternion q;
        q.FromAngleAxis( angle, axis );
        rotate( q, relativeTo );
    }

    //-----------------------------------------------------------------------
    void Node::rotate( const Quaternion &q, TransformSpace relativeTo )
    {
        Quaternion orientation;
        mTransform.mOrientation->getAsQuaternion( orientation, mTransform.mIndex );

        switch( relativeTo )
        {
        case TS_PARENT:
            // Rotations are normally relative to local axes, transform up
            orientation = q * orientation;
            break;
        case TS_WORLD:
            // Rotations are normally relative to local axes, transform up
            orientation =
                orientation * _getDerivedOrientation().Inverse() * q * _getDerivedOrientation();
            break;
        case TS_LOCAL:
            // Note the order of the mult, i.e. q comes after
            orientation = orientation * q;
            break;
        }

        // Normalise quaternion to avoid drift
        orientation.normalise();

        mTransform.mOrientation->setFromQuaternion( orientation, mTransform.mIndex );
        CACHED_TRANSFORM_OUT_OF_DATE();
    }

    //-----------------------------------------------------------------------
    void Node::_setDerivedPosition( const Vector3 &pos )
    {
        // find where the node would end up in parent's local space
        if( mParent )
        {
            setPosition( mParent->convertWorldToLocalPosition( pos ) );
            mTransform.mDerivedPosition->setFromVector3( pos, mTransform.mIndex );
            mTransform.mDerivedTransform[mTransform.mIndex].makeTrans( pos );
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
            mCachedTransformOutOfDate = false;
#endif
        }
    }
    //-----------------------------------------------------------------------
    void Node::_setDerivedOrientation( const Quaternion &q )
    {
        // find where the node would end up in parent's local space
        if( mParent )
        {
            setOrientation( mParent->convertWorldToLocalOrientation( q ) );
            mTransform.mDerivedOrientation->setFromQuaternion( q, mTransform.mIndex );
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
            mCachedTransformOutOfDate = false;
#endif
            mTransform.mDerivedTransform[mTransform.mIndex].makeTransform( _getDerivedPosition(),
                                                                           _getDerivedScale(), q );
        }
    }
    //-----------------------------------------------------------------------
    Quaternion Node::_getDerivedOrientation() const
    {
        OGRE_ASSERT_MEDIUM( !mCachedTransformOutOfDate );
        return mTransform.mDerivedOrientation->getAsQuaternion( mTransform.mIndex );
    }
    //-----------------------------------------------------------------------
    Quaternion Node::_getDerivedOrientationUpdated()
    {
        _updateFromParent();
        return mTransform.mDerivedOrientation->getAsQuaternion( mTransform.mIndex );
    }
    //-----------------------------------------------------------------------
    Vector3 Node::_getDerivedPosition() const
    {
        OGRE_ASSERT_MEDIUM( !mCachedTransformOutOfDate );
        return mTransform.mDerivedPosition->getAsVector3( mTransform.mIndex );
    }
    //-----------------------------------------------------------------------
    Vector3 Node::_getDerivedPositionUpdated()
    {
        _updateFromParent();
        return mTransform.mDerivedPosition->getAsVector3( mTransform.mIndex );
    }
    //-----------------------------------------------------------------------
    Vector3 Node::_getDerivedScale() const
    {
        OGRE_ASSERT_MEDIUM( !mCachedTransformOutOfDate );
        return mTransform.mDerivedScale->getAsVector3( mTransform.mIndex );
    }
    //-----------------------------------------------------------------------
    Vector3 Node::_getDerivedScaleUpdated()
    {
        _updateFromParent();
        return mTransform.mDerivedScale->getAsVector3( mTransform.mIndex );
    }
    //-----------------------------------------------------------------------
    Vector3 Node::convertWorldToLocalPosition( const Vector3 &worldPos )
    {
        OGRE_ASSERT_MEDIUM( !mCachedTransformOutOfDate );
#if OGRE_NODE_INHERIT_TRANSFORM
        return Node::_getFullTransform().inverseAffine().transformAffine( worldPos );  // non virt call
#else
        // result is the same as above, but inversion of Quaternion is faster than inversion of Mat3x4
        return Node::_getDerivedOrientation().Inverse() * ( worldPos - Node::_getDerivedPosition() ) /
               Node::_getDerivedScale();  // non virt calls
#endif
    }
    //-----------------------------------------------------------------------
    Vector3 Node::convertLocalToWorldPosition( const Vector3 &localPos )
    {
        OGRE_ASSERT_MEDIUM( !mCachedTransformOutOfDate );
        return Node::_getFullTransform().transformAffine( localPos );  // non virt call
    }
    //-----------------------------------------------------------------------
    Vector3 Node::convertWorldToLocalDirection( const Vector3 &worldDir, bool useScale )
    {
        OGRE_ASSERT_MEDIUM( !mCachedTransformOutOfDate );
#if OGRE_NODE_INHERIT_TRANSFORM
        return useScale ? Node::_getFullTransform().inverseAffine().transformDirectionAffine( worldDir )
                        :  // non virt calls
                   Node::_getDerivedOrientation().Inverse() * worldDir;
#else
        // result is the same as above, but inversion of Quaternion is faster than inversion of Mat3x4
        return useScale ? Node::_getDerivedOrientation().Inverse() * worldDir / Node::_getDerivedScale()
                        :  // non virt calls
                   Node::_getDerivedOrientation().Inverse() * worldDir;
#endif
    }
    //-----------------------------------------------------------------------
    Vector3 Node::convertLocalToWorldDirection( const Vector3 &localDir, bool useScale )
    {
        OGRE_ASSERT_MEDIUM( !mCachedTransformOutOfDate );
        return useScale ? Node::_getFullTransform().transformDirectionAffine( localDir )
                        :  // non virt calls
                   Node::_getDerivedOrientation() * localDir;
    }
    //-----------------------------------------------------------------------
    Quaternion Node::convertWorldToLocalOrientation( const Quaternion &worldOrientation )
    {
        OGRE_ASSERT_MEDIUM( !mCachedTransformOutOfDate );
        return Node::_getDerivedOrientation().Inverse() * worldOrientation;  // non virt call
    }
    //-----------------------------------------------------------------------
    Quaternion Node::convertLocalToWorldOrientation( const Quaternion &localOrientation )
    {
        OGRE_ASSERT_MEDIUM( !mCachedTransformOutOfDate );
        return Node::_getDerivedOrientation() * localOrientation;  // non virt call
    }
    //-----------------------------------------------------------------------
    void Node::removeAllChildren()
    {
        for( Node *child : mChildren )
        {
            child->unsetParent();
            child->mParentIndex = std::numeric_limits<size_t>::max();
        }
        mChildren.clear();
    }
    //-----------------------------------------------------------------------
    void Node::setScale( const Vector3 &inScale )
    {
        assert( !inScale.isNaN() && "Invalid vector supplied as parameter" );
        mTransform.mScale->setFromVector3( inScale, mTransform.mIndex );
        CACHED_TRANSFORM_OUT_OF_DATE();
    }
    //-----------------------------------------------------------------------
    void Node::setScale( Real x, Real y, Real z ) { setScale( Vector3( x, y, z ) ); }
    //-----------------------------------------------------------------------
    Vector3 Node::getScale() const { return mTransform.mScale->getAsVector3( mTransform.mIndex ); }
    //-----------------------------------------------------------------------
    void Node::setInheritOrientation( bool inherit )
    {
        mTransform.mInheritOrientation[mTransform.mIndex] = inherit;
        CACHED_TRANSFORM_OUT_OF_DATE();
    }
    //-----------------------------------------------------------------------
    bool Node::getInheritOrientation() const
    {
        return mTransform.mInheritOrientation[mTransform.mIndex];
    }
    //-----------------------------------------------------------------------
    void Node::setInheritScale( bool inherit )
    {
        mTransform.mInheritScale[mTransform.mIndex] = inherit;
        CACHED_TRANSFORM_OUT_OF_DATE();
    }
    //-----------------------------------------------------------------------
    bool Node::getInheritScale() const { return mTransform.mInheritScale[mTransform.mIndex]; }
    //-----------------------------------------------------------------------
    void Node::scale( const Vector3 &inScale )
    {
        mTransform.mScale->setFromVector3(
            mTransform.mScale->getAsVector3( mTransform.mIndex ) * inScale, mTransform.mIndex );
        CACHED_TRANSFORM_OUT_OF_DATE();
    }
    //-----------------------------------------------------------------------
    void Node::scale( Real x, Real y, Real z ) { scale( Vector3( x, y, z ) ); }
    //-----------------------------------------------------------------------
    Node::NodeVecIterator Node::getChildIterator()
    {
        return NodeVecIterator( mChildren.begin(), mChildren.end() );
    }
    //-----------------------------------------------------------------------
    Node::ConstNodeVecIterator Node::getChildIterator() const
    {
        return ConstNodeVecIterator( mChildren.begin(), mChildren.end() );
    }
    //-----------------------------------------------------------------------
    Real Node::getSquaredViewDepth( const Camera *cam ) const
    {
        Vector3 diff = _getDerivedPosition() - cam->getDerivedPosition();

        // NB use squared length rather than real depth to avoid square root
        return diff.squaredLength();
    }
    //---------------------------------------------------------------------
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
    void Node::_setCachedTransformOutOfDate()
    {
        mCachedTransformOutOfDate = true;

#    if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
        for( Node *child : mChildren )
            child->_setCachedTransformOutOfDate();
#    endif
    }
#endif
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    /*Node::DebugRenderable::DebugRenderable(Node* parent)
        : mParent(parent)
    {
        String matName = "Ogre/Debug/AxesMat";
        mMat = MaterialManager::getSingleton().getByName(matName);
        if (!mMat)
        {
            mMat = MaterialManager::getSingleton().create(matName,
    ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME); Pass* p = mMat->getTechnique(0)->getPass(0);
            p->setLightingEnabled(false);
            p->setPolygonModeOverrideable(false);
            p->setVertexColourTracking(TVC_AMBIENT);
            p->setSceneBlending(SBT_TRANSPARENT_ALPHA);
            p->setCullingMode(CULL_NONE);
            p->setDepthWriteEnabled(false);
        }

        String meshName = "Ogre/Debug/AxesMesh";
        mMeshPtr = MeshManager::getSingleton().getByName(meshName);
        if (!mMeshPtr)
        {
            ManualObject mo( 0, 0 );
            mo.begin(mMat->getName());*/
    /* 3 axes, each made up of 2 of these (base plane = XY)
     *   .------------|\
     *   '------------|/
     *//*
            mo.estimateVertexCount(7 * 2 * 3);
            mo.estimateIndexCount(3 * 2 * 3);
            Quaternion quat[6];
            ColourValue col[3];

            // x-axis
            quat[0] = Quaternion::IDENTITY;
            quat[1].FromAxes(Vector3::UNIT_X, Vector3::NEGATIVE_UNIT_Z, Vector3::UNIT_Y);
            col[0] = ColourValue::Red;
            col[0].a = 0.8;
            // y-axis
            quat[2].FromAxes(Vector3::UNIT_Y, Vector3::NEGATIVE_UNIT_X, Vector3::UNIT_Z);
            quat[3].FromAxes(Vector3::UNIT_Y, Vector3::UNIT_Z, Vector3::UNIT_X);
            col[1] = ColourValue::Green;
            col[1].a = 0.8;
            // z-axis
            quat[4].FromAxes(Vector3::UNIT_Z, Vector3::UNIT_Y, Vector3::NEGATIVE_UNIT_X);
            quat[5].FromAxes(Vector3::UNIT_Z, Vector3::UNIT_X, Vector3::UNIT_Y);
            col[2] = ColourValue::Blue;
            col[2].a = 0.8;

            Vector3 basepos[7] = 
            {
                // stalk
                Vector3(0, 0.05, 0), 
                Vector3(0, -0.05, 0),
                Vector3(0.7, -0.05, 0),
                Vector3(0.7, 0.05, 0),
                // head
                Vector3(0.7, -0.15, 0),
                Vector3(1, 0, 0),
                Vector3(0.7, 0.15, 0)
            };


            // vertices
            // 6 arrows
            for (size_t i = 0; i < 6; ++i)
            {
                // 7 points
                for (size_t p = 0; p < 7; ++p)
                {
                    Vector3 pos = quat[i] * basepos[p];
                    mo.position(pos);
                    mo.colour(col[i / 2]);
                }
            }

            // indices
            // 6 arrows
            for (uint32 i = 0; i < 6; ++i)
            {
                uint32 base = i * 7;
                mo.triangle(base + 0, base + 1, base + 2);
                mo.triangle(base + 0, base + 2, base + 3);
                mo.triangle(base + 4, base + 5, base + 6);
            }

            mo.end();

            mMeshPtr = mo.convertToMesh(meshName, ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME);

        }

    }
    //---------------------------------------------------------------------
    Node::DebugRenderable::~DebugRenderable()
    {
    }
    //-----------------------------------------------------------------------
    const MaterialPtr& Node::DebugRenderable::getMaterial() const
    {
        return mMat;
    }
    //---------------------------------------------------------------------
    void Node::DebugRenderable::getRenderOperation(RenderOperation& op)
    {
        return mMeshPtr->getSubMesh(0)->_getRenderOperation(op);
    }
    //-----------------------------------------------------------------------
    void Node::DebugRenderable::getWorldTransforms(Matrix4* xform) const
    {
        // Assumes up to date
        *xform = mParent->_getFullTransform();
        if (!Math::RealEqual(mScaling, 1.0))
        {
            Matrix4 m = Matrix4::IDENTITY;
            Vector3 s(mScaling, mScaling, mScaling);
            m.setScale(s);
            *xform = (*xform) * m;
        }
    }
    //-----------------------------------------------------------------------
    Real Node::DebugRenderable::getSquaredViewDepth(const Camera* cam) const
    {
        return mParent->getSquaredViewDepth(cam);
    }
    //-----------------------------------------------------------------------
    const LightList& Node::DebugRenderable::getLights() const
    {
        // Nodes should not be lit by the scene, this will not get called
        static LightList ll;
        return ll;
    }*/
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    Node::Listener::~Listener() {}
}  // namespace Ogre

#undef CACHED_TRANSFORM_OUT_OF_DATE
