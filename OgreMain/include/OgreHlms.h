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
#ifndef _OgreHlms_H_
#define _OgreHlms_H_

#include "OgrePrerequisites.h"

#include "OgreHlmsCommon.h"
#include "OgreHlmsPso.h"
#include "OgreStringVector.h"
#include "Threading/OgreLightweightMutex.h"
#if !OGRE_NO_JSON
#    include "OgreHlmsJson.h"
#endif

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class CompositorShadowNode;
    struct QueuedRenderable;
    typedef vector<Archive *>::type ArchiveVec;

    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */

    class ParallelHlmsCompileQueue;

    /** HLMS stands for "High Level Material System".

        The Hlms has multiple caches:

        mRenderableCache
            This cache contains all the properties set to a Renderable class and can be evaluated
            early, when a Renderable is assigned a datablock i.e. inside Renderable::setDatablock.
            Contains properties such as whether the material has normal mapping, if the mesh
            has UV sets, evaluates if the material requires tangents for normal mapping, etc.
            The main function in charge of filling this cache is Hlms::calculateHashFor

        mPassCache
            This cache contains per-pass information, such as how many lights are in the scene,
            whether this is a shadow mapping pass, etc.
            The main function in charge of filling this cache is Hlms::preparePassHash

        mShaderCodeCache
            Contains a cache of unique shaders (from Hlms templates -> actual valid shader code)
            based on the properties merged from mRenderableCache & mPassCache.
            However it is possible that two shaders are exactly the same and thus be duplicated,
            this can happen if two combinations of properties end up producing the exact same code.
            The Microcode cache (GpuProgramManager::setSaveMicrocodesToCache) can help with that issue.

        mShaderCache
            Contains a cache of the PSOs. The difference between this and mShaderCodeCache is
            that PSOs require additional information, such as HlmsMacroblock. HlmsBlendblock.
            For more information of all that is required, see HlmsPso
    */
    class _OgreExport Hlms : public AllocatedObject<AlignAllocPolicy<>>
    {
    public:
        friend class HlmsDiskCache;

        enum PrecisionMode
        {
            /// midf datatype maps to float (i.e., 32-bit)
            /// This setting is always supported
            PrecisionFull32,

            /// midf datatype maps to float16_t (i.e., forced 16-bit)
            ///
            /// It forces the driver to produce 16-bit code, even if unoptimal
            /// Great for testing quality downgrades caused by 16-bit support
            ///
            /// - This depends on RSC_SHADER_FLOAT16.
            /// - If unsupported, we fallback to PrecisionRelaxed (RSC_SHADER_RELAXED_FLOAT)
            /// - If unsupported, we then fallback to PrecisionFull32
            PrecisionMidf16,

            /// midf datatype maps to mediump float / min16float
            ///
            /// The driver is allowed to work in either 16-bit or 32-bit code
            ///
            /// - This depends on RSC_SHADER_RELAXED_FLOAT.
            /// - If unsupported, we fallback to PrecisionMidf16 (RSC_SHADER_FLOAT16)
            /// - If unsupported, we then fallback to PrecisionFull32
            PrecisionRelaxed
        };

        enum LightGatheringMode
        {
            LightGatherForward,
            LightGatherForwardPlus,
            LightGatherDeferred,
            LightGatherNone,
        };

        struct DatablockEntry
        {
            HlmsDatablock *datablock;
            bool           visibleToManager;
            String         name;
            String         srcFile;           ///< Filename in which it was defined, if any
            String         srcResourceGroup;  ///< ResourceGroup in which it was defined, if any
            DatablockEntry() : datablock( 0 ), visibleToManager( false ) {}
            DatablockEntry( HlmsDatablock *_datablock, bool _visibleToManager, const String &_name,
                            const String &_srcFile, const String &_srcGroup ) :
                datablock( _datablock ),
                visibleToManager( _visibleToManager ),
                name( _name ),
                srcFile( _srcFile ),
                srcResourceGroup( _srcGroup )
            {
            }
        };

        typedef std::map<IdString, DatablockEntry> HlmsDatablockMap;

        /// For single-threaded operations
        static constexpr size_t kNoTid = 0u;

#ifdef OGRE_SHADER_THREADING_BACKWARDS_COMPATIBLE_API
#    ifdef OGRE_SHADER_THREADING_USE_TLS
        static thread_local uint32 msThreadId;
#    else
        static constexpr uint32 msThreadId = 0u;
#    endif
#endif

    protected:
        struct RenderableCache
        {
            HlmsPropertyVec setProperties;
            PiecesMap       pieces[NumShaderTypes];

            RenderableCache( const HlmsPropertyVec &properties, const PiecesMap *_pieces ) :
                setProperties( properties )
            {
                if( _pieces )
                {
                    for( size_t i = 0; i < NumShaderTypes; ++i )
                        pieces[i] = _pieces[i];
                }
            }

            bool operator==( const RenderableCache &_r ) const
            {
                bool piecesEqual = true;
                for( size_t i = 0; i < NumShaderTypes; ++i )
                    piecesEqual &= pieces[i] == _r.pieces[i];

                return setProperties == _r.setProperties && piecesEqual;
            }
        };

        struct PassCache
        {
            HlmsPropertyVec properties;
            HlmsPassPso     passPso;

            bool operator==( const PassCache &_r ) const
            {
                return properties == _r.properties && passPso == _r.passPso;
            }
        };

        struct ShaderCodeCache
        {
            /// Contains merged properties (pass and renderable's)
            RenderableCache mergedCache;
            GpuProgramPtr   shaders[NumShaderTypes];

            ShaderCodeCache( const PiecesMap *_pieces ) : mergedCache( HlmsPropertyVec(), _pieces ) {}

            bool operator==( const ShaderCodeCache &_r ) const
            {
                return this->mergedCache == _r.mergedCache;
            }
        };

        struct TextureRegs
        {
            uint32 strNameIdxStart;
            int32  texUnit;
            int32  numTexUnits;
            TextureRegs( uint32 _strNameIdxStart, int32 _texUnit, int32 _numTexUnits ) :
                strNameIdxStart( _strNameIdxStart ),
                texUnit( _texUnit ),
                numTexUnits( _numTexUnits )
            {
            }
        };
        typedef vector<char>::type        TextureNameStrings;
        typedef vector<TextureRegs>::type TextureRegsVec;

        typedef vector<PassCache>::type       PassCacheVec;
        typedef vector<RenderableCache>::type RenderableCacheVec;
        typedef vector<ShaderCodeCache>::type ShaderCodeCacheVec;

        PassCacheVec       mPassCache;
        RenderableCacheVec mRenderableCache;
        ShaderCodeCacheVec mShaderCodeCache;  // GUARDED_BY( mMutex )
        HlmsCacheVec       mShaderCache;      // GUARDED_BY( mMutex )

        typedef std::vector<HlmsPropertyVec> HlmsPropertyVecVec;
        typedef std::vector<PiecesMap>       PiecesMapVec;

        struct ThreadData
        {
            TextureNameStrings textureNameStrings;
            TextureRegsVec     textureRegs[NumShaderTypes];

            HlmsPropertyVec setProperties;
            PiecesMap       pieces;

            // Prevent false cache sharing
            uint8_t padding[64];
        };

        typedef std::vector<ThreadData> ThreadDataVec;

        ThreadDataVec    mT;
        LightweightMutex mMutex;

        static LightweightMutex msGlobalMutex;

        static bool msHasParticleFX2Plugin;

    public:
        struct Library
        {
            Archive     *dataFolder;
            StringVector pieceFiles[NumShaderTypes];
        };

        typedef vector<Library>::type LibraryVec;

    protected:
        LibraryVec   mLibrary;
        Archive     *mDataFolder;
        StringVector mPieceFiles[NumShaderTypes];
        HlmsManager *mHlmsManager;

        uint32_t           mShadersGenerated;  // GUARDED_BY( mMutex )
        LightGatheringMode mLightGatheringMode;
        bool               mStaticBranchingLights;
        bool               mShaderCodeCacheDirty;
        uint8              mParticleSystemConstSlot;
        uint8              mParticleSystemSlot;
        uint16             mNumLightsLimit;
        uint16             mNumAreaApproxLightsLimit;
        uint16             mNumAreaLtcLightsLimit;
        uint32             mAreaLightsGlobalLightListStart;
        uint32             mRealNumDirectionalLights;
        uint32             mRealShadowMapPointLights;
        uint32             mRealShadowMapSpotLights;
        uint32             mRealNumAreaApproxLightsWithMask;
        uint32             mRealNumAreaApproxLights;
        uint32             mRealNumAreaLtcLights;

        /// Listener for adding extensions. @see setListener.
        /// Pointer is [b]never[/b] null.
        HlmsListener *mListener;
        RenderSystem *mRenderSystem;

        HlmsDatablockMap mDatablocks;

        /// This is what we tell the RenderSystem to compile as.
        ///
        /// For example if we set at the same time:
        ///     - mShaderProfile = "metal"
        ///     - mShaderSyntax = "hlsl"
        ///     - mShaderFileExt = ".glsl"
        ///
        /// Then we will:
        ///     - Look for *.hlsl files.
        ///     - Tell the Hlms parser that it is glsl, i.e. `@property( syntax == glsl )`.
        ///     - Tell the RenderSystem to compile it as Metal.
        String mShaderProfile;  //< "glsl", "glslvk", "hlsl", "metal"
        /// This is what we tell the Hlms parser what the syntax is.
        IdString      mShaderSyntax;
        IdStringVec   mRsSpecificExtensions;
        String const *mShaderTargets[NumShaderTypes];  ///[0] = "vs_4_0", etc. Only used by D3D
        /// This is the extension we look for. e.g. .glsl, .hlsl, etc.
        String mShaderFileExt;
        String mOutputPath;
        bool   mDebugOutput;
        bool   mDebugOutputProperties;
        uint8  mPrecisionMode;  ///< See PrecisionMode
        bool   mFastShaderBuildHack;

    public:
        struct DatablockCustomPieceFile
        {
            String filename;
            String resourceGroup;
            String sourceCode;

            void getCodeChecksum( uint64 outHash[2] ) const;

            bool isCacheable() const { return !resourceGroup.empty(); }
        };

        typedef std::map<int32, DatablockCustomPieceFile> DatablockCustomPieceFileMap;

    protected:
        /// See HlmsDatablock::setCustomPieceFile.
        DatablockCustomPieceFileMap mDatablockCustomPieceFiles;

        /// The default datablock occupies the name IdString(); which is not the same as IdString("")
        HlmsDatablock *mDefaultDatablock;

        HlmsTypes mType;
        IdString  mTypeName;
        String    mTypeNameStr;

        /** Populates all mPieceFiles with all files in mDataFolder with suffix ending in
                piece_vs    - Vertex Shader
                piece_ps    - Pixel Shader
                piece_gs    - Geometry Shader
                piece_hs    - Hull Shader
                piece_ds    - Domain Shader
            Case insensitive.
        */
        void enumeratePieceFiles();
        /// Populates pieceFiles, returns true if found at least one piece file.
        static bool enumeratePieceFiles( Archive *dataFolder, StringVector *pieceFiles );

        void  setProperty( size_t tid, IdString key, int32 value );
        int32 getProperty( size_t tid, IdString key, int32 defaultVal = 0 ) const;

        void unsetProperty( size_t tid, IdString key );

        enum ExpressionType
        {
            EXPR_OPERATOR_OR,    //||
            EXPR_OPERATOR_AND,   //&&
            EXPR_OPERATOR_LE,    //<
            EXPR_OPERATOR_LEEQ,  //<=
            EXPR_OPERATOR_EQ,    //==
            EXPR_OPERATOR_NEQ,   //!=
            EXPR_OPERATOR_GR,    //>
            EXPR_OPERATOR_GREQ,  //>=
            EXPR_OBJECT,         //(...)
            EXPR_VAR
        };

        struct Expression
        {
            int32                   result;
            bool                    negated;
            ExpressionType          type;
            std::vector<Expression> children;
            String                  value;

            Expression() : result( false ), negated( false ), type( EXPR_VAR ) {}

            bool isOperator() const { return type >= EXPR_OPERATOR_OR && type <= EXPR_OPERATOR_GREQ; }
            inline void swap( Expression &other );
        };

        typedef std::vector<Expression> ExpressionVec;

        inline int interpretAsNumberThenAsProperty( const String &argValue, size_t tid ) const;

        static void copy( String &outBuffer, const SubStringRef &inSubString, size_t length );
        static void repeat( String &outBuffer, const SubStringRef &inSubString, size_t length,
                            size_t passNum, const String &counterVar );

        bool parseMath( const String &inBuffer, String &outBuffer, size_t tid );
        bool parseForEach( const String &inBuffer, String &outBuffer, size_t tid ) const;
        bool parseProperties( String &inBuffer, String &outBuffer, size_t tid ) const;
        bool parseUndefPieces( String &inBuffer, String &outBuffer, size_t tid );
        bool collectPieces( const String &inBuffer, String &outBuffer, size_t tid );
        bool insertPieces( String &inBuffer, String &outBuffer, size_t tid ) const;
        bool parseCounter( const String &inString, String &outString, size_t tid );

    public:
        /// For standalone parsing.
        bool parseOffline( const String &filename, String &inBuffer, String &outBuffer, size_t tid );

    protected:
        /** Goes through 'buffer', starting from startPos (inclusive) looking for the given
            character while skipping whitespace. If any character other than whitespace or
            EOLs if found returns String::npos
        @return
            String::npos if not found or wasn't the next character. If found, the position
            in the buffer, from start
        */
        static size_t findNextCharacter( const String &buffer, size_t startPos, char character );

        /// Returns true if we found an @end, false if we found
        /// \@else instead (can only happen if allowsElse=true).
        static bool findBlockEnd( SubStringRef &outSubString, bool &syntaxError,
                                  bool allowsElse = false );

        bool  evaluateExpression( SubStringRef &outSubString, bool &outSyntaxError, size_t tid ) const;
        int32 evaluateExpressionRecursive( ExpressionVec &expression, bool &outSyntaxError,
                                           size_t tid ) const;
        static size_t evaluateExpressionEnd( const SubStringRef &outSubString );

        static void evaluateParamArgs( SubStringRef &outSubString, StringVector &outArgs,
                                       bool &outSyntaxError );

        static unsigned long calculateLineCount( const String &buffer, size_t idx );
        static unsigned long calculateLineCount( const SubStringRef &subString );

        /** Caches a set of properties (i.e. key-value pairs) & snippets of shaders. If an
            exact entry exists in the cache, its index is returned. Otherwise a new entry
            will be created.
        @param renderableSetProperties
            A vector containing key-value pairs of data
        @param pieces
            Shader snippets for each type of shader. Can be null. When not null, must hold
            NumShaderTypes entries, i.e. String val = pieces[NumShaderTypes-1][IdString]
        @return
            The index to the cache entry.
        */
        uint32 addRenderableCache( const HlmsPropertyVec &renderableSetProperties,
                                   const PiecesMap       *pieces );

        /// Retrieves a cache entry using the returned value from @addRenderableCache
        const RenderableCache &getRenderableCache( uint32 hash ) const;

        HlmsCache       *addStubShaderCache( uint32 hash );
        const HlmsCache *addShaderCache( uint32 hash, const HlmsPso &pso );
        const HlmsCache *getShaderCache( uint32 hash ) const;
        virtual void     clearShaderCache();

        void processPieces( Archive *archive, const StringVector &pieceFiles, size_t tid );
        void hashPieceFiles( Archive *archive, const StringVector &pieceFiles,
                             FastArray<uint8> &fileContents ) const;

        void dumpProperties( std::ofstream &outFile, size_t tid );

        /** Modifies the PSO's macroblock if there are reasons to do that, and creates
            a strong reference to the macroblock that the PSO will own.
        @param pso [in/out]
            PSO to (potentially) modify.
        */
        void applyStrongMacroblockRules( HlmsPso &pso );

        virtual void setupRootLayout( RootLayout &rootLayout, size_t tid ) = 0;

        HighLevelGpuProgramPtr compileShaderCode( const String &source,
                                                  const String &debugFilenameOutput, uint32 finalHash,
                                                  ShaderType shaderType, size_t tid );

    public:
        void _compileShaderFromPreprocessedSource( const RenderableCache &mergedCache,
                                                   const String           source[NumShaderTypes],
                                                   const uint32 shaderCounter, size_t tid );

        /** Compiles input properties and adds it to the shader code cache
        @param codeCache [in/out]
            All variables must be filled except for ShaderCodeCache::shaders which is the output
        */
        void compileShaderCode( ShaderCodeCache &codeCache, uint32 shaderCounter, size_t tid );

        const ShaderCodeCacheVec &getShaderCodeCache() const { return mShaderCodeCache; }

    protected:
        /** Creates a shader based on input parameters. Caller is responsible for ensuring
            this shader hasn't already been created.
            Shader template files will be processed and then compiled.
        @param renderableHash
            The hash calculated in from Hlms::calculateHashFor that lives in Renderable
        @param passCache
            The return value of Hlms::preparePassHash
        @param finalHash
            A hash calculated on the pass' & renderable' hash. Must be unique. Caller is
            responsible for ensuring this hash stays unique.
        @param queuedRenderable
            The renderable who owns the renderableHash. Not used by the base class, but
            derived implementations may overload this function and take advantage of
            some of the direct access it provides.
        @param reservedStubEntry
            If nullptr, then we create a new ptr in addShaderCache()
            If non null, then reservedStubEntry IS the entry returned from a previously called
            addShaderCache and we will fill its members.
        @param threadIdx
            Thread idx
        @return
            The newly created shader (or reservedStubEntry, if non-null).
        */
        virtual const HlmsCache *createShaderCacheEntry( uint32           renderableHash,
                                                         const HlmsCache &passCache, uint32 finalHash,
                                                         const QueuedRenderable &queuedRenderable,
                                                         HlmsCache              *reservedStubEntry,
                                                         size_t                  threadIdx );

        /// This function gets called right before starting parsing all templates, and after
        /// the renderable properties have been merged with the pass properties.
        ///
        /// Warning: For the HlmsDiskCache to work properly, this function should not rely
        /// on member variables or other state. All state info should come from getProperty()
        virtual void notifyPropertiesMergedPreGenerationStep( size_t tid );

        virtual HlmsDatablock *createDatablockImpl( IdString              datablockName,
                                                    const HlmsMacroblock *macroblock,
                                                    const HlmsBlendblock *blendblock,
                                                    const HlmsParamVec   &paramVec ) = 0;

        virtual HlmsDatablock *createDefaultDatablock();

        void _destroyAllDatablocks();

        inline void calculateHashForSemantic( VertexElementSemantic semantic, VertexElementType type,
                                              uint16 index, uint &inOutNumTexCoords );

        uint16 calculateHashForV1( Renderable *renderable );
        uint16 calculateHashForV2( Renderable *renderable );

        virtual void calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces ) {}
        virtual void calculateHashForPreCaster( Renderable *renderable, PiecesMap *inOutPieces,
                                                const PiecesMap *normalPassPieces )
        {
        }

        HlmsCache preparePassHashBase( const Ogre::CompositorShadowNode *shadowNode, bool casterPass,
                                       bool dualParaboloid, SceneManager *sceneManager );

        HlmsPassPso getPassPsoForScene( SceneManager *sceneManager );

        /// OpenGL sets texture binding slots from C++
        /// All other APIs set the slots from shader.
        /// However managing the slots in the template can be troublesome. It's best
        /// managed in C++
        ///
        /// This function will set a property with name 'texName' to the value 'texUnit'
        /// so that the template can use it (e.g. D3D11, Metal).
        ///
        /// In OpenGL, applyTextureRegisters will later be called so the params are set
        void setTextureReg( size_t tid, ShaderType shaderType, const char *texName, int32 texUnit,
                            int32 numTexUnits = 1u );

        /// See Hlms::setTextureReg
        ///
        /// This function does NOT call RenderSystem::bindGpuProgramParameters to
        /// make the changes effective.
        void applyTextureRegisters( const HlmsCache *psoEntry, size_t tid );

        /// Returns the hash of the property Full32/Half16/Relaxed.
        /// See Hlms::getSupportedPrecisionMode
        int32 getSupportedPrecisionModeHash() const;

    public:
        /**
        @param libraryFolders
            Path to folders to be processed first for collecting pieces. Will be processed in order.
            Pointer can be null.
        */
        Hlms( HlmsTypes type, const String &typeName, Archive *dataFolder, ArchiveVec *libraryFolders );
        virtual ~Hlms();

        HlmsTypes getType() const { return mType; }
        IdString  getTypeName() const { return mTypeName; }

        const String &getTypeNameStr() const { return mTypeNameStr; }
        void          _notifyManager( HlmsManager *manager ) { mHlmsManager = manager; }
        HlmsManager  *getHlmsManager() const { return mHlmsManager; }
        const String &getShaderProfile() const { return mShaderProfile; }
        IdString      getShaderSyntax() const { return mShaderSyntax; }

        void getTemplateChecksum( uint64 outHash[2] ) const;

        /// Sets the precision mode of Hlms. See PrecisionMode
        /// Note: This call may invalidate the shader cache!
        /// Call as early as possible.
        void setPrecisionMode( PrecisionMode precisionMode );

        /// Returns requested precision mode (i.e., value passed to setPrecisionMode)
        /// See getSupportedPrecisionMode
        PrecisionMode getPrecisionMode() const;

        /// Some GPUs don't support all precision modes. Therefore this
        /// will returns the actually used precision mode after checking
        /// HW support
        PrecisionMode getSupportedPrecisionMode() const;

        /// Returns true if shaders are being compiled with Fast Shader Build Hack (D3D11 only)
        bool getFastShaderBuildHack() const;

        uint8 getParticleSystemConstSlot() const { return mParticleSystemConstSlot; }
        uint8 getParticleSystemSlot() const { return mParticleSystemSlot; }

        /** Non-caster directional lights are hardcoded into shaders. This means that if you
            have 6 directional lights and then you add a 7th one, a whole new set of shaders
            will be created.

            This setting allows you to tremendously reduce the amount of shader permutations
            by forcing Ogre to switching to static branching with an upper limit to the max
            number of non-shadow-casting directional lights.

            There is no such switch for shadow-casting directional/point/spot lights because
            of technical limitations at the GPU level (cannot index shadow map textures in DX11,
            nor samplers in any known GPU).

            @see    setAreaLightForwardSettings
        @param maxLights
            @parblock
            Maximum number of non-caster directional lights. 0 to allow unlimited number of lights,
            at the cost of shader recompilations when directional lights are added or removed.

            Default value is 0.

            Note: There is little to no performance impact for setting this value higher than you need.
            e.g. If you set maxLights = 4, but you only have 2 non-caster dir. lights on scene,
            you'll pay the price of 2 lights (but the RAM price of 4).

            Beware of setting this value too high (e.g. 65535) as the amount of memory space is limited
            (we cannot exceed 64kb, including unrelated data to lighting, but required to the pass)
            @endparblock
         */
        void   setMaxNonCasterDirectionalLights( uint16 maxLights );
        uint16 getMaxNonCasterDirectionalLights() const { return mNumLightsLimit; }

        /** By default shadow-caster spot and point lights are hardcoded into shaders.

            This means that if you have 8 spot/point lights and then you add a 9th one,
            a whole new set of shaders will be created.
            Even more if you have a combination of 3 spot and 5 point lights and the combination
            has changed to 4 spot and 4 point lights then you'll get the next set of shaders

            This setting allows you to tremendously reduce the amount of shader permutations
            by forcing Ogre to switching to static branching with an upper limit to the max
            number of shadow-casting spot or point lights.

            See    Hlms::setAreaLightForwardSettings
        @remarks
            All point and spot lights must share the same hlms_shadowmap atlas

            This is mostly an D3D11 / HLSL SM 5.0 restriction
            (https://github.com/OGRECave/ogre-next/pull/255) but it may also help with
            performance in other APIs.

            If multiple atlas support is needed, using Texture2DArrays may be a good solution,
            although it is currently untested and may need additional fixes to get it working

        @param staticBranchingLights
            True to evalute number of lights in the shader using static branching (less shader variants).
            False to recompile the shader more often (more variants, but better optimized shaders).
         */
        virtual void setStaticBranchingLights( bool staticBranchingLights );
        bool         getStaticBranchingLights() const { return mStaticBranchingLights; }

        void _tagShaderCodeCacheUpToDate() { mShaderCodeCacheDirty = false; }

        /// Users can check this function to tell if HlmsDiskCache needs saving.
        /// If this value returns false, then HlmsDiskCache doesn't need saving.
        bool isShaderCodeCacheDirty() const { return mShaderCodeCacheDirty; }

        static void _setHasParticleFX2Plugin( bool bHasPfx2Plugin )
        {
            msHasParticleFX2Plugin = bHasPfx2Plugin;
        }

        static bool hasParticleFX2Plugin() { return msHasParticleFX2Plugin; }

        /** Area lights use regular Forward.
        @param areaLightsApproxLimit
            @parblock
            Maximum number of area approx lights that will be considered by the shader.
            Default value is 1.
            Use 0 to disable area lights.

            Note: There is little to no performance impact for setting this value higher than you need.
            e.g. If you set areaLightsApproxLimit = 4, but you only have 2 area lights on scene,
            you'll pay the price of 2 area lights (but the RAM price of 4).

            Beware of setting this value too high (e.g. 65535) as the amount of memory space is limited
            (we cannot exceed 64kb, including unrelated data to lighting, but required to the pass)
            @endparblock
        @param areaLightsLtcLimit
            Same as areaLightsApproxLimit, but for LTC lights
        */
        void   setAreaLightForwardSettings( uint16 areaLightsApproxLimit, uint16 areaLightsLtcLimit );
        uint16 getAreaLightsApproxLimit() const { return mNumAreaApproxLightsLimit; }
        uint16 getAreaLightsLtcLimit() const { return mNumAreaLtcLightsLimit; }

