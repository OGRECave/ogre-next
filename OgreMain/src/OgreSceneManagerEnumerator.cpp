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

#include "OgreSceneManagerEnumerator.h"

#include "OgreException.h"
#include "OgreLogManager.h"

namespace Ogre
{
    //-----------------------------------------------------------------------
    template <>
    SceneManagerEnumerator *Singleton<SceneManagerEnumerator>::msSingleton = 0;
    SceneManagerEnumerator *SceneManagerEnumerator::getSingletonPtr() { return msSingleton; }
    SceneManagerEnumerator &SceneManagerEnumerator::getSingleton()
    {
        assert( msSingleton );
        return ( *msSingleton );
    }

    //-----------------------------------------------------------------------
    SceneManagerEnumerator::SceneManagerEnumerator() :
        mInstanceCreateCount( 0 ),
        mCurrentRenderSystem( 0 )
    {
        addFactory( &mDefaultFactory );
    }
    //-----------------------------------------------------------------------
    SceneManagerEnumerator::~SceneManagerEnumerator()
    {
        // Destroy all remaining instances
        // Really should have shutdown and unregistered by now, but catch here in case
        Instances instancesCopy = mInstances;
        for( Instances::iterator i = instancesCopy.begin(); i != instancesCopy.end(); ++i )
        {
            // destroy instances
            for( Factories::iterator f = mFactories.begin(); f != mFactories.end(); ++f )
            {
                if( ( *f )->getMetaData().typeName == i->second->getTypeName() )
                {
                    ( *f )->destroyInstance( i->second );
                    mInstances.erase( i->first );
                    break;
                }
            }
        }
        mInstances.clear();
    }
    //-----------------------------------------------------------------------
    void SceneManagerEnumerator::addFactory( SceneManagerFactory *fact )
    {
        mFactories.push_back( fact );
        // add to metadata
        mMetaDataList.push_back( &fact->getMetaData() );
        // Log
        LogManager::getSingleton().logMessage( "SceneManagerFactory for type '" +
                                               fact->getMetaData().typeName + "' registered." );
    }
    //-----------------------------------------------------------------------
    void SceneManagerEnumerator::removeFactory( SceneManagerFactory *fact )
    {
        if( !fact )
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Cannot remove a null SceneManagerFactory.",
                         "SceneManagerEnumerator::removeFactory" );

