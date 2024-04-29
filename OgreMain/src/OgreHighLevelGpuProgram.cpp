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

#include "OgreHighLevelGpuProgram.h"

#include "OgreException.h"
#include "OgreGpuProgramManager.h"
#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgreLogManager.h"
#include "OgreProfiler.h"
#include "OgreRoot.h"
#include "OgreStringConverter.h"

#include <sstream>

namespace Ogre
{
    //---------------------------------------------------------------------------
    HighLevelGpuProgram::CmdEnableIncludeHeader HighLevelGpuProgram::msEnableIncludeHeaderCmd;
    HighLevelGpuProgram::CmdUseHlmsParser HighLevelGpuProgram::msUseHlmsParser;
    //---------------------------------------------------------------------------
    HighLevelGpuProgram::HighLevelGpuProgram( ResourceManager *creator, const String &name,
                                              ResourceHandle handle, const String &group, bool isManual,
                                              ManualResourceLoader *loader ) :
        GpuProgram( creator, name, handle, group, isManual, loader ),
        mHighLevelLoaded( false ),
        mEnableIncludeHeader( false ),
        mUseHlmsParser( false ),
        mAssemblerProgram(),
        mConstantDefsBuilt( false )
    {
    }
    //---------------------------------------------------------------------------
    void HighLevelGpuProgram::setupBaseParamDictionary()
    {
        GpuProgram::setupBaseParamDictionary();

        ParamDictionary *dict = getParamDictionary();

        dict->addParameter(
            ParameterDef( "enable_include_header",
                          "Whether we should parse the source code looking for include files and\n"
                          "embedding the file. Disabled by default to avoid slowing down when\n"
                          "#include is not used. Not needed if the API natively supports it (D3D11).\n"
                          "\n"
                          "Single line comments are supported:\n"
                          "    // #include \"MyFile.h\" --> won't be included.\n"
                          "\n"
                          "Block comment lines are not supported, but may not matter if\n"
                          "the included file does not close the block:\n"
                          "    / *\n"
                          "         #include \"MyFile.h\" --> file will be included anyway.\n"
                          "     * /\n"
                          "\n"
                          "Preprocessor macros are not supported, but should not matter:\n"
                          "     #if SOME_MACRO\n"
                          "         #include \"MyFile.h\" --> file will be included anyway.\n"
                          "     #endif\n"
                          "\n"
                          "Recursive includes are supported (e.g. header includes a header)\n"
                          "\n"
                          "Beware included files mess up error reporting (wrong lines)",
                          PT_BOOL ),
            &msEnableIncludeHeaderCmd );
        dict->addParameter( ParameterDef( "use_hlms_parser",
                                          "Whether we should run the shader through the Hlms parser.\n"
                                          "Disabled by default.\n"
                                          "\n"
                                          "This parser doesn't yet have access to the entire PSO"
                                          "(i.e. Pixel Shader can't see Vertex Shader).\n"
                                          "\n"
                                          "However it's still useful, specially @foreach()\n"
                                          "\n"
                                          "Beware included files mess up error reporting (wrong lines)\n"
                                          "\n"
                                          "Beware included files mess up error reporting (wrong lines)",
                                          PT_BOOL ),
                            &msUseHlmsParser );
    }
    //---------------------------------------------------------------------------
    void HighLevelGpuProgram::loadImpl()
    {
        if( isSupported() )
        {
            // load self
            loadHighLevel();

            // create low-level implementation
            createLowLevelImpl();
            // load constructed assembler program (if it exists)
            if( mAssemblerProgram && mAssemblerProgram.get() != this )
            {
                mAssemblerProgram->load();
            }
        }
    }
    //---------------------------------------------------------------------------
    void HighLevelGpuProgram::unloadImpl()
    {
        if( mAssemblerProgram && mAssemblerProgram.get() != this )
        {
            mAssemblerProgram->getCreator()->remove( mAssemblerProgram->getHandle() );
            mAssemblerProgram.reset();
        }

        unloadHighLevel();
        resetCompileError();
    }
    //---------------------------------------------------------------------------
    HighLevelGpuProgram::~HighLevelGpuProgram()
    {
        // superclasses will trigger unload
    }
    //---------------------------------------------------------------------------
    GpuProgramParametersSharedPtr HighLevelGpuProgram::createParameters()
    {
        // Lock mutex before allowing this since this is a top-level method
        // called outside of the load()
        OGRE_LOCK_AUTO_MUTEX;

        // Make sure param defs are loaded
        GpuProgramParametersSharedPtr params = GpuProgramManager::getSingleton().createParameters();
        // Only populate named parameters if we can support this program
        if( this->isSupported() )
        {
            loadHighLevel();
            // Errors during load may have prevented compile
            if( this->isSupported() )
            {
                populateParameterNames( params );
            }
        }
        // Copy in default parameters if present
        if( mDefaultParams )
            params->copyConstantsFrom( *( mDefaultParams.get() ) );
        return params;
    }
    size_t HighLevelGpuProgram::calculateSize() const
    {
        size_t memSize = 0;
        memSize += sizeof( bool );
        if( mAssemblerProgram && ( mAssemblerProgram.get() != this ) )
            memSize += mAssemblerProgram->calculateSize();

        memSize += GpuProgram::calculateSize();

        return memSize;
    }

