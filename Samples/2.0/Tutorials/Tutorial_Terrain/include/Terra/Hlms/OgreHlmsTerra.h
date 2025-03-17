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
#ifndef _OgreHlmsTerra_H_
#define _OgreHlmsTerra_H_

#include "Terra/Hlms/OgreHlmsTerraPrerequisites.h"

#include "OgreHlmsPbs.h"

#include "OgreConstBufferPool.h"
#include "OgreHlmsBufferManager.h"
#include "OgreMatrix4.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class CompositorShadowNode;
    struct QueuedRenderable;

    class Terra;

    /** \addtogroup Component
     *  @{
     */
    /** \addtogroup Material
     *  @{
     */

    class HlmsTerraDatablock;

    /** Physically based shading implementation specfically designed for
        OpenGL 3+, D3D11 and other RenderSystems which support uniform buffers.
    */
    class HlmsTerra : public HlmsPbs
    {
        MovableObject const *mLastMovableObject;

        FastArray<Terra *> mLinkedTerras;

    protected:
        HlmsDatablock *createDatablockImpl( IdString datablockName, const HlmsMacroblock *macroblock,
                                            const HlmsBlendblock *blendblock,
                                            const HlmsParamVec   &paramVec ) override;

        void setDetailMapProperties( size_t tid, HlmsTerraDatablock *datablock, PiecesMap *inOutPieces );
        void setTextureProperty( size_t tid, const char *propertyName, HlmsTerraDatablock *datablock,
                                 TerraTextureTypes texType );
        void setDetailTextureProperty( size_t tid, const char *propertyName,
                                       HlmsTerraDatablock *datablock, TerraTextureTypes baseTexType,
                                       uint8 detailIdx );

        void calculateHashFor( Renderable *renderable, uint32 &outHash, uint32 &outCasterHash ) override;
        void calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces ) override;
        void calculateHashForPreCaster( Renderable *renderable, PiecesMap *inOutPieces,
                                        const PiecesMap *normalPassPieces ) override;

        PropertiesMergeStatus notifyPropertiesMergedPreGenerationStep( size_t     tid,
                                                                       PiecesMap *inOutPieces ) override;

        FORCEINLINE uint32 fillBuffersFor( const HlmsCache        *cache,
                                           const QueuedRenderable &queuedRenderable, bool casterPass,
                                           uint32 lastCacheHash, CommandBuffer *commandBuffer,
                                           bool isV1 );

    public:
        HlmsTerra( Archive *dataFolder, ArchiveVec *libraryFolders );
        ~HlmsTerra() override;

        const FastArray<Terra *> &getLinkedTerras() const { return mLinkedTerras; }

        void _linkTerra( Terra *terra );
        void _unlinkTerra( Terra *terra );

        void _changeRenderSystem( RenderSystem *newRs ) override;

        void analyzeBarriers( BarrierSolver &barrierSolver, ResourceTransitionArray &resourceTransitions,
                              Camera *renderingCamera, const bool bCasterPass ) override;

        uint32 fillBuffersFor( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                               bool casterPass, uint32 lastCacheHash, uint32 lastTextureHash ) override;

        uint32 fillBuffersForV1( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                 bool casterPass, uint32 lastCacheHash,
                                 CommandBuffer *commandBuffer ) override;
        uint32 fillBuffersForV2( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                 bool casterPass, uint32 lastCacheHash,
                                 CommandBuffer *commandBuffer ) override;

        static void getDefaultPaths( String &outDataFolderPath, StringVector &outLibraryFoldersPaths );

#if !OGRE_NO_JSON
        /// @copydoc Hlms::_loadJson
        void _loadJson( const rapidjson::Value &jsonValue, const HlmsJson::NamedBlocks &blocks,
                        HlmsDatablock *datablock, const String &resourceGroup,
                        HlmsJsonListener *listener,
                        const String     &additionalTextureExtension ) const override;
        /// @copydoc Hlms::_saveJson
        void _saveJson( const HlmsDatablock *datablock, String &outString, HlmsJsonListener *listener,
                        const String &additionalTextureExtension ) const override;

        /// @copydoc Hlms::_collectSamplerblocks
        void _collectSamplerblocks( set<const HlmsSamplerblock *>::type &outSamplerblocks,
                                    const HlmsDatablock                 *datablock ) const override;
#endif
    };

    struct TerraProperty
    {
        static const IdString UseSkirts;
        static const IdString ZUp;

        static const IdString NumTextures;
        static const char    *DiffuseMap;
        static const char    *EnvProbeMap;
        static const char    *DetailWeightMap;
        static const char    *DetailMapN;
        static const char    *DetailMapNmN;
        static const char    *RoughnessMap;
        static const char    *MetalnessMap;

        static const IdString DetailTriplanar;
        static const IdString DetailTriplanarDiffuse;
        static const IdString DetailTriplanarNormal;
        static const IdString DetailTriplanarRoughness;
        static const IdString DetailTriplanarMetalness;
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