        // destroy all instances for this factory
        for( Instances::iterator i = mInstances.begin(); i != mInstances.end(); )
        {
            SceneManager *instance = i->second;
            if( instance->getTypeName() == fact->getMetaData().typeName )
            {
                fact->destroyInstance( instance );
                Instances::iterator deli = i++;
                mInstances.erase( deli );
            }
            else
            {
                ++i;
            }
        }
        // remove from metadata
        for( MetaDataList::iterator m = mMetaDataList.begin(); m != mMetaDataList.end(); ++m )
        {
            if( *m == &( fact->getMetaData() ) )
            {
                mMetaDataList.erase( m );
                break;
            }
        }
        mFactories.remove( fact );
    }
    //-----------------------------------------------------------------------
    const SceneManagerMetaData *SceneManagerEnumerator::getMetaData( const String &typeName ) const
    {
        for( MetaDataList::const_iterator i = mMetaDataList.begin(); i != mMetaDataList.end(); ++i )
        {
            if( typeName == ( *i )->typeName )
            {
                return *i;
            }
        }

        OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                     "No metadata found for scene manager of type '" + typeName + "'",
                     "SceneManagerEnumerator::createSceneManager" );
    }
    //-----------------------------------------------------------------------
    SceneManagerEnumerator::MetaDataIterator SceneManagerEnumerator::getMetaDataIterator() const
    {
        return MetaDataIterator( mMetaDataList.begin(), mMetaDataList.end() );
    }
    //-----------------------------------------------------------------------
    SceneManager *SceneManagerEnumerator::createSceneManager( const String &typeName,
                                                              size_t numWorkerThreads,
                                                              const String &instanceName )
    {
        if( mInstances.find( instanceName ) != mInstances.end() )
        {
            OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                         "SceneManager instance called '" + instanceName + "' already exists",
                         "SceneManagerEnumerator::createSceneManager" );
        }

        SceneManager *inst = 0;
        for( Factories::iterator i = mFactories.begin(); i != mFactories.end(); ++i )
        {
            if( ( *i )->getMetaData().typeName == typeName )
            {
                if( instanceName.empty() )
                {
                    // generate a name
                    char tmpBuffer[32];
                    LwString str( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
                    str.a( "SceneManagerInstance", (uint32)++mInstanceCreateCount );
                    inst = ( *i )->createInstance( str.c_str(), numWorkerThreads );
                }
                else
                {
                    inst = ( *i )->createInstance( instanceName, numWorkerThreads );
                }
                break;
            }
        }

        if( !inst )
        {
            // Error!
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "No factory found for scene manager of type '" + typeName + "'",
                         "SceneManagerEnumerator::createSceneManager" );
        }

        /// assign rs if already configured
        if( mCurrentRenderSystem )
            inst->_setDestinationRenderSystem( mCurrentRenderSystem );

        mInstances[inst->getName()] = inst;

        return inst;
    }
    //-----------------------------------------------------------------------
    SceneManager *SceneManagerEnumerator::createSceneManager( SceneTypeMask typeMask,
                                                              size_t numWorkerThreads,
                                                              const String &instanceName )
    {
        if( mInstances.find( instanceName ) != mInstances.end() )
        {
            OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                         "SceneManager instance called '" + instanceName + "' already exists",
                         "SceneManagerEnumerator::createSceneManager" );
        }

        SceneManager *inst = 0;
        String name = instanceName;
        if( name.empty() )
        {
            // generate a name
            char tmpBuffer[32];
            LwString str( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
            str.a( "SceneManagerInstance", (uint32)++mInstanceCreateCount );
            name = str.c_str();
        }

        // Iterate backwards to find the matching factory registered last
        for( Factories::reverse_iterator i = mFactories.rbegin(); i != mFactories.rend(); ++i )
        {
            if( ( *i )->getMetaData().sceneTypeMask & typeMask )
            {
                inst = ( *i )->createInstance( name, numWorkerThreads );
                break;
            }
        }

        // use default factory if none
        if( !inst )
            inst = mDefaultFactory.createInstance( name, numWorkerThreads );

        /// assign rs if already configured
        if( mCurrentRenderSystem )
            inst->_setDestinationRenderSystem( mCurrentRenderSystem );

        mInstances[inst->getName()] = inst;

        return inst;
    }
    //-----------------------------------------------------------------------
    void SceneManagerEnumerator::destroySceneManager( SceneManager *sm )
    {
        if( !sm )
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Cannot destroy a null SceneManager.",
                         "SceneManagerEnumerator::destroySceneManager" );

        // Erase instance from map
        mInstances.erase( sm->getName() );

        // Find factory to destroy
        for( Factories::iterator i = mFactories.begin(); i != mFactories.end(); ++i )
        {
            if( ( *i )->getMetaData().typeName == sm->getTypeName() )
            {
                ( *i )->destroyInstance( sm );
                break;
            }
        }
    }
    //-----------------------------------------------------------------------
    SceneManager *SceneManagerEnumerator::getSceneManager( const String &instanceName ) const
    {
        Instances::const_iterator i = mInstances.find( instanceName );
        if( i != mInstances.end() )
        {
            return i->second;
        }
        else
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "SceneManager instance with name '" + instanceName + "' not found.",
                         "SceneManagerEnumerator::getSceneManager" );
        }
    }
    //---------------------------------------------------------------------
    bool SceneManagerEnumerator::hasSceneManager( const String &instanceName ) const
    {
        return mInstances.find( instanceName ) != mInstances.end();
    }
    //-----------------------------------------------------------------------
    SceneManagerEnumerator::SceneManagerIterator SceneManagerEnumerator::getSceneManagerIterator()
    {
        return SceneManagerIterator( mInstances.begin(), mInstances.end() );
    }
    //-----------------------------------------------------------------------
    void SceneManagerEnumerator::setRenderSystem( RenderSystem *rs )
    {
        mCurrentRenderSystem = rs;

        for( Instances::iterator i = mInstances.begin(); i != mInstances.end(); ++i )
        {
            i->second->_setDestinationRenderSystem( rs );
        }
    }
    //-----------------------------------------------------------------------
    void SceneManagerEnumerator::shutdownAll()
    {
        for( Instances::iterator i = mInstances.begin(); i != mInstances.end(); ++i )
        {
            // shutdown instances (clear scene)
            i->second->clearScene( true, false );
        }
    }
    //-----------------------------------------------------------------------
    const String DefaultSceneManagerFactory::FACTORY_TYPE_NAME = "DefaultSceneManager";
    //-----------------------------------------------------------------------
    void DefaultSceneManagerFactory::initMetaData() const
    {
        mMetaData.typeName = FACTORY_TYPE_NAME;
        mMetaData.description = "The default scene manager";
        mMetaData.sceneTypeMask = ST_GENERIC;
        mMetaData.worldGeometrySupported = false;
    }
    //-----------------------------------------------------------------------
    SceneManager *DefaultSceneManagerFactory::createInstance( const String &instanceName,
                                                              size_t numWorkerThreads )
    {
        return OGRE_NEW DefaultSceneManager( instanceName, numWorkerThreads );
    }
    //-----------------------------------------------------------------------
    void DefaultSceneManagerFactory::destroyInstance( SceneManager *instance ) { OGRE_DELETE instance; }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    DefaultSceneManager::DefaultSceneManager( const String &name, size_t numWorkerThreads ) :
        SceneManager( name, numWorkerThreads )
    {
    }
    //-----------------------------------------------------------------------
    DefaultSceneManager::~DefaultSceneManager() {}
    //-----------------------------------------------------------------------
    const String &DefaultSceneManager::getTypeName() const
    {
        return DefaultSceneManagerFactory::FACTORY_TYPE_NAME;
    }
    //-----------------------------------------------------------------------

}  // namespace Ogre
