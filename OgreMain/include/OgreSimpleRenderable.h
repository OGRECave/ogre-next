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
#ifndef __SimpleRenderable_H__
#define __SimpleRenderable_H__

#include "OgrePrerequisites.h"

#include "OgreAxisAlignedBox.h"
#include "OgreMovableObject.h"
#include "OgreRenderOperation.h"
#include "OgreRenderable.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    namespace v1
    {
        /** \addtogroup Core
         *  @{
         */
        /** \addtogroup Scene
         *  @{
         */
        /** Simple implementation of MovableObject and Renderable for single-part custom objects.
        @see ManualObject for a simpler interface with more flexibility
        */
        class _OgreExport SimpleRenderable : public MovableObject, public Renderable
        {
        protected:
            RenderOperation mRenderOp;

            Matrix4        mWorldTransform;
            AxisAlignedBox mBox;

            String      mMatName;
            MaterialPtr mMaterial;

            /// The scene manager for the current frame.
            SceneManager *mParentSceneManager;

        public:
            /// Constructor
            SimpleRenderable( IdType id, ObjectMemoryManager *objectMemoryManager,
                              SceneManager *manager );
            ~SimpleRenderable() override;

            virtual void               setMaterial( const String &matName );
            void                       setMaterial( const MaterialPtr &mat ) override;
            virtual const MaterialPtr &getMaterial() const;

            virtual void setRenderOperation( const RenderOperation &rend );
            void         getRenderOperation( RenderOperation &op, bool casterPass ) override;

            void setWorldTransform( const Matrix4 &xform );
            void getWorldTransforms( Matrix4 *xform ) const override;

            void                          setBoundingBox( const AxisAlignedBox &box );
            virtual const AxisAlignedBox &getBoundingBox() const;

            /** Overridden from MovableObject */
            const String &getMovableType() const override;

            /** @copydoc Renderable::getLights */
            const LightList &getLights() const override;
        };
        /** @} */
        /** @} */
    }  // namespace v1
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
