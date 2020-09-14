/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#include "OgreStableHeaders.h"

#include "OgreSceneManager.h"

#include "OgreCamera.h"
#include "OgreMeshManager.h"
#include "OgreDecal.h"
#include "OgreInternalCubemapProbe.h"
#include "OgreEntity.h"
#include "OgreSubEntity.h"
#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreLight.h"
#include "OgreControllerManager.h"
#include "OgreMaterialManager.h"
#include "OgreAnimation.h"
#include "OgreRenderObjectListener.h"
#include "OgreBillboardSet.h"
#include "OgreTechnique.h"
#include "OgreLogManager.h"
#include "OgreRoot.h"
#include "OgreGpuProgramManager.h"
#include "OgreGpuProgram.h"
#include "OgreDataStream.h"
#include "OgreStaticGeometry.h"
#include "OgreManualObject.h"
#include "OgreManualObject2.h"
#include "OgreBillboardChain.h"
#include "OgreRibbonTrail.h"
#include "OgreParticleSystemManager.h"
#include "OgreParticleSystem.h"
#include "OgreProfiler.h"
#include "OgreTextureGpuManager.h"
#include "OgreSceneNode.h"
#include "OgreRadialDensityMask.h"
#include "OgreRectangle2D2.h"
#include "OgreLodListener.h"
#include "OgreOldNode.h"
#include "OgreLodStrategyManager.h"
#include "OgreRenderQueueListener.h"
#include "OgreViewport.h"
#include "OgreWireAabb.h"
#include "OgreHlmsManager.h"
#include "OgreForward3D.h"
#include "OgreForwardClustered.h"
#include "Animation/OgreSkeletonDef.h"
#include "Animation/OgreSkeletonInstance.h"
#include "Animation/OgreTagPoint.h"
#include "Compositor/OgreCompositorShadowNode.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h"
#include "Threading/OgreBarrier.h"
#include "Threading/OgreUniformScalableTask.h"

// This class implements the most basic scene manager

#include <cstdio>

namespace Ogre {

//-----------------------------------------------------------------------
uint32 SceneManager::QUERY_ENTITY_DEFAULT_MASK         = 0x80000000;
uint32 SceneManager::QUERY_FX_DEFAULT_MASK             = 0x40000000;
uint32 SceneManager::QUERY_STATICGEOMETRY_DEFAULT_MASK = 0x20000000;
uint32 SceneManager::QUERY_LIGHT_DEFAULT_MASK          = 0x10000000;
uint32 SceneManager::QUERY_FRUSTUM_DEFAULT_MASK        = 0x08000000;
//-----------------------------------------------------------------------
SceneManager::SceneManager( const String& name, size_t numWorkerThreads ) :
IdObject( Id::generateNewId<SceneManager>() ),
mNumDecals( 0 ),
mNumCubemapProbes( 0 ),
mStaticMinDepthLevelDirty( 0 ),
mStaticEntitiesDirty( true ),
mPrePassMode( PrePassNone ),
mSsrTexture( 0 ),
mRefractionsTexture( 0 ),
mName(name),
mRenderQueue( 0 ),
mForwardPlusSystem( 0 ),
mForwardPlusImpl( 0 ),
mBuildLegacyLightList( false ),
mDecalsDiffuseTex( 0 ),
mDecalsNormalsTex( 0 ),
mDecalsEmissiveTex( 0 ),
mCamerasInProgress(0),
mCurrentViewport0(0),
mCurrentPass(0),
mCurrentShadowNode(0),
mShadowNodeIsReused( false ),
mSkyMethod( SkyCubemap ),
mSky( 0 ),
mRadialDensityMask( 0 ),
mFogMode(FOG_NONE),
mFogColour(),
mFogStart(0),
mFogEnd(0),
mFogDensity(0),
mLastFrameNumber(0),
mResetIdentityView(false),
mResetIdentityProj(false),
mFlipCullingOnNegativeScale(true),
mShadowCasterPlainBlackPass(0),
mDisplayNodes(false),
mShowBoundingBoxes(false),
mAutoParamDataSource(0),
mLateMaterialResolving(false),
mShadowColour(ColourValue(0.25, 0.25, 0.25)),
mShadowDirLightExtrudeDist(10000),
mIlluminationStage(IRS_NONE),
mLightClippingInfoMapFrameNumber(999),
mDefaultShadowFarDist(200),
mDefaultShadowFarDistSquared(200*200),
mShadowTextureOffset(0.6),
mShadowTextureFadeStart(0.7),
mShadowTextureFadeEnd(0.9),
mShadowTextureCustomCasterPass(0),
mVisibilityMask(0xFFFFFFFF & VisibilityFlags::RESERVED_VISIBILITY_FLAGS),
mLightMask(0xFFFFFFFF),
mFindVisibleObjects(true),
mNumWorkerThreads( std::max<size_t>( numWorkerThreads, 1u ) ),
mForceMainThread( numWorkerThreads == 0u ? true : false ),
mUpdateBoundsRequest( 0 ),
mUserTask( 0 ),
mRequestType( NUM_REQUESTS ),
mWorkerThreadsBarrier( 0 ),
mSuppressRenderStateChanges(false),
mLastLightHash(0),
mLastLightLimit(0),
mLastLightHashGpuProgram(0),
mGpuParamsDirty((uint16)GPV_ALL)
{
    for( size_t i=0; i<NUM_SCENE_MEMORY_MANAGER_TYPES; ++i )
        mSceneRoot[i] = 0;
    mSceneDummy = 0;

    memset( mAmbientSphericalHarmonics, 0, sizeof( mAmbientSphericalHarmonics ) );

    setAmbientLight( ColourValue::Black, ColourValue::Black, Vector3::UNIT_Y, 1.0f );

    mNodeMemoryManager[SCENE_STATIC]._setTwin( SCENE_STATIC, &mNodeMemoryManager[SCENE_DYNAMIC] );
    mNodeMemoryManager[SCENE_DYNAMIC]._setTwin( SCENE_DYNAMIC, &mNodeMemoryManager[SCENE_STATIC] );
    mEntityMemoryManager[SCENE_STATIC]._setTwin( SCENE_STATIC, &mEntityMemoryManager[SCENE_DYNAMIC] );
    mEntityMemoryManager[SCENE_DYNAMIC]._setTwin( SCENE_DYNAMIC, &mEntityMemoryManager[SCENE_STATIC] );
    mForwardPlusMemoryManager[SCENE_STATIC]._setTwin( SCENE_STATIC, &mForwardPlusMemoryManager[SCENE_DYNAMIC] );
    mForwardPlusMemoryManager[SCENE_DYNAMIC]._setTwin( SCENE_DYNAMIC, &mForwardPlusMemoryManager[SCENE_STATIC] );

    Root *root = Root::getSingletonPtr();
    if (root)
        _setDestinationRenderSystem(root->getRenderSystem());

    mRenderQueue = OGRE_NEW RenderQueue( root->getHlmsManager(), this,
                                         mDestRenderSystem->getVaoManager() );

    // create the auto param data source instance
    mAutoParamDataSource = createAutoParamDataSource();

    mGlobalLightListPerThread.resize( mNumWorkerThreads );
    mBuildLightListRequestPerThread.resize( mNumWorkerThreads );
    mVisibleObjects.resize( mNumWorkerThreads );
    mTmpVisibleObjects.resize( mNumWorkerThreads );

    startWorkerThreads();

    // Init shadow caster material for texture shadows
    if (!mShadowCasterPlainBlackPass && mDestRenderSystem)
    {
        MaterialPtr matPlainBlack = MaterialManager::getSingleton().getByName(
            "Ogre/TextureShadowCaster");
        if (matPlainBlack.isNull())
        {
            matPlainBlack = MaterialManager::getSingleton().create(
                "Ogre/TextureShadowCaster",
                ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME);
            mShadowCasterPlainBlackPass = matPlainBlack->getTechnique(0)->getPass(0);
            // Lighting has to be on, because we need shadow coloured objects
            // Note that because we can't predict vertex programs, we'll have to
            // bind light values to those, and so we bind White to ambient
            // reflectance, and we'll set the ambient colour to the shadow colour
            mShadowCasterPlainBlackPass->setAmbient(ColourValue::White);
            mShadowCasterPlainBlackPass->setDiffuse(ColourValue::Black);
            mShadowCasterPlainBlackPass->setSelfIllumination(ColourValue::Black);
            mShadowCasterPlainBlackPass->setSpecular(ColourValue::Black);
            // Override fog
            mShadowCasterPlainBlackPass->setFog(true, FOG_NONE);
            // no textures or anything else, we will bind vertex programs
            // every so often though
        }
        else
        {
            mShadowCasterPlainBlackPass = matPlainBlack->getTechnique(0)->getPass(0);
        }
    }

    for( size_t i=0; i<NUM_SCENE_MEMORY_MANAGER_TYPES; ++i )
    {
        // Create root scene node
        mSceneRoot[i] = createSceneNodeImpl( (SceneNode*)0, &mNodeMemoryManager[i] );
        mSceneRoot[i]->setName( "Ogre/SceneRoot" + StringConverter::toString( i ) );
        mSceneRoot[i]->_getDerivedPositionUpdated();
    }

    mSceneDummy = createSceneNodeImpl( (SceneNode*)0, &mNodeMemoryManager[SCENE_DYNAMIC] );
    mSceneDummy->setName( "Ogre/SceneManager/Dummy");
    mSceneDummy->_getDerivedPositionUpdated();
}
//-----------------------------------------------------------------------
SceneManager::~SceneManager()
{
    OGRE_DELETE mForwardPlusSystem;
    mForwardPlusSystem  = 0;
    mForwardPlusImpl    = 0;

    OGRE_DELETE mSky;
    mSky = 0;

    OGRE_DELETE mRadialDensityMask;
    mRadialDensityMask = 0;

    fireSceneManagerDestroyed();
    clearScene( true, false );
    destroyAllCameras();

    // clear down movable object collection map
    {
            OGRE_LOCK_MUTEX(mMovableObjectCollectionMapMutex);
        for (MovableObjectCollectionMap::iterator i = mMovableObjectCollectionMap.begin();
            i != mMovableObjectCollectionMap.end(); ++i)
        {
            OGRE_DELETE_T(i->second, MovableObjectCollection, MEMCATEGORY_SCENE_CONTROL);
        }
        mMovableObjectCollectionMap.clear();
    }

    OGRE_DELETE mSceneDummy;
    mSceneDummy = 0;

    for( size_t i=0; i<NUM_SCENE_MEMORY_MANAGER_TYPES; ++i )
    {
        OGRE_DELETE mSceneRoot[i];
        mSceneRoot[i] = 0;
    }
    OGRE_DELETE mRenderQueue;
    OGRE_DELETE mAutoParamDataSource;

    mRenderQueue            = 0;
    mAutoParamDataSource    = 0;

    stopWorkerThreads();
}
//-----------------------------------------------------------------------
SceneManager::MovableObjectVec SceneManager::findMovableObjects( const String& type, const String& name )
{
    MovableObjectVec objects;
    MovableObjectIterator itor = getMovableObjectIterator( type );
    while( itor.hasMoreElements() )
    {
        MovableObject* object = itor.peekNext();
        if( object->getName() == name )
            objects.push_back( object );

        itor.moveNext();
    }

    return objects;
}
//-----------------------------------------------------------------------
Camera* SceneManager::createCamera( const String &name, bool isVisible, bool forCubemapping )
{
    if( mCamerasByName.find( name ) != mCamerasByName.end() )
    {
        OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM, 
                     "Camera with name '" + name + "' already exists", "SceneManager::createCamera" );
    }

    Camera *c = OGRE_NEW Camera( Id::generateNewId<MovableObject>(),
                                 &mEntityMemoryManager[SCENE_DYNAMIC], this );
    mCameras.push_back( c );
    c->mGlobalIndex = mCameras.size() - 1;
    c->setName( name );
    mCamerasByName[name] = c;

    if( isVisible )
    {
        if( !forCubemapping )
            mVisibleCameras.push_back( c );
        else
            mCubeMapCameras.push_back( c );
    }

    mSceneRoot[SCENE_DYNAMIC]->attachObject( c );
    return c;
}
//-----------------------------------------------------------------------
Camera* SceneManager::findCamera( IdString name ) const
{
    Camera *retVal = findCameraNoThrow( name );
    if( !retVal )
    {
        OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                     "Camera with name '" + name.getFriendlyText() + "' not found",
                     "SceneManager::findCamera" );
    }

    return retVal;
}
//-----------------------------------------------------------------------
Camera* SceneManager::findCameraNoThrow( IdString name ) const
{
    Camera *retVal = 0;
    CameraMap::const_iterator itor = mCamerasByName.find( name );
    if( itor != mCamerasByName.end() )
        retVal = itor->second;

    return retVal;
}
//-----------------------------------------------------------------------
void SceneManager::destroyCamera(Camera *cam)
{
    if(!cam)
        OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot destroy a null Camera.", "SceneManager::destroyCamera");

    checkMovableObjectIntegrity( mCameras, cam );

    {
        FrustumVec::iterator it = std::find( mVisibleCameras.begin(), mVisibleCameras.end(), cam );
        if( it != mVisibleCameras.end() )
            efficientVectorRemove( mVisibleCameras, it );

        it = std::find( mCubeMapCameras.begin(), mCubeMapCameras.end(), cam );
        if( it != mCubeMapCameras.end() )
            efficientVectorRemove( mCubeMapCameras, it );
    }

    IdString camName( cam->getName() );

    // Find in list
    CameraList::iterator itor = mCameras.begin() + cam->mGlobalIndex;
    itor = efficientVectorRemove( mCameras, itor );
    OGRE_DELETE cam;
    cam = 0;

    //The node that was at the end got swapped and has now a different index
    if( itor != mCameras.end() )
        (*itor)->mGlobalIndex = itor - mCameras.begin();

    CameraMap::iterator itorMap = mCamerasByName.find( camName );
    if( itorMap == mCamerasByName.end() )
    {
        OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, 
                     "Camera with name '" + camName.getFriendlyText() + "' not found!",
                     "SceneManager::destroyCamera" );
    }
    else
    {
        mCamerasByName.erase( itorMap );
    }
}

