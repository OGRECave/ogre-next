/*
 * -----------------------------------------------------------------------------
 * This source file is part of OGRE-Next
 * (Object-oriented Graphics Rendering Engine)
 * For the latest info, see http://www.ogre3d.org/
 *
 * Copyright (c) 2000-2014 Torus Knot Software Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -----------------------------------------------------------------------------
 */

#include "OgreLodOutputProviderMeshV2.h"

#include "OgreException.h"
#include "OgreLodData.h"
#include "OgreLogManager.h"
#include "OgreMesh2.h"
#include "OgreStringConverter.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

namespace Ogre
{
    void LodOutputProviderMeshV2::prepare( LodData *data )
    {
        OGRE_UNUSED_VAR( data );

        // Nothing to pre-size: unlike v1's LODFaceList (which is resized up front),
        // we append a new VAO per LOD level directly onto mVao[...] as each level is
        // baked. LOD-0 already exists untouched -- it's the mesh as it was loaded.
        unsigned submeshCount = mMesh->getNumSubMeshes();
        for( unsigned i = 0; i < submeshCount; ++i )
        {
            SubMesh *subMesh = mMesh->getSubMesh( i );
            OgreAssert( !subMesh->mVao[VpNormal].empty(),
                        "SubMesh has no LOD-0 Vao to base generated LODs on." );
        }
    }

    void LodOutputProviderMeshV2::bakeManualLodLevel( LodData *data, String &manualMeshName,
                                                      int lodIndex )
    {
        OGRE_UNUSED_VAR( data );
        OGRE_UNUSED_VAR( lodIndex );

        // Manual (mesh-swap) LOD levels are out of scope for this provider: they
        // require an entirely separate source mesh's geometry to be merged in as a
        // new Vao, which is a distinct feature from collapse-generated LOD and adds
        // significant complexity (a different mesh likely has different vertex
        // layout, bone assignments, etc). Left as explicit future work rather than
        // silently producing an incorrect/empty LOD level.
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "Manual LOD levels (manualMeshName = '" + manualMeshName +
                         "') are not supported by LodOutputProviderMeshV2. "
                         "Use generated (collapse-based) LOD levels only.",
                     "LodOutputProviderMeshV2::bakeManualLodLevel" );
    }

    IndexBufferPacked *LodOutputProviderMeshV2::buildIndexBufferForSubmesh( LodData *data,
                                                                            unsigned submeshID )
    {
        VaoManager *vaoManager = mMesh->_getVaoManager();
        const size_t indexSize = data->mIndexBufferInfoList[submeshID].indexSize;
        size_t indexCount = data->mIndexBufferInfoList[submeshID].indexCount;

        // Mirrors LodOutputProviderMesh::bakeLodLevel's v1 handling: an empty index
        // buffer can crash some render systems, so keep a single dummy (zeroed)
        // triangle instead of a truly empty buffer.
        const bool isDummyTriangle = ( indexCount == 0 );
        if( isDummyTriangle )
            indexCount = 3;

        void *indexDataPtr = OGRE_MALLOC_SIMD( indexCount * indexSize, MEMCATEGORY_GEOMETRY );
        FreeOnDestructor indexDataPtrContainer( indexDataPtr );

        if( isDummyTriangle )
        {
            memset( indexDataPtr, 0, indexCount * indexSize );
        }
        else
        {
            data->mIndexBufferInfoList[submeshID].buf.pshort =
                static_cast<unsigned short *>( indexDataPtr );

            const size_t triangleCount = data->mTriangleList.size();
            for( size_t i = 0; i < triangleCount; ++i )
            {
                if( data->mTriangleList[i].isRemoved() )
                    continue;
                if( data->mTriangleList[i].submeshID() != submeshID )
                    continue;

                if( indexSize == 2u )
                {
                    for( int m = 0; m < 3; ++m )
                    {
                        *( data->mIndexBufferInfoList[submeshID].buf.pshort++ ) =
                            static_cast<unsigned short>( data->mTriangleList[i].vertexID[m] );
                    }
                }
                else
                {
                    for( int m = 0; m < 3; ++m )
                    {
                        *( data->mIndexBufferInfoList[submeshID].buf.pint++ ) =
                            static_cast<unsigned int>( data->mTriangleList[i].vertexID[m] );
                    }
                }
            }
        }

        const IndexBufferPacked::IndexType indexType =
            ( indexSize == 2u ) ? IndexBufferPacked::IT_16BIT : IndexBufferPacked::IT_32BIT;
        const bool keepAsShadow = mMesh->isIndexBufferShadowed();

        IndexBufferPacked *indexBuffer = vaoManager->createIndexBuffer(
            indexType, indexCount, mMesh->getIndexBufferDefaultType(), indexDataPtr, keepAsShadow );

        if( keepAsShadow )  // Don't free the pointer ourselves; the buffer now owns it.
            indexDataPtrContainer.ptr = 0;

        return indexBuffer;
    }

    void LodOutputProviderMeshV2::bakeLodLevel( LodData *data, int lodIndex )
    {
        OGRE_UNUSED_VAR( lodIndex );

        VaoManager *vaoManager = mMesh->_getVaoManager();
        unsigned submeshCount = mMesh->getNumSubMeshes();

        for( unsigned i = 0; i < submeshCount; ++i )
        {
            SubMesh *subMesh = mMesh->getSubMesh( i );
            VertexArrayObject *lod0Vao = subMesh->mVao[VpNormal][0];

            IndexBufferPacked *newIndexBuffer = buildIndexBufferForSubmesh( data, i );

            // LOD generation never touches vertex data -- only which triangles
            // survive -- so the new VAO reuses LOD-0's vertex buffers untouched,
            // exactly like SubMesh::importBuffersFromV1 does for v1-imported LODs.
            VertexArrayObject *newVao = vaoManager->createVertexArrayObject(
                lod0Vao->getVertexBuffers(), newIndexBuffer, lod0Vao->getOperationType() );

            subMesh->mVao[VpNormal].push_back( newVao );

            const bool shadowSharesNormal =
                !subMesh->mVao[VpShadow].empty() && subMesh->mVao[VpShadow][0] == lod0Vao;

            if( shadowSharesNormal )
            {
                // Common case (prepareForShadowMapping(false), the default): shadow
                // and regular rendering already share the same Vaos, so keep sharing
                // for this new LOD level too.
                subMesh->mVao[VpShadow].push_back( newVao );
            }
            else if( !subMesh->mVao[VpShadow].empty() )
            {
                // Independent shadow-mapping geometry (prepareForShadowMapping(true))
                // is out of scope here: its vertex buffer may be reordered/deduped
                // relative to the regular Vao, so this LOD level's triangle list
                // (built against the regular Vao's vertex ordering) cannot be reused
                // for it as-is. Call Mesh::prepareForShadowMapping() again after
                // generating LODs to rebuild a consistent independent shadow Vao set
                // across all LOD levels, rather than silently leaving mVao[VpShadow]
                // shorter than mVao[VpNormal] (hasValidShadowMappingVaos() would then
                // correctly report false).
                LogManager::getSingleton().logMessage(
                    "WARNING: LodOutputProviderMeshV2::bakeLodLevel: submesh " +
                    StringConverter::toString( i ) + " of mesh '" + mMesh->getName() +
                    "' has independent shadow-mapping Vaos. Call "
                    "Mesh::prepareForShadowMapping() again after LOD generation to "
                    "rebuild shadow Vaos for the new LOD levels." );
            }
        }
    }

}  // namespace Ogre