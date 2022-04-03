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

#include "OgreOverlayManager.h"

#include "Math/Array/OgreNodeMemoryManager.h"
#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreOverlay.h"
#include "OgreOverlayContainer.h"
#include "OgreOverlayElementFactory.h"
#include "OgreRenderQueue.h"
#include "OgreResourceGroupManager.h"
#include "OgreString.h"
#include "OgreStringConverter.h"
#include "OgreViewport.h"

namespace Ogre
{
    template <>
    v1::OverlayManager *Singleton<v1::OverlayManager>::msSingleton = 0;
    namespace v1
    {
        //---------------------------------------------------------------------
        OverlayManager *OverlayManager::getSingletonPtr() { return msSingleton; }
        OverlayManager &OverlayManager::getSingleton()
        {
            assert( msSingleton );
            return ( *msSingleton );
        }
        //---------------------------------------------------------------------
        OverlayManager::OverlayManager() :
            mDefaultRenderQueueId( 254 ),
            mLastViewportWidth( 0 ),
            mLastViewportHeight( 0 ),
            mDummyNode( 0 ),
            mNodeMemoryManager( 0 )
        {
            // Scripting is supported by this manager
            mScriptPatterns.push_back( "*.overlay" );
            ResourceGroupManager::getSingleton()._registerScriptLoader( this );

            mNodeMemoryManager = new NodeMemoryManager();
            mDummyNode = OGRE_NEW SceneNode( 0, 0, mNodeMemoryManager, 0 );
            mDummyNode->_getFullTransformUpdated();
        }
        //---------------------------------------------------------------------
        OverlayManager::~OverlayManager()
        {
            destroyAllOverlayElements( false );
            destroyAllOverlayElements( true );
            destroyAll();

            for( FactoryMap::iterator i = mFactories.begin(); i != mFactories.end(); ++i )
            {
                OGRE_DELETE i->second;
            }

            OGRE_DELETE mDummyNode;
            delete mNodeMemoryManager;
            mDummyNode = 0;
            mNodeMemoryManager = 0;

            // Unregister with resource group manager
            ResourceGroupManager::getSingleton()._unregisterScriptLoader( this );
        }
        //---------------------------------------------------------------------
        void OverlayManager::_releaseManualHardwareResources()
        {
            for( int iTemplate = 0; iTemplate <= 1; ++iTemplate )
            {
                ElementMap &elementMap = getElementMap( iTemplate != 0 );
                for( ElementMap::iterator i = elementMap.begin(), i_end = elementMap.end(); i != i_end;
                     ++i )
                    i->second->_releaseManualHardwareResources();
            }
        }
        //---------------------------------------------------------------------
        void OverlayManager::_restoreManualHardwareResources()
        {
            for( int iTemplate = 0; iTemplate <= 1; ++iTemplate )
            {
                ElementMap &elementMap = getElementMap( iTemplate != 0 );
                for( ElementMap::iterator i = elementMap.begin(), i_end = elementMap.end(); i != i_end;
                     ++i )
                    i->second->_restoreManualHardwareResources();
            }
        }
        //---------------------------------------------------------------------
        const StringVector &OverlayManager::getScriptPatterns() const { return mScriptPatterns; }
        //---------------------------------------------------------------------
        Real OverlayManager::getLoadingOrder() const
        {
            // Load late
            return 1100.0f;
        }
        //---------------------------------------------------------------------
        Overlay *OverlayManager::create( const String &name )
        {
            Overlay *ret = 0;
            OverlayMap::iterator i = mOverlayMap.find( name );

            if( i == mOverlayMap.end() )
            {
                ret = OGRE_NEW Overlay( name, Id::generateNewId<Overlay>(), &mOverlayMemoryManager,
                                        mDefaultRenderQueueId );
                assert( ret && "Overlay creation failed" );
                mDummyNode->attachObject( ret );
                ret->setVisible( false );
                mOverlayMap[name] = ret;
            }
            else
            {
                OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                             "Overlay with name '" + name + "' already exists!",
                             "OverlayManager::create" );
            }

