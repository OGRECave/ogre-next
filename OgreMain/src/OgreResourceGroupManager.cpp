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

#include "OgreResourceGroupManager.h"

#include "OgreArchive.h"
#include "OgreArchiveManager.h"
#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreResourceManager.h"
#include "OgreSceneManager.h"
#include "OgreScriptLoader.h"
#include "OgreString.h"

#include <sstream>

namespace Ogre
{
    //-----------------------------------------------------------------------
    template <>
    ResourceGroupManager *Singleton<ResourceGroupManager>::msSingleton = 0;
    ResourceGroupManager *ResourceGroupManager::getSingletonPtr() { return msSingleton; }
    ResourceGroupManager &ResourceGroupManager::getSingleton()
    {
        assert( msSingleton );
        return ( *msSingleton );
    }
    String ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME = "General";
    String ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME = "Internal";
    String ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME = "Autodetect";
    // A reference count of 3 means that only RGM and RM have references
    // RGM has one (this one) and RM has 2 (by name and by handle)
    long ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS = 3;
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    ResourceGroupManager::ResourceGroupManager() : mLoadingListener( 0 ), mCurrentGroup( 0 )
    {
        // Create the 'General' group
        createResourceGroup( DEFAULT_RESOURCE_GROUP_NAME );
        // Create the 'Internal' group
        createResourceGroup( INTERNAL_RESOURCE_GROUP_NAME );
        // Create the 'Autodetect' group (only used for temp storage)
        createResourceGroup( AUTODETECT_RESOURCE_GROUP_NAME );
    }
    //-----------------------------------------------------------------------
    ResourceGroupManager::~ResourceGroupManager()
    {
        // delete all resource groups
        for( ResourceGroupMap::value_type &rge : mResourceGroupMap )
            deleteGroup( rge.second );

        mResourceGroupMap.clear();
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::createResourceGroup( const String &name,
                                                    const bool inGlobalPool /* = true */ )
    {
        OGRE_LOCK_AUTO_MUTEX;

        LogManager::getSingleton().logMessage( "Creating resource group " + name );
        if( getResourceGroup( name ) )
        {
            OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                         "Resource group with name '" + name + "' already exists!",
                         "ResourceGroupManager::createResourceGroup" );
        }
        ResourceGroup *grp = OGRE_NEW_T( ResourceGroup, MEMCATEGORY_RESOURCE )();
        grp->groupStatus = ResourceGroup::UNINITIALSED;
        grp->name = name;
        grp->inGlobalPool = inGlobalPool;
        mResourceGroupMap.emplace( name, grp );
    }
    //-----------------------------------------------------------------------
    class ScopedCLocale
    {
        char mSavedLocale[64];
        bool mChangeLocaleTemporarily;