//-----------------------------------------------------------------------
void SceneManager::destroyAllCameras(void)
{
    CameraList::iterator camIt  = mCameras.begin();
    CameraList::iterator camEnd = mCameras.end();

    while( camIt != camEnd )
    {
        const size_t oldIdx = camIt - mCameras.begin();
        destroyCamera( *camIt );
        camIt  = mCameras.begin() + oldIdx;
        camEnd = mCameras.end();
    }
}
//-----------------------------------------------------------------------
void SceneManager::_setLightCullingVisibility( Camera *camera, bool collectLights, bool isCubemap )
{
    isCubemap &= collectLights;

    FrustumVec::iterator it = std::find( mVisibleCameras.begin(), mVisibleCameras.end(), camera );
    if( it != mVisibleCameras.end() && !collectLights )
        efficientVectorRemove( mVisibleCameras, it );
    else if( it == mVisibleCameras.end() && collectLights && !isCubemap )
        mVisibleCameras.push_back( camera );

    it = std::find( mCubeMapCameras.begin(), mCubeMapCameras.end(), camera );
    if( it != mCubeMapCameras.end() && !isCubemap )
        efficientVectorRemove( mCubeMapCameras, it );
    else if( it == mCubeMapCameras.end() && isCubemap )
        mCubeMapCameras.push_back( camera );
}
//-----------------------------------------------------------------------
void SceneManager::clearFrameData(void)
{
    mGlobalLightList.lights.clear();
    mRenderQueue->clearState();
}
//-----------------------------------------------------------------------
Light* SceneManager::createLight()
{
    //We need calculateTotalNumObjectDataIncludingFragmentedSlots instead of getTotalNumObjects
    //because of the "skips" we perform when doing multithreading
    //(see BuildLightListRequest::startLightIdx). And these skips include fragmented slots
    //(unused slots). If we use getTotalNumObjects, we'll get memory corruption when writing
    //to visibilityMask and boundingSphere from the last thread.
    const size_t requiredCapacity =
            mLightMemoryManager.calculateTotalNumObjectDataIncludingFragmentedSlots() + 1u;
    if( mGlobalLightList.lights.capacity() < requiredCapacity )
    {
        assert( mGlobalLightList.lights.empty() &&
                "Don't create objects in the middle of a scene update!" );
        mGlobalLightList.lights.reserve( requiredCapacity );
        OGRE_FREE_SIMD( mGlobalLightList.visibilityMask, MEMCATEGORY_SCENE_CONTROL );
        OGRE_FREE_SIMD( mGlobalLightList.boundingSphere, MEMCATEGORY_SCENE_CONTROL );
        mGlobalLightList.visibilityMask = OGRE_ALLOC_T_SIMD( uint32, requiredCapacity,
                                                            MEMCATEGORY_SCENE_CONTROL );
        mGlobalLightList.boundingSphere = OGRE_ALLOC_T_SIMD( Sphere, requiredCapacity,
                                                            MEMCATEGORY_SCENE_CONTROL );
    }

    Light *newLight = static_cast<Light*>(
                            createMovableObject(LightFactory::FACTORY_TYPE_NAME, &mLightMemoryManager));
    return newLight;
}
//-----------------------------------------------------------------------
void SceneManager::destroyLight(Light *l)
{
    destroyMovableObject(l);
}
//-----------------------------------------------------------------------
void SceneManager::destroyAllLights(void)
{
    destroyAllMovableObjectsByType(LightFactory::FACTORY_TYPE_NAME);
}
//-----------------------------------------------------------------------
void SceneManager::defragmentMemoryPools(void)
{
    for( size_t i=0; i<NUM_SCENE_MEMORY_MANAGER_TYPES; ++i )
    {
        mNodeMemoryManager[i].defragment();
        mEntityMemoryManager[i].defragment();
        mForwardPlusMemoryManager[i].defragment();
    }

    mLightMemoryManager.defragment();
    //Skeletons are more complex because the new slot count must be multiple bonesPerDepth
    //mSkeletonAnimationManager.defragment();
    mTagPointNodeMemoryManager.defragment();
}
//-----------------------------------------------------------------------
void SceneManager::shrinkToFitMemoryPools(void)
{
    for( size_t i=0; i<NUM_SCENE_MEMORY_MANAGER_TYPES; ++i )
    {
        mNodeMemoryManager[i].shrinkToFit();
        mEntityMemoryManager[i].shrinkToFit();
        mForwardPlusMemoryManager[i].shrinkToFit();
    }

    mLightMemoryManager.shrinkToFit();
    //Skeletons are more complex because the new slot count must be multiple bonesPerDepth
    //mSkeletonAnimationManager.shrinkToFit();
    mTagPointNodeMemoryManager.shrinkToFit();
}
//-----------------------------------------------------------------------
Item* SceneManager::createItem( const String& meshName,
                                const String& groupName, /* = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME */
                                SceneMemoryMgrTypes sceneType /*= SCENE_DYNAMIC */ )
{
    // delegate to factory implementation
    NameValuePairList params;
    params["mesh"] = meshName;
    params["resourceGroup"] = groupName;
    return static_cast<Item*>( createMovableObject( ItemFactory::FACTORY_TYPE_NAME,
                                                    &mEntityMemoryManager[sceneType], &params) );

}
//---------------------------------------------------------------------
Item* SceneManager::createItem( const MeshPtr& pMesh, SceneMemoryMgrTypes sceneType )
{
    return createItem(pMesh->getName(), pMesh->getGroup(), sceneType);
}
//-----------------------------------------------------------------------
void SceneManager::destroyItem( Item *i )
{
    destroyMovableObject( i );
}
//-----------------------------------------------------------------------
void SceneManager::destroyAllItems(void)
{
    destroyAllMovableObjectsByType(ItemFactory::FACTORY_TYPE_NAME);
}
//-----------------------------------------------------------------------
WireAabb* SceneManager::createWireAabb(void)
{
    return static_cast<WireAabb*>( createMovableObject( WireAabbFactory::FACTORY_TYPE_NAME,
                                                        &mEntityMemoryManager[SCENE_DYNAMIC], 0 ) );

}
//-----------------------------------------------------------------------
void SceneManager::destroyWireAabb( WireAabb *i )
{
    destroyMovableObject( i );
}
//-----------------------------------------------------------------------
void SceneManager::destroyAllWireAabbs(void)
{
    destroyAllMovableObjectsByType(WireAabbFactory::FACTORY_TYPE_NAME);
}
//-----------------------------------------------------------------------
void SceneManager::_addWireAabb( WireAabb *wireAabb )
{
    mTrackingWireAabbs.push_back( wireAabb );
}
//-----------------------------------------------------------------------
void SceneManager::_removeWireAabb( WireAabb *wireAabb )
{
    WireAabbVec::iterator itor = std::find( mTrackingWireAabbs.begin(),
                                            mTrackingWireAabbs.end(), wireAabb );
    assert( itor != mTrackingWireAabbs.end() );
    efficientVectorRemove( mTrackingWireAabbs, itor );
}
//-----------------------------------------------------------------------
Decal* SceneManager::createDecal( SceneMemoryMgrTypes sceneType )
{
    ++mNumDecals;
    return static_cast<Decal*>( createMovableObject( DecalFactory::FACTORY_TYPE_NAME,
                                                     &mForwardPlusMemoryManager[sceneType], 0 ) );

}
//-----------------------------------------------------------------------
void SceneManager::destroyDecal( Decal *i )
{
    --mNumDecals;
    destroyMovableObject( i );
}
//-----------------------------------------------------------------------
void SceneManager::destroyAllDecals(void)
{
    mNumDecals = 0;
    destroyAllMovableObjectsByType( DecalFactory::FACTORY_TYPE_NAME );
}
//-----------------------------------------------------------------------
InternalCubemapProbe* SceneManager::_createCubemapProbe( SceneMemoryMgrTypes sceneType )
{
    ++mNumCubemapProbes;
    return static_cast<InternalCubemapProbe*>( createMovableObject(
                                                   InternalCubemapProbeFactory::FACTORY_TYPE_NAME,
                                                   &mForwardPlusMemoryManager[sceneType], 0 ) );

}
//-----------------------------------------------------------------------
void SceneManager::_destroyCubemapProbe( InternalCubemapProbe *i )
{
    --mNumCubemapProbes;
    destroyMovableObject( i );
}
//-----------------------------------------------------------------------
void SceneManager::_destroyAllCubemapProbes(void)
{
    mNumCubemapProbes = 0;
    destroyAllMovableObjectsByType( InternalCubemapProbeFactory::FACTORY_TYPE_NAME );
}
//-----------------------------------------------------------------------
v1::Entity* SceneManager::createEntity( PrefabType ptype, SceneMemoryMgrTypes sceneType )
{
    switch (ptype)
    {
    case PT_PLANE:
        return createEntity( "Prefab_Plane", ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                             sceneType );
    case PT_CUBE:
        return createEntity( "Prefab_Cube", ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                             sceneType );
    case PT_SPHERE:
        return createEntity( "Prefab_Sphere", ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                             sceneType );

        break;
    }

    OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Unknown prefab type", "SceneManager::createEntity" );
}
//-----------------------------------------------------------------------
v1::Entity* SceneManager::createEntity( const String& meshName,
                                        const String& groupName, /* = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME */
                                        SceneMemoryMgrTypes sceneType /*= SCENE_DYNAMIC */ )
{
    // delegate to factory implementation
    NameValuePairList params;
    params["mesh"] = meshName;
    params["resourceGroup"] = groupName;
    return static_cast<v1::Entity*>( createMovableObject( v1::EntityFactory::FACTORY_TYPE_NAME,
                                                          &mEntityMemoryManager[sceneType], &params) );

}
//---------------------------------------------------------------------
v1::Entity* SceneManager::createEntity( const v1::MeshPtr& pMesh, SceneMemoryMgrTypes sceneType )
{
    return createEntity(pMesh->getName(), pMesh->getGroup(), sceneType);
}
//-----------------------------------------------------------------------
void SceneManager::destroyEntity(v1::Entity *e)
{
    destroyMovableObject(e);
}
//-----------------------------------------------------------------------
void SceneManager::destroyAllEntities(void)
{
    destroyAllMovableObjectsByType(v1::EntityFactory::FACTORY_TYPE_NAME);
}
//-----------------------------------------------------------------------
Rectangle2D* SceneManager::createRectangle2D( SceneMemoryMgrTypes sceneType )
{
    // delegate to factory implementation
    return static_cast<Rectangle2D*>( createMovableObject( Rectangle2DFactory::FACTORY_TYPE_NAME,
                                                           &mEntityMemoryManager[sceneType], 0 ) );
}
//-----------------------------------------------------------------------
void SceneManager::destroyRectangle2D( Rectangle2D *rect )
{
    destroyMovableObject( rect );
}
//-----------------------------------------------------------------------
void SceneManager::destroyAllRectangle2D(void)
{
    destroyAllMovableObjectsByType(Rectangle2DFactory::FACTORY_TYPE_NAME);
}
//-----------------------------------------------------------------------
void SceneManager::_addCompositorTexture( IdString name, TextureGpu *tex )
{
    mCompositorTextures.push_back( CompositorTexture( name, tex ) );
}
//-----------------------------------------------------------------------
void SceneManager::_removeCompositorTextures( size_t from )
{
    mCompositorTextures.erase( mCompositorTextures.begin() + from, mCompositorTextures.end() );
}
//-----------------------------------------------------------------------
SkeletonInstance* SceneManager::createSkeletonInstance( const SkeletonDef *skeletonDef )
{
    return mSkeletonAnimationManager.createSkeletonInstance( skeletonDef, mNumWorkerThreads );
}
//-----------------------------------------------------------------------
void SceneManager::destroySkeletonInstance( SkeletonInstance *skeletonInstance )
{
    mSkeletonAnimationManager.destroySkeletonInstance( skeletonInstance );
}
//-----------------------------------------------------------------------
void SceneManager::_removeSkeletonDef( const SkeletonDef *skeletonDef )
{
    mSkeletonAnimationManager.removeSkeletonDef( skeletonDef );
}
//-----------------------------------------------------------------------
void SceneManager::destroyAllBillboardSets(void)
{
    destroyAllMovableObjectsByType(v1::BillboardSetFactory::FACTORY_TYPE_NAME);
}
//-----------------------------------------------------------------------
ManualObject * SceneManager::createManualObject( SceneMemoryMgrTypes sceneType )
{
    return static_cast<ManualObject*>(
        createMovableObject(ManualObjectFactory::FACTORY_TYPE_NAME, &mEntityMemoryManager[sceneType]) );
}
//-----------------------------------------------------------------------
void SceneManager::destroyManualObject(ManualObject* obj)
{
    destroyMovableObject(obj);
}
//-----------------------------------------------------------------------
void SceneManager::destroyAllManualObjects(void)
{
    destroyAllMovableObjectsByType(ManualObjectFactory::FACTORY_TYPE_NAME);
}
//-----------------------------------------------------------------------
v1::BillboardChain* SceneManager::createBillboardChain()
{
    return static_cast<v1::BillboardChain*>( createMovableObject(
                                                 v1::BillboardChainFactory::FACTORY_TYPE_NAME,
                                                 &mEntityMemoryManager[SCENE_DYNAMIC]) );
}
//-----------------------------------------------------------------------
void SceneManager::destroyBillboardChain(v1::BillboardChain* obj)
{
    destroyMovableObject(obj);
}
//-----------------------------------------------------------------------
void SceneManager::destroyAllBillboardChains(void)
{
    destroyAllMovableObjectsByType(v1::BillboardChainFactory::FACTORY_TYPE_NAME);
}
//-----------------------------------------------------------------------
v1::RibbonTrail* SceneManager::createRibbonTrail()
{
    return static_cast<v1::RibbonTrail*>( createMovableObject( v1::RibbonTrailFactory::FACTORY_TYPE_NAME,
                                                               &mEntityMemoryManager[SCENE_DYNAMIC]) );
}
//-----------------------------------------------------------------------
void SceneManager::destroyRibbonTrail(v1::RibbonTrail* obj)
{
    destroyMovableObject(obj);
}
//-----------------------------------------------------------------------
void SceneManager::destroyAllRibbonTrails(void)
{
    destroyAllMovableObjectsByType(v1::RibbonTrailFactory::FACTORY_TYPE_NAME);
}
//-----------------------------------------------------------------------
ParticleSystem* SceneManager::createParticleSystem(const String& templateName)
{
    NameValuePairList params;
    params["templateName"] = templateName;
    
    return static_cast<ParticleSystem*>( createMovableObject(ParticleSystemFactory::FACTORY_TYPE_NAME,
                                                             &mEntityMemoryManager[SCENE_DYNAMIC],
                                                             &params) );
}
//-----------------------------------------------------------------------
ParticleSystem* SceneManager::createParticleSystem( size_t quota, const String& group )
{
    NameValuePairList params;
    params["quota"] = StringConverter::toString(quota);
    params["resourceGroup"] = group;
    
    return static_cast<ParticleSystem*>( createMovableObject(ParticleSystemFactory::FACTORY_TYPE_NAME,
                                                             &mEntityMemoryManager[SCENE_DYNAMIC],
                                                             &params) );
}
//-----------------------------------------------------------------------
void SceneManager::destroyParticleSystem(ParticleSystem* obj)
{
    destroyMovableObject(obj);
}
//-----------------------------------------------------------------------
void SceneManager::destroyAllParticleSystems(void)
{
    destroyAllMovableObjectsByType(ParticleSystemFactory::FACTORY_TYPE_NAME);
}
//-----------------------------------------------------------------------
void SceneManager::clearScene( bool deleteIndestructibleToo, bool reattachCameras )
{
    destroyAllStaticGeometry();
    destroyAllMovableObjects();

    // Clear root node of all children
    for( int i=0; i<NUM_SCENE_MEMORY_MANAGER_TYPES; ++i )
    {
        SceneMemoryMgrTypes currentMgrType = static_cast<SceneMemoryMgrTypes>( i );
        getRootSceneNode(currentMgrType)->removeAllChildren();
        getRootSceneNode(currentMgrType)->detachAllObjects();
    }

    SceneNodeList newSceneNodeList;

    // Delete all SceneNodes, except root that is
    for (SceneNodeList::iterator i = mSceneNodes.begin();
        i != mSceneNodes.end(); ++i)
    {
        if( deleteIndestructibleToo || !(*i)->getIndestructibleByClearScene() )
            OGRE_DELETE *i;
        else
            newSceneNodeList.push_back( *i );
    }
    mSceneNodes.clear();
    mAutoTrackingSceneNodes.clear();
    
    // Clear animations
    destroyAllAnimations();

    if (mRenderQueue)
        mRenderQueue->clear();

    {
        //Add back to the mSceneNodes list the indestructible nodes.
        SceneNodeList::const_iterator itor = newSceneNodeList.begin();
        SceneNodeList::const_iterator end  = newSceneNodeList.end();

        while( itor != end )
        {
            SceneNode *sceneNode = *itor;
            sceneNode->mGlobalIndex = mSceneNodes.size();
            mSceneNodes.push_back( sceneNode );
            ++itor;
        }
    }

    if( mSky )
        mSceneRoot[SCENE_STATIC]->attachObject( mSky );

    if( mRadialDensityMask )
        mSceneRoot[SCENE_STATIC]->attachObject( mRadialDensityMask->getRectangle() );

    if( reattachCameras )
    {
        //Reattach all cameras to the root scene node
        CameraList::const_iterator itor = mCameras.begin();
        CameraList::const_iterator end  = mCameras.end();

        while( itor != end )
        {
            Camera *camera = *itor;
            if( !camera->isAttached() )
                mSceneRoot[camera->isStatic()]->attachObject( camera );
            ++itor;
        }
    }
}
//-----------------------------------------------------------------------
SceneNode* SceneManager::createSceneNodeImpl( SceneNode *parent, NodeMemoryManager *nodeMemoryManager )
{
    SceneNode *retVal = OGRE_NEW SceneNode( Id::generateNewId<Node>(), this,
                                            nodeMemoryManager, parent );
	if( nodeMemoryManager->getMemoryManagerType() == SCENE_STATIC )
        notifyStaticDirty( retVal );
    return retVal;
}
//-----------------------------------------------------------------------
TagPoint* SceneManager::createTagPointImpl( SceneNode *parent, NodeMemoryManager *nodeMemoryManager )
{
    TagPoint *retVal = OGRE_NEW TagPoint( Id::generateNewId<Node>(), this,
                                          nodeMemoryManager, parent );
    return retVal;
}
//-----------------------------------------------------------------------
TagPoint* SceneManager::_createTagPoint( SceneNode *parent, NodeMemoryManager *nodeMemoryManager )
{
    TagPoint* sn = createTagPointImpl( parent, nodeMemoryManager );
    mSceneNodes.push_back( sn );
    sn->mGlobalIndex = mSceneNodes.size() - 1;
    return sn;
}
//-----------------------------------------------------------------------
TagPoint* SceneManager::createTagPoint(void)
{
    TagPoint* sn = createTagPointImpl( (SceneNode*)0, &mNodeMemoryManager[SCENE_DYNAMIC] );
    mSceneNodes.push_back( sn );
    sn->mGlobalIndex = mSceneNodes.size() - 1;
    return sn;
}
//-----------------------------------------------------------------------
SceneNode* SceneManager::_createSceneNode( SceneNode *parent, NodeMemoryManager *nodeMemoryManager )
{
    SceneNode* sn = createSceneNodeImpl( parent, nodeMemoryManager );
    mSceneNodes.push_back( sn );
    sn->mGlobalIndex = mSceneNodes.size() - 1;
    return sn;
}
//-----------------------------------------------------------------------
SceneNode* SceneManager::createSceneNode( SceneMemoryMgrTypes sceneType )
{
    SceneNode* sn = createSceneNodeImpl( (SceneNode*)0, &mNodeMemoryManager[sceneType] );
    mSceneNodes.push_back( sn );
    sn->mGlobalIndex = mSceneNodes.size() - 1;
    return sn;
}
//-----------------------------------------------------------------------
void SceneManager::destroySceneNode( SceneNode* sn )
{
    if( sn->mGlobalIndex >= mSceneNodes.size() || sn != *(mSceneNodes.begin() + sn->mGlobalIndex) )
    {
        OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "SceneNode ID: " +
            StringConverter::toString( sn->getId() ) + ", named '" + sn->getName() +
            "' had it's mGlobalIndex out of date!!! (or the SceneNode wasn't "
            "created with this SceneManager)", "SceneManager::destroySceneNode");
    }

	{
		// For any scene nodes which are tracking this node
		// (or if this node is a tracker), remove its entry.
		AutoTrackingSceneNodeVec::iterator itor = mAutoTrackingSceneNodes.begin();
		AutoTrackingSceneNodeVec::iterator end  = mAutoTrackingSceneNodes.end();

		while( itor != end )
		{
			if( itor->source == sn || itor->target == sn )
			{
				itor = efficientVectorRemove( mAutoTrackingSceneNodes, itor );
				end  = mAutoTrackingSceneNodes.end();
			}
			else
			{
				++itor;
			}
		}
	}

    SceneNodeList::iterator itor = mSceneNodes.begin() + sn->mGlobalIndex;

    // detach from parent (don't do this in destructor since bulk destruction
    // behaves differently)
    Node *parentNode = sn->getParent();
    if( parentNode )
    {
        parentNode->removeChild( sn );
    }
    itor = efficientVectorRemove( mSceneNodes, itor );
    OGRE_DELETE sn;
    sn = 0;

    //The node that was at the end got swapped and has now a different index
    if( itor != mSceneNodes.end() )
        (*itor)->mGlobalIndex = itor - mSceneNodes.begin();
}
//-----------------------------------------------------------------------
SceneNode* SceneManager::getRootSceneNode( SceneMemoryMgrTypes sceneType )
{
    return mSceneRoot[sceneType];
}
//-----------------------------------------------------------------------
SceneNode* SceneManager::getSceneNode( IdType id )
{
    // Use reverse iterators, as we assume the most used nodes are the last ones created.
    SceneNode *retVal = 0;
    SceneNodeList::reverse_iterator ritor = mSceneNodes.rbegin();
    SceneNodeList::reverse_iterator rend  = mSceneNodes.rend();

    while( ritor != rend && (*ritor)->getId() != id )
        ++ritor;

    if( ritor != mSceneNodes.rend() )
        retVal = *ritor;

    return retVal;
}
//-----------------------------------------------------------------------
const SceneNode* SceneManager::getSceneNode( IdType id ) const
{
    // Use reverse iterators, as we assume the most used nodes are the last ones created.
    SceneNode const *retVal = 0;
    SceneNodeList::const_reverse_iterator ritor = mSceneNodes.rbegin();
    SceneNodeList::const_reverse_iterator rend  = mSceneNodes.rend();

    while( ritor != rend && (*ritor)->getId() != id )
        ++ritor;

    if( ritor != mSceneNodes.rend() )
        retVal = *ritor;

    return retVal;
}
//-----------------------------------------------------------------------
SceneManager::SceneNodeList SceneManager::findSceneNodes( const String& name ) const
{
    SceneNodeList nodes;
    for( SceneNodeList::const_iterator itor = mSceneNodes.begin(); itor != mSceneNodes.end(); ++itor )
    {
        SceneNode* node = *itor;
        if( node->getName() == name )
            nodes.push_back( node );
    }

    return nodes;
}
//-----------------------------------------------------------------------
void SceneManager::registerSceneNodeListener( SceneNode *sceneNode )
{
    SceneNodeList::iterator itor = std::lower_bound( mSceneNodesWithListeners.begin(),
                                                     mSceneNodesWithListeners.end(), sceneNode );
    mSceneNodesWithListeners.insert( itor, sceneNode );
}
//-----------------------------------------------------------------------
void SceneManager::unregisterSceneNodeListener( SceneNode *sceneNode )
{
    SceneNodeList::iterator itor = std::lower_bound( mSceneNodesWithListeners.begin(),
                                                     mSceneNodesWithListeners.end(), sceneNode );
    assert( itor != mSceneNodesWithListeners.end() && *itor == sceneNode );
    if( itor != mSceneNodesWithListeners.end() && *itor == sceneNode )
        mSceneNodesWithListeners.erase( itor );
}
//-----------------------------------------------------------------------
void SceneManager::setSky( bool bEnabled, SkyMethod skyMethod, TextureGpu *texture )
{
    MaterialManager &materialManager = MaterialManager::getSingleton();

    if( bEnabled )
    {
        if( !mSky )
        {
            mSky = OGRE_NEW Rectangle2D( Id::generateNewId<MovableObject>(),
                                         &mEntityMemoryManager[SCENE_STATIC], this );
            // We can't use BT_DYNAMIC_* because the scene may be rendered from multiple cameras
            // in the same frame, and dynamic supports only one set of values per frame
            mSky->initialize( BT_DEFAULT,
                              Rectangle2D::GeometryFlagQuad | Rectangle2D::GeometryFlagNormals );
            mSky->setGeometry( -Ogre::Vector2::UNIT_SCALE, Ogre::Vector2( 2.0f ) );
            mSky->setRenderQueueGroup( 212u ); // Render after most stuff
            mSceneRoot[SCENE_STATIC]->attachObject( mSky );
        }

        const IdType sceneManagerId = getId();

        const char *baseMatNames[] =
        {
            "Ogre/Sky/Cubemap",
            "Ogre/Sky/Equirectangular",
        };

        const String matName = baseMatNames[skyMethod] + StringConverter::toString( sceneManagerId );

        mSkyMaterial = materialManager.getByName( matName );
        if( !mSkyMaterial )
        {
            mSkyMaterial = materialManager.getByName(
                baseMatNames[skyMethod], ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
            if( !mSkyMaterial )
            {
                OGRE_EXCEPT( Exception::ERR_FILE_NOT_FOUND,
                             "To use the sky bundle the resources included in Samples/Media/Common",
                             "SceneManager::setSky" );
            }
            mSkyMaterial->load();
            mSkyMaterial = mSkyMaterial->clone( matName );
        }

        if( skyMethod == SkyCubemap && texture->getTextureType() != TextureTypes::TypeCube )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "SkyCubemap method requires a cubemap texture",
                         "SceneManager::setSky" );
        }
        else if( skyMethod == SkyEquirectangular &&
                 texture->getInternalTextureType() != TextureTypes::Type2DArray )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "SkyEquirectangular method requires a 2D texture",
                         "SceneManager::setSky" );
        }

        Pass *pass = mSkyMaterial->getTechnique( 0 )->getPass( 0 );

        if( skyMethod == SkyEquirectangular )
        {
            GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();
            psParams->setNamedConstant( "sliceIdx", (float)texture->getInternalSliceStart() );
        }

        TextureUnitState *tu = pass->getTextureUnitState( 0 );
        // Ensure we don't accidentally clone the texture (minimize
        // mem consumption by being Automatic Batching aware)
        tu->setAutomaticBatching( texture->hasAutomaticBatching() );
        tu->setTexture( texture );

        mSky->setMaterial( mSkyMaterial );
    }
    else
    {
        if( mSky )
            mSky->detachFromParent();
        OGRE_DELETE mSky;
        mSky = 0;
        if( mSkyMaterial )
        {
            materialManager.remove( mSkyMaterial );
            mSkyMaterial.reset();
        }
    }
}
//-----------------------------------------------------------------------
void SceneManager::setSky( bool bEnabled, SkyMethod skyMethod, const String &texName,
                           const String &resourceGroup )
{
    TextureGpu *texture = 0;

    if( bEnabled )
    {
        TextureGpuManager *textureManager = mDestRenderSystem->getTextureGpuManager();
        if( skyMethod == SkyCubemap )
        {
            texture = textureManager->createOrRetrieveTexture(
                texName, GpuPageOutStrategy::Discard, CommonTextureTypes::EnvMap, resourceGroup );
        }
        else
        {
            // Do not generate mipmaps for equirectangular maps.
            // They cause artifacts when it wraps around due to the sudden change in UV
            texture = textureManager->createOrRetrieveTexture(
                texName, texName, GpuPageOutStrategy::Discard,
                TextureFlags::AutomaticBatching | TextureFlags::PrefersLoadingFromFileAsSRGB,
                TextureTypes::Type2D, resourceGroup, 0 );
        }

        texture->scheduleTransitionTo( GpuResidency::Resident );
    }

    setSky( bEnabled, skyMethod, texture );
}
//-----------------------------------------------------------------------
void SceneManager::setRadialDensityMask( bool bEnabled, const float radius[3] )
{
    if( bEnabled )
    {
        if( !mRadialDensityMask )
        {
            mRadialDensityMask =
                    OGRE_NEW RadialDensityMask( this, radius, Root::getSingleton().getHlmsManager() );
        }
        else
        {
            mRadialDensityMask->setNewRadius( radius );
        }
    }
    else
    {
        OGRE_DELETE mRadialDensityMask;
        mRadialDensityMask = 0;
    }
}
//-----------------------------------------------------------------------
void SceneManager::setForward3D( bool bEnable, uint32 width, uint32 height, uint32 numSlices,
                                 uint32 lightsPerCell, float minDistance, float maxDistance )
{
    OGRE_DELETE mForwardPlusSystem;
    mForwardPlusSystem = 0;
    mForwardPlusImpl = 0;

    if( bEnable )
    {
        mForwardPlusSystem = OGRE_NEW Forward3D( width, height, numSlices, lightsPerCell,
                                                 minDistance, maxDistance, this );

        if( mDestRenderSystem )
            mForwardPlusSystem->_changeRenderSystem( mDestRenderSystem );
    }
}
//-----------------------------------------------------------------------
void SceneManager::setForwardClustered( bool bEnable, uint32 width, uint32 height, uint32 numSlices,
                                        uint32 lightsPerCell, uint32 decalsPerCell,
                                        uint32 cubemapProbesPerCel,
                                        float minDistance, float maxDistance )
{
    OGRE_DELETE mForwardPlusSystem;
    mForwardPlusSystem = 0;
    mForwardPlusImpl = 0;

    if( bEnable )
    {
        mForwardPlusSystem = OGRE_NEW ForwardClustered( width, height, numSlices, lightsPerCell,
                                                        decalsPerCell, cubemapProbesPerCel,
                                                        minDistance, maxDistance, this );

        if( mDestRenderSystem )
            mForwardPlusSystem->_changeRenderSystem( mDestRenderSystem );
    }
}
//-----------------------------------------------------------------------
void SceneManager::_setForwardPlusEnabledInPass( bool bEnable )
{
    if( bEnable )
        mForwardPlusImpl = mForwardPlusSystem;
    else
        mForwardPlusImpl = 0;
}
//-----------------------------------------------------------------------
void SceneManager::setBuildLegacyLightList( bool bEnable )
{
    mBuildLegacyLightList = bEnable;
}
//-----------------------------------------------------------------------
void SceneManager::_setPrePassMode( PrePassMode mode, const TextureGpuVec &prepassTextures,
                                    TextureGpu *prepassDepthTexture,
                                    TextureGpu *ssrTexture )
{
    mPrePassMode = mode;
    mPrePassTextures = prepassTextures;
    mPrePassDepthTexture = prepassDepthTexture;
    mSsrTexture = ssrTexture;
}
//-----------------------------------------------------------------------
void SceneManager::_setRefractions( TextureGpu *depthTextureNoMsaa, TextureGpu *refractionsTexture )
{
    mPassDepthTextureNoMsaa = depthTextureNoMsaa;
    mRefractionsTexture = refractionsTexture;
}
//-----------------------------------------------------------------------
void SceneManager::setDecalsDiffuse( TextureGpu *tex )
{
    if( tex )
        tex->scheduleTransitionTo( GpuResidency::Resident );
    mDecalsDiffuseTex = tex;
}
//-----------------------------------------------------------------------
void SceneManager::setDecalsNormals( TextureGpu *tex )
{
    if( tex )
        tex->scheduleTransitionTo( GpuResidency::Resident );
    mDecalsNormalsTex = tex;
}
//-----------------------------------------------------------------------
void SceneManager::setDecalsEmissive( TextureGpu *tex )
{
    if( tex )
        tex->scheduleTransitionTo( GpuResidency::Resident );
    mDecalsEmissiveTex = tex;
}
//-----------------------------------------------------------------------
bool SceneManager::isDecalsDiffuseEmissiveMerged(void) const
{
    bool retVal = false;
    if( mDecalsDiffuseTex && mDecalsEmissiveTex &&
        (mDecalsDiffuseTex == mDecalsEmissiveTex ||
         (mDecalsDiffuseTex->hasAutomaticBatching() &&
          mDecalsDiffuseTex->getTexturePool() == mDecalsEmissiveTex->getTexturePool())) )
    {
        retVal = true;
    }
    return retVal;
}
//-----------------------------------------------------------------------
bool SceneManager::_collectForwardPlusObjects( const Camera *camera )
{
    bool retVal = false;
    if( mNumDecals > 0 || mNumCubemapProbes > 0 )
    {
        OgreProfile( "Forward+ Decal collect" );

        assert( !mForwardPlusMemoryManagerCullList.empty() );
        mVisibleObjects.swap( mTmpVisibleObjects );
        CullFrustumRequest cullRequest( 0, 255,
                                        mIlluminationStage == IRS_RENDER_TO_TEXTURE, false, false,
                                        &mForwardPlusMemoryManagerCullList, camera, camera );
        fireCullFrustumThreads( cullRequest );
        mVisibleObjects.swap( mTmpVisibleObjects );

        retVal = true;
    }

    return retVal;
}
//-----------------------------------------------------------------------
void SceneManager::_cullPhase01( Camera *cullCamera, Camera *renderCamera, const Camera *lodCamera,
                                 uint8 firstRq, uint8 lastRq, bool reuseCullData )
{
    OgreProfileGroup( "Frustum Culling", OGREPROF_CULLING );

    Root::getSingleton()._pushCurrentSceneManager(this);
    mAutoParamDataSource->setCurrentSceneManager(this);

    mCamerasInProgress = CamerasInProgress(renderCamera,cullCamera, lodCamera);

    if( !reuseCullData )
    {
        // Lock scene graph mutex, no more changes until we're ready to render
        OGRE_LOCK_MUTEX(sceneGraphMutex);

        // Prepare render queue for receiving new objects
        /*{
            OgreProfileGroup("prepareRenderQueue", OGREPROF_GENERAL);
            prepareRenderQueue();
        }*/

        mRenderQueue->clear();

        // Invert vertex winding?
        if( cullCamera->isReflected() )
            mDestRenderSystem->setInvertVertexWinding(true);
        else
            mDestRenderSystem->setInvertVertexWinding(false);

        {
            //TODO: Remove this hacky listener (mostly needed by OverlayManager)
            //Overlays REQUIRED THIS to be called before RenderQueue::renderPassPrepare.
            OgreProfileGroup( "RenderQueue Listeners (i.e. Overlays)", OGREPROF_RENDERING );
            for( uint8 i=firstRq; i<lastRq; ++i )
                fireRenderQueueStarted( i, BLANKSTRING );
        }

        if( mIlluminationStage != IRS_RENDER_TO_TEXTURE && mForwardPlusImpl )
            mForwardPlusImpl->collectLights( cullCamera );

        mRenderQueue->renderPassPrepare( mIlluminationStage == IRS_RENDER_TO_TEXTURE, false );

        if (mFindVisibleObjects)
        {
            assert( !mEntitiesMemoryManagerCulledList.empty() );

            // Quick way of reducing overhead/stress on VisibleObjectsBoundsInfo
            // calculation (lastRq can be up to 255)
            uint8 realFirstRq= firstRq;
            uint8 realLastRq = 0;
            {
                ObjectMemoryManagerVec::const_iterator itor = mEntitiesMemoryManagerCulledList.begin();
                ObjectMemoryManagerVec::const_iterator end  = mEntitiesMemoryManagerCulledList.end();
                while( itor != end )
                {
                    realFirstRq = std::min<uint8>( realFirstRq, (*itor)->_getTotalRenderQueues() );
                    realLastRq  = std::max<uint8>( realLastRq, (*itor)->_getTotalRenderQueues() );
                    ++itor;
                }

                //clamp RQ values to the real RQ range
                realFirstRq = std::min(realLastRq, std::max(realFirstRq, firstRq));
                realLastRq = std::min(realLastRq, std::max(realFirstRq, lastRq));
            }

            cullCamera->_setRenderedRqs( realFirstRq, realLastRq );

            CullFrustumRequest cullRequest( realFirstRq, realLastRq,
                                            mIlluminationStage == IRS_RENDER_TO_TEXTURE, true, false,
                                            &mEntitiesMemoryManagerCulledList, cullCamera, lodCamera );
            fireCullFrustumThreads( cullRequest );
        }
    } // end lock on scene graph mutex
    else
    {
        {
            //TODO: Remove this hacky listener (mostly needed by OverlayManager)
            //Overlays REQUIRED THIS to be called before RenderQueue::renderPassPrepare.
            OgreProfileGroup( "RenderQueue Listeners (i.e. Overlays)", OGREPROF_RENDERING );
            for( uint8 i=firstRq; i<lastRq; ++i )
                fireRenderQueueStarted( i, BLANKSTRING );
        }

        if( mIlluminationStage != IRS_RENDER_TO_TEXTURE && mForwardPlusImpl )
            mForwardPlusImpl->collectLights( cullCamera );

        mRenderQueue->renderPassPrepare( mIlluminationStage == IRS_RENDER_TO_TEXTURE, false );
    }

    Root::getSingleton()._popCurrentSceneManager(this);
}
//-----------------------------------------------------------------------
void SceneManager::_renderPhase02(Camera* camera, const Camera *lodCamera,
                                  uint8 firstRq, uint8 lastRq, bool includeOverlays)
{
    OgreProfileGroup( "Rendering", OGREPROF_RENDERING );

    Root::getSingleton()._pushCurrentSceneManager(this);
    mAutoParamDataSource->setCurrentSceneManager(this);

    // reset light hash so even if light list is the same, we refresh the content every frame
    LightList emptyLightList;
    useLights(emptyLightList, 0);

    {
        // Lock scene graph mutex, no more changes until we're ready to render
        OGRE_LOCK_MUTEX(sceneGraphMutex);

        mCamerasInProgress.renderingCamera = camera;
        mCamerasInProgress.lodCamera = lodCamera;

        // Tell params about camera
        mAutoParamDataSource->setCurrentCamera(camera);
        // Set autoparams for finite dir light extrusion
        mAutoParamDataSource->setShadowDirLightExtrusionDistance(mShadowDirLightExtrudeDist);

        // Tell params about current ambient light
        mAutoParamDataSource->setAmbientLightColour( mAmbientLight, mAmbientLightHemisphereDir );

        // Set camera window clipping planes (if any)
        if (mDestRenderSystem->getCapabilities()->hasCapability(RSC_USER_CLIP_PLANES))
        {
            mDestRenderSystem->resetClipPlanes();
            if (camera->isWindowSet())
            {
                mDestRenderSystem->setClipPlanes(camera->getWindowPlanes());
            }
        }

        if( mSky && mIlluminationStage != IRS_RENDER_TO_TEXTURE )
        {
            const Vector3 *corners = camera->getWorldSpaceCorners();
            const Vector3 &cameraPos = camera->getDerivedPosition();

            const Real invFarPlane = 1.0f / camera->getFarClipDistance();
            Vector3 cameraDirs[4];
            cameraDirs[0] = ( corners[5] - cameraPos ) * invFarPlane;
            cameraDirs[1] = ( corners[6] - cameraPos ) * invFarPlane;
            cameraDirs[2] = ( corners[4] - cameraPos ) * invFarPlane;
            cameraDirs[3] = ( corners[7] - cameraPos ) * invFarPlane;

            mSky->setNormals( cameraDirs[0], cameraDirs[1], cameraDirs[2], cameraDirs[3] );
            mSky->update();
        }

        if( mRadialDensityMask && mIlluminationStage != IRS_RENDER_TO_TEXTURE &&
            isUsingInstancedStereo() )
        {
            mRadialDensityMask->update( mDestRenderSystem->getCurrentRenderViewports() );
        }

        if (mFindVisibleObjects)
        {
            OgreProfileGroup( "V1 Renderable update", OGREPROF_RENDERING );

            //mVisibleObjects should be filled in phase 01
            VisibleObjectsPerThreadArray::const_iterator it = mVisibleObjects.begin();
            VisibleObjectsPerThreadArray::const_iterator en = mVisibleObjects.end();

            bool casterPass = mIlluminationStage == IRS_RENDER_TO_TEXTURE;

            firePreFindVisibleObjects( mCurrentViewport0 );

            //Some v1 Renderables may bind their own GL buffers during _updateRenderQueue,
            //thus we need to be sure the correct VAO is bound.
            mDestRenderSystem->_startLegacyV1Rendering();

            while( it != en )
            {
                for( uint8 i=firstRq; i<lastRq; ++i )
                {
                    MovableObject::MovableObjectArray::const_iterator itor = (*it)[i].begin();
                    MovableObject::MovableObjectArray::const_iterator end  = (*it)[i].end();

                    while( itor != end )
                    {
                        //Only v1 are added here. v2 objects have already
                        //been added in parallel in phase 01.
                        (*itor)->_updateRenderQueue( mRenderQueue, camera, lodCamera );

                        RenderableArray::const_iterator itRend = (*itor)->mRenderables.begin();
                        RenderableArray::const_iterator enRend = (*itor)->mRenderables.end();

                        while( itRend != enRend )
                        {
                            mRenderQueue->addRenderableV1( i, casterPass, *itRend, *itor );
                            ++itRend;
                        }

                        ++itor;
                    }
                }

                ++it;
            }

            firePostFindVisibleObjects( mCurrentViewport0 );
        }
    } // end lock on scene graph mutex

    mDestRenderSystem->_resetMetrics();

    // Set initial camera state
    mDestRenderSystem->_setProjectionMatrix( Matrix4::IDENTITY );

    mCachedViewMatrix = mCamerasInProgress.renderingCamera->getViewMatrix(true);

    setViewMatrix(mCachedViewMatrix);

    mDestRenderSystem->executeRenderPassDescriptorDelayedActions();

    // Render scene content
    {
        //OgreProfileGroup("_renderVisibleObjects", OGREPROF_RENDERING);
        //_renderVisibleObjects();
        OgreProfileGroup( "RenderQueue", OGREPROF_RENDERING );
        //TODO: RENDER QUEUE Add Dual Paraboloid mapping
        mRenderQueue->render( mDestRenderSystem, firstRq, lastRq,
                              mIlluminationStage == IRS_RENDER_TO_TEXTURE, false );
    }

    //Restore vertex winding
    mDestRenderSystem->setInvertVertexWinding(false);

    // Notify camera of vis faces
    camera->_notifyRenderedFaces( mDestRenderSystem->getMetrics().mFaceCount );

    // Notify camera of vis batches
    camera->_notifyRenderedBatches( mDestRenderSystem->getMetrics().mBatchCount );

    Root::getSingleton()._popCurrentSceneManager(this);
}
//-----------------------------------------------------------------------
void SceneManager::cullLights( Camera *camera, Light::LightTypes startType,
                               Light::LightTypes endType, LightArray &outLights )
{
    mVisibleObjects.swap( mTmpVisibleObjects );
    CullFrustumRequest cullRequest( startType, endType, false, false, true,
                                    &mLightsMemoryManagerCulledList, camera, camera );
    fireCullFrustumThreads( cullRequest );

    outLights.clear();

    VisibleObjectsPerThreadArray::const_iterator it = mVisibleObjects.begin();
    VisibleObjectsPerThreadArray::const_iterator en = mVisibleObjects.end();

    while( it != en )
    {
        for( uint8 i=startType; i<endType; ++i )
        {
            MovableObject::MovableObjectArray::const_iterator itor = (*it)[i].begin();
            MovableObject::MovableObjectArray::const_iterator end  = (*it)[i].end();

            Light * const *lightBegin  = reinterpret_cast<Light * const *>( itor );
            Light * const *lightEnd    = reinterpret_cast<Light * const *>( end );

            outLights.appendPOD( lightBegin, lightEnd );
        }

        ++it;
    }

    mVisibleObjects.swap( mTmpVisibleObjects );
}
//-----------------------------------------------------------------------
void SceneManager::_frameEnded(void)
{
    mRenderQueue->frameEnded();
}
//-----------------------------------------------------------------------
void SceneManager::_setDestinationRenderSystem(RenderSystem* sys)
{
    mDestRenderSystem = sys;
    const uint32 maxBoundVps = mDestRenderSystem->getMaxBoundViewports();
    Viewport *currVps = mDestRenderSystem->getCurrentRenderViewports();
    Viewport *vp[16];
    for( size_t i=0; i<maxBoundVps; ++i )
        vp[i] = &currVps[i];
    setViewports( vp, maxBoundVps );

    if( mForwardPlusSystem )
        mForwardPlusSystem->_changeRenderSystem( sys );
}
//-----------------------------------------------------------------------
void SceneManager::_releaseManualHardwareResources()
{
    // release hardware resources inside all movable objects
    OGRE_LOCK_MUTEX(mMovableObjectCollectionMapMutex);
    for(MovableObjectCollectionMap::iterator ci = mMovableObjectCollectionMap.begin(),
        ci_end = mMovableObjectCollectionMap.end(); ci != ci_end; ++ci)
    {
        MovableObjectCollection* coll = ci->second;
        OGRE_LOCK_MUTEX(coll->mutex);
        for(MovableObjectVec::iterator i = coll->movableObjects.begin(), i_end = coll->movableObjects.end(); i != i_end; ++i)
            (*i)->_releaseManualHardwareResources();
    }

    if(mForwardPlusSystem)
        mForwardPlusSystem->_releaseManualHardwareResources();
}
//-----------------------------------------------------------------------
void SceneManager::_restoreManualHardwareResources()
{
    // restore hardware resources inside all movable objects
    OGRE_LOCK_MUTEX(mMovableObjectCollectionMapMutex);
    for(MovableObjectCollectionMap::iterator ci = mMovableObjectCollectionMap.begin(),
        ci_end = mMovableObjectCollectionMap.end(); ci != ci_end; ++ci)
    {
        MovableObjectCollection* coll = ci->second;
        OGRE_LOCK_MUTEX(coll->mutex);
        for(MovableObjectVec::iterator i = coll->movableObjects.begin(), i_end = coll->movableObjects.end(); i != i_end; ++i)
            (*i)->_restoreManualHardwareResources();
    }
    mStaticEntitiesDirty = true; // mObjectData.mWorldAabb could be corrupted as part of reinitialization
}
//-----------------------------------------------------------------------
void SceneManager::prepareWorldGeometry(const String& filename)
{
    // This default implementation cannot handle world geometry
    OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
        "World geometry is not supported by the generic SceneManager.",
        "SceneManager::prepareWorldGeometry");
}
//-----------------------------------------------------------------------
void SceneManager::prepareWorldGeometry(DataStreamPtr& stream, 
    const String& typeName)
{
    // This default implementation cannot handle world geometry
    OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
        "World geometry is not supported by the generic SceneManager.",
        "SceneManager::prepareWorldGeometry");
}