            return ret;
        }
        //---------------------------------------------------------------------
        Overlay *OverlayManager::getByName( const String &name )
        {
            OverlayMap::iterator i = mOverlayMap.find( name );
            if( i == mOverlayMap.end() )
            {
                return 0;
            }
            else
            {
                return i->second;
            }
        }
        //---------------------------------------------------------------------
        void OverlayManager::destroy( const String &name )
        {
            OverlayMap::iterator i = mOverlayMap.find( name );
            if( i == mOverlayMap.end() )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                             "Overlay with name '" + name + "' not found.", "OverlayManager::destroy" );
            }
            else
            {
                mDummyNode->detachObject( i->second );
                OGRE_DELETE i->second;
                mOverlayMap.erase( i );
            }
        }
        //---------------------------------------------------------------------
        void OverlayManager::destroy( Overlay *overlay )
        {
            for( OverlayMap::iterator i = mOverlayMap.begin(); i != mOverlayMap.end(); ++i )
            {
                if( i->second == overlay )
                {
                    mDummyNode->detachObject( i->second );
                    OGRE_DELETE i->second;
                    mOverlayMap.erase( i );
                    return;
                }
            }

            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Overlay not found.",
                         "OverlayManager::destroy" );
        }
        //---------------------------------------------------------------------
        void OverlayManager::destroyAll()
        {
            mDummyNode->detachAllObjects();
            for( OverlayMap::iterator i = mOverlayMap.begin(); i != mOverlayMap.end(); ++i )
            {
                OGRE_DELETE i->second;
            }
            mOverlayMap.clear();
            mLoadedScripts.clear();
        }
        //---------------------------------------------------------------------
        OverlayManager::OverlayMapIterator OverlayManager::getOverlayIterator()
        {
            return OverlayMapIterator( mOverlayMap.begin(), mOverlayMap.end() );
        }
        //---------------------------------------------------------------------
        void OverlayManager::parseScript( DataStreamPtr &stream, const String &groupName )
        {
            // check if we've seen this script before (can happen if included
            // multiple times)
            if( !stream->getName().empty() &&
                mLoadedScripts.find( stream->getName() ) != mLoadedScripts.end() )
            {
                LogManager::getSingleton().logMessage( "Skipping loading overlay include: '" +
                                                       stream->getName() + " as it is already loaded." );
                return;
            }
            String line;
            Overlay *pOverlay = 0;

            while( !stream->eof() )
            {
                bool isATemplate = false;
                bool skipLine = false;
                line = stream->getLine();
                // Ignore comments & blanks
                if( !( line.length() == 0 || line.substr( 0, 2 ) == "//" ) )
                {
                    if( line.substr( 0, 8 ) == "#include" )
                    {
                        vector<String>::type params = StringUtil::split( line, "\t\n ()<>" );
                        DataStreamPtr includeStream =
                            ResourceGroupManager::getSingleton().openResource( params[1], groupName );
                        parseScript( includeStream, groupName );
                        continue;
                    }
                    if( !pOverlay )
                    {
                        // No current overlay

                        // check to see if there is a template
                        if( line.substr( 0, 8 ) == "template" )
                        {
                            isATemplate = true;
                        }
                        else
                        {
                            // So first valid data should be overlay name
                            if( StringUtil::startsWith( line, "overlay " ) )
                            {
                                // chop off the 'particle_system ' needed by new compilers
                                line = line.substr( 8 );
                            }
                            pOverlay = create( line );
                            pOverlay->_notifyOrigin( stream->getName() );
                            // Skip to and over next {
                            skipToNextOpenBrace( stream );
                            skipLine = true;
                        }
                    }
                    if( ( pOverlay && !skipLine ) || isATemplate )
                    {
                        // Already in overlay
                        vector<String>::type params = StringUtil::split( line, "\t\n ()" );

                        if( line == "}" )
                        {
                            // Finished overlay
                            pOverlay = 0;
                        }
                        else if( parseChildren( stream, line, pOverlay, isATemplate, NULL ) )
                        {
                        }
                        else
                        {
                            // Attribute
                            if( !isATemplate )
                            {
                                parseAttrib( line, pOverlay );
                            }
                        }
                    }
                }
            }

            // record as parsed
            mLoadedScripts.insert( stream->getName() );
        }
        //---------------------------------------------------------------------
        void OverlayManager::_queueOverlaysForRendering( RenderQueue *pQueue, Viewport *vp )
        {
            // Flag for update pixel-based GUIElements if viewport has changed dimensions
            if( mLastViewportWidth != vp->getActualWidth() ||
                mLastViewportHeight != vp->getActualHeight() )
            {
                mLastViewportWidth = vp->getActualWidth();
                mLastViewportHeight = vp->getActualHeight();
            }

            OverlayMap::iterator i, iend;
            iend = mOverlayMap.end();
            for( i = mOverlayMap.begin(); i != iend; ++i )
            {
                Overlay *o = i->second;
                o->_updateRenderQueue( pQueue, (Camera *)( 0 ), (Camera *)( 0 ), vp );
            }
        }
        //---------------------------------------------------------------------
        void OverlayManager::parseNewElement( DataStreamPtr &stream, String &elemType, String &elemName,
                                              bool isContainer, Overlay *pOverlay, bool isATemplate,
                                              String templateName, OverlayContainer *container )
        {
            String line;

            OverlayElement *newElement = NULL;
            newElement = OverlayManager::getSingleton().createOverlayElementFromTemplate(
                templateName, elemType, elemName, isATemplate );

            // do not add a template to an overlay

            // add new element to parent
            if( container )
            {
                // Attach to container
                container->addChild( newElement );
            }
            // do not add a template to the overlay. For templates overlay = 0
            else if( pOverlay )
            {
                pOverlay->add2D( (OverlayContainer *)newElement );
            }

            while( !stream->eof() )
            {
                line = stream->getLine();
                // Ignore comments & blanks
                if( !( line.length() == 0 || line.substr( 0, 2 ) == "//" ) )
                {
                    if( line == "}" )
                    {
                        // Finished element
                        break;
                    }
                    else
                    {
                        if( isContainer &&
                            parseChildren( stream, line, pOverlay, isATemplate,
                                           static_cast<OverlayContainer *>( newElement ) ) )
                        {
                            // nested children... don't reparse it
                        }
                        else
                        {
                            // Attribute
                            parseElementAttrib( line, pOverlay, newElement );
                        }
                    }
                }
            }
        }

        //---------------------------------------------------------------------
        bool OverlayManager::parseChildren( DataStreamPtr &stream, const String &line, Overlay *pOverlay,
                                            bool isATemplate, OverlayContainer *parent )
        {
            bool ret = false;
            uint skipParam = 0;
            vector<String>::type params = StringUtil::split( line, "\t\n ()" );

            if( isATemplate )
            {
                if( params[0] == "template" )
                {
                    skipParam++;  // the first param = 'template' on a new child element
                }
            }

            // top level component cannot be an element, it must be a container unless it is a template
            if( params[0 + skipParam] == "container" ||
                ( params[0 + skipParam] == "element" && ( isATemplate || parent != NULL ) ) )
            {
                String templateName;
                ret = true;
                // nested container/element
                if( params.size() > 3 + skipParam )
                {
                    if( params.size() != 5 + skipParam )
                    {
                        LogManager::getSingleton().logMessage(
                            "Bad element/container line: '" + line + "' in " + parent->getTypeName() +
                                " " + parent->getName() + ", expecting ':' templateName",
                            LML_CRITICAL );
                        skipToNextCloseBrace( stream );
                        // barf
                        return ret;
                    }
                    if( params[3 + skipParam] != ":" )
                    {
                        LogManager::getSingleton().logMessage(
                            "Bad element/container line: '" + line + "' in " + parent->getTypeName() +
                                " " + parent->getName() + ", expecting ':' for element inheritance",
                            LML_CRITICAL );
                        skipToNextCloseBrace( stream );
                        // barf
                        return ret;
                    }

                    templateName = params[4 + skipParam];
                }

                else if( params.size() != 3 + skipParam )
                {
                    LogManager::getSingleton().logMessage(
                        "Bad element/container line: '" + line + "' in " + parent->getTypeName() + " " +
                        parent->getName() + ", expecting 'element type(name)'" );
                    skipToNextCloseBrace( stream );
                    // barf
                    return ret;
                }

                skipToNextOpenBrace( stream );
                parseNewElement( stream, params[1 + skipParam], params[2 + skipParam], true, pOverlay,
                                 isATemplate, templateName, (OverlayContainer *)parent );
            }

            return ret;
        }

        //---------------------------------------------------------------------
        void OverlayManager::parseAttrib( const String &line, Overlay *pOverlay )
        {
            // Split params on first space
            vector<String>::type vecparams = StringUtil::split( line, "\t ", 1 );

            // Look up first param (command setting)
            StringUtil::toLowerCase( vecparams[0] );
            if( vecparams[0] == "zorder" )
            {
                pOverlay->setZOrder( (ushort)StringConverter::parseUnsignedInt( vecparams[1] ) );
            }
            else
            {
                LogManager::getSingleton().logMessage(
                    "Bad overlay attribute line: '" + line + "' for overlay " + pOverlay->getName(),
                    LML_CRITICAL );
            }
        }
        //---------------------------------------------------------------------
        void OverlayManager::parseElementAttrib( const String &line, Overlay *pOverlay,
                                                 OverlayElement *pElement )
        {
            // Split params on first space
            vector<String>::type vecparams = StringUtil::split( line, "\t ", 1 );

            // Look up first param (command setting)
            StringUtil::toLowerCase( vecparams[0] );
            if( !pElement->setParameter( vecparams[0], vecparams[1] ) )
            {
                // BAD command. BAD!
                LogManager::getSingleton().logMessage(
                    "Bad element attribute line: '" + line + "' for element " + pElement->getName() +
                        " in overlay " + ( !pOverlay ? BLANKSTRING : pOverlay->getName() ),
                    LML_CRITICAL );
            }
        }
        //-----------------------------------------------------------------------
        void OverlayManager::skipToNextCloseBrace( DataStreamPtr &stream )
        {
            String line;
            while( !stream->eof() && line != "}" )
            {
                line = stream->getLine();
            }
        }
        //-----------------------------------------------------------------------
        void OverlayManager::skipToNextOpenBrace( DataStreamPtr &stream )
        {
            String line;
            while( !stream->eof() && line != "{" )
            {
                line = stream->getLine();
            }
        }
        //---------------------------------------------------------------------
        int OverlayManager::getViewportHeight() const { return mLastViewportHeight; }
        //---------------------------------------------------------------------
        int OverlayManager::getViewportWidth() const { return mLastViewportWidth; }
        //---------------------------------------------------------------------
        Real OverlayManager::getViewportAspectRatio() const
        {
            return (Real)mLastViewportWidth / (Real)mLastViewportHeight;
        }
        //---------------------------------------------------------------------
        OverlayManager::ElementMap &OverlayManager::getElementMap( bool isATemplate )
        {
            return ( isATemplate ) ? mTemplates : mInstances;
        }
        //---------------------------------------------------------------------
        OverlayElement *OverlayManager::createOverlayElementFromTemplate( const String &templateName,
                                                                          const String &typeName,
                                                                          const String &instanceName,
                                                                          bool isATemplate )
        {
            OverlayElement *newObj = NULL;

            if( templateName.empty() )
            {
                newObj = createOverlayElement( typeName, instanceName, isATemplate );
            }
            else
            {
                // no template
                OverlayElement *templateGui = getOverlayElement( templateName, true );

                String typeNameToCreate;
                if( typeName.empty() )
                {
                    typeNameToCreate = templateGui->getTypeName();
                }
                else
                {
                    typeNameToCreate = typeName;
                }

                newObj = createOverlayElement( typeNameToCreate, instanceName, isATemplate );

                ( (OverlayContainer *)newObj )->copyFromTemplate( templateGui );
            }

            return newObj;
        }

        //---------------------------------------------------------------------
        OverlayElement *OverlayManager::cloneOverlayElementFromTemplate( const String &templateName,
                                                                         const String &instanceName )
        {
            OverlayElement *templateGui = getOverlayElement( templateName, true );
            return templateGui->clone( instanceName );
        }

        //---------------------------------------------------------------------
        OverlayElement *OverlayManager::createOverlayElement( const String &typeName,
                                                              const String &instanceName,
                                                              bool isATemplate )
        {
            return createOverlayElementImpl( typeName, instanceName, getElementMap( isATemplate ) );
        }

        //---------------------------------------------------------------------
        OverlayElement *OverlayManager::createOverlayElementImpl( const String &typeName,
                                                                  const String &instanceName,
                                                                  ElementMap &elementMap )
        {
            // Check not duplicated
            ElementMap::iterator ii = elementMap.find( instanceName );
            if( ii != elementMap.end() )
            {
                OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                             "OverlayElement with name " + instanceName + " already exists.",
                             "OverlayManager::createOverlayElement" );
            }
            OverlayElement *newElem = createOverlayElementFromFactory( typeName, instanceName );

            // Register
            elementMap.insert( ElementMap::value_type( instanceName, newElem ) );

            return newElem;
        }

        //---------------------------------------------------------------------
        OverlayElement *OverlayManager::createOverlayElementFromFactory( const String &typeName,
                                                                         const String &instanceName )
        {
            // Look up factory
            FactoryMap::iterator fi = mFactories.find( typeName );
            if( fi == mFactories.end() )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                             "Cannot locate factory for element type " + typeName,
                             "OverlayManager::createOverlayElement" );
            }

            // create
            return fi->second->createOverlayElement( instanceName );
        }

        //---------------------------------------------------------------------
        OverlayElement *OverlayManager::getOverlayElement( const String &name, bool isATemplate )
        {
            return getOverlayElementImpl( name, getElementMap( isATemplate ) );
        }
        //---------------------------------------------------------------------
        bool OverlayManager::hasOverlayElement( const String &name, bool isATemplate )
        {
            return hasOverlayElementImpl( name, getElementMap( isATemplate ) );
        }
        //---------------------------------------------------------------------
        OverlayElement *OverlayManager::getOverlayElementImpl( const String &name,
                                                               ElementMap &elementMap )
        {
            // Locate instance
            ElementMap::iterator ii = elementMap.find( name );
            if( ii == elementMap.end() )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                             "OverlayElement with name " + name + " not found.",
                             "OverlayManager::getOverlayElementImpl" );
            }

            return ii->second;
        }
        //---------------------------------------------------------------------
        bool OverlayManager::hasOverlayElementImpl( const String &name, ElementMap &elementMap )
        {
            ElementMap::iterator ii = elementMap.find( name );
            return ii != elementMap.end();
        }
        //---------------------------------------------------------------------
        void OverlayManager::destroyOverlayElement( const String &instanceName, bool isATemplate )
        {
            destroyOverlayElementImpl( instanceName, getElementMap( isATemplate ) );
        }

        //---------------------------------------------------------------------
        void OverlayManager::destroyOverlayElement( OverlayElement *pInstance, bool isATemplate )
        {
            destroyOverlayElementImpl( pInstance->getName(), getElementMap( isATemplate ) );
        }

        //---------------------------------------------------------------------
        void OverlayManager::destroyOverlayElementImpl( const String &instanceName,
                                                        ElementMap &elementMap )
        {
            // Locate instance
            ElementMap::iterator ii = elementMap.find( instanceName );
            if( ii == elementMap.end() )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                             "OverlayElement with name " + instanceName + " not found.",
                             "OverlayManager::destroyOverlayElement" );
            }
            // Look up factory
            const String &typeName = ii->second->getTypeName();
            FactoryMap::iterator fi = mFactories.find( typeName );
            if( fi == mFactories.end() )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                             "Cannot locate factory for element type " + typeName,
                             "OverlayManager::destroyOverlayElement" );
            }

            fi->second->destroyOverlayElement( ii->second );
            elementMap.erase( ii );
        }
        //---------------------------------------------------------------------
        void OverlayManager::destroyAllOverlayElements( bool isATemplate )
        {
            destroyAllOverlayElementsImpl( getElementMap( isATemplate ) );
        }
        //---------------------------------------------------------------------
        void OverlayManager::destroyAllOverlayElementsImpl( ElementMap &elementMap )
        {
            ElementMap::iterator i;

            while( ( i = elementMap.begin() ) != elementMap.end() )
            {
                OverlayElement *element = i->second;

                // Get factory to delete
                FactoryMap::iterator fi = mFactories.find( element->getTypeName() );
                if( fi == mFactories.end() )
                {
                    OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                 "Cannot locate factory for element " + element->getName(),
                                 "OverlayManager::destroyAllOverlayElements" );
                }

                // remove from parent, if any
                OverlayContainer *parent;
                if( ( parent = element->getParent() ) != 0 )
                {
                    parent->_removeChild( element->getName() );
                }

                // children of containers will be auto-removed when container is destroyed.
                // destroy the element and remove it from the list
                fi->second->destroyOverlayElement( element );
                elementMap.erase( i );
            }
        }
        //---------------------------------------------------------------------
        void OverlayManager::addOverlayElementFactory( OverlayElementFactory *elemFactory )
        {
            // Add / replace
            mFactories[elemFactory->getTypeName()] = elemFactory;

            LogManager::getSingleton().logMessage( "OverlayElementFactory for type " +
                                                   elemFactory->getTypeName() + " registered." );
        }
    }  // namespace v1
}  // namespace Ogre
