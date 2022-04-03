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

#include "OgreOldBone.h"

#include "OgreSkeleton.h"

namespace Ogre
{
    namespace v1
    {
        //---------------------------------------------------------------------
        OldBone::OldBone( unsigned short handle, Skeleton *creator ) :
            OldNode(),
            mHandle( handle ),
            mManuallyControlled( false ),
            mCreator( creator )
        {
        }
        //---------------------------------------------------------------------
        OldBone::OldBone( const String &name, unsigned short handle, Skeleton *creator ) :
            OldNode( name ),
            mHandle( handle ),
            mManuallyControlled( false ),
            mCreator( creator )
        {
        }
        //---------------------------------------------------------------------
        OldBone::~OldBone() {}
        //---------------------------------------------------------------------
        OldBone *OldBone::createChild( unsigned short handle, const Vector3 &inTranslate,
                                       const Quaternion &inRotate )
        {
            OldBone *retBone = mCreator->createBone( handle );
            retBone->translate( inTranslate );
            retBone->rotate( inRotate );
            this->addChild( retBone );
            return retBone;
        }
        //---------------------------------------------------------------------
        OldNode *OldBone::createChildImpl() { return mCreator->createBone(); }
        //---------------------------------------------------------------------
        OldNode *OldBone::createChildImpl( const String &name ) { return mCreator->createBone( name ); }
        //---------------------------------------------------------------------
        void OldBone::setBindingPose()
        {
            setInitialState();

            // Save inverse derived position/scale/orientation, used for calculate offset transform later
            mBindDerivedInversePosition = -_getDerivedPosition();
            mBindDerivedInverseScale = Vector3::UNIT_SCALE / _getDerivedScale();
            mBindDerivedInverseOrientation = _getDerivedOrientation().Inverse();
        }
        //---------------------------------------------------------------------
        void OldBone::reset() { resetToInitialState(); }
        //---------------------------------------------------------------------
        void OldBone::setManuallyControlled( bool manuallyControlled )
        {
            mManuallyControlled = manuallyControlled;
            mCreator->_notifyManualBoneStateChange( this );
        }
        //---------------------------------------------------------------------
        bool OldBone::isManuallyControlled() const { return mManuallyControlled; }
        //---------------------------------------------------------------------
        void OldBone::_getOffsetTransform( Matrix4 &m ) const
        {
            // Combine scale with binding pose inverse scale,
            // NB just combine as equivalent axes, no shearing
            Vector3 locScale = _getDerivedScale() * mBindDerivedInverseScale;

            // Combine orientation with binding pose inverse orientation
            Quaternion locRotate = _getDerivedOrientation() * mBindDerivedInverseOrientation;

            // Combine position with binding pose inverse position,
            // Note that translation is relative to scale & rotation,
            // so first reverse transform original derived position to
            // binding pose bone space, and then transform to current
            // derived bone space.
            Vector3 locTranslate =
                _getDerivedPosition() + locRotate * ( locScale * mBindDerivedInversePosition );

            m.makeTransform( locTranslate, locScale, locRotate );
        }
        //---------------------------------------------------------------------
        unsigned short OldBone::getHandle() const { return mHandle; }
        //---------------------------------------------------------------------
        void OldBone::needUpdate( bool forceParentUpdate )
        {
            OldNode::needUpdate( forceParentUpdate );

            if( isManuallyControlled() )
            {
                // Dirty the skeleton if manually controlled so animation can be updated
                mCreator->_notifyManualBonesDirty();
            }
        }
    }  // namespace v1
}  // namespace Ogre
