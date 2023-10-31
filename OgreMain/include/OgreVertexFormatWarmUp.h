/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2023 Torus Knot Software Ltd

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

#ifndef OgreVertexFormatWarmUp_H
#define OgreVertexFormatWarmUp_H

#include "OgreCommon.h"
#include "OgreIdString.h"
#include "Vao/OgreVertexBufferPacked.h"

#include <set>

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    class WarmUpRenderable;

    /** The purpose of this class is to trigger shader compilations on demand (e.g. at startup
        or at loading time) without having to load meshes and skeletons.

        Normally to warm up all shader caches, you'd have to create an Item of each Mesh + Material
        combo that is needed.

        This is tedious to keep by hand and can increase loading time and RAM consumption because a
        significant time is spent on loading meshes and skeletons.

        This class simplifies that task by splitting up it up in parts:

        ## Part 1:
            1. Create a scene with all objects you plan on ever using (or at least as much as possible).
            2. Run VertexFormatWarmUpStorage::analyze.
            3. Save the results to file via VertexFormatWarmUpStorage::saveTo.

        ## Part 2:
            1. Load the saved file via VertexFormatWarmUpStorage::loadFrom.
            2. Call VertexFormatWarmUpStorage::createWarmUp.
            3. Render one frame (or multiple ones). Either directly to the screen or with the help of
               warm_up compositor passeses (see CompositorPassWarmUpDef and see WarmUpHelper).
            4. Destroy the VertexFormatWarmUpStorage.

        What VertexFormatWarmUpStorage in Part I simply does is to collect all useful combos of
        Vertex_Format + Material settings and saves them to file.

        @note Many Materials when applied to the same Vertex_Format may result in the exact same shader.
        In that case, the VertexFormatWarmUpStorage will only save one material (the first one it sees
        with for that combo). It doesn't waste time saving all materials.

        In Part II, VertexFormatWarmUpStorage will create a very small vertex buffer with the same
        vertex format and apply the material to it. This way attempting to render will result in nothing
        on screen (because it's all 0s and degenerate triangles), the shader gets parsed and the hit
        to VRAM and disk loading times are minimum.
    */
    class _OgreExport VertexFormatWarmUpStorage
    {
        struct VertexFormatEntry
        {
            struct VFPair
            {
                bool                 hasSkeleton;
                OperationType        opType;
                OperationType        opTypeShadow;
                VertexElement2VecVec normal;
                VertexElement2VecVec shadow;

                bool operator!=( const VFPair &other ) const
                {
                    return this->hasSkeleton != other.hasSkeleton || this->opType != other.opType ||
                           this->opTypeShadow != other.opTypeShadow || this->normal != other.normal ||
                           this->shadow != other.shadow;
                }

                bool operator<( const VFPair &other ) const
                {
                    if( this->hasSkeleton < other.hasSkeleton )
                        return true;
                    if( this->opType < other.opType )
                        return true;
                    if( this->opTypeShadow < other.opTypeShadow )
                        return true;
                    if( this->normal < other.normal )
                        return true;
                    return this->shadow < other.shadow;
                }
            };

            struct MaterialRqId
            {
                uint8  renderQueueId;
                String name;
            };

            VFPair                    vertexFormats;
            std::vector<MaterialRqId> materials;
            // We don't keep 'seenHlmsShadowHashes' because Hlms implementations guarantee
            // That the shadow Hlms hashes will be the same if the base one are the same.
            // The hash is actually 48 bits:
            //  32 bits for the hlms hash
            //   8 bits for the render queue ID.
            std::set<uint64> seenHlmsHashes;

            std::vector<WarmUpRenderable *> renderables;

            bool operator<( const VFPair &other ) const { return this->vertexFormats < other; }
        };

        typedef std::vector<VertexFormatEntry> VertexFormatEntryVec;

        VertexFormatEntryVec mEntries;

        typedef FastArray<unsigned short> IndexMap;

        bool mNeedsSkeleton;
        /// Dummy used by all entries that need a skeleton.
        SkeletonInstance *mSkeleton;
        IndexMap          mBlendIndexToBoneIndexMap;

        void createSkeleton( SceneManager *sceneManager );
        void destroySkeleton( SceneManager *sceneManager );

        void analyze( uint8 renderQueueId, const Renderable *renderable );

        void save( DataStreamPtr &dataStream, const VertexElement2VecVec &vertexElements );
        void load( DataStreamPtr &dataStream, VertexElement2VecVec &outVertexElements );

    public:
        VertexFormatWarmUpStorage();
        ~VertexFormatWarmUpStorage();

        void analyze( SceneManager *sceneManager );

        void saveTo( DataStreamPtr &dataStream );
        void loadFrom( DataStreamPtr &dataStream );

        void createWarmUp( SceneManager *sceneManager );
        void destroyWarmUp();
    };

    OGRE_ASSUME_NONNULL_END
};  // namespace Ogre

#endif
