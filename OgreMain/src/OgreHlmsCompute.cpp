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

#include "OgreHlmsCompute.h"

#include "CommandBuffer/OgreCommandBuffer.h"
#include "Compositor/OgreCompositorShadowNode.h"
#include "Hash/MurmurHash3.h"
#include "OgreFileSystem.h"
#include "OgreHighLevelGpuProgram.h"
#include "OgreHighLevelGpuProgramManager.h"
#include "OgreHlmsComputeJob.h"
#include "OgreHlmsManager.h"
#include "OgreLogManager.h"
#include "OgreRootLayout.h"
#include "OgreSceneManager.h"
#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreTexBufferPacked.h"
#include "Vao/OgreUavBufferPacked.h"

#if OGRE_ARCH_TYPE == OGRE_ARCHITECTURE_32
#    define OGRE_HASH128_FUNC MurmurHash3_x86_128
#else
#    define OGRE_HASH128_FUNC MurmurHash3_x64_128
#endif

#include <fstream>

namespace Ogre
{
    const IdString ComputeProperty::ThreadsPerGroupX = IdString( "threads_per_group_x" );
    const IdString ComputeProperty::ThreadsPerGroupY = IdString( "threads_per_group_y" );
    const IdString ComputeProperty::ThreadsPerGroupZ = IdString( "threads_per_group_z" );
    const IdString ComputeProperty::NumThreadGroupsX = IdString( "num_thread_groups_x" );
    const IdString ComputeProperty::NumThreadGroupsY = IdString( "num_thread_groups_y" );
    const IdString ComputeProperty::NumThreadGroupsZ = IdString( "num_thread_groups_z" );

    const IdString ComputeProperty::TypedUavLoad = IdString( "typed_uav_load" );

    const IdString ComputeProperty::NumTextureSlots = IdString( "num_texture_slots" );
    const IdString ComputeProperty::MaxTextureSlot = IdString( "max_texture_slot" );
    const char *ComputeProperty::Texture = "texture";

    const IdString ComputeProperty::NumUavSlots = IdString( "num_uav_slots" );
    const IdString ComputeProperty::MaxUavSlot = IdString( "max_uav_slot" );
    const char *ComputeProperty::Uav = "uav";

    // Must be sorted from best to worst
    const String BestD3DComputeShaderTargets[3] = { "cs_5_0", "cs_4_1", "cs_4_0" };