    public:
        ScopedCLocale( bool changeLocaleTemporarily ) :
            mChangeLocaleTemporarily( changeLocaleTemporarily )
        {
            memset( mSavedLocale, 0, sizeof( mSavedLocale ) );
            if( mChangeLocaleTemporarily )
            {
                const char *currentLocale = setlocale( LC_NUMERIC, 0 );
                strncpy( mSavedLocale, currentLocale, 64u );
                mSavedLocale[63] = '\0';
                setlocale( LC_NUMERIC, "C" );
            }
        }
        ~ScopedCLocale()
        {
            if( mChangeLocaleTemporarily )
            {
                // Restore
                setlocale( LC_NUMERIC, mSavedLocale );
            }
        }
    };
    //-----------------------------------------------------------------------
    void ResourceGroupManager::initialiseResourceGroup( const String &name,
                                                        bool changeLocaleTemporarily )
    {
        OGRE_LOCK_AUTO_MUTEX;

        ScopedCLocale scopedCLocale( changeLocaleTemporarily );

        LogManager::getSingleton().logMessage( "Initialising resource group " + name );
        ResourceGroup *grp = getResourceGroup( name );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Cannot find a group named " + name,
                         "ResourceGroupManager::initialiseResourceGroup" );
        }
        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex;

        if( grp->groupStatus == ResourceGroup::UNINITIALSED )
        {
            // in the process of initialising
            grp->groupStatus = ResourceGroup::INITIALISING;
            // Set current group
            parseResourceGroupScripts( grp );
            mCurrentGroup = grp;
            LogManager::getSingleton().logMessage( "Creating resources for group " + name );
            createDeclaredResources( grp );
            grp->groupStatus = ResourceGroup::INITIALISED;
            LogManager::getSingleton().logMessage( "All done" );
            // Reset current group
            mCurrentGroup = 0;
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::initialiseAllResourceGroups( bool changeLocaleTemporarily )
    {
        OGRE_LOCK_AUTO_MUTEX;

        ScopedCLocale scopedCLocale( changeLocaleTemporarily );

        // Intialise all declared resource groups
        for( ResourceGroupMap::value_type &rge : mResourceGroupMap )
        {
            ResourceGroup *grp = rge.second;
            OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex
            if( grp->groupStatus == ResourceGroup::UNINITIALSED )
            {
                // in the process of initialising
                grp->groupStatus = ResourceGroup::INITIALISING;
                // Set current group
                mCurrentGroup = grp;
                parseResourceGroupScripts( grp );
                LogManager::getSingleton().logMessage( "Creating resources for group " + rge.first );
                createDeclaredResources( grp );
                grp->groupStatus = ResourceGroup::INITIALISED;
                LogManager::getSingleton().logMessage( "All done" );
                // Reset current group
                mCurrentGroup = 0;
            }
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::prepareResourceGroup( const String &name, bool prepareMainResources )
    {
        // Can only bulk-load one group at a time (reasonable limitation I think)
        OGRE_LOCK_AUTO_MUTEX;

        LogManager::getSingleton().stream()
            << "Preparing resource group '" << name << "' - Resources: " << prepareMainResources;
        // load all created resources
        ResourceGroup *grp = getResourceGroup( name );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Cannot find a group named " + name,
                         "ResourceGroupManager::prepareResourceGroup" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex
        // Set current group
        mCurrentGroup = grp;

        // Count up resources for starting event
        ResourceGroup::LoadResourceOrderMap::iterator oi;
        size_t resourceCount = 0;
        if( prepareMainResources )
        {
            for( oi = grp->loadResourceOrderMap.begin(); oi != grp->loadResourceOrderMap.end(); ++oi )
            {
                resourceCount += oi->second->size();
            }
        }

        fireResourceGroupPrepareStarted( name, resourceCount );

        // Now load for real
        if( prepareMainResources )
        {
            for( oi = grp->loadResourceOrderMap.begin(); oi != grp->loadResourceOrderMap.end(); ++oi )
            {
                size_t n = 0;
                LoadUnloadResourceSet::iterator l = oi->second->begin();
                while( l != oi->second->end() )
                {
                    ResourcePtr res = *l;

                    // Fire resource events no matter whether resource needs preparing
                    // or not. This ensures that the number of callbacks
                    // matches the number originally estimated, which is important
                    // for progress bars.
                    fireResourcePrepareStarted( res );

                    // If preparing one of these resources cascade-prepares another resource,
                    // the list will get longer! But these should be prepared immediately
                    // Call prepare regardless, already prepared or loaded resources will be skipped
                    res->prepare();

                    fireResourcePrepareEnded();

                    ++n;

                    // Did the resource change group? if so, our iterator will have
                    // been invalidated
                    if( res->getGroup() != name )
                    {
                        l = oi->second->begin();
                        std::advance( l, static_cast<ptrdiff_t>( n ) );
                    }
                    else
                    {
                        ++l;
                    }
                }
            }
        }
        fireResourceGroupPrepareEnded( name );

        // reset current group
        mCurrentGroup = 0;

        LogManager::getSingleton().logMessage( "Finished preparing resource group " + name );
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::loadResourceGroup( const String &name, bool loadMainResources )
    {
        // Can only bulk-load one group at a time (reasonable limitation I think)
        OGRE_LOCK_AUTO_MUTEX;

        LogManager::getSingleton().stream()
            << "Loading resource group '" << name << "' - Resources: " << loadMainResources;
        // load all created resources
        ResourceGroup *grp = getResourceGroup( name );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Cannot find a group named " + name,
                         "ResourceGroupManager::loadResourceGroup" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex
        // Set current group
        mCurrentGroup = grp;

        // Count up resources for starting event
        ResourceGroup::LoadResourceOrderMap::iterator oi;
        size_t resourceCount = 0;
        if( loadMainResources )
        {
            for( oi = grp->loadResourceOrderMap.begin(); oi != grp->loadResourceOrderMap.end(); ++oi )
            {
                resourceCount += oi->second->size();
            }
        }
        fireResourceGroupLoadStarted( name, resourceCount );

        // Now load for real
        if( loadMainResources )
        {
            for( oi = grp->loadResourceOrderMap.begin(); oi != grp->loadResourceOrderMap.end(); ++oi )
            {
                size_t n = 0;
                LoadUnloadResourceSet::iterator l = oi->second->begin();
                while( l != oi->second->end() )
                {
                    ResourcePtr res = *l;

                    // Fire resource events no matter whether resource is already
                    // loaded or not. This ensures that the number of callbacks
                    // matches the number originally estimated, which is important
                    // for progress bars.
                    fireResourceLoadStarted( res );

                    // If loading one of these resources cascade-loads another resource,
                    // the list will get longer! But these should be loaded immediately
                    // Call load regardless, already loaded resources will be skipped
                    res->load();

                    fireResourceLoadEnded();

                    ++n;

                    // Did the resource change group? if so, our iterator will have
                    // been invalidated
                    if( res->getGroup() != name )
                    {
                        l = oi->second->begin();
                        std::advance( l, static_cast<ptrdiff_t>( n ) );
                    }
                    else
                    {
                        ++l;
                    }
                }
            }
        }
        fireResourceGroupLoadEnded( name );

        // group is loaded
        grp->groupStatus = ResourceGroup::LOADED;

        // reset current group
        mCurrentGroup = 0;

        LogManager::getSingleton().logMessage( "Finished loading resource group " + name );
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::unloadResourceGroup( const String &name, bool reloadableOnly )
    {
        // Can only bulk-unload one group at a time (reasonable limitation I think)
        OGRE_LOCK_AUTO_MUTEX;

        LogManager::getSingleton().logMessage( "Unloading resource group " + name );
        ResourceGroup *grp = getResourceGroup( name );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Cannot find a group named " + name,
                         "ResourceGroupManager::unloadResourceGroup" );
        }
        // Set current group
        mCurrentGroup = grp;
        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        // Count up resources for starting event
        ResourceGroup::LoadResourceOrderMap::reverse_iterator oi;
        // unload in reverse order
        for( oi = grp->loadResourceOrderMap.rbegin(); oi != grp->loadResourceOrderMap.rend(); ++oi )
        {
            for( LoadUnloadResourceSet::iterator l = oi->second->begin(); l != oi->second->end(); ++l )
            {
                Resource *resource = l->get();
                if( !reloadableOnly || resource->isReloadable() )
                {
                    resource->unload();
                }
            }
        }

        grp->groupStatus = ResourceGroup::INITIALISED;

        // reset current group
        mCurrentGroup = 0;
        LogManager::getSingleton().logMessage( "Finished unloading resource group " + name );
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::unloadUnreferencedResourcesInGroup( const String &name,
                                                                   bool reloadableOnly )
    {
        // Can only bulk-unload one group at a time (reasonable limitation I think)
        OGRE_LOCK_AUTO_MUTEX;

        LogManager::getSingleton().logMessage( "Unloading unused resources in resource group " + name );
        ResourceGroup *grp = getResourceGroup( name );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Cannot find a group named " + name,
                         "ResourceGroupManager::unloadUnreferencedResourcesInGroup" );
        }
        // Set current group
        mCurrentGroup = grp;

        ResourceGroup::LoadResourceOrderMap::reverse_iterator oi;
        // unload in reverse order
        for( oi = grp->loadResourceOrderMap.rbegin(); oi != grp->loadResourceOrderMap.rend(); ++oi )
        {
            for( LoadUnloadResourceSet::iterator l = oi->second->begin(); l != oi->second->end(); ++l )
            {
                // A use count of 3 means that only RGM and RM have references
                // RGM has one (this one) and RM has 2 (by name and by handle)
                if( l->use_count() == RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS )
                {
                    Resource *resource = l->get();
                    if( !reloadableOnly || resource->isReloadable() )
                    {
                        resource->unload();
                    }
                }
            }
        }

        grp->groupStatus = ResourceGroup::INITIALISED;

        // reset current group
        mCurrentGroup = 0;
        LogManager::getSingleton().logMessage( "Finished unloading unused resources in resource group " +
                                               name );
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::clearResourceGroup( const String &name )
    {
        // Can only bulk-clear one group at a time (reasonable limitation I think)
        OGRE_LOCK_AUTO_MUTEX;

        LogManager::getSingleton().logMessage( "Clearing resource group " + name );
        ResourceGroup *grp = getResourceGroup( name );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Cannot find a group named " + name,
                         "ResourceGroupManager::clearResourceGroup" );
        }
        // set current group
        mCurrentGroup = grp;
        dropGroupContents( grp );
        // clear initialised flag
        grp->groupStatus = ResourceGroup::UNINITIALSED;
        // reset current group
        mCurrentGroup = 0;
        LogManager::getSingleton().logMessage( "Finished clearing resource group " + name );
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::destroyResourceGroup( const String &name )
    {
        // Can only bulk-destroy one group at a time (reasonable limitation I think)
        OGRE_LOCK_AUTO_MUTEX;

        LogManager::getSingleton().logMessage( "Destroying resource group " + name );
        ResourceGroup *grp = getResourceGroup( name );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Cannot find a group named " + name,
                         "ResourceGroupManager::destroyResourceGroup" );
        }
        // set current group
        mCurrentGroup = grp;
        unloadResourceGroup( name, false );  // will throw an exception if name not valid
        dropGroupContents( grp );
        deleteGroup( grp );
        mResourceGroupMap.erase( mResourceGroupMap.find( name ) );
        // reset current group
        mCurrentGroup = 0;
    }
    //-----------------------------------------------------------------------
    bool ResourceGroupManager::isResourceGroupInitialised( const String &name )
    {
        // Can only bulk-destroy one group at a time (reasonable limitation I think)
        OGRE_LOCK_AUTO_MUTEX;

        ResourceGroup *grp = getResourceGroup( name );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Cannot find a group named " + name,
                         "ResourceGroupManager::isResourceGroupInitialised" );
        }
        return ( grp->groupStatus != ResourceGroup::UNINITIALSED &&
                 grp->groupStatus != ResourceGroup::INITIALISING );
    }
    //-----------------------------------------------------------------------
    bool ResourceGroupManager::isResourceGroupLoaded( const String &name )
    {
        // Can only bulk-destroy one group at a time (reasonable limitation I think)
        OGRE_LOCK_AUTO_MUTEX;

        ResourceGroup *grp = getResourceGroup( name );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Cannot find a group named " + name,
                         "ResourceGroupManager::isResourceGroupInitialised" );
        }
        return ( grp->groupStatus == ResourceGroup::LOADED );
    }
    //-----------------------------------------------------------------------
    bool ResourceGroupManager::resourceGroupExists( const String &name )
    {
        return getResourceGroup( name ) ? true : false;
    }
    //-----------------------------------------------------------------------
    bool ResourceGroupManager::resourceLocationExists( const String &name, const String &resGroup )
    {
        ResourceGroup *grp = getResourceGroup( resGroup );
        if( !grp )
            return false;

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        LocationList::iterator li, liend;
        liend = grp->locationList.end();
        for( li = grp->locationList.begin(); li != liend; ++li )
        {
            Archive *pArch = ( *li )->archive;
            if( pArch->getName() == name )
                // Delete indexes
                return true;
        }
        return false;
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::addResourceLocation( const String &name, const String &locType,
                                                    const String &resGroup, bool recursive,
                                                    bool readOnly )
    {
        ResourceGroup *grp = getResourceGroup( resGroup );
        if( !grp )
        {
            createResourceGroup( resGroup );
            grp = getResourceGroup( resGroup );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        // Get archive
        Archive *pArch = ArchiveManager::getSingleton().load( name, locType, readOnly );
        // Add to location list
        ResourceLocation *loc = OGRE_NEW_T( ResourceLocation, MEMCATEGORY_RESOURCE );
        loc->archive = pArch;
        loc->recursive = recursive;
        grp->locationList.push_back( loc );
        // Index resources
        StringVectorPtr vec = pArch->find( "*", recursive );
        for( StringVector::iterator it = vec->begin(); it != vec->end(); ++it )
            grp->addToIndex( *it, pArch );

        StringStream msg;
        msg << "Added resource location '" << name << "' of type '" << locType << "' to resource group '"
            << resGroup << "'";
        if( recursive )
            msg << " with recursive option";
        LogManager::getSingleton().logMessage( msg.str() );
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::removeResourceLocation( const String &name, const String &resGroup )
    {
        ResourceGroup *grp = getResourceGroup( resGroup );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot locate a resource group called '" + resGroup + "'",
                         "ResourceGroupManager::removeResourceLocation" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        // Remove from location list
        LocationList::iterator li, liend;
        liend = grp->locationList.end();
        for( li = grp->locationList.begin(); li != liend; ++li )
        {
            Archive *pArch = ( *li )->archive;
            if( pArch->getName() == name )
            {
                grp->removeFromIndex( pArch );
                // Erase list entry
                OGRE_DELETE_T( *li, ResourceLocation, MEMCATEGORY_RESOURCE );
                grp->locationList.erase( li );

                break;
            }
        }

        LogManager::getSingleton().logMessage( "Removed resource location " + name );
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::declareResource( const String &name, const String &resourceType,
                                                const String &groupName,
                                                const NameValuePairList &loadParameters )
    {
        declareResource( name, resourceType, groupName, 0, loadParameters );
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::declareResource( const String &name, const String &resourceType,
                                                const String &groupName, ManualResourceLoader *loader,
                                                const NameValuePairList &loadParameters )
    {
        ResourceGroup *grp = getResourceGroup( groupName );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Cannot find a group named " + groupName,
                         "ResourceGroupManager::declareResource" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        ResourceDeclaration dcl;
        dcl.loader = loader;
        dcl.parameters = loadParameters;
        dcl.resourceName = name;
        dcl.resourceType = resourceType;
        grp->resourceDeclarations.push_back( dcl );
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::undeclareResource( const String &name, const String &groupName )
    {
        ResourceGroup *grp = getResourceGroup( groupName );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Cannot find a group named " + groupName,
                         "ResourceGroupManager::undeclareResource" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        for( ResourceDeclarationList::iterator i = grp->resourceDeclarations.begin();
             i != grp->resourceDeclarations.end(); ++i )
        {
            if( i->resourceName == name )
            {
                grp->resourceDeclarations.erase( i );
                break;
            }
        }
    }
    //-----------------------------------------------------------------------
    DataStreamPtr ResourceGroupManager::openResource( const String &resourceName,
                                                      const String &groupName,
                                                      bool searchGroupsIfNotFound,
                                                      Resource *resourceBeingLoaded )
    {
        OgreAssert( !resourceName.empty(), "resourceName is empty string" );
        OGRE_LOCK_AUTO_MUTEX;

        if( mLoadingListener )
        {
            DataStreamPtr stream =
                mLoadingListener->resourceLoading( resourceName, groupName, resourceBeingLoaded );
            if( stream )
                return stream;
        }

        // Try to find in resource index first
        ResourceGroup *grp = getResourceGroup( groupName );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot locate a resource group called '" + groupName + "' for resource '" +
                             resourceName + "'",
                         "ResourceGroupManager::openResource" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        Archive *pArch = 0;
        ResourceLocationIndex::iterator rit = grp->resourceIndexCaseSensitive.find( resourceName );
        if( rit != grp->resourceIndexCaseSensitive.end() )
        {
            // Found in the index
            pArch = rit->second;
            DataStreamPtr stream = pArch->open( resourceName );
            if( mLoadingListener )
                mLoadingListener->resourceStreamOpened( resourceName, groupName, resourceBeingLoaded,
                                                        stream );
            return stream;
        }
        else
        {
            // try case insensitive
            String lcResourceName = resourceName;
            StringUtil::toLowerCase( lcResourceName );
            rit = grp->resourceIndexCaseInsensitive.find( lcResourceName );
            if( rit != grp->resourceIndexCaseInsensitive.end() )
            {
                // Found in the index
                pArch = rit->second;
                DataStreamPtr stream = pArch->open( resourceName );
                if( mLoadingListener )
                    mLoadingListener->resourceStreamOpened( resourceName, groupName, resourceBeingLoaded,
                                                            stream );
                return stream;
            }
            else
            {
                // Search the hard way
                LocationList::iterator li, liend;
                liend = grp->locationList.end();
                for( li = grp->locationList.begin(); li != liend; ++li )
                {
                    Archive *arch = ( *li )->archive;
                    if( arch->exists( resourceName ) )
                    {
                        DataStreamPtr ptr = arch->open( resourceName );
                        if( mLoadingListener )
                            mLoadingListener->resourceStreamOpened( resourceName, groupName,
                                                                    resourceBeingLoaded, ptr );
                        return ptr;
                    }
                }
            }
        }

        // Not found
        if( searchGroupsIfNotFound )
        {
            ResourceGroup *foundGrp = findGroupContainingResourceImpl( resourceName );
            if( foundGrp )
            {
                if( resourceBeingLoaded )
                {
                    resourceBeingLoaded->changeGroupOwnership( foundGrp->name );
                }
                return openResource( resourceName, foundGrp->name, false );
            }
            else
            {
                OGRE_EXCEPT( Exception::ERR_FILE_NOT_FOUND,
                             "Cannot locate resource " + resourceName + " in resource group " +
                                 groupName + " or any other group.",
                             "ResourceGroupManager::openResource" );
            }
        }
        OGRE_EXCEPT( Exception::ERR_FILE_NOT_FOUND,
                     "Cannot locate resource " + resourceName + " in resource group " + groupName + ".",
                     "ResourceGroupManager::openResource" );
    }
    //-----------------------------------------------------------------------
    Archive *ResourceGroupManager::_getArchiveToResource( const String &resourceName,
                                                          const String &groupName,
                                                          bool searchGroupsIfNotFound )
    {
        OgreAssert( !resourceName.empty(), "resourceName is empty string" );
        OGRE_LOCK_AUTO_MUTEX;

        /*TODO
        if(mLoadingListener)
        {
            DataStreamPtr stream = mLoadingListener->resourceLoading(resourceName, groupName,
        resourceBeingLoaded); if(stream) return stream;
        }*/

        // Try to find in resource index first
        ResourceGroup *grp = getResourceGroup( groupName );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot locate a resource group called '" + groupName + "' for resource '" +
                             resourceName + "'",
                         "ResourceGroupManager::_getArchiveToResource" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        Archive *pArch = 0;
        ResourceLocationIndex::iterator rit = grp->resourceIndexCaseSensitive.find( resourceName );
        if( rit != grp->resourceIndexCaseSensitive.end() )
        {
            // Found in the index
            pArch = rit->second;
            return pArch;
        }
        else
        {
            // try case insensitive
            String lcResourceName = resourceName;
            StringUtil::toLowerCase( lcResourceName );
            rit = grp->resourceIndexCaseInsensitive.find( lcResourceName );
            if( rit != grp->resourceIndexCaseInsensitive.end() )
            {
                // Found in the index
                pArch = rit->second;
                return pArch;
            }
            else
            {
                // Search the hard way
                LocationList::iterator li, liend;
                liend = grp->locationList.end();
                for( li = grp->locationList.begin(); li != liend; ++li )
                {
                    Archive *arch = ( *li )->archive;
                    if( arch->exists( resourceName ) )
                        return arch;
                }
            }
        }

        // Not found
        if( searchGroupsIfNotFound )
        {
            ResourceGroup *foundGrp = findGroupContainingResourceImpl( resourceName );
            if( foundGrp )
            {
                return _getArchiveToResource( resourceName, foundGrp->name, false );
            }
            else
            {
                OGRE_EXCEPT( Exception::ERR_FILE_NOT_FOUND,
                             "Cannot locate resource " + resourceName + " in resource group " +
                                 groupName + " or any other group.",
                             "ResourceGroupManager::_getArchiveToResource" );
            }
        }

        OGRE_EXCEPT( Exception::ERR_FILE_NOT_FOUND,
                     "Cannot locate resource " + resourceName + " in resource group " + groupName + ".",
                     "ResourceGroupManager::_getArchiveToResource" );
    }
    //-----------------------------------------------------------------------
    DataStreamListPtr ResourceGroupManager::openResources( const String &pattern,
                                                           const String &groupName )
    {
        OGRE_LOCK_AUTO_MUTEX;
        ResourceGroup *grp = getResourceGroup( groupName );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot locate a resource group called '" + groupName + "'",
                         "ResourceGroupManager::openResources" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        // Iterate through all the archives and build up a combined list of
        // streams
        // MEMCATEGORY_GENERAL is the only category supported for SharedPtr
        DataStreamListPtr ret =
            DataStreamListPtr( OGRE_NEW_T( DataStreamList, MEMCATEGORY_GENERAL )(), SPFM_DELETE_T );

        LocationList::iterator li, liend;
        liend = grp->locationList.end();
        for( li = grp->locationList.begin(); li != liend; ++li )
        {
            Archive *arch = ( *li )->archive;
            // Find all the names based on whether this archive is recursive
            StringVectorPtr names = arch->find( pattern, ( *li )->recursive );

            // Iterate over the names and load a stream for each
            for( StringVector::iterator ni = names->begin(); ni != names->end(); ++ni )
            {
                DataStreamPtr ptr = arch->open( *ni );
                if( ptr )
                {
                    ret->push_back( ptr );
                }
            }
        }
        return ret;
    }
    //---------------------------------------------------------------------
    DataStreamPtr ResourceGroupManager::createResource( const String &filename, const String &groupName,
                                                        bool overwrite, const String &locationPattern )
    {
        OGRE_LOCK_AUTO_MUTEX;
        ResourceGroup *grp = getResourceGroup( groupName );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot locate a resource group called '" + groupName + "'",
                         "ResourceGroupManager::createResource" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        for( LocationList::iterator li = grp->locationList.begin(); li != grp->locationList.end(); ++li )
        {
            Archive *arch = ( *li )->archive;

            if( !arch->isReadOnly() && ( locationPattern.empty() ||
                                         StringUtil::match( arch->getName(), locationPattern, false ) ) )
            {
                if( !overwrite && arch->exists( filename ) )
                    OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                                 "Cannot overwrite existing file " + filename,
                                 "ResourceGroupManager::createResource" );

                // create it
                DataStreamPtr ret = arch->create( filename );
                grp->addToIndex( filename, arch );

                return ret;
            }
        }

        OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                     "Cannot find a writable location in group " + groupName,
                     "ResourceGroupManager::createResource" );
    }
    //---------------------------------------------------------------------
    void ResourceGroupManager::deleteResource( const String &filename, const String &groupName,
                                               const String &locationPattern )
    {
        OGRE_LOCK_AUTO_MUTEX;
        ResourceGroup *grp = getResourceGroup( groupName );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot locate a resource group called '" + groupName + "'",
                         "ResourceGroupManager::createResource" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        for( LocationList::iterator li = grp->locationList.begin(); li != grp->locationList.end(); ++li )
        {
            Archive *arch = ( *li )->archive;

            if( !arch->isReadOnly() && ( locationPattern.empty() ||
                                         StringUtil::match( arch->getName(), locationPattern, false ) ) )
            {
                if( arch->exists( filename ) )
                {
                    arch->remove( filename );
                    grp->removeFromIndex( filename, arch );

                    // only remove one file
                    break;
                }
            }
        }
    }
    //---------------------------------------------------------------------
    void ResourceGroupManager::deleteMatchingResources( const String &filePattern,
                                                        const String &groupName,
                                                        const String &locationPattern )
    {
        OGRE_LOCK_AUTO_MUTEX;
        ResourceGroup *grp = getResourceGroup( groupName );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot locate a resource group called '" + groupName + "'",
                         "ResourceGroupManager::createResource" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        for( LocationList::iterator li = grp->locationList.begin(); li != grp->locationList.end(); ++li )
        {
            Archive *arch = ( *li )->archive;

            if( !arch->isReadOnly() && ( locationPattern.empty() ||
                                         StringUtil::match( arch->getName(), locationPattern, false ) ) )
            {
                StringVectorPtr matchingFiles = arch->find( filePattern );
                for( StringVector::iterator f = matchingFiles->begin(); f != matchingFiles->end(); ++f )
                {
                    arch->remove( *f );
                    grp->removeFromIndex( *f, arch );
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::addResourceGroupListener( ResourceGroupListener *l )
    {
        OGRE_LOCK_AUTO_MUTEX;

        mResourceGroupListenerList.push_back( l );
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::removeResourceGroupListener( ResourceGroupListener *l )
    {
        OGRE_LOCK_AUTO_MUTEX;

        for( ResourceGroupListenerList::iterator i = mResourceGroupListenerList.begin();
             i != mResourceGroupListenerList.end(); ++i )
        {
            if( *i == l )
            {
                mResourceGroupListenerList.erase( i );
                break;
            }
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::_registerResourceManager( const String &resourceType,
                                                         ResourceManager *rm )
    {
        OGRE_LOCK_AUTO_MUTEX;

        LogManager::getSingleton().logMessage( "Registering ResourceManager for type " + resourceType );
        mResourceManagerMap[resourceType] = rm;
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::_unregisterResourceManager( const String &resourceType )
    {
        OGRE_LOCK_AUTO_MUTEX;

        LogManager::getSingleton().logMessage( "Unregistering ResourceManager for type " +
                                               resourceType );

        ResourceManagerMap::iterator i = mResourceManagerMap.find( resourceType );
        if( i != mResourceManagerMap.end() )
        {
            mResourceManagerMap.erase( i );
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::_registerScriptLoader( ScriptLoader *su )
    {
        OGRE_LOCK_AUTO_MUTEX;

        mScriptLoaderOrderMap.emplace( su->getLoadingOrder(), su );
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::_unregisterScriptLoader( ScriptLoader *su )
    {
        OGRE_LOCK_AUTO_MUTEX;

        Real order = su->getLoadingOrder();
        ScriptLoaderOrderMap::iterator oi = mScriptLoaderOrderMap.find( order );
        while( oi != mScriptLoaderOrderMap.end() && oi->first == order )
        {
            if( oi->second == su )
            {
                // erase does not invalidate on multimap, except current
                ScriptLoaderOrderMap::iterator del = oi++;
                mScriptLoaderOrderMap.erase( del );
            }
            else
            {
                ++oi;
            }
        }
    }
    //-----------------------------------------------------------------------
    ScriptLoader *ResourceGroupManager::_findScriptLoader( const String &pattern )
    {
        OGRE_LOCK_AUTO_MUTEX;

        ScriptLoaderOrderMap::iterator oi;
        for( oi = mScriptLoaderOrderMap.begin(); oi != mScriptLoaderOrderMap.end(); ++oi )
        {
            ScriptLoader *su = oi->second;
            const StringVector &patterns = su->getScriptPatterns();

            // Search for matches in the patterns
            for( StringVector::const_iterator p = patterns.begin(); p != patterns.end(); ++p )
            {
                if( *p == pattern )
                    return su;
            }
        }

        return 0;  // No loader was found
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::parseResourceGroupScripts( ResourceGroup *grp )
    {
        LogManager::getSingleton().logMessage( "Parsing scripts for resource group " + grp->name );

        // Count up the number of scripts we have to parse
        typedef list<FileInfoListPtr>::type FileListList;
        typedef SharedPtr<FileListList> FileListListPtr;
        typedef std::pair<ScriptLoader *, FileListListPtr> LoaderFileListPair;
        typedef list<LoaderFileListPair>::type ScriptLoaderFileList;
        ScriptLoaderFileList scriptLoaderFileList;
        size_t scriptCount = 0;
        // Iterate over script users in loading order and get streams
        ScriptLoaderOrderMap::iterator oi;
        for( oi = mScriptLoaderOrderMap.begin(); oi != mScriptLoaderOrderMap.end(); ++oi )
        {
            ScriptLoader *su = oi->second;
            // MEMCATEGORY_GENERAL is the only category supported for SharedPtr
            FileListListPtr fileListList( OGRE_NEW_T( FileListList, MEMCATEGORY_GENERAL )(),
                                          SPFM_DELETE_T );

            // Get all the patterns and search them
            const StringVector &patterns = su->getScriptPatterns();
            for( StringVector::const_iterator p = patterns.begin(); p != patterns.end(); ++p )
            {
                FileInfoListPtr fileList = findResourceFileInfo( grp->name, *p );
                scriptCount += fileList->size();
                fileListList->push_back( fileList );
            }
            scriptLoaderFileList.push_back( LoaderFileListPair( su, fileListList ) );
        }
        // Fire scripting event
        fireResourceGroupScriptingStarted( grp->name, scriptCount );

        // Iterate over scripts and parse
        // Note we respect original ordering
        for( ScriptLoaderFileList::iterator slfli = scriptLoaderFileList.begin();
             slfli != scriptLoaderFileList.end(); ++slfli )
        {
            ScriptLoader *su = slfli->first;
            // Iterate over each list
            for( FileListList::iterator flli = slfli->second->begin(); flli != slfli->second->end();
                 ++flli )
            {
                // Iterate over each item in the list
                for( FileInfoList::iterator fii = ( *flli )->begin(); fii != ( *flli )->end(); ++fii )
                {
                    bool skipScript = false;
                    fireScriptStarted( fii->filename, skipScript );
                    if( skipScript )
                    {
                        LogManager::getSingleton().logMessage( "Skipping script " + fii->filename );
                    }
                    else
                    {
                        LogManager::getSingleton().logMessage( "Parsing script " + fii->filename );
                        DataStreamPtr stream = fii->archive->open( fii->filename );
                        if( stream )
                        {
                            if( mLoadingListener )
                                mLoadingListener->resourceStreamOpened( fii->filename, grp->name, 0,
                                                                        stream );

                            if( fii->archive->getType() == "FileSystem" &&
                                stream->size() <= 1024 * 1024 )
                            {
                                DataStreamPtr cachedCopy;
                                cachedCopy.reset(
                                    OGRE_NEW MemoryDataStream( stream->getName(), stream ) );
                                su->parseScript( cachedCopy, grp->name );
                            }
                            else
                                su->parseScript( stream, grp->name );
                        }
                    }
                    fireScriptEnded( fii->filename, skipScript );
                }
            }
        }

        fireResourceGroupScriptingEnded( grp->name );
        LogManager::getSingleton().logMessage( "Finished parsing scripts for resource group " +
                                               grp->name );
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::createDeclaredResources( ResourceGroup *grp )
    {
        for( ResourceDeclarationList::iterator i = grp->resourceDeclarations.begin();
             i != grp->resourceDeclarations.end(); ++i )
        {
            ResourceDeclaration &dcl = *i;
            // Retrieve the appropriate manager
            ResourceManager *mgr = _getResourceManager( dcl.resourceType );
            // Create the resource
            ResourcePtr res = mgr->createResource( dcl.resourceName, grp->name, dcl.loader != 0,
                                                   dcl.loader, &dcl.parameters );
            // Add resource to load list
            ResourceGroup::LoadResourceOrderMap::iterator li =
                grp->loadResourceOrderMap.find( mgr->getLoadingOrder() );
            LoadUnloadResourceSet *loadList;
            if( li == grp->loadResourceOrderMap.end() )
            {
                loadList = OGRE_NEW_T( LoadUnloadResourceSet, MEMCATEGORY_RESOURCE )();
                grp->loadResourceOrderMap[mgr->getLoadingOrder()] = loadList;
            }
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::_notifyResourceCreated( ResourcePtr &res )
    {
        if( mCurrentGroup && res->getGroup() == mCurrentGroup->name )
        {
            // Use current group (batch loading)
            addCreatedResource( res, *mCurrentGroup );
        }
        else
        {
            // Find group
            ResourceGroup *grp = getResourceGroup( res->getGroup() );
            if( grp )
            {
                addCreatedResource( res, *grp );
            }
        }

        fireResourceCreated( res );
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::_notifyResourceRemoved( const ResourcePtr &res )
    {
        fireResourceRemove( res );

        if( mCurrentGroup )
        {
            // Do nothing - we're batch unloading so list will be cleared
        }
        else
        {
            // Find group
            ResourceGroup *grp = getResourceGroup( res->getGroup() );
            if( grp )
            {
                OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex
                ResourceGroup::LoadResourceOrderMap::iterator i =
                    grp->loadResourceOrderMap.find( res->getCreator()->getLoadingOrder() );
                if( i != grp->loadResourceOrderMap.end() )
                {
                    LoadUnloadResourceSet *resList = i->second;
                    LoadUnloadResourceSet::iterator l = resList->find( res );
                    if( l != resList->end() )
                        resList->erase( l );
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::_notifyResourceGroupChanged( const String &oldGroup, Resource *res )
    {
        ResourcePtr resPtr;

        // find old entry
        ResourceGroup *grp = getResourceGroup( oldGroup );

        if( grp )
        {
            OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

            Real order = res->getCreator()->getLoadingOrder();
            ResourceGroup::LoadResourceOrderMap::iterator i = grp->loadResourceOrderMap.find( order );
            assert( i != grp->loadResourceOrderMap.end() );
            LoadUnloadResourceSet *loadList = i->second;

            ResourcePtr rawWrapper( res, [](Resource*) {} );  // this will ensure it wont free on destruction.

            LoadUnloadResourceSet::iterator l = loadList->find( rawWrapper );
            if( l != loadList->end() )
            {
                resPtr = *l;
                loadList->erase( l );
            }
        }

        if( resPtr )
        {
            // New group
            ResourceGroup *newGrp = getResourceGroup( res->getGroup() );

            addCreatedResource( resPtr, *newGrp );
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::_notifyAllResourcesRemoved( ResourceManager *manager )
    {
        OGRE_LOCK_AUTO_MUTEX;

        // Iterate over all groups
        for( ResourceGroupMap::iterator grpi = mResourceGroupMap.begin();
             grpi != mResourceGroupMap.end(); ++grpi )
        {
            OGRE_LOCK_MUTEX( grpi->second->OGRE_AUTO_MUTEX_NAME );
            // Iterate over all priorities
            for( ResourceGroup::LoadResourceOrderMap::iterator oi =
                     grpi->second->loadResourceOrderMap.begin();
                 oi != grpi->second->loadResourceOrderMap.end(); ++oi )
            {
                // Iterate over all resources
                for( LoadUnloadResourceSet::iterator l = oi->second->begin(); l != oi->second->end(); )
                {
                    if( ( *l )->getCreator() == manager )
                    {
                        // Increment first since iterator will be invalidated
                        LoadUnloadResourceSet::iterator del = l++;
                        oi->second->erase( del );
                    }
                    else
                    {
                        ++l;
                    }
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::addCreatedResource( ResourcePtr &res, ResourceGroup &grp )
    {
        OGRE_LOCK_MUTEX( grp.OGRE_AUTO_MUTEX_NAME );
        Real order = res->getCreator()->getLoadingOrder();

        ResourceGroup::LoadResourceOrderMap::iterator i = grp.loadResourceOrderMap.find( order );
        LoadUnloadResourceSet *loadList;
        if( i == grp.loadResourceOrderMap.end() )
        {
            loadList = OGRE_NEW_T( LoadUnloadResourceSet, MEMCATEGORY_RESOURCE )();
            grp.loadResourceOrderMap[order] = loadList;
        }
        else
        {
            loadList = i->second;
        }
        loadList->insert( res );
    }
    //-----------------------------------------------------------------------
    ResourceGroupManager::ResourceGroup *ResourceGroupManager::getResourceGroup( const String &name )
    {
        OGRE_LOCK_AUTO_MUTEX;

        ResourceGroupMap::iterator i = mResourceGroupMap.find( name );
        if( i != mResourceGroupMap.end() )
        {
            return i->second;
        }
        return 0;
    }
    //-----------------------------------------------------------------------
    ResourceManager *ResourceGroupManager::_getResourceManager( const String &resourceType )
    {
        OGRE_LOCK_AUTO_MUTEX;

        ResourceManagerMap::iterator i = mResourceManagerMap.find( resourceType );
        if( i == mResourceManagerMap.end() )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot locate resource manager for resource type '" + resourceType + "'",
                         "ResourceGroupManager::_getResourceManager" );
        }
        return i->second;
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::dropGroupContents( ResourceGroup *grp )
    {
        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );

        bool groupSet = false;
        if( !mCurrentGroup )
        {
            // Set current group to indicate ignoring of notifications
            mCurrentGroup = grp;
            groupSet = true;
        }
        // delete all the load list entries
        ResourceGroup::LoadResourceOrderMap::iterator j, jend;
        jend = grp->loadResourceOrderMap.end();
        for( j = grp->loadResourceOrderMap.begin(); j != jend; ++j )
        {
            // Iterate over resources
            for( LoadUnloadResourceSet::iterator k = j->second->begin(); k != j->second->end(); ++k )
            {
                ( *k )->getCreator()->remove( ( *k )->getHandle() );
            }
            OGRE_DELETE_T( j->second, LoadUnloadResourceSet, MEMCATEGORY_RESOURCE );
        }
        grp->loadResourceOrderMap.clear();

        if( groupSet )
        {
            mCurrentGroup = 0;
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::deleteGroup( ResourceGroup *grp )
    {
        {
            OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );
            // delete all the load list entries
            ResourceGroup::LoadResourceOrderMap::iterator j, jend;
            jend = grp->loadResourceOrderMap.end();
            for( j = grp->loadResourceOrderMap.begin(); j != jend; ++j )
            {
                // Don't iterate over resources to drop with ResourceManager
                // Assume this is being done anyway since this is a shutdown method
                OGRE_DELETE_T( j->second, LoadUnloadResourceSet, MEMCATEGORY_RESOURCE );
            }
            // Drop location list
            for( LocationList::iterator ll = grp->locationList.begin(); ll != grp->locationList.end();
                 ++ll )
            {
                OGRE_DELETE_T( *ll, ResourceLocation, MEMCATEGORY_RESOURCE );
            }
        }

        // delete ResourceGroup
        OGRE_DELETE_T( grp, ResourceGroup, MEMCATEGORY_RESOURCE );
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceGroupScriptingStarted( const String &groupName,
                                                                  size_t scriptCount )
    {
        OGRE_LOCK_AUTO_MUTEX;
        for( ResourceGroupListenerList::iterator l = mResourceGroupListenerList.begin();
             l != mResourceGroupListenerList.end(); ++l )
        {
            ( *l )->resourceGroupScriptingStarted( groupName, scriptCount );
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireScriptStarted( const String &scriptName, bool &skipScript )
    {
        OGRE_LOCK_AUTO_MUTEX;
        for( ResourceGroupListenerList::iterator l = mResourceGroupListenerList.begin();
             l != mResourceGroupListenerList.end(); ++l )
        {
            bool temp = false;
            ( *l )->scriptParseStarted( scriptName, temp );
            if( temp )
                skipScript = true;
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireScriptEnded( const String &scriptName, bool skipped )
    {
        OGRE_LOCK_AUTO_MUTEX;
        for( ResourceGroupListenerList::iterator l = mResourceGroupListenerList.begin();
             l != mResourceGroupListenerList.end(); ++l )
        {
            ( *l )->scriptParseEnded( scriptName, skipped );
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceGroupScriptingEnded( const String &groupName )
    {
        OGRE_LOCK_AUTO_MUTEX;
        for( ResourceGroupListenerList::iterator l = mResourceGroupListenerList.begin();
             l != mResourceGroupListenerList.end(); ++l )
        {
            ( *l )->resourceGroupScriptingEnded( groupName );
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceGroupLoadStarted( const String &groupName,
                                                             size_t resourceCount )
    {
        OGRE_LOCK_AUTO_MUTEX;
        for( ResourceGroupListenerList::iterator l = mResourceGroupListenerList.begin();
             l != mResourceGroupListenerList.end(); ++l )
        {
            ( *l )->resourceGroupLoadStarted( groupName, resourceCount );
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceLoadStarted( const ResourcePtr &resource )
    {
        OGRE_LOCK_AUTO_MUTEX;
        for( ResourceGroupListenerList::iterator l = mResourceGroupListenerList.begin();
             l != mResourceGroupListenerList.end(); ++l )
        {
            ( *l )->resourceLoadStarted( resource );
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceLoadEnded()
    {
        OGRE_LOCK_AUTO_MUTEX;
        for( ResourceGroupListenerList::iterator l = mResourceGroupListenerList.begin();
             l != mResourceGroupListenerList.end(); ++l )
        {
            ( *l )->resourceLoadEnded();
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceGroupLoadEnded( const String &groupName )
    {
        OGRE_LOCK_AUTO_MUTEX;
        for( ResourceGroupListenerList::iterator l = mResourceGroupListenerList.begin();
             l != mResourceGroupListenerList.end(); ++l )
        {
            ( *l )->resourceGroupLoadEnded( groupName );
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceGroupPrepareStarted( const String &groupName,
                                                                size_t resourceCount )
    {
        OGRE_LOCK_AUTO_MUTEX;
        for( ResourceGroupListenerList::iterator l = mResourceGroupListenerList.begin();
             l != mResourceGroupListenerList.end(); ++l )
        {
            ( *l )->resourceGroupPrepareStarted( groupName, resourceCount );
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourcePrepareStarted( const ResourcePtr &resource )
    {
        OGRE_LOCK_AUTO_MUTEX;
        for( ResourceGroupListenerList::iterator l = mResourceGroupListenerList.begin();
             l != mResourceGroupListenerList.end(); ++l )
        {
            ( *l )->resourcePrepareStarted( resource );
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourcePrepareEnded()
    {
        OGRE_LOCK_AUTO_MUTEX;
        for( ResourceGroupListenerList::iterator l = mResourceGroupListenerList.begin();
             l != mResourceGroupListenerList.end(); ++l )
        {
            ( *l )->resourcePrepareEnded();
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceGroupPrepareEnded( const String &groupName )
    {
        OGRE_LOCK_AUTO_MUTEX;
        for( ResourceGroupListenerList::iterator l = mResourceGroupListenerList.begin();
             l != mResourceGroupListenerList.end(); ++l )
        {
            ( *l )->resourceGroupPrepareEnded( groupName );
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceCreated( const ResourcePtr &resource )
    {
        OGRE_LOCK_AUTO_MUTEX;
        for( ResourceGroupListenerList::iterator l = mResourceGroupListenerList.begin();
             l != mResourceGroupListenerList.end(); ++l )
        {
            ( *l )->resourceCreated( resource );
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceRemove( const ResourcePtr &resource )
    {
        OGRE_LOCK_AUTO_MUTEX;
        for( ResourceGroupListenerList::iterator l = mResourceGroupListenerList.begin();
             l != mResourceGroupListenerList.end(); ++l )
        {
            ( *l )->resourceRemove( resource );
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::shutdownAll()
    {
        OGRE_LOCK_AUTO_MUTEX;

        for( ResourceManagerMap::value_type &rme : mResourceManagerMap )
            rme.second->removeAll();
    }
    //-----------------------------------------------------------------------
    StringVectorPtr ResourceGroupManager::listResourceNames( const String &groupName, bool dirs )
    {
        OGRE_LOCK_AUTO_MUTEX;
        // MEMCATEGORY_GENERAL is the only category supported for SharedPtr
        StringVectorPtr vec( OGRE_NEW_T( StringVector, MEMCATEGORY_GENERAL )(), SPFM_DELETE_T );

        // Try to find in resource index first
        ResourceGroup *grp = getResourceGroup( groupName );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot locate a resource group called '" + groupName + "'",
                         "ResourceGroupManager::listResourceNames" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        // Iterate over the archives
        for( ResourceLocation *loc : grp->locationList )
        {
            StringVectorPtr lst = loc->archive->list( loc->recursive, dirs );
            vec->insert( vec->end(), lst->begin(), lst->end() );
        }

        return vec;
    }
    //-----------------------------------------------------------------------
    FileInfoListPtr ResourceGroupManager::listResourceFileInfo( const String &groupName, bool dirs )
    {
        OGRE_LOCK_AUTO_MUTEX;
        // MEMCATEGORY_GENERAL is the only category supported for SharedPtr
        FileInfoListPtr vec( OGRE_NEW_T( FileInfoList, MEMCATEGORY_GENERAL )(), SPFM_DELETE_T );

        // Try to find in resource index first
        ResourceGroup *grp = getResourceGroup( groupName );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot locate a resource group called '" + groupName + "'",
                         "ResourceGroupManager::listResourceFileInfo" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        // Iterate over the archives
        for( ResourceLocation *loc : grp->locationList )
        {
            FileInfoListPtr lst = loc->archive->listFileInfo( loc->recursive, dirs );
            vec->insert( vec->end(), lst->begin(), lst->end() );
        }

        return vec;
    }
    //-----------------------------------------------------------------------
    StringVectorPtr ResourceGroupManager::findResourceNames( const String &groupName,
                                                             const String &pattern, bool dirs )
    {
        OGRE_LOCK_AUTO_MUTEX;
        // MEMCATEGORY_GENERAL is the only category supported for SharedPtr
        StringVectorPtr vec( OGRE_NEW_T( StringVector, MEMCATEGORY_GENERAL )(), SPFM_DELETE_T );

        // Try to find in resource index first
        ResourceGroup *grp = getResourceGroup( groupName );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot locate a resource group called '" + groupName + "'",
                         "ResourceGroupManager::findResourceNames" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        // Iterate over the archives
        for( ResourceLocation *loc : grp->locationList )
        {
            StringVectorPtr lst = loc->archive->find( pattern, loc->recursive, dirs );
            vec->insert( vec->end(), lst->begin(), lst->end() );
        }

        return vec;
    }
    //-----------------------------------------------------------------------
    FileInfoListPtr ResourceGroupManager::findResourceFileInfo( const String &groupName,
                                                                const String &pattern, bool dirs )
    {
        OGRE_LOCK_AUTO_MUTEX;
        // MEMCATEGORY_GENERAL is the only category supported for SharedPtr
        FileInfoListPtr vec( OGRE_NEW_T( FileInfoList, MEMCATEGORY_GENERAL )(), SPFM_DELETE_T );

        // Try to find in resource index first
        ResourceGroup *grp = getResourceGroup( groupName );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot locate a resource group called '" + groupName + "'",
                         "ResourceGroupManager::findResourceFileInfo" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        // Iterate over the archives
        for( ResourceLocation *loc : grp->locationList )
        {
            FileInfoListPtr lst = loc->archive->findFileInfo( pattern, loc->recursive, dirs );
            vec->insert( vec->end(), lst->begin(), lst->end() );
        }

        return vec;
    }
    //-----------------------------------------------------------------------
    bool ResourceGroupManager::resourceExists( const String &groupName, const String &resourceName )
    {
        OGRE_LOCK_AUTO_MUTEX;

        // Try to find in resource index first
        ResourceGroup *grp = getResourceGroup( groupName );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot locate a resource group called '" + groupName + "'",
                         "ResourceGroupManager::resourceExists" );
        }

        return resourceExists( grp, resourceName );
    }
    //-----------------------------------------------------------------------
    bool ResourceGroupManager::resourceExists( ResourceGroup *grp, const String &resourceName )
    {
        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        // Try indexes first
        ResourceLocationIndex::iterator rit = grp->resourceIndexCaseSensitive.find( resourceName );
        if( rit != grp->resourceIndexCaseSensitive.end() )
        {
            // Found in the index
            return true;
        }
        else
        {
            // try case insensitive
            String lcResourceName = resourceName;
            StringUtil::toLowerCase( lcResourceName );
            rit = grp->resourceIndexCaseInsensitive.find( lcResourceName );
            if( rit != grp->resourceIndexCaseInsensitive.end() )
            {
                // Found in the index
                return true;
            }
            else
            {
                // Search the hard way
                LocationList::iterator li, liend;
                liend = grp->locationList.end();
                for( li = grp->locationList.begin(); li != liend; ++li )
                {
                    Archive *arch = ( *li )->archive;
                    if( arch->exists( resourceName ) )
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }
    //-----------------------------------------------------------------------
    time_t ResourceGroupManager::resourceModifiedTime( const String &groupName,
                                                       const String &resourceName )
    {
        OGRE_LOCK_AUTO_MUTEX;

        // Try to find in resource index first
        ResourceGroup *grp = getResourceGroup( groupName );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot locate a resource group called '" + groupName + "'",
                         "ResourceGroupManager::resourceModifiedTime" );
        }

        return resourceModifiedTime( grp, resourceName );
    }
    //-----------------------------------------------------------------------
    time_t ResourceGroupManager::resourceModifiedTime( ResourceGroup *grp, const String &resourceName )
    {
        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        // Try indexes first
        ResourceLocationIndex::iterator rit = grp->resourceIndexCaseSensitive.find( resourceName );
        if( rit != grp->resourceIndexCaseSensitive.end() )
        {
            return rit->second->getModifiedTime( resourceName );
        }
        else
        {
            // try case insensitive
            String lcResourceName = resourceName;
            StringUtil::toLowerCase( lcResourceName );
            rit = grp->resourceIndexCaseInsensitive.find( lcResourceName );
            if( rit != grp->resourceIndexCaseInsensitive.end() )
            {
                return rit->second->getModifiedTime( resourceName );
            }
            else
            {
                // Search the hard way
                LocationList::iterator li, liend;
                liend = grp->locationList.end();
                for( li = grp->locationList.begin(); li != liend; ++li )
                {
                    Archive *arch = ( *li )->archive;
                    time_t testTime = arch->getModifiedTime( resourceName );

                    if( testTime > 0 )
                    {
                        return testTime;
                    }
                }
            }
        }

        return 0;
    }
    //-----------------------------------------------------------------------
    ResourceGroupManager::ResourceGroup *ResourceGroupManager::findGroupContainingResourceImpl(
        const String &filename )
    {
        OGRE_LOCK_AUTO_MUTEX;

        // Iterate over resource groups and find
        for( ResourceGroupMap::iterator i = mResourceGroupMap.begin(); i != mResourceGroupMap.end();
             ++i )
        {
            ResourceGroup *grp = i->second;

            OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

            if( resourceExists( grp, filename ) )
                return grp;
        }
        // Not found
        return 0;
    }
    //-----------------------------------------------------------------------
    bool ResourceGroupManager::resourceExistsInAnyGroup( const String &filename )
    {
        ResourceGroup *grp = findGroupContainingResourceImpl( filename );
        if( !grp )
            return false;
        return true;
    }
    //-----------------------------------------------------------------------
    const String &ResourceGroupManager::findGroupContainingResource( const String &filename )
    {
        ResourceGroup *grp = findGroupContainingResourceImpl( filename );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Unable to derive resource group for " + filename +
                             " automatically since the resource was not "
                             "found.",
                         "ResourceGroupManager::findGroupContainingResource" );
        }
        return grp->name;
    }
    //-----------------------------------------------------------------------
    StringVectorPtr ResourceGroupManager::listResourceLocations( const String &groupName )
    {
        OGRE_LOCK_AUTO_MUTEX;
        StringVectorPtr vec( OGRE_NEW_T( StringVector, MEMCATEGORY_GENERAL )(), SPFM_DELETE_T );

        // Try to find in resource index first
        ResourceGroup *grp = getResourceGroup( groupName );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot locate a resource group called '" + groupName + "'",
                         "ResourceGroupManager::listResourceNames" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        // Iterate over the archives
        for( ResourceLocation *loc : grp->locationList )
            vec->push_back( loc->archive->getName() );

        return vec;
    }
    //-----------------------------------------------------------------------
    StringVectorPtr ResourceGroupManager::findResourceLocation( const String &groupName,
                                                                const String &pattern )
    {
        OGRE_LOCK_AUTO_MUTEX;
        StringVectorPtr vec( OGRE_NEW_T( StringVector, MEMCATEGORY_GENERAL )(), SPFM_DELETE_T );

        // Try to find in resource index first
        ResourceGroup *grp = getResourceGroup( groupName );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot locate a resource group called '" + groupName + "'",
                         "ResourceGroupManager::listResourceNames" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex

        // Iterate over the archives
        for( ResourceLocation *loc : grp->locationList )
        {
            String location = loc->archive->getName();
            // Search for the pattern
            if( StringUtil::match( location, pattern ) )
            {
                vec->push_back( location );
            }
        }

        return vec;
    }
    //-----------------------------------------------------------------------
    bool ResourceGroupManager::isResourceGroupInGlobalPool( const String &name )
    {
        OGRE_LOCK_AUTO_MUTEX;

        ResourceGroup *grp = getResourceGroup( name );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Cannot find a group named " + name,
                         "ResourceGroupManager::isResourceGroupInitialised" );
        }
        return grp->inGlobalPool;
    }
    //-----------------------------------------------------------------------
    StringVector ResourceGroupManager::getResourceGroups()
    {
        OGRE_LOCK_AUTO_MUTEX;
        StringVector vec;
        for( ResourceGroupMap::iterator i = mResourceGroupMap.begin(); i != mResourceGroupMap.end();
             ++i )
        {
            vec.push_back( i->second->name );
        }
        return vec;
    }
    //-----------------------------------------------------------------------
    ResourceGroupManager::ResourceDeclarationList ResourceGroupManager::getResourceDeclarationList(
        const String &group )
    {
        OGRE_LOCK_AUTO_MUTEX;
        ResourceGroup *grp = getResourceGroup( group );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot locate a resource group called '" + group + "'",
                         "ResourceGroupManager::getResourceDeclarationList" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex
        return grp->resourceDeclarations;
    }
    //---------------------------------------------------------------------
    const ResourceGroupManager::LocationList &ResourceGroupManager::getResourceLocationList(
        const String &group )
    {
        OGRE_LOCK_AUTO_MUTEX;
        ResourceGroup *grp = getResourceGroup( group );
        if( !grp )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot locate a resource group called '" + group + "'",
                         "ResourceGroupManager::getResourceLocationList" );
        }

        OGRE_LOCK_MUTEX( grp->OGRE_AUTO_MUTEX_NAME );  // lock group mutex
        return grp->locationList;
    }
    //-------------------------------------------------------------------------
    void ResourceGroupManager::setLoadingListener( ResourceLoadingListener *listener )
    {
        mLoadingListener = listener;
    }
    //-------------------------------------------------------------------------
    ResourceLoadingListener *ResourceGroupManager::getLoadingListener() { return mLoadingListener; }
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    void ResourceGroupManager::ResourceGroup::addToIndex( const String &filename, Archive *arch )
    {
        // internal, assumes mutex lock has already been obtained
        this->resourceIndexCaseSensitive[filename] = arch;

        if( !arch->isCaseSensitive() )
        {
            String lcase = filename;
            StringUtil::toLowerCase( lcase );
            this->resourceIndexCaseInsensitive[lcase] = arch;
        }
    }
    //---------------------------------------------------------------------
    void ResourceGroupManager::ResourceGroup::removeFromIndex( const String &filename, Archive *arch )
    {
        // internal, assumes mutex lock has already been obtained
        ResourceLocationIndex::iterator i = this->resourceIndexCaseSensitive.find( filename );
        if( i != this->resourceIndexCaseSensitive.end() && i->second == arch )
            this->resourceIndexCaseSensitive.erase( i );

        if( !arch->isCaseSensitive() )
        {
            String lcase = filename;
            StringUtil::toLowerCase( lcase );
            i = this->resourceIndexCaseInsensitive.find( lcase );
            if( i != this->resourceIndexCaseInsensitive.end() && i->second == arch )
                this->resourceIndexCaseInsensitive.erase( i );
        }
    }
    //---------------------------------------------------------------------
    void ResourceGroupManager::ResourceGroup::removeFromIndex( Archive *arch )
    {
        // Delete indexes
        ResourceLocationIndex::iterator rit, ritend;
        ritend = this->resourceIndexCaseInsensitive.end();
        for( rit = this->resourceIndexCaseInsensitive.begin(); rit != ritend; )
        {
            if( rit->second == arch )
            {
                ResourceLocationIndex::iterator del = rit++;
                this->resourceIndexCaseInsensitive.erase( del );
            }
            else
            {
                ++rit;
            }
        }
        ritend = this->resourceIndexCaseSensitive.end();
        for( rit = this->resourceIndexCaseSensitive.begin(); rit != ritend; )
        {
            if( rit->second == arch )
            {
                ResourceLocationIndex::iterator del = rit++;
                this->resourceIndexCaseSensitive.erase( del );
            }
            else
            {
                ++rit;
            }
        }
    }
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    ResourceGroupListener::~ResourceGroupListener() {}
    ResourceLoadingListener::~ResourceLoadingListener() {}
}  // namespace Ogre
