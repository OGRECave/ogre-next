/*-------------------------------------------------------------------------
This source file is a part of OGRE-Next
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
THE SOFTWARE
-------------------------------------------------------------------------*/

#include "OgreFont.h"

#include "OgreBitwise.h"
#include "OgreException.h"
#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgreLogManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreRoot.h"
#include "OgreStagingTexture.h"
#include "OgreString.h"
#include "OgreStringConverter.h"
#include "OgreTextureGpuManager.h"

#ifdef OGRE_BUILD_COMPONENT_HLMS_UNLIT
#    include "OgreHlmsUnlitDatablock.h"
#else
#    include "OgreHlmsUnlitMobileDatablock.h"
#endif

#define generic _generic  // keyword for C++/CX
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#undef generic

#include <sstream>

namespace Ogre
{
    //---------------------------------------------------------------------
    Font::CmdType Font::msTypeCmd;
    Font::CmdSource Font::msSourceCmd;
    Font::CmdCharSpacer Font::msCharacterSpacerCmd;
    Font::CmdSize Font::msSizeCmd;
    Font::CmdResolution Font::msResolutionCmd;
    Font::CmdCodePoints Font::msCodePointsCmd;

    //---------------------------------------------------------------------
    Font::Font( ResourceManager *creator, const String &name, ResourceHandle handle, const String &group,
                bool isManual, ManualResourceLoader *loader ) :
        Resource( creator, name, handle, group, isManual, loader ),
        mType( FT_TRUETYPE ),
        mCharacterSpacer( 5 ),
        mTtfSize( 0 ),
        mTtfResolution( 0 ),
        mTtfMaxBearingY( 0 ),
        mHlmsDatablock( 0 ),
        mTexture( 0 ),
        mTextureLoadingInProgress( false ),
        mAntialiasColour( false )
    {
        if( createParamDictionary( "Font" ) )
        {
            ParamDictionary *dict = getParamDictionary();
            dict->addParameter( ParameterDef( "type", "'truetype' or 'image' based font", PT_STRING ),
                                &msTypeCmd );
            dict->addParameter(
                ParameterDef( "source", "Filename of the source of the font.", PT_STRING ),
                &msSourceCmd );
            dict->addParameter(
                ParameterDef( "character_spacer",
                              "Spacing between characters to prevent overlap artifacts.", PT_STRING ),
                &msCharacterSpacerCmd );
            dict->addParameter( ParameterDef( "size", "True type size", PT_REAL ), &msSizeCmd );
            dict->addParameter( ParameterDef( "resolution", "True type resolution", PT_UNSIGNED_INT ),
                                &msResolutionCmd );
            dict->addParameter( ParameterDef( "code_points", "Add a range of code points", PT_STRING ),
                                &msCodePointsCmd );
        }
    }
    //---------------------------------------------------------------------
    Font::~Font()
    {
        // have to call this here rather than in Resource destructor
        // since calling virtual methods in base destructors causes crash
        unload();
    }
    //---------------------------------------------------------------------
    void Font::setType( FontType ftype ) { mType = ftype; }
    //---------------------------------------------------------------------
    FontType Font::getType() const { return mType; }
    //---------------------------------------------------------------------
    void Font::setSource( const String &source ) { mSource = source; }
    //---------------------------------------------------------------------
    void Font::setTrueTypeSize( Real ttfSize ) { mTtfSize = ttfSize; }
    //---------------------------------------------------------------------
    void Font::setCharacterSpacer( uint charSpacer ) { mCharacterSpacer = charSpacer; }
    //---------------------------------------------------------------------
    void Font::setTrueTypeResolution( uint ttfResolution ) { mTtfResolution = ttfResolution; }
    //---------------------------------------------------------------------
    const String &Font::getSource() const { return mSource; }
    //---------------------------------------------------------------------
    uint Font::getCharacterSpacer() const { return mCharacterSpacer; }
    //---------------------------------------------------------------------
    Real Font::getTrueTypeSize() const { return mTtfSize; }
    //---------------------------------------------------------------------
    uint Font::getTrueTypeResolution() const { return mTtfResolution; }
    //---------------------------------------------------------------------
    int Font::getTrueTypeMaxBearingY() const { return mTtfMaxBearingY; }
    //---------------------------------------------------------------------
    const Font::GlyphInfo &Font::getGlyphInfo( CodePoint id ) const
    {
        CodePointMap::const_iterator i = mCodePointMap.find( id );
        if( i == mCodePointMap.end() )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Code point " + StringConverter::toString( id ) + " not found in font " + mName,
                         "Font::getGlyphInfo" );
        }
        return i->second;
    }
    //---------------------------------------------------------------------
    void Font::loadImpl()
    {
        // Create a new material
        bool blendByAlpha = true;
        if( mType == FT_TRUETYPE )
        {
            createTextureFromFont();
            // Always blend by alpha
            blendByAlpha = true;
        }
        else
        {
            // Manually load since we need to load to get alpha
#define TODO
            TODO;
            // mTexture = TextureManager::getSingleton().load(mSource, mGroup, TEX_TYPE_2D, 0);
            // blendByAlpha = mTexture->hasAlpha();
        }

        Hlms *hlmsGui = Root::getSingleton().getHlmsManager()->getHlms( HLMS_UNLIT );

        HlmsMacroblock macroblock;
        HlmsBlendblock blendblock;
        macroblock.mCullMode = CULL_NONE;
        macroblock.mDepthCheck = false;
        macroblock.mDepthWrite = false;
        if( blendByAlpha )
        {
            // Alpha blending
            blendblock.mSourceBlendFactor = SBF_SOURCE_ALPHA;
            blendblock.mSourceBlendFactorAlpha = SBF_SOURCE_ALPHA;
            blendblock.mDestBlendFactor = SBF_ONE_MINUS_SOURCE_ALPHA;
            blendblock.mDestBlendFactorAlpha = SBF_ONE_MINUS_SOURCE_ALPHA;
        }
        else
        {
            // Add
            blendblock.mSourceBlendFactor = SBF_ONE;
            blendblock.mSourceBlendFactorAlpha = SBF_ONE;
            blendblock.mDestBlendFactor = SBF_ONE;
            blendblock.mDestBlendFactorAlpha = SBF_ONE;
        }

        HlmsParamVec paramsVec;
        // paramsVec.push_back( std::pair<IdString, String>( "diffuse_map", "" ) );
        std::sort( paramsVec.begin(), paramsVec.end() );

        String datablockName = "Fonts/" + mName;
        mHlmsDatablock =
            hlmsGui->createDatablock( datablockName, datablockName, macroblock, blendblock, paramsVec );

        assert( dynamic_cast<OverlayUnlitDatablock *>( mHlmsDatablock ) );

        HlmsSamplerblock samplerblock;
        // Clamp to avoid fuzzy edges
        samplerblock.setAddressingMode( TAM_CLAMP );
        // Allow min/mag filter, but no mip
        samplerblock.mMinFilter = FO_LINEAR;
        samplerblock.mMagFilter = FO_LINEAR;
        samplerblock.mMipFilter = FO_NONE;

        OverlayUnlitDatablock *guiDatablock = static_cast<OverlayUnlitDatablock *>( mHlmsDatablock );
#ifdef OGRE_BUILD_COMPONENT_HLMS_UNLIT
        guiDatablock->setTexture( 0, mTexture, &samplerblock );
#else
        guiDatablock->setTexture( 0, mTexture, OverlayUnlitDatablock::UvAtlasParams() );
#endif
        if( mType == FT_TRUETYPE || !blendByAlpha )
        {
            // Source the alpha from the green channel.
            guiDatablock->setTextureSwizzle( 0, HlmsUnlitDatablock::R_MASK, HlmsUnlitDatablock::R_MASK,
                                             HlmsUnlitDatablock::R_MASK, HlmsUnlitDatablock::G_MASK );
        }
    }
    //---------------------------------------------------------------------
    void Font::unloadImpl()
    {
        RenderSystem *renderSystem = Root::getSingleton().getRenderSystem();
        TextureGpuManager *textureManager = renderSystem->getTextureGpuManager();

        mHlmsDatablock->getCreator()->destroyDatablock( mHlmsDatablock->getName() );
        mHlmsDatablock = 0;

        if( mTexture )
        {
            mTexture->removeListener( this );
            textureManager->destroyTexture( mTexture );
            mTexture = 0;
        }
    }
    //---------------------------------------------------------------------
    void Font::createTextureFromFont()
    {
        RenderSystem *renderSystem = Root::getSingleton().getRenderSystem();
        TextureGpuManager *textureManager = renderSystem->getTextureGpuManager();

        // Just create the texture here, and point it at ourselves for when
        // it wants to (re)load for real
        String texName = mName + "/Texture";
        // Create, setting isManual to true and passing self as loader
        // mGroup
        mTexture = textureManager->createTexture( texName, GpuPageOutStrategy::SaveToSystemRam,
                                                  TextureFlags::ManualTexture, TextureTypes::Type2D );
        mTexture->setPixelFormat( PFG_RG8_UNORM );
        mTexture->setTextureType( TextureTypes::Type2D );
        mTexture->setNumMipmaps( 1u );
        mTexture->addListener( this );

        loadTextureFromFont( textureManager );
    }
    //---------------------------------------------------------------------
    void Font::loadTextureFromFont( TextureGpuManager *textureManager )
    {
        // ManualResourceLoader implementation - load the texture
        FT_Library ftLibrary;
        // Init freetype
        if( FT_Init_FreeType( &ftLibrary ) )
        {
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "Could not init FreeType library!",
                         "Font::Font" );
        }

        FT_Face face;

        // Locate ttf file, load it pre-buffered into memory by wrapping the
        // original DataStream in a MemoryDataStream
        DataStreamPtr dataStreamPtr =
            ResourceGroupManager::getSingleton().openResource( mSource, mGroup, true, this );
        MemoryDataStream ttfchunk( dataStreamPtr );

        // Load font
        if( FT_New_Memory_Face( ftLibrary, ttfchunk.getPtr(), (FT_Long)ttfchunk.size(), 0, &face ) )
        {
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "Could not open font face!",
                         "Font::createTextureFromFont" );
        }

        // Convert our point size to freetype 26.6 fixed point format
        FT_F26Dot6 ftSize = (FT_F26Dot6)( mTtfSize * ( 1 << 6 ) );
        if( FT_Set_Char_Size( face, ftSize, 0, mTtfResolution, mTtfResolution ) )
        {
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "Could not set char size!",
                         "Font::createTextureFromFont" );
        }

        // FILE *fo_def = stdout;
        FT_Pos max_height = 0, max_width = 0;

        // Backwards compatibility - if codepoints not supplied, assume 33-166
        if( mCodePointRangeList.empty() )
        {
            mCodePointRangeList.push_back( CodePointRange( 33, 166 ) );
        }

        // Calculate maximum width, height and bearing
        size_t glyphCount = 0;
        for( CodePointRangeList::const_iterator r = mCodePointRangeList.begin();
             r != mCodePointRangeList.end(); ++r )
        {
            const CodePointRange &range = *r;
            for( CodePoint cp = range.first; cp <= range.second; ++cp, ++glyphCount )
            {
                FT_Load_Char( face, cp, FT_LOAD_RENDER );

                if( ( 2 * ( ( (int)face->glyph->bitmap.rows ) << 6 ) -
                      face->glyph->metrics.horiBearingY ) > max_height )
                    max_height = ( 2 * ( ( (int)face->glyph->bitmap.rows ) << 6 ) -
                                   face->glyph->metrics.horiBearingY );
                if( face->glyph->metrics.horiBearingY > mTtfMaxBearingY )
                    mTtfMaxBearingY = static_cast<int>( face->glyph->metrics.horiBearingY );

                if( ( face->glyph->advance.x >> 6 ) + ( face->glyph->metrics.horiBearingX >> 6 ) >
                    max_width )
                    max_width =
                        ( face->glyph->advance.x >> 6 ) + ( face->glyph->metrics.horiBearingX >> 6 );
            }
        }

        // Now work out how big our texture needs to be
        size_t rawSize = ( static_cast<size_t>( max_width ) + mCharacterSpacer ) *
                         ( static_cast<size_t>( max_height >> 6 ) + mCharacterSpacer ) * glyphCount;

        uint32 tex_side = static_cast<uint32>( Math::Sqrt( (Real)rawSize ) );
        // just in case the size might chop a glyph in half, add another glyph width/height
        tex_side += std::max( max_width, ( max_height >> 6 ) );
        // Now round up to nearest power of two
        uint32 roundUpSize = Bitwise::firstPO2From( tex_side );

        // Would we benefit from using a non-square texture (2X width)
        uint32 finalWidth, finalHeight;
        if( ( ( roundUpSize * roundUpSize ) >> 1u ) >= rawSize )
        {
            finalHeight = roundUpSize >> 1u;
        }
        else
        {
            finalHeight = roundUpSize;
        }
        finalWidth = roundUpSize;

        if( mTexture->getResidencyStatus() == GpuResidency::OnStorage )
            mTexture->setResolution( finalWidth, finalHeight );

        Real textureAspect = (Real)finalWidth / (Real)finalHeight;

        const uint32 rowAlignment = 4u;
        const size_t dataSize = PixelFormatGpuUtils::getSizeBytes(
            finalWidth, finalHeight, 1u, 1u, mTexture->getPixelFormat(), rowAlignment );
        const uint32 bytesPerRow = mTexture->_getSysRamCopyBytesPerRow( 0 );
        const size_t bytesPerPixel = 2u;

        LogManager::getSingleton().logMessage( "Font " + mName + " using texture size " +
                                               StringConverter::toString( finalWidth ) + "x" +
                                               StringConverter::toString( finalHeight ) );

        uint8 *imageData =
            reinterpret_cast<uint8 *>( OGRE_MALLOC_SIMD( dataSize, MEMCATEGORY_RESOURCE ) );
        // Reset content (White, transparent)
        for( size_t i = 0; i < dataSize; i += bytesPerPixel )
        {
            imageData[i + 0] = 0xFF;  // luminance
            imageData[i + 1] = 0x00;  // alpha
        }

        size_t l = 0, m = 0;
        CodePointRangeList::const_iterator itor = mCodePointRangeList.begin();
        CodePointRangeList::const_iterator endt = mCodePointRangeList.end();
        while( itor != endt )
        {
            const CodePointRange &range = *itor;
            for( CodePoint cp = range.first; cp <= range.second; ++cp )
            {
                FT_Error ftResult;

                // Load & render glyph
                ftResult = FT_Load_Char( face, cp, FT_LOAD_RENDER );
                if( ftResult )
                {
                    // problem loading this glyph, continue
                    LogManager::getSingleton().logMessage( "Info: cannot load character " +
                                                               StringConverter::toString( cp ) +
                                                               " in font " + mName,
                                                           LML_CRITICAL );
                    continue;
                }

                FT_Pos advance = face->glyph->advance.x >> 6;

                uint8 const *buffer = face->glyph->bitmap.buffer;
                if( !buffer )
                {
                    // Yuck, FT didn't detect this but generated a null pointer!
                    LogManager::getSingleton().logMessage(
                        "Info: Freetype returned null for character " + StringConverter::toString( cp ) +
                        " in font " + mName );
                    continue;
                }

                FT_Pos y_bearing = ( mTtfMaxBearingY >> 6 ) - ( face->glyph->metrics.horiBearingY >> 6 );
                FT_Pos x_bearing = face->glyph->metrics.horiBearingX >> 6;

                for( int j = 0; j < face->glyph->bitmap.rows; ++j )
                {
                    const size_t row = static_cast<size_t>( j ) + m + static_cast<size_t>( y_bearing );
                    uint8 *pDest = &imageData[( row * bytesPerRow ) +
                                              ( l + static_cast<size_t>( x_bearing ) ) * bytesPerPixel];
                    for( int k = 0; k < face->glyph->bitmap.width; k++ )
                    {
                        if( mAntialiasColour )
                        {
                            // Use the same greyscale pixel for all components RGBA
                            *pDest++ = *buffer;
                        }
                        else
                        {
                            // Always white whether 'on' or 'off' pixel, since alpha
                            // will turn off
                            *pDest++ = 0xFF;
                        }
                        // Always use the greyscale value for alpha
                        *pDest++ = *buffer++;
                    }
                }

                this->setGlyphTexCoords(
                    cp,
                    (Real)l / (Real)finalWidth,   // u1
                    (Real)m / (Real)finalHeight,  // v1
                    (Real)( l + static_cast<size_t>( face->glyph->advance.x >> 6 ) ) /
                        (Real)finalWidth,                                                    // u2
                    Real( m + static_cast<size_t>( max_height >> 6 ) ) / (Real)finalHeight,  // v2
                    textureAspect );

                // Advance a column
                l += ( static_cast<uint>( advance ) + mCharacterSpacer );

                // If at end of row
                if( finalWidth - 1 < l + static_cast<size_t>( advance ) )
                {
                    m += static_cast<size_t>( max_height >> 6 ) + mCharacterSpacer;
                    l = 0;
                }
            }

            ++itor;
        }

        if( mTexture->getResidencyStatus() == GpuResidency::OnStorage )
        {
            mTextureLoadingInProgress = true;  // avoid recursion
            mTexture->_transitionTo( GpuResidency::Resident, imageData );
            mTexture->_setNextResidencyStatus( GpuResidency::Resident );
            mTextureLoadingInProgress = false;
        }

        StagingTexture *stagingTexture = textureManager->getStagingTexture(
            finalWidth, finalHeight, 1u, 1u, mTexture->getPixelFormat() );
        stagingTexture->startMapRegion();
        TextureBox texBox =
            stagingTexture->mapRegion( finalWidth, finalHeight, 1u, 1u, mTexture->getPixelFormat() );
        texBox.copyFrom( imageData, finalWidth, finalHeight, bytesPerRow );
        stagingTexture->stopMapRegion();
        stagingTexture->upload( texBox, mTexture, 0, 0, 0, true );
        textureManager->removeStagingTexture( stagingTexture );
        stagingTexture = 0;

        OGRE_FREE_SIMD( imageData, MEMCATEGORY_RESOURCE );
        imageData = 0;

        FT_Done_FreeType( ftLibrary );
    }
    //---------------------------------------------------------------------
    void Font::notifyTextureChanged( TextureGpu *texture, TextureGpuListener::Reason reason,
                                     void *extraData )
    {
        if( reason == TextureGpuListener::GainedResidency && !mTextureLoadingInProgress )
        {
            RenderSystem *renderSystem = Root::getSingleton().getRenderSystem();
            TextureGpuManager *textureManager = renderSystem->getTextureGpuManager();
            loadTextureFromFont( textureManager );
        }
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    String Font::CmdType::doGet( const void *target ) const
    {
        const Font *f = static_cast<const Font *>( target );
        if( f->getType() == FT_TRUETYPE )
        {
            return "truetype";
        }
        else
        {
            return "image";
        }
    }
    void Font::CmdType::doSet( void *target, const String &val )
    {
        Font *f = static_cast<Font *>( target );
        if( val == "truetype" )
        {
            f->setType( FT_TRUETYPE );
        }
        else
        {
            f->setType( FT_IMAGE );
        }
    }
    //-----------------------------------------------------------------------
    String Font::CmdSource::doGet( const void *target ) const
    {
        const Font *f = static_cast<const Font *>( target );
        return f->getSource();
    }
    void Font::CmdSource::doSet( void *target, const String &val )
    {
        Font *f = static_cast<Font *>( target );
        f->setSource( val );
    }
    //-----------------------------------------------------------------------
    String Font::CmdCharSpacer::doGet( const void *target ) const
    {
        const Font *f = static_cast<const Font *>( target );
        char buf[sizeof( uint )];
        sprintf( buf, "%d", f->getCharacterSpacer() );
        return String( buf );
    }
    void Font::CmdCharSpacer::doSet( void *target, const String &val )
    {
        Font *f = static_cast<Font *>( target );
        f->setCharacterSpacer( static_cast<uint>( atoi( val.c_str() ) ) );
    }
    //-----------------------------------------------------------------------
    String Font::CmdSize::doGet( const void *target ) const
    {
        const Font *f = static_cast<const Font *>( target );
        return StringConverter::toString( f->getTrueTypeSize() );
    }
    void Font::CmdSize::doSet( void *target, const String &val )
    {
        Font *f = static_cast<Font *>( target );
        f->setTrueTypeSize( StringConverter::parseReal( val ) );
    }
    //-----------------------------------------------------------------------
    String Font::CmdResolution::doGet( const void *target ) const
    {
        const Font *f = static_cast<const Font *>( target );
        return StringConverter::toString( f->getTrueTypeResolution() );
    }
    void Font::CmdResolution::doSet( void *target, const String &val )
    {
        Font *f = static_cast<Font *>( target );
        f->setTrueTypeResolution( StringConverter::parseUnsignedInt( val ) );
    }
    //-----------------------------------------------------------------------
    String Font::CmdCodePoints::doGet( const void *target ) const
    {
        const Font *f = static_cast<const Font *>( target );
        const CodePointRangeList &rangeList = f->getCodePointRangeList();
        StringStream str;
        for( CodePointRangeList::const_iterator i = rangeList.begin(); i != rangeList.end(); ++i )
        {
            str << i->first << "-" << i->second << " ";
        }
        return str.str();
    }
    void Font::CmdCodePoints::doSet( void *target, const String &val )
    {
        // Format is "code_points start1-end1 start2-end2"
        Font *f = static_cast<Font *>( target );

        StringVector vec = StringUtil::split( val, " \t" );
        for( StringVector::iterator i = vec.begin(); i != vec.end(); ++i )
        {
            String &item = *i;
            StringVector itemVec = StringUtil::split( item, "-" );
            if( itemVec.size() == 2 )
            {
                f->addCodePointRange(
                    CodePointRange( StringConverter::parseUnsignedInt( itemVec[0] ),
                                    StringConverter::parseUnsignedInt( itemVec[1] ) ) );
            }
        }
    }

}  // namespace Ogre