//-----------------------------------------------------------------------
void SceneManager::setWorldGeometry(const String& filename)
{
    // This default implementation cannot handle world geometry
    OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
        "World geometry is not supported by the generic SceneManager.",
        "SceneManager::setWorldGeometry");
}
//-----------------------------------------------------------------------
void SceneManager::setWorldGeometry(DataStreamPtr& stream, 
    const String& typeName)
{
    // This default implementation cannot handle world geometry
    OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
        "World geometry is not supported by the generic SceneManager.",
        "SceneManager::setWorldGeometry");
}
//-----------------------------------------------------------------------
void SceneManager::notifyStaticAabbDirty( MovableObject *movableObject )
{
    mStaticEntitiesDirty = true;
    movableObject->_notifyStaticDirty();
}
//-----------------------------------------------------------------------
void SceneManager::notifyStaticDirty( Node *node )
{
    assert( node->isStatic() );

    mStaticMinDepthLevelDirty = std::min<uint16>( mStaticMinDepthLevelDirty, node->getDepthLevel() );
    node->_notifyStaticDirty();
}
//-----------------------------------------------------------------------
void SceneManager::updateAllAnimationsThread( size_t threadIdx )
{
    SkeletonAnimManagerVec::const_iterator it = mSkeletonAnimManagerCulledList.begin();
    SkeletonAnimManagerVec::const_iterator en = mSkeletonAnimManagerCulledList.end();

    while( it != en )
    {
        SkeletonAnimManager::BySkeletonDefList::iterator itByDef = (*it)->bySkeletonDefs.begin();
        SkeletonAnimManager::BySkeletonDefList::iterator enByDef = (*it)->bySkeletonDefs.end();

        while( itByDef != enByDef )
        {
            FastArray<SkeletonInstance*>::iterator itor = itByDef->skeletons.begin() +
                                                                    itByDef->threadStarts[threadIdx];
            FastArray<SkeletonInstance*>::iterator end  = itByDef->skeletons.begin() +
                                                                    itByDef->threadStarts[threadIdx+1];
            while( itor != end )
            {
                (*itor)->update();
                ++itor;
            }

            if( !itByDef->skeletons.empty() )
                updateAnimationTransforms( *itByDef, threadIdx );

            ++itByDef;
        }

        ++it;
    }
}
//-----------------------------------------------------------------------
void SceneManager::updateAnimationTransforms( BySkeletonDef &bySkeletonDef, size_t threadIdx )
{
    assert( !bySkeletonDef.skeletons.empty() );

#ifndef NDEBUG
    BoneTransform _hiddenTransform;
#endif

    //Unlike regular nodes, bones' number of parents and children is known before hand, thus
    //when magicDistance = 25; we update the root bones of the first 25 skeletons, then the children
    //of those bones, and so on; then we move to the next 25 skeletons. This behavior slightly
    //improves performance since the parent data is still hot in the cache when updating its child.
    //The value of 25 is arbitrary.
    const size_t magicDistance = 25;

    const SkeletonDef *skeletonDef                          = bySkeletonDef.skeletonDef;
    const SkeletonDef::DepthLevelInfoVec &depthLevelInfo    = skeletonDef->getDepthLevelInfo();

    size_t firstIdx = bySkeletonDef.threadStarts[threadIdx];
    size_t lastIdx  = std::min( firstIdx + magicDistance, bySkeletonDef.threadStarts[threadIdx+1] );
    while( firstIdx != lastIdx )
    {
        SkeletonInstance *first = *(bySkeletonDef.skeletons.begin() + firstIdx);
        SkeletonInstance *last  = *(bySkeletonDef.skeletons.begin() + lastIdx - 1);

        const TransformArray &firstTransforms   = first->_getTransformArray();
        const TransformArray &lastTransforms    = last->_getTransformArray();
        ArrayMatrixAf4x3 const *reverseBind     = skeletonDef->getReverseBindPose().get();

        assert( bySkeletonDef.boneMemoryManager.getNumDepths() == firstTransforms.size() );

        for( size_t i=0; i<firstTransforms.size(); ++i )
        {
            size_t numNodes = lastTransforms[i].mOwner - firstTransforms[i].mOwner +
                                lastTransforms[i].mIndex +
                                depthLevelInfo[i].numBonesInLevel;
            assert( numNodes <= bySkeletonDef.boneMemoryManager.getFirstNode( _hiddenTransform, i ) );

            Bone::updateAllTransforms( numNodes, firstTransforms[i], reverseBind,
                                        depthLevelInfo[i].numBonesInLevel );
            reverseBind += (depthLevelInfo[i].numBonesInLevel - 1 + ARRAY_PACKED_REALS) / ARRAY_PACKED_REALS;
        }

        firstIdx = lastIdx;
        lastIdx += magicDistance;
        lastIdx = std::min( lastIdx, bySkeletonDef.threadStarts[threadIdx+1] );
    }
}
//-----------------------------------------------------------------------
void SceneManager::updateAllAnimations()
{
    mRequestType = UPDATE_ALL_ANIMATIONS;
    fireWorkerThreadsAndWait();
}
//-----------------------------------------------------------------------
void SceneManager::updateAllTransformsThread( const UpdateTransformRequest &request, size_t threadIdx )
{
    Transform t( request.t );
    const size_t toAdvance = std::min( threadIdx * request.numNodesPerThread,
                                        request.numTotalNodes );

    //Prevent going out of bounds (usually in the last threadIdx, or
    //when there are less nodes than ARRAY_PACKED_REALS
    const size_t numNodes = std::min( request.numNodesPerThread, request.numTotalNodes - toAdvance );
    t.advancePack( toAdvance / ARRAY_PACKED_REALS );

    Node::updateAllTransforms( numNodes, t );
}
//-----------------------------------------------------------------------
void SceneManager::updateAllTransforms()
{
    mRequestType = UPDATE_ALL_TRANSFORMS;
    NodeMemoryManagerVec::const_iterator it = mNodeMemoryManagerUpdateList.begin();
    NodeMemoryManagerVec::const_iterator en = mNodeMemoryManagerUpdateList.end();

    while( it != en )
    {
        NodeMemoryManager *nodeMemoryManager = *it;
        const size_t numDepths = nodeMemoryManager->getNumDepths();

        size_t start = nodeMemoryManager->getMemoryManagerType() == SCENE_STATIC ?
                                                    mStaticMinDepthLevelDirty : 0;

        //Start from the zeroth level (root) unless static (start from first dirty)
        for( size_t i=start; i<numDepths; ++i )
        {
            Transform t;
            const size_t numNodes = nodeMemoryManager->getFirstNode( t, i );

            //nodesPerThread must be multiple of ARRAY_PACKED_REALS
            size_t nodesPerThread = ( numNodes + (mNumWorkerThreads-1) ) / mNumWorkerThreads;
            nodesPerThread        = ( (nodesPerThread + ARRAY_PACKED_REALS - 1) / ARRAY_PACKED_REALS ) *
                                    ARRAY_PACKED_REALS;

            if( numNodes )
            {
                //Send them to worker threads. We need to go depth by depth because
                //we may depend on parents which could be processed by different threads.
                mUpdateTransformRequest = UpdateTransformRequest( t, nodesPerThread, numNodes );
                fireWorkerThreadsAndWait();
                //Node::updateAllTransforms( numNodes, t );
            }
        }

        ++it;
    }

    //Call all listeners
    SceneNodeList::const_iterator itor = mSceneNodesWithListeners.begin();
    SceneNodeList::const_iterator end  = mSceneNodesWithListeners.end();

    while( itor != end )
    {
        (*itor)->getListener()->nodeUpdated( *itor );
        ++itor;
    }
}
//-----------------------------------------------------------------------
void SceneManager::updateAllTagPoints()
{
    NodeMemoryManagerVec::const_iterator it = mTagPointNodeMemoryManagerUpdateList.begin();
    NodeMemoryManagerVec::const_iterator en = mTagPointNodeMemoryManagerUpdateList.end();

    while( it != en )
    {
        NodeMemoryManager *nodeMemoryManager = *it;
        const size_t numDepths = nodeMemoryManager->getNumDepths();

        //Start from the first level (not root) unless static (start from first dirty)
        for( size_t i=0; i<numDepths; ++i )
        {
            mRequestType = i == 0 ? UPDATE_ALL_BONE_TO_TAG_TRANSFORMS :
                                    UPDATE_ALL_TAG_ON_TAG_TRANSFORMS;

            Transform t;
            const size_t numNodes = nodeMemoryManager->getFirstNode( t, i );

            //nodesPerThread must be multiple of ARRAY_PACKED_REALS
            size_t nodesPerThread = ( numNodes + (mNumWorkerThreads-1) ) / mNumWorkerThreads;
            nodesPerThread        = ( (nodesPerThread + ARRAY_PACKED_REALS - 1) / ARRAY_PACKED_REALS ) *
                                    ARRAY_PACKED_REALS;

            if( numNodes )
            {
                //Send them to worker threads. We need to go depth by depth because
                //we may depend on parents which could be processed by different threads.
                mUpdateTransformRequest = UpdateTransformRequest( t, nodesPerThread, numNodes );
                fireWorkerThreadsAndWait();
            }
        }

        ++it;
    }
}
//-----------------------------------------------------------------------
void SceneManager::updateAllTransformsBoneToTagThread( const UpdateTransformRequest &request,
                                                       size_t threadIdx )
{
    Transform t( request.t );
    const size_t toAdvance = std::min( threadIdx * request.numNodesPerThread,
                                        request.numTotalNodes );

    //Prevent going out of bounds (usually in the last threadIdx, or
    //when there are less nodes than ARRAY_PACKED_REALS
    const size_t numNodes = std::min( request.numNodesPerThread, request.numTotalNodes - toAdvance );
    t.advancePack( toAdvance / ARRAY_PACKED_REALS );

    TagPoint::updateAllTransformsBoneToTag( numNodes, t );
}
//-----------------------------------------------------------------------
void SceneManager::updateAllTransformsTagOnTagThread( const UpdateTransformRequest &request,
                                                      size_t threadIdx )
{
    Transform t( request.t );
    const size_t toAdvance = std::min( threadIdx * request.numNodesPerThread,
                                        request.numTotalNodes );

    //Prevent going out of bounds (usually in the last threadIdx, or
    //when there are less nodes than ARRAY_PACKED_REALS
    const size_t numNodes = std::min( request.numNodesPerThread, request.numTotalNodes - toAdvance );
    t.advancePack( toAdvance / ARRAY_PACKED_REALS );

    TagPoint::updateAllTransformsTagOnTag( numNodes, t );
}
//-----------------------------------------------------------------------
void SceneManager::updateAllBoundsThread( const ObjectMemoryManagerVec &objectMemManager, size_t threadIdx )
{
    ObjectMemoryManagerVec::const_iterator it = objectMemManager.begin();
    ObjectMemoryManagerVec::const_iterator en = objectMemManager.end();

    while( it != en )
    {
        ObjectMemoryManager *memoryManager = *it;
        const size_t numRenderQueues = memoryManager->getNumRenderQueues();

        for( size_t i=0; i<numRenderQueues; ++i )
        {
            ObjectData objData;
            const size_t totalObjs = memoryManager->getFirstObjectData( objData, i );

            //Distribute the work evenly across all threads (not perfect), taking into
            //account we need to distribute in multiples of ARRAY_PACKED_REALS
            size_t numObjs  = ( totalObjs + (mNumWorkerThreads-1) ) / mNumWorkerThreads;
            numObjs         = ( (numObjs + ARRAY_PACKED_REALS - 1) / ARRAY_PACKED_REALS ) *
                                ARRAY_PACKED_REALS;

            const size_t toAdvance = std::min( threadIdx * numObjs, totalObjs );

            //Prevent going out of bounds (usually in the last threadIdx, or
            //when there are less entities than ARRAY_PACKED_REALS
            numObjs = std::min( numObjs, totalObjs - toAdvance );
            objData.advancePack( toAdvance / ARRAY_PACKED_REALS );

            MovableObject::updateAllBounds( numObjs, objData );
        }

        ++it;
    }
}
//-----------------------------------------------------------------------
void SceneManager::updateAllBounds( const ObjectMemoryManagerVec &objectMemManager )
{
    mUpdateBoundsRequest    = &objectMemManager;
    mRequestType            = UPDATE_ALL_BOUNDS;
    fireWorkerThreadsAndWait();
}
//-----------------------------------------------------------------------
void SceneManager::updateAllLodsThread( const UpdateLodRequest &request, size_t threadIdx )
{
    LodStrategy *lodStrategy = LodStrategyManager::getSingleton().getDefaultStrategy();

    const Camera *lodCamera = request.lodCamera;
    ObjectMemoryManagerVec::const_iterator it = request.objectMemManager->begin();
    ObjectMemoryManagerVec::const_iterator en = request.objectMemManager->end();

    while( it != en )
    {
        ObjectMemoryManager *memoryManager = *it;
        const size_t numRenderQueues = memoryManager->getNumRenderQueues();

        size_t firstRq = std::min<size_t>( request.firstRq, numRenderQueues );
        size_t lastRq  = std::min<size_t>( request.lastRq,  numRenderQueues );

        for( size_t i=firstRq; i<lastRq; ++i )
        {
            ObjectData objData;
            const size_t totalObjs = memoryManager->getFirstObjectData( objData, i );

            //Distribute the work evenly across all threads (not perfect), taking into
            //account we need to distribute in multiples of ARRAY_PACKED_REALS
            size_t numObjs  = ( totalObjs + (mNumWorkerThreads-1) ) / mNumWorkerThreads;
            numObjs         = ( (numObjs + ARRAY_PACKED_REALS - 1) / ARRAY_PACKED_REALS ) *
                                ARRAY_PACKED_REALS;

            const size_t toAdvance = std::min( threadIdx * numObjs, totalObjs );

            //Prevent going out of bounds (usually in the last threadIdx, or
            //when there are less entities than ARRAY_PACKED_REALS
            numObjs = std::min( numObjs, totalObjs - toAdvance );
            objData.advancePack( toAdvance / ARRAY_PACKED_REALS );

            lodStrategy->lodUpdateImpl( numObjs, objData, lodCamera, request.lodBias );
        }

        ++it;
    }
}
//-----------------------------------------------------------------------
void SceneManager::updateAllLods( const Camera *lodCamera, Real lodBias, uint8 firstRq, uint8 lastRq )
{
    mRequestType        = UPDATE_ALL_LODS;
    mUpdateLodRequest   = UpdateLodRequest( firstRq, lastRq, &mEntitiesMemoryManagerCulledList,
                                             lodCamera, lodCamera, lodBias );

    mUpdateLodRequest.camera->getFrustumPlanes();
    mUpdateLodRequest.lodCamera->getFrustumPlanes();

    fireWorkerThreadsAndWait();
}
//-----------------------------------------------------------------------
void SceneManager::cullFrustum( const CullFrustumRequest &request, size_t threadIdx )
{
    VisibleObjectsPerRq &visibleObjectsPerRq = *(mVisibleObjects.begin() + threadIdx);
    {
        visibleObjectsPerRq.resize( 255 );
        VisibleObjectsPerRq::iterator itor = visibleObjectsPerRq.begin();
        VisibleObjectsPerRq::iterator end  = visibleObjectsPerRq.end();

        while( itor != end )
        {
            itor->clear();
            ++itor;
        }
    }

    const Camera *camera    = request.camera;
    const Camera *lodCamera = request.lodCamera;

    const uint32 visibilityMask = request.cullingLights ?
                (camera->getLastViewport()->getLightVisibilityMask() & mLightMask) :
                ((camera->getLastViewport()->getVisibilityMask() & this->getVisibilityMask()) |
                 (camera->getLastViewport()->getVisibilityMask() &
                                    ~VisibilityFlags::RESERVED_VISIBILITY_FLAGS));

    ObjectMemoryManagerVec::const_iterator it = request.objectMemManager->begin();
    ObjectMemoryManagerVec::const_iterator en = request.objectMemManager->end();

    while( it != en )
    {
        ObjectMemoryManager *memoryManager = *it;
        const size_t numRenderQueues = memoryManager->getNumRenderQueues();

        size_t firstRq = std::min<size_t>( request.firstRq, numRenderQueues );
        size_t lastRq  = std::min<size_t>( request.lastRq,  numRenderQueues );

        for( size_t i=firstRq; i<lastRq; ++i )
        {
            MovableObject::MovableObjectArray &outVisibleObjects = *(visibleObjectsPerRq.begin() + i);

            ObjectData objData;
            const size_t totalObjs = memoryManager->getFirstObjectData( objData, i );

            //Distribute the work evenly across all threads (not perfect), taking into
            //account we need to distribute in multiples of ARRAY_PACKED_REALS
            size_t numObjs  = ( totalObjs + (mNumWorkerThreads-1) ) / mNumWorkerThreads;
            numObjs         = ( (numObjs + ARRAY_PACKED_REALS - 1) / ARRAY_PACKED_REALS ) *
                                ARRAY_PACKED_REALS;

            const size_t toAdvance = std::min( threadIdx * numObjs, totalObjs );

            //Prevent going out of bounds (usually in the last threadIdx, or
            //when there are less entities than ARRAY_PACKED_REALS
            numObjs = std::min( numObjs, totalObjs - toAdvance );
            objData.advancePack( toAdvance / ARRAY_PACKED_REALS );

            MovableObject::cullFrustum( numObjs, objData, camera, visibilityMask,
                                        outVisibleObjects, lodCamera );

            if( mRenderQueue->getRenderQueueMode(i) == RenderQueue::FAST && request.addToRenderQueue )
            {
                //V2 meshes can be added to the render queue in parallel
                bool casterPass = request.casterPass;
                MovableObject::MovableObjectArray::const_iterator itor = outVisibleObjects.begin();
                MovableObject::MovableObjectArray::const_iterator end  = outVisibleObjects.end();

                while( itor != end )
                {
                    RenderableArray::const_iterator itRend = (*itor)->mRenderables.begin();
                    RenderableArray::const_iterator enRend = (*itor)->mRenderables.end();

                    while( itRend != enRend )
                    {
                        mRenderQueue->addRenderableV2( threadIdx, i, casterPass, *itRend, *itor );
                        ++itRend;
                    }
                    ++itor;
                }

                outVisibleObjects.clear();
            }
        }

        ++it;
    }
}
//-----------------------------------------------------------------------
inline bool OrderLightByShadowCastThenId( const Light *_l, const Light *_r )
{
    if( _l->getCastShadows() && !_r->getCastShadows() )
        return true;
    if( !_l->getCastShadows() && _r->getCastShadows() )
        return false;

    return _l->getId() < _r->getId();
}

