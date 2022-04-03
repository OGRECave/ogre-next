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
#ifndef __SubMesh_H_
#define __SubMesh_H_

#include "OgrePrerequisites.h"

#include "OgreAnimationTrack.h"
#include "OgreRenderOperation.h"
#include "OgreResourceGroupManager.h"
#include "OgreVertexBoneAssignment.h"
#include "Vao/OgreVertexArrayObject.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    namespace v1
    {
        /** \addtogroup Core
         *  @{
         */
        /** \addtogroup Resources
         *  @{
         */
        /** Defines a part of a complete mesh.
            @remarks
                Meshes which make up the definition of a discrete 3D object
                are made up of potentially multiple parts. This is because
                different parts of the mesh may use different materials or
                use different vertex formats, such that a rendering state
                change is required between them.
            @par
                Like the Mesh class, instantiations of 3D objects in the scene
                share the SubMesh instances, and have the option of overriding
                their material differences on a per-object basis if required.
                See the SubEntity class for more information.
        */
        class _OgreExport SubMesh : public OgreAllocatedObj
        {
            friend class Mesh;
            friend class MeshSerializerImpl;
            friend class MeshSerializerImpl_v1_2;
            friend class MeshSerializerImpl_v1_1;

        public:
            SubMesh();
            ~SubMesh();

            /// Indicates if this submesh shares vertex data with other meshes or whether it has it's own
            /// vertices.
            bool useSharedVertices;

            uint32 renderOpMeshIndex;

            /// The render operation type used to render this submesh
            OperationType operationType;

            /** Dedicated vertex data (only valid if useSharedVertices = false).
                [0] is for the regular pass, [1] is for the caster. Note that
                vertexData[0] = vertexData[1] is possible, but if one
                submesh has vertexData[0] = vertexData[1], then all submeshes in the mesh
                must have vertexData[0] = vertexData[1] as well for the sake of simplicity.
                @remarks
                    This data is completely owned by this submesh.
                @par
                    The use of shared or non-shared buffers is determined when
                    model data is converted to the OGRE .mesh format.
            */
            VertexData *vertexData[NumVertexPass];

            /// Face index data
            IndexData *indexData[NumVertexPass];

            /** Dedicated index map for translate blend index to bone index (only valid if
               useSharedVertices = false).
                @remarks
                    This data is completely owned by this submesh.
                @par
                    We collect actually used bones of all bone assignments, and build the
                    blend index in 'packed' form, then the range of the blend index in vertex
                    data VES_BLEND_INDICES element is continuous, with no gaps. Thus, by
                    minimising the world matrix array constants passing to GPU, we can support
                    more bones for a mesh when hardware skinning is used. The hardware skinning
                    support limit is applied to each set of vertex data in the mesh, in other words, the
                    hardware skinning support limit is applied only to the actually used bones of each
                    SubMeshes, not all bones across the entire Mesh.
                @par
                    Because the blend index is different to the bone index, therefore, we use
                    the index map to translate the blend index to bone index.
                @par
                    The use of shared or non-shared index map is determined when
                    model data is converted to the OGRE .mesh format.
            */
            typedef FastArray<unsigned short> IndexMap;
            IndexMap                          blendIndexToBoneIndexMap;

            typedef vector<IndexData *>::type LODFaceList;
            LODFaceList                       mLodFaceList[NumVertexPass];

            /** A list of extreme points on the submesh (optional).
                @remarks
                    These points are some arbitrary points on the mesh that are used
                    by engine to better sort submeshes by depth. This doesn't matter
                    much for non-transparent submeshes, as Z-buffer takes care of invisible
                    surface culling anyway, but is pretty useful for semi-transparent
                    submeshes because the order in which transparent submeshes must be
                    rendered cannot be always correctly deduced from entity position.
                @par
                    These points are intelligently chosen from the points that make up
                    the submesh, the criteria for choosing them should be that these points
                    somewhat characterize the submesh outline, e.g. they should not be
                    close to each other, and they should be on the outer hull of the submesh.
                    They can be stored in the .mesh file, or generated at runtime
                    (see generateExtremes ()).
                @par
                    If this array is empty, submesh sorting is done like in older versions -
                    by comparing the positions of the owning entity.
             */
            vector<Vector3>::type extremityPoints;

            /// Reference to parent Mesh (not a smart pointer so child does not keep parent alive).
            Mesh *parent;

            /// Sets the name of the Material which this SubMesh will use
            void setMaterialName(
                const String &matName,
                const String &groupName = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
            const String &getMaterialName() const;

            /** Returns true if a material has been assigned to the submesh, otherwise returns false.
             */
            bool isMatInitialised() const;

            /** Returns a RenderOperation structure required to render this mesh.
                @param
                    rend Reference to a RenderOperation structure to populate.
                @param
                    lodIndex The index of the LOD to use.
            */
            void _getRenderOperation( RenderOperation &rend, ushort lodIndex, bool casterPass );

            /** Assigns a vertex to a bone with a given weight, for skeletal animation.
            @remarks
                This method is only valid after calling setSkeletonName.
                Since this is a one-off process there exists only 'addBoneAssignment' and
                'clearBoneAssignments' methods, no 'editBoneAssignment'. You should not need
                to modify bone assignments during rendering (only the positions of bones) and OGRE
                reserves the right to do some internal data reformatting of this information, depending
                on render system requirements.
            @par
                This method is for assigning weights to the dedicated geometry of the SubMesh. To assign
                weights to the shared Mesh geometry, see the equivalent methods on Mesh.
            */
            void addBoneAssignment( const VertexBoneAssignment &vertBoneAssign );

            /** Removes all bone assignments for this mesh.
            @par
                This method is for assigning weights to the dedicated geometry of the SubMesh. To assign
                weights to the shared Mesh geometry, see the equivalent methods on Mesh.
            */
            void clearBoneAssignments();

            /// Multimap of verex bone assignments (orders by vertex index)
            typedef multimap<size_t, VertexBoneAssignment>::type VertexBoneAssignmentList;
            typedef MapIterator<VertexBoneAssignmentList>        BoneAssignmentIterator;

            /** Gets an iterator for access all bone assignments.
            @remarks
                Only valid if this SubMesh has dedicated geometry.
            */
            BoneAssignmentIterator getBoneAssignmentIterator();

            /** Gets a const reference to the list of bone assignments
             */
            const VertexBoneAssignmentList &getBoneAssignments() { return mBoneAssignments; }

            /** Must be called once to compile bone assignments into geometry buffer. */
            void _compileBoneAssignments();

            typedef ConstMapIterator<AliasTextureNamePairList> AliasTextureIterator;
            /** Gets an constant iterator to access all texture alias names assigned to this submesh.

            */
            AliasTextureIterator getAliasTextureIterator() const;
            /** Adds the alias or replaces an existing one and associates the texture name to it.
            @remarks
              The submesh uses the texture alias to replace textures used in the material applied
              to the submesh.
            @param
                aliasName is the name of the alias.
            @param
                textureName is the name of the texture to be associated with the alias

            */
            void addTextureAlias( const String &aliasName, const String &textureName );
            /** Remove a specific texture alias name from the sub mesh
            @param
                aliasName is the name of the alias to be removed.  If it is not found
                then it is ignored.
            */
            void removeTextureAlias( const String &aliasName );
            /** removes all texture aliases from the sub mesh
             */
            void removeAllTextureAliases();
            /** returns true if the sub mesh has texture aliases
             */
            bool hasTextureAliases() const { return !mTextureAliases.empty(); }
            /** Gets the number of texture aliases assigned to the sub mesh.
             */
            size_t getTextureAliasCount() const { return mTextureAliases.size(); }

            /**  The current material used by the submesh is copied into a new material
                and the submesh's texture aliases are applied if the current texture alias
                names match those found in the original material.
            @remarks
                The submesh's texture aliases must be setup prior to calling this method.
                If a new material has to be created, the subMesh autogenerates the new name.
                The new name is the old name + "_" + number.
            @return
                True if texture aliases were applied and a new material was created.
            */
            bool updateMaterialUsingTextureAliases();

            /** Get the type of any vertex animation used by dedicated geometry.
             */
            VertexAnimationType getVertexAnimationType() const;

            /// Returns whether animation on dedicated vertex data includes normals
            bool getVertexAnimationIncludesNormals() const { return mVertexAnimationIncludesNormals; }

            /** Generate the submesh extremes (@see extremityPoints).
            @param count
                Number of extreme points to compute for the submesh.
            */
            void generateExtremes( size_t count );

            /** Returns true(by default) if the submesh should be included in the mesh EdgeList,
             * otherwise returns false.
             */
            bool isBuildEdgesEnabled() const { return mBuildEdgesEnabled; }
            void setBuildEdgesEnabled( bool b );
            /** Makes a copy of this submesh object and gives it a new name.
             @param newName
             The name to give the clone.
             @param parentMesh
             Optional mesh to make the parent of the newly created clone.
             If you leave this blank, the clone will be parented to the same Mesh as the original.
             */
            SubMesh *clone( const String &newName, Mesh *parentMesh = 0 );

            /// Imports a v2 SubMesh @See Mesh::importV2.
            void importFromV2( Ogre::SubMesh *subMesh );

            void arrangeEfficient( bool halfPos, bool halfTexCoords, bool qTangents );

            void dearrangeToInefficient();

        protected:
            /// @See v1::Mesh::arrangeEfficient
            void arrangeEfficient( bool halfPos, bool halfTexCoords, bool qTangents, size_t vaoPassIdx );

        protected:
            /// Name of the material this SubMesh uses.
            String mMaterialName;

            /// Is there a material yet?
            bool mMatInitialised;

            /// paired list of texture aliases and texture names
            AliasTextureNamePairList mTextureAliases;

            VertexBoneAssignmentList mBoneAssignments;

            /// Flag indicating that bone assignments need to be recompiled
            bool mBoneAssignmentsOutOfDate;

            /// Type of vertex animation for dedicated vertex data (populated by Mesh)
            mutable VertexAnimationType mVertexAnimationType;

            /// Whether normals are included in vertex animation keyframes
            mutable bool mVertexAnimationIncludesNormals;

            /// Is Build Edges Enabled
            bool mBuildEdgesEnabled;

            /// Internal method for removing LOD data
            void removeLodLevels();

            static void removeLodLevel( LODFaceList &lodList );

            /** Rearranges the buffers to be efficiently rendered in Ogre 2.0 with Hlms
            @remarks
                vertexData->vertexDeclaration is modified and vertexData->vertexBufferBinding
                is destroyed. Caller must reallocate the vertex buffer filled with the returned
                pointer
            @param halfPos
                @See Mesh::arrangeEfficientFor
            @param halfTexCoords
                @See Mesh::arrangeEfficientFor
            @param outVertexElements [out]
                Description of the buffer in the new Vao system. Matches the same as
                vertexData->vertexDeclaration, provided as out param as convenience.
                Can be null.
            @return
                Buffer pointer with reorganized data.
                Caller MUST free the pointer with OGRE_FREE_SIMD( MEMCATEGORY_GEOMETRY ).
            */
            char *arrangeEfficient( bool halfPos, bool halfTexCoords,
                                    VertexElement2Vec *outVertexElements );
        };
        /** @} */
        /** @} */

    }  // namespace v1
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
