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

#include "OgreMeshManager2.h"

#include "OgreException.h"
#include "OgreMatrix4.h"
#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgrePatchMesh.h"
#include "OgrePrefabFactory.h"
#include "OgreSubMesh2.h"

namespace Ogre
{
    template <>
    MeshManager *Singleton<MeshManager>::msSingleton = 0;
    //-----------------------------------------------------------------------
    MeshManager *MeshManager::getSingletonPtr() { return msSingleton; }
    MeshManager &MeshManager::getSingleton()
    {
        assert( msSingleton );
        return ( *msSingleton );
    }
    //-----------------------------------------------------------------------
    MeshManager::MeshManager() : mVaoManager( 0 ), mBoundsPaddingFactor( Real( 0.01 ) )
    {
        mLoadOrder = 300.0f;
        mResourceType = "Mesh2";

        ResourceGroupManager::getSingleton()._registerResourceManager( mResourceType, this );
    }
    //-----------------------------------------------------------------------
    MeshManager::~MeshManager()
    {
        ResourceGroupManager::getSingleton()._unregisterResourceManager( mResourceType );
    }
    //-----------------------------------------------------------------------
    MeshPtr MeshManager::getByName( const String &name, const String &groupName )
    {
        return std::static_pointer_cast<Mesh>( getResourceByName( name, groupName ) );
    }
    //-----------------------------------------------------------------------
    void MeshManager::_initialise() {}
    //-----------------------------------------------------------------------
    void MeshManager::_setVaoManager( VaoManager *vaoManager ) { mVaoManager = vaoManager; }
    //-----------------------------------------------------------------------
    MeshManager::ResourceCreateOrRetrieveResult MeshManager::createOrRetrieve(
        const String &name, const String &group, bool isManual, ManualResourceLoader *loader,
        const NameValuePairList *params, BufferType vertexBufferType, BufferType indexBufferType,
        bool vertexBufferShadowed, bool indexBufferShadowed )
    {
        ResourceCreateOrRetrieveResult res =
            ResourceManager::createOrRetrieve( name, group, isManual, loader, params );
        MeshPtr pMesh = std::static_pointer_cast<Mesh>( res.first );
        // Was it created?
        if( res.second )
        {
            pMesh->setVertexBufferPolicy( vertexBufferType, vertexBufferShadowed );
            pMesh->setIndexBufferPolicy( indexBufferType, indexBufferShadowed );
        }
        return res;
    }
    //-----------------------------------------------------------------------
    MeshPtr MeshManager::prepare( const String &filename, const String &groupName,
                                  BufferType vertexBufferType, BufferType indexBufferType,
                                  bool vertexBufferShadowed, bool indexBufferShadowed )
    {
        MeshPtr pMesh = std::static_pointer_cast<Mesh>(
            createOrRetrieve( filename, groupName, false, 0, 0, vertexBufferType, indexBufferType,
                              vertexBufferShadowed, indexBufferShadowed )
                .first );
        pMesh->prepare();
        return pMesh;
    }
    //-----------------------------------------------------------------------
    MeshPtr MeshManager::load( const String &filename, const String &groupName,
                               BufferType vertexBufferType, BufferType indexBufferType,
                               bool vertexBufferShadowed, bool indexBufferShadowed )
    {
        MeshPtr pMesh = std::static_pointer_cast<Mesh>(
            createOrRetrieve( filename, groupName, false, 0, 0, vertexBufferType, indexBufferType,
                              vertexBufferShadowed, indexBufferShadowed )
                .first );
        pMesh->load();
        return pMesh;
    }
    //-----------------------------------------------------------------------
    MeshPtr MeshManager::create( const String &name, const String &group, bool isManual,
                                 ManualResourceLoader *loader, const NameValuePairList *createParams )
    {
        return std::static_pointer_cast<Mesh>(
            createResource( name, group, isManual, loader, createParams ) );
    }
    //-----------------------------------------------------------------------
    MeshPtr MeshManager::createManual( const String &name, const String &groupName,
                                       ManualResourceLoader *loader )
    {
        // Don't try to get existing, create should fail if already exists
        if( this->getResourceByName( name, groupName ) )
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_DUPLICATE_ITEM,
                         "v2 Mesh with name '" + name + "' already exists.",
                         "MeshManager::createManual" );
        }
        return create( name, groupName, true, loader );
    }
    //-------------------------------------------------------------------------
    MeshPtr MeshManager::createByImportingV1( const String &name, const String &groupName,
                                              v1::Mesh *mesh, bool halfPos, bool halfTexCoords,
                                              bool qTangents, bool halfPose )
    {
        // Create manual mesh which calls back self to load
        MeshPtr pMesh = createManual( name, groupName, this );
        // store parameters
        V1MeshImportParams params;
        params.name = mesh->getName();
        params.groupName = mesh->getGroup();
        params.halfPos = halfPos;
        params.halfTexCoords = halfTexCoords;
        params.qTangents = qTangents;
        params.halfPose = halfPose;
        mV1MeshImportParams[pMesh.get()] = params;

        return pMesh;
    }
    //-------------------------------------------------------------------------
    void MeshManager::loadResource( Resource *res )
    {
        Mesh *mesh = static_cast<Mesh *>( res );

        // Find build parameters
        V1MeshImportParamsMap::iterator it = mV1MeshImportParams.find( res );
        if( it == mV1MeshImportParams.end() )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot find build parameters for " + res->getName(),
                         "MeshManager::loadResource" );
        }
        V1MeshImportParams &params = it->second;

        Ogre::v1::MeshPtr meshV1 =
            Ogre::v1::MeshManager::getSingleton().getByName( params.name, params.groupName );

        bool unloadV1 =
            ( meshV1->isReloadable() && meshV1->getLoadingState() == Resource::LOADSTATE_UNLOADED );

        mesh->importV1( meshV1.get(), params.halfPos, params.halfTexCoords, params.qTangents,
                        params.halfPose );

        if( unloadV1 )
            meshV1->unload();
    }
    //-----------------------------------------------------------------------
    Real MeshManager::getBoundsPaddingFactor() { return mBoundsPaddingFactor; }
    //-----------------------------------------------------------------------
    void MeshManager::setBoundsPaddingFactor( Real paddingFactor )
    {
        mBoundsPaddingFactor = paddingFactor;
    }
    //-----------------------------------------------------------------------
    Resource *MeshManager::createImpl( const String &name, ResourceHandle handle, const String &group,
                                       bool isManual, ManualResourceLoader *loader,
                                       const NameValuePairList *createParams )
    {
        // no use for createParams here
        return OGRE_NEW Mesh( this, name, handle, group, mVaoManager, isManual, loader );
    }
    //-----------------------------------------------------------------------
}  // namespace Ogre