#if !OGRE_NO_JSON
        /** Loads datablock values from a JSON value. @see HlmsJson.
        @param jsonValue
            JSON Object containing the definition of this datablock.
        @param blocks
            All the loaded Macro-, Blend- & Samplerblocks the JSON has
            defined and may be referenced by the datablock declaration.
        @param datablock
            Datablock to fill the values.
        */
        virtual void _loadJson( const rapidjson::Value &jsonValue, const HlmsJson::NamedBlocks &blocks,
                                HlmsDatablock *datablock, const String &resourceGroup,
                                HlmsJsonListener *listener,
                                const String     &additionalTextureExtension ) const
        {
        }
        virtual void _saveJson( const HlmsDatablock *datablock, String &outString,
                                HlmsJsonListener *listener,
                                const String     &additionalTextureExtension ) const
        {
        }

        virtual void _collectSamplerblocks( set<const HlmsSamplerblock *>::type &outSamplerblocks,
                                            const HlmsDatablock                 *datablock ) const
        {
        }
#endif

        void saveAllTexturesFromDatablocks( const String &folderPath, set<String>::type &savedTextures,
                                            bool saveOitd, bool saveOriginal,
                                            HlmsTextureExportListener *listener );

        /** Destroys all the cached shaders and in the next opportunity will recreate them
            from the new location. This is very useful for fast iteration and real-time
            editing of Hlms shader templates.
        @remarks
            Calling with null pointer is possible and will only invalidate existing shaders
            but you should provide a valid pointer before we start generating the first
            shader (or else crash).
        @par
            Existing datablock materials won't be reloaded from files, so their properties
            won't change (i.e. changed from blue to red), but the shaders will.
        @param libraryFolders
            When null pointer, the library folders paths won't be changed at all
            (but still will be reloaded).
            When non-null pointer, the library folders will be overwriten.
            Pass an empty container if you want to stop using libraries.
        */
        virtual void reloadFrom( Archive *newDataFolder, ArchiveVec *libraryFolders = 0 );

        Archive          *getDataFolder() { return mDataFolder; }
        const LibraryVec &getPiecesLibrary() const { return mLibrary; }
        ArchiveVec        getPiecesLibraryAsArchiveVec() const;

        void _setNumThreads( size_t numThreads );
        void _setShadersGenerated( uint32 shadersGenerated );

        /** Creates a unique datablock that can be shared by multiple renderables.
        @remarks
            The name of the datablock must be in paramVec["name"] and must be unique
            Throws if a datablock with the same name paramVec["name"] already exists
        @param name
            Name of the Datablock, must be unique within all Hlms types, not just this one.
            99% you want this to be IdString( refName ); however this is not enforced.
        @param refName
            Name of the Datablock. The engine doesn't use this value at all. It is only
            useful for UI editors which want to enumerate all existing datablocks and
            display its name to the user.
        @param macroblockRef
            see HlmsManager::getMacroblock
        @param blendblockRef
            see HlmsManager::getBlendblock
        @param paramVec
            Key - String Value list of paramters. MUST BE SORTED.
        @param visibleToManager
            When false, HlmsManager::getDatablock won't find this datablock. True by default
        @param filename
            Filename in which it was defined, so that this information can be retrieved
            later by the user if needed. This is only for informational purposes.
        @param resourceGroup
            ResourceGroup. See filename param.
        @return
            Pointer to created Datablock
        */
        HlmsDatablock *createDatablock( IdString name, const String &refName,
                                        const HlmsMacroblock &macroblockRef,
                                        const HlmsBlendblock &blendblockRef,
                                        const HlmsParamVec &paramVec, bool visibleToManager = true,
                                        const String &filename = BLANKSTRING,
                                        const String &resourceGroup = BLANKSTRING );

        /** Finds an existing datablock based on its name (@see createDatablock)
        @return
            The datablock associated with that name. Null pointer if not found. Doesn't throw.
        */
        HlmsDatablock *getDatablock( IdString name ) const;

        /// Returns the string name associated with its hashed name (this was
        /// passed as refName in createDatablock()). Returns null ptr if
        /// not found.
        /// The reason this String doesn't live in HlmsDatablock is to prevent
        /// cache trashing (datablocks are hot iterated every frame, and the
        /// full name is rarely ever used)
        const String *getNameStr( IdString name ) const;

        /// Returns the filaname & resource group a datablock was created from, and
        /// is associated with its hashed name (this was passed as in createDatablock()).
        /// Returns null ptr if not found. Note that it may also be a valid pointer but
        /// contain an empty string.
        /// The reason this String doesn't live in HlmsDatablock is to prevent
        /// cache trashing (datablocks are hot iterated every frame, and the
        /// filename & resource groups are rarely ever used).
        /// @par
        /// Usage:
        /// @code
        ///     String const *filename;
        ///     String const *resourceGroup;
        ///     datablock->getFilenameAndResourceGroup( &filename, &resourceGroup );
        ///     if( filename && resourceGroup && !filename->empty() && !resourceGroup->empty() )
        ///     {
        ///         //Valid filename & resource group.
        ///     }
        /// @endcode
        void getFilenameAndResourceGroup( IdString name, String const **outFilename,
                                          String const **outResourceGroup ) const;

        /** Destroys a datablocks given its name. Caller is responsible for ensuring
            those pointers aren't still in use (i.e. dangling pointers)
        @remarks
            Throws if no datablock with the given name exists.
        */
        void destroyDatablock( IdString name );

        /// Destroys all datablocks created with createDatablock(). Caller is responsible
        /// for ensuring those pointers aren't still in use (i.e. dangling pointers)
        /// The default datablock will be recreated.
        void destroyAllDatablocks();

        /// @copydoc HlmsManager::getDefaultDatablock
        HlmsDatablock *getDefaultDatablock() const;

        /// Returns all datablocks owned by this Hlms, including the default one.
        const HlmsDatablockMap &getDatablockMap() const { return mDatablocks; }

        /** Finds the parameter with key 'key' in the given 'paramVec'. If found, outputs
            the value to 'inOut', otherwise leaves 'inOut' as is.
        @return
            True if the key was found (inOut was modified), false otherwise
        @remarks
            Assumes paramVec is sorted by key.
        */
        static bool findParamInVec( const HlmsParamVec &paramVec, IdString key, String &inOut );

    protected:
        void setupSharedBasicProperties( Renderable *renderable, const bool bCasterPass );

    public:
        /** Called by the renderable when either it changes the material,
            or its properties change (e.g., the mesh's uvs are stripped)
        @param renderable
            The renderable the material will be used on.
        @param outHash
            A hash. This hash references property parameters that are already cached.
        */
        virtual void calculateHashFor( Renderable *renderable, uint32 &outHash, uint32 &outCasterHash );

        virtual void analyzeBarriers( BarrierSolver           &barrierSolver,
                                      ResourceTransitionArray &resourceTransitions,
                                      Camera *renderingCamera, const bool bCasterPass );

        /** Called every frame by the Render Queue to cache the properties needed by this
            pass. i.e. Number of PSSM splits, number of shadow casting lights, etc
        @param shadowNode
            The shadow node currently in effect. Can be null.
        @return
            A hash and cached property parameters. Unlike calculateHashFor(), the cache
            must be kept by the caller and not by us (because it may change every frame
            and is one for the whole pass, but Mesh' properties usually stay consistent
            through its lifetime but may differ per mesh)
        */
        virtual HlmsCache preparePassHash( const Ogre::CompositorShadowNode *shadowNode, bool casterPass,
                                           bool dualParaboloid, SceneManager *sceneManager );

        /** Retrieves an HlmsCache filled with the GPU programs to be used by the given
            renderable. If the shaders have already been created (i.e. whether for this
            renderable, or another one) it gets them from a cache. Otherwise we create it.
            It assumes that renderable->setHlms( this, parameters ) has already called.
        @param lastReturnedValue
            The last value returned by getMaterial.
        @param passCache
            The cache returned by preparePassHash().
        @param renderable
            The renderable the caller wants us to give the shaders.
        @param movableObject
            The MovableObject owner of the renderable (we need it to know if renderable
            should cast shadows)
        @param casterPass
            True if this pass is the shadow mapping caster pass, false otherwise
        @param parallelQueue
            If non-null, the returned pointer will be a stub pointer; and
            caller is expected to call compileStubEntry() from a worker thread.
        @return
            Structure containing all necessary shaders
        */
        const HlmsCache *getMaterial( HlmsCache const *lastReturnedValue, const HlmsCache &passCache,
                                      const QueuedRenderable &queuedRenderable, bool casterPass,
                                      ParallelHlmsCompileQueue *parallelQueue );

        /** Called by ParallelHlmsCompileQueue to finish the job started in getMaterial()
        @param passCache
            See lastReturnedValue from getMaterial()
        @param reservedStubEntry
            The stub cache entry (return value of getMaterial()) to fill.
        @param queuedRenderable
            See getMaterial()
        @param renderableHash
        @param finalHash
        @param tid
            Thread idx of caller
        */
        void compileStubEntry( const HlmsCache &passCache, HlmsCache *reservedStubEntry,
                               QueuedRenderable queuedRenderable, uint32 renderableHash,
                               uint32 finalHash, size_t tid );

        /** This is extremely similar to getMaterial() except it's been designed to be always
            in parallel and to be used by warm_up passes.

            The main difference is that getMaterial() starts firing shaders for parallel compilation
            as soon as they are seen, while this function accumulates as much as possible (even
            crossing multiple warm_up passes if they are in Collect mode) and then fire everything
            at once.

            This can result in greater throughput.
        @param lastReturnedValue
            Hash of the last value we've returned.
        @param passCache
            See getMaterial()
        @param passCacheIdx
            We can't send a permanent reference of passCache to parallelQueue,
            because the passCache won't survive that long. So we send instead
            and index that parallelQueue will later use to send the right pass cache
            to compileStubEntry()
        @param queuedRenderable
            See getMaterial()
        @param casterPass
            See getMaterial()
        @param parallelQueue [in/out]
            Queue to push our work to
        @return
            The hash the shader will end up with.
            Caller must track whether we've already returned this value.
        */
        uint32 getMaterialSerial01( uint32 lastReturnedValue, const HlmsCache &passCache,
                                    const size_t passCacheIdx, const QueuedRenderable &queuedRenderable,
                                    bool casterPass, ParallelHlmsCompileQueue &parallelQueue );

        /** Fills the constant buffers. Gets executed right before drawing the mesh.
        @param cache
            Current cache of Shaders to be used.
        @param queuedRenderable
            The Renderable-MovableObject pair about to be rendered.
        @param casterPass
            Whether this is a shadow mapping caster pass.
        @param lastCacheHash
            The hash of the cache of shaders that was the used by the previous renderable.
        @param lastTextureHash
            Last Texture Hash, used to let the Hlms know whether the textures should be changed again
        @return
            New Texture hash (may be equal or different to lastTextureHash).
        */
        virtual uint32 fillBuffersFor( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                       bool casterPass, uint32 lastCacheHash,
                                       uint32 lastTextureHash ) = 0;

        virtual uint32 fillBuffersForV1( const HlmsCache        *cache,
                                         const QueuedRenderable &queuedRenderable, bool casterPass,
                                         uint32 lastCacheHash, CommandBuffer *commandBuffer ) = 0;

        virtual uint32 fillBuffersForV2( const HlmsCache        *cache,
                                         const QueuedRenderable &queuedRenderable, bool casterPass,
                                         uint32 lastCacheHash, CommandBuffer *commandBuffer ) = 0;

        /// This gets called right before executing the command buffer.
        virtual void preCommandBufferExecution( CommandBuffer *commandBuffer ) {}
        /// This gets called after executing the command buffer.
        virtual void postCommandBufferExecution( CommandBuffer *commandBuffer ) {}

        /// Called when the frame has fully ended (ALL passes have been executed to all RTTs)
        virtual void frameEnded() {}

        /** Call to output the automatically generated shaders (which are usually made from templates)
            on the given folder for inspection, analyzing, debugging, etc.
        @remarks
            The shader will be dumped when it is generated, not when this function gets called.
            You should call this function at start up
        @param enableDebugOutput
            Whether to enable or disable dumping the shaders into a folder
        @param outputProperties
            Whether to dump properties and pieces at the beginning of the shader file.
            This is very useful for determining what caused Ogre to compile a new variation.
            Note that this setting may not always produce valid shader code in the dumped files
            (but it we'll still produce valid shader code while at runtime)
            If you want to compile the dumped file and it is invalid, just strip this info.
        @param path
            Path location on where to dump it. Should end with slash for proper concatenation
            (i.e. C:/path/ instead of C:/path; or /home/user/ instead of /home/user)
        */
        void setDebugOutputPath( bool enableDebugOutput, bool outputProperties,
                                 const String &path = BLANKSTRING );

        /** Sets a listener to extend an existing Hlms implementation's with custom code,
            without having to rewrite it or modify the source code directly.
        @remarks
            Other alternatives for extending an existing implementation is to derive
            from the class and override particular virtual functions.
            For performance reasons, listeners are never called on a per-object basis.
            Consult the section "Customizing an existing implementation" from the
            manual in the Docs/2.0 folder.
        @param listener
            Listener pointer. Use null to disable.
        */
        void setListener( HlmsListener *listener );

        /** Returns the current listener. @see setListener();
        @remarks
            If the default listener is being used (that does nothing) then null is returned.
        */
        HlmsListener *getListener() const;

        /// For debugging stuff. I.e. the Command line uses it for testing manually set properties
        void  _setProperty( size_t tid, IdString key, int32 value ) { setProperty( tid, key, value ); }
        int32 _getProperty( size_t tid, IdString key, int32 defaultVal = 0 ) const
        {
            return getProperty( tid, key, defaultVal );
        }

        void _setTextureReg( size_t tid, ShaderType shaderType, const char *texName, int32 texUnit )
        {
            setTextureReg( tid, shaderType, texName, texUnit );
        }

        /// Utility helper, mostly useful to HlmsListener implementations.
        static void setProperty( HlmsPropertyVec &properties, IdString key, int32 value );
        /// Utility helper, mostly useful to HlmsListener implementations.
        static int32 getProperty( const HlmsPropertyVec &properties, IdString key,
                                  int32 defaultVal = 0 );

        void _clearShaderCache();

        /** See HlmsDatablock::setCustomPieceCodeFromMemory & HlmsDatablock::setCustomPieceFile.
        @param filename
            Name of the file.
        @param resourceGroup
            The name of the resource group in which to look for the file.
        */
        void _addDatablockCustomPieceFile( const String &filename, const String &resourceGroup );

        enum CachedCustomPieceFileStatus
        {
            /// Everything ok.
            CCPFS_Success,
            /// Recompile the cache from the templates.
            CCPFS_OutOfDate,
            /// The cache contains unrecoverable errors. Do not use the cache.
            CCPFS_CriticalError,
        };

        /**
        @param filename
            See _addDatablockCustomPieceFile() overload.
            Unlike the other overload, file not found errors are ignored.
        @param resourceGroup
            The name of the resource group in which to look for the file.
        @param templateHash
            The expected hash of the file. File won't be added if the hash does not match.
        @return
            See CachedCustomPieceFileStatus.
        */
        CachedCustomPieceFileStatus _addDatablockCustomPieceFile( const String &filename,
                                                                  const String &resourceGroup,
                                                                  const uint64  sourceCodeHash[2] );

        /**
                @param filename
                    Name of the file.
                @param sourceCode
                    The contents of the file.
         */
        void _addDatablockCustomPieceFileFromMemory( const String &filename, const String &sourceCode );

        bool isDatablockCustomPieceFileCacheable( int32 filenameHashId ) const;

        const String &getDatablockCustomPieceFileNameStr( int32 filenameHashId ) const;

        /// Returns all the data we know about filenameHashId. Can be nullptr if not found.
        const DatablockCustomPieceFile * /*ogre_nullable*/
        getDatablockCustomPieceData( int32 filenameHashId ) const;

        virtual void _changeRenderSystem( RenderSystem *newRs );

        RenderSystem *getRenderSystem() const { return mRenderSystem; }