void quickSortDirectionalLights( LightListInfo &_globalLightList, const size_t startIdx, const size_t n )
{
    if( n < 2 )
        return;

    LightListInfo lightList;
    lightList.lights.swap( _globalLightList.lights );
    lightList.visibilityMask = _globalLightList.visibilityMask;
    lightList.boundingSphere = _globalLightList.boundingSphere;

    size_t i, j;
    Light const *p;

    p = lightList.lights[startIdx + (n >> 1)];
    for( i = 0, j = n - 1;; ++i, --j )
    {
        while( OrderLightByShadowCastThenId( lightList.lights[startIdx + i], p ) )
            i++;
        while( OrderLightByShadowCastThenId( p, lightList.lights[startIdx + j] ) )
            j--;
        if( i >= j )
            break;

        std::swap( lightList.lights[startIdx + i],          lightList.lights[startIdx + j] );
        std::swap( lightList.visibilityMask[startIdx + i],  lightList.visibilityMask[startIdx + j] );
        std::swap( lightList.boundingSphere[startIdx + i],  lightList.boundingSphere[startIdx + j] );
    }

    quickSortDirectionalLights( lightList, startIdx, i );
    quickSortDirectionalLights( lightList, startIdx + i, n - i );

    //Nullify the pointers to prevent freeing them in the destructor (we do not own them).
    lightList.lights.swap( _globalLightList.lights );
    lightList.visibilityMask = 0;
    lightList.boundingSphere = 0;
}

