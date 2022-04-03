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
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#include "OgreStableHeaders.h"

#include "OgreSimpleRenderable.h"

#include "OgreException.h"
#include "OgreMaterialManager.h"
#include "OgreRenderQueue.h"

namespace Ogre
{
    namespace v1
    {
        SimpleRenderable::SimpleRenderable( IdType id, ObjectMemoryManager *objectMemoryManager,
                                            SceneManager *manager ) :
            MovableObject( id, objectMemoryManager, manager, 110u ),
            mWorldTransform( Matrix4::IDENTITY ),
            mMatName( "BaseWhite" ),
            mMaterial( MaterialManager::getSingleton().getByName( "BaseWhite" ) ),
            mParentSceneManager( NULL )
        {
        }

        void SimpleRenderable::setMaterial( const String &matName )
        {
            mMatName = matName;
            mMaterial = MaterialManager::getSingleton().getByName( mMatName );
            if( !mMaterial )
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Could not find material " + mMatName,
                             "SimpleRenderable::setMaterial" );

            // Won't load twice anyway
            mMaterial->load();
        }

        void SimpleRenderable::setMaterial( const MaterialPtr &mat )
        {
            mMatName = mat->getName();
            mMaterial = mat;
            // Won't load twice anyway
            mMaterial->load();
        }

        const MaterialPtr &SimpleRenderable::getMaterial() const { return mMaterial; }

        void SimpleRenderable::getRenderOperation( RenderOperation &op, bool casterPass )
        {
            op = mRenderOp;
        }

        void SimpleRenderable::setRenderOperation( const RenderOperation &rend ) { mRenderOp = rend; }

        void SimpleRenderable::setWorldTransform( const Matrix4 &xform ) { mWorldTransform = xform; }

        void SimpleRenderable::getWorldTransforms( Matrix4 *xform ) const
        {
            *xform = mWorldTransform * mParentNode->_getFullTransform();
        }

        void SimpleRenderable::setBoundingBox( const AxisAlignedBox &box ) { mBox = box; }

        const AxisAlignedBox &SimpleRenderable::getBoundingBox() const { return mBox; }

        SimpleRenderable::~SimpleRenderable() {}
        //-----------------------------------------------------------------------
        const String &SimpleRenderable::getMovableType() const
        {
            static String movType = "SimpleRenderable";
            return movType;
        }
        //-----------------------------------------------------------------------
        const LightList &SimpleRenderable::getLights() const
        {
            // Use movable query lights
            return queryLights();
        }
    }  // namespace v1
}  // namespace Ogre
