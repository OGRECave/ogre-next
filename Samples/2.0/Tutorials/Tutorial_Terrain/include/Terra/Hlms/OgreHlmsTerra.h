/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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
#include "OgreHlmsBufferManager.h"
#include "OgreConstBufferPool.h"
#include "OgreMatrix4.h"
#include "OgreHlmsPbs.h"
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
        virtual HlmsDatablock* createDatablockImpl( IdString datablockName,
                                                    const HlmsMacroblock *macroblock,
                                                    const HlmsBlendblock *blendblock,
                                                    const HlmsParamVec &paramVec );

        void setDetailMapProperties( HlmsTerraDatablock *datablock, PiecesMap *inOutPieces );
        void setTextureProperty( const char *propertyName, HlmsTerraDatablock *datablock,
                                 TerraTextureTypes texType );
        void setDetailTextureProperty( const char *propertyName, HlmsTerraDatablock *datablock,
                                       TerraTextureTypes baseTexType, uint8 detailIdx );

        virtual void calculateHashFor( Renderable *renderable, uint32 &outHash, uint32 &outCasterHash );
        virtual void calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces );
        virtual void calculateHashForPreCaster( Renderable *renderable, PiecesMap *inOutPieces );

        virtual void notifyPropertiesMergedPreGenerationStep(void);

        FORCEINLINE uint32 fillBuffersFor( const HlmsCache *cache,
                                           const QueuedRenderable &queuedRenderable,
                                           bool casterPass, uint32 lastCacheHash,
                                           CommandBuffer *commandBuffer, bool isV1 );

    public:
        HlmsTerra( Archive *dataFolder, ArchiveVec *libraryFolders );
        virtual ~HlmsTerra();

        void _linkTerra( Terra *terra );
        void _unlinkTerra( Terra *terra );

        virtual void _changeRenderSystem( RenderSystem *newRs );

        virtual void analyzeBarriers( BarrierSolver &barrierSolver,
                                      ResourceTransitionArray &resourceTransitions,
                                      Camera *renderingCamera, const bool bCasterPass );

        virtual uint32 fillBuffersFor( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                       bool casterPass, uint32 lastCacheHash,
                                       uint32 lastTextureHash );

        virtual uint32 fillBuffersForV1( const HlmsCache *cache,
                                         const QueuedRenderable &queuedRenderable,
                                         bool casterPass, uint32 lastCacheHash,
                                         CommandBuffer *commandBuffer );
        virtual uint32 fillBuffersForV2( const HlmsCache *cache,
                                         const QueuedRenderable &queuedRenderable,
                                         bool casterPass, uint32 lastCacheHash,
                                         CommandBuffer *commandBuffer );

        static void getDefaultPaths( String& outDataFolderPath, StringVector& outLibraryFoldersPaths );

#if !OGRE_NO_JSON
        /// @copydoc Hlms::_loadJson
        virtual void _loadJson( const rapidjson::Value &jsonValue, const HlmsJson::NamedBlocks &blocks,
                                HlmsDatablock *datablock, const String &resourceGroup,
                                HlmsJsonListener *listener,
                                const String &additionalTextureExtension ) const;
        /// @copydoc Hlms::_saveJson
        virtual void _saveJson( const HlmsDatablock *datablock, String &outString,
                                HlmsJsonListener *listener,
                                const String &additionalTextureExtension ) const;

        /// @copydoc Hlms::_collectSamplerblocks
        virtual void _collectSamplerblocks( set<const HlmsSamplerblock*>::type &outSamplerblocks,
                                            const HlmsDatablock *datablock ) const;
#endif
    };

    struct TerraProperty
    {
        static const IdString UseSkirts;

        static const IdString NumTextures;
        static const char *DiffuseMap;
        static const char *EnvProbeMap;
        static const char *DetailWeightMap;
        static const char *DetailMapN;
        static const char *DetailMapNmN;
        static const char *RoughnessMap;
        static const char *MetalnessMap;
    };

    /** @} */
    /** @} */

}

#include "OgreHeaderSuffix.h"

#endif
