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

#include "OgreResourceBackgroundQueue.h"

#include "OgreException.h"
#include "OgreResourceManager.h"
#include "OgreRoot.h"

namespace Ogre
{
    // Note, no locks are required here anymore because all of the parallelisation
    // is now contained in WorkQueue - this class is entirely single-threaded
    //------------------------------------------------------------------------
    //-----------------------------------------------------------------------
    template <>
    ResourceBackgroundQueue *Singleton<ResourceBackgroundQueue>::msSingleton = 0;
    ResourceBackgroundQueue *ResourceBackgroundQueue::getSingletonPtr() { return msSingleton; }
    ResourceBackgroundQueue &ResourceBackgroundQueue::getSingleton()
    {
        assert( msSingleton );
        return ( *msSingleton );
    }
    //-----------------------------------------------------------------------
    //------------------------------------------------------------------------
    ResourceBackgroundQueue::ResourceBackgroundQueue() : mWorkQueueChannel( 0 ) {}
    //------------------------------------------------------------------------
    ResourceBackgroundQueue::~ResourceBackgroundQueue() { shutdown(); }
    //---------------------------------------------------------------------
    void ResourceBackgroundQueue::initialise()
    {
        WorkQueue *wq = Root::getSingleton().getWorkQueue();
        mWorkQueueChannel = wq->getChannel( "Ogre/ResourceBGQ" );
        wq->addResponseHandler( mWorkQueueChannel, this );
        wq->addRequestHandler( mWorkQueueChannel, this );
    }
    //---------------------------------------------------------------------
    void ResourceBackgroundQueue::shutdown()
    {
        WorkQueue *wq = Root::getSingleton().getWorkQueue();
        wq->abortRequestsByChannel( mWorkQueueChannel );
        wq->removeRequestHandler( mWorkQueueChannel, this );
        wq->removeResponseHandler( mWorkQueueChannel, this );
    }
    //------------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::initialiseResourceGroup(
        const String &name, ResourceBackgroundQueue::Listener *listener )
    {
#if OGRE_THREAD_SUPPORT
        // queue a request
        ResourceRequest req;
        req.type = RT_INITIALISE_GROUP;
        req.groupName = name;
        req.listener = listener;
        return addRequest( req );
#else
        // synchronous
        ResourceGroupManager::getSingleton().initialiseResourceGroup( name, false );
        return 0;
#endif
    }
    //------------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::initialiseAllResourceGroups(
        ResourceBackgroundQueue::Listener *listener )
    {
#if OGRE_THREAD_SUPPORT
        // queue a request
        ResourceRequest req;
        req.type = RT_INITIALISE_ALL_GROUPS;
        req.listener = listener;
        return addRequest( req );
#else
        // synchronous
        ResourceGroupManager::getSingleton().initialiseAllResourceGroups( false );
        return 0;
#endif
    }
    //------------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::prepareResourceGroup(
        const String &name, ResourceBackgroundQueue::Listener *listener )
    {
#if OGRE_THREAD_SUPPORT
        // queue a request
        ResourceRequest req;
        req.type = RT_PREPARE_GROUP;
        req.groupName = name;
        req.listener = listener;
        return addRequest( req );
#else
        // synchronous
        ResourceGroupManager::getSingleton().prepareResourceGroup( name );
        return 0;
#endif
    }
    //------------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::loadResourceGroup(
        const String &name, ResourceBackgroundQueue::Listener *listener )
    {
#if OGRE_THREAD_SUPPORT
        // queue a request
        ResourceRequest req;
        req.type = RT_LOAD_GROUP;
        req.groupName = name;
        req.listener = listener;
        return addRequest( req );
#else
        // synchronous
        ResourceGroupManager::getSingleton().loadResourceGroup( name );
        return 0;
#endif
    }
    //------------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::prepare(
        const String &resType, const String &name, const String &group, bool isManual,
        ManualResourceLoader *loader, const NameValuePairList *loadParams,
        ResourceBackgroundQueue::Listener *listener )
    {
#if OGRE_THREAD_SUPPORT
        // queue a request
        ResourceRequest req;
        req.type = RT_PREPARE_RESOURCE;
        req.resourceType = resType;
        req.resourceName = name;
        req.groupName = group;
        req.isManual = isManual;
        req.loader = loader;
        // Make instance copy of loadParams for thread independence
        req.loadParams =
            ( loadParams ? OGRE_NEW_T( NameValuePairList, MEMCATEGORY_GENERAL )( *loadParams ) : 0 );
        req.listener = listener;
        return addRequest( req );
#else
        // synchronous
        ResourceManager *rm = ResourceGroupManager::getSingleton()._getResourceManager( resType );
        rm->prepare( name, group, isManual, loader, loadParams );
        return 0;
#endif
    }
    //------------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::load( const String &resType, const String &name,
                                                           const String &group, bool isManual,
                                                           ManualResourceLoader *loader,
                                                           const NameValuePairList *loadParams,
                                                           ResourceBackgroundQueue::Listener *listener )
    {
#if OGRE_THREAD_SUPPORT
        // queue a request
        ResourceRequest req;
        req.type = RT_LOAD_RESOURCE;
        req.resourceType = resType;
        req.resourceName = name;
        req.groupName = group;
        req.isManual = isManual;
        req.loader = loader;
        // Make instance copy of loadParams for thread independence
        req.loadParams =
            ( loadParams ? OGRE_NEW_T( NameValuePairList, MEMCATEGORY_GENERAL )( *loadParams ) : 0 );
        req.listener = listener;
        return addRequest( req );
#else
        // synchronous
        ResourceManager *rm = ResourceGroupManager::getSingleton()._getResourceManager( resType );
        rm->load( name, group, isManual, loader, loadParams );
        return 0;
#endif
    }
    //---------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::unload( const String &resType, const String &name,
                                                             Listener *listener )
    {
#if OGRE_THREAD_SUPPORT
        // queue a request
        ResourceRequest req;
        req.type = RT_UNLOAD_RESOURCE;
        req.resourceType = resType;
        req.resourceName = name;
        req.listener = listener;
        return addRequest( req );
#else
        // synchronous
        ResourceManager *rm = ResourceGroupManager::getSingleton()._getResourceManager( resType );
        rm->unload( name );
        return 0;
#endif
    }
    //---------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::unload( const String &resType,
                                                             ResourceHandle handle, Listener *listener )
    {
#if OGRE_THREAD_SUPPORT
        // queue a request
        ResourceRequest req;
        req.type = RT_UNLOAD_RESOURCE;
        req.resourceType = resType;
        req.resourceHandle = handle;
        req.listener = listener;
        return addRequest( req );
#else
        // synchronous
        ResourceManager *rm = ResourceGroupManager::getSingleton()._getResourceManager( resType );
        rm->unload( handle );
        return 0;
#endif
    }
    //---------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::unloadResourceGroup( const String &name,
                                                                          Listener *listener )
    {
#if OGRE_THREAD_SUPPORT
        // queue a request
        ResourceRequest req;
        req.type = RT_UNLOAD_GROUP;
        req.groupName = name;
        req.listener = listener;
        return addRequest( req );
#else
        // synchronous
        ResourceGroupManager::getSingleton().unloadResourceGroup( name );
        return 0;
#endif
    }
    //------------------------------------------------------------------------
    bool ResourceBackgroundQueue::isProcessComplete( BackgroundProcessTicket ticket )
    {
        return mOutstandingRequestSet.find( ticket ) == mOutstandingRequestSet.end();
    }
    //------------------------------------------------------------------------
    void ResourceBackgroundQueue::abortRequest( BackgroundProcessTicket ticket )
    {
        WorkQueue *queue = Root::getSingleton().getWorkQueue();

        queue->abortRequest( ticket );
    }
    //------------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::addRequest( ResourceRequest &req )
    {
        WorkQueue *queue = Root::getSingleton().getWorkQueue();

        Any data( req );

        WorkQueue::RequestID requestID = queue->addRequest( mWorkQueueChannel, (uint16)req.type, data );

        mOutstandingRequestSet.insert( requestID );

        return requestID;
    }
    //-----------------------------------------------------------------------
    bool ResourceBackgroundQueue::canHandleRequest( const WorkQueue::Request *req,
                                                    const WorkQueue *srcQ )
    {
        return true;
    }
    //-----------------------------------------------------------------------
    WorkQueue::Response *ResourceBackgroundQueue::handleRequest( const WorkQueue::Request *req,
                                                                 const WorkQueue *srcQ )
    {
        ResourceRequest resreq = any_cast<ResourceRequest>( req->getData() );

        if( req->getAborted() )
        {
            if( resreq.type == RT_PREPARE_RESOURCE || resreq.type == RT_LOAD_RESOURCE )
            {
                OGRE_DELETE_T( resreq.loadParams, NameValuePairList, MEMCATEGORY_GENERAL );
                resreq.loadParams = 0;
            }
            resreq.result.error = false;
            ResourceResponse resresp( ResourcePtr(), resreq );
            return OGRE_NEW WorkQueue::Response( req, true, Any( resresp ) );
        }

        ResourceManager *rm = 0;
        ResourcePtr resource;
        try
        {
            switch( resreq.type )
            {
            case RT_INITIALISE_GROUP:
                ResourceGroupManager::getSingleton().initialiseResourceGroup( resreq.groupName, false );
                break;
            case RT_INITIALISE_ALL_GROUPS:
                ResourceGroupManager::getSingleton().initialiseAllResourceGroups( false );
                break;
            case RT_PREPARE_GROUP:
                ResourceGroupManager::getSingleton().prepareResourceGroup( resreq.groupName );
                break;
            case RT_LOAD_GROUP:
#if OGRE_THREAD_SUPPORT == 2
                ResourceGroupManager::getSingleton().prepareResourceGroup( resreq.groupName );
#else
                ResourceGroupManager::getSingleton().loadResourceGroup( resreq.groupName );
#endif
                break;
            case RT_UNLOAD_GROUP:
                ResourceGroupManager::getSingleton().unloadResourceGroup( resreq.groupName );
                break;
            case RT_PREPARE_RESOURCE:
                rm = ResourceGroupManager::getSingleton()._getResourceManager( resreq.resourceType );
                resource = rm->prepare( resreq.resourceName, resreq.groupName, resreq.isManual,
                                        resreq.loader, resreq.loadParams, true );
                break;
            case RT_LOAD_RESOURCE:
                rm = ResourceGroupManager::getSingleton()._getResourceManager( resreq.resourceType );
#if OGRE_THREAD_SUPPORT == 2
                resource = rm->prepare( resreq.resourceName, resreq.groupName, resreq.isManual,
                                        resreq.loader, resreq.loadParams, true );
#else
                resource = rm->load( resreq.resourceName, resreq.groupName, resreq.isManual,
                                     resreq.loader, resreq.loadParams, true );
#endif
                break;
            case RT_UNLOAD_RESOURCE:
                rm = ResourceGroupManager::getSingleton()._getResourceManager( resreq.resourceType );
                if( resreq.resourceName.empty() )
                    rm->unload( resreq.resourceHandle );
                else
                    rm->unload( resreq.resourceName );
                break;
            };
        }
        catch( Exception &e )
        {
            if( resreq.type == RT_PREPARE_RESOURCE || resreq.type == RT_LOAD_RESOURCE )
            {
                OGRE_DELETE_T( resreq.loadParams, NameValuePairList, MEMCATEGORY_GENERAL );
                resreq.loadParams = 0;
            }
            resreq.result.error = true;
            resreq.result.message = e.getFullDescription();

            // return error response
            ResourceResponse resresp( resource, resreq );
            return OGRE_NEW WorkQueue::Response( req, false, Any( resresp ), e.getFullDescription() );
        }

        // success
        if( resreq.type == RT_PREPARE_RESOURCE || resreq.type == RT_LOAD_RESOURCE )
        {
            OGRE_DELETE_T( resreq.loadParams, NameValuePairList, MEMCATEGORY_GENERAL );
            resreq.loadParams = 0;
        }
        resreq.result.error = false;
        ResourceResponse resresp( resource, resreq );
        return OGRE_NEW WorkQueue::Response( req, true, Any( resresp ) );
    }
    //------------------------------------------------------------------------
    bool ResourceBackgroundQueue::canHandleResponse( const WorkQueue::Response *res,
                                                     const WorkQueue *srcQ )
    {
        return true;
    }
    //------------------------------------------------------------------------
    void ResourceBackgroundQueue::handleResponse( const WorkQueue::Response *res, const WorkQueue *srcQ )
    {
        if( res->getRequest()->getAborted() )
        {
            mOutstandingRequestSet.erase( res->getRequest()->getID() );
            return;
        }

        ResourceResponse resresp = any_cast<ResourceResponse>( res->getData() );

        // Complete full loading in main thread if semithreading
        const ResourceRequest &req = resresp.request;

        if( res->succeeded() )
        {
#if OGRE_THREAD_SUPPORT == 2
            // These load commands would have been downgraded to prepare() for the background
            if( req.type == RT_LOAD_RESOURCE )
            {
                ResourceManager *rm =
                    ResourceGroupManager::getSingleton()._getResourceManager( req.resourceType );
                rm->load( req.resourceName, req.groupName, req.isManual, req.loader, req.loadParams,
                          true );
            }
            else if( req.type == RT_LOAD_GROUP )
            {
                ResourceGroupManager::getSingleton().loadResourceGroup( req.groupName );
            }
#endif
            mOutstandingRequestSet.erase( res->getRequest()->getID() );

            // Call resource listener
            if( resresp.resource )
            {
                if( req.type == RT_LOAD_RESOURCE )
                {
                    resresp.resource->_fireLoadingComplete( true );
                }
                else
                {
                    resresp.resource->_firePreparingComplete( true );
                }
            }
        }
        // Call queue listener
        if( req.listener )
            req.listener->operationCompleted( res->getRequest()->getID(), req.result );
    }
    //------------------------------------------------------------------------

}  // namespace Ogre