    //---------------------------------------------------------------------------
    void HighLevelGpuProgram::loadHighLevel()
    {
        if( !mHighLevelLoaded )
        {
            try
            {
                loadHighLevelImpl();
                mHighLevelLoaded = true;
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
            catch( const RuntimeAssertionException & )
            {
                throw;
            }
            catch( const Exception &e )
            {
                // will already have been logged
                LogManager::getSingleton().stream()
                    << "High-level program " << mName << " encountered an error "
                    << "during loading and is thus not supported.\n"
                    << e.getFullDescription();

                mCompileError = true;
            }
        }
    }
    //---------------------------------------------------------------------------
    void HighLevelGpuProgram::unloadHighLevel()
    {
        if( mHighLevelLoaded )
        {
            unloadHighLevelImpl();
            // Clear saved constant defs
            mConstantDefsBuilt = false;
            createParameterMappingStructures( true );

            mHighLevelLoaded = false;
        }
    }
    //---------------------------------------------------------------------------
    void HighLevelGpuProgram::dumpSourceIfHasIncludeEnabled()
    {
        if( ( mEnableIncludeHeader || mUseHlmsParser ) && mCompileError )
        {
            LogManager::getSingleton().logMessage(
                "Error found while compiling with use_hlms_parser or enable_include_header. "
                "This is the final output:\n"
                ">>> BEGIN SOURCE " +
                    mFilename,
                LML_CRITICAL );
            LogManager::getSingleton().logMessage( mSource, LML_CRITICAL );
            LogManager::getSingleton().logMessage( ">>> END SOURCE " + mFilename, LML_CRITICAL );
        }
    }
    //---------------------------------------------------------------------------
    void HighLevelGpuProgram::parseIncludeFile( String &source )
    {
        size_t startPos = 0;
        String includeKeyword = "#include ";

        startPos = source.find( includeKeyword, startPos );
        while( startPos != String::npos )
        {
            // Backtrack to see if have to skip commented lines like "// #include".
            // We do not support block comments /**/
            const size_t lineStartPos = source.rfind( '\n', startPos );
            if( lineStartPos == String::npos || lineStartPos + 2u >= source.size() ||
                source[lineStartPos + 1] != '/' || source[lineStartPos + 2] != '/' )
            {
                size_t pos = startPos + includeKeyword.length() + 1u;
                size_t eolMarkerPos = source.find( '\n', pos );
                size_t endPos0 = source.find( '"', pos );
                size_t endPos1 = source.find( '>', pos );
                size_t endPos = std::min( endPos0, endPos1 );

                if( endPos == String::npos ||
                    ( endPos >= eolMarkerPos && eolMarkerPos != String::npos ) )
                {
                    mCompileError = true;
                    dumpSourceIfHasIncludeEnabled();
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "Invalid #include syntax near " +
                                     source.substr( startPos, std::min( startPos + 10u,
                                                                        source.size() - startPos ) ),
                                 "HighLevelGpuProgram::parseIncludeFile" );
                }

                String file = source.substr( pos, endPos - pos );

                try
                {
                    DataStreamPtr includeStream =
                        ResourceGroupManager::getSingleton().openResource( file, mGroup, true, this );

                    String content = includeStream->getAsString();
                    source.replace( startPos, ( endPos + 1u ) - startPos, content );
                }
                catch( FileNotFoundException & )
                {
                    // Leave the included header, fallback to the compiler
                    //(e.g. Metal can include system headers)
                    startPos = endPos;
                }
            }
            else
            {
                startPos += sizeof( "#include " ) - 1u;
            }

            startPos = source.find( includeKeyword, startPos );
        }
    }
    //---------------------------------------------------------------------------
    void HighLevelGpuProgram::loadHighLevelImpl()
    {
        OgreProfileExhaustive( "HighLevelGpuProgram::loadHighLevelImpl" );

        if( mLoadFromFile )
        {
            // find & load source code
            DataStreamPtr stream =
                ResourceGroupManager::getSingleton().openResource( mFilename, mGroup, true, this );

            mSource = stream->getAsString();

            if( mEnableIncludeHeader )
                parseIncludeFile( mSource );

            if( mUseHlmsParser )
            {
                Hlms *hlms = Root::getSingletonPtr()->getHlmsManager()->getHlms( HLMS_LOW_LEVEL );
                String inputStr( mSource );
                hlms->parseOffline( mName, inputStr, mSource, Hlms::kNoTid );
            }
        }

        try
        {
            loadFromSource();
        }
        catch( RenderingAPIException & )
        {
            dumpSourceIfHasIncludeEnabled();
            throw;
        }

        dumpSourceIfHasIncludeEnabled();
    }
    //---------------------------------------------------------------------
    void HighLevelGpuProgram::setEnableIncludeHeader( bool bEnable ) { mEnableIncludeHeader = bEnable; }
    //---------------------------------------------------------------------
    bool HighLevelGpuProgram::getEnableIncludeHeader() const { return mEnableIncludeHeader; }
    //---------------------------------------------------------------------
    void HighLevelGpuProgram::setUseHlmsParser( bool bUse ) { mUseHlmsParser = bUse; }
    //---------------------------------------------------------------------
    bool HighLevelGpuProgram::getUseHlmsParser() const { return mUseHlmsParser; }
    //---------------------------------------------------------------------
    const GpuNamedConstants &HighLevelGpuProgram::getConstantDefinitions() const
    {
        if( !mConstantDefsBuilt )
        {
            buildConstantDefinitions();
            mConstantDefsBuilt = true;
        }
        return *mConstantDefs.get();
    }
    //---------------------------------------------------------------------
    void HighLevelGpuProgram::populateParameterNames( GpuProgramParametersSharedPtr params )
    {
        getConstantDefinitions();
        params->_setNamedConstants( mConstantDefs );
        // also set logical / physical maps for programs which use this
        params->_setLogicalIndexes( mFloatLogicalToPhysical, mDoubleLogicalToPhysical,
                                    mIntLogicalToPhysical, mUIntLogicalToPhysical,
                                    mBoolLogicalToPhysical );
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    String HighLevelGpuProgram::CmdEnableIncludeHeader::doGet( const void *target ) const
    {
        bool retVal = static_cast<const HighLevelGpuProgram *>( target )->getEnableIncludeHeader();
        return StringConverter::toString( retVal );
    }
    //-----------------------------------------------------------------------------------
    void HighLevelGpuProgram::CmdEnableIncludeHeader::doSet( void *target, const String &val )
    {
        bool enableIncludeHeader = StringConverter::parseBool( val );
        static_cast<HighLevelGpuProgram *>( target )->setEnableIncludeHeader( enableIncludeHeader );
    }
    //-----------------------------------------------------------------------------------
    String HighLevelGpuProgram::CmdUseHlmsParser::doGet( const void *target ) const
    {
        const bool retVal = static_cast<const HighLevelGpuProgram *>( target )->getUseHlmsParser();
        return StringConverter::toString( retVal );
    }
    //-----------------------------------------------------------------------------------
    void HighLevelGpuProgram::CmdUseHlmsParser::doSet( void *target, const String &val )
    {
        const bool bUse = StringConverter::parseBool( val );
        static_cast<HighLevelGpuProgram *>( target )->setUseHlmsParser( bUse );
    }
}  // namespace Ogre
