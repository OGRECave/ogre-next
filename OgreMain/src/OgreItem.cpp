/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE-Next
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org

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
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     WISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR      DEALINGS IN
  THE SOFTWARE.
  -----------------------------------------------------------------------------
*/

#include "OgreStableHeaders.h"

#include "OgreItem.h"

#include "Animation/OgreSkeletonInstance.h"
#include "OgreException.h"
#include "OgreHlmsManager.h"
#include "OgreLogManager.h"
#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreSceneNode.h"
#include "OgreSubItem.h"
#include "OgreSubMesh2.h"

namespace Ogre
{
    extern const FastArray<Real> c_DefaultLodMesh;
    //-----------------------------------------------------------------------
    Item::Item( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *manager ) :
        MovableObject( id, objectMemoryManager, manager, 10u ),
        mInitialised( false )
    {
        mObjectData.mQueryFlags[mObjectData.mIndex] = SceneManager::QUERY_ENTITY_DEFAULT_MASK;
    }
    //-----------------------------------------------------------------------
    Item::Item( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *manager,
                const MeshPtr &mesh, bool bUseMeshMat /*= true */ ) :
        MovableObject( id, objectMemoryManager, manager, 10u ),
        mMesh( mesh ),
        mInitialised( false )
    {
        _initialise( false, bUseMeshMat );
        mObjectData.mQueryFlags[mObjectData.mIndex] = SceneManager::QUERY_ENTITY_DEFAULT_MASK;
    }

    //-----------------------------------------------------------------------
    void Item::loadingComplete( Resource *res )
    {
        if( res == mMesh.get() && mInitialised )
        {
            _initialise( true );
        }
    }
    //-----------------------------------------------------------------------
    void Item::_initialise( bool forceReinitialise /*= false*/, bool bUseMeshMat /*= true */ )
    {
        vector<String>::type prevMaterialsList;
        if( forceReinitialise )
        {
            if( mMesh->getNumSubMeshes() == mSubItems.size() )
            {
                for( SubItem &subitem : mSubItems )
                    prevMaterialsList.push_back( subitem.getDatablockOrMaterialName() );
            }
            _deinitialise();
        }

        if( mInitialised )
            return;

        // register for a callback when mesh is finished loading
        mMesh->addListener( this );

        // On-demand load
        mMesh->load();
        // If loading failed, or deferred loading isn't done yet, defer
        // Will get a callback in the case of deferred loading
        // Skeletons are cascade-loaded so no issues there
        if( !mMesh->isLoaded() )
            return;

        // Is mesh skeletally animated?
        if( mMesh->hasSkeleton() && mMesh->getSkeleton() && mManager )
        {
            const SkeletonDef *skeletonDef = mMesh->getSkeleton().get();
            mSkeletonInstance = mManager->createSkeletonInstance( skeletonDef );
        }

        mLodMesh = mMesh->_getLodValueArray();

        // Build main subItem list
        buildSubItems( prevMaterialsList.empty() ? 0 : &prevMaterialsList, bUseMeshMat );

        {
            // Without filling the renderables list, the RenderQueue won't
            // catch our sub entities and thus we won't be rendered
            mRenderables.reserve( mSubItems.size() );
            for( SubItem &subitem : mSubItems )
                mRenderables.push_back( &subitem );
        }

        Aabb aabb( mMesh->getAabb() );
        mObjectData.mLocalAabb->setFromAabb( aabb, mObjectData.mIndex );
        mObjectData.mWorldAabb->setFromAabb( aabb, mObjectData.mIndex );
        mObjectData.mLocalRadius[mObjectData.mIndex] = aabb.getRadius();
        mObjectData.mWorldRadius[mObjectData.mIndex] = aabb.getRadius();
        if( mParentNode )
        {
            updateSingleWorldAabb();
            updateSingleWorldRadius();
        }

        mInitialised = true;
    }

