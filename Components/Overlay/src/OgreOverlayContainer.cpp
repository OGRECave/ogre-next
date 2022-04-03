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

#include "OgreOverlayContainer.h"

#include "OgreException.h"
#include "OgreOverlay.h"
#include "OgreOverlayManager.h"

namespace Ogre
{
    namespace v1
    {
        //---------------------------------------------------------------------
        OverlayContainer::OverlayContainer( const String &name ) :
            OverlayElement( name ),
            mChildrenProcessEvents( true )
        {
        }
        //---------------------------------------------------------------------
        OverlayContainer::~OverlayContainer()
        {
            // remove from parent overlay if root
            if( mOverlay && !mParent )
            {
                mOverlay->remove2D( this );
            }

            OverlayContainer::ChildIterator ci = getChildIterator();
            while( ci.hasMoreElements() )
            {
                OverlayElement *child = ci.getNext();
                child->_notifyParent( 0, 0 );
            }
        }
        //---------------------------------------------------------------------
        void OverlayContainer::addChild( OverlayElement *elem )
        {
            if( elem->isContainer() )
            {
                addChildImpl( static_cast<OverlayContainer *>( elem ) );
            }
            else
            {
                addChildImpl( elem );
            }
        }
        //---------------------------------------------------------------------
        void OverlayContainer::addChildImpl( OverlayElement *elem )
        {
            String name = elem->getName();
            ChildMap::iterator i = mChildren.find( name );
            if( i != mChildren.end() )
            {
                OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                             "Child with name " + name + " already defined.",
                             "OverlayContainer::addChild" );
            }