void SceneManager::buildLightList()
{
    mGlobalLightList.lights.clear();

    {
        //Copy directional lights first. They aren't many. And we need them sorted:
        //  1. Shadow casting lights first
        //  2. Then ordered by ID (order of creation)
        ObjectMemoryManagerVec::const_iterator it = mLightsMemoryManagerCulledList.begin();
        ObjectMemoryManagerVec::const_iterator en = mLightsMemoryManagerCulledList.end();

        const uint32 visibilityMask = mLightMask;

        size_t idx = 0;

        while( it != en )
        {
            ObjectMemoryManager *objMemoryManager = *it;

            //Cull the lights against all cameras to build the list of visible lights.
            ObjectData objData;

            const size_t numRenderQueues = objMemoryManager->getNumRenderQueues();

            if( numRenderQueues )
            {
                const size_t totalObjs = objMemoryManager->getFirstObjectData( objData, 0 );

                for( size_t i=0; i<totalObjs; i += ARRAY_PACKED_REALS )
                {
                    for( size_t j=0; j<ARRAY_PACKED_REALS; ++j )
                    {
                        const bool isVisible = (objData.mVisibilityFlags[j] &
                                                VisibilityFlags::LAYER_VISIBILITY) != 0;
                        if( isVisible && objData.mVisibilityFlags[j] & visibilityMask )
                        {
                            mGlobalLightList.visibilityMask[idx] = objData.mVisibilityFlags[j];
                            mGlobalLightList.boundingSphere[idx] = Sphere(
                                        objData.mWorldAabb->mCenter.getAsVector3( j ),
                                        objData.mWorldRadius[j] );
                            assert( dynamic_cast<Light*>( objData.mOwner[j] ) );
                            mGlobalLightList.lights.push_back( static_cast<Light*>(
                                                                   objData.mOwner[j] ) );

                            ++idx;
                        }
                    }

                    objData.advanceFrustumPack();
                }
            }

            ++it;
        }

        quickSortDirectionalLights( mGlobalLightList, 0, mGlobalLightList.lights.size() );
    }

    //Perform frustum culling against the lights to build the light list,
    //but first, calculate the startLightIdx for each thread.
    size_t accumStartLightIdx = mGlobalLightList.lights.size();
    for( size_t threadIdx=0; threadIdx<mNumWorkerThreads; ++threadIdx )
    {
        size_t totalObjsInThread = 0;
        ObjectMemoryManagerVec::const_iterator it = mLightsMemoryManagerCulledList.begin();
        ObjectMemoryManagerVec::const_iterator en = mLightsMemoryManagerCulledList.end();

        while( it != en )
        {
            ObjectMemoryManager *objMemoryManager = *it;
            const size_t numRenderQueues = objMemoryManager->getNumRenderQueues();

            //Cull the lights against all cameras to build the list of visible lights.
            for( size_t i=1; i<numRenderQueues; ++i )
            {
                ObjectData objData;
                const size_t totalObjs = objMemoryManager->getFirstObjectData( objData, i );

                //Distribute the work evenly across all threads (not perfect), taking into
                //account we need to distribute in multiples of ARRAY_PACKED_REALS
                size_t numObjs  = ( totalObjs + (mNumWorkerThreads-1) ) / mNumWorkerThreads;
                numObjs         = ( (numObjs + ARRAY_PACKED_REALS - 1) / ARRAY_PACKED_REALS ) *
                                    ARRAY_PACKED_REALS;

                const size_t toAdvance = std::min( threadIdx * numObjs, totalObjs );

                //Prevent going out of bounds (usually in the last threadIdx, or
                //when there are less entities than ARRAY_PACKED_REALS
                numObjs = std::min( numObjs, totalObjs - toAdvance );

                totalObjsInThread += numObjs;
            }

            ++it;
        }

        mBuildLightListRequestPerThread[threadIdx].startLightIdx = accumStartLightIdx;
        accumStartLightIdx += totalObjsInThread;
    }

    if( accumStartLightIdx == mGlobalLightList.lights.size() && !mBuildLegacyLightList )
    {
        //All of the lights were directional. We're done. Avoid the sync point with worker threads.
        return;
    }

    mRequestType = BUILD_LIGHT_LIST01;
    {
        //This is where I figuratively kill whoever made mutable variables inside a
        //const function, silencing a race condition: Update the frustum planes now
        //in case they weren't up to date.
        FrustumVec::const_iterator itor = mVisibleCameras.begin();
        FrustumVec::const_iterator end  = mVisibleCameras.end();

        while( itor != end )
        {
            (*itor)->getFrustumPlanes();
            ++itor;
        }

        itor = mCubeMapCameras.begin();
        end  = mCubeMapCameras.end();

        while( itor != end )
        {
            (*itor)->getFrustumPlanes();
            ++itor;
        }
    }

    if( mForceMainThread )
        updateWorkerThreadImpl( 0 );
    else
    {
        mWorkerThreadsBarrier->sync(); //Fire threads
        mWorkerThreadsBarrier->sync(); //Wait them to complete
    }

    //Now merge the results into a single list.

    size_t dstOffset = mGlobalLightList.lights.size(); //Start where the directional lights end
    for( size_t i=0; i<mNumWorkerThreads; ++i )
    {
        mGlobalLightList.lights.appendPOD( mGlobalLightListPerThread[i].begin(),
                                           mGlobalLightListPerThread[i].end() );

        size_t numCollectedLights = mGlobalLightListPerThread[i].size();

        size_t srcOffset = mBuildLightListRequestPerThread[i].startLightIdx;

        if( dstOffset != srcOffset )
        {
            //Make it contiguous
            memmove( mGlobalLightList.visibilityMask + dstOffset,
                     mGlobalLightList.visibilityMask + srcOffset,
                     sizeof( uint32 ) * numCollectedLights );
            memmove( mGlobalLightList.boundingSphere + dstOffset,
                     mGlobalLightList.boundingSphere + srcOffset,
                     sizeof( Sphere ) * numCollectedLights );
        }

        dstOffset += numCollectedLights;
    }

    if( mBuildLegacyLightList )
    {
        //Now fire the threads again, to build the per-MovableObject lists
        mRequestType = BUILD_LIGHT_LIST02;
        if( mForceMainThread )
            updateWorkerThreadImpl( 0 );
        else
        {
            mWorkerThreadsBarrier->sync(); //Fire threads
            mWorkerThreadsBarrier->sync(); //Wait them to complete
        }
    }
}
//-----------------------------------------------------------------------
void SceneManager::buildLightListThread01( const BuildLightListRequest &buildLightListRequest,
                                           size_t threadIdx )
{
    LightArray &outVisibleLights = *(mGlobalLightListPerThread.begin() + threadIdx);
    outVisibleLights.clear();

    LightListInfo threadLocalLightList;
    threadLocalLightList.lights.swap( outVisibleLights );
    threadLocalLightList.boundingSphere = mGlobalLightList.boundingSphere +
                                            buildLightListRequest.startLightIdx;
    threadLocalLightList.visibilityMask = mGlobalLightList.visibilityMask +
                                            buildLightListRequest.startLightIdx;

    ObjectMemoryManagerVec::const_iterator it = mLightsMemoryManagerCulledList.begin();
    ObjectMemoryManagerVec::const_iterator en = mLightsMemoryManagerCulledList.end();

    while( it != en )
    {
        ObjectMemoryManager *objMemoryManager = *it;
        const size_t numRenderQueues = objMemoryManager->getNumRenderQueues();

        //Cull the lights against all cameras to build the list of visible lights.
        for( size_t i=1; i<numRenderQueues; ++i )
        {
            ObjectData objData;
            const size_t totalObjs = objMemoryManager->getFirstObjectData( objData, i );

            //Distribute the work evenly across all threads (not perfect), taking into
            //account we need to distribute in multiples of ARRAY_PACKED_REALS
            size_t numObjs  = ( totalObjs + (mNumWorkerThreads-1) ) / mNumWorkerThreads;
            numObjs         = ( (numObjs + ARRAY_PACKED_REALS - 1) / ARRAY_PACKED_REALS ) *
                                ARRAY_PACKED_REALS;

            const size_t toAdvance = std::min( threadIdx * numObjs, totalObjs );

            //Prevent going out of bounds (usually in the last threadIdx, or
            //when there are less entities than ARRAY_PACKED_REALS
            numObjs = std::min( numObjs, totalObjs - toAdvance );
            objData.advancePack( toAdvance / ARRAY_PACKED_REALS );

            Light::cullLights( numObjs, objData, mLightMask, threadLocalLightList,
                               mVisibleCameras, mCubeMapCameras );
        }

        ++it;
    }

    threadLocalLightList.lights.swap( outVisibleLights );
    //Nullify the pointers to prevent freeing them in the destructor (we do not own them).
    threadLocalLightList.visibilityMask = 0;
    threadLocalLightList.boundingSphere = 0;
}
//-----------------------------------------------------------------------
void SceneManager::buildLightListThread02( size_t threadIdx )
{
    //Global light list built. Now build a per-movable object light list
    ObjectMemoryManagerVec::const_iterator it = mEntitiesMemoryManagerCulledList.begin();
    ObjectMemoryManagerVec::const_iterator en = mEntitiesMemoryManagerCulledList.end();
    while( it != en )
    {
        ObjectMemoryManager *objMemoryManager = *it;
        const size_t numRenderQueues = objMemoryManager->getNumRenderQueues();

        for( size_t i=0; i<numRenderQueues; ++i )
        {
            ObjectData objData;
            const size_t totalObjs = objMemoryManager->getFirstObjectData( objData, i );

            //Distribute the work evenly across all threads (not perfect), taking into
            //account we need to distribute in multiples of ARRAY_PACKED_REALS
            size_t numObjs  = ( totalObjs + (mNumWorkerThreads-1) ) / mNumWorkerThreads;
            numObjs         = ( (numObjs + ARRAY_PACKED_REALS - 1) / ARRAY_PACKED_REALS ) *
                                ARRAY_PACKED_REALS;

            const size_t toAdvance = std::min( threadIdx * numObjs, totalObjs );

            //Prevent going out of bounds (usually in the last threadIdx, or
            //when there are less entities than ARRAY_PACKED_REALS
            numObjs = std::min( numObjs, totalObjs - toAdvance );
            objData.advancePack( toAdvance / ARRAY_PACKED_REALS );

            MovableObject::buildLightList( numObjs, objData, mGlobalLightList );
        }

        ++it;
    }
}
//-----------------------------------------------------------------------
void SceneManager::highLevelCull()
{
    mNodeMemoryManagerUpdateList.clear();
    mEntitiesMemoryManagerCulledList.clear();
    mEntitiesMemoryManagerUpdateList.clear();
    mLightsMemoryManagerCulledList.clear();
    mForwardPlusMemoryManagerCullList.clear();
    mSkeletonAnimManagerCulledList.clear();
    mTagPointNodeMemoryManagerUpdateList.clear();

    mNodeMemoryManagerUpdateList.push_back( &mNodeMemoryManager[SCENE_DYNAMIC] );
    mEntitiesMemoryManagerCulledList.push_back( &mEntityMemoryManager[SCENE_DYNAMIC] );
    mEntitiesMemoryManagerCulledList.push_back( &mEntityMemoryManager[SCENE_STATIC] );
    mEntitiesMemoryManagerUpdateList.push_back( &mEntityMemoryManager[SCENE_DYNAMIC] );
    mEntitiesMemoryManagerUpdateList.push_back( &mForwardPlusMemoryManager[SCENE_DYNAMIC] );
    mLightsMemoryManagerCulledList.push_back( &mLightMemoryManager );
    mForwardPlusMemoryManagerCullList.push_back( &mForwardPlusMemoryManager[SCENE_DYNAMIC] );
    mForwardPlusMemoryManagerCullList.push_back( &mForwardPlusMemoryManager[SCENE_STATIC] );
    mSkeletonAnimManagerCulledList.push_back( &mSkeletonAnimationManager );
    mTagPointNodeMemoryManagerUpdateList.push_back( &mTagPointNodeMemoryManager );

    if( mStaticEntitiesDirty )
    {
        //Entities have changed
        mEntitiesMemoryManagerUpdateList.push_back( &mEntityMemoryManager[SCENE_STATIC] );
        mEntitiesMemoryManagerUpdateList.push_back( &mForwardPlusMemoryManager[SCENE_STATIC] );
    }

    if( mStaticMinDepthLevelDirty < mNodeMemoryManager[SCENE_STATIC].getNumDepths() )
    {
        //Nodes have changed
        mNodeMemoryManagerUpdateList.push_back( &mNodeMemoryManager[SCENE_STATIC] );
    }
}
//-----------------------------------------------------------------------
void SceneManager::updateSceneGraph()
{
    //TODO: Enable auto tracking again, first manually update the tracked scene nodes for correct math. (dark_sylinc)
    // Update scene graph for this camera (can happen multiple times per frame)
    /*{
        // Auto-track nodes
        AutoTrackingSceneNodes::iterator atsni, atsniend;
        atsniend = mAutoTrackingSceneNodes.end();
        for (atsni = mAutoTrackingSceneNodes.begin(); atsni != atsniend; ++atsni)
        {
            (*atsni)->_autoTrack();
        }
        // Auto-track camera if required
        camera->_autoTrack();
    }*/

    OgreProfileGroup( "updateSceneGraph", OGREPROF_GENERAL );

    // Update controllers 
    ControllerManager::getSingleton().updateAllControllers();

    highLevelCull();
    _applySceneAnimations();
    updateAllTransforms();
    updateAllAnimations();
    updateAllTagPoints();
    updateAllBounds( mEntitiesMemoryManagerUpdateList );
    updateAllBounds( mLightsMemoryManagerCulledList );

    {
        // Auto-track nodes
        AutoTrackingSceneNodeVec::const_iterator itor = mAutoTrackingSceneNodes.begin();
        AutoTrackingSceneNodeVec::const_iterator end  = mAutoTrackingSceneNodes.end();

        while( itor != end )
        {
            itor->source->lookAt( itor->target->_getDerivedPosition() + itor->offset,
                                     Node::TS_WORLD, itor->localDirection );
            itor->source->_getDerivedPositionUpdated();
            ++itor;
        }
    }

    {
        // Auto-track camera if required
        CameraList::const_iterator itor = mCameras.begin();
        CameraList::const_iterator end  = mCameras.end();
        while( itor != end )
        {
            (*itor)->_autoTrack();
            ++itor;
        }
    }

    {
        WireAabbVec::const_iterator itor = mTrackingWireAabbs.begin();
        WireAabbVec::const_iterator end  = mTrackingWireAabbs.end();

        while( itor != end )
        {
            (*itor)->_updateTracking();
            (*itor)->getParentNode()->_getFullTransformUpdated();
            (*itor)->getWorldAabbUpdated();
            ++itor;
        }
    }

    buildLightList();

    //Reset the list of render RQs for all cameras that are in a PASS_SCENE (except shadow passes)
    uint8 numRqs = 0;
    {
        ObjectMemoryManagerVec::const_iterator itor = mEntitiesMemoryManagerCulledList.begin();
        ObjectMemoryManagerVec::const_iterator end  = mEntitiesMemoryManagerCulledList.end();
        while( itor != end )
        {
            numRqs = std::max<uint8>( numRqs, (*itor)->_getTotalRenderQueues() );
            ++itor;
        }
    }

    CameraList::const_iterator itor = mCameras.begin();
    CameraList::const_iterator end  = mCameras.end();
    while( itor != end )
    {
        (*itor)->_resetRenderedRqs( numRqs );
        ++itor;
    }

    // Reset these
    mStaticMinDepthLevelDirty = std::numeric_limits<uint16>::max();
    mStaticEntitiesDirty = false;

    for( size_t i=0; i<OGRE_MAX_SIMULTANEOUS_LIGHTS; ++i )
        mAutoParamDataSource->setTextureProjector( 0, i );
}
//-----------------------------------------------------------------------
void SceneManager::renderSingleObject(Renderable* rend, const Pass* pass, 
                                      bool lightScissoringClipping, bool doLightIteration )
{
    unsigned short numMatrices;
    v1::RenderOperation ro;

    OgreProfileBeginGPUEvent("Material: " + pass->getParent()->getParent()->getName());
#if OGRE_DEBUG_MODE
    ro.srcRenderable = rend;
#endif

    GpuProgram* vprog = pass->hasVertexProgram() ? pass->getVertexProgram().get() : 0;

    bool passTransformState = true;

    if (vprog)
    {
        passTransformState = vprog->getPassTransformStates();
    }

    // Set world transformation
    numMatrices = rend->getNumWorldTransforms();
    
    if (numMatrices > 0)
    {
        rend->getWorldTransforms(mTempXform);

        if (passTransformState)
        {
            if (numMatrices > 1)
            {
                mDestRenderSystem->_setWorldMatrices(mTempXform, numMatrices);
            }
            else
            {
                mDestRenderSystem->_setWorldMatrix(*mTempXform);
            }
        }
    }
    // Issue view / projection changes if any
    useRenderableViewProjMode(rend, passTransformState);

    // mark per-object params as dirty
    mGpuParamsDirty |= (uint16)GPV_PER_OBJECT;

    if (!mSuppressRenderStateChanges)
    {
        bool passSurfaceAndLightParams = true;

        if (pass->isProgrammable())
        {
            // Tell auto params object about the renderable change
            mAutoParamDataSource->setCurrentRenderable(rend);
            // Tell auto params object about the world matrices, eliminated query from renderable again
            mAutoParamDataSource->setWorldMatrices(mTempXform, numMatrices);
            if (vprog)
            {
                passSurfaceAndLightParams = vprog->getPassSurfaceAndLightStates();
            }
        }

        // Reissue any texture gen settings which are dependent on view matrix
        Pass::ConstTextureUnitStateIterator texIter =  pass->getTextureUnitStateIterator();
        size_t unit = 0;
        while(texIter.hasMoreElements())
        {
            TextureUnitState* pTex = texIter.getNext();
            if (pTex->hasViewRelativeTextureCoordinateGeneration())
            {
                mDestRenderSystem->_setTextureUnitSettings(unit, *pTex);
            }
            ++unit;
        }

        // Sort out negative scaling
        // Assume first world matrix representative 
        if (mFlipCullingOnNegativeScale)
        {
            CullingMode cullMode = mPassCullingMode;

            if (mTempXform[0].hasNegativeScale())
            {
                switch(mPassCullingMode)
                {
                case CULL_CLOCKWISE:
                    cullMode = CULL_ANTICLOCKWISE;
                    break;
                case CULL_ANTICLOCKWISE:
                    cullMode = CULL_CLOCKWISE;
                    break;
                case CULL_NONE:
                    break;
                };
            }

#ifdef ENABLE_INCOMPATIBLE_OGRE_2_0
            // this also copes with returning from negative scale in previous render op
            // for same pass
            if (cullMode != mDestRenderSystem->_getCullingMode())
                mDestRenderSystem->_setCullingMode(cullMode);
#endif
        }

        if (doLightIteration)
        {
            // Create local light list for faster light iteration setup
            static LightList localLightList;

            // Here's where we issue the rendering operation to the render system
            // Note that we may do this once per light, therefore it's in a loop
            // and the light parameters are updated once per traversal through the
            // loop
            const LightList& rendLightList = rend->getLights();

            bool iteratePerLight = pass->getIteratePerLight();

            // deliberately unsigned in case start light exceeds number of lights
            // in which case this pass would be skipped
            int lightsLeft = 1;
            if (iteratePerLight)
            {
                lightsLeft = static_cast<int>(rendLightList.size()) - pass->getStartLight();
                // Don't allow total light count for all iterations to exceed max per pass
                if (lightsLeft > static_cast<int>(pass->getMaxSimultaneousLights()))
                {
                    lightsLeft = static_cast<int>(pass->getMaxSimultaneousLights());
                }
            }

            const LightList* pLightListToUse;
            // Start counting from the start light
            size_t lightIndex = pass->getStartLight();
            size_t depthInc = 0;

            while (lightsLeft > 0)
            {
                // Determine light list to use
                if (iteratePerLight)
                {
                    localLightList.resize(pass->getLightCountPerIteration());

                    LightList::iterator destit = localLightList.begin();
                    for (; destit != localLightList.end()
                            && lightIndex < rendLightList.size(); 
                        ++lightIndex, --lightsLeft)
                    {
                        Light const * currLight = rendLightList[lightIndex].light;

                        // Check whether we need to filter this one out
                        if ((pass->getRunOnlyForOneLightType() && 
                            pass->getOnlyLightType() != currLight->getType()) ||
                            (pass->getLightMask() & currLight->getLightMask()) == 0)
                        {
                            // Skip
                            continue;
                        }

                        *destit = rendLightList[lightIndex];
                        ++destit;
                    }
                    // Did we run out of lights before slots? e.g. 5 lights, 2 per iteration
                    if (destit != localLightList.end())
                    {
                        localLightList.erase(destit, localLightList.end());
                        lightsLeft = 0;
                    }
                    pLightListToUse = &localLightList;

                    // deal with the case where we found no lights
                    // since this is light iteration, we shouldn't render at all
                    if (pLightListToUse->empty())
                        return;
                }
                else // !iterate per light
                {
                    // Use complete light list potentially adjusted by start light
                    if (pass->getStartLight() || pass->getMaxSimultaneousLights() != OGRE_MAX_SIMULTANEOUS_LIGHTS || 
                        pass->getLightMask() != 0xFFFFFFFF)
                    {
                        // out of lights?
                        // skip manual 2nd lighting passes onwards if we run out of lights, but never the first one
                        if (pass->getStartLight() > 0 &&
                            pass->getStartLight() >= rendLightList.size())
                        {
                            break;
                        }
                        else
                        {
                            localLightList.clear();
                            LightList::const_iterator copyStart = rendLightList.begin();
                            std::advance(copyStart, pass->getStartLight());
                            // Clamp lights to copy to avoid overrunning the end of the list
                            size_t lightsCopied = 0, lightsToCopy = std::min(
                                static_cast<size_t>(pass->getMaxSimultaneousLights()), 
                                rendLightList.size() - pass->getStartLight());

                            //localLightList.insert(localLightList.begin(), 
                            //  copyStart, copyEnd);

                            // Copy lights over
                            for(LightList::const_iterator iter = copyStart; iter != rendLightList.end() && lightsCopied < lightsToCopy; ++iter)
                            {
                                if((pass->getLightMask() & iter->light->getLightMask()) != 0)
                                {
                                    localLightList.push_back(*iter);
                                    lightsCopied++;
                                }
                            }

                            pLightListToUse = &localLightList;
                        }
                    }
                    else
                    {
                        pLightListToUse = &rendLightList;
                    }
                    lightsLeft = 0;
                }

                fireRenderSingleObject(rend, pass, mAutoParamDataSource, pLightListToUse, mSuppressRenderStateChanges);

                // Do we need to update GPU program parameters?
                if (pass->isProgrammable())
                {
                    if( mCurrentShadowNode )
                    {
                        pLightListToUse = mCurrentShadowNode->setShadowMapsToPass( rend, pass,
                                                                                   mAutoParamDataSource,
                                                                                   pass->getStartLight() );
                    }

                    useLightsGpuProgram(pass, pLightListToUse);
                }
                // Do we need to update light states? 
                // Only do this if fixed-function vertex lighting applies
                if (passSurfaceAndLightParams)
                {
                    useLights(*pLightListToUse, pass->getMaxSimultaneousLights());
                }
                // optional light scissoring & clipping
                ClipResult scissored = CLIPPED_NONE;
                ClipResult clipped = CLIPPED_NONE;
                if (lightScissoringClipping && 
                    (pass->getLightScissoringEnabled() || pass->getLightClipPlanesEnabled()))
                {
                    // if there's no lights hitting the scene, then we might as 
                    // well stop since clipping cannot include anything
                    if (pLightListToUse->empty())
                        continue;

                    if (pass->getLightScissoringEnabled())
                        scissored = buildAndSetScissor(*pLightListToUse, mCamerasInProgress.renderingCamera);
                
                    if (pass->getLightClipPlanesEnabled())
                        clipped = buildAndSetLightClip(*pLightListToUse);

                    if (scissored == CLIPPED_ALL || clipped == CLIPPED_ALL)
                        continue;
                }
                // issue the render op      
                // nfz: check for gpu_multipass
                mDestRenderSystem->setCurrentPassIterationCount(pass->getPassIterationCount());
                // We might need to update the depth bias each iteration
                /*if (pass->getIterationDepthBias() != 0.0f)
                {
                    float depthBiasBase = pass->getDepthBiasConstant() + 
                        pass->getIterationDepthBias() * depthInc;
                    // depthInc deals with light iteration 
                    
                    // Note that we have to set the depth bias here even if the depthInc
                    // is zero (in which case you would think there is no change from
                    // what was set in _setPass(). The reason is that if there are
                    // multiple Renderables with this Pass, we won't go through _setPass
                    // again at the start of the iteration for the next Renderable
                    // because of Pass state grouping. So set it always

                    // Set modified depth bias right away
                    mDestRenderSystem->_setDepthBias(depthBiasBase, pass->getDepthBiasSlopeScale());

                    // Set to increment internally too if rendersystem iterates
                    mDestRenderSystem->setDeriveDepthBias(true, 
                        depthBiasBase, pass->getIterationDepthBias(), 
                        pass->getDepthBiasSlopeScale());
                }
                else
                {
                    mDestRenderSystem->setDeriveDepthBias(false);
                }*/
                depthInc += pass->getPassIterationCount();

                // Finalise GPU parameter bindings
                updateGpuProgramParameters(pass);

                rend->getRenderOperation(ro, false);

                if (rend->preRender(this, mDestRenderSystem))
                    mDestRenderSystem->_render(ro);
                rend->postRender(this, mDestRenderSystem);

                /*if (scissored == CLIPPED_SOME)
                    resetScissor();
                if (clipped == CLIPPED_SOME)
                    resetLightClip();*/
            } // possibly iterate per light
        }
        else // no automatic light processing
        {
            // Even if manually driving lights, check light type passes
            bool skipBecauseOfLightType = false;
            if (pass->getRunOnlyForOneLightType())
            {
                if( rend->getLights().size() == 1 && 
                    rend->getLights().at(0).light->getType() != pass->getOnlyLightType() )
                {
                    skipBecauseOfLightType = true;
                }
            }

            if (!skipBecauseOfLightType)
            {
                const LightList *lightList = &rend->getLights();
                fireRenderSingleObject( rend, pass, mAutoParamDataSource,
                                        lightList, mSuppressRenderStateChanges );

                // Do we need to update GPU program parameters?
                if( pass->isProgrammable() )
                {
                    if( mCurrentShadowNode )
                    {
                        lightList = mCurrentShadowNode->setShadowMapsToPass( rend, pass,
                                                                             mAutoParamDataSource,
                                                                             pass->getStartLight() );
                    }

                    useLightsGpuProgram( pass, lightList );
                }
                else if( passSurfaceAndLightParams )
                {
                    // Use manual lights if present, and not using vertex programs that don't use fixed pipeline
                    useLights( *lightList, pass->getMaxSimultaneousLights() );
                }

                // optional light scissoring
                ClipResult scissored = CLIPPED_NONE;
                ClipResult clipped = CLIPPED_NONE;
                if (lightScissoringClipping && pass->getLightScissoringEnabled())
                {
                    scissored = buildAndSetScissor( *lightList, mCamerasInProgress.renderingCamera );
                }
                if (lightScissoringClipping && pass->getLightClipPlanesEnabled())
                {
                    clipped = buildAndSetLightClip( *lightList );
                }
    
                // don't bother rendering if clipped / scissored entirely
                if (scissored != CLIPPED_ALL && clipped != CLIPPED_ALL)
                {
                    // issue the render op      
                    // nfz: set up multipass rendering
                    mDestRenderSystem->setCurrentPassIterationCount(pass->getPassIterationCount());
                    // Finalise GPU parameter bindings
                    updateGpuProgramParameters(pass);

                    rend->getRenderOperation(ro, false);

                    if (rend->preRender(this, mDestRenderSystem))
                        mDestRenderSystem->_render(ro);
                    rend->postRender(this, mDestRenderSystem);
                }
                /*if (scissored == CLIPPED_SOME)
                    resetScissor();
                if (clipped == CLIPPED_SOME)
                    resetLightClip();*/
                
            } // !skipBecauseOfLightType
        }

    }
    else // mSuppressRenderStateChanges
    {
        fireRenderSingleObject(rend, pass, mAutoParamDataSource, NULL, mSuppressRenderStateChanges);
        // Just render
        mDestRenderSystem->setCurrentPassIterationCount(1);
        if (rend->preRender(this, mDestRenderSystem))
        {
            rend->getRenderOperation(ro, false);
            try
            {
                mDestRenderSystem->_render(ro);
            }
            catch (RenderingAPIException& e)
            {
                OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                            "Exception when rendering material: " + pass->getParent()->getParent()->getName() +
                            "\nOriginal Exception description: " + e.getFullDescription() + "\n" ,
                            "SceneManager::renderSingleObject");
                
            }
        }
        rend->postRender(this, mDestRenderSystem);
    }
    
    // Reset view / projection changes if any
    resetViewProjMode(passTransformState);
    OgreProfileEndGPUEvent("Material: " + pass->getParent()->getParent()->getName());
}
//-----------------------------------------------------------------------
void SceneManager::setAmbientLight( const ColourValue &upperHemisphere,
                                    const ColourValue &lowerHemisphere,
                                    const Vector3 &hemisphereDir,
                                    Real envmapScale, uint32 envFeatures )
{
    mAmbientLight[0] = upperHemisphere;
    mAmbientLight[1] = lowerHemisphere;
    mAmbientLightHemisphereDir = hemisphereDir;
    mAmbientLightHemisphereDir.normalise();
    mAmbientLight[0].a = envmapScale;
    mEnvFeatures = envFeatures;
}
//-----------------------------------------------------------------------
void SceneManager::setSphericalHarmonics( Vector3 ambientSphericalHarmonics[9] )
{
    for( size_t i = 0u; i < 9u; ++i )
    {
        mAmbientSphericalHarmonics[i * 3u + 0u] = (float)ambientSphericalHarmonics[i].x;
        mAmbientSphericalHarmonics[i * 3u + 1u] = (float)ambientSphericalHarmonics[i].y;
        mAmbientSphericalHarmonics[i * 3u + 2u] = (float)ambientSphericalHarmonics[i].z;
    }
}
//-----------------------------------------------------------------------
ViewPoint SceneManager::getSuggestedViewpoint(bool random)
{
    // By default return the origin
    ViewPoint vp;
    vp.position = Vector3::ZERO;
    vp.orientation = Quaternion::IDENTITY;
    return vp;
}
//-----------------------------------------------------------------------
void SceneManager::setFog(FogMode mode, const ColourValue& colour, Real density, Real start, Real end)
{
    mFogMode = mode;
    mFogColour = colour;
    mFogStart = start;
    mFogEnd = end;
    mFogDensity = density;
}
//-----------------------------------------------------------------------
FogMode SceneManager::getFogMode(void) const
{
    return mFogMode;
}
//-----------------------------------------------------------------------
const ColourValue& SceneManager::getFogColour(void) const
{
    return mFogColour;
}
//-----------------------------------------------------------------------
Real SceneManager::getFogStart(void) const
{
    return mFogStart;
}
//-----------------------------------------------------------------------
Real SceneManager::getFogEnd(void) const
{
    return mFogEnd;
}
//-----------------------------------------------------------------------
Real SceneManager::getFogDensity(void) const
{
    return mFogDensity;
}
//-----------------------------------------------------------------------
v1::BillboardSet* SceneManager::createBillboardSet(unsigned int poolSize)
{
    NameValuePairList params;
    params["poolSize"] = StringConverter::toString(poolSize);
    return static_cast<v1::BillboardSet*>( createMovableObject(
                                               v1::BillboardSetFactory::FACTORY_TYPE_NAME,
                                               &mEntityMemoryManager[SCENE_DYNAMIC],
                                               &params) );
}
//-----------------------------------------------------------------------
void SceneManager::destroyBillboardSet(v1::BillboardSet* set)
{
    destroyMovableObject(set);
}
//-----------------------------------------------------------------------
void SceneManager::setDisplaySceneNodes(bool display)
{
    mDisplayNodes = display;
}
//-----------------------------------------------------------------------
v1::Animation* SceneManager::createAnimation(const String& name, Real length)
{
    OGRE_LOCK_MUTEX(mAnimationsListMutex);

    // Check name not used
    if (mAnimationsList.find(name) != mAnimationsList.end())
    {
        OGRE_EXCEPT(
            Exception::ERR_DUPLICATE_ITEM,
            "An animation with the name " + name + " already exists",
            "SceneManager::createAnimation" );
    }

    v1::Animation* pAnim = OGRE_NEW v1::Animation(name, length);
    mAnimationsList[name] = pAnim;
    return pAnim;
}
//-----------------------------------------------------------------------
v1::Animation* SceneManager::getAnimation(const String& name) const
{
    OGRE_LOCK_MUTEX(mAnimationsListMutex);

    AnimationList::const_iterator i = mAnimationsList.find(name);
    if (i == mAnimationsList.end())
    {
        OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, 
            "Cannot find animation with name " + name, 
            "SceneManager::getAnimation");
    }
    return i->second;
}
//-----------------------------------------------------------------------
bool SceneManager::hasAnimation(const String& name) const
{
    OGRE_LOCK_MUTEX(mAnimationsListMutex);
    return (mAnimationsList.find(name) != mAnimationsList.end());
}
//-----------------------------------------------------------------------
void SceneManager::destroyAnimation(const String& name)
{
    OGRE_LOCK_MUTEX(mAnimationsListMutex);

    // Also destroy any animation states referencing this animation
    mAnimationStates.removeAnimationState(name);

    AnimationList::iterator i = mAnimationsList.find(name);
    if (i == mAnimationsList.end())
    {
        OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, 
            "Cannot find animation with name " + name, 
            "SceneManager::getAnimation");
    }

    // Free memory
    OGRE_DELETE i->second;

    mAnimationsList.erase(i);

}
//-----------------------------------------------------------------------
void SceneManager::destroyAllAnimations(void)
{
    OGRE_LOCK_MUTEX(mAnimationsListMutex);
    // Destroy all states too, since they cannot reference destroyed animations
    destroyAllAnimationStates();

    AnimationList::iterator i;
    for (i = mAnimationsList.begin(); i != mAnimationsList.end(); ++i)
    {
        // destroy
        OGRE_DELETE i->second;
    }
    mAnimationsList.clear();
}
//-----------------------------------------------------------------------
v1::AnimationState* SceneManager::createAnimationState(const String& animName)
{
    // Get animation, this will throw an exception if not found
    v1::Animation* anim = getAnimation(animName);

    // Create new state
    return mAnimationStates.createAnimationState(animName, 0, anim->getLength());

}
//-----------------------------------------------------------------------
v1::AnimationState* SceneManager::getAnimationState(const String& animName) const
{
    return mAnimationStates.getAnimationState(animName);

}
//-----------------------------------------------------------------------
bool SceneManager::hasAnimationState(const String& name) const
{
    return mAnimationStates.hasAnimationState(name);
}
//-----------------------------------------------------------------------
void SceneManager::destroyAnimationState(const String& name)
{
    mAnimationStates.removeAnimationState(name);
}
//-----------------------------------------------------------------------
void SceneManager::destroyAllAnimationStates(void)
{
    mAnimationStates.removeAllAnimationStates();
}
//-----------------------------------------------------------------------
void SceneManager::_applySceneAnimations(void)
{
    // manual lock over states (extended duration required)
    OGRE_LOCK_MUTEX(mAnimationStates.OGRE_AUTO_MUTEX_NAME);

    // Iterate twice, once to reset, once to apply, to allow blending
    v1::ConstEnabledAnimationStateIterator stateIt = mAnimationStates.getEnabledAnimationStateIterator();

    while (stateIt.hasMoreElements())
    {
        const v1::AnimationState* state = stateIt.getNext();
        v1::Animation* anim = getAnimation(state->getAnimationName());

        // Reset any nodes involved
        v1::Animation::NodeTrackIterator nodeTrackIt = anim->getNodeTrackIterator();
        while(nodeTrackIt.hasMoreElements())
        {
			nodeTrackIt.getNext()->resetNodeToInitialState();
        }

        v1::Animation::OldNodeTrackIterator OldNodeTrackIt = anim->getOldNodeTrackIterator();
		while(OldNodeTrackIt.hasMoreElements())
		{
            v1::OldNode* nd = OldNodeTrackIt.getNext()->getAssociatedNode();
			if (nd)
				nd->resetToInitialState();
		}

        v1::Animation::NumericTrackIterator numTrackIt = anim->getNumericTrackIterator();
        while(numTrackIt.hasMoreElements())
        {
            const AnimableValuePtr& animPtr = numTrackIt.getNext()->getAssociatedAnimable();
            if (!animPtr.isNull())
                animPtr->resetToBaseValue();
        }
    }

    // this should allow blended animations
    stateIt = mAnimationStates.getEnabledAnimationStateIterator();
    while (stateIt.hasMoreElements())
    {
        const v1::AnimationState* state = stateIt.getNext();
        v1::Animation* anim = getAnimation(state->getAnimationName());
        // Apply the animation
        anim->apply(state->getTimePosition(), state->getWeight());
    }
}
//---------------------------------------------------------------------
void SceneManager::useRenderableViewProjMode(const Renderable* pRend, bool fixedFunction)
{
    // Check view matrix
    bool useIdentityView = pRend->getUseIdentityView();
    if (useIdentityView)
    {
        // Using identity view now, change it
        if (fixedFunction)
            setViewMatrix(Matrix4::IDENTITY);
        mGpuParamsDirty |= (uint16)GPV_GLOBAL;
        mResetIdentityView = true;
    }

    bool useIdentityProj = pRend->getUseIdentityProjection();
    if (useIdentityProj)
    {
        // Use identity projection matrix, still need to take RS depth into account.
        if (fixedFunction)
        {
            Matrix4 mat;
            mDestRenderSystem->_convertProjectionMatrix(Matrix4::IDENTITY, mat);
            mDestRenderSystem->_setProjectionMatrix(mat);
        }
        mGpuParamsDirty |= (uint16)GPV_GLOBAL;

        mResetIdentityProj = true;
    }

    
}
//---------------------------------------------------------------------
void SceneManager::resetViewProjMode(bool fixedFunction)
{
    if (mResetIdentityView)
    {
        // Coming back to normal from identity view
        if (fixedFunction)
            setViewMatrix(mCachedViewMatrix);
        mGpuParamsDirty |= (uint16)GPV_GLOBAL;

        mResetIdentityView = false;
    }
    
    if (mResetIdentityProj)
    {
        // Coming back from flat projection
        if (fixedFunction)
            mDestRenderSystem->_setProjectionMatrix( Matrix4::IDENTITY );
        mGpuParamsDirty |= (uint16)GPV_GLOBAL;

        mResetIdentityProj = false;
    }
    

}
//---------------------------------------------------------------------
void SceneManager::addRenderQueueListener(RenderQueueListener* newListener)
{
    mRenderQueueListeners.push_back(newListener);
}
//---------------------------------------------------------------------
void SceneManager::removeRenderQueueListener(RenderQueueListener* delListener)
{
    RenderQueueListenerList::iterator i, iend;
    iend = mRenderQueueListeners.end();
    for (i = mRenderQueueListeners.begin(); i != iend; ++i)
    {
        if (*i == delListener)
        {
            mRenderQueueListeners.erase(i);
            break;
        }
    }

}
//---------------------------------------------------------------------
void SceneManager::addRenderObjectListener(RenderObjectListener* newListener)
{
    mRenderObjectListeners.push_back(newListener);
}
//---------------------------------------------------------------------
void SceneManager::removeRenderObjectListener(RenderObjectListener* delListener)
{
    RenderObjectListenerList::iterator i, iend;
    iend = mRenderObjectListeners.end();
    for (i = mRenderObjectListeners.begin(); i != iend; ++i)
    {
        if (*i == delListener)
        {
            mRenderObjectListeners.erase(i);
            break;
        }
    }
}
//---------------------------------------------------------------------
void SceneManager::_setCurrentCompositorPass( CompositorPass *pass )
{
    mCurrentPass = pass;
}
//---------------------------------------------------------------------
void SceneManager::_setCurrentShadowNode( CompositorShadowNode *shadowNode, bool isReused )
{
    mCurrentShadowNode = shadowNode;
    mShadowNodeIsReused= isReused;
    if( !shadowNode )
        mShadowNodeIsReused = false;
    mAutoParamDataSource->setCurrentShadowNode( shadowNode );
}
//---------------------------------------------------------------------
bool SceneManager::isUsingInstancedStereo(void) const
{
    bool retVal = false;
    if( mCurrentPass && mCurrentPass->getType() == PASS_SCENE )
    {
        OGRE_ASSERT_HIGH( dynamic_cast<const CompositorPassSceneDef*>( mCurrentPass->getDefinition() ) );
        CompositorPassSceneDef const *passSceneDef = static_cast<const CompositorPassSceneDef*>(
                                                         mCurrentPass->getDefinition() );
        retVal = passSceneDef->mInstancedStereo;
    }
    return retVal;
}
//---------------------------------------------------------------------
void SceneManager::addListener(Listener* newListener)
{
    mListeners.push_back(newListener);
}
//---------------------------------------------------------------------
void SceneManager::removeListener(Listener* delListener)
{
    ListenerList::iterator i = std::find(mListeners.begin(), mListeners.end(), delListener);
    if (i != mListeners.end())
        mListeners.erase(i);
}
//---------------------------------------------------------------------
bool SceneManager::fireRenderQueueStarted(uint8 id, const String& invocation)
{
    RenderQueueListenerList::iterator i, iend;
    bool skip = false;

    RenderQueue *rq = mRenderQueue;

    iend = mRenderQueueListeners.end();
    for (i = mRenderQueueListeners.begin(); i != iend; ++i)
    {
        (*i)->renderQueueStarted( rq, id, invocation, skip );
    }
    return skip;
}
//---------------------------------------------------------------------
bool SceneManager::fireRenderQueueEnded(uint8 id, const String& invocation)
{
    RenderQueueListenerList::iterator i, iend;
    bool repeat = false;

    iend = mRenderQueueListeners.end();
    for (i = mRenderQueueListeners.begin(); i != iend; ++i)
    {
        (*i)->renderQueueEnded(id, invocation, repeat);
    }
    return repeat;
}
//---------------------------------------------------------------------
void SceneManager::fireRenderSingleObject(Renderable* rend, const Pass* pass,
                                           const AutoParamDataSource* source, 
                                           const LightList* pLightList, bool suppressRenderStateChanges)
{
    RenderObjectListenerList::iterator i, iend;

    iend = mRenderObjectListeners.end();
    for (i = mRenderObjectListeners.begin(); i != iend; ++i)
    {
        (*i)->notifyRenderSingleObject(rend, pass, source, pLightList, suppressRenderStateChanges);
    }
}
//---------------------------------------------------------------------
void SceneManager::fireShadowTexturesUpdated(size_t numberOfShadowTextures)
{
    ListenerList listenersCopy = mListeners;
    ListenerList::iterator i, iend;

    iend = listenersCopy.end();
    for (i = listenersCopy.begin(); i != iend; ++i)
    {
        (*i)->shadowTexturesUpdated(numberOfShadowTextures);
    }
}
//---------------------------------------------------------------------
void SceneManager::fireShadowTexturesPreCaster(const Light* light, Camera* camera, size_t iteration)
{
    ListenerList listenersCopy = mListeners;
    ListenerList::iterator i, iend;

    iend = listenersCopy.end();
    for (i = listenersCopy.begin(); i != iend; ++i)
    {
        (*i)->shadowTextureCasterPreViewProj(light, camera, iteration);
    }
}
//---------------------------------------------------------------------
void SceneManager::firePreFindVisibleObjects(Viewport* v)
{
    ListenerList listenersCopy = mListeners;
    ListenerList::iterator i, iend;

    iend = listenersCopy.end();
    for (i = listenersCopy.begin(); i != iend; ++i)
    {
        (*i)->preFindVisibleObjects(this, mIlluminationStage, v);
    }

}
//---------------------------------------------------------------------
void SceneManager::firePostFindVisibleObjects(Viewport* v)
{
    ListenerList listenersCopy = mListeners;
    ListenerList::iterator i, iend;

    iend = listenersCopy.end();
    for (i = listenersCopy.begin(); i != iend; ++i)
    {
        (*i)->postFindVisibleObjects(this, mIlluminationStage, v);
    }


}
//---------------------------------------------------------------------
void SceneManager::fireSceneManagerDestroyed()
{
    ListenerList listenersCopy = mListeners;
    ListenerList::iterator i, iend;

    iend = listenersCopy.end();
    for (i = listenersCopy.begin(); i != iend; ++i)
    {
        (*i)->sceneManagerDestroyed(this);
    }
}
//---------------------------------------------------------------------
void SceneManager::setViewports( Viewport **vp, size_t numViewports )
{
    if( numViewports >= 1u )
        mCurrentViewport0 = vp[0];
    else
        mCurrentViewport0 = 0;

    // Set viewport in render system
    if( mAutoParamDataSource )
        mAutoParamDataSource->setCurrentViewport( mCurrentViewport0 );
    if( mCurrentViewport0 )
    {
        // Set the active material scheme for this viewport
        MaterialManager::getSingleton().setActiveScheme( mCurrentViewport0->getMaterialScheme() );
    }
}
//---------------------------------------------------------------------
void SceneManager::showBoundingBoxes(bool bShow) 
{
    mShowBoundingBoxes = bShow;
}
//---------------------------------------------------------------------
bool SceneManager::getShowBoundingBoxes() const
{
    return mShowBoundingBoxes;
}
//---------------------------------------------------------------------
void SceneManager::_addAutotrackingSceneNode( SceneNode* source, SceneNode* target,
												const Vector3 &offset, const Vector3 &localDirection )
{
	_removeAutotrackingSceneNode( source );
	mAutoTrackingSceneNodes.push_back( AutoTrackingSceneNode( source, target, offset, localDirection ) );
}
//---------------------------------------------------------------------
void SceneManager::_removeAutotrackingSceneNode( SceneNode* source )
{
	AutoTrackingSceneNodeVec::iterator itor = mAutoTrackingSceneNodes.begin();
	AutoTrackingSceneNodeVec::iterator end  = mAutoTrackingSceneNodes.end();

	while( itor != end && itor->source != source )
		++itor;

	if( itor != end )
		efficientVectorRemove( mAutoTrackingSceneNodes, itor );
}
//---------------------------------------------------------------------
void SceneManager::_suppressRenderStateChanges(bool suppress)
{
    mSuppressRenderStateChanges = suppress;
}
//---------------------------------------------------------------------
const Pass* SceneManager::deriveShadowCasterPass(const Pass* pass)
{
    Pass* retPass;  
    if (!pass->getParent()->getShadowCasterMaterial().isNull())
    {
        return pass->getParent()->getShadowCasterMaterial()->getBestTechnique()->getPass(0); 
    }
    else 
    {
        retPass = mShadowTextureCustomCasterPass ? 
            mShadowTextureCustomCasterPass : mShadowCasterPlainBlackPass;
    }

    
    // Special case alpha-blended passes
    /*if ((pass->getSourceBlendFactor() == SBF_SOURCE_ALPHA &&
        pass->getDestBlendFactor() == SBF_ONE_MINUS_SOURCE_ALPHA))
        //|| pass->getAlphaRejectFunction() != CMPF_ALWAYS_PASS)
    {
        // Alpha blended passes must retain their transparency
        retPass->setAlphaRejectSettings(pass->getAlphaRejectFunction(),
            pass->getAlphaRejectValue());
        retPass->setSceneBlending(pass->getSourceBlendFactor(), pass->getDestBlendFactor());
        retPass->getParent()->getParent()->setTransparencyCastsShadows(true);

        // So we allow the texture units, but override the colour functions
        // Copy texture state, shift up one since 0 is shadow texture
        unsigned short origPassTUCount = pass->getNumTextureUnitStates();
        for (unsigned short t = 0; t < origPassTUCount; ++t)
        {
            TextureUnitState* tex;
            if (retPass->getNumTextureUnitStates() <= t)
            {
                tex = retPass->createTextureUnitState();
            }
            else
            {
                tex = retPass->getTextureUnitState(t);
            }
            // copy base state
            (*tex) = *(pass->getTextureUnitState(t));
            // override colour function
            tex->setColourOperationEx( LBX_SOURCE1, LBS_MANUAL, LBS_CURRENT, mShadowColour );

        }
        // Remove any extras
        while (retPass->getNumTextureUnitStates() > origPassTUCount)
        {
            retPass->removeTextureUnitState(origPassTUCount);
        }

    }
    else
    {
        // reset
        retPass->setSceneBlending(SBT_REPLACE);
        retPass->setAlphaRejectFunction(CMPF_ALWAYS_PASS);
        while (retPass->getNumTextureUnitStates() > 0)
        {
            retPass->removeTextureUnitState(0);
        }
    }*/

    // Does incoming pass have a custom shadow caster program?
    if (!pass->getShadowCasterVertexProgramName().empty())
    {
        // Have to merge the shadow caster vertex program in
        retPass->setVertexProgram(
            pass->getShadowCasterVertexProgramName(), false);
        const GpuProgramPtr& prg = retPass->getVertexProgram();
        // Load this program if not done already
        if (!prg->isLoaded())
            prg->load();
        // Copy params
        retPass->setVertexProgramParameters(
            pass->getShadowCasterVertexProgramParameters());
        // Also have to hack the light autoparams, that is done later
    }
    else 
    {
        if (retPass == mShadowTextureCustomCasterPass)
        {
            // reset vp?
            if (mShadowTextureCustomCasterPass->getVertexProgramName() !=
                mShadowTextureCustomCasterVertexProgram)
            {
                mShadowTextureCustomCasterPass->setVertexProgram(
                    mShadowTextureCustomCasterVertexProgram, false);
                if(mShadowTextureCustomCasterPass->hasVertexProgram())
                {
                    mShadowTextureCustomCasterPass->setVertexProgramParameters(
                        mShadowTextureCustomCasterVPParams);

                }

            }

        }
        else
        {
            // Standard shadow caster pass, reset to no vp
            retPass->setVertexProgram(BLANKSTRING);
        }
    }

    if (!pass->getShadowCasterFragmentProgramName().empty())
    {
        // Have to merge the shadow caster fragment program in
        retPass->setFragmentProgram(
                                  pass->getShadowCasterFragmentProgramName(), false);
        const GpuProgramPtr& prg = retPass->getFragmentProgram();
        // Load this program if not done already
        if (!prg->isLoaded())
            prg->load();
        // Copy params
        retPass->setFragmentProgramParameters(
                                            pass->getShadowCasterFragmentProgramParameters());
        // Also have to hack the light autoparams, that is done later
    }
    else 
    {
        if (retPass == mShadowTextureCustomCasterPass)
        {
            // reset fp?
            if (mShadowTextureCustomCasterPass->getFragmentProgramName() !=
                mShadowTextureCustomCasterFragmentProgram)
            {
                mShadowTextureCustomCasterPass->setFragmentProgram(
                                                                 mShadowTextureCustomCasterFragmentProgram, false);
                if(mShadowTextureCustomCasterPass->hasFragmentProgram())
                {
                    mShadowTextureCustomCasterPass->setFragmentProgramParameters(
                                                                               mShadowTextureCustomCasterFPParams);
                }
            }
        }
        else
        {
            // Standard shadow caster pass, reset to no fp
            retPass->setFragmentProgram(BLANKSTRING);
        }
    }
    
    // handle the case where there is no fixed pipeline support
    retPass->getParent()->getParent()->compile();
    Technique* btech = retPass->getParent()->getParent()->getBestTechnique();
    if( btech )
    {
        retPass = btech->getPass(0);
    }

    return retPass;
}
//---------------------------------------------------------------------
const RealRect& SceneManager::getLightScissorRect( const Light* l, const Camera* cam )
{
    checkCachedLightClippingInfo();

    // Re-use calculations if possible
    LightClippingInfoMap::iterator ci = mLightClippingInfoMap.find(l);
    if (ci == mLightClippingInfoMap.end())
    {
        // create new entry
        ci = mLightClippingInfoMap.insert(LightClippingInfoMap::value_type(l, LightClippingInfo())).first;
    }
    if (!ci->second.scissorValid)
    {

        buildScissor(l, cam, ci->second.scissorRect);
        ci->second.scissorValid = true;
    }

    return ci->second.scissorRect;

}
//---------------------------------------------------------------------
ClipResult SceneManager::buildAndSetScissor(const LightList& ll, const Camera* cam)
{
#ifdef ENABLE_INCOMPATIBLE_OGRE_2_0
    RealRect finalRect;
    // init (inverted since we want to grow from nothing)
    finalRect.left = finalRect.bottom = 1.0f;
    finalRect.right = finalRect.top = -1.0f;

    for (LightList::const_iterator i = ll.begin(); i != ll.end(); ++i)
    {
        Light const * l = i->light;
        // a directional light is being used, no scissoring can be done, period.
        if (l->getType() == Light::LT_DIRECTIONAL)
            return CLIPPED_NONE;

        const RealRect& scissorRect = getLightScissorRect(l, cam);

        // merge with final
        finalRect.left = std::min(finalRect.left, scissorRect.left);
        finalRect.bottom = std::min(finalRect.bottom, scissorRect.bottom);
        finalRect.right= std::max(finalRect.right, scissorRect.right);
        finalRect.top = std::max(finalRect.top, scissorRect.top);


    }

    if (finalRect.left >= 1.0f || finalRect.right <= -1.0f ||
        finalRect.top <= -1.0f || finalRect.bottom >= 1.0f)
    {
        // rect was offscreen
        return CLIPPED_ALL;
    }

    // Some scissoring?
    if (finalRect.left > -1.0f || finalRect.right < 1.0f || 
        finalRect.bottom > -1.0f || finalRect.top < 1.0f)
    {
        // Turn normalised device coordinates into pixels
        int iLeft, iTop, iWidth, iHeight;
        mCurrentViewport->getActualDimensions(iLeft, iTop, iWidth, iHeight);
        size_t szLeft, szRight, szTop, szBottom;

        szLeft = (size_t)(iLeft + ((finalRect.left + 1) * 0.5 * iWidth));
        szRight = (size_t)(iLeft + ((finalRect.right + 1) * 0.5 * iWidth));
        szTop = (size_t)(iTop + ((-finalRect.top + 1) * 0.5 * iHeight));
        szBottom = (size_t)(iTop + ((-finalRect.bottom + 1) * 0.5 * iHeight));

        mDestRenderSystem->setScissorTest(true, szLeft, szTop, szRight, szBottom);

        return CLIPPED_SOME;
    }
    else
#endif
        return CLIPPED_NONE;

}
//---------------------------------------------------------------------
void SceneManager::buildScissor(const Light* light, const Camera* cam, RealRect& rect)
{
    // Project the sphere onto the camera
    Sphere sphere(light->getParentNode()->_getDerivedPosition(), light->getAttenuationRange());
    cam->projectSphere(sphere, &(rect.left), &(rect.top), &(rect.right), &(rect.bottom));
}
//---------------------------------------------------------------------
const AxisAlignedBox& SceneManager::getCurrentCastersBox(void) const
{
    if( !mCurrentShadowNode )
        return AxisAlignedBox::BOX_NULL;
    else
        return mCurrentShadowNode->getCastersBox();
}
//---------------------------------------------------------------------
void SceneManager::getMinMaxDepthRange( const Frustum *shadowMapCamera,
                                        Real &outMin, Real &outMax ) const
{
    if( !mCurrentShadowNode )
    {
        outMin = 0.0f;
        outMax = 100000.0f;
    }
    else
    {
        mCurrentShadowNode->getMinMaxDepthRange( shadowMapCamera, outMin, outMax );
    }
}
//---------------------------------------------------------------------
AxisAlignedBox SceneManager::_calculateCurrentCastersBox( uint32 viewportVisibilityMask,
                                                            uint8 _firstRq, uint8 _lastRq ) const
{
    AxisAlignedBox retVal;

    ObjectMemoryManagerVec::const_iterator it = mEntitiesMemoryManagerCulledList.begin();
    ObjectMemoryManagerVec::const_iterator en = mEntitiesMemoryManagerCulledList.end();

    while( it != en )
    {
        ObjectMemoryManager *objMemoryManager = *it;
        const size_t numRenderQueues = objMemoryManager->getNumRenderQueues();

        //TODO: Send this to worker threads (dark_sylinc)

        size_t firstRq = std::min<size_t>( _firstRq, numRenderQueues );
        size_t lastRq  = std::min<size_t>( _lastRq,  numRenderQueues );

        for( size_t i=firstRq; i<lastRq; ++i )
        {
            AxisAlignedBox tmpBox;

            ObjectData objData;
            const size_t numObjs = objMemoryManager->getFirstObjectData( objData, i );

            MovableObject::calculateCastersBox( numObjs, objData,
                                                (viewportVisibilityMask&getVisibilityMask()) |
                                                (viewportVisibilityMask &
                                                 ~VisibilityFlags::RESERVED_VISIBILITY_FLAGS),
                                                &tmpBox );
            retVal.merge( tmpBox );
        }

        ++it;
    }

    return retVal;
}
//---------------------------------------------------------------------
void SceneManager::propagateRelativeOrigin( SceneNode *sceneNode, const Vector3 &relativeOrigin )
{
    if( sceneNode->numAttachedObjects() > 0 )
    {
        size_t numAttachedCameras = 0;
        for( size_t i=0; i<sceneNode->numAttachedObjects(); ++i )
        {
            MovableObject *attachedObj = sceneNode->getAttachedObject( i );

            CameraMap::const_iterator itor = mCamerasByName.find( attachedObj->getName() ); 
            if( itor != mCamerasByName.end() && attachedObj == itor->second )
                ++numAttachedCameras;
        }

        if( numAttachedCameras == sceneNode->numAttachedObjects() )
        {
            //All of the attached objects are actually cameras. We can propagate
            //the change to them and continue with this node's children.
            for( size_t i=0; i<sceneNode->numAttachedObjects(); ++i )
            {
                MovableObject *attachedObj = sceneNode->getAttachedObject( i );
                assert( dynamic_cast<Camera*>( attachedObj ) );
                Camera *camera = static_cast<Camera*>( attachedObj );
                camera->setPosition( camera->getPosition() - relativeOrigin );
            }

            for( size_t i=0; i<sceneNode->numChildren(); ++i )
            {
                propagateRelativeOrigin( static_cast<SceneNode*>(sceneNode->getChild( i )),
                                         relativeOrigin );
            }
        }
        else
        {
            //There are attachments here. We can't keep propagating. Change here.
            sceneNode->setPosition( sceneNode->getPosition() - relativeOrigin );
        }
    }
    else if( sceneNode->numChildren() == 0 )
    {
        sceneNode->setPosition( sceneNode->getPosition() - relativeOrigin );
    }
    else
    {
        for( size_t i=0; i<sceneNode->numChildren(); ++i )
            propagateRelativeOrigin( static_cast<SceneNode*>(sceneNode->getChild( i )), relativeOrigin );
    }
}
//---------------------------------------------------------------------
void SceneManager::setRelativeOrigin( const Vector3 &relativeOrigin, bool bPermanent )
{
    if( !bPermanent )
    {
        for( size_t i=0; i<NUM_SCENE_MEMORY_MANAGER_TYPES; ++i )
            mSceneRoot[i]->setPosition( -relativeOrigin );
    }
    else
    {
        for( size_t i=0; i<NUM_SCENE_MEMORY_MANAGER_TYPES; ++i )
        {
            mSceneRoot[i]->setPosition( Vector3::ZERO );
            propagateRelativeOrigin( mSceneRoot[i], relativeOrigin );
        }
    }

    notifyStaticDirty( mSceneRoot[SCENE_STATIC] );
}
//---------------------------------------------------------------------
Vector3 SceneManager::getRelativeOrigin(void) const
{
    return mSceneRoot[SCENE_DYNAMIC]->getPosition();
}
//---------------------------------------------------------------------
void SceneManager::checkCachedLightClippingInfo()
{
    unsigned long frame = Root::getSingleton().getNextFrameNumber();
    if (frame != mLightClippingInfoMapFrameNumber)
    {
        // reset cached clip information
        mLightClippingInfoMap.clear();
        mLightClippingInfoMapFrameNumber = frame;
    }
}
//---------------------------------------------------------------------
const PlaneList& SceneManager::getLightClippingPlanes( const Light* l )
{
    checkCachedLightClippingInfo();

    // Try to re-use clipping info if already calculated
    LightClippingInfoMap::iterator ci = mLightClippingInfoMap.find(l);
    if (ci == mLightClippingInfoMap.end())
    {
        // create new entry
        ci = mLightClippingInfoMap.insert(LightClippingInfoMap::value_type(l, LightClippingInfo())).first;
    }
    if (!ci->second.clipPlanesValid)
    {
        buildLightClip(l, ci->second.clipPlanes);
        ci->second.clipPlanesValid = true;
    }
    return ci->second.clipPlanes;
    
}
//---------------------------------------------------------------------
ClipResult SceneManager::buildAndSetLightClip(const LightList& ll)
{
    if (!mDestRenderSystem->getCapabilities()->hasCapability(RSC_USER_CLIP_PLANES))
        return CLIPPED_NONE;

    Light const * clipBase = 0;
    for (LightList::const_iterator i = ll.begin(); i != ll.end(); ++i)
    {
        // a directional light is being used, no clipping can be done, period.
        if (i->light->getType() == Light::LT_DIRECTIONAL)
            return CLIPPED_NONE;

        if (clipBase)
        {
            // we already have a clip base, so we had more than one light
            // in this list we could clip by, so clip none
            return CLIPPED_NONE;
        }
        clipBase = i->light;
    }

    if (clipBase)
    {
        const PlaneList& clipPlanes = getLightClippingPlanes(clipBase);
        
        mDestRenderSystem->setClipPlanes(clipPlanes);
        return CLIPPED_SOME;
    }
    else
    {
        // Can only get here if no non-directional lights from which to clip from
        // ie list must be empty
        return CLIPPED_ALL;
    }


}
//---------------------------------------------------------------------
void SceneManager::buildLightClip(const Light* l, PlaneList& planes)
{
    if (!mDestRenderSystem->getCapabilities()->hasCapability(RSC_USER_CLIP_PLANES))
        return;

    planes.clear();

    Vector3 pos = l->getParentNode()->_getDerivedPosition();
    Real r = l->getAttenuationRange();
    switch(l->getType())
    {
    case Light::LT_POINT:
        {
            planes.push_back(Plane(Vector3::UNIT_X, pos + Vector3(-r, 0, 0)));
            planes.push_back(Plane(Vector3::NEGATIVE_UNIT_X, pos + Vector3(r, 0, 0)));
            planes.push_back(Plane(Vector3::UNIT_Y, pos + Vector3(0, -r, 0)));
            planes.push_back(Plane(Vector3::NEGATIVE_UNIT_Y, pos + Vector3(0, r, 0)));
            planes.push_back(Plane(Vector3::UNIT_Z, pos + Vector3(0, 0, -r)));
            planes.push_back(Plane(Vector3::NEGATIVE_UNIT_Z, pos + Vector3(0, 0, r)));
        }
        break;
    case Light::LT_SPOTLIGHT:
        {
            Vector3 dir = l->getDerivedDirection();
            // near & far planes
            planes.push_back(Plane(dir, pos + dir * l->getSpotlightNearClipDistance()));
            planes.push_back(Plane(-dir, pos + dir * r));
            // 4 sides of pyramids
            // derive orientation
            Vector3 up = Vector3::UNIT_Y;
            // Check it's not coincident with dir
            if (Math::Abs(up.dotProduct(dir)) >= 1.0f)
            {
                up = Vector3::UNIT_Z;
            }
            // cross twice to rederive, only direction is unaltered
            Vector3 right = dir.crossProduct(up);
            right.normalise();
            up = right.crossProduct(dir);
            up.normalise();
            // Derive quaternion from axes (negate dir since -Z)
            Quaternion q;
            q.FromAxes(right, up, -dir);

            // derive pyramid corner vectors in world orientation
            Vector3 tl, tr, bl, br;
            Real d = Math::Tan(l->getSpotlightOuterAngle() * 0.5) * r;
            tl = q * Vector3(-d, d, -r);
            tr = q * Vector3(d, d, -r);
            bl = q * Vector3(-d, -d, -r);
            br = q * Vector3(d, -d, -r);

            // use cross product to derive normals, pass through light world pos
            // top
            planes.push_back(Plane(tl.crossProduct(tr).normalisedCopy(), pos));
            // right
            planes.push_back(Plane(tr.crossProduct(br).normalisedCopy(), pos));
            // bottom
            planes.push_back(Plane(br.crossProduct(bl).normalisedCopy(), pos));
            // left
            planes.push_back(Plane(bl.crossProduct(tl).normalisedCopy(), pos));

        }
        break;
    default:
        // do nothing
        break;
    };

}
//---------------------------------------------------------------------
void SceneManager::resetLightClip()
{
    if (!mDestRenderSystem->getCapabilities()->hasCapability(RSC_USER_CLIP_PLANES))
        return;

    mDestRenderSystem->resetClipPlanes();
}
//---------------------------------------------------------------------
void SceneManager::setShadowColour(const ColourValue& colour)
{
    mShadowColour = colour;
}
//---------------------------------------------------------------------
const ColourValue& SceneManager::getShadowColour(void) const
{
    return mShadowColour;
}
//---------------------------------------------------------------------
void SceneManager::setShadowFarDistance(Real distance)
{
    mDefaultShadowFarDist = distance;
    mDefaultShadowFarDistSquared = distance * distance;
}
//---------------------------------------------------------------------
void SceneManager::setShadowDirectionalLightExtrusionDistance(Real dist)
{
    mShadowDirLightExtrudeDist = dist;
}
//---------------------------------------------------------------------
Real SceneManager::getShadowDirectionalLightExtrusionDistance(void) const
{
    return mShadowDirLightExtrudeDist;
}
//---------------------------------------------------------------------
void SceneManager::setShadowTextureCasterMaterial(const String& name)
{
    if (name.empty())
    {
        mShadowTextureCustomCasterPass = 0;
    }
    else
    {
        MaterialPtr mat = MaterialManager::getSingleton().getByName(name);
        if (mat.isNull())
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
                "Cannot locate material called '" + name + "'", 
                "SceneManager::setShadowTextureCasterMaterial");
        }
        mat->load();
        if (!mat->getBestTechnique())
        {
            // unsupported
            mShadowTextureCustomCasterPass = 0;
        }
        else
        {

            mShadowTextureCustomCasterPass = mat->getBestTechnique()->getPass(0);
            if (mShadowTextureCustomCasterPass->hasVertexProgram())
            {
                // Save vertex program and params in case we have to swap them out
                mShadowTextureCustomCasterVertexProgram = 
                    mShadowTextureCustomCasterPass->getVertexProgramName();
                mShadowTextureCustomCasterVPParams = 
                    mShadowTextureCustomCasterPass->getVertexProgramParameters();
            }
            if (mShadowTextureCustomCasterPass->hasFragmentProgram())
            {
                // Save fragment program and params in case we have to swap them out
                mShadowTextureCustomCasterFragmentProgram = 
                mShadowTextureCustomCasterPass->getFragmentProgramName();
                mShadowTextureCustomCasterFPParams = 
                mShadowTextureCustomCasterPass->getFragmentProgramParameters();
            }
        }
    }
}
//---------------------------------------------------------------------
template<typename T>
void SceneManager::checkMovableObjectIntegrity( const typename vector<T*>::type &container,
                                                const T *mo ) const
{
    if( mo->mGlobalIndex >= container.size() || mo != *(container.begin() + mo->mGlobalIndex) )
    {
        OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "MovableObject ID: " +
            StringConverter::toString( mo->getId() ) + ", named '" + mo->getName() +
            "' of type '" + mo->getMovableType() + "'\nHad it's mGlobalIndex out of "
            "date!!! (or the MovableObject wasn't created with this SceneManager)",
            "SceneManager::checkMovableObjectIntegrity" );
    }
}
//---------------------------------------------------------------------
SceneManager::RenderContext* SceneManager::_pauseRendering()
{
    RenderContext* context = new RenderContext;
    context->renderQueue = mRenderQueue;
    //context->viewport = mCurrentViewport;
    context->camerasInProgress = mCamerasInProgress;

    context->rsContext = mDestRenderSystem->_pauseFrame();
    mRenderQueue = 0;
    return context;
}
//---------------------------------------------------------------------
void SceneManager::_resumeRendering(SceneManager::RenderContext* context) 
{
    delete mRenderQueue;
    mRenderQueue = context->renderQueue;
    Ogre::Viewport* vp = context->viewport;
    const Ogre::Camera* camera = context->camerasInProgress.renderingCamera;

    // Tell params about viewport
    //setViewport(vp);

    // Tell params about camera
    mAutoParamDataSource->setCurrentCamera(camera);
    // Set autoparams for finite dir light extrusion
    mAutoParamDataSource->setShadowDirLightExtrusionDistance(mShadowDirLightExtrudeDist);

    // Tell params about current ambient light
    mAutoParamDataSource->setAmbientLightColour(mAmbientLight, mAmbientLightHemisphereDir);

    // Set camera window clipping planes (if any)
    if (mDestRenderSystem->getCapabilities()->hasCapability(RSC_USER_CLIP_PLANES))
    {
        mDestRenderSystem->resetClipPlanes();
        if (camera->isWindowSet())  
        {
            mDestRenderSystem->setClipPlanes(camera->getWindowPlanes());
        }
    }
    mCamerasInProgress = context->camerasInProgress;
    mDestRenderSystem->_resumeFrame(context->rsContext);

    // Set initial camera state
    mDestRenderSystem->_setProjectionMatrix( Matrix4::IDENTITY );
    
    mCachedViewMatrix = mCamerasInProgress.renderingCamera->getViewMatrix(true);

    setViewMatrix(mCachedViewMatrix);
    delete context;
}
//---------------------------------------------------------------------
v1::StaticGeometry* SceneManager::createStaticGeometry(const String& name)
{
    // Check not existing
    if (mStaticGeometryList.find(name) != mStaticGeometryList.end())
    {
        OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, 
            "StaticGeometry with name '" + name + "' already exists!", 
            "SceneManager::createStaticGeometry");
    }
    v1::StaticGeometry* ret = OGRE_NEW v1::StaticGeometry(this, name);
    mStaticGeometryList[name] = ret;
    return ret;
}
//---------------------------------------------------------------------
v1::StaticGeometry* SceneManager::getStaticGeometry(const String& name) const
{
    StaticGeometryList::const_iterator i = mStaticGeometryList.find(name);
    if (i == mStaticGeometryList.end())
    {
        OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, 
            "StaticGeometry with name '" + name + "' not found", 
            "SceneManager::createStaticGeometry");
    }
    return i->second;
}
//-----------------------------------------------------------------------
bool SceneManager::hasStaticGeometry(const String& name) const
{
    return (mStaticGeometryList.find(name) != mStaticGeometryList.end());
}

