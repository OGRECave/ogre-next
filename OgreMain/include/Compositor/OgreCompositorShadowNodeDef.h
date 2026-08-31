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

#ifndef __CompositorShadowNodeDef_H__
#define __CompositorShadowNodeDef_H__

#include "OgreHeaderPrefix.h"

#include "Compositor/OgreCompositorNodeDef.h"
#include "OgreMath.h"
#include "OgreVector2.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Effects
     *  @{
     */

    enum ShadowMapTechniques
    {
        SHADOWMAP_UNIFORM,  // Default
        SHADOWMAP_PLANEOPTIMAL,
        SHADOWMAP_FOCUSED,
        SHADOWMAP_PSSM
    };

    /// Local texture definition
    class ShadowTextureDefinition : public OgreAllocatedObj
    {
    public:
        Vector2 uvOffset;
        Vector2 uvLength;
        uint8   arrayIdx;

        size_t light;  // Render Nth closest light
        size_t split;  // Split for that light (only for PSSM/CSM)

        /// Constant bias is per material (tweak HlmsDatablock::mShadowConstantBias).
        /// This value lets you multiply it 'mShadowConstantBias * constantBiasScale'
        /// per cascade / shadow map
        ///
        /// This is applied on top of the autocalculated bias from autoConstantBiasScale
        float constantBiasScale;
        /// Normal offset bias is per cascade / shadow map
        ///
        /// This is applied on top of the autocalculated bias from autoNormalOffsetBiasScale
        float normalOffsetBias;

        /// 0 to disable.
        /// Non-zero to increase bias based on orthographic projection's window size.
        float autoConstantBiasScale;
        /// 0 to disable.
        /// Non-zero to increase bias based on orthographic projection's window size.
        float autoNormalOffsetBiasScale;

        ShadowMapTechniques shadowMapTechnique;

        // Focused params (also applies to PSSM)
        float xyPadding;

        // PSSM params
        Real   pssmLambda;
        Real   splitPadding;
        Real   splitBlend;
        Real   splitFade;
        uint32 numSplits;
        uint32 numStableSplits;

    protected:
        IdString texName;
        String   texNameStr;
        size_t   sharesSetupWith;

    public:
        ShadowTextureDefinition( ShadowMapTechniques t, const String &texRefName,
                                 const Vector2 &_uvOffset, const Vector2 &_uvLength, uint8 _arrayIdx,
                                 size_t _light, size_t _split ) :
            uvOffset( _uvOffset ),
            uvLength( _uvLength ),
            arrayIdx( _arrayIdx ),
            light( _light ),
            split( _split ),
            constantBiasScale( 1.0f ),
            normalOffsetBias( 168.0f ),
            autoConstantBiasScale( 100.0f ),
            autoNormalOffsetBiasScale( 4.0f ),
            shadowMapTechnique( t ),
            xyPadding( 1.5f ),
            pssmLambda( 0.95f ),
            splitPadding( 1.0f ),
            splitBlend( 0.125f ),
            splitFade( 0.313f ),
            numSplits( 3u ),
            numStableSplits( 0u ),
            texName( texRefName ),
            texNameStr( texRefName ),
            sharesSetupWith( std::numeric_limits<size_t>::max() )
        {
        }

        IdString getTextureName() const { return texName; }
        String   getTextureNameStr() const { return texNameStr; }

        void   _setSharesSetupWithIdx( size_t idx ) { sharesSetupWith = idx; }
        size_t getSharesSetupWith() const { return sharesSetupWith; }
    };

    /** Shadow Nodes are special nodes (not to be confused with CompositorNode)
        that are only used for rendering shadow maps.
        Normal Compositor Nodes can share or own a ShadowNode. The ShadowNode will
        render the scene enough times to fill all shadow maps so the main scene pass
        can use them.
    @par
        ShadowNode are very flexible compared to Ogre 1.x; as they allow mixing multiple
        shadow camera setups for different lights.
    @author
        Matias N. Goldberg
    @version
        1.0
    */
    class _OgreExport CompositorShadowNodeDef : public CompositorNodeDef
    {
        friend class CompositorShadowNode;

    protected:
        typedef vector<ShadowTextureDefinition>::type ShadowMapTexDefVec;
        typedef vector<uint8>::type                   LightTypeMaskVec;
        ShadowMapTexDefVec                            mShadowMapTexDefinitions;
        /// Some shadow maps may only support a few light types (e.g.
        /// PSSM only supports directional lights).
        /// In the example this would be 1u << Light::LT_DIRECTIONAL
        /// The size is one per light, not per shadow map.
        LightTypeMaskVec    mLightTypesMask;
        ShadowMapTechniques mDefaultTechnique;

        /// Not the same as mShadowMapTexDefinitions.size(), because splits aren't included
        size_t mNumLights;
        size_t mMinRq;  // Minimum RQ included by one of our passes
        size_t mMaxRq;  // Maximum RQ included by one of our passes

    public:
        CompositorShadowNodeDef( const String &name, CompositorManager2 *compositorManager ) :
            CompositorNodeDef( name, compositorManager ),
            mDefaultTechnique( SHADOWMAP_UNIFORM ),
            mNumLights( 0 ),
            mMinRq( std::numeric_limits<size_t>::max() ),
            mMaxRq( 0 )
        {
        }
        ~CompositorShadowNodeDef() override {}

        /// Overloaded to prevent creating input channels.
        IdString addTextureSourceName( const String &name, size_t index,
                                       TextureSource textureSource ) override;
        void     addBufferInput( size_t inputChannel, IdString name ) override;

        void postInitializePassDef( CompositorPassDef *passDef ) override;

        void setDefaultTechnique( ShadowMapTechniques techn ) { mDefaultTechnique = techn; }

        /** Reserves enough memory for all texture definitions
        @remarks
            Calling this function is not obligatory, but recommended
        @param numTex
            The number of shadow textures expected to contain.
        */
        void setNumShadowTextureDefinitions( size_t numTex );

        /** Adds a new ShadowTexture definition.
        @remarks
            WARNING: Calling this function may invalidate all previous returned pointers
            unless you've properly called setNumShadowTextureDefinitions
        @param lightIdx
            Nth Closest Light to assign this texture to. Must be unique unless split is different.
        @param split
            Split for the given light. Only valid for CSM/PSSM shadow maps.
            Must be unique for the same lightIdx.
        @param name
            Name to a declared texture that will hold our shadow map.
            Must not contain the "global_" prefix.
        @param uvOffset
            Values in range [0; 1] to determine what region of the texture will hold our shadow map
            (i.e. UV atlas). Use Vector2::ZERO if it covers the entire texture.
        @param uvLength
            Values in range [0; 1] to determine what region of the texture will hold our shadow map
            (i.e. UV atlas). Use Vector2::UNIT_SCALE if it covers the entire texture.
        @param arrayIdx
            If the texture is an array texture, index to the slice that holds our shadow map.
        */
        ShadowTextureDefinition *addShadowTextureDefinition( size_t lightIdx, size_t split,
                                                             const String &name, const Vector2 &uvOffset,
                                                             const Vector2 &uvLength, uint8 arrayIdx );

        /// Gets the number of shadow texture definitions in this node.
        size_t getNumShadowTextureDefinitions() const { return mShadowMapTexDefinitions.size(); }

        /// Retrieves a shadow texture definition by its index.
        const ShadowTextureDefinition *getShadowTextureDefinition( size_t texIndex ) const
        {
            return &mShadowMapTexDefinitions[texIndex];
        }
        ShadowTextureDefinition *getShadowTextureDefinitionNonConst( size_t texIndex )
        {
            return &mShadowMapTexDefinitions[texIndex];
        }

        /** Checks that paremeters are correctly set, and finalizes whatever needs to be
            done, probably because not enough data was available at the time of creation.
        @remarks
            If possible, try to validate parameters at creation time to avoid delaying
            when the error shows up.
            We should validate here if it's not possible to validate at any other time
            or if it's substantially easier to do so here.
        */
        virtual void _validateAndFinish();
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
