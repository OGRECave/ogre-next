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
#ifndef _OgreHlmsDatablock_H_
#define _OgreHlmsDatablock_H_

#include "OgreHlmsCommon.h"
#include "OgreStringVector.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */

    enum HlmsBasicBlock
    {
        BLOCK_MACRO,
        BLOCK_BLEND,
        BLOCK_SAMPLER,
        NUM_BASIC_BLOCKS,
    };

    struct _OgreExport BasicBlock
    {
        void  *mRsData;  ///< Render-System specific data
        uint16 mRefCount;
        /// The mId is only valid while mRefCount > 0; which means mRsData
        /// may contain valid data, else it's null.
        uint16 mId;
        /// Except for HlmsSamplerblocks, mLifetimeId is valid throghout the entire life of
        /// HlmsManager. This guarantees HlmsMacroblock & HlmsBlendblock pointers are always
        /// valid, although they may be inactive (i.e. mId invalid, mRefCount = 0 and mRsData = 0)
        uint16 mLifetimeId;
        uint8  mBlockType;  ///< @see HlmsBasicBlock

        /// When zero, HlmsManager cannot override the block's values with
        /// enforced global settings. (such as lower quality texture filtering or
        /// turning off depth checks for debugging)
        uint8 mAllowGlobalDefaults;

        BasicBlock( uint8 blockType );
    };

    /** A macro block contains settings that will rarely change, and thus are common to many materials.
        This is very analogous to D3D11_RASTERIZER_DESC. See HlmsDatablock
        Up to 32 different blocks are allowed!
    */
    struct _OgreExport HlmsMacroblock : public BasicBlock
    {
        bool            mScissorTestEnabled;
        bool            mDepthClamp;
        bool            mDepthCheck;
        bool            mDepthWrite;
        CompareFunction mDepthFunc;

        /// When polygons are coplanar, you can get problems with 'depth fighting' where
        /// the pixels from the two polys compete for the same screen pixel. This is particularly
        /// a problem for decals (polys attached to another surface to represent details such as
        /// bulletholes etc.).
        ///
        /// A way to combat this problem is to use a depth bias to adjust the depth buffer value
        /// used for the decal such that it is slightly higher than the true value, ensuring that
        /// the decal appears on top. There are two aspects to the biasing, a constant
        /// bias value and a slope-relative biasing value, which varies according to the
        ///  maximum depth slope relative to the camera, ie:
        /// <pre>finalBias = maxSlope * slopeScaleBias + constantBias</pre>
        /// Note that slope scale bias, whilst more accurate, may be ignored by old hardware.
        ///
        /// The constant bias value, expressed as a factor of the minimum observable depth
        float mDepthBiasConstant;
        /// The slope-relative bias value, expressed as a factor of the depth slope
        float mDepthBiasSlopeScale;

        /// Culling mode based on the 'vertex winding'.
        /// A typical way for the rendering engine to cull triangles is based on the
        /// 'vertex winding' of triangles. Vertex winding refers to the direction in
        /// which the vertices are passed or indexed to in the rendering operation as viewed
        /// from the camera, and will wither be clockwise or anticlockwise (that's 'counterclockwise' for
        /// you Americans out there ;) The default is CULL_CLOCKWISE i.e. that only triangles whose
        /// vertices are passed/indexed in anticlockwise order are rendered - this is a common approach
        /// and is used in 3D studio models for example. You can alter this culling mode if you wish
        /// but it is not advised unless you know what you are doing.
        /// You may wish to use the CULL_NONE option for mesh data that you cull yourself where the
        /// vertex winding is uncertain.
        CullingMode mCullMode;
        PolygonMode mPolygonMode;

        HlmsMacroblock();

        bool operator==( const HlmsMacroblock &_r ) const { return !( *this != _r ); }

        bool operator!=( const HlmsMacroblock &_r ) const
        {
            // Don't include the ID in the comparision
            return mAllowGlobalDefaults != _r.mAllowGlobalDefaults ||  //
                   mScissorTestEnabled != _r.mScissorTestEnabled ||    //
                   mDepthClamp != _r.mDepthClamp ||                    //
                   mDepthCheck != _r.mDepthCheck ||                    //
                   mDepthWrite != _r.mDepthWrite ||                    //
                   mDepthFunc != _r.mDepthFunc ||                      //
                   mDepthBiasConstant != _r.mDepthBiasConstant ||      //
                   mDepthBiasSlopeScale != _r.mDepthBiasSlopeScale ||  //
                   mCullMode != _r.mCullMode ||                        //
                   mPolygonMode != _r.mPolygonMode;
        }
    };

    /** A blend block contains settings that rarely change, and thus are common to many materials.
        The reasons this structure isn't joined with HlmsMacroblock is that:
            + The D3D11 API makes this distinction (much higher API overhead if we
              change i.e. depth settings) due to D3D11_RASTERIZER_DESC.
            + This block contains information of whether the material is transparent.
              Transparent materials are sorted differently than opaque ones.
        Up to 32 different blocks are allowed!
    */
    struct _OgreExport HlmsBlendblock : public BasicBlock
    {
        enum BlendChannelMasks
        {
            // clang-format off
            BlendChannelRed   = 0x01,
            BlendChannelGreen = 0x02,
            BlendChannelBlue  = 0x04,
            BlendChannelAlpha = 0x08,
            // clang-format on

            BlendChannelAll = BlendChannelRed | BlendChannelGreen | BlendChannelBlue | BlendChannelAlpha,

            /// Setting mBlendChannelMask = 0 will skip all channels (i.e. only depth and stencil)
            /// But pixel shader may still run due to side effects (e.g. pixel discard/alpha testing
            /// UAV outputs). Some drivers may not optimize this case due to the amount of checks
            /// needed to see if the PS has side effects or not.
            ///
            /// When force-disabled, we will not set a pixel shader, thus you can be certain
            /// it's running as optimized as it can be. However beware if you rely on
            /// discard/alpha testing or any other side effect, it will not render correctly
            ///
            /// Note: BlendChannelForceDisabled will be ignored if any of the other flags are set
            BlendChannelForceDisabled = 0x10
        };

        enum A2CSetting
        {
            /// Alpha to Coverage is always disabled.
            A2cDisabled,
            /// Alpha to Coverage is always enabled.
            A2cEnabled,
            /// Alpha to Coverage is enabled only if RenderTarget uses MSAA.
            A2cEnabledMsaaOnly
        };

        uint8 mAlphaToCoverage;  /// See A2CSetting

        /// Masks which colour channels will be writing to. Default: BlendChannelAll
        /// For some advanced effects, you may wish to turn off the writing of certain colour
        /// channels, or even all of the colour channels so that only the depth buffer is updated
        /// in a rendering pass (if depth writes are on; may be you want to only update the
        /// stencil buffer).
        uint8 mBlendChannelMask;

        /// @parblock
        /// This value calculated by HlmsManager::getBlendblock.
        ///
        /// - mIsTransparent = 0  -> Not transparent
        /// - mIsTransparent |= 1 -> Automatically determined as transparent
        /// - mIsTransparent |= 2 -> Forced to be considered as transparent by RenderQueue for render
        /// order
        /// - mIsTransparent = 3  -> Forced & also automatically determined as transparent
        /// @endparblock
        uint8 mIsTransparent;
        /// Used to determine if separate alpha blending should be used for color and alpha channels
        bool mSeparateBlend;

        SceneBlendFactor mSourceBlendFactor;
        SceneBlendFactor mDestBlendFactor;
        SceneBlendFactor mSourceBlendFactorAlpha;
        SceneBlendFactor mDestBlendFactorAlpha;

        // Blending operations
        SceneBlendOperation mBlendOperation;
        SceneBlendOperation mBlendOperationAlpha;

        HlmsBlendblock();

        /// Shortcut to set the blend factors to common blending operations.
        /// Sets both blend and alpha to the same value and mSeparateBlend is
        /// turned off.
        void setBlendType( SceneBlendType blendType );

        /// Shortcut to set the blend factors to common blending operations.
        /// Sets colour and alpha individually, turns mSeparateBlend on.
        void setBlendType( SceneBlendType colour, SceneBlendType alpha );

        /// Sets mSeparateBlend to true or false based on current settings
        void calculateSeparateBlendMode();

        /** Sometimes you want to force the RenderQueue to render back to front even if
            the object isn't alpha blended (e.g., you're rendering refractive materials)
        @param bForceTransparent
            True to always render back to front, like any transparent.
            False for default behavior (opaque objects are rendered front to back, alpha
            blended objects are rendered back to front)
        */
        void setForceTransparentRenderOrder( bool bForceTransparent );

        bool isAutoTransparent() const { return ( mIsTransparent & 0x01u ) != 0u; }
        bool isForcedTransparent() const { return ( mIsTransparent & 0x02u ) != 0u; }

        bool isAlphaToCoverage( const SampleDescription &sd ) const
        {
            switch( static_cast<HlmsBlendblock::A2CSetting>( mAlphaToCoverage ) )
            {
            case HlmsBlendblock::A2cDisabled:
                return false;
            case HlmsBlendblock::A2cEnabled:
                return true;
            case HlmsBlendblock::A2cEnabledMsaaOnly:
                return sd.isMultisample();
            }

            return false;
        }

        bool operator==( const HlmsBlendblock &_r ) const { return !( *this != _r ); }

        bool operator!=( const HlmsBlendblock &_r ) const
        {
            // Don't include the ID in the comparision
            // AND ignore alpha blend factors when not doing separate blends
            // AND don't include mIsTransparent's first bit, which is filled
            // automatically only for some managed objects.
            return mAllowGlobalDefaults != _r.mAllowGlobalDefaults ||            //
                   mSeparateBlend != _r.mSeparateBlend ||                        //
                   mSourceBlendFactor != _r.mSourceBlendFactor ||                //
                   mDestBlendFactor != _r.mDestBlendFactor ||                    //
                   ( mSeparateBlend &&                                           //
                     ( mSourceBlendFactorAlpha != _r.mSourceBlendFactorAlpha ||  //
                       mDestBlendFactorAlpha != _r.mDestBlendFactorAlpha ) ) ||  //
                   mBlendOperation != _r.mBlendOperation ||                      //
                   mBlendOperationAlpha != _r.mBlendOperationAlpha ||            //
                   mAlphaToCoverage != _r.mAlphaToCoverage ||                    //
                   mBlendChannelMask != _r.mBlendChannelMask ||                  //
                   ( mIsTransparent & 0x02u ) != ( _r.mIsTransparent & 0x02u );
        }
    };

    class _OgreExport HlmsTextureExportListener
    {
    public:
        /// Gives you a chance to completely change the name of the texture when saving a material
        virtual void savingChangeTextureNameOriginal( const String &aliasName, String &inOutResourceName,
                                                      String &inOutFilename )
        {
        }
        virtual void savingChangeTextureNameOitd( String &inOutFilename, TextureGpu *texture ) {}
    };

    /** An hlms datablock contains individual information about a specific material. It consists of:
            + A const pointer to an HlmsMacroblock we do not own and may be shared by other datablocks.
            + A const pointer to an HlmsBlendblock we do not own and may be shared by other datablocks.
            + The original properties from which this datablock was constructed.
            + This type may be derived to contain additional information.

        Derived types can cache information present in mOriginalProperties as strings, like diffuse
        colour values, etc.

        A datablock is the internal representation of the surface parameters (depth settings,
        textures to be used, diffuse colour, specular colour, etc).
        The notion of a datablock is the closest you'll get to a "material"
    @remarks
        Macro- & Blendblocks are immutable, hence const pointers. Trying to const cast these
        pointers in order to modify them may work on certain RenderSystems (i.e. GLES2) but
        will seriously break on other RenderSystems (i.e. D3D11).
    @par
        If you need to change a macroblock, create a new one (HlmsManager keeps them cached
        if already created) and change the entire pointer.
    @par
        Each datablock has a pair of macroblocks and blendblocks. One of is for the regular passes,
        the other is for shadow mapping passes, since in some cases you don't want them to be the same.
        As for blendblocks, with transparent objects you may want to turn off alpha blending,
        but enable alpha testing instead.
    */
    class _OgreExport HlmsDatablock : public OgreAllocatedObj
    {
        friend class RenderQueue;

    protected:
        // Non-hot variables first (can't put them last as HlmsDatablock may be derived and
        // it's better if mShadowConstantBias is together with the derived type's variables
        /// List of renderables currently using this datablock
        vector<Renderable *>::type mLinkedRenderables;

        int32 mCustomPieceFileIdHash[NumShaderTypes];

        Hlms    *mCreator;
        IdString mName;

        /** Updates the mHlmsHash & mHlmsCasterHash for all linked renderables, which may have
            if a sensitive setting has changed that would need a different shader to be created
        @remarks
            The operation itself isn't expensive, but the need to call this function indicates
            that another shader will be created (unless already cached too). If so, doing that
            will be slow.
        @param onlyNullHashes
            When true, we will only flush those that had null Hlms hash, which means
            calculateHashFor was never called on this Datablock, or more likely calculateHashFor
            delayed the calculation for later.

            This can happen if an object was delayed for later due to the textures not being
            ready, but when they are, turns out nothing needs to be changed, yet the hashes
            are null and thus those renderables need to be flushed.
        */
        void flushRenderables( bool onlyNullHashes = false );

        void updateMacroblockHash( bool casterPass );

    public:
        uint32 mTextureHash;        // TextureHash comes before macroblock for alignment reasons
        uint16 mMacroblockHash[2];  // Not all bits are used
        uint8  mType;               ///< @see HlmsTypes
    protected:
        HlmsMacroblock const *mMacroblock[2];
        HlmsBlendblock const *mBlendblock[2];

    public:
        /// When false, we won't try to have Textures become resident
        bool mAllowTextureResidencyChange;

    protected:
        bool  mIgnoreFlushRenderables;
        bool  mAlphaHashing;
        uint8 mAlphaTestCmp;  ///< @see CompareFunction
        bool  mAlphaTestShadowCasterOnly;
        float mAlphaTestThreshold;

    public:
        float mShadowConstantBias;

    public:
        HlmsDatablock( IdString name, Hlms *creator, const HlmsMacroblock *macroblock,
                       const HlmsBlendblock *blendblock, const HlmsParamVec &params );
        virtual ~HlmsDatablock();

        /** Creates a copy of this datablock with the same settings, but a different name.
        @param name
            Name of the cloned datablock.
        */
        HlmsDatablock *clone( String name ) const;

        /// Calculates the hashes needed for sorting by the RenderQueue (i.e. mTextureHash)
        virtual void calculateHash() {}

        IdString getName() const { return mName; }
        Hlms    *getCreator() const { return mCreator; }

        /** Same as setCustomPieceFile() but sources the code from memory instead of from disk.

            Calling this function triggers HlmsDatablock::flushRenderables.
        @remarks
            HlmsDiskCache cannot cache shaders generated from memory as it
            cannot guarantee the cache isn't out of date.
        @param filename
            "Filename" to identify it. It must be unique. Empty to disable.
            If the filename has already been previously provided, the contents must be an exact match.
        @param shaderCode
            Shader source code.
        @param shaderType
            Shader stage to be used.
        */
        void setCustomPieceCodeFromMemory( const String &filename, const String &shaderCode,
                                           ShaderType shaderType );

        /** Sets the filename of a piece file to be parsed from disk. First, before all other files.

            Calling this function triggers HlmsDatablock::flushRenderables.
        @remarks
            HlmsDiskCache can cache shaders generated with setCustomPieceFile().
        @param filename
            Filename of the piece file. Must be unique. Empty to disable.
            If the filename has already been previously provided, the contents must be an exact match.
        @param resourceGroup
        @param shaderType
            Shader stage to be used.
        */
        void setCustomPieceFile( const String &filename, const String &resourceGroup,
                                 ShaderType shaderType );

        /// Returns the internal ID generated by setCustomPieceFile() and setCustomPieceCodeFromMemory().
        /// All calls with the same filename share the same ID. This ID is a deterministic hash.
        /// Returns 0 if unset.
        int32 getCustomPieceFileIdHash( ShaderType shaderType ) const;

        /// Returns the filename argument set to setCustomPieceFile() and setCustomPieceCodeFromMemory().
        const String &getCustomPieceFileStr( ShaderType shaderType ) const;

        /** Sets a new macroblock that matches the same parameter as the input.
            Decreases the reference count of the previously set one.
            Runs an O(N) search to get the right block.
            Calling this function triggers a HlmsDatablock::flushRenderables
        @param macroblock
            see HlmsManager::getMacroblock
        @param casterBlock
            True to directly set the macroblock to be used during the shadow mapping's caster pass.
            When false, the value of overrideCasterBlock becomes relevant.
        @param overrideCasterBlock
            If true and casterBlock = false, the caster block will also be set to the input value.
        */
        void setMacroblock( const HlmsMacroblock &macroblock, bool casterBlock = false,
                            bool overrideCasterBlock = true );

        /** Sets the macroblock from the given pointer that was already
            retrieved from the HlmsManager. Unlike the other overload,
            this operation is O(1).
            Calling this function triggers a HlmsDatablock::flushRenderables
        @param macroblock
            A valid block. The reference count is increased inside this function.
        @param casterBlock
            True to directly set the macroblock to be used during the shadow mapping's caster pass.
            When false, the value of overrideCasterBlock becomes relevant.
        @param overrideCasterBlock
            If true and casterBlock = false, the caster block will also be set to the input value.
        */
        void setMacroblock( const HlmsMacroblock *macroblock, bool casterBlock = false,
                            bool overrideCasterBlock = true );

        /** Sets a new blendblock that matches the same parameter as the input.
            Decreases the reference count of the previous mBlendblock.
            Runs an O(N) search to get the right block.
            Calling this function triggers a HlmsDatablock::flushRenderables
        @param blendblock
            see HlmsManager::getBlendblock
        @param casterBlock
            True to directly set the blendblock to be used during the shadow mapping's caster pass.
            When false, the value of overrideCasterBlock becomes relevant.
        @param overrideCasterBlock
            If true and casterBlock = false, the caster block will also be set to the input value.
        */
        void setBlendblock( const HlmsBlendblock &blendblock, bool casterBlock = false,
                            bool overrideCasterBlock = true );

        /** Sets the blendblock from the given pointer that was already
            retrieved from the HlmsManager. Unlike the other overload,
            this operation is O(1).
            Calling this function triggers a HlmsDatablock::flushRenderables
        @param blendblock
            A valid block. The reference count is increased inside this function.
        @param casterBlock
            True to directly set the blendblock to be used during the shadow mapping's caster pass.
            When false, the value of overrideCasterBlock becomes relevant.
        @param overrideCasterBlock
            If true and casterBlock = false, the caster block will also be set to the input value.
        */
        void setBlendblock( const HlmsBlendblock *blendblock, bool casterBlock = false,
                            bool overrideCasterBlock = true );

        const HlmsMacroblock *getMacroblock( bool casterBlock = false ) const
        {
            return mMacroblock[casterBlock];
        }
        const HlmsBlendblock *getBlendblock( bool casterBlock = false ) const
        {
            return mBlendblock[casterBlock];
        }

        /** Uses a trick to *mimic* true Order Independent Transparency alpha blending.
            The advantage of this method is that it is compatible with depth buffer writes
            and is order independent.

            Calling this function triggers a HlmsDatablock::flushRenderables
        @remarks
            @parblock
            For best results:

            @code
                // Disable alpha test (default)
                datablock->setAlphaTest( CMPF_ALWAYS_PASS );
                // Do NOT enable alpha blending in the HlmsBlendblock (default)
                HlmsBlendblock blendblock;
                blendblock.setBlendType( SBT_REPLACE );
                datablock->setBlendblock( blendblock );

                datablock->setAlphaHashing( true );
            @endcode
            @endparblock
        @param bAlphaHashing
            True to enable alpha hashing.
        */
        void setAlphaHashing( bool bAlphaHashing );

        bool getAlphaHashing() const { return mAlphaHashing; }

        /** Sets the alpha test to the given compare function. CMPF_ALWAYS_PASS means disabled.
            @see mAlphaTestThreshold.
            Calling this function triggers a HlmsDatablock::flushRenderables
        @remarks
            It is to the derived implementation to actually implement the alpha test.
        @param compareFunction
            Compare function to use. Default is CMPF_ALWAYS_PASS, which means disabled.
            Note: CMPF_ALWAYS_FAIL is not supported. Set a negative threshold to
            workaround this issue.
        @param shadowCasterOnly
            When true, only the caster should use alpha testing.
            Useful if you want alpha blending (i.e. transparency) while rendering normally,
            but want semi-transparent shadows.
        @param useAlphaFromTextures
            Whether you want the diffuse texture's alpha to influence the alpha test
            Most likely you want this to be true, unless you're customizing the
            shader and have special use for alpha testing
        */
        virtual void setAlphaTest( CompareFunction compareFunction, bool shadowCasterOnly = false,
                                   bool useAlphaFromTextures = true );

        CompareFunction getAlphaTest() const;
        bool            getAlphaTestShadowCasterOnly() const;

        /** Alpha test's threshold. @see setAlphaTest
        @param threshold
            Value typically in the range [0; 1)
        */
        virtual void setAlphaTestThreshold( float threshold );
        float        getAlphaTestThreshold() const { return mAlphaTestThreshold; }

        /// @see Hlms::getNameStr. This operation is NOT fast. Might return null
        /// (if the datablock was removed from the Hlms but somehow is still alive)
        const String *getNameStr() const;

        /// @see Hlms::getFilenameAndResourceGroup. This operation is NOT fast. Might return
        /// null (if the datablock was removed from the Hlms but somehow is still alive)
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
        void getFilenameAndResourceGroup( String const **outFilename,
                                          String const **outResourceGroup ) const;

        void _linkRenderable( Renderable *renderable );
        void _unlinkRenderable( Renderable *renderable );

        const vector<Renderable *>::type &getLinkedRenderables() const { return mLinkedRenderables; }

        /// Tells datablock to start loading all of its textures (if not loaded already)
        /// and any other resource it may need.
        ///
        /// Useful to cut loading times by anticipating what the user will do.
        ///
        /// Do not call this function aggressively (e.g. for lots of material every frame)
        virtual void preload();

        virtual bool hasCustomShadowMacroblock() const;

        /// Returns the closest match for a diffuse colour,
        /// if applicable by the actual implementation.
        ///
        /// Note that Unlit implementation returns 0 as diffuse, since it's considered
        /// emissive instead due to being bright even in the absence lights.
        virtual ColourValue getDiffuseColour() const;
        /// Returns the closest match for a emissive colour,
        /// if applicable by the actual implementation.
        /// See HlmsDatablock::getDiffuseColour
        virtual ColourValue getEmissiveColour() const;

        /// Returns the closest match for a diffuse texture,
        /// if applicable by the actual implementation.
        /// See HlmsDatablock::getDiffuseColour
        virtual TextureGpu *getDiffuseTexture() const;
        /// Returns the closest match for a emissive texture,
        /// if applicable by the actual implementation.
        /// See HlmsDatablock::getDiffuseColour
        virtual TextureGpu *getEmissiveTexture() const;

        /**
        @remarks
            It's possible to set both saveOitd & saveOriginal to true, but will likely double
            storage requirements (2x as many textures). Setting both to true is useful
            for troubleshooting obscure Ogre bugs.
        @param folderPath
            Folder where to dump the textures.
        @param savedTextures [in/out]
            Set of texture names. Textures whose name is already in the set won't be saved again.
            Textures that were saved will be inserted into the set.
        @param saveOitd
            When true, we will download the texture from GPU and save it in OITD format.
            OITD is faster to load as it's stored in Ogre's native format it understands,
            but it cannot be opened by traditional image editors; also OITD is not backwards
            compatible with older versions of Ogre.
        @param saveOriginal
            When true, we will attempt to read the raw filestream of the original texture
            and save it (i.e. copy the original png/dds/etc file).
        */
        virtual void saveTextures( const String &folderPath, set<String>::type &savedTextures,
                                   bool saveOitd, bool saveOriginal,
                                   HlmsTextureExportListener *listener );

        static const char *getCmpString( CompareFunction compareFunction );

    protected:
        virtual void cloneImpl( HlmsDatablock *datablock ) const {};
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