//---------------------------------------------------------------------
void SceneManager::destroyStaticGeometry(v1::StaticGeometry* geom)
{
    destroyStaticGeometry(geom->getName());
}
//---------------------------------------------------------------------
void SceneManager::destroyStaticGeometry(const String& name)
{
    StaticGeometryList::iterator i = mStaticGeometryList.find(name);
    if (i != mStaticGeometryList.end())
    {
        OGRE_DELETE i->second;
        mStaticGeometryList.erase(i);
    }

}
//---------------------------------------------------------------------
void SceneManager::destroyAllStaticGeometry(void)
{
    StaticGeometryList::iterator i, iend;
    iend = mStaticGeometryList.end();
    for (i = mStaticGeometryList.begin(); i != iend; ++i)
    {
        OGRE_DELETE i->second;
    }
    mStaticGeometryList.clear();
}
//---------------------------------------------------------------------
AxisAlignedBoxSceneQuery* 
SceneManager::createAABBQuery(const AxisAlignedBox& box, uint32 mask)
{
    DefaultAxisAlignedBoxSceneQuery* q = OGRE_NEW DefaultAxisAlignedBoxSceneQuery(this);
    q->setBox(box);
    q->setQueryMask(mask);
    return q;
}
//---------------------------------------------------------------------
SphereSceneQuery* 
SceneManager::createSphereQuery(const Sphere& sphere, uint32 mask)
{
    DefaultSphereSceneQuery* q = OGRE_NEW DefaultSphereSceneQuery(this);
    q->setSphere(sphere);
    q->setQueryMask(mask);
    return q;
}
//---------------------------------------------------------------------
PlaneBoundedVolumeListSceneQuery* 
SceneManager::createPlaneBoundedVolumeQuery(const PlaneBoundedVolumeList& volumes, 
                                            uint32 mask)
{
    DefaultPlaneBoundedVolumeListSceneQuery* q = OGRE_NEW DefaultPlaneBoundedVolumeListSceneQuery(this);
    q->setVolumes(volumes);
    q->setQueryMask(mask);
    return q;
}

