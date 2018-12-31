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

#include "OgreBuildSettings.h"

#if !OGRE_NO_JSON

#include "Terra/Hlms/OgreHlmsJsonTerra.h"
#include "OgreHlmsManager.h"
#include "OgreTextureGpuManager.h"

#include "OgreLwString.h"
#include "OgreStringConverter.h"

#include "rapidjson/document.h"

namespace Ogre
{
    HlmsJsonTerra::HlmsJsonTerra( HlmsManager *hlmsManager, TextureGpuManager *textureManager ) :
        mHlmsManager( hlmsManager ),
        mTextureManager( textureManager )
    {
    }
    //-----------------------------------------------------------------------------------
    TerraBrdf::TerraBrdf HlmsJsonTerra::parseBrdf( const char *value )
    {
        if( !strcmp( value, "default" ) )
            return TerraBrdf::Default;
        if( !strcmp( value, "cook_torrance" ) )
            return TerraBrdf::CookTorrance;
        if( !strcmp( value, "blinn_phong" ) )
            return TerraBrdf::BlinnPhong;
        if( !strcmp( value, "default_uncorrelated" ) )
            return TerraBrdf::DefaultUncorrelated;
        if( !strcmp( value, "default_separate_diffuse_fresnel" ) )
            return TerraBrdf::DefaultSeparateDiffuseFresnel;
        if( !strcmp( value, "cook_torrance_separate_diffuse_fresnel" ) )
            return TerraBrdf::CookTorranceSeparateDiffuseFresnel;
        if( !strcmp( value, "blinn_phong_separate_diffuse_fresnel" ) )
            return TerraBrdf::BlinnPhongSeparateDiffuseFresnel;

        return TerraBrdf::Default;
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonTerra::parseOffset( const rapidjson::Value &jsonArray, Vector4 &offsetScale )
    {
        const rapidjson::SizeType arraySize = std::min( 2u, jsonArray.Size() );
        for( rapidjson::SizeType i=0; i<arraySize; ++i )
        {
            if( jsonArray[i].IsNumber() )
                offsetScale[i] = static_cast<float>( jsonArray[i].GetDouble() );
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonTerra::parseScale( const rapidjson::Value &jsonArray, Vector4 &offsetScale )
    {
        const rapidjson::SizeType arraySize = std::min( 2u, jsonArray.Size() );
        for( rapidjson::SizeType i=0; i<arraySize; ++i )
        {
            if( jsonArray[i].IsNumber() )
                offsetScale[i+2u] = static_cast<float>( jsonArray[i].GetDouble() );
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonTerra::loadTexture( const rapidjson::Value &json, const char *keyName,
                                     TerraTextureTypes textureType, HlmsTerraDatablock *datablock,
                                     const String &resourceGroup )
    {
        assert( textureType != TERRA_REFLECTION );

        TextureGpu *texture = 0;

        const CommonTextureTypes::CommonTextureTypes texMapTypes[NUM_TERRA_TEXTURE_TYPES] =
        {
            CommonTextureTypes::Diffuse,
            CommonTextureTypes::NonColourData,

            CommonTextureTypes::Diffuse,
            CommonTextureTypes::Diffuse,
            CommonTextureTypes::Diffuse,
            CommonTextureTypes::Diffuse,
            CommonTextureTypes::NormalMap,
            CommonTextureTypes::NormalMap,
            CommonTextureTypes::NormalMap,
            CommonTextureTypes::NormalMap,

            CommonTextureTypes::Monochrome,
            CommonTextureTypes::Monochrome,
            CommonTextureTypes::Monochrome,
            CommonTextureTypes::Monochrome,
            CommonTextureTypes::Monochrome,
            CommonTextureTypes::Monochrome,
            CommonTextureTypes::Monochrome,
            CommonTextureTypes::Monochrome,
            CommonTextureTypes::EnvMap
        };

        rapidjson::Value::ConstMemberIterator itor = json.FindMember( keyName );
        if( itor != json.MemberEnd() && itor->value.IsString() )
        {
            const char *textureName = itor->value.GetString();
            texture = mTextureManager->createOrRetrieveTexture( textureName,
                                                                GpuPageOutStrategy::Discard,
                                                                texMapTypes[textureType],
                                                                resourceGroup );
        }

        HlmsSamplerblock samplerBlockRef;
        if( textureType >= TERRA_DETAIL0 && textureType <= TERRA_DETAIL_METALNESS3 )
        {
            //Detail maps default to wrap mode.
            samplerBlockRef.mU = TAM_WRAP;
            samplerBlockRef.mV = TAM_WRAP;
            samplerBlockRef.mW = TAM_WRAP;
        }
        datablock->setTexture( textureType, texture, &samplerBlockRef );
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonTerra::loadTexture( const rapidjson::Value &json, const HlmsJson::NamedBlocks &blocks,
                                     TerraTextureTypes textureType, HlmsTerraDatablock *datablock,
                                     const String &resourceGroup )
    {
        TextureGpu *texture = 0;
        HlmsSamplerblock const *samplerblock = 0;

        const CommonTextureTypes::CommonTextureTypes texMapTypes[NUM_TERRA_TEXTURE_TYPES] =
        {
            CommonTextureTypes::Diffuse,
            CommonTextureTypes::NonColourData,

            CommonTextureTypes::Diffuse,
            CommonTextureTypes::Diffuse,
            CommonTextureTypes::Diffuse,
            CommonTextureTypes::Diffuse,
            CommonTextureTypes::NormalMap,
            CommonTextureTypes::NormalMap,
            CommonTextureTypes::NormalMap,
            CommonTextureTypes::NormalMap,

            CommonTextureTypes::Monochrome,
            CommonTextureTypes::Monochrome,
            CommonTextureTypes::Monochrome,
            CommonTextureTypes::Monochrome,
            CommonTextureTypes::Monochrome,
            CommonTextureTypes::Monochrome,
            CommonTextureTypes::Monochrome,
            CommonTextureTypes::Monochrome,
            CommonTextureTypes::EnvMap
        };

        rapidjson::Value::ConstMemberIterator itor = json.FindMember( "texture" );
        if( itor != json.MemberEnd() && itor->value.IsString() )
        {
            const char *textureName = itor->value.GetString();
            texture = mTextureManager->createOrRetrieveTexture( textureName,
                                                                GpuPageOutStrategy::Discard,
                                                                texMapTypes[textureType],
                                                                resourceGroup );
        }

        itor = json.FindMember("sampler");
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

        if( texture )
        {
            if( !samplerblock )
                samplerblock = mHlmsManager->getSamplerblock( HlmsSamplerblock() );
            datablock->_setTexture( textureType, texture, samplerblock );
        }
        else if( samplerblock )
            datablock->_setSamplerblock( textureType, samplerblock );
    }
    //-----------------------------------------------------------------------------------
    inline Vector3 HlmsJsonTerra::parseVector3Array( const rapidjson::Value &jsonArray )
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
    void HlmsJsonTerra::loadMaterial( const rapidjson::Value &json, const HlmsJson::NamedBlocks &blocks,
                                      HlmsDatablock *datablock, const String &resourceGroup )
    {
        assert( dynamic_cast<HlmsTerraDatablock*>(datablock) );
        HlmsTerraDatablock *terraDatablock = static_cast<HlmsTerraDatablock*>(datablock);

        rapidjson::Value::ConstMemberIterator itor = json.FindMember("brdf");
        if (itor != json.MemberEnd() && itor->value.IsString())
            terraDatablock->setBrdf(parseBrdf(itor->value.GetString()));

        itor = json.FindMember("diffuse");
        if( itor != json.MemberEnd() && itor->value.IsObject() )
        {
            const rapidjson::Value &subobj = itor->value;
            loadTexture( subobj, blocks, TERRA_DIFFUSE, terraDatablock, resourceGroup );

            itor = subobj.FindMember( "value" );
            if( itor != subobj.MemberEnd() && itor->value.IsArray() )
                terraDatablock->setDiffuse( parseVector3Array( itor->value ) );
        }

        itor = json.FindMember("detail_weight");
        if( itor != json.MemberEnd() && itor->value.IsObject() )
        {
            const rapidjson::Value &subobj = itor->value;
            loadTexture( subobj, blocks, TERRA_DETAIL_WEIGHT, terraDatablock, resourceGroup );
        }

        for( int i=0; i<4; ++i )
        {
            const String iAsStr = StringConverter::toString(i);
            String texTypeName = "detail" + iAsStr;

            itor = json.FindMember(texTypeName.c_str());
            if( itor != json.MemberEnd() && itor->value.IsObject() )
            {
                const rapidjson::Value &subobj = itor->value;
                loadTexture( subobj, blocks, static_cast<TerraTextureTypes>(TERRA_DETAIL0 + i),
                             terraDatablock, resourceGroup );

                itor = subobj.FindMember( "roughness" );
                if( itor != subobj.MemberEnd() && itor->value.IsNumber() )
                    terraDatablock->setRoughness( i, static_cast<float>( itor->value.GetDouble() ) );

                itor = subobj.FindMember( "metalness" );
                if( itor != subobj.MemberEnd() && itor->value.IsNumber() )
                    terraDatablock->setMetalness( i, static_cast<float>( itor->value.GetDouble() ) );

                Vector4 offsetScale( 0, 0, 1, 1 );

                itor = subobj.FindMember( "offset" );
                if( itor != subobj.MemberEnd() && itor->value.IsArray() )
                    parseOffset( itor->value, offsetScale );

                itor = subobj.FindMember( "scale" );
                if( itor != subobj.MemberEnd() && itor->value.IsArray() )
                    parseScale( itor->value, offsetScale );

                terraDatablock->setDetailMapOffsetScale( i, offsetScale );

                loadTexture( subobj, "diffuse_map",
                             static_cast<TerraTextureTypes>( TERRA_DETAIL0 + i ),
                             terraDatablock, resourceGroup );
                loadTexture( subobj, "normal_map",
                             static_cast<TerraTextureTypes>( TERRA_DETAIL0_NM + i ),
                             terraDatablock, resourceGroup );
                loadTexture( subobj, "roughness_map",
                             static_cast<TerraTextureTypes>( TERRA_DETAIL_ROUGHNESS0 + i ),
                             terraDatablock, resourceGroup );
                loadTexture( subobj, "metalness_map",
                             static_cast<TerraTextureTypes>( TERRA_DETAIL_METALNESS0 + i ),
                             terraDatablock, resourceGroup );

//                itor = subobjec.FindMember("sampler");
//                if( itor != subobjec.MemberEnd() && itor->value.IsString() )
//                {
//                    map<LwConstString, const HlmsSamplerblock*>::type::const_iterator it =
//                            blocks.samplerblocks.find(
//                                LwConstString::FromUnsafeCStr( itor->value.GetString()) );
//                    if( it != blocks.samplerblocks.end() )
//                    {
//                        textures[TERRA_DETAIL0 + i].samplerblock = it->second;
//                        textures[TERRA_DETAIL0_NM + i].samplerblock = it->second;
//                        textures[TERRA_DETAIL_ROUGHNESS0 + i].samplerblock = it->second;
//                        textures[TERRA_DETAIL_METALNESS0 + i].samplerblock = it->second;
//                        for( int i=0; i<4; ++i )
//                            mHlmsManager->addReference( textures[TERRA_DETAIL0 + i].samplerblock );
//                    }
//                }
            }
        }

        itor = json.FindMember("reflection");
        if( itor != json.MemberEnd() && itor->value.IsObject() )
        {
            const rapidjson::Value &subobj = itor->value;
            loadTexture( subobj, blocks, TERRA_REFLECTION, terraDatablock, resourceGroup );
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonTerra::saveTexture( const char *blockName,
                                   TerraTextureTypes textureType,
                                   const HlmsTerraDatablock *datablock, String &outString,
                                   bool writeTexture )
    {
        saveTexture( Vector3( 0.0f ), blockName, textureType,
                     false, writeTexture, datablock, outString );
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonTerra::saveTexture( const Vector3 &value, const char *blockName,
                                   TerraTextureTypes textureType,
                                   const HlmsTerraDatablock *datablock, String &outString,
                                   bool writeTexture )
    {
        saveTexture( value, blockName, textureType,
                     true, writeTexture, datablock, outString );
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonTerra::saveTexture( const Vector3 &value, const char *blockName,
                                   TerraTextureTypes textureType,
                                   bool writeValue, bool writeTexture,
                                   const HlmsTerraDatablock *datablock, String &outString )
    {
        outString += ",\n\t\t\t\"";
        outString += blockName;
        outString += "\" :\n\t\t\t{\n";

        const size_t currentOffset = outString.size();

        if( writeValue )
        {
            outString += "\t\t\t\t\"value\" : ";
            HlmsJson::toStr( value, outString );
        }

        if( textureType >= TERRA_DETAIL0 && textureType <= TERRA_DETAIL3_NM )
        {
            const Vector4 &offsetScale =
                    datablock->getDetailMapOffsetScale( textureType - TERRA_DETAIL0 );
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

        /*if( writeTexture )
        {
            HlmsTextureManager::TextureLocation texLocation;
            texLocation.texture = datablock->getTexture( textureType );
            if( !texLocation.texture.isNull() )
            {
                texLocation.xIdx = datablock->_getTextureIdx( textureType );
                texLocation.yIdx = 0;
                texLocation.divisor = 1;

                const String *texName = mHlmsManager->getTextureManager()->findAliasName( texLocation );

                if( texName )
                {
                    outString += ",\n\t\t\t\t\"texture\" : \"";
                    outString += *texName;
                    outString += '"';
                }
            }

            const HlmsSamplerblock *samplerblock = datablock->getSamplerblock( textureType );
            if( samplerblock )
            {
                outString += ",\n\t\t\t\t\"sampler\" : ";
                outString += HlmsJson::getName( samplerblock );
            }
        }*/

        if( !writeValue && outString.size() != currentOffset )
        {
            //Remove an extra comma and newline characters.
            outString.erase( currentOffset, 2 );
        }

        outString += "\n\t\t\t}";
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonTerra::saveMaterial( const HlmsDatablock *datablock, String &outString )
    {
//        assert( dynamic_cast<const HlmsTerraDatablock*>(datablock) );
//        const HlmsTerraDatablock *terraDatablock = static_cast<const HlmsTerraDatablock*>(datablock);

////        outString += ",\n\t\t\t\"workflow\" : ";
////        toQuotedStr( terraDatablock->getWorkflow(), outString );

//        saveTexture( terraDatablock->getDiffuse(), "diffuse", TERRA_DIFFUSE,
//                     terraDatablock, outString );

//        if( !terraDatablock->getTexture( TERRA_DETAIL_WEIGHT ).isNull() )
//            saveTexture( "detail_weight", TERRA_DETAIL_WEIGHT, terraDatablock, outString );

//        for( int i=0; i<4; ++i )
//        {
//            const Vector4 &offsetScale = terraDatablock->getDetailMapOffsetScale( i );
//            const Vector2 offset( offsetScale.x, offsetScale.y );
//            const Vector2 scale( offsetScale.z, offsetScale.w );

//            const TerraTextureTypes textureType = static_cast<TerraTextureTypes>(TERRA_DETAIL0 + i);

//            if( offset != Vector2::ZERO ||
//                scale != Vector2::UNIT_SCALE || terraDatablock->getDetailMapWeight( i ) != 1.0f ||
//                !terraDatablock->getTexture( textureType ).isNull() )
//            {
//                char tmpBuffer[64];
//                LwString blockName( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

//                blockName.a( "detail_diffuse", i );

//                saveTexture( terraDatablock->getDetailMapWeight( i ), blockName.c_str(),
//                             static_cast<TerraTextureTypes>(TERRA_DETAIL0 + i), terraDatablock,
//                             outString );
//            }
//        }

//        for( int i=0; i<4; ++i )
//        {
//            const Vector4 &offsetScale = terraDatablock->getDetailMapOffsetScale( i + 4 );
//            const Vector2 offset( offsetScale.x, offsetScale.y );
//            const Vector2 scale( offsetScale.z, offsetScale.w );

//            const TerraTextureTypes textureType = static_cast<TerraTextureTypes>(TERRA_DETAIL0_NM + i);

//            if( offset != Vector2::ZERO || scale != Vector2::UNIT_SCALE ||
//                terraDatablock->getDetailNormalWeight( i ) != 1.0f ||
//                !terraDatablock->getTexture( textureType ).isNull() )
//            {
//                char tmpBuffer[64];
//                LwString blockName( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

//                blockName.a( "detail_normal", i );
//                saveTexture( terraDatablock->getDetailNormalWeight( i ), blockName.c_str(),
//                             static_cast<TerraTextureTypes>(TERRA_DETAIL0_NM + i), terraDatablock,
//                             outString );
//            }
//        }

//        if( !terraDatablock->getTexture( TERRA_REFLECTION ).isNull() )
//            saveTexture( "reflection", TERRA_REFLECTION, terraDatablock, outString );
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonTerra::collectSamplerblocks( const HlmsDatablock *datablock,
                                            set<const HlmsSamplerblock*>::type &outSamplerblocks )
    {
        assert( dynamic_cast<const HlmsTerraDatablock*>(datablock) );
        const HlmsTerraDatablock *terraDatablock = static_cast<const HlmsTerraDatablock*>(datablock);

        for( int i=0; i<NUM_TERRA_TEXTURE_TYPES; ++i )
        {
            const TerraTextureTypes textureType = static_cast<TerraTextureTypes>( i );
            const HlmsSamplerblock *samplerblock = terraDatablock->getSamplerblock( textureType );
            if( samplerblock )
                outSamplerblocks.insert( samplerblock );
        }
    }
}

#endif