#ifdef OGRE_SHADER_THREADING_BACKWARDS_COMPATIBLE_API
    protected:
        /// Backwards-compatible versions of setProperty/getProperty/unsetProperty
        /// that don't ask for tid and rely instead on Thread Local Storage
        void  setProperty( IdString key, int32 value ) { setProperty( msThreadId, key, value ); }
        int32 getProperty( IdString key, int32 defaultVal = 0 ) const
        {
            return getProperty( msThreadId, key, defaultVal );
        }
        void unsetProperty( IdString key ) { unsetProperty( msThreadId, key ); }

    public:
        void setTextureReg( ShaderType shaderType, const char *texName, int32 texUnit,
                            int32 numTexUnits = 1u )
        {
            setTextureReg( msThreadId, shaderType, texName, texUnit, numTexUnits );
        }
        void  _setProperty( IdString key, int32 value ) { setProperty( msThreadId, key, value ); }
        int32 _getProperty( IdString key, int32 defaultVal = 0 ) const
        {
            return getProperty( msThreadId, key, defaultVal );
        }
        void _setTextureReg( ShaderType shaderType, const char *texName, int32 texUnit )
        {
            setTextureReg( msThreadId, shaderType, texName, texUnit );
        }
#endif
    };

    /// These are "default" or "Base" properties common to many implementations and thus defined here.
    /// Most of them start with the suffix hlms_
    struct _OgreExport HlmsBaseProp
    {
        static const IdString Skeleton;
        static const IdString BonesPerVertex;
        static const IdString Pose;
        static const IdString PoseHalfPrecision;
        static const IdString PoseNormals;

        static const IdString Normal;
        static const IdString QTangent;
        static const IdString Tangent;
        static const IdString Tangent4;

        static const IdString Colour;

        static const IdString IdentityWorld;
        static const IdString IdentityViewProj;
        /// When this is set, the value of IdentityViewProj is meaningless.
        static const IdString IdentityViewProjDynamic;

        static const IdString UvCount;
        static const IdString UvCount0;
        static const IdString UvCount1;
        static const IdString UvCount2;
        static const IdString UvCount3;
        static const IdString UvCount4;
        static const IdString UvCount5;
        static const IdString UvCount6;
        static const IdString UvCount7;

        // Change per frame (grouped together with scene pass)
        static const IdString LightsDirectional;
        static const IdString LightsDirNonCaster;
        static const IdString LightsPoint;
        static const IdString LightsSpot;
        static const IdString LightsAreaApprox;
        static const IdString LightsAreaLtc;
        static const IdString LightsAreaTexMask;
        static const IdString LightsAreaTexColour;
        static const IdString AllPointLights;

        // Change per scene pass
        static const IdString PsoClipDistances;
        static const IdString GlobalClipPlanes;
        static const IdString EmulateClipDistances;
        static const IdString DualParaboloidMapping;
        static const IdString InstancedStereo;
        static const IdString ViewMatrix;
        static const IdString StaticBranchLights;
        static const IdString StaticBranchShadowMapLights;
        static const IdString NumShadowMapLights;
        static const IdString NumShadowMapTextures;
        static const IdString PssmSplits;
        static const IdString PssmBlend;
        static const IdString PssmFade;
        static const IdString ShadowCaster;
        static const IdString ShadowCasterDirectional;
        static const IdString ShadowCasterPoint;
        static const IdString ShadowUsesDepthTexture;
        static const IdString RenderDepthOnly;
        static const IdString FineLightMask;
        static const IdString UseUvBaking;
        static const IdString UvBaking;
        static const IdString BakeLightingOnly;
        static const IdString MsaaSamples;
        static const IdString GenNormalsGBuf;
        static const IdString PrePass;
        static const IdString UsePrePass;
        static const IdString UsePrePassMsaa;
        static const IdString UseSsr;
        // Per pass. Related with ScreenSpaceRefractions
        static const IdString SsRefractionsAvailable;
        static const IdString Fog;
        static const IdString EnableVpls;
        static const IdString ForwardPlus;
        static const IdString ForwardPlusFlipY;
        static const IdString ForwardPlusDebug;
        static const IdString ForwardPlusFadeAttenRange;
        static const IdString ForwardPlusFineLightMask;
        static const IdString ForwardPlusCoversEntireTarget;
        static const IdString Forward3DNumSlices;
        static const IdString FwdClusteredWidthxHeight;
        static const IdString FwdClusteredWidth;
        static const IdString FwdClusteredLightsPerCell;
        static const IdString EnableDecals;
        static const IdString FwdPlusDecalsSlotOffset;
        static const IdString DecalsDiffuse;
        static const IdString DecalsNormals;
        static const IdString DecalsEmissive;
        static const IdString FwdPlusCubemapSlotOffset;
        static const IdString BlueNoise;
        static const IdString ParticleSystem;
        // Change per Object (specific to Particles)
        static const IdString ParticleType;
        static const IdString ParticleTypePoint;
        static const IdString ParticleTypeOrientedCommon;
        static const IdString ParticleTypeOrientedSelf;
        static const IdString ParticleTypePerpendicularCommon;
        static const IdString ParticleTypePerpendicularSelf;
        static const IdString ParticleRotation;

        static const IdString Forward3D;
        static const IdString ForwardClustered;
        static const IdString VPos;
        static const IdString ScreenPosInt;
        static const IdString ScreenPosUv;
        static const IdString VertexId;

        // Change per material (hash can be cached on the renderable)
        static const IdString AlphaTest;
        static const IdString AlphaTestShadowCasterOnly;
        static const IdString AlphaBlend;
        static const IdString AlphaToCoverage;
        static const IdString AlphaHash;
        // Per material. Related with SsRefractionsAvailable
        static const IdString ScreenSpaceRefractions;
        static const IdString
            _DatablockCustomPieceShaderName[NumShaderTypes];  // Do not set/get directly.

        // Standard depth range is being used instead of reverse Z.
        static const IdString NoReverseDepth;
        static const IdString ReadOnlyIsTex;

        static const IdString Syntax;
        static const IdString Hlsl;
        static const IdString Glsl;
        static const IdString Glslvk;
        static const IdString Hlslvk;
        static const IdString Metal;
        static const IdString GL3Plus;
        static const IdString iOS;
        static const IdString macOS;
        static const IdString GLVersion;
        static const IdString PrecisionMode;
        static const IdString Full32;
        static const IdString Midf16;
        static const IdString Relaxed;
        static const IdString FastShaderBuildHack;
        static const IdString TexGather;
        static const IdString DisableStage;

        // Useful GL Extensions
        static const IdString GlAmdTrinaryMinMax;

        static const IdString *UvCountPtrs[8];
    };

    struct _OgreExport HlmsPsoProp
    {
        static const IdString Macroblock;
        static const IdString Blendblock;
        static const IdString InputLayoutId;
    };

    struct _OgreExport HlmsBasePieces
    {
        static const IdString AlphaTestCmpFunc;
    };

    struct _OgreExport HlmsBits
    {
        static const int HlmsTypeBits;
        static const int RenderableBits;
        static const int PassBits;

        static const int HlmsTypeShift;
        static const int RenderableShift;
        static const int PassShift;
        static const int InputLayoutShift;

        static const int RendarebleHlmsTypeMask;
        static const int HlmsTypeMask;
        static const int RenderableMask;
        static const int PassMask;
        static const int InputLayoutMask;
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