//---------------------------------------------------------------------
RaySceneQuery* 
SceneManager::createRayQuery(const Ray& ray, uint32 mask)
{
    DefaultRaySceneQuery* q = OGRE_NEW DefaultRaySceneQuery(this);
    q->setRay(ray);
    q->setQueryMask(mask);
    return q;
}
//---------------------------------------------------------------------
IntersectionSceneQuery* 
SceneManager::createIntersectionQuery(uint32 mask)
{

    DefaultIntersectionSceneQuery* q = OGRE_NEW DefaultIntersectionSceneQuery(this);
    q->setQueryMask(mask);
    return q;
}
//---------------------------------------------------------------------
void SceneManager::destroyQuery(SceneQuery* query)
{
    OGRE_DELETE query;
}
//---------------------------------------------------------------------
SceneManager::MovableObjectCollection* 
SceneManager::getMovableObjectCollection(const String& typeName)
{
    // lock collection mutex
    OGRE_LOCK_MUTEX(mMovableObjectCollectionMapMutex);

    MovableObjectCollectionMap::iterator i = 
        mMovableObjectCollectionMap.find(typeName);
    if (i == mMovableObjectCollectionMap.end())
    {
        // create
        MovableObjectCollection* newCollection = OGRE_NEW_T(MovableObjectCollection, MEMCATEGORY_SCENE_CONTROL)();
        mMovableObjectCollectionMap[typeName] = newCollection;
        return newCollection;
    }
    else
    {
        return i->second;
    }
}
//---------------------------------------------------------------------
const SceneManager::MovableObjectCollection* 
SceneManager::getMovableObjectCollection(const String& typeName) const
{
    // lock collection mutex
    OGRE_LOCK_MUTEX(mMovableObjectCollectionMapMutex);

    MovableObjectCollectionMap::const_iterator i = 
        mMovableObjectCollectionMap.find(typeName);
    if (i == mMovableObjectCollectionMap.end())
    {
        OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, 
            "Object collection named '" + typeName + "' does not exist.", 
            "SceneManager::getMovableObjectCollection");
    }
    else
    {
        return i->second;
    }
}
//---------------------------------------------------------------------
MovableObject* SceneManager::createMovableObject( const String& typeName,
                                                    ObjectMemoryManager *objectMemMgr,
                                                    const NameValuePairList* params )
{
    // Nasty hack to make generalised Camera functions work without breaking add-on SMs
    if (typeName == "Camera")
    {
        return createCamera( "", true );
    }
    MovableObjectFactory* factory = 
        Root::getSingleton().getMovableObjectFactory(typeName);
    // Check for duplicate names
    MovableObjectCollection* objectMap = getMovableObjectCollection(typeName);

    {
        OGRE_LOCK_MUTEX(objectMap->mutex);

        MovableObject* newObj = factory->createInstance( Id::generateNewId<MovableObject>(),
                                                         objectMemMgr, this, params );
        objectMap->movableObjects.push_back( newObj );
        newObj->mGlobalIndex = objectMap->movableObjects.size() - 1;
        return newObj;
    }

}
//---------------------------------------------------------------------
bool SceneManager::hasMovableObject( MovableObject *m )
{
    if(!m)
        OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot look for a null object", "SceneManager::hasMovableObject");

    MovableObjectVec objects = getMovableObjectCollection(m->getMovableType())->movableObjects;
    return (m->mGlobalIndex < objects.size() && m == *(objects.begin() + m->mGlobalIndex));
}
//---------------------------------------------------------------------
void SceneManager::destroyMovableObject( MovableObject *m, const String& typeName )
{
    {
        WireAabbVec::const_iterator itor = mTrackingWireAabbs.begin();
        WireAabbVec::const_iterator end  = mTrackingWireAabbs.end();

        while( itor != end )
        {
            if( (*itor)->getTrackedObject() == m )
            {
                (*itor)->track( (MovableObject*)0 );
                //Iterators got invalidated. Also stop iterating
                itor = end = mTrackingWireAabbs.end();
            }
            else
            {
                ++itor;
            }
        }
    }

    // Nasty hack to make generalised Camera functions work without breaking add-on SMs
    if (typeName == "Camera")
    {
        assert( dynamic_cast<Camera*>(m) );
        destroyCamera( static_cast<Camera*>(m) );
        return;
    }
    MovableObjectCollection* objectMap = getMovableObjectCollection(typeName);
    MovableObjectFactory* factory = 
        Root::getSingleton().getMovableObjectFactory(typeName);

    {
            OGRE_LOCK_MUTEX(objectMap->mutex);

        checkMovableObjectIntegrity( objectMap->movableObjects, m );

        MovableObjectVec::iterator itor = objectMap->movableObjects.begin() + m->mGlobalIndex;

        //If itor is invalid then something is terribly wrong (deleting a ptr twice may be?)
        itor = efficientVectorRemove( objectMap->movableObjects, itor );
        factory->destroyInstance( m );
        m = 0;

        //The MovableObject that was at the end got swapped and has now a different index
        if( itor != objectMap->movableObjects.end() )
            (*itor)->mGlobalIndex = itor - objectMap->movableObjects.begin();
    }
}
//---------------------------------------------------------------------
void SceneManager::destroyAllMovableObjectsByType(const String& typeName)
{
    // Nasty hack to make generalised Camera functions work without breaking add-on SMs
    if (typeName == "Camera")
    {
        destroyAllCameras();
        return;
    }
    MovableObjectCollection* objectMap = getMovableObjectCollection(typeName);
    MovableObjectFactory* factory = 
        Root::getSingleton().getMovableObjectFactory(typeName);
    
    {
            OGRE_LOCK_MUTEX(objectMap->mutex);
        MovableObjectVec::iterator itor = objectMap->movableObjects.begin();
        MovableObjectVec::iterator end  = objectMap->movableObjects.end();
        while( itor != end )
        {
            if( (*itor)->_getManager() == this )
            {
                // Only destroy our own
                MovableObject *mo = *itor;
                itor = efficientVectorRemove( objectMap->movableObjects, itor );
                end  = objectMap->movableObjects.end();
                factory->destroyInstance( mo );

                //The node that was at the end got swapped and has now a different index
                if( itor != end )
                    (*itor)->mGlobalIndex = itor - objectMap->movableObjects.begin();
            }
            else
            {
                ++itor;
            }
        }
    }
}
//---------------------------------------------------------------------
void SceneManager::destroyAllMovableObjects(void)
{
    // Lock collection mutex
    OGRE_LOCK_MUTEX(mMovableObjectCollectionMapMutex);

    MovableObjectCollectionMap::iterator ci = mMovableObjectCollectionMap.begin();

    for(;ci != mMovableObjectCollectionMap.end(); ++ci)
    {
        MovableObjectCollection* coll = ci->second;

        // lock map mutex
        OGRE_LOCK_MUTEX(coll->mutex);

        if (Root::getSingleton().hasMovableObjectFactory(ci->first))
        {
            // Only destroy if we have a factory instance; otherwise must be injected
            MovableObjectFactory* factory = 
                Root::getSingleton().getMovableObjectFactory(ci->first);

            MovableObjectVec::iterator itor = coll->movableObjects.begin();
            MovableObjectVec::iterator end  = coll->movableObjects.end();
            while( itor != end )
            {
                if( (*itor)->_getManager() == this )
                {
                    // Only destroy our own
                    MovableObject *mo = *itor;
                    itor = efficientVectorRemove( coll->movableObjects, itor );
                    end  = coll->movableObjects.end();
                    factory->destroyInstance( mo );

                    //The node that was at the end got swapped and has now a different index
                    if( itor != end )
                        (*itor)->mGlobalIndex = itor - coll->movableObjects.begin();
                }
                else
                {
                    ++itor;
                }
            }
        }
    }

}

