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
#include "OgreStableHeaders.h"

#include "OgreGpuProgram.h"

#include "OgreGpuProgramManager.h"
#include "OgreLogManager.h"
#include "OgreRenderSystem.h"
#include "OgreRenderSystemCapabilities.h"
#include "OgreRoot.h"
#include "OgreRootLayout.h"
#include "OgreStringConverter.h"

#include <sstream>

namespace Ogre
{
    //-----------------------------------------------------------------------------
    GpuProgram::CmdType GpuProgram::msTypeCmd;
    GpuProgram::CmdSyntax GpuProgram::msSyntaxCmd;
    GpuProgram::CmdBuildParamsFromRefl GpuProgram::msBuildParamsFromReflCmd;
    GpuProgram::CmdClipDistance GpuProgram::msClipDistanceCmd;
    GpuProgram::CmdSkeletal GpuProgram::msSkeletalCmd;
    GpuProgram::CmdMorph GpuProgram::msMorphCmd;
    GpuProgram::CmdPose GpuProgram::msPoseCmd;
    GpuProgram::CmdVTF GpuProgram::msVTFCmd;
    GpuProgram::CmdVPRTI GpuProgram::msVPRTICmd;
    GpuProgram::CmdManualNamedConstsFile GpuProgram::msManNamedConstsFileCmd;
    GpuProgram::CmdAdjacency GpuProgram::msAdjacencyCmd;
    GpuProgram::CmdComputeGroupDims GpuProgram::msComputeGroupDimsCmd;
    GpuProgram::CmdRootLayout GpuProgram::msRootLayout;
    GpuProgram::CmdUsesArrayBindings GpuProgram::msUsesArrayBindings;

    //-----------------------------------------------------------------------------
    GpuProgram::GpuProgram( ResourceManager *creator, const String &name, ResourceHandle handle,
                            const String &group, bool isManual, ManualResourceLoader *loader ) :
        Resource( creator, name, handle, group, isManual, loader ),
        mType( GPT_VERTEX_PROGRAM ),
        mLoadFromFile( true ),
        mBuildParametersFromReflection( true ),
        mNumClipDistances( 0u ),
        mSkeletalAnimation( false ),
        mMorphAnimation( false ),
        mPoseAnimation( 0 ),
        mVertexTextureFetch( false ),
        mVpAndRtArrayIndexFromAnyShader( false ),
        mNeedsAdjacencyInfo( false ),
        mCompileError( false ),
        mLoadedManualNamedConstants( false )
    {
        createParameterMappingStructures();
    }
    //-----------------------------------------------------------------------------
    void GpuProgram::setType( GpuProgramType t ) { mType = t; }
    //-----------------------------------------------------------------------------
    void GpuProgram::setRootLayout( GpuProgramType t, const RootLayout &rootLayout )
    {
        setType( t );

        const bool bIsCompute = mType == GPT_COMPUTE_PROGRAM;
        if( bIsCompute != rootLayout.mCompute )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "RootLayout::mCompute disagrees with GpuProgramType",
                         "GpuProgram::setRootLayout" );
        }

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_LOW
        try
        {
            // In OGRE_DEBUG_NONE we assume programmatically generated
            // root layouts are well formed to speed up compilation
            rootLayout.validate( mName );
        }
        catch( Exception & )
        {
            String dumpStr;
            dumpStr = "Error in " + mName + " with its Root Layout:\n";
            rootLayout.dump( dumpStr );
            LogManager::getSingleton().logMessage( dumpStr, LML_CRITICAL );
            throw;
        }
