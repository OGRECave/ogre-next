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
#ifndef __SubEntity_H__
#define __SubEntity_H__

#include "OgrePrerequisites.h"

#include "OgreHardwareBufferManager.h"
#include "OgreRenderable.h"
#include "OgreResourceGroupManager.h"

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
        /** Utility class which defines the sub-parts of an Entity.
            @remarks
                Just as meshes are split into submeshes, an Entity is made up of
                potentially multiple SubMeshes. These are mainly here to provide the
                link between the Material which the SubEntity uses (which may be the
                default Material for the SubMesh or may have been changed for this
                object) and the SubMesh data.
            @par
                The SubEntity also allows the application some flexibility in the
                material properties for this section of a particular instance of this
                Mesh, e.g. tinting the windows on a car model.
            @par
                SubEntity instances are never created manually. They are created at
                the same time as their parent Entity by the SceneManager method
                createEntity.
        */
        class _OgreExport SubEntity : public Renderable, public OgreAllocatedObj
        {
            // Note no virtual functions for efficiency
            friend class Entity;
            friend class SceneManager;

        protected:
            /** Private constructor - don't allow creation by anybody else.
             */
            SubEntity( Entity *parent, SubMesh *subMeshBasis );

        public:
            /** Destructor.
             */
            ~SubEntity() override;

        protected:
            /// Pointer to parent.
            Entity *mParentEntity;

            /// Pointer to the SubMesh defining geometry.
            SubMesh *mSubMesh;

            unsigned char mMaterialLodIndex;

            /// override the start index for the RenderOperation
            size_t mIndexStart;

            /// override the end index for the RenderOperation
            size_t mIndexEnd;

            /// Blend buffer details for dedicated geometry
            VertexData *mSkelAnimVertexData;
            /// Quick lookup of buffers
            TempBlendedBufferInfo mTempSkelAnimInfo;
            /// Temp buffer details for software Vertex anim geometry
            TempBlendedBufferInfo mTempVertexAnimInfo;
            /// Vertex data details for software Vertex anim of shared geometry
            VertexData *mSoftwareVertexAnimVertexData;
            /// Vertex data details for hardware Vertex anim of shared geometry
            /// - separate since we need to s/w anim for shadows whilst still altering
            ///   the vertex data for hardware morphing (pos2 binding)
            VertexData *mHardwareVertexAnimVertexData;
            /// Have we applied any vertex animation to geometry?
            bool mVertexAnimationAppliedThisFrame;
            /// Number of hardware blended poses supported by material
            ushort mHardwarePoseCount;

            /** Internal method for preparing this Entity for use in animation. */
            void prepareTempBlendBuffers();

        public:
            /** Sets a Material to be used.
                @remarks
                    By default a SubEntity uses the default Material that the SubMesh
                    uses. This call can alter that so that the Material is different
                    for this instance.
            */
            void setMaterial( const MaterialPtr &material ) override;

            /** Make every setDatablock method from Renderable available.
                See http://www.research.att.com/~bs/bs_faq2.html#overloadderived
            */
            using Renderable::setDatablock;

            void setDatablock( HlmsDatablock *datablock ) override;
            void _setNullDatablock() override;

            /** Accessor method to read mesh data.
             */
            SubMesh *getSubMesh() const;

            /** Accessor to get parent Entity */
            Entity *getParent() const { return mParentEntity; }

            /** Overridden - see Renderable.
             */
            void getRenderOperation( RenderOperation &op, bool casterPass ) override;

            /** Tells this SubEntity to draw a subset of the SubMesh by adjusting the index buffer
             * extents. Default value is zero so that the entire index buffer is used when drawing. Valid
             * values are zero to getIndexDataEndIndex()
             */
            void setIndexDataStartIndex( size_t start_index );

            /** Returns the current value of the start index used for drawing.
             * \see setIndexDataStartIndex
             */
            size_t getIndexDataStartIndex() const;

            /** Tells this SubEntity to draw a subset of the SubMesh by adjusting the index buffer
             * extents. Default value is SubMesh::indexData::indexCount so that the entire index buffer
             * is used when drawing. Valid values are mStartIndex to SubMesh::indexData::indexCount
             */
            void setIndexDataEndIndex( size_t end_index );

            /** Returns the current value of the start index used for drawing.
             */
            size_t getIndexDataEndIndex() const;

            /** Reset the custom start/end index to the default values.
             */
            void resetIndexDataStartEndIndex();

            /** Overridden - see Renderable.
             */
            void getWorldTransforms( Matrix4 *xform ) const override;
            /** Overridden - see Renderable.
             */
            unsigned short getNumWorldTransforms() const override;
            /** Overridden, see Renderable */
            Real getSquaredViewDepth( const Camera *cam ) const;
            /** @copydoc Renderable::getLights */
            const LightList &getLights() const override;
            /** @copydoc Renderable::getCastsShadows */
            bool getCastsShadows() const override;
            /** Advanced method to get the temporarily blended vertex information
            for entities which are software skinned.
            @remarks
                Internal engine will eliminate software animation if possible, this
                information is unreliable unless added request for software animation
                via Entity::addSoftwareAnimationRequest.
            @note
                The positions/normals of the returned vertex data is in object space.
            */
            VertexData *_getSkelAnimVertexData();
            /** Advanced method to get the temporarily blended software morph vertex information
            @remarks
                Internal engine will eliminate software animation if possible, this
                information is unreliable unless added request for software animation
                via Entity::addSoftwareAnimationRequest.
            @note
                The positions/normals of the returned vertex data is in object space.
            */
            VertexData *_getSoftwareVertexAnimVertexData();
            /** Advanced method to get the hardware morph vertex information
            @note
                The positions/normals of the returned vertex data is in object space.
            */
            VertexData *_getHardwareVertexAnimVertexData();
            /** Advanced method to get the temp buffer information for software
            skeletal animation.
            */
            TempBlendedBufferInfo *_getSkelAnimTempBufferInfo();
            /** Advanced method to get the temp buffer information for software
            morph animation.
            */
            TempBlendedBufferInfo       *_getVertexAnimTempBufferInfo();
            const TempBlendedBufferInfo *_getVertexAnimTempBufferInfo() const;
            /// Retrieve the VertexData which should be used for GPU binding
            VertexData *getVertexDataForBinding( bool casterPass );

            /** Mark all vertex data as so far unanimated.
             */
            void _markBuffersUnusedForAnimation();
            /** Mark all vertex data as animated.
             */
            void _markBuffersUsedForAnimation();
            /** Are buffers already marked as vertex animated? */
            bool _getBuffersMarkedForAnimation() const { return mVertexAnimationAppliedThisFrame; }
            /** Internal method to copy original vertex data to the morph structures
            should there be no active animation in use.
            */
            void _restoreBuffersForUnusedAnimation( bool hardwareAnimation );

            /** Overridden from Renderable to provide some custom behaviour. */
            void _updateCustomGpuParameter( const GpuProgramParameters_AutoConstantEntry &constantEntry,
                                            GpuProgramParameters *params ) const override;
        };
        /** @} */
        /** @} */

    }  // namespace v1
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
