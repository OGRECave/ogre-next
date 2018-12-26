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
#ifndef _OgreHlmsTerraDatablock_H_
#define _OgreHlmsTerraDatablock_H_

#include "Terra/Hlms/OgreHlmsTerraPrerequisites.h"
#include "OgreHlmsDatablock.h"
#include "OgreHlmsTextureManager.h"
#include "OgreConstBufferPool.h"
#include "OgreVector4.h"

#define _OgreHlmsTextureBaseClassExport
#define OGRE_HLMS_TEXTURE_BASE_CLASS HlmsTerraBaseTextureDatablock
#define OGRE_HLMS_TEXTURE_BASE_MAX_TEX NUM_TERRA_TEXTURE_TYPES
#define OGRE_HLMS_CREATOR_CLASS HlmsTerra
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
    namespace TerraBrdf
    {
    enum TerraBrdf
    {
        FLAG_UNCORRELATED                           = 0x80000000,
        FLAG_SPERATE_DIFFUSE_FRESNEL                = 0x40000000,
        FLAG_LEGACY_MATH                            = 0x20000000,
        FLAG_FULL_LEGACY                            = 0x08000000,
        BRDF_MASK                                   = 0x00000FFF,

        /// Most physically accurate BRDF we have. Good for representing
        /// the majority of materials.
        /// Uses:
        ///     * Roughness/Distribution/NDF term: GGX
        ///     * Geometric/Visibility term: Smith GGX Height-Correlated
        ///     * Normalized Disney Diffuse BRDF,see
        ///         "Moving Frostbite to Physically Based Rendering" from
        ///         Sebastien Lagarde & Charles de Rousiers
        Default         = 0x00000000,

        /// Implements Cook-Torrance BRDF.
        /// Uses:
        ///     * Roughness/Distribution/NDF term: Beckmann
        ///     * Geometric/Visibility term: Cook-Torrance
        ///     * Lambertian Diffuse.
        ///
        /// Ideal for silk (use high roughness values), synthetic fabric
        CookTorrance    = 0x00000001,

        /// Implements Normalized Blinn Phong using a normalization
        /// factor of (n + 8) / (8 * pi)
        /// The main reason to use this BRDF is performance. It's cheaper,
        /// while still looking somewhat similar to Default.
        /// If you still need more performance, see BlinnPhongLegacy
        BlinnPhong      = 0x00000002,

        /// Same as Default, but the geometry term is not height-correlated
        /// which most notably causes edges to be dimmer and is less correct.
        /// Unity (Marmoset too?) use an uncorrelated term, so you may want to
        /// use this BRDF to get the closest look for a nice exchangeable
        /// pipeline workflow.
        DefaultUncorrelated             = Default|FLAG_UNCORRELATED,

        /// Same as Default but the fresnel of the diffuse is calculated
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
        DefaultSeparateDiffuseFresnel   = Default|FLAG_SPERATE_DIFFUSE_FRESNEL,

        /// @see DefaultSeparateDiffuseFresnel. This is the same
        /// but the Cook Torrance model is used instead.
        ///
        /// Ideal for shiny objects like glass toy marbles, some types of rubber.
        /// silk, synthetic fabric.
        CookTorranceSeparateDiffuseFresnel  = CookTorrance|FLAG_SPERATE_DIFFUSE_FRESNEL,

        /// Like DefaultSeparateDiffuseFresnel, but uses BlinnPhong as base.
        BlinnPhongSeparateDiffuseFresnel    = BlinnPhong|FLAG_SPERATE_DIFFUSE_FRESNEL,

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
        BlinnPhongLegacyMath                = BlinnPhong|FLAG_LEGACY_MATH,

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
        BlinnPhongFullLegacy                = BlinnPhongLegacyMath|FLAG_FULL_LEGACY,
    };
    }

    /** Contains information needed by TERRA (Physically Based Shading) for OpenGL 3+ & D3D11+
    */
    class HlmsTerraDatablock : public HlmsTerraBaseTextureDatablock
    {
        friend class HlmsTerra;

    protected:
        float   mkDr, mkDg, mkDb;                   //kD
        float   _padding0;
        float   mRoughness[4];
        float   mMetalness[4];
        Vector4 mDetailsOffsetScale[4];
        //uint16  mTexIndices[NUM_TERRA_TEXTURE_TYPES];

        /// @see TerraBrdf::TerraBrdf
        uint32  mBrdf;

        virtual void cloneImpl( HlmsDatablock *datablock ) const;

        void scheduleConstBufferUpdate(void);
        virtual void uploadToConstBuffer( char *dstPtr, uint8 dirtyFlags );

    public:
        HlmsTerraDatablock( IdString name, HlmsTerra *creator,
                          const HlmsMacroblock *macroblock,
                          const HlmsBlendblock *blendblock,
                          const HlmsParamVec &params );
        virtual ~HlmsTerraDatablock();

        /// Sets overall diffuse colour. The colour will be divided by PI for energy conservation.
        void setDiffuse( const Vector3 &diffuseColour );
        Vector3 getDiffuse(void) const;

        /// Sets the roughness
        void setRoughness( uint8 detailMapIdx, float roughness );
        float getRoughness( uint8 detailMapIdx ) const;

        /** Sets the metalness in a metallic workflow.
        @remarks
            Overrides any fresnel value.
            Should be in Metallic mode. @see setWorkflow;
        @param metalness
            Value in range [0; 1]
        */
        void setMetalness( uint8 detailMapIdx, float metalness );
        float getMetalness( uint8 detailMapIdx ) const;

        /** Sets the scale and offset of the detail map.
        @remarks
            A value of Vector4( 0, 0, 1, 1 ) will cause a flushRenderables as we
            remove the code from the shader.
        @param detailMap
            Value in the range [0; 8)
            Range [0; 4) affects diffuse maps.
            Range [4; 8) affects normal maps.
        @param offsetScale
            XY = Contains the UV offset.
            ZW = Constains the UV scale.
            Default value is Vector4( 0, 0, 1, 1 )
        */
        void setDetailMapOffsetScale( uint8 detailMap, const Vector4 &offsetScale );
        const Vector4& getDetailMapOffsetScale( uint8 detailMap ) const;

        /// Overloaded to tell it's unsupported
        virtual void setAlphaTestThreshold( float threshold );

        /// Changes the BRDF in use. Calling this function may trigger an
        /// HlmsDatablock::flushRenderables
        void setBrdf( TerraBrdf::TerraBrdf brdf );
        uint32 getBrdf(void) const;

        /** Suggests the TextureMapType (aka texture category) for each type of texture
            (i.e. normals should load from TEXTURE_TYPE_NORMALS).
        @remarks
            Remember that if "myTexture" was loaded as TEXTURE_TYPE_DIFFUSE and then you try
            to load it as TEXTURE_TYPE_NORMALS, the first one will prevail until it's removed.
            You could create an alias however, and thus have two copies of the same texture with
            different loading parameters.
        */
//        static HlmsTextureManager::TextureMapType suggestMapTypeBasedOnTextureType(
//                                                                TerraTextureTypes type );
        bool suggestUsingSRGB( TerraTextureTypes type ) const;
        uint32 suggestFiltersForType( TerraTextureTypes type ) const;

        virtual void calculateHash();

        static const size_t MaterialSizeInGpu;
        static const size_t MaterialSizeInGpuAligned;
    };

    /** @} */
    /** @} */

}

#include "OgreHeaderSuffix.h"

#endif
