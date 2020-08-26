/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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
#include "OgreResourceManager.h"

#include "OgreException.h"
#include "OgreResourceGroupManager.h"
#include "OgreStringConverter.h"

namespace Ogre {

    //-----------------------------------------------------------------------
    ResourceManager::ResourceManager()
        : mNextHandle(1), mMemoryUsage(0), mVerbose(true), mLoadOrder(0)
    {
        // Init memory limit & usage
        mMemoryBudget = std::numeric_limits<unsigned long>::max();
    }
    //-----------------------------------------------------------------------
    ResourceManager::~ResourceManager()
    {
        destroyAllResourcePools();
        removeAll();
    }
    //-----------------------------------------------------------------------
    ResourcePtr ResourceManager::createResource(const String& name, const String& group,
        bool isManual, ManualResourceLoader* loader, const NameValuePairList* params)
    {
        // Call creation implementation
        ResourcePtr ret = ResourcePtr(
            createImpl(name, getNextHandle(), group, isManual, loader, params));
        if (params)
            ret->setParameterList(*params);

        addImpl(ret);
        // Tell resource group manager
        ResourceGroupManager::getSingleton()._notifyResourceCreated(ret);
        return ret;

    }
    //-----------------------------------------------------------------------
    ResourceManager::ResourceCreateOrRetrieveResult 
    ResourceManager::createOrRetrieve(
        const String& name, const String& group, 
        bool isManual, ManualResourceLoader* loader, 
        const NameValuePairList* params)
    {
        // Lock for the whole get / insert
            OGRE_LOCK_AUTO_MUTEX;

        ResourcePtr res = getResourceByName(name, group);
        bool created = false;
        if (res.isNull())
        {
            created = true;
            res = createResource(name, group, isManual, loader, params);
        }

        return ResourceCreateOrRetrieveResult(res, created);
    }
    //-----------------------------------------------------------------------
    ResourcePtr ResourceManager::prepare(const String& name, 
        const String& group, bool isManual, ManualResourceLoader* loader, 
        const NameValuePairList* loadParams, bool backgroundThread)
    {
        ResourcePtr r = createOrRetrieve(name,group,isManual,loader,loadParams).first;
        // ensure prepared
        r->prepare(backgroundThread);
        return r;
    }
    //-----------------------------------------------------------------------
    ResourcePtr ResourceManager::load(const String& name, 
        const String& group, bool isManual, ManualResourceLoader* loader, 
        const NameValuePairList* loadParams, bool backgroundThread)
    {
        ResourcePtr r = createOrRetrieve(name,group,isManual,loader,loadParams).first;
        // ensure loaded
        r->load(backgroundThread);

        return r;
    }
    //-----------------------------------------------------------------------
    void ResourceManager::addImpl( ResourcePtr& res )
    {
            OGRE_LOCK_AUTO_MUTEX;

            std::pair<ResourceMap::iterator, bool> result;
        if(ResourceGroupManager::getSingleton().isResourceGroupInGlobalPool(res->getGroup()))
        {
            result = mResources.insert( ResourceMap::value_type( res->getName(), res ) );
        }
        else
        {
            ResourceWithGroupMap::iterator itGroup = mResourcesWithGroup.find(res->getGroup());

            // we will create the group if it doesn't exists in our list
            if( itGroup == mResourcesWithGroup.end())
            {
                ResourceMap dummy;
                mResourcesWithGroup.insert( ResourceWithGroupMap::value_type( res->getGroup(), dummy ) );
                itGroup = mResourcesWithGroup.find(res->getGroup());
            }
            result = itGroup->second.insert( ResourceMap::value_type( res->getName(), res ) );

        }

        if (!result.second)
        {
            // Attempt to resolve the collision
            if(ResourceGroupManager::getSingleton().getLoadingListener())
            {
                if(ResourceGroupManager::getSingleton().getLoadingListener()->resourceCollision(res.get(), this))
                {
                    // Try to do the addition again, no seconds attempts to resolve collisions are allowed
                    std::pair<ResourceMap::iterator, bool> insertResult;
                    if(ResourceGroupManager::getSingleton().isResourceGroupInGlobalPool(res->getGroup()))
                    {
                        insertResult = mResources.insert( ResourceMap::value_type( res->getName(), res ) );
                    }
                    else
                    {
                        ResourceWithGroupMap::iterator itGroup = mResourcesWithGroup.find(res->getGroup());
                        insertResult = itGroup->second.insert( ResourceMap::value_type( res->getName(), res ) );
                    }
                    if (!insertResult.second)
                    {
                        OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, "Resource with the name " + res->getName() + 
                            " already exists.", "ResourceManager::add");
                    }

                    std::pair<ResourceHandleMap::iterator, bool> resultHandle = 
                        mResourcesByHandle.insert( ResourceHandleMap::value_type( res->getHandle(), res ) );
                    if (!resultHandle.second)
                    {
                        OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, "Resource with the handle " + 
                            StringConverter::toString((long) (res->getHandle())) + 
                            " already exists.", "ResourceManager::add");
                    }
                }
            }
        }
        else
        {
            // Insert the handle
            std::pair<ResourceHandleMap::iterator, bool> resultHandle = 
                mResourcesByHandle.insert( ResourceHandleMap::value_type( res->getHandle(), res ) );
            if (!resultHandle.second)
            {
                OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, "Resource with the handle " + 
                    StringConverter::toString((long) (res->getHandle())) + 
                    " already exists.", "ResourceManager::add");
            }
        }
    }
    //-----------------------------------------------------------------------
    void ResourceManager::removeImpl(const ResourcePtr& res )
    {
            OGRE_LOCK_AUTO_MUTEX;

        if(ResourceGroupManager::getSingleton().isResourceGroupInGlobalPool(res->getGroup()))
        {
            ResourceMap::iterator nameIt = mResources.find(res->getName());
            if (nameIt != mResources.end())
            {
                mResources.erase(nameIt);
            }
        }
        else
        {
            ResourceWithGroupMap::iterator groupIt = mResourcesWithGroup.find(res->getGroup());
            if (groupIt != mResourcesWithGroup.end())
            {
                ResourceMap::iterator nameIt = groupIt->second.find(res->getName());
                if (nameIt != groupIt->second.end())
                {
                    groupIt->second.erase(nameIt);
                }

                if (groupIt->second.empty())
                {
                    mResourcesWithGroup.erase(groupIt);
                }
            }
        }

        ResourceHandleMap::iterator handleIt = mResourcesByHandle.find(res->getHandle());
        if (handleIt != mResourcesByHandle.end())
        {
            mResourcesByHandle.erase(handleIt);
        }
        // Tell resource group manager
        ResourceGroupManager::getSingleton()._notifyResourceRemoved(res);
    }
    //-----------------------------------------------------------------------
    void ResourceManager::setMemoryBudget( size_t bytes)
    {
        // Update limit & check usage
        mMemoryBudget = bytes;
        checkUsage();
    }
    //-----------------------------------------------------------------------
    size_t ResourceManager::getMemoryBudget(void) const
    {
        return mMemoryBudget;
    }
    //-----------------------------------------------------------------------
    void ResourceManager::unload(const String& name)
    {
        ResourcePtr res = getResourceByName(name);

        if (!res.isNull())
        {
            // Unload resource
            res->unload();

        }
    }
    //-----------------------------------------------------------------------
    void ResourceManager::unload(ResourceHandle handle)
    {
        ResourcePtr res = getByHandle(handle);

        if (!res.isNull())
        {
            // Unload resource
            res->unload();

        }
    }
    //-----------------------------------------------------------------------
    void ResourceManager::unloadAll(Resource::LoadingFlags flags)
    {
        OGRE_LOCK_AUTO_MUTEX;

        bool reloadableOnly = (flags & Resource::LF_INCLUDE_NON_RELOADABLE) == 0;
        bool unreferencedOnly = (flags & Resource::LF_ONLY_UNREFERENCED) != 0;

        ResourceMap::iterator i, iend;
        iend = mResources.end();
        for (i = mResources.begin(); i != iend; ++i)
        {
            // A use count of 3 means that only RGM and RM have references
            // RGM has one (this one) and RM has 2 (by name and by handle)
            if (!unreferencedOnly || i->second.useCount() == ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS)
            {
                Resource* res = i->second.get();
                if (!reloadableOnly || res->isReloadable())
                {
                    bool markForReload = (flags & Resource::LF_MARKED_FOR_RELOAD) != 0 && res->isLoaded();
                    res->unload();
                    if (markForReload)
                        res->markForReload();
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void ResourceManager::reloadAll(Resource::LoadingFlags flags)
    {
        OGRE_LOCK_AUTO_MUTEX;

        bool reloadableOnly = (flags & Resource::LF_INCLUDE_NON_RELOADABLE) == 0;
        bool unreferencedOnly = (flags & Resource::LF_ONLY_UNREFERENCED) != 0;

        ResourceMap::iterator i, iend;
        iend = mResources.end();
        for (i = mResources.begin(); i != iend; ++i)
        {
            // A use count of 3 means that only RGM and RM have references
            // RGM has one (this one) and RM has 2 (by name and by handle)
            if (!unreferencedOnly || i->second.useCount() == ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS)
            {
                Resource* res = i->second.get();
                if (!reloadableOnly || res->isReloadable())
                {
                    res->reload(flags);
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void ResourceManager::remove(const ResourcePtr& res)
    {
        removeImpl(res);
    }
    //-----------------------------------------------------------------------
    void ResourceManager::remove(const String& name)
    {
        ResourcePtr res = getResourceByName(name);

        if (!res.isNull())
        {
            removeImpl(res);
        }
    }
    //-----------------------------------------------------------------------
    void ResourceManager::remove(ResourceHandle handle)
    {
        ResourcePtr res = getByHandle(handle);

        if (!res.isNull())
        {
            removeImpl(res);
        }
    }
    //-----------------------------------------------------------------------
    void ResourceManager::removeAll(void)
    {
            OGRE_LOCK_AUTO_MUTEX;

        mResources.clear();
        mResourcesWithGroup.clear();
        mResourcesByHandle.clear();
        // Notify resource group manager
        ResourceGroupManager::getSingleton()._notifyAllResourcesRemoved(this);
    }
    //-----------------------------------------------------------------------
    void ResourceManager::removeUnreferencedResources(bool reloadableOnly)
    {
        OGRE_LOCK_AUTO_MUTEX;

        ResourceMap::iterator i, iend;
        iend = mResources.end();
        for (i = mResources.begin(); i != iend; )
        {
            // A use count of 3 means that only RGM and RM have references
            // RGM has one (this one) and RM has 2 (by name and by handle)
            if (i->second.useCount() == ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS)
            {
                Resource* res = (i++)->second.get();
                if (!reloadableOnly || res->isReloadable())
                {
                    remove( res->getHandle() );
                }
            }
            else
            {
                ++i;
            }
        }
    }
    //-----------------------------------------------------------------------
    ResourcePtr ResourceManager::getResourceByName(const String& name, const String& groupName /* = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME */)
    {       
        ResourcePtr res;

        // if not in the global pool - get it from the grouped pool 
        if(!ResourceGroupManager::getSingleton().isResourceGroupInGlobalPool(groupName))
        {
                    OGRE_LOCK_AUTO_MUTEX;
            ResourceWithGroupMap::iterator itGroup = mResourcesWithGroup.find(groupName);

            if( itGroup != mResourcesWithGroup.end())
            {
                ResourceMap::iterator it = itGroup->second.find(name);

                if( it != itGroup->second.end())
                {
                    res = it->second;
                }
            }
        }

        // if didn't find it the grouped pool - get it from the global pool 
        if (res.isNull())
        {
                    OGRE_LOCK_AUTO_MUTEX;

            ResourceMap::iterator it = mResources.find(name);

            if( it != mResources.end())
            {
                res = it->second;
            }
            else
            {
                // this is the case when we need to search also in the grouped hash
                if (groupName == ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME)
                {
                    ResourceWithGroupMap::iterator iter = mResourcesWithGroup.begin();
                    ResourceWithGroupMap::iterator iterE = mResourcesWithGroup.end();
                    for ( ; iter != iterE ; ++iter )
                    {
                        ResourceMap::iterator resMapIt = iter->second.find(name);

                        if( resMapIt != iter->second.end())
                        {
                            res = resMapIt->second;
                            break;
                        }
                    }
                }
            }
        }
    
        return res;
    }
    //-----------------------------------------------------------------------
    ResourcePtr ResourceManager::getByHandle(ResourceHandle handle)
    {
        OGRE_LOCK_AUTO_MUTEX;

        ResourceHandleMap::iterator it = mResourcesByHandle.find(handle);
        if (it == mResourcesByHandle.end())
        {
            return ResourcePtr();
        }
        else
        {
            return it->second;
        }
    }
    //-----------------------------------------------------------------------
    ResourceHandle ResourceManager::getNextHandle(void)
    {
        // This is an atomic operation and hence needs no locking
        return mNextHandle++;
    }
    //-----------------------------------------------------------------------
    void ResourceManager::checkUsage(void)
    {
        if (getMemoryUsage() > mMemoryBudget)
        {
            OGRE_LOCK_AUTO_MUTEX;
            // unload unreferenced resources until we are within our budget again
            ResourceMap::iterator i, iend;
            iend = mResources.end();
            for (i = mResources.begin(); i != iend && getMemoryUsage() > mMemoryBudget; ++i)
            {
                // A use count of 3 means that only RGM and RM have references
                // RGM has one (this one) and RM has 2 (by name and by handle)
                if (i->second.useCount() == ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS)
                {
                    Resource* res = i->second.get();
                    if (res->isReloadable())
                    {
                        res->unload();
                    }
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void ResourceManager::_notifyResourceTouched(Resource* res)
    {
        // TODO
    }
    //-----------------------------------------------------------------------
    void ResourceManager::_notifyResourceLoaded(Resource* res)
    {
        mMemoryUsage += res->getSize();
        checkUsage();
    }
    //-----------------------------------------------------------------------
    void ResourceManager::_notifyResourceUnloaded(Resource* res)
    {
        mMemoryUsage -= res->getSize();
    }
    //---------------------------------------------------------------------
    ResourceManager::ResourcePool* ResourceManager::getResourcePool(const String& name)
    {
        OGRE_LOCK_AUTO_MUTEX;

        ResourcePoolMap::iterator i = mResourcePoolMap.find(name);
        if (i == mResourcePoolMap.end())
        {
            i = mResourcePoolMap.insert(ResourcePoolMap::value_type(name, 
                OGRE_NEW ResourcePool(name))).first;
        }
        return i->second;

    }
    //---------------------------------------------------------------------
    void ResourceManager::destroyResourcePool(ResourcePool* pool)
    {
        if(!pool)
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot destroy a null ResourcePool.", "ResourceManager::destroyResourcePool");

        OGRE_LOCK_AUTO_MUTEX;

        ResourcePoolMap::iterator i = mResourcePoolMap.find(pool->getName());
        if (i != mResourcePoolMap.end())
            mResourcePoolMap.erase(i);

        OGRE_DELETE pool;
        
    }
    //---------------------------------------------------------------------
    void ResourceManager::destroyResourcePool(const String& name)
    {
        OGRE_LOCK_AUTO_MUTEX;

        ResourcePoolMap::iterator i = mResourcePoolMap.find(name);
        if (i != mResourcePoolMap.end())
        {
            OGRE_DELETE i->second;
            mResourcePoolMap.erase(i);
        }

    }
    //---------------------------------------------------------------------
    void ResourceManager::destroyAllResourcePools()
    {
        OGRE_LOCK_AUTO_MUTEX;

        for (ResourcePoolMap::iterator i = mResourcePoolMap.begin(); 
            i != mResourcePoolMap.end(); ++i)
            OGRE_DELETE i->second;

        mResourcePoolMap.clear();
    }
    //-----------------------------------------------------------------------
    //---------------------------------------------------------------------
    ResourceManager::ResourcePool::ResourcePool(const String& name)
        : mName(name)
    {

    }
    //---------------------------------------------------------------------
    ResourceManager::ResourcePool::~ResourcePool()
    {
        clear();
    }
    //---------------------------------------------------------------------
    const String& ResourceManager::ResourcePool::getName() const
    {
        return mName;
    }
    //---------------------------------------------------------------------
    void ResourceManager::ResourcePool::clear()
    {
            OGRE_LOCK_AUTO_MUTEX;
        for (ItemList::iterator i = mItems.begin(); i != mItems.end(); ++i)
        {
            (*i)->getCreator()->remove((*i)->getHandle());
        }
        mItems.clear();
    }
}