    //-----------------------------------------------------------------------
    void Item::_deinitialise()
    {
        if( !mInitialised )
            return;

        // Delete submeshes
        mSubItems.clear();
        mRenderables.clear();

        // If mesh is skeletally animated: destroy instance
        assert( mManager || !mSkeletonInstance );
        if( mSkeletonInstance )
        {
            mSkeletonInstance->_decrementRefCount();
            if( mSkeletonInstance->_getRefCount() == 0u )
                mManager->destroySkeletonInstance( mSkeletonInstance );

            mSkeletonInstance = 0;
        }

        mInitialised = false;
    }
    //-----------------------------------------------------------------------
    Item::~Item()
    {
        _deinitialise();
        // Unregister our listener
        mMesh->removeListener( this );
    }
    //-----------------------------------------------------------------------
    const MeshPtr &Item::getMesh() const { return mMesh; }
    //-----------------------------------------------------------------------
    SubItem *Item::getSubItem( size_t index )
    {
        if( index >= mSubItems.size() )
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Index out of bounds.", "Item::getSubItem" );
        return &mSubItems[index];
    }
    //-----------------------------------------------------------------------
    const SubItem *Item::getSubItem( size_t index ) const
    {
        if( index >= mSubItems.size() )
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Index out of bounds.", "Item::getSubItem" );
        return &mSubItems[index];
    }
    //-----------------------------------------------------------------------
    size_t Item::getNumSubItems() const { return mSubItems.size(); }
    //-----------------------------------------------------------------------
    void Item::setDatablock( HlmsDatablock *datablock )
    {
        for( SubItem &subitem : mSubItems )
            subitem.setDatablock( datablock );
    }
    //-----------------------------------------------------------------------
    void Item::setDatablock( IdString datablockName )
    {
        HlmsManager *hlmsManager = Root::getSingleton().getHlmsManager();
        HlmsDatablock *datablock = hlmsManager->getDatablock( datablockName );

        setDatablock( datablock );
    }
    //-----------------------------------------------------------------------
    void Item::setDatablockOrMaterialName(
        const String &name,
        const String &groupName /* = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME */ )
    {
        // Set for all subentities
        for( SubItem &subitem : mSubItems )
            subitem.setDatablockOrMaterialName( name, groupName );
    }
    //-----------------------------------------------------------------------
    void Item::setMaterialName(
        const String &name,
        const String &groupName /* = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME */ )
    {
        // Set for all subentities
        for( SubItem &subitem : mSubItems )
            subitem.setMaterialName( name, groupName );
    }
    //-----------------------------------------------------------------------
    void Item::setMaterial( const MaterialPtr &material )
    {
        // Set for all subentities
        for( SubItem &subitem : mSubItems )
            subitem.setMaterial( material );
    }
    //-----------------------------------------------------------------------
    const String &Item::getMovableType() const { return ItemFactory::FACTORY_TYPE_NAME; }
    //-----------------------------------------------------------------------
    void Item::buildSubItems( vector<String>::type *materialsList, bool bUseMeshMat /* = true*/ )
    {
        // Create SubEntities
        unsigned numSubMeshes = mMesh->getNumSubMeshes();
        mSubItems.reserve( numSubMeshes );
        const Ogre::String defaultDatablock;
        for( unsigned i = 0; i < numSubMeshes; ++i )
        {
            SubMesh *subMesh = mMesh->getSubMesh( i );
            mSubItems.push_back( SubItem( this, subMesh ) );

            // Try first Hlms materials, then the low level ones.

            mSubItems.back().setDatablockOrMaterialName(
                materialsList ? ( *materialsList )[i]
                              : ( bUseMeshMat ? subMesh->mMaterialName : defaultDatablock ),
                mMesh->getGroup() );
        }
    }
    //-----------------------------------------------------------------------
    void Item::useSkeletonInstanceFrom( Item *master )
    {
        if( mMesh->getSkeletonName() != master->mMesh->getSkeletonName() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Cannot share skeleton instance if meshes use different skeletons",
                         "Item::useSkeletonInstanceFrom" );
        }

        if( mSkeletonInstance )
        {
            mSkeletonInstance->_decrementRefCount();
            if( mSkeletonInstance->_getRefCount() == 0u )
                mManager->destroySkeletonInstance( mSkeletonInstance );
        }

