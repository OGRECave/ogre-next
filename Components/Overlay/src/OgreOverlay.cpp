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

#include "OgreOverlay.h"

#include "OgreCamera.h"
#include "OgreOverlayContainer.h"
#include "OgreOverlayManager.h"
#include "OgreRenderQueue.h"
#include "OgreSceneNode.h"
#include "OgreVector3.h"

namespace Ogre
{
    namespace v1
    {
        static const String OVERLAY_NAME( "Overlay" );

        //---------------------------------------------------------------------
        Overlay::Overlay( const String &name, IdType id, ObjectMemoryManager *objectMemoryManager,
                          uint8 renderQueueId ) :
            MovableObject( id, objectMemoryManager, (SceneManager *)0, renderQueueId ),
            mRotate( 0.0f ),
            mScrollX( 0.0f ),
            mScrollY( 0.0f ),
            mScaleX( 1.0f ),
            mScaleY( 1.0f ),
            mLastViewportWidth( 0 ),
            mLastViewportHeight( 0 ),
            mTransformOutOfDate( true ),
            mInitialised( false )

        {
            this->setName( name );
        }
        //---------------------------------------------------------------------
        Overlay::~Overlay()
        {
            // remove children
            for( OverlayContainerList::iterator i = m2DElements.begin(); i != m2DElements.end(); ++i )
            {
                ( *i )->_notifyParent( 0, 0 );
            }
        }
        //---------------------------------------------------------------------
        const String &Overlay::getMovableType() const { return OVERLAY_NAME; }
        //---------------------------------------------------------------------
        void Overlay::setZOrder( uint16 zorder )
        {
            mObjectData.mDistanceToCamera[mObjectData.mIndex] =
                static_cast<uint32>( zorder ^ 0xffff )
                << ( 32u - static_cast<uint32>( RqBits::DepthBits ) );
        }
        //---------------------------------------------------------------------
        uint16 Overlay::getZOrder() const
        {
            return (uint16)( mObjectData.mDistanceToCamera[mObjectData.mIndex] >>
                             ( 32u - static_cast<uint32>( RqBits::DepthBits ) ) ) ^
                   0xffff;
        }
        //---------------------------------------------------------------------
        void Overlay::show()
        {
            setVisible( true );
            if( !mInitialised )
            {
                initialise();
            }
        }
        //---------------------------------------------------------------------
        void Overlay::hide() { setVisible( false ); }
        //---------------------------------------------------------------------
        void Overlay::initialise()
        {
            OverlayContainerList::iterator i, iend;
            iend = m2DElements.end();
            for( i = m2DElements.begin(); i != m2DElements.end(); ++i )
            {
                ( *i )->initialise();
            }
            mInitialised = true;
        }
        //---------------------------------------------------------------------
        void Overlay::add2D( OverlayContainer *cont )
        {
            m2DElements.push_back( cont );
            // Notify parent
            cont->_notifyParent( 0, this );
            cont->_notifyViewport();
        }
        //---------------------------------------------------------------------
        void Overlay::remove2D( OverlayContainer *cont )
        {
            m2DElements.remove( cont );
            cont->_notifyParent( 0, 0 );
        }
        //---------------------------------------------------------------------
        void Overlay::clear()
        {
            m2DElements.clear();
            // Note no deallocation, memory handled by OverlayManager & SceneManager
        }
        //---------------------------------------------------------------------
        void Overlay::setScroll( Real x, Real y )
        {
            mScrollX = x;
            mScrollY = y;
            mTransformOutOfDate = true;
        }
        //---------------------------------------------------------------------
        Real Overlay::getScrollX() const { return mScrollX; }
        //---------------------------------------------------------------------
        Real Overlay::getScrollY() const { return mScrollY; }
        //---------------------------------------------------------------------
        OverlayContainer *Overlay::getChild( const String &name )
        {
            OverlayContainerList::iterator i, iend;
            iend = m2DElements.end();
            for( i = m2DElements.begin(); i != iend; ++i )
            {
                if( ( *i )->getName() == name )
                {
                    return *i;
                }
            }
            return NULL;
        }
        //---------------------------------------------------------------------
        void Overlay::scroll( Real xoff, Real yoff )
        {
            mScrollX += xoff;
            mScrollY += yoff;
            mTransformOutOfDate = true;
        }
        //---------------------------------------------------------------------
        void Overlay::setRotate( const Radian &angle )
        {
            mRotate = angle;
            mTransformOutOfDate = true;
        }
        //---------------------------------------------------------------------
        void Overlay::rotate( const Radian &angle ) { setRotate( mRotate + angle ); }
        //---------------------------------------------------------------------
        void Overlay::setScale( Real x, Real y )
        {
            mScaleX = x;
            mScaleY = y;
            mTransformOutOfDate = true;
        }
        //---------------------------------------------------------------------
        Real Overlay::getScaleX() const { return mScaleX; }
        //---------------------------------------------------------------------
        Real Overlay::getScaleY() const { return mScaleY; }
        //---------------------------------------------------------------------
        void Overlay::_getWorldTransforms( Matrix4 *xform ) const
        {
            if( mTransformOutOfDate )
            {
                updateTransform();
            }
            *xform = mTransform;
        }
        //---------------------------------------------------------------------
        void Overlay::_updateRenderQueue( RenderQueue *queue, Camera *camera, const Camera *lodCamera,
                                          Viewport *vp )
        {
            if( getVisible() )
            {
                // Flag for update pixel-based GUIElements if viewport has changed dimensions
                bool tmpViewportDimensionsChanged = false;
                if( mLastViewportWidth != vp->getActualWidth() ||
                    mLastViewportHeight != vp->getActualHeight() )
                {
                    tmpViewportDimensionsChanged = true;
                    mLastViewportWidth = vp->getActualWidth();
                    mLastViewportHeight = vp->getActualHeight();
                }

                OverlayContainerList::iterator i, iend;

                if( tmpViewportDimensionsChanged )
                {
                    iend = m2DElements.end();
                    for( i = m2DElements.begin(); i != iend; ++i )
                    {
                        ( *i )->_notifyViewport();
                    }
                }

                // Add 2D elements
                iend = m2DElements.end();
                for( i = m2DElements.begin(); i != iend; ++i )
                {
                    ( *i )->_update();
                    ( *i )->_updateRenderQueue( queue, camera, lodCamera );
                }
            }
        }
        //---------------------------------------------------------------------
        void Overlay::updateTransform() const
        {
            // Ordering:
            //    1. Scale
            //    2. Rotate
            //    3. Translate
            Matrix3 rot3x3, scale3x3;
            rot3x3.FromEulerAnglesXYZ( Radian( 0 ), Radian( 0 ), mRotate );
            scale3x3 = Matrix3::ZERO;
            scale3x3[0][0] = mScaleX;
            scale3x3[1][1] = mScaleY;
            scale3x3[2][2] = 1.0f;

            mTransform = Matrix4::IDENTITY;
            mTransform = rot3x3 * scale3x3;
            mTransform.setTrans( Vector3( mScrollX, mScrollY, 0 ) );

            mTransformOutOfDate = false;
        }
        //---------------------------------------------------------------------
        OverlayElement *Overlay::findElementAt( Real x, Real y )
        {
            OverlayElement *ret = NULL;
            OverlayContainerList::iterator i, iend;
            iend = m2DElements.end();
            for( i = m2DElements.begin(); i != iend; ++i )
            {
                OverlayElement *elementFound = ( *i )->findElementAt( x, y );
                if( elementFound )
                    ret = elementFound;
            }

            return ret;
        }

    }  // namespace v1
}  // namespace Ogre
