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

#include "OgreStableHeaders.h"

#if !OGRE_NO_JSON

#include "OgreHlmsJsonPbs.h"
#include "OgreHlmsManager.h"
#include "OgreTextureGpuManager.h"
#include "OgreTextureFilters.h"

#include "OgreLwString.h"

#include "OgreStringConverter.h"

#include "rapidjson/document.h"

namespace Ogre
{
    extern const String c_pbsBlendModes[];

    const char* c_workflows[HlmsPbsDatablock::MetallicWorkflow+1] =
    {
        "specular_ogre",
        "specular_fresnel",
        "metallic"
    };
    const char* c_transparencyModes[HlmsPbsDatablock::Refractive+1] =
    {
        "None",
        "Transparent",
        "Fade",
        "Refractive"
    };

    HlmsJsonPbs::HlmsJsonPbs( HlmsManager *hlmsManager, TextureGpuManager *textureManager,
                                                        HlmsJsonListener *listener, const String &additionalExtension ) :
        mHlmsManager( hlmsManager ),
        mTextureManager( textureManager ),
        mListener( listener ),
        mAdditionalExtension( additionalExtension )
    {
    }
    //-----------------------------------------------------------------------------------
    HlmsPbsDatablock::Workflows HlmsJsonPbs::parseWorkflow( const char *value )
    {
        if( !strcmp( value, "specular_ogre" ) )
            return HlmsPbsDatablock::SpecularWorkflow;
        if( !strcmp( value, "specular_fresnel" ) )
            return HlmsPbsDatablock::SpecularAsFresnelWorkflow;
        if( !strcmp( value, "metallic" ) )
            return HlmsPbsDatablock::MetallicWorkflow;

        return HlmsPbsDatablock::SpecularWorkflow;
    }
    //-----------------------------------------------------------------------------------
    PbsBrdf::PbsBrdf HlmsJsonPbs::parseBrdf( const char *value )
    {
        if( !strcmp( value, "default" ) )
            return PbsBrdf::Default;
        if( !strcmp( value, "cook_torrance" ) )
            return PbsBrdf::CookTorrance;
        if( !strcmp( value, "blinn_phong" ) )
            return PbsBrdf::BlinnPhong;
        if( !strcmp( value, "default_uncorrelated" ) )
            return PbsBrdf::DefaultUncorrelated;
        if( !strcmp( value, "default_separate_diffuse_fresnel" ) )
            return PbsBrdf::DefaultSeparateDiffuseFresnel;
        if( !strcmp( value, "cook_torrance_separate_diffuse_fresnel" ) )
            return PbsBrdf::CookTorranceSeparateDiffuseFresnel;
        if( !strcmp( value, "blinn_phong_separate_diffuse_fresnel" ) )
            return PbsBrdf::BlinnPhongSeparateDiffuseFresnel;
        if( !strcmp( value, "blinn_phong_legacy_math" ) )
            return PbsBrdf::BlinnPhongLegacyMath;
        if( !strcmp( value, "blinn_phong_full_legacy" ) )
            return PbsBrdf::BlinnPhongFullLegacy;

        return PbsBrdf::Default;
    }
    //-----------------------------------------------------------------------------------
    HlmsPbsDatablock::TransparencyModes HlmsJsonPbs::parseTransparencyMode( const char *value )
    {
        if( !strcmp( value, "None" ) )
            return HlmsPbsDatablock::None;
        if( !strcmp( value, "Transparent" ) )
            return HlmsPbsDatablock::Transparent;
        if( !strcmp( value, "Fade" ) )
            return HlmsPbsDatablock::Fade;

        return HlmsPbsDatablock::None;
    }
    //-----------------------------------------------------------------------------------
    PbsBlendModes HlmsJsonPbs::parseBlendMode( const char *value )
    {
        for( int i=0; i<NUM_PBSM_BLEND_MODES; ++i )
        {
            if( !strcmp( value, c_pbsBlendModes[i].c_str() ) )
                return static_cast<PbsBlendModes>( i );
        }

        return PBSM_BLEND_NORMAL_NON_PREMUL;
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonPbs::parseFresnelMode( const char *value, bool &outIsColoured, bool &outUseIOR )
    {
        if( !strcmp( value, "coeff" ) )
        {
            outUseIOR       = false;
            outIsColoured   = false;
        }
        else if( !strcmp( value, "ior" ) )
        {
            outUseIOR       = true;
            outIsColoured   = false;
        }
        else if( !strcmp( value, "coloured" ) )
        {
            outUseIOR       = false;
            outIsColoured   = true;
        }
        else if( !strcmp( value, "coloured_ior" ) )
        {
            outUseIOR       = true;
            outIsColoured   = true;
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonPbs::parseOffset( const rapidjson::Value &jsonArray, Vector4 &offsetScale )
    {
        const rapidjson::SizeType arraySize = std::min( 2u, jsonArray.Size() );
        for( rapidjson::SizeType i=0; i<arraySize; ++i )
        {
            if( jsonArray[i].IsNumber() )
                offsetScale[i] = static_cast<float>( jsonArray[i].GetDouble() );
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonPbs::parseScale( const rapidjson::Value &jsonArray, Vector4 &offsetScale )
    {
        const rapidjson::SizeType arraySize = std::min( 2u, jsonArray.Size() );
        for( rapidjson::SizeType i=0; i<arraySize; ++i )
        {
            if( jsonArray[i].IsNumber() )
                offsetScale[i+2u] = static_cast<float>( jsonArray[i].GetDouble() );
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonPbs::loadTexture( const rapidjson::Value &json, const HlmsJson::NamedBlocks &blocks,
                                   PbsTextureTypes textureType, HlmsPbsDatablock *datablock,
                                   const String &resourceGroup )
    {
        TextureGpu *texture = 0;
        HlmsSamplerblock const *samplerblock = 0;

        rapidjson::Value::ConstMemberIterator itor = json.FindMember( "texture" );
        if( itor != json.MemberEnd() &&
            (itor->value.IsString() || (itor->value.IsArray() && itor->value.Size() == 2u &&
                                        itor->value[0].IsString() && itor->value[1].IsString())) )
        {
            char const *textureName = 0;
            char const *aliasName = 0;
            if( !itor->value.IsArray() )
            {
                textureName = itor->value.GetString();
                aliasName = textureName;
            }
            else
            {
                textureName = itor->value[0].GetString();
                aliasName = itor->value[1].GetString();
            }

            if( textureType == PBSM_DIFFUSE )
            {
                itor = json.FindMember("grayscale");
                if( itor != json.MemberEnd() && itor->value.IsBool() )
                {
                    datablock->setUseDiffuseMapAsGrayscale( itor->value.GetBool() );
                }
            }

            if( textureType == PBSM_EMISSIVE )
            {
                itor = json.FindMember( "lightmap" );
                if( itor != json.MemberEnd() && itor->value.IsBool() )
                {
                    datablock->setUseEmissiveAsLightmap( itor->value.GetBool() );
                }
            }

            uint32 textureFlags = TextureFlags::AutomaticBatching;

            if( datablock->suggestUsingSRGB( textureType ) )
                textureFlags |= TextureFlags::PrefersLoadingFromFileAsSRGB;

            TextureTypes::TextureTypes internalTextureType = TextureTypes::Type2D;
            if( textureType == PBSM_REFLECTION )
            {
                internalTextureType = TextureTypes::TypeCube;
                textureFlags &= ~TextureFlags::AutomaticBatching;
            }

            uint32 filters = TextureFilter::TypeGenerateDefaultMipmaps;
            filters |= datablock->suggestFiltersForType( textureType );

            texture = mTextureManager->createOrRetrieveTexture( textureName + mAdditionalExtension,
                                                                aliasName,
                                                                GpuPageOutStrategy::Discard,
                                                                textureFlags, internalTextureType,
                                                                resourceGroup, filters );
        }

        itor = json.FindMember( "sampler" );
        if( itor != json.MemberEnd() && itor->value.IsString() )
        {
            map<LwConstString, const HlmsSamplerblock*>::type::const_iterator it =
                    blocks.samplerblocks.find( LwConstString::FromUnsafeCStr(itor->value.GetString()) );
            if( it != blocks.samplerblocks.end() )
            {
                samplerblock = it->second;
                mHlmsManager->addReference( samplerblock );
            }
        }

        if (texture)
        {
            if (!samplerblock)
                samplerblock = mHlmsManager->getSamplerblock(HlmsSamplerblock());
            datablock->_setTexture(textureType, texture, samplerblock);
        }
        else if (samplerblock)
            datablock->_setSamplerblock(textureType, samplerblock);

        itor = json.FindMember( "uv" );
        if( itor != json.MemberEnd() && itor->value.IsUint() )
        {
            unsigned uv = itor->value.GetUint();
            datablock->setTextureUvSource( textureType, static_cast<uint8>( uv ) );
        }
    }
    //-----------------------------------------------------------------------------------
    inline Vector3 HlmsJsonPbs::parseVector3Array( const rapidjson::Value &jsonArray )
    {
        Vector3 retVal( Vector3::ZERO );

        const rapidjson::SizeType arraySize = std::min( 3u, jsonArray.Size() );
        for( rapidjson::SizeType i=0; i<arraySize; ++i )
        {
            if( jsonArray[i].IsNumber() )
                retVal[i] = static_cast<float>( jsonArray[i].GetDouble() );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    inline ColourValue HlmsJsonPbs::parseColourValueArray( const rapidjson::Value &jsonArray,
                                                           const ColourValue &defaultValue )
    {
        ColourValue retVal( defaultValue );

        const rapidjson::SizeType arraySize = std::min( 4u, jsonArray.Size() );
        for( rapidjson::SizeType i=0; i<arraySize; ++i )
        {
            if( jsonArray[i].IsNumber() )
                retVal[i] = static_cast<float>( jsonArray[i].GetDouble() );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonPbs::loadMaterial( const rapidjson::Value &json, const HlmsJson::NamedBlocks &blocks,
                                    HlmsDatablock *datablock, const String &resourceGroup )
    {
        assert( dynamic_cast<HlmsPbsDatablock*>(datablock) );
        HlmsPbsDatablock *pbsDatablock = static_cast<HlmsPbsDatablock*>(datablock);

        rapidjson::Value::ConstMemberIterator itor = json.FindMember("workflow");
        if( itor != json.MemberEnd() && itor->value.IsString() )
            pbsDatablock->setWorkflow( parseWorkflow( itor->value.GetString() ) );

        itor = json.FindMember("brdf");
        if( itor != json.MemberEnd() && itor->value.IsString() )
            pbsDatablock->setBrdf( parseBrdf(itor->value.GetString()) );

        itor = json.FindMember("two_sided");
        if( itor != json.MemberEnd() && itor->value.IsBool() )
        {
            pbsDatablock->setTwoSidedLighting( itor->value.GetBool(), true,
                                               pbsDatablock->getMacroblock(true)->mCullMode );
        }

        itor = json.FindMember( "receive_shadows" );
        if( itor != json.MemberEnd() && itor->value.IsBool() )
            pbsDatablock->setReceiveShadows( itor->value.GetBool() );

        itor = json.FindMember("transparency");
        if( itor != json.MemberEnd() && itor->value.IsObject() )
        {
            const rapidjson::Value &subobj = itor->value;

            float transparencyValue = pbsDatablock->getTransparency();
            HlmsPbsDatablock::TransparencyModes transpMode = pbsDatablock->getTransparencyMode();
            bool useAlphaFromTextures = pbsDatablock->getUseAlphaFromTextures();

            itor = subobj.FindMember( "value" );
            if( itor != subobj.MemberEnd() && itor->value.IsNumber() )
                transparencyValue = static_cast<float>( itor->value.GetDouble() );

            itor = subobj.FindMember( "mode" );
            if( itor != subobj.MemberEnd() && itor->value.IsString() )
                transpMode = parseTransparencyMode( itor->value.GetString() );

            itor = subobj.FindMember( "use_alpha_from_textures" );
            if( itor != subobj.MemberEnd() && itor->value.IsBool() )
                useAlphaFromTextures = itor->value.GetBool();

            const bool changeBlendblock = !json.HasMember( "blendblock" );
            pbsDatablock->setTransparency( transparencyValue, transpMode,
                                           useAlphaFromTextures, changeBlendblock );
        }

        itor = json.FindMember( "clear_coat" );
        if( itor != json.MemberEnd() && itor->value.IsObject() )
        {
            const rapidjson::Value &subobj = itor->value;

            itor = subobj.FindMember( "value" );
            if( itor != subobj.MemberEnd() && itor->value.IsNumber() )
                pbsDatablock->setClearCoat( static_cast<float>( itor->value.GetDouble() ) );

            itor = subobj.FindMember( "roughness" );
            if( itor != subobj.MemberEnd() && itor->value.IsNumber() )
                pbsDatablock->setClearCoatRoughness( static_cast<float>( itor->value.GetDouble() ) );
        }

        itor = json.FindMember("diffuse");
        if( itor != json.MemberEnd() && itor->value.IsObject() )
        {
            const rapidjson::Value &subobj = itor->value;
            loadTexture( subobj, blocks, PBSM_DIFFUSE, pbsDatablock, resourceGroup );

            itor = subobj.FindMember( "value" );
            if( itor != subobj.MemberEnd() && itor->value.IsArray() )
                pbsDatablock->setDiffuse( parseVector3Array( itor->value ) );

            itor = subobj.FindMember( "background" );
            if( itor != subobj.MemberEnd() && itor->value.IsArray() )
                pbsDatablock->setBackgroundDiffuse( parseColourValueArray( itor->value ) );
        }

        itor = json.FindMember("specular");
        if( itor != json.MemberEnd() && itor->value.IsObject() )
        {
            const rapidjson::Value &subobj = itor->value;
            loadTexture( subobj, blocks, PBSM_SPECULAR, pbsDatablock, resourceGroup );

            itor = subobj.FindMember( "value" );
            if( itor != subobj.MemberEnd() && itor->value.IsArray() )
                pbsDatablock->setSpecular( parseVector3Array( itor->value ) );
        }

        itor = json.FindMember("roughness");
        if( itor != json.MemberEnd() && itor->value.IsObject() )
        {
            const rapidjson::Value &subobj = itor->value;
            loadTexture( subobj, blocks, PBSM_ROUGHNESS, pbsDatablock, resourceGroup );

            itor = subobj.FindMember( "value" );
            if( itor != subobj.MemberEnd() && itor->value.IsNumber() )
                pbsDatablock->setRoughness( static_cast<float>( itor->value.GetDouble() ) );
        }

        itor = json.FindMember("fresnel");
        if( itor != json.MemberEnd() && itor->value.IsObject() )
        {
            const rapidjson::Value &subobj = itor->value;
            loadTexture( subobj, blocks, PBSM_SPECULAR, pbsDatablock, resourceGroup );

            bool useIOR = false;
            bool isColoured = false;
            itor = subobj.FindMember( "mode" );
            if( itor != subobj.MemberEnd() && itor->value.IsString() )
                parseFresnelMode( itor->value.GetString(), isColoured, useIOR );

            itor = subobj.FindMember( "value" );
            if( itor != subobj.MemberEnd() && (itor->value.IsArray() || itor->value.IsNumber()) )
            {
                Vector3 value;
                if( itor->value.IsArray() )
                    value = parseVector3Array( itor->value );
                else
                    value = static_cast<Real>( itor->value.GetDouble() );

                if( !useIOR )
                    pbsDatablock->setFresnel( value, isColoured );
                else
                    pbsDatablock->setIndexOfRefraction( value, isColoured );
            }
        }

        //There used to be a typo, so allow the wrong spelling.
        itor = json.FindMember("metalness");
        if( itor == json.MemberEnd() )
            itor = json.FindMember("metallness");

        if( itor != json.MemberEnd() && itor->value.IsObject() )
        {
            const rapidjson::Value &subobj = itor->value;
            loadTexture( subobj, blocks, PBSM_METALLIC, pbsDatablock, resourceGroup );

            itor = subobj.FindMember( "value" );
            if( itor != subobj.MemberEnd() && itor->value.IsNumber() )
                pbsDatablock->setMetalness( static_cast<float>( itor->value.GetDouble() ) );
        }

        itor = json.FindMember("normal");
        if( itor != json.MemberEnd() && itor->value.IsObject() )
        {
            const rapidjson::Value &subobj = itor->value;
            loadTexture( subobj, blocks, PBSM_NORMAL, pbsDatablock, resourceGroup );

            itor = subobj.FindMember( "value" );
            if( itor != subobj.MemberEnd() && itor->value.IsNumber() )
                pbsDatablock->setNormalMapWeight( static_cast<float>( itor->value.GetDouble() ) );
        }

        itor = json.FindMember("detail_weight");
        if( itor != json.MemberEnd() && itor->value.IsObject() )
        {
            const rapidjson::Value &subobj = itor->value;
            loadTexture( subobj, blocks, PBSM_DETAIL_WEIGHT, pbsDatablock, resourceGroup );
        }

        for( int i=0; i<4; ++i )
        {
            const String iAsStr = StringConverter::toString(i);
            String texTypeName = "detail_diffuse" + iAsStr;

            itor = json.FindMember(texTypeName.c_str());
            if( itor != json.MemberEnd() && itor->value.IsObject() )
            {
                const rapidjson::Value &subobj = itor->value;
                loadTexture( subobj, blocks, static_cast<PbsTextureTypes>(PBSM_DETAIL0 + i),
                             pbsDatablock, resourceGroup );

                itor = subobj.FindMember( "value" );
                if( itor != subobj.MemberEnd() && itor->value.IsNumber() )
                    pbsDatablock->setDetailMapWeight( i, static_cast<float>( itor->value.GetDouble() ) );

                itor = subobj.FindMember( "mode" );
                if( itor != subobj.MemberEnd() && itor->value.IsString() )
                    pbsDatablock->setDetailMapBlendMode( i, parseBlendMode( itor->value.GetString() ) );

                Vector4 offsetScale( 0, 0, 1, 1 );

                itor = subobj.FindMember( "offset" );
                if( itor != subobj.MemberEnd() && itor->value.IsArray() )
                    parseOffset( itor->value, offsetScale );

                itor = subobj.FindMember( "scale" );
                if( itor != subobj.MemberEnd() && itor->value.IsArray() )
                    parseScale( itor->value, offsetScale );

                pbsDatablock->setDetailMapOffsetScale( i, offsetScale );
            }

            texTypeName = "detail_normal" + iAsStr;
            itor = json.FindMember(texTypeName.c_str());
            if( itor != json.MemberEnd() && itor->value.IsObject() )
            {
                const rapidjson::Value &subobj = itor->value;
                loadTexture( subobj, blocks, static_cast<PbsTextureTypes>(PBSM_DETAIL0_NM + i),
                             pbsDatablock, resourceGroup );

                itor = subobj.FindMember( "value" );
                if( itor != subobj.MemberEnd() && itor->value.IsNumber() )
                {
                    pbsDatablock->setDetailNormalWeight( i,
                                                         static_cast<float>( itor->value.GetDouble() ) );
                }

                Vector4 offsetScale( 0, 0, 1, 1 );

                itor = subobj.FindMember( "offset" );
                if( itor != subobj.MemberEnd() && itor->value.IsArray() )
                    parseOffset( itor->value, offsetScale );

                itor = subobj.FindMember( "scale" );
                if( itor != subobj.MemberEnd() && itor->value.IsArray() )
                    parseScale( itor->value, offsetScale );

                pbsDatablock->setDetailMapOffsetScale( i, offsetScale );
            }
        }

        itor = json.FindMember("emissive");
        if( itor != json.MemberEnd() && itor->value.IsObject() )
        {
            const rapidjson::Value &subobj = itor->value;
            loadTexture( subobj, blocks, PBSM_EMISSIVE, pbsDatablock, resourceGroup );

            itor = subobj.FindMember( "value" );
            if( itor != subobj.MemberEnd() && itor->value.IsArray() )
                pbsDatablock->setEmissive( parseVector3Array( itor->value ) );
        }

        itor = json.FindMember("reflection");
        if( itor != json.MemberEnd() && itor->value.IsObject() )
        {
            const rapidjson::Value &subobj = itor->value;
            loadTexture( subobj, blocks, PBSM_REFLECTION, pbsDatablock, resourceGroup );
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonPbs::toQuotedStr( HlmsPbsDatablock::Workflows value, String &outString )
    {
        outString += '"';
        outString += c_workflows[value];
        outString += '"';
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonPbs::toQuotedStr( uint32 value, String &outString )
    {
        outString += '"';
        switch( value )
        {
        case PbsBrdf::Default:
            outString += "default";
            break;
        case PbsBrdf::CookTorrance:
            outString += "cook_torrance";
            break;
        case PbsBrdf::BlinnPhong:
            outString += "blinn_phong";
            break;
        case PbsBrdf::DefaultUncorrelated:
            outString += "default_uncorrelated";
            break;
        case PbsBrdf::DefaultSeparateDiffuseFresnel:
            outString += "default_separate_diffuse_fresnel";
            break;
        case PbsBrdf::CookTorranceSeparateDiffuseFresnel:
            outString += "cook_torrance_separate_diffuse_fresnel";
            break;
        case PbsBrdf::BlinnPhongSeparateDiffuseFresnel:
            outString += "blinn_phong_separate_diffuse_fresnel";
            break;
        case PbsBrdf::BlinnPhongLegacyMath:
            outString += "blinn_phong_legacy_math";
            break;
        case PbsBrdf::BlinnPhongFullLegacy:
            outString += "blinn_phong_full_legacy";
            break;
        default:
            outString += "unknown / custom";
            break;
        }
        outString += '"';
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonPbs::toQuotedStr( HlmsPbsDatablock::TransparencyModes value, String &outString )
    {
        outString += '"';
        outString += c_transparencyModes[value];
        outString += '"';
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonPbs::saveFresnel( const HlmsPbsDatablock *datablock, String &outString )
    {
        saveTexture( datablock->getFresnel(), ColourValue::ZERO, "fresnel", PBSM_SPECULAR,
                     true, false, true, true,
                     datablock->getWorkflow() == HlmsPbsDatablock::SpecularAsFresnelWorkflow,
                     datablock, outString );
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonPbs::saveTexture( const char *blockName,
                                   PbsTextureTypes textureType,
                                   const HlmsPbsDatablock *datablock, String &outString,
                                   bool writeTexture )
    {
        saveTexture( Vector3(0.0f), ColourValue::ZERO, blockName, textureType,
                     false, false, false, false, writeTexture, datablock, outString);
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonPbs::saveTexture( float value, const char *blockName,
                                   PbsTextureTypes textureType,
                                   const HlmsPbsDatablock *datablock, String &outString,
                                   bool writeTexture )
    {
        saveTexture( Vector3(value), ColourValue::ZERO, blockName, textureType,
                     true, false, true, false, writeTexture, datablock, outString);
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonPbs::saveTexture(const Vector3 &value, const char *blockName,
                                   PbsTextureTypes textureType,
                                   const HlmsPbsDatablock *datablock, String &outString,
                                   bool writeTexture, const ColourValue &bgColour )
    {
        const bool writeBgDiffuse = textureType == PBSM_DIFFUSE;
        saveTexture( value, bgColour, blockName, textureType,
                     true, writeBgDiffuse, false, false, writeTexture,
                     datablock, outString );
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonPbs::saveTexture( const Vector3 &value, const ColourValue &bgDiffuse,
                                   const char *blockName, PbsTextureTypes textureType,
                                   bool writeValue, bool writeBgDiffuse, bool scalarValue,
                                   bool isFresnel, bool writeTexture,
                                   const HlmsPbsDatablock *datablock, String &outString )
    {
        outString += ",\n\t\t\t\"";
        outString += blockName;
        outString += "\" :\n\t\t\t{\n";

        const size_t currentOffset = outString.size();

        if( isFresnel )
            scalarValue = !datablock->hasSeparateFresnel();

        if( writeValue )
        {
            outString += "\t\t\t\t\"value\" : ";
            if( scalarValue )
                outString += StringConverter::toString( value.x );
            else
            {
                HlmsJson::toStr( value, outString );
            }
        }

        if( writeBgDiffuse )
        {
            outString += ",\n\t\t\t\t\"background\" : ";
            HlmsJson::toStr( bgDiffuse, outString );
        }

        if( isFresnel )
        {
            if( datablock->hasSeparateFresnel() )
                outString += ",\n\t\t\t\t\"mode\" : \"coloured\"";
            else
                outString += ",\n\t\t\t\t\"mode\" : \"coeff\"";
        }

        if( textureType >= PBSM_DETAIL0 && textureType <= PBSM_DETAIL3 )
        {
            PbsBlendModes blendMode = datablock->getDetailMapBlendMode( textureType - PBSM_DETAIL0 );

            if( blendMode != PBSM_BLEND_NORMAL_NON_PREMUL )
            {
                outString += ",\n\t\t\t\t\"mode\" : \"";
                outString += c_pbsBlendModes[blendMode];
                outString += '"';
            }

            const Vector4 &offsetScale =
                    datablock->getDetailMapOffsetScale( textureType - PBSM_DETAIL0 );
            const Vector2 offset( offsetScale.x, offsetScale.y );
            const Vector2 scale( offsetScale.z, offsetScale.w );

            if( offset != Vector2::ZERO )
            {
                outString += ",\n\t\t\t\t\"offset\" : ";
                HlmsJson::toStr( offset, outString );
            }

            if( scale != Vector2::UNIT_SCALE )
            {
                outString += ",\n\t\t\t\t\"scale\" : ";
                HlmsJson::toStr( scale, outString );
            }
        }

        if( writeTexture )
        {
            TextureGpu *texture = datablock->getTexture( textureType );
            if( texture )
            {
                const String *texName = mTextureManager->findResourceNameStr( texture->getName() );
                const String *aliasName = mTextureManager->findAliasNameStr( texture->getName() );
                if( texName && aliasName )
                {
                    String finalAliasName = *aliasName;
                    String finalTexName = *texName;
                    mListener->savingChangeTextureName( finalAliasName, finalTexName );

                    if( finalTexName != finalAliasName )
                    {
                        outString += ",\n\t\t\t\t\"texture\" : [\"";
                        outString += finalAliasName;
                        outString += "\", \"";
                        outString += finalTexName;
                        outString += "\"]";
                    }
                    else
                    {
                        outString += ",\n\t\t\t\t\"texture\" : \"";
                        outString += finalTexName;
                        outString += '"';
                    }
                    
                    if(textureType == PBSM_DIFFUSE && datablock->getUseDiffuseMapAsGrayscale())
                    {
                        outString += ",\n\t\t\t\t\"grayscale\" : true";
                    }

                    if( textureType == PBSM_EMISSIVE && datablock->getUseEmissiveAsLightmap() )
                    {
                        outString += ",\n\t\t\t\t\"lightmap\" : true";
                    }
                }
            }

            const HlmsSamplerblock *samplerblock = datablock->getSamplerblock( textureType );
            if( samplerblock )
            {
                outString += ",\n\t\t\t\t\"sampler\" : ";
                outString += HlmsJson::getName( samplerblock );
            }

            if( textureType < NUM_PBSM_SOURCES && datablock->getTextureUvSource( textureType ) != 0 )
            {
                outString += ",\n\t\t\t\t\"uv\" : ";
                outString += StringConverter::toString( datablock->getTextureUvSource( textureType ) );
            }
        }

        if( !writeValue && outString.size() != currentOffset )
        {
            //Remove an extra comma and newline characters.
            outString.erase( currentOffset, 2 );
        }

        outString += "\n\t\t\t}";
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonPbs::saveMaterial( const HlmsDatablock *datablock, String &outString )
    {
        assert( dynamic_cast<const HlmsPbsDatablock*>(datablock) );
        const HlmsPbsDatablock *pbsDatablock = static_cast<const HlmsPbsDatablock*>(datablock);

        outString += ",\n\t\t\t\"workflow\" : ";
        toQuotedStr( pbsDatablock->getWorkflow(), outString );

        if( pbsDatablock->getBrdf() != PbsBrdf::Default )
        {
            outString += ",\n\t\t\t\"brdf\" : ";
            toQuotedStr( pbsDatablock->getBrdf(), outString );
        }

        if( pbsDatablock->getTwoSidedLighting() )
            outString += ",\n\t\t\t\"two_sided\" : true";

        if( !pbsDatablock->getReceiveShadows() )
            outString += ",\n\t\t\t\"receive_shadows\" : false";

        // The default is getUseAlphaFromTextures == true, so save this value if it's not default
        if( pbsDatablock->getTransparencyMode() != HlmsPbsDatablock::None ||
            !pbsDatablock->getUseAlphaFromTextures() )
        {
            outString += ",\n\t\t\t\"transparency\" :\n\t\t\t{";
            outString += "\n\t\t\t\t\"value\" : ";
            outString += StringConverter::toString( pbsDatablock->getTransparency() );
            outString += ",\n\t\t\t\t\"mode\" : ";
            toQuotedStr( pbsDatablock->getTransparencyMode(), outString );
            outString += ",\n\t\t\t\t\"use_alpha_from_textures\" : ";
            outString += pbsDatablock->getUseAlphaFromTextures() ? "true" : "false";
            outString += "\n\t\t\t}";
        }

        if( pbsDatablock->getClearCoat() != 0.0f )
        {
            outString += ",\n\t\t\t\"clear_coat\" :\n\t\t\t{";
            outString += "\n\t\t\t\t\"value\" : ";
            outString += StringConverter::toString( pbsDatablock->getClearCoat() );
            outString += ",\n\t\t\t\t\"roughness\" : ";
            outString += StringConverter::toString( pbsDatablock->getClearCoatRoughness() );
            outString += "\n\t\t\t}";
        }

        saveTexture( pbsDatablock->getDiffuse(),  "diffuse", PBSM_DIFFUSE,
                     pbsDatablock, outString, true, pbsDatablock->getBackgroundDiffuse() );
        saveTexture( pbsDatablock->getSpecular(), "specular", PBSM_SPECULAR,
                     pbsDatablock, outString,
                     pbsDatablock->getWorkflow() == HlmsPbsDatablock::SpecularWorkflow );
        if( pbsDatablock->getWorkflow() != HlmsPbsDatablock::MetallicWorkflow )
        {
            saveFresnel( pbsDatablock, outString );
        }
        else
        {
            saveTexture( pbsDatablock->getMetalness(), "metalness", PBSM_METALLIC,
                         pbsDatablock, outString );
        }

        if( pbsDatablock->getNormalMapWeight() != 1.0f ||
            pbsDatablock->getTexture( PBSM_NORMAL ) )
        {
            saveTexture( pbsDatablock->getNormalMapWeight(), "normal", PBSM_NORMAL,
                         pbsDatablock, outString );
        }

        saveTexture( pbsDatablock->getRoughness(), "roughness", PBSM_ROUGHNESS,
                     pbsDatablock, outString );

        if( pbsDatablock->getTexture( PBSM_DETAIL_WEIGHT ) )
            saveTexture( "detail_weight", PBSM_DETAIL_WEIGHT, pbsDatablock, outString );

        for( int i=0; i<4; ++i )
        {
            PbsBlendModes blendMode = pbsDatablock->getDetailMapBlendMode( i );
            const Vector4 &offsetScale = pbsDatablock->getDetailMapOffsetScale( i );
            const Vector2 offset( offsetScale.x, offsetScale.y );
            const Vector2 scale( offsetScale.z, offsetScale.w );

            const PbsTextureTypes textureType = static_cast<PbsTextureTypes>(PBSM_DETAIL0 + i);

            if( blendMode != PBSM_BLEND_NORMAL_NON_PREMUL || offset != Vector2::ZERO ||
                scale != Vector2::UNIT_SCALE || pbsDatablock->getDetailMapWeight( i ) != 1.0f ||
                pbsDatablock->getTexture( textureType ) )
            {
                char tmpBuffer[64];
                LwString blockName( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

                blockName.a( "detail_diffuse", i );

                saveTexture( pbsDatablock->getDetailMapWeight( i ), blockName.c_str(),
                             static_cast<PbsTextureTypes>(PBSM_DETAIL0 + i), pbsDatablock,
                             outString );
            }
        }

        for( int i=0; i<4; ++i )
        {
            const PbsTextureTypes textureType = static_cast<PbsTextureTypes>(PBSM_DETAIL0_NM + i);

            if( pbsDatablock->getDetailNormalWeight( i ) != 1.0f ||
                pbsDatablock->getTexture( textureType ) )
            {
                char tmpBuffer[64];
                LwString blockName( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

                blockName.a( "detail_normal", i );
                saveTexture( pbsDatablock->getDetailNormalWeight( i ), blockName.c_str(),
                             static_cast<PbsTextureTypes>(PBSM_DETAIL0_NM + i), pbsDatablock,
                             outString );
            }
        }

        if( pbsDatablock->_hasEmissive() )
        {
            saveTexture( pbsDatablock->getEmissive(), "emissive", PBSM_EMISSIVE,
                         pbsDatablock, outString );
        }

        if( pbsDatablock->getTexture( PBSM_REFLECTION ) )
            saveTexture( "reflection", PBSM_REFLECTION, pbsDatablock, outString );
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonPbs::collectSamplerblocks( const HlmsDatablock *datablock,
                                            set<const HlmsSamplerblock*>::type &outSamplerblocks )
    {
        assert( dynamic_cast<const HlmsPbsDatablock*>(datablock) );
        const HlmsPbsDatablock *pbsDatablock = static_cast<const HlmsPbsDatablock*>(datablock);

        for( int i=0; i<NUM_PBSM_TEXTURE_TYPES; ++i )
        {
            const PbsTextureTypes textureType = static_cast<PbsTextureTypes>( i );
            const HlmsSamplerblock *samplerblock = pbsDatablock->getSamplerblock( textureType );
            if( samplerblock )
                outSamplerblocks.insert( samplerblock );
        }
    }
}

#endif
