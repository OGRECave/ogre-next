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
#ifndef _OgreHlmsPbsDatablock_H_
#define _OgreHlmsPbsDatablock_H_

#include "OgreHlmsPbsPrerequisites.h"

#include "OgreHlmsDatablock.h"

#define _OgreHlmsTextureBaseClassExport _OgreHlmsPbsExport
#define OGRE_HLMS_TEXTURE_BASE_CLASS HlmsPbsBaseTextureDatablock
#define OGRE_HLMS_TEXTURE_BASE_MAX_TEX NUM_PBSM_TEXTURE_TYPES
#define OGRE_HLMS_CREATOR_CLASS HlmsPbs
#include "OgreHlmsTextureBaseClass.h"
#undef _OgreHlmsTextureBaseClassExport
#undef OGRE_HLMS_TEXTURE_BASE_CLASS
#undef OGRE_HLMS_TEXTURE_BASE_MAX_TEX
#undef OGRE_HLMS_CREATOR_CLASS

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */

    namespace PbsBrdf
    {
        enum PbsBrdf
        {
            FLAG_UNCORRELATED = 1u << 31u,
            FLAG_SPERATE_DIFFUSE_FRESNEL = 1u << 30u,
            FLAG_LEGACY_MATH = 1u << 29u,
            FLAG_FULL_LEGACY = 1u << 28u,
            FLAG_HAS_DIFFUSE_FRESNEL = 1u << 27u,
            BRDF_MASK = 0x00000FFF,

            /// Most physically accurate BRDF we have. Good for representing
            /// the majority of materials.
            /// Uses:
            ///     * Roughness/Distribution/NDF term: GGX
            ///     * Geometric/Visibility term: Smith GGX Height-Correlated
            ///     * Normalized Disney Diffuse BRDF,see
            ///         "Moving Frostbite to Physically Based Rendering" from
            ///         Sebastien Lagarde & Charles de Rousiers
            Default = 0x00000000,

            /// Implements Cook-Torrance BRDF.
            /// Uses:
            ///     * Roughness/Distribution/NDF term: Beckmann
            ///     * Geometric/Visibility term: Cook-Torrance
            ///     * Lambertian Diffuse.
            ///
            /// Ideal for silk (use high roughness values), synthetic fabric
            CookTorrance = 0x00000001,

            /// Implements Normalized Blinn Phong using a normalization
            /// factor of (n + 8) / (8 * pi)
            /// The main reason to use this BRDF is performance. It's cheaper,
            /// while still looking somewhat similar to Default.
            /// If you still need more performance, see BlinnPhongLegacy
            BlinnPhong = 0x00000002,

            /// Same as Default, but the geometry term is not height-correlated
            /// which most notably causes edges to be dimmer and is less correct.
            /// Unity (Marmoset too?) use an uncorrelated term, so you may want to
            /// use this BRDF to get the closest look for a nice exchangeable
            /// pipeline workflow.
            DefaultUncorrelated = Default | FLAG_UNCORRELATED,

            /// This used to be 'Default' in OgreNext 2.4 and earlier.
            /// The diffuse component is multiplied against
            /// the inverse of the specular's fresnel to maintain energy conservation.
            ///
            /// This has the nice side effect that to achieve a perfect mirror effect,
            /// you just need to raise the fresnel term to 1; which is very intuitive
            /// to artists (specially if using coloured fresnel)
            ///
            /// However after research and feedback from the community, most
            /// DCC tools out there (e.g. Marmoset) don't do this; and users
            /// expect consistency.
            ///
            /// Therefore the new Default BRDF in 3.0 does not include diffuse fresnel
            DefaultHasDiffuseFresnel = Default | FLAG_HAS_DIFFUSE_FRESNEL,

            /// Same as DefaultHasDiffuseFresnel but the fresnel of the diffuse is calculated
            /// differently. Normally the diffuse component would be multiplied against
            /// the inverse of the specular's fresnel to maintain energy conservation.
            /// This has the nice side effect that to achieve a perfect mirror effect,
            /// you just need to raise the fresnel term to 1; which is very intuitive
            /// to artists (specially if using coloured fresnel)
            ///
            /// When using this BRDF, the diffuse fresnel will be calculated differently,
            /// causing the diffuse component to still affect the colour even when
            /// the fresnel = 1 (although subtly). To achieve a perfect mirror you will
            /// have to set the fresnel to 1 *and* the diffuse colour to black;
            /// which can be unintuitive for artists.
            ///
            /// This BRDF is very useful for representing surfaces with complex refractions
            /// and reflections like glass, transparent plastics, fur, and surface with
            /// refractions and multiple rescattering that cannot be represented well
            /// with the default BRDF.
            DefaultSeparateDiffuseFresnel =
                Default | FLAG_HAS_DIFFUSE_FRESNEL | FLAG_SPERATE_DIFFUSE_FRESNEL,

            /// CookTorrance, but w/ diffuse fresnel. See DefaultHasDiffuseFresnel.
            CookTorranceHasDiffuseFresnel = CookTorrance | FLAG_HAS_DIFFUSE_FRESNEL,

            /// CookTorrance but w/ separate diffuse fresnel. See DefaultSeparateDiffuseFresnel
            ///
            /// Ideal for shiny objects like glass toy marbles, some types of rubber.
            /// silk, synthetic fabric.
            CookTorranceSeparateDiffuseFresnel =
                CookTorrance | FLAG_HAS_DIFFUSE_FRESNEL | FLAG_SPERATE_DIFFUSE_FRESNEL,

            /// BlinnPhong but w/ diffuse fresnel. See DefaultHasDiffuseFresnel.
            BlinnPhongHasDiffuseFresnel = BlinnPhong | FLAG_HAS_DIFFUSE_FRESNEL,

            /// BlinnPhong but w/ diffuse fresnel. See DefaultSeparateDiffuseFresnel.
            BlinnPhongSeparateDiffuseFresnel =
                BlinnPhong | FLAG_HAS_DIFFUSE_FRESNEL | FLAG_SPERATE_DIFFUSE_FRESNEL,

            /// Implements traditional / the original non-PBR blinn phong:
            ///     * Looks more like a 2000-2005's game
            ///     * Ignores fresnel completely.
            ///     * Works with Roughness in range (0; 1]. We automatically convert
            ///       this parameter for you to shininess.
            ///     * Assumes your Light power is set to PI (or a multiple) like with
            ///       most other Brdfs.
            ///     * Diffuse & Specular will automatically be
            ///       multiplied/divided by PI for you (assuming you
            ///       set your Light power to PI).
            /// The main scenario to use this BRDF is:
            ///     * Performance. This is the fastest BRDF.
            ///     * You were using Default, but are ok with how this one looks,
            ///       so you switch to this one instead.
            BlinnPhongLegacyMath = BlinnPhong | FLAG_LEGACY_MATH,

            /// Implements traditional / the original non-PBR blinn phong:
            ///     * Looks more like a 2000-2005's game
            ///     * Ignores fresnel completely.
            ///     * Roughness is actually the shininess parameter; which is in range (0; inf)
            ///       although most used ranges are in (0; 500].
            ///     * Assumes your Light power is set to 1.0.
            ///     * Diffuse & Specular is unmodified.
            /// There are two possible reasons to use this BRDF:
            ///     * Performance. This is the fastest BRDF.
            ///     * You're porting your app from Ogre 1.x and want to maintain that
            ///       Fixed-Function look for some odd reason, and your materials
            ///       already dealt in shininess, and your lights are already calibrated.
            ///
            /// Important: If switching from Default to BlinnPhongFullLegacy, you'll probably see
            /// that your scene is too bright. This is probably because Default divides diffuse
            /// by PI and you usually set your lights' power to a multiple of PI to compensate.
            /// If your scene is too bright, kist divide your lights by PI.
            /// BlinnPhongLegacyMath performs that conversion for you automatically at
            /// material level instead of doing it at light level.
            BlinnPhongFullLegacy = BlinnPhongLegacyMath | FLAG_FULL_LEGACY,
        };
    }

    /** Contains information needed by PBS (Physically Based Shading) for OpenGL 3+ & D3D11+
     */
    class _OgreHlmsPbsExport HlmsPbsDatablock : public HlmsPbsBaseTextureDatablock
    {
        friend class HlmsPbs;

    public:
        enum TransparencyModes
        {
            /// No alpha blending. Default.
            None,

            /// Realistic transparency that preserves lighting reflections
            /// (particularly specular on the edges). Great for glass, transparent
            /// plastic, most stuff. Note that at transparency = 0 the object may
            /// not be fully invisible.
            Transparent,

            /// Good 'ol regular alpha blending. Ideal for just fading out an
            /// object until it completely disappears.
            Fade,

            /// Similar to transparent, but also performs refractions.
            /// The compositor scene pass must be set to render refractive
            /// objects in its own pass.
            ///
            /// See Samples/2.0/ApiUsage/Refractions
            Refractive
        };

        enum Workflows
        {
            /// Specular workflow. Many popular PBRs use SpecularAsFresnelWorkflow
            /// though. @see setWorkflow
            SpecularWorkflow,

            /// Specular workflow where the specular texture is addressed to the fresnel
            /// instead of kS. This is normally referred as simply Specular workflow
            /// in many other PBRs. @see setWorkflow
            SpecularAsFresnelWorkflow,

            //// Metallic workflow. @see setWorkflow
            MetallicWorkflow,
        };

    protected:
        /// [0] = Regular one.
        /// [1] = Used during shadow mapping
        // uint16  mFullParametersBytes[2];
        uint8             mUvSource[NUM_PBSM_SOURCES];
        uint8             mBlendModes[4];
        uint8             mFresnelTypeSizeBytes;  // 4 if mFresnel is float, 12 if it is vec3
        bool              mTwoSided;
        bool              mUseAlphaFromTextures;
        uint8             mWorkflow;
        bool              mReceiveShadows;
        uint8             mCubemapIdxInDescSet;
        bool              mUseEmissiveAsLightmap;
        bool              mUseDiffuseMapAsGrayscale;
        TransparencyModes mTransparencyMode;

        float mBgDiffuse[4];
        float mkDr, mkDg, mkDb;  // kD
        float _padding0;
        float mkSr, mkSg, mkSb;  // kS
        float mRoughness;
        float mFresnelR, mFresnelG, mFresnelB;  // F0
        float mTransparencyValue;
        float mDetailNormalWeight[4];
        float mDetailWeight[4];
        float mDetailsOffsetScale[4][4];
        float mEmissive[3];
        float mNormalMapWeight;
        float mRefractionStrength;
        float mClearCoat;
        float mClearCoatRoughness;
        float _padding1;
        float mUserValue[3][4];  // can be used in custom pieces
        // uint16  mTexIndices[NUM_PBSM_TEXTURE_TYPES];

        CubemapProbe *mCubemapProbe;

        /// @see PbsBrdf::PbsBrdf
        uint32 mBrdf;

        void cloneImpl( HlmsDatablock *datablock ) const override;

        void bakeTextures( bool hasSeparateSamplers ) override;
        void scheduleConstBufferUpdate();
        void uploadToConstBuffer( char *dstPtr, uint8 dirtyFlags ) override;
        void notifyOptimizationStrategyChanged() override;

    public:
        /** Valid parameters in params:
        @param params
            + fresnel \<value [g, b]>
                The IOR. See setIndexOfRefraction()
                When specifying three values, the fresnel is separate for each
                colour component

            + fresnel_coeff \<value [g, b]>
                Directly sets the fresnel values, instead of using IORs
                "F0" in most books about PBS

            + roughness \<value>
                Specifies the roughness value. Should be in range (0; inf)
                Note: Values extremely close to zero could cause NaNs and
                INFs in the pixel shader, also depends on the GPU's precision.

            + background_diffuse \<r g b a>
                Specifies diffuse colour to use as a background when diffuse texture are not present.
                Does not replace 'diffuse \<r g b>'
                Default: background_diffuse 1 1 1 1

            + diffuse \<r g b>
                Specifies the RGB diffuse colour. "kD" in most books about PBS
                Default: diffuse 1 1 1 1
                Note: Internally the diffuse colour is divided by PI.

            + diffuse_map \<texture name>
                Name of the diffuse texture for the base image (optional)

            + diffuse_map_grayscale \<true, false>
                When set to true diffuse map would be sampled with .rrra swizzle
                Default: false

            + specular \<r g b>
                Specifies the RGB specular colour. "kS" in most books about PBS
                Default: specular 1 1 1 1

            + specular_map \<texture name>
                Name of the specular texture for the base image (optional).

            + roughness_map \<texture name>
                Name of the roughness texture for the base image (optional)
                Note: Only the Red channel will be used, and the texture will be converted to
                an efficient monochrome representation.

            + normal_map \<texture name>
                Name of the normal texture for the base image (optional) for normal mapping

            + detail_weight_map \<texture name>
                Texture that when present, will be used as mask/weight for the 4 detail maps.
                The R channel is used for detail map #0; the G for detail map #1, B for #2,
                and Alpha for #3.
                This affects both the diffuse and normal-mapped detail maps.

            + detail_map0 \<texture name>
              Similar: detail_map1, detail_map2, detail_map3
                Name of the detail map to be used on top of the diffuse colour.
                There can be gaps (i.e. set detail maps 0 and 2 but not 1)

            + detail_blend_mode0 \<blend_mode>
              Similar: detail_blend_mode1, detail_blend_mode2, detail_blend_mode3
                Blend mode to use for each detail map. Valid values are:
                    "NormalNonPremul", "NormalPremul", "Add", "Subtract", "Multiply",
                    "Multiply2x", "Screen", "Overlay", "Lighten", "Darken",
                    "GrainExtract", "GrainMerge", "Difference"

            + detail_offset_scale0 \<offset_u> \<offset_v> \<scale_u> \<scale_v>
              Similar: detail_offset_scale1, detail_offset_scale2, detail_offset_scale3
                Sets the UV offset and scale of the detail maps.

            + detail_normal_map0 \<texture name>
              Similar: detail_normal_map1, detail_normal_map2, detail_normal_map3
                Name of the detail map's normal map to be used.
                It's not affected by blend mode. May be used even if
                there is no detail_map

            + detail_normal_offset_scale0 \<offset_u> \<offset_v> \<scale_u> \<scale_v>
              Similar: detail_normal_offset_scale1, detail_normal_offset_scale2,
                       detail_normal_offset_scale3
                Sets the UV offset and scale of the detail normal maps.

            + reflection_map \<texture name>
                Name of the reflection map. Must be a cubemap. Doesn't use an UV set because
                the tex. coords are automatically calculated.

            + uv_diffuse_map \<uv>
              Similar: uv_specular_map, uv_normal_map, uv_detail_mapN, uv_detail_normal_mapN,
                       uv_detail_weight_map
              where N is a number between 0 and 3.
                UV set to use for the particular texture map.
                The UV value must be in range [0; 8)

            + transparency \<value>
              Specifies the transparency amount. Value in range [0; 1]
              where 0 = full transparency and 1 = fully opaque.

            + transparency_mode \<transparent, none, fade>
              Specifies the transparency mode. @see TransparencyModes

            + alpha_from_textures \<true, false>
              When set to false transparency calculations ignore the alpha channel in
              the textures
        */
        HlmsPbsDatablock( IdString name, HlmsPbs *creator, const HlmsMacroblock *macroblock,
                          const HlmsBlendblock *blendblock, const HlmsParamVec &params );
        ~HlmsPbsDatablock() override;

        /// Sets the diffuse background colour. When no diffuse texture is present, this
        /// solid colour replaces it, and can act as a background for the detail maps.
        void        setBackgroundDiffuse( const ColourValue &bgDiffuse );
        ColourValue getBackgroundDiffuse() const;

        /// Sets the diffuse colour (final multiplier). The colour will be divided by PI for energy
        /// conservation.
        void    setDiffuse( const Vector3 &diffuseColour );
        Vector3 getDiffuse() const;

        /// Sets the specular colour.
        void    setSpecular( const Vector3 &specularColour );
        Vector3 getSpecular() const;

        /// Sets the roughness
        void  setRoughness( float roughness );
        float getRoughness() const;

        /// Sets emissive colour (e.g. a firefly). Emissive colour has no physical basis.
        /// Though in HDR, if you're working in lumens, this value should probably be in lumens too.
        /// To disable emissive, setEmissive( Vector3::ZERO ) and unset any texture
        /// in PBSM_EMISSIVE slot.
        void    setEmissive( const Vector3 &emissiveColour );
        Vector3 getEmissive() const;
        /// Returns true iif getEmissive is non-zero
        bool hasEmissiveConstant() const;
        /// Returns true if getEmissive is non-zero or if there is an emissive texture set
        bool _hasEmissive() const;

        /** Sets whether to use a specular workflow, or a metallic workflow.
        @remarks
            The texture types PBSM_SPECULAR & PBSM_METALLIC map to the same value.
        @par
            When in metal workflow, the texture is used as a metallic texture,
            and is expected to be a monochrome texture. Global specularity's strength
            can still be affected via setSpecular. The fresnel settings should not
            be used (metalness is stored where fresnel used to).
        @par
            When in specular workflow, the texture is used as a specular texture,
            and is expected to be either coloured or monochrome.
            setMetalness should not be called in this mode.
        @par
            If "workflow" was different from the current setting, it will call
            HlmsDatablock::flushRenderables. If the another shader must be created,
            it could cause a stall.
        @param bEnableMetallic
        */
        void      setWorkflow( Workflows workflow );
        Workflows getWorkflow() const;

        /** Sets the metalness in a metallic workflow.
        @remarks
            Overrides any fresnel value.
            Should be in Metallic mode. @see setWorkflow;
        @param metalness
            Value in range [0; 1]
        */
        void  setMetalness( float metalness );
        float getMetalness() const;

        /** Calculates fresnel (F0 in most books) based on the IOR.
            The formula used is ( (1 - idx) / (1 + idx) )²
        @remarks
            If "separateFresnel" was different from the current setting, it will call
            HlmsDatablock::flushRenderables(). If the another shader must be created,
            it could cause a stall.
        @param refractionIdx
            The index of refraction of the material for each colour component.
            When separateFresnel = false, the Y and Z components are ignored.
        @param separateFresnel
            Whether to use the same fresnel term for RGB channel, or individual ones.
        */
        void setIndexOfRefraction( const Vector3 &refractionIdx, bool separateFresnel );

        /** Sets the fresnel (F0) directly, instead of using the IOR. See setIndexOfRefraction().
        @remarks
            If "separateFresnel" was different from the current setting, it will call
            HlmsDatablock::flushRenderables. If the another shader must be created,
            it could cause a stall.
        @param refractionIdx
            The fresnel of the material for each colour component.
            When separateFresnel = false, the Y and Z components are ignored.
        @param separateFresnel
            Whether to use the same fresnel term for RGB channel, or individual ones.
        */
        void setFresnel( const Vector3 &fresnel, bool separateFresnel );

        /// Returns the current fresnel. Note: when hasSeparateFresnel returns false,
        /// the Y and Z components still correspond to mFresnelG & mFresnelB just
        /// in case you want to preserve this data (i.e. toggling separate fresnel
        /// often (which is not a good idea though, in terms of performance)
        Vector3 getFresnel() const;

        /// Whether the same fresnel term is used for RGB, or individual ones for each channel
        bool hasSeparateFresnel() const;

        using HlmsPbsBaseTextureDatablock::setTexture;
        void setTexture( PbsTextureTypes texUnit, const String &name,
                         const HlmsSamplerblock *refParams = 0 );

        /** Sets which UV set to use for the given texture.
            Calling this function triggers a HlmsDatablock::flushRenderables.
        @param sourceType
            Source texture to modify. Note that we don't enforce
            PBSM_SOURCE_DETAIL0 = PBSM_SOURCE_DETAIL_NM0, but you probably
            want to have both textures using the same UV source.
            Must be lower than NUM_PBSM_SOURCES.
        @param uvSet
            UV coordinate set. Value must be between in range [0; 8)
        */
        void setTextureUvSource( PbsTextureTypes sourceType, uint8 uvSet );

        uint8 getTextureUvSource( PbsTextureTypes sourceType ) const;

        /** Changes the blend mode of the detail map. Calling this function triggers a
            HlmsDatablock::flushRenderables even if you never use detail maps (they
            affect the cache's hash)
        @remarks
            This parameter only affects the diffuse detail map. Not the normal map.
        @param detailMapIdx
            Value in the range [0; 4)
        @param blendMode
            Blend mode
        */
        void setDetailMapBlendMode( uint8 detailMapIdx, PbsBlendModes blendMode );

        PbsBlendModes getDetailMapBlendMode( uint8 detailMapIdx ) const;

        /** Sets the normal mapping weight. The range doesn't necessarily have to be in [0; 1]
        @remarks
            An exact value of 1 will generate a shader without the weighting math, while any
            other value will generate another shader that uses this weight (i.e. will
            cause a flushRenderables)
        @param detailNormalMapIdx
            Value in the range [0; 4)
        @param weight
            The weight for the normal map.
            A value of 0 means no effect (tangent space normal is 0, 0, 1); and would be
            the same as disabling the normal map texture.
            A value of 1 means full normal map effect.
            A value outside the [0; 1] range extrapolates.
            Default value is 1.
        */
        void setDetailNormalWeight( uint8 detailNormalMapIdx, Real weight );

        /// Returns the detail normal maps' weight
        Real getDetailNormalWeight( uint8 detailNormalMapIdx ) const;

        /// See setDetailNormalWeight(). This is the same, but for the main normal map.
        void setNormalMapWeight( Real weight );

        /// Returns the detail normal maps' weight
        Real getNormalMapWeight() const;

        /** Sets the weight of detail map. Affects both diffuse and
            normal at the same time.
        @remarks
            A value of 1 will cause a flushRenderables as we remove the code from the
            shader.
            The weight from setNormalMapWeight() is multiplied against this value
            when it comes to the detail normal map.
        @param detailMap
            Value in the range [0; 4)
        @param weight
            The weight for the detail map. Usual values are in range [0; 1] but any
            value is accepted and valid.
            Default value is 1
        */
        void setDetailMapWeight( uint8 detailMap, Real weight );
        Real getDetailMapWeight( uint8 detailMap ) const;

        /** Sets the scale and offset of the detail map.
        @remarks
            A value of Vector4( 0, 0, 1, 1 ) will cause a flushRenderables as we
            remove the code from the shader.
        @param detailMap
            Value in the range [0; 4)
        @param offsetScale
            XY = Contains the UV offset.
            ZW = Constains the UV scale.
            Default value is Vector4( 0, 0, 1, 1 )
        */
        void    setDetailMapOffsetScale( uint8 detailMap, const Vector4 &offsetScale );
        Vector4 getDetailMapOffsetScale( uint8 detailMap ) const;

        /** Allows support for two sided lighting. Disabled by default (faster)
        @remarks
            Changing this parameter will cause a flushRenderables
        @param twoSided
            Whether to enable or disable.
        @param changeMacroblock
            Whether to change the current macroblock for one that has cullingMode = CULL_NONE
            or set it to false to leave the current macroblock as is.
        @param oneSidedShadowCast
            If changeMacroblock == true; this parameter controls the culling mode of the
            shadow caster.
            While oneSidedShadowCast == CULL_NONE is usually the "correct" option, setting
            oneSidedShadowCast=CULL_ANTICLOCKWISE can prevent ugly self-shadowing on interiors.
        */
        void setTwoSidedLighting( bool twoSided, bool changeMacroblock = true,
                                  CullingMode oneSidedShadowCast = CULL_NONE );
        bool getTwoSidedLighting() const;

        void setAlphaTest( CompareFunction compareFunction, bool shadowCasterOnly = false,
                           bool useAlphaFromTextures = true ) override;

        /** @see HlmsDatablock::setAlphaTest
        @remarks
            Alpha testing works on the alpha channel of the diffuse texture.
            If there is no diffuse texture, the first diffuse detail map after
            applying the blend weights (texture + params) is used.
            If there are no diffuse nor detail-diffuse maps, the alpha test is
            compared against the value 1.0
        */
        void setAlphaTestThreshold( float threshold ) override;

        /** Makes the material transparent, and sets the amount of transparency
        @param transparency
            Value in range [0; 1] where 0 = full transparency and 1 = fully opaque.
        @param mode
            see #TransparencyModes
        @param useAlphaFromTextures
            When false, the alpha channel of the diffuse maps and detail maps will be
            ignored. It's a GPU performance optimization.
        @param changeBlendblock
            When true, the routine prepares the ideal blendblock to use according to the
            selected mode.
            When false, the user is expected to change the blendblock manually before
            calling this function to prevent false warnings. Useful for advanced experiments
            or artistic license.
        */
        void setTransparency( float transparency, TransparencyModes mode = Transparent,
                              bool useAlphaFromTextures = true, bool changeBlendblock = true );

        float             getTransparency() const { return mTransparencyValue; }
        TransparencyModes getTransparencyMode() const { return mTransparencyMode; }
        bool              getUseAlphaFromTextures() const { return mUseAlphaFromTextures; }

        /** Sets the strength of the refraction, i.e. how much displacement in screen space.

            This value is not physically based.
            Only used when HlmsPbsDatablock::setTransparency was set to HlmsPbsDatablock::Refractive
        @param strength
            Refraction strength. Useful range is often (0; 1) but any value is valid (even negative),
            but the bigger the number, the more likely glitches will appear (with large values
            we have to fallback to regular alpha blending due to the screen space pixel landing
            outside the screen)
        */
        void  setRefractionStrength( float strength );
        float getRefractionStrength() const { return mRefractionStrength; }

        /** Sets the strength of the of the clear coat layer and its roughness.
        @param clearCoat
            This should be treated as a binary value, set to either 0 or 1. Intermediate values are
            useful to control transitions between parts of the surface that have a clear coat layers and
            parts that don't.
        */
        void  setClearCoat( float clearCoat );
        void  setClearCoatRoughness( float roughness );
        float getClearCoat() const { return mClearCoat; }
        float getClearCoatRoughness() const { return mClearCoatRoughness; }

        /** When false, objects with this material will not receive shadows (independent of
            whether they case shadows or not)
        @remarks
            Changing this parameter will cause a flushRenderables
            Shadow casting lights (which are normally processed in a Forward way)
            will still lit the object, it just won't have shadows in it.
        @param receiveShadows
            Whether to enable or disable receiving shadows.
        */
        void setReceiveShadows( bool receiveShadows );
        bool getReceiveShadows() const;

        /** Sets the value of the userValue, this can be used in a custom piece
        @param userValueIdx
            Which userValue to modify, in the range [0; 3)
        */
        void    setUserValue( uint8 userValueIdx, const Vector4 &value );
        Vector4 getUserValue( uint8 userValueIdx ) const;

        /** When set, it treats the emissive map as a lightmap; which means it will
            be multiplied against the diffuse component.
        @remarks
            Note that HlmsPbsDatablock::setEmissive still applies,
            thus set it to 1 to avoid surprises.
        */
        void setUseEmissiveAsLightmap( bool bUseEmissiveAsLightmap );
        bool getUseEmissiveAsLightmap() const;

        /** When set, it treats the diffuse map as a grayscale map; which means it will
            spread red component to all rgb channels.
        @remarks
            With this option you can use PFG_R8_UNORM for diffuse map in the same way as old PF_L8 format
        */
        void setUseDiffuseMapAsGrayscale( bool bUseDiffuseMapAsGrayscale );
        bool getUseDiffuseMapAsGrayscale() const;

        /** Manually set a probe to affect this particular material.
        @remarks
            PCC (Parallax Corrected Cubemaps) have two main forms of operation: Auto and manual.
            They both have advantages and disadvantages. This method allows you to enable
            the manual mode of operation.
        @par
            Manual Advantages:
                + It's independent of camera position.
                + The reflections are always visible and working on the object.
                + Works best for static objects
                + Also works well on dynamic objects that you can guarantee are going
                  to be constrained to the probe's area.
            Manual Disadvantages:
                + Needs to be manually applied on the material by the user.
                + Can produce harsh lighting/reflection seams when two objects affected
                  by different probes are close together.
                + Sucks for dynamic objects.

            To use manual probes just call:
                datablock->setCubemapProbe( probe );
        @par
            Auto Advantages:
                + Smoothly blends between probes, making smooth transitions
                + Avoids seams.
                + No need to change anything on the material or the object,
                  you don't need to do anything.
                + Works best for dynamic objects (eg. characters)
                + Also works well on static objects if the camera is inside rooms/corridors, thus
                  blocking the view from distant rooms that aren't receiving reflections.
            Auto Disadvantages:
                + Objects that are further away won't have reflections as
                  only one probe can be active.
                + It depends on the camera's position.
                + Doesn't work well if the user can see many distant objects at once, as they
                  won't have reflections until you get close.

            To use Auto you don't need to do anything. Just enable PCC:
                hlmsPbs->setParallaxCorrectedCubemap( mParallaxCorrectedCubemap );
            and leave PBSM_REFLECTION empty and don't enable manual mode.
        @par
            When other reflection methods can be used as fallback (Planar reflections, SSR),
            then usually auto will be the preferred choice.
            But if multiple reflections/fallbacks aren't available, you'll likely have to make
            good use of manual and auto
        @param probe
            The probe that should affect this material to enable manual mode.
            Null pointer to disable manual mode and switch to auto.
        */
        void          setCubemapProbe( CubemapProbe *probe );
        CubemapProbe *getCubemapProbe() const;

        /// Changes the BRDF in use. Calling this function may trigger an
        /// HlmsDatablock::flushRenderables
        void   setBrdf( PbsBrdf::PbsBrdf brdf );
        uint32 getBrdf() const;

        /** Helper function to import & convert values from Unity (specular workflow).
        @param changeBrdf
            True if you want us to select a BRDF that closely matches that of Unity.
            It will change the BRDF to PbsBrdf::DefaultUncorrelated.
            For best realism though, it is advised that you use that you actually use
            PbsBrdf::Default.
        */
        void importUnity( const Vector3 &diffuse, const Vector3 &specular, Real roughness,
                          bool changeBrdf );

        /** Helper function to import values from Unity (metallic workflow).
        @remarks
            The metallic parameter will be converted to a specular workflow.
        */
        void importUnity( const Vector3 &colour, Real metallic, Real roughness, bool changeBrdf );

        /** Suggests the TextureMapType (aka texture category) for each type of texture
            (i.e. normals should load from TEXTURE_TYPE_NORMALS).
        @remarks
            Remember that if "myTexture" was loaded as TEXTURE_TYPE_DIFFUSE and then you try
            to load it as TEXTURE_TYPE_NORMALS, the first one will prevail until it's removed.
            You could create an alias however, and thus have two copies of the same texture with
            different loading parameters.
        */
        bool   suggestUsingSRGB( PbsTextureTypes type ) const;
        uint32 suggestFiltersForType( PbsTextureTypes type ) const;

        ColourValue getDiffuseColour() const override;
        ColourValue getEmissiveColour() const override;
        TextureGpu *getDiffuseTexture() const override;
        TextureGpu *getEmissiveTexture() const override;

        void notifyTextureChanged( TextureGpu *texture, TextureGpuListener::Reason reason,
                                   void *extraData ) override;

        void calculateHash() override;

        static const uint32 MaterialSizeInGpu;
        static const uint32 MaterialSizeInGpuAligned;
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
