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
#ifndef _OgreHlmsUnlit_H_
#define _OgreHlmsUnlit_H_

#include "OgreHlmsUnlitPrerequisites.h"

#include "OgreConstBufferPool.h"
#include "OgreHlmsBufferManager.h"
#include "OgreMatrix4.h"
#include "OgreRoot.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class CompositorShadowNode;
    struct QueuedRenderable;

    /** \addtogroup Component
     *  @{
     */
    /** \addtogroup Material
     *  @{
     */

    class HlmsUnlitDatablock;

    /** Implementation without lighting or skeletal animation specfically designed for
        OpenGL 3+, D3D11 and other RenderSystems which support uniform buffers.
        Useful for GUI, ParticleFXs, other misc objects that don't require lighting.
    */
    class _OgreHlmsUnlitExport HlmsUnlit : public HlmsBufferManager, public ConstBufferPool
    {
    protected:
        typedef vector<HlmsDatablock *>::type HlmsDatablockVec;

        struct PassData
        {
            // [0] = Normal viewProj matrix (left eye)
            // [1] = Identity view & identity proj matrix, still flips by Y in GL (left eye)
            // [2] = Normal viewProj matrix (right eye)
            // [3] = Identity proj matrix, still flips by Y in GL (right eye)
            // [4] = Identity matrix. Used only by C++ to 'passthrough' the world matrix to shader
            //       while rendering with instanced stereo.
            Matrix4 viewProjMatrix[5];
        };

        PassData             mPreparedPass;
        ConstBufferPackedVec mPassBuffers;
        uint32               mCurrentPassBuffer;  ///< Resets to zero every new frame.

        ConstBufferPool::BufferPool const *mLastBoundPool;

        bool                        mHasSeparateSamplers;
        DescriptorSetTexture const *mLastDescTexture;
        DescriptorSetSampler const *mLastDescSampler;

        float mConstantBiasScale;
        bool  mUsingInstancedStereo;

        bool mDefaultGenerateMipmaps;

        bool   mUsingExponentialShadowMaps;
        uint16 mEsmK;  ///< K parameter for ESM.

        uint8 mReservedTexBufferSlots;  // Includes ReadOnly
        uint8 mReservedTexSlots;        // These get added to mReservedTexBufferSlots

        uint32 mTexUnitSlotStart;

        void setupRootLayout( RootLayout &rootLayout, size_t tid ) override;

        const HlmsCache *createShaderCacheEntry( uint32 renderableHash, const HlmsCache &passCache,
                                                 uint32                  finalHash,
                                                 const QueuedRenderable &queuedRenderable,
                                                 HlmsCache *reservedStubEntry, uint64 deadline,
                                                 size_t tid ) override;

        HlmsDatablock *createDatablockImpl( IdString datablockName, const HlmsMacroblock *macroblock,
                                            const HlmsBlendblock *blendblock,
                                            const HlmsParamVec   &paramVec ) override;

        void setTextureProperty( size_t tid, LwString &propertyName, HlmsUnlitDatablock *datablock,
                                 uint8 texType );

        void calculateHashFor( Renderable *renderable, uint32 &outHash, uint32 &outCasterHash ) override;
        void calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces ) override;
        void calculateHashForPreCaster( Renderable *renderable, PiecesMap *inOutPieces,
                                        const PiecesMap *normalPassPieces ) override;

        PropertiesMergeStatus notifyPropertiesMergedPreGenerationStep( size_t     tid,
                                                                       PiecesMap *inOutPieces ) override;

        void destroyAllBuffers() override;

        FORCEINLINE uint32 fillBuffersFor( const HlmsCache        *cache,
                                           const QueuedRenderable &queuedRenderable, bool casterPass,
                                           uint32 lastCacheHash, CommandBuffer *commandBuffer,
                                           bool isV1 );

        HlmsUnlit( Archive *dataFolder, ArchiveVec *libraryFolders, uint32 constBufferSize );
        HlmsUnlit( Archive *dataFolder, ArchiveVec *libraryFolders, HlmsTypes type,
                   const String &typeName, uint32 constBufferSize );

    public:
        HlmsUnlit( Archive *dataFolder, ArchiveVec *libraryFolders );
        HlmsUnlit( Archive *dataFolder, ArchiveVec *libraryFolders, HlmsTypes type,
                   const String &typeName );
        ~HlmsUnlit() override;

        void _changeRenderSystem( RenderSystem *newRs ) override;

        /// Not supported
        void setOptimizationStrategy( OptimizationStrategy ) override {}

        HlmsCache preparePassHash( const Ogre::CompositorShadowNode *shadowNode, bool casterPass,
                                   bool dualParaboloid, SceneManager *sceneManager ) override;

        uint32 fillBuffersFor( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                               bool casterPass, uint32 lastCacheHash, uint32 lastTextureHash ) override;

        uint32 fillBuffersForV1( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                 bool casterPass, uint32 lastCacheHash,
                                 CommandBuffer *commandBuffer ) override;

        uint32 fillBuffersForV2( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                 bool casterPass, uint32 lastCacheHash,
                                 CommandBuffer *commandBuffer ) override;

        void frameEnded() override;

        void setShadowSettings( bool useExponentialShadowMaps );
        bool getShadowFilter() const { return mUsingExponentialShadowMaps; }

        /// @copydoc HlmsPbs::setEsmK
        void   setEsmK( uint16 K );
        uint16 getEsmK() const { return mEsmK; }

        /** By default Unlit does not automatically generate mipmaps for textures loaded (the texture
            may already have mipmaps though).

            This behavior is the opposite from HlmsPbs.

            This is because often Unlit will be used for 2D stuff, which doesn't require mipmaps.
            However for 3D stuff, mipmaps are highly useful.

            JSON materials can override this behavior.
        @param bDefaultGenerateMips
            True to automatically generate mipmaps
            False to not generate. If the texture was already loaded by HlmsPbs it may have mipmaps, or
            if the texture on disk already has mipmaps (e.g. DDS) mipmaps will be loaded.

            This setting only affects automatic generation when the texture does not have mipmaps and
            is being loaded by Unlit.
        */
        void setDefaultGenerateMipmaps( bool bDefaultGenerateMips );

        /// Returns true if automatic mipmap generation when loading textures is on.
        /// See setDefaultGenerateMipmaps()
        bool getDefaultGenerateMipmaps() const { return mDefaultGenerateMipmaps; }

        /// @copydoc HlmsPbs::getDefaultPaths
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

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