            mChildren.insert( ChildMap::value_type( name, elem ) );
            // tell child about parent & Z-order
            elem->_notifyParent( this, mOverlay );
            elem->_notifyViewport();
        }
        //---------------------------------------------------------------------
        void OverlayContainer::addChildImpl( OverlayContainer *cont )
        {
            // Add to main map first
            // This will pick up duplicates
            OverlayElement *pElem = cont;
            addChildImpl( pElem );

            /*
            cont->_notifyParent(this, mOverlay);
            cont->_notifyZOrder(mZOrder + 1);
            cont->_notifyWorldTransforms(mXForm);

            // tell children of new container the current overlay
            ChildIterator it = cont->getChildIterator();
            while (it.hasMoreElements())
            {
                // Give children Z-order 1 higher than this
                GuiElement* pElemChild = it.getNext();
                pElemChild->_notifyParent(cont, mOverlay);
                pElemChild->_notifyZOrder(cont->getZOrder() + 1);
                pElemChild->_notifyWorldTransforms(mXForm);
            }
            */

            // Now add to specific map too
            mChildContainers.insert( ChildContainerMap::value_type( cont->getName(), cont ) );
        }
        //---------------------------------------------------------------------
        void OverlayContainer::removeChild( const String &name )
        {
            ChildMap::iterator i = mChildren.find( name );
            if( i == mChildren.end() )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Child with name " + name + " not found.",
                             "OverlayContainer::removeChild" );
            }

            OverlayElement *element = i->second;
            mChildren.erase( i );

            // Remove from container list (if found)
            ChildContainerMap::iterator j = mChildContainers.find( name );
            if( j != mChildContainers.end() )
                mChildContainers.erase( j );

            element->_setParent( 0 );
        }
        //---------------------------------------------------------------------
        void OverlayContainer::_addChild( OverlayElement *elem )
        {
            if( elem->isContainer() )
            {
                addChildImpl( static_cast<OverlayContainer *>( elem ) );
            }
            else
            {
                addChildImpl( elem );
            }
        }
        //---------------------------------------------------------------------
        void OverlayContainer::_removeChild( const String &name )
        {
            ChildMap::iterator i = mChildren.find( name );
            if( i == mChildren.end() )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Child with name " + name + " not found.",
                             "OverlayContainer::removeChild" );
            }

            OverlayElement *element = i->second;
            mChildren.erase( i );

            // Remove from container list (if found)
            ChildContainerMap::iterator j = mChildContainers.find( name );
            if( j != mChildContainers.end() )
                mChildContainers.erase( j );

            element->_setParent( 0 );
        }
        //---------------------------------------------------------------------
        OverlayElement *OverlayContainer::getChild( const String &name )
        {
            ChildMap::iterator i = mChildren.find( name );
            if( i == mChildren.end() )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Child with name " + name + " not found.",
                             "OverlayContainer::getChild" );
            }

            return i->second;
        }
        //---------------------------------------------------------------------
        OverlayContainer::ChildIterator OverlayContainer::getChildIterator()
        {
            return ChildIterator( mChildren.begin(), mChildren.end() );
        }
        //---------------------------------------------------------------------
        OverlayContainer::ChildContainerIterator OverlayContainer::getChildContainerIterator()
        {
            return ChildContainerIterator( mChildContainers.begin(), mChildContainers.end() );
        }
        //---------------------------------------------------------------------
        void OverlayContainer::initialise()
        {
            ChildContainerMap::iterator coni;
            for( coni = mChildContainers.begin(); coni != mChildContainers.end(); ++coni )
            {
                coni->second->initialise();
            }
            ChildMap::iterator ci;
            for( ci = mChildren.begin(); ci != mChildren.end(); ++ci )
            {
                ci->second->initialise();
            }
        }
        //---------------------------------------------------------------------
        void OverlayContainer::_positionsOutOfDate()
        {
            OverlayElement::_positionsOutOfDate();

            ChildIterator it = getChildIterator();
            while( it.hasMoreElements() )
            {
                it.getNext()->_positionsOutOfDate();
            }
        }

        //---------------------------------------------------------------------
        void OverlayContainer::_update()
        {
            // call superclass
            OverlayElement::_update();

            // Update children
            ChildIterator it = getChildIterator();
            while( it.hasMoreElements() )
            {
                it.getNext()->_update();
            }
        }
        //---------------------------------------------------------------------
        void OverlayContainer::_notifyViewport()
        {
            OverlayElement::_notifyViewport();

            // Update children
            ChildIterator it = getChildIterator();
            while( it.hasMoreElements() )
            {
                it.getNext()->_notifyViewport();
            }
        }
        //---------------------------------------------------------------------
        void OverlayContainer::_notifyParent( OverlayContainer *parent, Overlay *overlay )
        {
            OverlayElement::_notifyParent( parent, overlay );

            // Update children
            ChildIterator it = getChildIterator();
            while( it.hasMoreElements() )
            {
                // Notify the children of the overlay
                it.getNext()->_notifyParent( this, overlay );
            }
        }

        //---------------------------------------------------------------------
        void OverlayContainer::_updateRenderQueue( RenderQueue *queue, Camera *camera,
                                                   const Camera *lodCamera )
        {
            if( mVisible )
            {
                OverlayElement::_updateRenderQueue( queue, camera, lodCamera );

                // Also add children
                ChildIterator it = getChildIterator();
                while( it.hasMoreElements() )
                {
                    // Give children Z-order 1 higher than this
                    it.getNext()->_updateRenderQueue( queue, camera, lodCamera );
                }
            }
        }

        OverlayElement *OverlayContainer::findElementAt( Real x, Real y )  // relative to parent
        {
            OverlayElement *ret = NULL;

            if( mVisible )
            {
                ret = OverlayElement::findElementAt(
                    x, y );  // default to the current container if no others are found
                if( ret && mChildrenProcessEvents )
                {
                    ChildIterator it = getChildIterator();
                    while( it.hasMoreElements() )
                    {
                        OverlayElement *currentOverlayElement = it.getNext();
                        if( currentOverlayElement->isVisible() && currentOverlayElement->isEnabled() )
                        {
                            OverlayElement *elementFound = currentOverlayElement->findElementAt( x, y );
                            if( elementFound )
                                ret = elementFound;
                        }
                    }
                }
            }

            return ret;
        }

        void OverlayContainer::copyFromTemplate( OverlayElement *templateOverlay )
        {
            OverlayElement::copyFromTemplate( templateOverlay );

            if( templateOverlay->isContainer() && isContainer() )
            {
                OverlayContainer::ChildIterator it =
                    static_cast<OverlayContainer *>( templateOverlay )->getChildIterator();
                while( it.hasMoreElements() )
                {
                    OverlayElement *oldChildElement = it.getNext();
                    if( oldChildElement->isCloneable() )
                    {
                        OverlayElement *newChildElement =
                            OverlayManager::getSingleton().createOverlayElement(
                                oldChildElement->getTypeName(),
                                mName + "/" + oldChildElement->getName() );
                        newChildElement->copyFromTemplate( oldChildElement );
                        addChild( static_cast<OverlayContainer *>( newChildElement ) );
                    }
                }
            }
        }

        OverlayElement *OverlayContainer::clone( const String &instanceName )
        {
            OverlayContainer *newContainer;

            newContainer = static_cast<OverlayContainer *>( OverlayElement::clone( instanceName ) );

            ChildIterator it = getChildIterator();
            while( it.hasMoreElements() )
            {
                OverlayElement *oldChildElement = it.getNext();
                if( oldChildElement->isCloneable() )
                {
                    OverlayElement *newChildElement = oldChildElement->clone( instanceName );
                    newContainer->_addChild( newChildElement );
                }
            }

            return newContainer;
        }

    }  // namespace v1
}  // namespace Ogre