//---------------------------------------------------------------------
SceneManager::MovableObjectIterator 
SceneManager::getMovableObjectIterator(const String& typeName)
{
    MovableObjectCollection* objectMap = getMovableObjectCollection(typeName);
    // Iterator not thread safe! Warned in header.
    return MovableObjectIterator(objectMap->movableObjects.begin(), objectMap->movableObjects.end());
}
//---------------------------------------------------------------------
void SceneManager::destroyMovableObject(MovableObject* m)
{
    if(!m)
        OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot destroy a null MovableObject.", "SceneManager::destroyMovableObject");
    destroyMovableObject(m, m->getMovableType());
}
//---------------------------------------------------------------------
void SceneManager::injectMovableObject(MovableObject* m)
{
    MovableObjectCollection* objectMap = getMovableObjectCollection(m->getMovableType());
    {
            OGRE_LOCK_MUTEX(objectMap->mutex);

        objectMap->movableObjects.push_back( m );
        m->mGlobalIndex = objectMap->movableObjects.size() - 1;
    }
}
//---------------------------------------------------------------------
void SceneManager::extractMovableObject(MovableObject* m)
{
    MovableObjectCollection* objectMap = getMovableObjectCollection(m->getMovableType());
    {
            OGRE_LOCK_MUTEX(objectMap->mutex);

        checkMovableObjectIntegrity( objectMap->movableObjects, m );
        MovableObjectVec::iterator itor = objectMap->movableObjects.begin() + m->mGlobalIndex;

        // no delete
        itor = efficientVectorRemove( objectMap->movableObjects, itor );
        //The node that was at the end got swapped and has now a different index
        if( itor != objectMap->movableObjects.end() )
            (*itor)->mGlobalIndex = itor - objectMap->movableObjects.begin();
    }
}
//---------------------------------------------------------------------
void SceneManager::extractAllMovableObjectsByType(const String& typeName)
{
    MovableObjectCollection* objectMap = getMovableObjectCollection(typeName);
    {
            OGRE_LOCK_MUTEX(objectMap->mutex);
        // no deletion
        objectMap->movableObjects.clear();
    }
}
//---------------------------------------------------------------------
void SceneManager::_renderSingleObject( Renderable* pRend, const MovableObject *pMovableObject,
                                        bool casterPass, bool dualParaboloid )
{
    mRenderQueue->renderSingleObject( pRend, pMovableObject, mDestRenderSystem,
                                      casterPass, dualParaboloid );
}
//---------------------------------------------------------------------
RenderSystem *SceneManager::getDestinationRenderSystem()
{
    return mDestRenderSystem;
}
//---------------------------------------------------------------------
uint32 SceneManager::_getCombinedVisibilityMask(void) const
{
    //Always preserve the settings of the reserved visibility flags in the viewport.
    return mCurrentViewport0 ?
        ((mCurrentViewport0->getVisibilityMask() & mVisibilityMask) |
        (mCurrentViewport0->getVisibilityMask() & ~VisibilityFlags::RESERVED_VISIBILITY_FLAGS)) :
                mVisibilityMask;

}
//---------------------------------------------------------------------
void SceneManager::addLodListener(LodListener *listener)
{
    mLodListeners.insert(listener);
}
//---------------------------------------------------------------------
void SceneManager::removeLodListener(LodListener *listener)
{
    LodListenerSet::iterator it = mLodListeners.find(listener);
    if (it != mLodListeners.end())
        mLodListeners.erase(it);
}
//---------------------------------------------------------------------
void SceneManager::_notifyMovableObjectLodChanged(MovableObjectLodChangedEvent& evt)
{
    // Notify listeners and determine if event needs to be queued
    bool queueEvent = false;
    for (LodListenerSet::iterator it = mLodListeners.begin(); it != mLodListeners.end(); ++it)
    {
        if ((*it)->prequeueMovableObjectLodChanged(evt))
            queueEvent = true;
    }

    // Push event onto queue if requested
    if (queueEvent)
        mMovableObjectLodChangedEvents.push_back(evt);
}
//---------------------------------------------------------------------
void SceneManager::_notifyEntityMeshLodChanged(EntityMeshLodChangedEvent& evt)
{
    // Notify listeners and determine if event needs to be queued
    bool queueEvent = false;
    for (LodListenerSet::iterator it = mLodListeners.begin(); it != mLodListeners.end(); ++it)
    {
        if ((*it)->prequeueEntityMeshLodChanged(evt))
            queueEvent = true;
    }

    // Push event onto queue if requested
    if (queueEvent)
        mEntityMeshLodChangedEvents.push_back(evt);
}
//---------------------------------------------------------------------
void SceneManager::_notifyEntityMaterialLodChanged(EntityMaterialLodChangedEvent& evt)
{
    // Notify listeners and determine if event needs to be queued
    bool queueEvent = false;
    for (LodListenerSet::iterator it = mLodListeners.begin(); it != mLodListeners.end(); ++it)
    {
        if ((*it)->prequeueEntityMaterialLodChanged(evt))
            queueEvent = true;
    }

    // Push event onto queue if requested
    if (queueEvent)
        mEntityMaterialLodChangedEvents.push_back(evt);
}
//---------------------------------------------------------------------
void SceneManager::_handleLodEvents()
{
    // Handle events with each listener
    for (LodListenerSet::iterator it = mLodListeners.begin(); it != mLodListeners.end(); ++it)
    {
        for (MovableObjectLodChangedEventList::const_iterator jt = mMovableObjectLodChangedEvents.begin(); jt != mMovableObjectLodChangedEvents.end(); ++jt)
            (*it)->postqueueMovableObjectLodChanged(*jt);

        for (EntityMeshLodChangedEventList::const_iterator jt = mEntityMeshLodChangedEvents.begin(); jt != mEntityMeshLodChangedEvents.end(); ++jt)
            (*it)->postqueueEntityMeshLodChanged(*jt);

        for (EntityMaterialLodChangedEventList::const_iterator jt = mEntityMaterialLodChangedEvents.begin(); jt != mEntityMaterialLodChangedEvents.end(); ++jt)
            (*it)->postqueueEntityMaterialLodChanged(*jt);
    }

    // Clear event queues
    mMovableObjectLodChangedEvents.clear();
    mEntityMeshLodChangedEvents.clear();
    mEntityMaterialLodChangedEvents.clear();
}
//---------------------------------------------------------------------
void SceneManager::setViewMatrix(const Matrix4& m)
{
    mDestRenderSystem->_setViewMatrix(m);
    if (mDestRenderSystem->areFixedFunctionLightsInViewSpace())
    {
        // reset light hash if we've got lights already set
        mLastLightHash = mLastLightHash ? 0 : mLastLightHash;
    }
}
//---------------------------------------------------------------------
void SceneManager::useLights(const LightList& lights, unsigned short limit)
{
    // only call the rendersystem if light list has changed
    if (lights.getHash() != mLastLightHash || limit != mLastLightLimit)
    {
        mDestRenderSystem->_useLights(lights, limit);
        mLastLightHash = lights.getHash();
        mLastLightLimit = limit;
    }
}
//---------------------------------------------------------------------
void SceneManager::useLightsGpuProgram(const Pass* pass, const LightList* lights)
{
    // only call the rendersystem if light list has changed
    if (lights->getHash() != mLastLightHashGpuProgram)
    {
        // Update any automatic gpu params for lights
        // Other bits of information will have to be looked up
        mAutoParamDataSource->setCurrentLightList(lights);
        mGpuParamsDirty |= GPV_LIGHTS;

        mLastLightHashGpuProgram = lights->getHash();

    }
}
//---------------------------------------------------------------------
void SceneManager::_markGpuParamsDirty(uint16 mask)
{
    mGpuParamsDirty |= mask;
}
//---------------------------------------------------------------------
void SceneManager::updateGpuProgramParameters(const Pass* pass)
{
    if (pass->isProgrammable())
    {

        if (!mGpuParamsDirty)
            return;

        if (mGpuParamsDirty)
            pass->_updateAutoParams(mAutoParamDataSource, mGpuParamsDirty);

        if (pass->hasVertexProgram())
        {
            mDestRenderSystem->bindGpuProgramParameters(GPT_VERTEX_PROGRAM, 
                pass->getVertexProgramParameters(), mGpuParamsDirty);
        }

        if (pass->hasGeometryProgram())
        {
            mDestRenderSystem->bindGpuProgramParameters(GPT_GEOMETRY_PROGRAM,
                pass->getGeometryProgramParameters(), mGpuParamsDirty);
        }

        if (pass->hasFragmentProgram())
        {
            mDestRenderSystem->bindGpuProgramParameters(GPT_FRAGMENT_PROGRAM, 
                pass->getFragmentProgramParameters(), mGpuParamsDirty);
        }

        if (pass->hasTessellationHullProgram())
        {
            mDestRenderSystem->bindGpuProgramParameters(GPT_HULL_PROGRAM, 
                pass->getTessellationHullProgramParameters(), mGpuParamsDirty);
        }

        if (pass->hasTessellationDomainProgram())
        {
            mDestRenderSystem->bindGpuProgramParameters(GPT_DOMAIN_PROGRAM, 
                pass->getTessellationDomainProgramParameters(), mGpuParamsDirty);
        }

                // if (pass->hasComputeProgram())
        // {
                //     mDestRenderSystem->bindGpuProgramParameters(GPT_COMPUTE_PROGRAM, 
                //                                                 pass->getComputeProgramParameters(), mGpuParamsDirty);
        // }

        mGpuParamsDirty = 0;
    }

}
void SceneManager::fireWorkerThreadsAndWait(void)
{
    if( mForceMainThread )
        updateWorkerThreadImpl( 0 );
    else
    {
        mWorkerThreadsBarrier->sync(); //Fire threads
        mWorkerThreadsBarrier->sync(); //Wait them to complete
    }
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SceneManager::fireCullFrustumThreads( const CullFrustumRequest &request )
{
    mCurrentCullFrustumRequest = request;
    mRequestType = CULL_FRUSTUM;
    //This is where I figuratively kill whoever made mutable variables inside a
    //const function, silencing a race condition: Update the frustum planes now
    //in case they weren't up to date.
    mCurrentCullFrustumRequest.camera->getFrustumPlanes();
    mCurrentCullFrustumRequest.lodCamera->getFrustumPlanes();
    fireWorkerThreadsAndWait();
}
//---------------------------------------------------------------------
void SceneManager::executeUserScalableTask( UniformScalableTask *task, bool bBlock )
{
    mRequestType = USER_UNIFORM_SCALABLE_TASK;
    mUserTask = task;

    if( mForceMainThread )
        updateWorkerThreadImpl( 0 );
    else
    {
        mWorkerThreadsBarrier->sync(); //Fire threads
        if( bBlock )
            mWorkerThreadsBarrier->sync(); //Wait them to complete
    }
}
//---------------------------------------------------------------------
void SceneManager::waitForPendingUserScalableTask()
{
    if( !mForceMainThread )
    {
        assert( mRequestType == USER_UNIFORM_SCALABLE_TASK );
        mWorkerThreadsBarrier->sync(); //Wait them to complete
    }
}
//---------------------------------------------------------------------
unsigned long updateWorkerThread( ThreadHandle *threadHandle )
{
    SceneManager *sceneManager = reinterpret_cast<SceneManager*>( threadHandle->getUserParam() );
    return sceneManager->_updateWorkerThread( threadHandle );
}
THREAD_DECLARE( updateWorkerThread );
//---------------------------------------------------------------------
void SceneManager::startWorkerThreads()
{
    if( !mForceMainThread )
    {
        mWorkerThreadsBarrier = new Barrier( mNumWorkerThreads+1 );
        mWorkerThreads.reserve( mNumWorkerThreads );
        for( size_t i=0; i<mNumWorkerThreads; ++i )
        {
            ThreadHandlePtr th = Threads::CreateThread( THREAD_GET( updateWorkerThread ), i, this );
            mWorkerThreads.push_back( th );
        }
    }
}
//---------------------------------------------------------------------
void SceneManager::stopWorkerThreads()
{
    if( !mForceMainThread )
    {
        mRequestType = STOP_THREADS;
        fireWorkerThreadsAndWait();

        Threads::WaitForThreads( mWorkerThreads );

        delete mWorkerThreadsBarrier;
        mWorkerThreadsBarrier = 0;
    }
}
//---------------------------------------------------------------------
unsigned long SceneManager::_updateWorkerThread( ThreadHandle *threadHandle )
{
    bool exitThread = false;
    size_t threadIdx = threadHandle->getThreadIdx();
    while( !exitThread )
    {
        mWorkerThreadsBarrier->sync();
        exitThread = updateWorkerThreadImpl( threadIdx );
        mWorkerThreadsBarrier->sync();
    }

    return 0;
}
//---------------------------------------------------------------------
inline bool SceneManager::updateWorkerThreadImpl( size_t threadIdx )
{
    bool exitThread = false;

    switch( mRequestType )
    {
    case CULL_FRUSTUM:
        cullFrustum( mCurrentCullFrustumRequest, threadIdx );
        break;
    case UPDATE_ALL_ANIMATIONS:
        updateAllAnimationsThread( threadIdx );
        break;
    case UPDATE_ALL_TRANSFORMS:
        updateAllTransformsThread( mUpdateTransformRequest, threadIdx );
        break;
    case UPDATE_ALL_BONE_TO_TAG_TRANSFORMS:
        updateAllTransformsBoneToTagThread( mUpdateTransformRequest, threadIdx );
        break;
    case UPDATE_ALL_TAG_ON_TAG_TRANSFORMS:
        updateAllTransformsTagOnTagThread( mUpdateTransformRequest, threadIdx );
        break;
    case UPDATE_ALL_BOUNDS:
        updateAllBoundsThread( *mUpdateBoundsRequest, threadIdx );
        break;
    case UPDATE_ALL_LODS:
        updateAllLodsThread( mUpdateLodRequest, threadIdx );
        break;
    case BUILD_LIGHT_LIST01:
        buildLightListThread01( mBuildLightListRequestPerThread[threadIdx], threadIdx );
        break;
    case BUILD_LIGHT_LIST02:
        buildLightListThread02( threadIdx );
        break;
    case USER_UNIFORM_SCALABLE_TASK:
        mUserTask->execute( threadIdx, mNumWorkerThreads );
        break;
    case STOP_THREADS:
        exitThread = true;
        break;
    default:
        break;
    }

    return exitThread;
}
SceneManagerFactory::~SceneManagerFactory() {}
}
