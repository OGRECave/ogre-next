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
    */
    class _OgreExport VertexFormatWarmUpStorage
    {
        struct VertexFormatEntry
        {
            struct VFPair
            {
                OperationType        opType;
                OperationType        opTypeShadow;
                VertexElement2VecVec normal;
                VertexElement2VecVec shadow;

                bool operator!=( const VFPair &other ) const
                {
                    return this->opType != other.opType || this->opTypeShadow != other.opTypeShadow ||
                           this->normal != other.normal || this->shadow != other.shadow;
                }

                bool operator<( const VFPair &other ) const
                {
                    if( this->opType < other.opType )
                        return true;
                    if( this->opTypeShadow < other.opTypeShadow )
                        return true;
                    if( this->normal < other.normal )
                        return true;
                    return this->shadow < other.shadow;
                }
            };

            VFPair            vertexFormats;
            FastArray<String> materials;
            // We don't keep seenHlmsShadowHashes because Hlms implementations guarantee
            // That the shadow Hlms hashes will be the same if the base one are the same.
            std::set<uint32> seenHlmsHashes;

            std::vector<WarmUpRenderable *> renderables;

            bool operator<( const VFPair &other ) const { return this->vertexFormats < other; }
        };

        typedef std::vector<VertexFormatEntry> VertexFormatEntryVec;

        VertexFormatEntryVec mEntries;

        void analyze( const Renderable *renderable );

        void save( DataStreamPtr &dataStream, const VertexElement2VecVec &vertexElements );
        void load( DataStreamPtr &dataStream, VertexElement2VecVec &outVertexElements );

    public:
        ~VertexFormatWarmUpStorage();

        void analyze( SceneManager *sceneManager );

        void saveTo( DataStreamPtr &dataStream );
        void loadFrom( DataStreamPtr &dataStream );

        void createWarmUp( SceneManager *sceneManager, uint8 renderQueueIdForV2 = 10u );
        void destroyWarmUp();
    };

    OGRE_ASSUME_NONNULL_END
};  // namespace Ogre

#endif