        mSkeletonInstance = master->mSkeletonInstance;
        mSkeletonInstance->_incrementRefCount();
    }
    //-----------------------------------------------------------------------
    void Item::stopUsingSkeletonInstanceFromMaster()
    {
        if( mSkeletonInstance )
        {
            assert( mSkeletonInstance->_getRefCount() > 1u &&
                    "This skeleton is Item is not sharing its skeleton!" );

            mSkeletonInstance->_decrementRefCount();
            if( mSkeletonInstance->_getRefCount() == 0u )
                mManager->destroySkeletonInstance( mSkeletonInstance );

            const SkeletonDef *skeletonDef = mMesh->getSkeleton().get();
            mSkeletonInstance = mManager->createSkeletonInstance( skeletonDef );
        }
    }
    //-----------------------------------------------------------------------
    bool Item::sharesSkeletonInstance() const
    {
        return mSkeletonInstance && mSkeletonInstance->_getRefCount() > 1u;
    }
    //-----------------------------------------------------------------------
    void Item::setSkeletonEnabled( const bool bEnable )
    {
        OGRE_ASSERT_LOW( !sharesSkeletonInstance() );
        if( mSkeletonInstance && !bEnable )
        {
            mSkeletonInstance->_decrementRefCount();
            if( mSkeletonInstance->_getRefCount() == 0u )
                mManager->destroySkeletonInstance( mSkeletonInstance );

            mSkeletonInstance = 0;

            for( SubItem &subitem : mSubItems )
            {
                HlmsDatablock *oldDatablock = subitem.getDatablock();

                subitem.mHasSkeletonAnimation = false;
                subitem.mBlendIndexToBoneIndexMap = nullptr;

                if( oldDatablock )
                {
                    subitem._setNullDatablock();
                    subitem.setDatablock( oldDatablock );
                }
            }
        }
        else if( !mSkeletonInstance && mMesh->hasSkeleton() && mMesh->getSkeleton() && bEnable )
        {
            const SkeletonDef *skeletonDef = mMesh->getSkeleton().get();
            mSkeletonInstance = mManager->createSkeletonInstance( skeletonDef );
            for( SubItem &subitem : mSubItems )
            {
                HlmsDatablock *oldDatablock = subitem.getDatablock();
                subitem.setupSkeleton();
                if( oldDatablock )
                {
                    subitem._setNullDatablock();
                    subitem.setDatablock( oldDatablock );
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void Item::_notifyParentNodeMemoryChanged()
    {
        if( mSkeletonInstance /*&& !mSharedTransformEntity*/ )
        {
            mSkeletonInstance->setParentNode( mSkeletonInstance->getParentNode() );
        }
    }

    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    String ItemFactory::FACTORY_TYPE_NAME = "Item";
    //-----------------------------------------------------------------------
    const String &ItemFactory::getType() const { return FACTORY_TYPE_NAME; }
    //-----------------------------------------------------------------------
    MovableObject *ItemFactory::createInstanceImpl( IdType id, ObjectMemoryManager *objectMemoryManager,
                                                    SceneManager *manager,
                                                    const NameValuePairList *params )
    {
        // must have mesh parameter
        MeshPtr pMesh;
        bool useMeshMat = true;
        if( params != 0 )
        {
            String groupName = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;

            NameValuePairList::const_iterator ni;
            ni = params->find( "resourceGroup" );
            if( ni != params->end() )
            {
                groupName = ni->second;
            }

            ni = params->find( "mesh" );
            if( ni != params->end() )
            {
                // Get mesh (load if required)
                pMesh = MeshManager::getSingleton().load( ni->second, groupName );
            }

            ni = params->find( "useMeshMat" );
            if( ni != params->end() )
            {
                useMeshMat = StringConverter::parseBool( ni->second );
            }
        }
        if( !pMesh )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "'mesh' parameter required when constructing an Item.",
                         "ItemFactory::createInstance" );
        }

        return OGRE_NEW Item( id, objectMemoryManager, manager, pMesh, useMeshMat );
    }
    //-----------------------------------------------------------------------
    void ItemFactory::destroyInstance( MovableObject *obj ) { OGRE_DELETE obj; }

}  // namespace Ogre