    HlmsCompute::HlmsCompute( AutoParamDataSource *autoParamDataSource ) :
        Hlms( HLMS_COMPUTE, "compute", 0, 0 ),
        mAutoParamDataSource( autoParamDataSource ),
        mComputeShaderTarget( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    HlmsCompute::~HlmsCompute()
    {
        destroyAllComputeJobs();
        if( mHlmsManager )
        {
            mHlmsManager->unregisterComputeHlms();
            mHlmsManager = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsCompute::_changeRenderSystem( RenderSystem *newRs )
    {
        Hlms::_changeRenderSystem( newRs );

        if( mRenderSystem )
        {
            const RenderSystemCapabilities *capabilities = mRenderSystem->getCapabilities();

            if( mShaderProfile == "hlsl" || mShaderProfile == "hlslvk" )
            {
                for( size_t j = 0; j < 3 && !mComputeShaderTarget; ++j )
                {
                    if( capabilities->isShaderProfileSupported( BestD3DComputeShaderTargets[j] ) )
                        mComputeShaderTarget = &BestD3DComputeShaderTargets[j];
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsCompute::processPieces( const StringVector &pieceFiles )
    {
        ResourceGroupManager &resourceGroupMgr = ResourceGroupManager::getSingleton();

        StringVector::const_iterator itor = pieceFiles.begin();
        StringVector::const_iterator endt = pieceFiles.end();

        while( itor != endt )
        {
            String filename = *itor;

            // If it has an explicit extension, only open it if it matches the current
            // render system's. If it doesn't, then we add it ourselves.
            String::size_type pos = filename.find_last_of( '.' );
            if( pos == String::npos ||
                ( filename.compare( pos + 1, String::npos, mShaderFileExt ) != 0 &&
                  filename.compare( pos + 1, String::npos, "any" ) != 0 &&
                  filename.compare( pos + 1, String::npos, "metal" ) != 0 &&
                  filename.compare( pos + 1, String::npos, "glsl" ) != 0 &&
                  filename.compare( pos + 1, String::npos, "hlsl" ) != 0 ) )
            {
                filename += mShaderFileExt;
            }

            DataStreamPtr inFile = resourceGroupMgr.openResource( filename );

            String inString;
            String outString;

            inString.resize( inFile->size() );
            inFile->read( &inString[0], inFile->size() );

            this->parseMath( inString, outString, kNoTid );
            while( outString.find( "@foreach" ) != String::npos )
            {
                this->parseForEach( outString, inString, kNoTid );
                inString.swap( outString );
            }
            this->parseProperties( outString, inString, kNoTid );
            this->parseUndefPieces( inString, outString, kNoTid );
            this->collectPieces( outString, inString, kNoTid );
            this->parseCounter( inString, outString, kNoTid );

            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    HlmsComputePso HlmsCompute::compileShader( HlmsComputeJob *job, uint32 finalHash )
    {
        // Assumes mSetProperties is already set
        // mSetProperties[kNoTid].clear();
        {
            // Add RenderSystem-specific properties
            IdStringVec::const_iterator itor = mRsSpecificExtensions.begin();
            IdStringVec::const_iterator endt = mRsSpecificExtensions.end();

            while( itor != endt )
                setProperty( kNoTid, *itor++, 1 );
        }

        GpuProgramPtr shader;
        // Generate the shader

        // Collect pieces
        mT[kNoTid].pieces.clear();

        // Start with the pieces sent by the user
        mT[kNoTid].pieces = job->mPieces;

        const String sourceFilename = job->mSourceFilename + mShaderFileExt;

        ResourceGroupManager &resourceGroupMgr = ResourceGroupManager::getSingleton();
        DataStreamPtr inFile = resourceGroupMgr.openResource( sourceFilename );

        if( mShaderProfile == "glsl" || mShaderProfile == "glslvk" )  // TODO: String comparision
        {
            setProperty( kNoTid, HlmsBaseProp::GL3Plus,
                         mRenderSystem->getNativeShadingLanguageVersion() );
        }

        setProperty( kNoTid, HlmsBaseProp::Syntax, static_cast<int32>( mShaderSyntax.getU32Value() ) );
        setProperty( kNoTid, HlmsBaseProp::Hlsl,
                     static_cast<int32>( HlmsBaseProp::Hlsl.getU32Value() ) );
        setProperty( kNoTid, HlmsBaseProp::Glsl,
                     static_cast<int32>( HlmsBaseProp::Glsl.getU32Value() ) );
        setProperty( kNoTid, HlmsBaseProp::Glslvk,
                     static_cast<int32>( HlmsBaseProp::Glslvk.getU32Value() ) );
        setProperty( kNoTid, HlmsBaseProp::Hlslvk,
                     static_cast<int32>( HlmsBaseProp::Hlslvk.getU32Value() ) );
        setProperty( kNoTid, HlmsBaseProp::Metal,
                     static_cast<int32>( HlmsBaseProp::Metal.getU32Value() ) );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        setProperty( kNoTid, HlmsBaseProp::iOS, 1 );
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        setProperty( kNoTid, HlmsBaseProp::macOS, 1 );
#endif
        setProperty( kNoTid, HlmsBaseProp::Full32,
                     static_cast<int32>( HlmsBaseProp::Full32.getU32Value() ) );
        setProperty( kNoTid, HlmsBaseProp::Midf16,
                     static_cast<int32>( HlmsBaseProp::Midf16.getU32Value() ) );
        setProperty( kNoTid, HlmsBaseProp::Relaxed,
                     static_cast<int32>( HlmsBaseProp::Relaxed.getU32Value() ) );
        setProperty( kNoTid, HlmsBaseProp::PrecisionMode, getSupportedPrecisionModeHash() );

        if( mFastShaderBuildHack )
            setProperty( kNoTid, HlmsBaseProp::FastShaderBuildHack, 1 );

        // Piece files
        processPieces( job->mIncludedPieceFiles );

        String inString;
        String outString;

        inString.resize( inFile->size() );
        inFile->read( &inString[0], inFile->size() );

        bool syntaxError = false;

        syntaxError |= this->parseMath( inString, outString, kNoTid );
        while( !syntaxError && outString.find( "@foreach" ) != String::npos )
        {
            syntaxError |= this->parseForEach( outString, inString, kNoTid );
            inString.swap( outString );
        }
        syntaxError |= this->parseProperties( outString, inString, kNoTid );
        syntaxError |= this->parseUndefPieces( inString, outString, kNoTid );
        while( !syntaxError && ( outString.find( "@piece" ) != String::npos ||
                                 outString.find( "@insertpiece" ) != String::npos ) )
        {
            syntaxError |= this->collectPieces( outString, inString, kNoTid );
            syntaxError |= this->insertPieces( inString, outString, kNoTid );
        }
        syntaxError |= this->parseCounter( outString, inString, kNoTid );

        outString.swap( inString );

        if( syntaxError )
        {
            LogManager::getSingleton().logMessage( "There were HLMS syntax errors while parsing " +
                                                   StringConverter::toString( finalHash ) +
                                                   job->mSourceFilename + mShaderFileExt );
        }

        String debugFilenameOutput;

        if( mDebugOutput )
        {
            debugFilenameOutput = mOutputPath + "./" + StringConverter::toString( finalHash ) +
                                  job->mSourceFilename + mShaderFileExt;
            std::ofstream outFile( Ogre::fileSystemPathFromString( debugFilenameOutput ).c_str(),
                                   std::ios::out | std::ios::binary );
            if( mDebugOutputProperties )
                dumpProperties( outFile, kNoTid );
            outFile.write( &outString[0], static_cast<std::streamsize>( outString.size() ) );
        }

        // Don't create and compile if template requested not to
        if( !getProperty( kNoTid, HlmsBaseProp::DisableStage ) )
        {
            // Very similar to what the GpuProgramManager does with its microcode cache,
            // but we **need** to know if two Compute Shaders share the same source code.
            Hash hashVal;
            OGRE_HASH128_FUNC( outString.c_str(), static_cast<int>( outString.size() ), IdString::Seed,
                               &hashVal );

            const RenderSystemCapabilities *capabilities = mRenderSystem->getCapabilities();

            if( capabilities->hasCapability( RSC_EXPLICIT_API ) )
            {
                // If two shaders have the exact same source code but different
                // Root Layout, we should treat them differently
                RootLayout rootLayout;
                // We MUST memset due to internal padding;
                // otherwise we'll be hashing uninitialized values
                memset( &rootLayout, 0, sizeof( rootLayout ) );
                rootLayout.mCompute = true;
                job->setupRootLayout( rootLayout );

                Hash hashValTmp[2] = { hashVal, Hash() };
                OGRE_HASH128_FUNC( &rootLayout, sizeof( rootLayout ), IdString::Seed, &hashValTmp[1] );
                OGRE_HASH128_FUNC( hashValTmp, sizeof( hashValTmp ), IdString::Seed, &hashVal );
            }

            CompiledShaderMap::const_iterator itor = mCompiledShaderCache.find( hashVal );
            if( itor != mCompiledShaderCache.end() )
            {
                shader = itor->second;
            }
            else
            {
                HighLevelGpuProgramManager *gpuProgramManager =
                    HighLevelGpuProgramManager::getSingletonPtr();

                HighLevelGpuProgramPtr gp = gpuProgramManager->createProgram(
                    StringConverter::toString( finalHash ) + job->mSourceFilename,
                    ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME, mShaderProfile,
                    GPT_COMPUTE_PROGRAM );
                gp->setSource( outString, debugFilenameOutput );

                {
                    RootLayout rootLayout;
                    rootLayout.mCompute = true;
                    job->setupRootLayout( rootLayout );
                    gp->setRootLayout( gp->getType(), rootLayout );
                    if( getProperty( kNoTid, "uses_array_bindings" ) )
                        gp->setAutoReflectArrayBindingsInRootLayout( true );
                }

                if( mComputeShaderTarget )
                {
                    // D3D-specific
                    gp->setParameter( "target", *mComputeShaderTarget );
                    gp->setParameter( "entry_point", "main" );
                }

                gp->setSkeletalAnimationIncluded( getProperty( kNoTid, HlmsBaseProp::Skeleton ) != 0 );
                gp->setMorphAnimationIncluded( false );
                gp->setPoseAnimationIncluded( getProperty( kNoTid, HlmsBaseProp::Pose ) != 0 );
                gp->setVertexTextureFetchRequired( false );

                gp->load();

                shader = gp;

                mCompiledShaderCache[hashVal] = shader;
            }
        }

        // Reset the disable flag.
        setProperty( kNoTid, HlmsBaseProp::DisableStage, 0 );

        HlmsComputePso pso;
        pso.initialize();
        pso.computeShader = shader;
        pso.computeParams = shader->createParameters();
        pso.mThreadsPerGroup[0] = (uint32)( getProperty( kNoTid, ComputeProperty::ThreadsPerGroupX ) );
        pso.mThreadsPerGroup[1] = (uint32)( getProperty( kNoTid, ComputeProperty::ThreadsPerGroupY ) );
        pso.mThreadsPerGroup[2] = (uint32)( getProperty( kNoTid, ComputeProperty::ThreadsPerGroupZ ) );
        pso.mNumThreadGroups[0] = (uint32)( getProperty( kNoTid, ComputeProperty::NumThreadGroupsX ) );
        pso.mNumThreadGroups[1] = (uint32)( getProperty( kNoTid, ComputeProperty::NumThreadGroupsY ) );
        pso.mNumThreadGroups[2] = (uint32)( getProperty( kNoTid, ComputeProperty::NumThreadGroupsZ ) );

        if( pso.mThreadsPerGroup[0] * pso.mThreadsPerGroup[1] * pso.mThreadsPerGroup[2] == 0u ||
            pso.mNumThreadGroups[0] * pso.mNumThreadGroups[1] * pso.mNumThreadGroups[2] == 0u )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         job->getNameStr() +
                             ": Shader or C++ must set threads_per_group_x, threads_per_group_y & "
                             "threads_per_group_z and num_thread_groups_x through num_thread_groups_z."
                             " Otherwise we can't run on Metal. Use @pset( threads_per_group_x, 512 );"
                             " or read the value using @value( threads_per_group_x ) if you've already"
                             " set it from C++ or the JSON material",
                         "HlmsCompute::compileShader" );
        }

        ShaderParams *shaderParams = job->_getShaderParams( "default" );
        if( shaderParams )
            shaderParams->updateParameters( pso.computeParams, true );

        shaderParams = job->_getShaderParams( mShaderProfile );
        if( shaderParams )
            shaderParams->updateParameters( pso.computeParams, true );

        mRenderSystem->_hlmsComputePipelineStateObjectCreated( &pso );

        return pso;
    }
    //-----------------------------------------------------------------------------------
    void HlmsCompute::destroyComputeJob( IdString name )
    {
        HlmsComputeJobMap::iterator itor = mComputeJobs.find( name );

        if( itor != mComputeJobs.end() )
        {
            HlmsComputeJob *job = itor->second.computeJob;
            ComputePsoCacheVec::iterator itCache = mComputeShaderCache.begin();
            ComputePsoCacheVec::iterator enCache = mComputeShaderCache.end();

            while( itCache != enCache )
            {
                if( itCache->job == job )
                {
                    mRenderSystem->_hlmsComputePipelineStateObjectDestroyed( &itCache->pso );
                    // We can't remove the entry, but we can at least cleanup
                    // some memory and leave an empty, unused entry
                    *itCache = ComputePsoCache();
                    mFreeShaderCacheEntries.push_back(
                        static_cast<size_t>( itCache - mComputeShaderCache.begin() ) );
                }
                ++itCache;
            }

            OGRE_DELETE itor->second.computeJob;
            mComputeJobs.erase( itor );
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsCompute::destroyAllComputeJobs()
    {
        clearShaderCache();

        HlmsComputeJobMap::const_iterator itor = mComputeJobs.begin();
        HlmsComputeJobMap::const_iterator endt = mComputeJobs.end();

        while( itor != endt )
        {
            OGRE_DELETE itor->second.computeJob;
            ++itor;
        }

        mComputeJobs.clear();
    }
    //-----------------------------------------------------------------------------------
    void HlmsCompute::clearShaderCache()
    {
        if( mRenderSystem )
            mRenderSystem->_setComputePso( 0 );

        ComputePsoCacheVec::iterator itor = mComputeShaderCache.begin();
        ComputePsoCacheVec::iterator endt = mComputeShaderCache.end();

        while( itor != endt )
        {
            if( itor->job )
            {
                mRenderSystem->_hlmsComputePipelineStateObjectDestroyed( &itor->pso );
                itor->job->mPsoCacheHash = std::numeric_limits<size_t>::max();
            }
            ++itor;
        }

        Hlms::clearShaderCache();
        mCompiledShaderCache.clear();
        mComputeShaderCache.clear();
        mFreeShaderCacheEntries.clear();
    }
    //-----------------------------------------------------------------------------------
    void HlmsCompute::dispatch( HlmsComputeJob *job, SceneManager *sceneManager, Camera *camera )
    {
        job->_calculateNumThreadGroupsBasedOnSetting();

        if( job->mPsoCacheHash >= mComputeShaderCache.size() )
        {
            // Potentially needs to recompile.
            job->_updateAutoProperties();

            ComputePsoCache psoCache;
            psoCache.job = job;
            // To perform the search, temporarily borrow the properties to avoid an allocation & a copy.
            psoCache.setProperties.swap( job->mSetProperties );
            ComputePsoCacheVec::const_iterator itor =
                std::find( mComputeShaderCache.begin(), mComputeShaderCache.end(), psoCache );
            if( itor == mComputeShaderCache.end() )
            {
                // Needs to recompile.

                // Return back the borrowed properties and make
                // a hard copy for starting the compilation.
                psoCache.setProperties.swap( job->mSetProperties );
                this->mT[kNoTid].setProperties = job->mSetProperties;

                // Uset the HlmsComputePso, as the ptr may be cached by the
                // RenderSystem and this could be invalidated
                mRenderSystem->_setComputePso( 0 );

                size_t newCacheEntryIdx = mComputeShaderCache.size();
                if( mFreeShaderCacheEntries.empty() )
                    mComputeShaderCache.push_back( ComputePsoCache() );
                else
                {
                    newCacheEntryIdx = mFreeShaderCacheEntries.back();
                    mFreeShaderCacheEntries.pop_back();
                }

                // Compile and add the PSO to the cache.
                psoCache.pso = compileShader( job, (uint32)newCacheEntryIdx );

                ShaderParams *shaderParams = job->_getShaderParams( "default" );
                if( shaderParams )
                    psoCache.paramsUpdateCounter = shaderParams->getUpdateCounter();
                if( shaderParams )
                    psoCache.paramsProfileUpdateCounter = shaderParams->getUpdateCounter();

                mComputeShaderCache[newCacheEntryIdx] = psoCache;

                // The PSO in the cache doesn't have the properties. Make a hard copy.
                // We can use this->mSetProperties as it may have been modified during
                // compilerShader by the template.
                mComputeShaderCache[newCacheEntryIdx].setProperties = job->mSetProperties;

                job->mPsoCacheHash = newCacheEntryIdx;
            }
            else
            {
                // It was already in the cache. Return back the borrowed
                // properties and set the proper index to the cache.
                psoCache.setProperties.swap( job->mSetProperties );
                job->mPsoCacheHash = static_cast<size_t>( itor - mComputeShaderCache.begin() );
            }
        }

        ComputePsoCache &psoCache = mComputeShaderCache[job->mPsoCacheHash];

        {
            // Update dirty parameters, if necessary
            ShaderParams *shaderParams = job->_getShaderParams( "default" );
            if( shaderParams && psoCache.paramsUpdateCounter != shaderParams->getUpdateCounter() )
            {
                shaderParams->updateParameters( psoCache.pso.computeParams, false );
                psoCache.paramsUpdateCounter = shaderParams->getUpdateCounter();
            }

            shaderParams = job->_getShaderParams( mShaderProfile );
            if( shaderParams && psoCache.paramsProfileUpdateCounter != shaderParams->getUpdateCounter() )
            {
                shaderParams->updateParameters( psoCache.pso.computeParams, false );
                psoCache.paramsProfileUpdateCounter = shaderParams->getUpdateCounter();
            }
        }

        mRenderSystem->_setComputePso( &psoCache.pso );

        HlmsComputeJob::ConstBufferSlotVec::const_iterator itConst = job->mConstBuffers.begin();
        HlmsComputeJob::ConstBufferSlotVec::const_iterator enConst = job->mConstBuffers.end();

        while( itConst != enConst )
        {
            itConst->buffer->bindBufferCS( itConst->slotIdx );
            ++itConst;
        }

        if( job->mTexturesDescSet )
            mRenderSystem->_setTexturesCS( job->getGlTexSlotStart(), job->mTexturesDescSet );
        if( job->mSamplersDescSet )
            mRenderSystem->_setSamplersCS( job->getGlTexSlotStart(), job->mSamplersDescSet );
        if( job->mUavsDescSet )
            mRenderSystem->_setUavCS( 0u, job->mUavsDescSet );

        mAutoParamDataSource->setCurrentJob( job );
        mAutoParamDataSource->setCurrentCamera( camera );
        mAutoParamDataSource->setCurrentSceneManager( sceneManager );
        // mAutoParamDataSource->setCurrentShadowNode( shadowNode );
        // mAutoParamDataSource->setCurrentViewport( sceneManager->getCurrentViewport() );

        GpuProgramParametersSharedPtr csParams = psoCache.pso.computeParams;
        csParams->_updateAutoParams( mAutoParamDataSource, GPV_ALL );
        mRenderSystem->bindGpuProgramParameters( GPT_COMPUTE_PROGRAM, csParams, GPV_ALL );

        mRenderSystem->_dispatch( psoCache.pso );
    }
    //----------------------------------------------------------------------------------
    HlmsDatablock *HlmsCompute::createDatablockImpl( IdString datablockName,
                                                     const HlmsMacroblock *macroblock,
                                                     const HlmsBlendblock *blendblock,
                                                     const HlmsParamVec &paramVec )
    {
        return 0;
    }
    //----------------------------------------------------------------------------------
    void HlmsCompute::setupRootLayout( RootLayout &rootLayout, const size_t tid ) {}
    //----------------------------------------------------------------------------------
    void HlmsCompute::reloadFrom( Archive *newDataFolder, ArchiveVec *libraryFolders )
    {
        Hlms::reloadFrom( newDataFolder, libraryFolders );

        HlmsComputeJobMap::const_iterator itor = mComputeJobs.begin();
        HlmsComputeJobMap::const_iterator endt = mComputeJobs.end();

        while( itor != endt )
        {
            HlmsComputeJob *job = itor->second.computeJob;
            map<IdString, ShaderParams>::type::iterator it = job->mShaderParams.begin();
            map<IdString, ShaderParams>::type::iterator en = job->mShaderParams.end();

            while( it != en )
            {
                ShaderParams &shaderParams = it->second;
                ShaderParams::ParamVec::iterator itParam = shaderParams.mParams.begin();
                ShaderParams::ParamVec::iterator enParam = shaderParams.mParams.end();

                while( itParam != enParam )
                {
                    itParam->isDirty = true;
                    ++itParam;
                }

                shaderParams.setDirty();
                ++it;
            }

            ++itor;
        }
    }
    //----------------------------------------------------------------------------------
    HlmsComputeJob *HlmsCompute::createComputeJob( IdString datablockName, const String &refName,
                                                   const String &sourceFilename,
                                                   const StringVector &includedPieceFiles )
    {
        HlmsComputeJob *retVal = 0;

        std::pair<HlmsComputeJobMap::iterator, bool> insertion =
            mComputeJobs.insert( { datablockName, ComputeJobEntry( 0, refName ) } );
        if( insertion.second )
        {
            retVal = OGRE_NEW HlmsComputeJob( datablockName, this, sourceFilename, includedPieceFiles );
            insertion.first->second.computeJob = retVal;
        }
        else
        {
            OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                         "Compute Job with name " + datablockName.getFriendlyText() + " already exists!",
                         "HlmsCompute::createComputeJob" );
        }

        return retVal;
    }
    //----------------------------------------------------------------------------------
    HlmsComputeJob *HlmsCompute::findComputeJob( IdString datablockName ) const
    {
        HlmsComputeJob *retVal = findComputeJobNoThrow( datablockName );

        if( !retVal )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Compute Job with name " + datablockName.getFriendlyText() + " not found",
                         "HlmsCompute::findComputeJob" );
        }

        return retVal;
    }
    //----------------------------------------------------------------------------------
    HlmsComputeJob *HlmsCompute::findComputeJobNoThrow( IdString datablockName ) const
    {
        HlmsComputeJob *retVal = 0;

        HlmsComputeJobMap::const_iterator itor = mComputeJobs.find( datablockName );
        if( itor != mComputeJobs.end() )
            retVal = itor->second.computeJob;

        return retVal;
    }
    //----------------------------------------------------------------------------------
    const String *HlmsCompute::getJobNameStr( IdString name ) const
    {
        String const *retVal = 0;
        HlmsComputeJobMap::const_iterator itor = mComputeJobs.find( name );
        if( itor != mComputeJobs.end() )
            retVal = &itor->second.name;

        return retVal;
    }
    //----------------------------------------------------------------------------------
    HlmsDatablock *HlmsCompute::createDefaultDatablock() { return 0; }
    //----------------------------------------------------------------------------------
    uint32 HlmsCompute::fillBuffersFor( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                        bool casterPass, uint32 lastCacheHash, uint32 lastTextureHash )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "This is a Compute Hlms",
                     "HlmsCompute::fillBuffersFor" );
    }
    uint32 HlmsCompute::fillBuffersForV1( const HlmsCache *cache,
                                          const QueuedRenderable &queuedRenderable, bool casterPass,
                                          uint32 lastCacheHash, CommandBuffer *commandBuffer )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "This is a Compute Hlms",
                     "HlmsCompute::fillBuffersForV1" );
    }
    uint32 HlmsCompute::fillBuffersForV2( const HlmsCache *cache,
                                          const QueuedRenderable &queuedRenderable, bool casterPass,
                                          uint32 lastCacheHash, CommandBuffer *commandBuffer )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "This is a Compute Hlms",
                     "HlmsCompute::fillBuffersForV2" );
    }
}  // namespace Ogre

#undef OGRE_HASH128_FUNC
