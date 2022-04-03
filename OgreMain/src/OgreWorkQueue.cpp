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

#include "OgreWorkQueue.h"

#include "OgreLogManager.h"
#include "OgreRoot.h"
#include "OgreTimer.h"

#include <sstream>

namespace Ogre
{
    //---------------------------------------------------------------------
    uint16 WorkQueue::getChannel( const String &channelName )
    {
        OGRE_LOCK_MUTEX( mChannelMapMutex );

        ChannelMap::iterator i = mChannelMap.find( channelName );
        if( i == mChannelMap.end() )
        {
            i = mChannelMap.insert( ChannelMap::value_type( channelName, mNextChannel++ ) ).first;
        }
        return i->second;
    }
    //---------------------------------------------------------------------
    WorkQueue::Request::Request( uint16 channel, uint16 rtype, const Any &rData, uint8 retry,
                                 RequestID rid ) :
        mChannel( channel ),
        mType( rtype ),
        mData( rData ),
        mRetryCount( retry ),
        mID( rid ),
        mAborted( false )
    {
    }
    //---------------------------------------------------------------------
    WorkQueue::Request::~Request() {}
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    WorkQueue::Response::Response( const Request *rq, bool success, const Any &data,
                                   const String &msg ) :
        mRequest( rq ),
        mSuccess( success ),
        mMessages( msg ),
        mData( data )
    {
    }
    //---------------------------------------------------------------------
    WorkQueue::Response::~Response() { OGRE_DELETE mRequest; }
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    DefaultWorkQueueBase::DefaultWorkQueueBase( const String &name ) :
        mName( name ),
        mWorkerThreadCount( 1 ),
        mWorkerRenderSystemAccess( false ),
        mIsRunning( false ),
        mResposeTimeLimitMS( 8 ),
        mWorkerFunc( 0 ),
        mRequestCount( 0 ),
        mPaused( false ),
        mAcceptRequests( true ),
        mShuttingDown( false ),
        mIdleThreadRunning( false ),
        mIdleProcessed( 0 )
    {
    }
    //---------------------------------------------------------------------
    const String &DefaultWorkQueueBase::getName() const { return mName; }
    //---------------------------------------------------------------------
    size_t DefaultWorkQueueBase::getWorkerThreadCount() const { return mWorkerThreadCount; }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::setWorkerThreadCount( size_t c ) { mWorkerThreadCount = c; }
    //---------------------------------------------------------------------
    bool DefaultWorkQueueBase::getWorkersCanAccessRenderSystem() const
    {
        return mWorkerRenderSystemAccess;
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::setWorkersCanAccessRenderSystem( bool access )
    {
        mWorkerRenderSystemAccess = access;
    }
    //---------------------------------------------------------------------
    DefaultWorkQueueBase::~DefaultWorkQueueBase()
    {
        // shutdown(); // can't call here; abstract function

        for( RequestQueue::iterator i = mRequestQueue.begin(); i != mRequestQueue.end(); ++i )
        {
            OGRE_DELETE( *i );
        }
        mRequestQueue.clear();

        for( ResponseQueue::iterator i = mResponseQueue.begin(); i != mResponseQueue.end(); ++i )
        {
            OGRE_DELETE( *i );
        }
        mResponseQueue.clear();
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::addRequestHandler( uint16 channel, RequestHandler *rh )
    {
        OGRE_LOCK_RW_MUTEX_WRITE( mRequestHandlerMutex );

        RequestHandlerListByChannel::iterator i = mRequestHandlers.find( channel );
        if( i == mRequestHandlers.end() )
            i = mRequestHandlers
                    .insert( RequestHandlerListByChannel::value_type( channel, RequestHandlerList() ) )
                    .first;

        RequestHandlerList &handlers = i->second;
        bool duplicate = false;
        for( RequestHandlerList::iterator j = handlers.begin(); j != handlers.end(); ++j )
        {
            if( ( *j )->getHandler() == rh )
            {
                duplicate = true;
                break;
            }
        }
        if( !duplicate )
            handlers.push_back( RequestHandlerHolderPtr( OGRE_NEW RequestHandlerHolder( rh ) ) );
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::removeRequestHandler( uint16 channel, RequestHandler *rh )
    {
        OGRE_LOCK_RW_MUTEX_WRITE( mRequestHandlerMutex );

        RequestHandlerListByChannel::iterator i = mRequestHandlers.find( channel );
        if( i != mRequestHandlers.end() )
        {
            RequestHandlerList &handlers = i->second;
            for( RequestHandlerList::iterator j = handlers.begin(); j != handlers.end(); ++j )
            {
                if( ( *j )->getHandler() == rh )
                {
                    // Disconnect - this will make it safe across copies of the list
                    // this is threadsafe and will wait for existing processes to finish
                    ( *j )->disconnectHandler();
                    handlers.erase( j );
                    break;
                }
            }
        }
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::addResponseHandler( uint16 channel, ResponseHandler *rh )
    {
        ResponseHandlerListByChannel::iterator i = mResponseHandlers.find( channel );
        if( i == mResponseHandlers.end() )
            i = mResponseHandlers
                    .insert( ResponseHandlerListByChannel::value_type( channel, ResponseHandlerList() ) )
                    .first;

        ResponseHandlerList &handlers = i->second;
        if( std::find( handlers.begin(), handlers.end(), rh ) == handlers.end() )
            handlers.push_back( rh );
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::removeResponseHandler( uint16 channel, ResponseHandler *rh )
    {
        ResponseHandlerListByChannel::iterator i = mResponseHandlers.find( channel );
        if( i != mResponseHandlers.end() )
        {
            ResponseHandlerList &handlers = i->second;
            ResponseHandlerList::iterator j = std::find( handlers.begin(), handlers.end(), rh );
            if( j != handlers.end() )
                handlers.erase( j );
        }
    }
    //---------------------------------------------------------------------
    WorkQueue::RequestID DefaultWorkQueueBase::addRequest( uint16 channel, uint16 requestType,
                                                           const Any &rData, uint8 retryCount,
                                                           bool forceSynchronous, bool idleThread )
    {
        Request *req = 0;
        RequestID rid = 0;

        {
            // lock to acquire rid and push request to the queue
            OGRE_LOCK_MUTEX( mRequestMutex );

            if( !mAcceptRequests || mShuttingDown )
                return 0;

            rid = ++mRequestCount;
            req = OGRE_NEW Request( channel, requestType, rData, retryCount, rid );

            LogManager::getSingleton().stream( LML_TRIVIAL )
                << "DefaultWorkQueueBase('" << mName << "') - QUEUED(thread:" <<
#if OGRE_THREAD_SUPPORT
                OGRE_THREAD_CURRENT_ID
#else
                "main"
#endif
                << "): ID=" << rid << " channel=" << channel << " requestType=" << requestType;
#if OGRE_THREAD_SUPPORT
            if( !forceSynchronous && !idleThread )
            {
                mRequestQueue.push_back( req );
                notifyWorkers();
                return rid;
            }
#endif
        }
        if( OGRE_THREAD_SUPPORT && idleThread )
        {
            OGRE_LOCK_MUTEX( mIdleMutex );
            mIdleRequestQueue.push_back( req );
            if( !mIdleThreadRunning )
            {
                notifyWorkers();
            }
        }
        else
        {  // forceSynchronous
            processRequestResponse( req, true );
        }
        return rid;
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::addRequestWithRID( WorkQueue::RequestID rid, uint16 channel,
                                                  uint16 requestType, const Any &rData,
                                                  uint8 retryCount )
    {
        // lock to push request to the queue
        OGRE_LOCK_MUTEX( mRequestMutex );

        if( mShuttingDown )
            return;

        Request *req = OGRE_NEW Request( channel, requestType, rData, retryCount, rid );

        LogManager::getSingleton().stream( LML_TRIVIAL )
            << "DefaultWorkQueueBase('" << mName << "') - REQUEUED(thread:" <<
#if OGRE_THREAD_SUPPORT
            OGRE_THREAD_CURRENT_ID
#else
            "main"
#endif
            << "): ID=" << rid << " channel=" << channel << " requestType=" << requestType;
#if OGRE_THREAD_SUPPORT
        mRequestQueue.push_back( req );
        notifyWorkers();
#else
        processRequestResponse( req, true );
#endif
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::abortRequest( RequestID id )
    {
        OGRE_LOCK_MUTEX( mProcessMutex );

        // NOTE: Pending requests are exist any of RequestQueue, ProcessQueue and
        // ResponseQueue when keeping ProcessMutex, so we check all of these queues.

        for( RequestQueue::iterator i = mProcessQueue.begin(); i != mProcessQueue.end(); ++i )
        {
            if( ( *i )->getID() == id )
            {
                ( *i )->abortRequest();
                break;
            }
        }

        {
            OGRE_LOCK_MUTEX( mRequestMutex );

            for( RequestQueue::iterator i = mRequestQueue.begin(); i != mRequestQueue.end(); ++i )
            {
                if( ( *i )->getID() == id )
                {
                    ( *i )->abortRequest();
                    break;
                }
            }
        }

        {
            if( mIdleProcessed )
            {
                mIdleProcessed->abortRequest();
            }

            OGRE_LOCK_MUTEX( mIdleMutex );
            for( RequestQueue::iterator i = mIdleRequestQueue.begin(); i != mIdleRequestQueue.end();
                 ++i )
            {
                ( *i )->abortRequest();
            }
        }

        {
            OGRE_LOCK_MUTEX( mResponseMutex );

            for( ResponseQueue::iterator i = mResponseQueue.begin(); i != mResponseQueue.end(); ++i )
            {
                if( ( *i )->getRequest()->getID() == id )
                {
                    ( *i )->abortRequest();
                    break;
                }
            }
        }
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::abortRequestsByChannel( uint16 channel )
    {
        OGRE_LOCK_MUTEX( mProcessMutex );

        for( RequestQueue::iterator i = mProcessQueue.begin(); i != mProcessQueue.end(); ++i )
        {
            if( ( *i )->getChannel() == channel )
            {
                ( *i )->abortRequest();
            }
        }

        {
            OGRE_LOCK_MUTEX( mRequestMutex );

            for( RequestQueue::iterator i = mRequestQueue.begin(); i != mRequestQueue.end(); ++i )
            {
                if( ( *i )->getChannel() == channel )
                {
                    ( *i )->abortRequest();
                }
            }
        }
        {
            if( mIdleProcessed && mIdleProcessed->getChannel() == channel )
            {
                mIdleProcessed->abortRequest();
            }

            OGRE_LOCK_MUTEX( mIdleMutex );

            for( RequestQueue::iterator i = mIdleRequestQueue.begin(); i != mIdleRequestQueue.end();
                 ++i )
            {
                if( ( *i )->getChannel() == channel )
                {
                    ( *i )->abortRequest();
                }
            }
        }

        {
            OGRE_LOCK_MUTEX( mResponseMutex );

            for( ResponseQueue::iterator i = mResponseQueue.begin(); i != mResponseQueue.end(); ++i )
            {
                if( ( *i )->getRequest()->getChannel() == channel )
                {
                    ( *i )->abortRequest();
                }
            }
        }
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::abortPendingRequestsByChannel( uint16 channel )
    {
        {
            OGRE_LOCK_MUTEX( mRequestMutex );
            for( RequestQueue::iterator i = mRequestQueue.begin(); i != mRequestQueue.end(); ++i )
            {
                if( ( *i )->getChannel() == channel )
                {
                    ( *i )->abortRequest();
                }
            }
        }
        {
            OGRE_LOCK_MUTEX( mIdleMutex );

            for( RequestQueue::iterator i = mIdleRequestQueue.begin(); i != mIdleRequestQueue.end();
                 ++i )
            {
                if( ( *i )->getChannel() == channel )
                {
                    ( *i )->abortRequest();
                }
            }
        }
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::abortAllRequests()
    {
        OGRE_LOCK_MUTEX( mProcessMutex );
        {
            for( RequestQueue::iterator i = mProcessQueue.begin(); i != mProcessQueue.end(); ++i )
            {
                ( *i )->abortRequest();
            }
        }

        {
            OGRE_LOCK_MUTEX( mRequestMutex );

            for( RequestQueue::iterator i = mRequestQueue.begin(); i != mRequestQueue.end(); ++i )
            {
                ( *i )->abortRequest();
            }
        }

        {
            if( mIdleProcessed )
            {
                mIdleProcessed->abortRequest();
            }

            OGRE_LOCK_MUTEX( mIdleMutex );

            for( RequestQueue::iterator i = mIdleRequestQueue.begin(); i != mIdleRequestQueue.end();
                 ++i )
            {
                ( *i )->abortRequest();
            }
        }

        {
            OGRE_LOCK_MUTEX( mResponseMutex );

            for( ResponseQueue::iterator i = mResponseQueue.begin(); i != mResponseQueue.end(); ++i )
            {
                ( *i )->abortRequest();
            }
        }
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::setPaused( bool pause )
    {
        OGRE_LOCK_MUTEX( mRequestMutex );

        mPaused = pause;
    }
    //---------------------------------------------------------------------
    bool DefaultWorkQueueBase::isPaused() const { return mPaused; }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::setRequestsAccepted( bool accept )
    {
        OGRE_LOCK_MUTEX( mRequestMutex );

        mAcceptRequests = accept;
    }
    //---------------------------------------------------------------------
    bool DefaultWorkQueueBase::getRequestsAccepted() const { return mAcceptRequests; }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::_processNextRequest()
    {
        if( processIdleRequests() )
        {
            // Found idle requests.
            return;
        }
        Request *request = 0;
        {
            // scoped to only lock while retrieving the next request
            OGRE_LOCK_MUTEX( mProcessMutex );
            {
                OGRE_LOCK_MUTEX( mRequestMutex );

                if( !mRequestQueue.empty() )
                {
                    request = mRequestQueue.front();
                    mRequestQueue.pop_front();
                    mProcessQueue.push_back( request );
                }
            }
        }

        if( request )
        {
            processRequestResponse( request, false );
        }
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::processRequestResponse( Request *r, bool synchronous )
    {
        Response *response = processRequest( r );

        OGRE_LOCK_MUTEX( mProcessMutex );

        RequestQueue::iterator it;
        for( it = mProcessQueue.begin(); it != mProcessQueue.end(); ++it )
        {
            if( ( *it ) == r )
            {
                mProcessQueue.erase( it );
                break;
            }
        }
        if( mIdleProcessed == r )
        {
            mIdleProcessed = 0;
        }
        if( response )
        {
            if( !response->succeeded() )
            {
                // Failed, should we retry?
                const Request *req = response->getRequest();
                if( req->getRetryCount() )
                {
                    addRequestWithRID( req->getID(), req->getChannel(), req->getType(), req->getData(),
                                       req->getRetryCount() - 1 );
                    // discard response (this also deletes request)
                    OGRE_DELETE response;
                    return;
                }
            }
            if( synchronous )
            {
                processResponse( response );
                OGRE_DELETE response;
            }
            else
            {
                if( response->getRequest()->getAborted() )
                {
                    // destroy response user data
                    response->abortRequest();
                }
                // Queue response
                OGRE_LOCK_MUTEX( mResponseMutex );
                mResponseQueue.push_back( response );
                // no need to wake thread, this is processed by the main thread
            }
        }
        else
        {
            if( !r->getAborted() )
            {
                // no response, delete request
                LogManager::getSingleton().stream()
                    << "DefaultWorkQueueBase('" << mName << "') warning: no handler processed request "
                    << r->getID() << ", channel " << r->getChannel() << ", type " << r->getType();
            }
            OGRE_DELETE r;
        }
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::processResponses()
    {
        const uint64 msStart = Root::getSingleton().getTimer()->getMilliseconds();
        uint64 msCurrent = 0;

        // keep going until we run out of responses or out of time
        while( true )
        {
            Response *response = 0;
            {
                OGRE_LOCK_MUTEX( mResponseMutex );

                if( mResponseQueue.empty() )
                    break;  // exit loop
                else
                {
                    response = mResponseQueue.front();
                    mResponseQueue.pop_front();
                }
            }

            if( response )
            {
                processResponse( response );

                OGRE_DELETE response;
            }

            // time limit
            if( mResposeTimeLimitMS )
            {
                msCurrent = Root::getSingleton().getTimer()->getMilliseconds();
                if( msCurrent - msStart > mResposeTimeLimitMS )
                    break;
            }
        }
    }
    //---------------------------------------------------------------------
    WorkQueue::Response *DefaultWorkQueueBase::processRequest( Request *r )
    {
        RequestHandlerListByChannel handlerListCopy;
        {
            // lock the list only to make a copy of it, to maximise parallelism
            OGRE_LOCK_RW_MUTEX_READ( mRequestHandlerMutex );

            handlerListCopy = mRequestHandlers;
        }

        Response *response = 0;

        StringStream dbgMsg;
        dbgMsg <<
#if OGRE_THREAD_SUPPORT
            OGRE_THREAD_CURRENT_ID
#else
            "main"
#endif
               << "): ID=" << r->getID() << " channel=" << r->getChannel()
               << " requestType=" << r->getType();

        LogManager::getSingleton().stream( LML_TRIVIAL )
            << "DefaultWorkQueueBase('" << mName << "') - PROCESS_REQUEST_START(" << dbgMsg.str();

        RequestHandlerListByChannel::iterator i = handlerListCopy.find( r->getChannel() );
        if( i != handlerListCopy.end() )
        {
            RequestHandlerList &handlers = i->second;
            for( RequestHandlerList::reverse_iterator j = handlers.rbegin(); j != handlers.rend(); ++j )
            {
                // threadsafe call which tests canHandleRequest and calls it if so
                response = ( *j )->handleRequest( r, this );

                if( response )
                    break;
            }
        }

        LogManager::getSingleton().stream( LML_TRIVIAL )
            << "DefaultWorkQueueBase('" << mName << "') - PROCESS_REQUEST_END(" << dbgMsg.str()
            << " processed=" << ( response != 0 );

        return response;
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::processResponse( Response *r )
    {
        StringStream dbgMsg;
        dbgMsg << "thread:" <<
#if OGRE_THREAD_SUPPORT
            OGRE_THREAD_CURRENT_ID
#else
            "main"
#endif
               << "): ID=" << r->getRequest()->getID() << " success=" << r->succeeded() << " messages=["
               << r->getMessages() << "] channel=" << r->getRequest()->getChannel()
               << " requestType=" << r->getRequest()->getType();

        LogManager::getSingleton().stream( LML_TRIVIAL )
            << "DefaultWorkQueueBase('" << mName << "') - PROCESS_RESPONSE_START(" << dbgMsg.str();

        ResponseHandlerListByChannel::iterator i =
            mResponseHandlers.find( r->getRequest()->getChannel() );
        if( i != mResponseHandlers.end() )
        {
            ResponseHandlerList &handlers = i->second;
            for( ResponseHandlerList::reverse_iterator j = handlers.rbegin(); j != handlers.rend(); ++j )
            {
                if( ( *j )->canHandleResponse( r, this ) )
                {
                    ( *j )->handleResponse( r, this );
                }
            }
        }
        LogManager::getSingleton().stream( LML_TRIVIAL )
            << "DefaultWorkQueueBase('" << mName << "') - PROCESS_RESPONSE_END(" << dbgMsg.str();
    }

    bool DefaultWorkQueueBase::processIdleRequests()
    {
        {
            OGRE_LOCK_MUTEX( mIdleMutex );
            if( mIdleRequestQueue.empty() || mIdleThreadRunning )
            {
                return false;
            }
            else
            {
                mIdleThreadRunning = true;
            }
        }
        try
        {
            while( 1 )
            {
                {
                    OGRE_LOCK_MUTEX(
                        mProcessMutex );  // mProcessMutex needs to be the top mutex to prevent livelocks
                    {
                        OGRE_LOCK_MUTEX( mIdleMutex );
                        if( !mIdleRequestQueue.empty() )
                        {
                            mIdleProcessed = mIdleRequestQueue.front();
                            mIdleRequestQueue.pop_front();
                        }
                        else
                        {
                            mIdleProcessed = 0;
                            mIdleThreadRunning = false;
                            return true;
                        }
                    }
                }
                processRequestResponse( mIdleProcessed, false );
            }
        }
        catch( ... )
        {  // Normally this should not happen.
            {
                // It is very important to clean up or the idle thread will be locked forever!
                OGRE_LOCK_MUTEX( mProcessMutex );
                {
                    OGRE_LOCK_MUTEX( mIdleMutex );
                    if( mIdleProcessed )
                    {
                        mIdleProcessed->abortRequest();
                    }
                    mIdleProcessed = 0;
                    mIdleThreadRunning = false;
                }
            }
            Ogre::LogManager::getSingleton().stream() << "Exception caught in top of worker thread!";

            return true;
        }
    }

    //---------------------------------------------------------------------

    void DefaultWorkQueueBase::WorkerFunc::operator()() { mQueue->_threadMain(); }

    void DefaultWorkQueueBase::WorkerFunc::operator()() const { mQueue->_threadMain(); }

    void DefaultWorkQueueBase::WorkerFunc::run() { mQueue->_threadMain(); }
}  // namespace Ogre