#endif
    }
    //-----------------------------------------------------------------------------
    void GpuProgram::unsetRootLayout() {}
    //-----------------------------------------------------------------------------
    void GpuProgram::setAutoReflectArrayBindingsInRootLayout( bool /*bReflectArrayRootLayouts*/ ) {}
    //-----------------------------------------------------------------------------
    void GpuProgram::setPrefabRootLayout( const PrefabRootLayout::PrefabRootLayout &prefab )
    {
        if( prefab == PrefabRootLayout::None )
        {
            unsetRootLayout();
            return;
        }

        RootLayout rootLayout;

        DescBindingRange *descBindingRange = rootLayout.mDescBindingRanges[0];

        uint16 maxTextures = 4u;
        if( prefab == PrefabRootLayout::High )
            maxTextures = 8u;
        if( prefab == PrefabRootLayout::Max )
            maxTextures = 32u;

        descBindingRange[DescBindingTypes::Texture].start = 0u;
        descBindingRange[DescBindingTypes::Texture].end = maxTextures;
        descBindingRange[DescBindingTypes::Sampler].start = 0u;
        descBindingRange[DescBindingTypes::Sampler].end = maxTextures;

        if( prefab == PrefabRootLayout::Standard || prefab == PrefabRootLayout::High )
        {
            rootLayout.mParamsBuffStages = ( 1u << VertexShader ) | ( 1u << PixelShader );
            descBindingRange[DescBindingTypes::ParamBuffer].start = 0u;
            descBindingRange[DescBindingTypes::ParamBuffer].end = 2u;
        }
        else if( prefab == PrefabRootLayout::Max )
        {
            for( size_t i = 0u; i < NumShaderTypes; ++i )
                rootLayout.mParamsBuffStages |= 1u << i;
            descBindingRange[DescBindingTypes::ParamBuffer].start = 0u;
            descBindingRange[DescBindingTypes::ParamBuffer].end = NumShaderTypes;
        }

        setRootLayout( mType, rootLayout );
    }
    //-----------------------------------------------------------------------------
    void GpuProgram::setReplaceVersionMacro( bool bReplace ) {}
    //-----------------------------------------------------------------------------
    void GpuProgram::setSyntaxCode( const String &syntax ) { mSyntaxCode = syntax; }
    //-----------------------------------------------------------------------------
    void GpuProgram::setSourceFile( const String &filename )
    {
        mFilename = filename;
        mSource.clear();
        mLoadFromFile = true;
        mCompileError = false;
    }
    //-----------------------------------------------------------------------------
    void GpuProgram::setSource( const String &source )
    {
        mSource = source;
        mFilename.clear();
        mLoadFromFile = false;
        mCompileError = false;
    }
    //-----------------------------------------------------------------------------
    void GpuProgram::setSource( const String &source, const String &debugFilename )
    {
        mSource = source;
        mFilename = debugFilename;
        mLoadFromFile = false;
        mCompileError = false;
    }
    size_t GpuProgram::calculateSize() const
    {
        size_t memSize = 0;
        memSize += sizeof( bool ) * 7;
        memSize += mManualNamedConstantsFile.size() * sizeof( char );
        memSize += mFilename.size() * sizeof( char );
        memSize += mSource.size() * sizeof( char );
        memSize += mSyntaxCode.size() * sizeof( char );
        memSize += sizeof( GpuProgramType );
        memSize += sizeof( ushort );

        size_t paramsSize = 0;
        if( mDefaultParams )
            paramsSize += mDefaultParams->calculateSize();
        if( mFloatLogicalToPhysical )
            paramsSize += mFloatLogicalToPhysical->bufferSize;
        if( mDoubleLogicalToPhysical )
            paramsSize += mDoubleLogicalToPhysical->bufferSize;
        if( mIntLogicalToPhysical )
            paramsSize += mIntLogicalToPhysical->bufferSize;
        if( mUIntLogicalToPhysical )
            paramsSize += mUIntLogicalToPhysical->bufferSize;
        if( mConstantDefs )
            paramsSize += mConstantDefs->calculateSize();

        return memSize + paramsSize;
    }
    //-----------------------------------------------------------------------------
    void GpuProgram::loadImpl()
    {
        if( mLoadFromFile )
        {
            // find & load source code
            DataStreamPtr stream =
                ResourceGroupManager::getSingleton().openResource( mFilename, mGroup, true, this );
            mSource = stream->getAsString();
        }

        // Call polymorphic load
        try
        {
            loadFromSource();

            if( mDefaultParams )
            {
                // Keep a reference to old ones to copy
                GpuProgramParametersSharedPtr savedParams = mDefaultParams;
                // reset params to stop them being referenced in the next create
                mDefaultParams.reset();

                // Create new params
                mDefaultParams = createParameters();

                // Copy old (matching) values across
                // Don't use copyConstantsFrom since program may be different
                mDefaultParams->copyMatchingNamedConstantsFrom( *savedParams.get() );
            }
        }
        catch( const Exception & )
        {
            // will already have been logged
            LogManager::getSingleton().stream() << "Gpu program " << mName << " encountered an error "
                                                << "during loading and is thus not supported.";

            mCompileError = true;
        }
    }
    //-----------------------------------------------------------------------------
    bool GpuProgram::isRequiredCapabilitiesSupported() const
    {
        const RenderSystemCapabilities *caps = Root::getSingleton().getRenderSystem()->getCapabilities();

        // If skeletal animation is being done, we need support for UBYTE4
        if( isSkeletalAnimationIncluded() && !caps->hasCapability( RSC_VERTEX_FORMAT_UBYTE4 ) )
        {
            return false;
        }

        // Vertex texture fetch required?
        if( isVertexTextureFetchRequired() && !caps->hasCapability( RSC_VERTEX_TEXTURE_FETCH ) )
        {
            return false;
        }

        // Can we choose viewport and render target in any shader or only geometry one?
        if( isVpAndRtArrayIndexFromAnyShaderRequired() &&
            !caps->hasCapability( RSC_VP_AND_RT_ARRAY_INDEX_FROM_ANY_SHADER ) )
        {
            return false;
        }

        return true;
    }
    //-----------------------------------------------------------------------------
    bool GpuProgram::isSupported() const
    {
        if( mCompileError || !isRequiredCapabilitiesSupported() )
            return false;

        return GpuProgramManager::getSingleton().isSyntaxSupported( mSyntaxCode );
    }
    //---------------------------------------------------------------------
    void GpuProgram::createParameterMappingStructures( bool recreateIfExists ) const
    {
        createLogicalParameterMappingStructures( recreateIfExists );
        createNamedParameterMappingStructures( recreateIfExists );
    }
    //---------------------------------------------------------------------
    void GpuProgram::createLogicalParameterMappingStructures( bool recreateIfExists ) const
    {
        // TODO: OpenGL doesn't use this AT ALL.
        if( recreateIfExists || !mFloatLogicalToPhysical )
            mFloatLogicalToPhysical = GpuLogicalBufferStructPtr( OGRE_NEW GpuLogicalBufferStruct() );
        if( recreateIfExists || !mIntLogicalToPhysical )
            mIntLogicalToPhysical = GpuLogicalBufferStructPtr( OGRE_NEW GpuLogicalBufferStruct() );
        if( recreateIfExists || !mUIntLogicalToPhysical )
            mUIntLogicalToPhysical = GpuLogicalBufferStructPtr( OGRE_NEW GpuLogicalBufferStruct() );
    }
    //---------------------------------------------------------------------
    void GpuProgram::createNamedParameterMappingStructures( bool recreateIfExists ) const
    {
        if( recreateIfExists || !mConstantDefs )
            mConstantDefs = GpuNamedConstantsPtr( OGRE_NEW GpuNamedConstants() );
    }
    //---------------------------------------------------------------------
    void GpuProgram::setManualNamedConstantsFile( const String &paramDefFile )
    {
        mManualNamedConstantsFile = paramDefFile;
        mLoadedManualNamedConstants = false;
    }
    //---------------------------------------------------------------------
    void GpuProgram::setManualNamedConstants( const GpuNamedConstants &namedConstants )
    {
        createParameterMappingStructures();
        *mConstantDefs.get() = namedConstants;

        mFloatLogicalToPhysical->bufferSize = mConstantDefs->floatBufferSize;
        mIntLogicalToPhysical->bufferSize = mConstantDefs->intBufferSize;
        mUIntLogicalToPhysical->bufferSize = mConstantDefs->uintBufferSize;
        mFloatLogicalToPhysical->map.clear();
        mIntLogicalToPhysical->map.clear();
        mUIntLogicalToPhysical->map.clear();
        // need to set up logical mappings too for some rendersystems
        for( GpuConstantDefinitionMap::const_iterator i = mConstantDefs->map.begin();
             i != mConstantDefs->map.end(); ++i )
        {
            const String &name = i->first;
            const GpuConstantDefinition &def = i->second;
            // only consider non-array entries
            if( name.find( "[" ) == String::npos )
            {
                GpuLogicalIndexUseMap::value_type val(
                    def.logicalIndex,
                    GpuLogicalIndexUse( def.physicalIndex, def.arraySize * def.elementSize,
                                        def.variability ) );
                if( def.isFloat() )
                {
                    mFloatLogicalToPhysical->map.insert( val );
                }
                else if( def.isUnsignedInt() )
                {
                    mUIntLogicalToPhysical->map.insert( val );
                }
                else
                {
                    mIntLogicalToPhysical->map.insert( val );
                }
            }
        }
    }
    //-----------------------------------------------------------------------------
    GpuProgramParametersSharedPtr GpuProgram::createParameters()
    {
        // Default implementation simply returns standard parameters.
        GpuProgramParametersSharedPtr ret = GpuProgramManager::getSingleton().createParameters();

        // optionally load manually supplied named constants
        if( !mManualNamedConstantsFile.empty() && !mLoadedManualNamedConstants )
        {
            try
            {
                GpuNamedConstants namedConstants;
                DataStreamPtr stream = ResourceGroupManager::getSingleton().openResource(
                    mManualNamedConstantsFile, mGroup, true, this );
                namedConstants.load( stream );
                setManualNamedConstants( namedConstants );
            }
            catch( const Exception &e )
            {
                LogManager::getSingleton().stream()
                    << "Unable to load manual named constants for GpuProgram " << mName << ": "
                    << e.getDescription();
            }
            mLoadedManualNamedConstants = true;
        }

        // set up named parameters, if any
        if( mConstantDefs && !mConstantDefs->map.empty() )
        {
            ret->_setNamedConstants( mConstantDefs );
        }
        // link shared logical / physical map for low-level use
        ret->_setLogicalIndexes( mFloatLogicalToPhysical, mDoubleLogicalToPhysical,
                                 mIntLogicalToPhysical, mUIntLogicalToPhysical, mBoolLogicalToPhysical );

        // Copy in default parameters if present
        if( mDefaultParams )
            ret->copyConstantsFrom( *( mDefaultParams.get() ) );

        return ret;
    }
    //-----------------------------------------------------------------------------
    void GpuProgram::setNumClipDistances( const uint8 numClipDistances )
    {
        if( mType != GPT_VERTEX_PROGRAM )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Only vertex shader supports this call",
                         "GpuProgram::setNumClipDistances" );
        }
        mNumClipDistances = numClipDistances;
    }
    //-----------------------------------------------------------------------------
    GpuProgramParametersSharedPtr GpuProgram::getDefaultParameters()
    {
        if( !mDefaultParams )
        {
            mDefaultParams = createParameters();
        }
        return mDefaultParams;
    }
    //-----------------------------------------------------------------------------
    void GpuProgram::setupBaseParamDictionary()
    {
        ParamDictionary *dict = getParamDictionary();

        dict->addParameter( ParameterDef( "type",
                                          "'vertex_program', 'geometry_program', 'fragment_program', "
                                          "'hull_program', 'domain_program', 'compute_program'",
                                          PT_STRING ),
                            &msTypeCmd );
        dict->addParameter( ParameterDef( "syntax", "Syntax code, e.g. vs_1_1", PT_STRING ),
                            &msSyntaxCmd );
        dict->addParameter( ParameterDef( "build_parameters_from_reflection",
                                          "Whether to parse the shader to build parameters for auto "
                                          "params and such (optimization when disabled)",
                                          PT_BOOL ),
                            &msBuildParamsFromReflCmd );
        dict->addParameter(
            ParameterDef( "num_clip_distances", "Number of clip distances this vertex shader uses",
                          PT_UNSIGNED_INT ),
            &msClipDistanceCmd );
        dict->addParameter(
            ParameterDef( "includes_skeletal_animation",
                          "Whether this vertex program includes skeletal animation", PT_BOOL ),
            &msSkeletalCmd );
        dict->addParameter(
            ParameterDef( "includes_morph_animation",
                          "Whether this vertex program includes morph animation", PT_BOOL ),
            &msMorphCmd );
        dict->addParameter(
            ParameterDef( "includes_pose_animation",
                          "The number of poses this vertex program supports for pose animation",
                          PT_INT ),
            &msPoseCmd );
        dict->addParameter(
            ParameterDef( "uses_vertex_texture_fetch",
                          "Whether this vertex program requires vertex texture fetch support.",
                          PT_BOOL ),
            &msVTFCmd );
        dict->addParameter( ParameterDef( "sets_vp_or_rt_array_index",
                                          "Whether this program requires support for choosing viewport "
                                          "or render target index from any shader.",
                                          PT_BOOL ),
                            &msVPRTICmd );
        dict->addParameter(
            ParameterDef( "manual_named_constants",
                          "File containing named parameter mappings for low-level programs.", PT_BOOL ),
            &msManNamedConstsFileCmd );
        dict->addParameter( ParameterDef( "uses_adjacency_information",
                                          "Whether this geometry program requires adjacency information "
                                          "from the input primitives.",
                                          PT_BOOL ),
                            &msAdjacencyCmd );
        dict->addParameter(
            ParameterDef( "compute_group_dimensions",
                          "The number of process groups created by this compute program.", PT_VECTOR3 ),
            &msComputeGroupDimsCmd );
        dict->addParameter(
            ParameterDef( "root_layout",
                          "Accepted values are standard, high, max & none. See PrefabRootLayout",
                          PT_STRING ),
            &msRootLayout );
        dict->addParameter( ParameterDef( "uses_array_bindings",
                                          "If your shader uses arrays of bindings (e.g. uniform "
                                          "texture2D myTex[123]) then set this to true for Vulkan",
                                          PT_STRING ),
                            &msUsesArrayBindings );
    }

    //-----------------------------------------------------------------------
    const String &GpuProgram::getLanguage() const
    {
        static const String language = "asm";

        return language;
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    String GpuProgram::CmdType::doGet( const void *target ) const
    {
        const GpuProgram *t = static_cast<const GpuProgram *>( target );
        if( t->getType() == GPT_VERTEX_PROGRAM )
        {
            return "vertex_program";
        }
        else if( t->getType() == GPT_GEOMETRY_PROGRAM )
        {
            return "geometry_program";
        }
        else if( t->getType() == GPT_DOMAIN_PROGRAM )
        {
            return "domain_program";
        }
        else if( t->getType() == GPT_HULL_PROGRAM )
        {
            return "hull_program";
        }
        else if( t->getType() == GPT_COMPUTE_PROGRAM )
        {
            return "compute_program";
        }
        else
        {
            return "fragment_program";
        }
    }
    void GpuProgram::CmdType::doSet( void *target, const String &val )
    {
        GpuProgram *t = static_cast<GpuProgram *>( target );
        if( val == "vertex_program" )
        {
            t->setType( GPT_VERTEX_PROGRAM );
        }
        else if( val == "geometry_program" )
        {
            t->setType( GPT_GEOMETRY_PROGRAM );
        }
        else if( val == "domain_program" )
        {
            t->setType( GPT_DOMAIN_PROGRAM );
        }
        else if( val == "hull_program" )
        {
            t->setType( GPT_HULL_PROGRAM );
        }
        else if( val == "compute_program" )
        {
            t->setType( GPT_COMPUTE_PROGRAM );
        }
        else
        {
            t->setType( GPT_FRAGMENT_PROGRAM );
        }
    }
    //-----------------------------------------------------------------------
    String GpuProgram::CmdSyntax::doGet( const void *target ) const
    {
        const GpuProgram *t = static_cast<const GpuProgram *>( target );
        return t->getSyntaxCode();
    }
    void GpuProgram::CmdSyntax::doSet( void *target, const String &val )
    {
        GpuProgram *t = static_cast<GpuProgram *>( target );
        t->setSyntaxCode( val );
    }
    //-----------------------------------------------------------------------
    String GpuProgram::CmdBuildParamsFromRefl::doGet( const void *target ) const
    {
        const GpuProgram *t = static_cast<const GpuProgram *>( target );
        return StringConverter::toString( t->getBuildParametersFromReflection() );
    }
    void GpuProgram::CmdBuildParamsFromRefl::doSet( void *target, const String &val )
    {
        GpuProgram *t = static_cast<GpuProgram *>( target );
        t->setBuildParametersFromReflection( StringConverter::parseBool( val ) );
    }
    //-----------------------------------------------------------------------
    String GpuProgram::CmdClipDistance::doGet( const void *target ) const
    {
        const GpuProgram *t = static_cast<const GpuProgram *>( target );
        return StringConverter::toString( t->getNumClipDistances() );
    }
    void GpuProgram::CmdClipDistance::doSet( void *target, const String &val )
    {
        GpuProgram *t = static_cast<GpuProgram *>( target );
        unsigned int asUint = StringConverter::parseUnsignedInt( val );
        t->setNumClipDistances( static_cast<uint8>( asUint ) );
    }
    //-----------------------------------------------------------------------
    String GpuProgram::CmdSkeletal::doGet( const void *target ) const
    {
        const GpuProgram *t = static_cast<const GpuProgram *>( target );
        return StringConverter::toString( t->isSkeletalAnimationIncluded() );
    }
    void GpuProgram::CmdSkeletal::doSet( void *target, const String &val )
    {
        GpuProgram *t = static_cast<GpuProgram *>( target );
        t->setSkeletalAnimationIncluded( StringConverter::parseBool( val ) );
    }
    //-----------------------------------------------------------------------
    String GpuProgram::CmdMorph::doGet( const void *target ) const
    {
        const GpuProgram *t = static_cast<const GpuProgram *>( target );
        return StringConverter::toString( t->isMorphAnimationIncluded() );
    }
    void GpuProgram::CmdMorph::doSet( void *target, const String &val )
    {
        GpuProgram *t = static_cast<GpuProgram *>( target );
        t->setMorphAnimationIncluded( StringConverter::parseBool( val ) );
    }
    //-----------------------------------------------------------------------
    String GpuProgram::CmdPose::doGet( const void *target ) const
    {
        const GpuProgram *t = static_cast<const GpuProgram *>( target );
        return StringConverter::toString( t->getNumberOfPosesIncluded() );
    }
    void GpuProgram::CmdPose::doSet( void *target, const String &val )
    {
        GpuProgram *t = static_cast<GpuProgram *>( target );
        t->setPoseAnimationIncluded( (ushort)StringConverter::parseUnsignedInt( val ) );
    }
    //-----------------------------------------------------------------------
    String GpuProgram::CmdVTF::doGet( const void *target ) const
    {
        const GpuProgram *t = static_cast<const GpuProgram *>( target );
        return StringConverter::toString( t->isVertexTextureFetchRequired() );
    }
    void GpuProgram::CmdVTF::doSet( void *target, const String &val )
    {
        GpuProgram *t = static_cast<GpuProgram *>( target );
        t->setVertexTextureFetchRequired( StringConverter::parseBool( val ) );
    }
    //-----------------------------------------------------------------------
    String GpuProgram::CmdVPRTI::doGet( const void *target ) const
    {
        const GpuProgram *t = static_cast<const GpuProgram *>( target );
        return StringConverter::toString( t->isVpAndRtArrayIndexFromAnyShaderRequired() );
    }
    void GpuProgram::CmdVPRTI::doSet( void *target, const String &val )
    {
        GpuProgram *t = static_cast<GpuProgram *>( target );
        t->setVpAndRtArrayIndexFromAnyShaderRequired( StringConverter::parseBool( val ) );
    }
    //-----------------------------------------------------------------------
    String GpuProgram::CmdManualNamedConstsFile::doGet( const void *target ) const
    {
        const GpuProgram *t = static_cast<const GpuProgram *>( target );
        return t->getManualNamedConstantsFile();
    }
    void GpuProgram::CmdManualNamedConstsFile::doSet( void *target, const String &val )
    {
        GpuProgram *t = static_cast<GpuProgram *>( target );
        t->setManualNamedConstantsFile( val );
    }
    //-----------------------------------------------------------------------
    String GpuProgram::CmdAdjacency::doGet( const void *target ) const
    {
        const GpuProgram *t = static_cast<const GpuProgram *>( target );
        return StringConverter::toString( t->mNeedsAdjacencyInfo );
    }
    void GpuProgram::CmdAdjacency::doSet( void *target, const String &val )
    {
        LogManager::getSingleton().logMessage(
            "'uses_adjacency_information' is deprecated. "
            "Set the respective RenderOperation::OperationType instead." );
        GpuProgram *t = static_cast<GpuProgram *>( target );
        t->mNeedsAdjacencyInfo = StringConverter::parseBool( val );
    }
    //-----------------------------------------------------------------------
    String GpuProgram::CmdComputeGroupDims::doGet( const void *target ) const
    {
        const GpuProgram *t = static_cast<const GpuProgram *>( target );
        return StringConverter::toString( t->getComputeGroupDimensions() );
    }
    void GpuProgram::CmdComputeGroupDims::doSet( void *target, const String &val )
    {
        GpuProgram *t = static_cast<GpuProgram *>( target );
        t->setComputeGroupDimensions( StringConverter::parseVector3( val ) );
    }
    //-----------------------------------------------------------------------
    String GpuProgram::CmdRootLayout::doGet( const void *target ) const
    {
        return "Cannot retrieve PrefabRootLayout";
    }
    void GpuProgram::CmdRootLayout::doSet( void *target, const String &val )
    {
        GpuProgram *t = static_cast<GpuProgram *>( target );
        if( val == "high" )
            t->setPrefabRootLayout( PrefabRootLayout::High );
        else if( val == "max" )
            t->setPrefabRootLayout( PrefabRootLayout::Max );
        else if( val == "standard" )
            t->setPrefabRootLayout( PrefabRootLayout::Standard );
        else if( val == "none" )
            t->setPrefabRootLayout( PrefabRootLayout::None );
    }
    //-----------------------------------------------------------------------
    String GpuProgram::CmdUsesArrayBindings::doGet( const void *target ) const
    {
        return "Cannot retrieve uses_array_bindings";
    }
    void GpuProgram::CmdUsesArrayBindings::doSet( void *target, const String &val )
    {
        GpuProgram *t = static_cast<GpuProgram *>( target );
        t->setAutoReflectArrayBindingsInRootLayout( StringConverter::parseBool( val ) );
    }
}  // namespace Ogre
