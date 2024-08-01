/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2023 Torus Knot Software Ltd

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

#ifndef OgreBillboard2_H
#define OgreBillboard2_H

#include "OgrePrerequisites.h"

#include "OgreQuaternion.h"
#include "OgreVector3.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    /// Basic Billboard holder that belongs to a ParticleSystemDef.
    struct _OgreExport Billboard
    {
        uint32        mHandle;
        BillboardSet *mBillboardSet;  /// Our owner.

        Billboard( uint32 handle, BillboardSet *billboardSet ) :
            mHandle( handle ),
            mBillboardSet( billboardSet )
        {
        }

        static Vector3 getDirection( const Quaternion &qRot ) { return -qRot.zAxis(); }

        void setVisible( const bool bVisible );
        void setPosition( const Vector3 &pos );
        void setOrientation( const Quaternion &qRot ) { setDirection( getDirection( qRot ) ); }
        void setDirection( const Vector3 &vDir );
        void setDirection( const Vector3 &vDir, const Ogre::Radian rot );
        void setRotation( const Ogre::Radian rot );
        void setDimensions( const Vector2 &dim );
        void setColour( const ColourValue &colour );

        void set( const Vector3 &pos, const Vector3 &dir, const Vector2 &dim, const ColourValue &colour,
                  const Ogre::Radian rot = Ogre::Radian( 0.0f ) );
    };

    OGRE_ASSUME_NONNULL_END
}  // namespace Ogre
#include "OgreHeaderSuffix.h"

#endif
