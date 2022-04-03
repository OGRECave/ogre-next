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
#ifndef __SubItem_H__
#define __SubItem_H__

#include "OgrePrerequisites.h"

#include "OgreHardwareBufferManager.h"
#include "OgreRenderable.h"
#include "OgreResourceGroupManager.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Scene
     *  @{
     */
    /** Utility class which defines the sub-parts of an Item.
        @remarks
            Just as meshes are split into submeshes, an Item is made up of
            potentially multiple SubMeshes. These are mainly here to provide the
            link between the Material which the SubItem uses (which may be the
            default Material for the SubMesh or may have been changed for this
            object) and the SubMesh data.
        @par
            The SubItem also allows the application some flexibility in the
            material properties for this section of a particular instance of this
            Mesh, e.g. tinting the windows on a car model.
        @par
            SubItem instances are never created manually. They are created at
            the same time as their parent Item by the SceneManager method
            createItem.
    */
    class _OgreExport SubItem : public RenderableAnimated, public OgreAllocatedObj
    {
        // Note no virtual functions for efficiency
        friend class Item;
        friend class SceneManager;

    protected:
        /** Private constructor - don't allow creation by anybody else.
         */
        SubItem( Item *parent, SubMesh *subMeshBasis );

    public:
        /** Destructor.
         */
        ~SubItem() override;

    protected:
        /// Pointer to parent.
        Item *mParentItem;

        /// Pointer to the SubMesh defining geometry.
        SubMesh      *mSubMesh;
        unsigned char mMaterialLodIndex;

    public:
        /** Accessor method to read mesh data.
         */
        SubMesh *getSubMesh() const;

        void _setHlmsHashes( uint32 hash, uint32 casterHash ) override;

        /** Accessor to get parent Item */
        Item *getParent() const { return mParentItem; }

        /** @copydoc Renderable::getLights */
        const LightList &getLights() const override;

        void getRenderOperation( v1::RenderOperation &op, bool casterPass ) override;
        void getWorldTransforms( Matrix4 *xform ) const override;
        bool getCastsShadows() const override;

        // needs this to not hide the base class' methods with same name
        using Renderable::addPoseWeight;
        using Renderable::getPoseWeight;
        using Renderable::setPoseWeight;

        float getPoseWeight( const Ogre::String &poseName ) const;
        void  setPoseWeight( const Ogre::String &poseName, float w );
        void  addPoseWeight( const Ogre::String &poseName, float w );
    };
    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
