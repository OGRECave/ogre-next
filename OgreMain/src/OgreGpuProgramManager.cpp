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

#include "OgreGpuProgramManager.h"

#include "OgreHighLevelGpuProgramManager.h"
#include "OgreRenderSystem.h"
#include "OgreRoot.h"

#include "Hash/MurmurHash3.h"

#if OGRE_ARCH_TYPE == OGRE_ARCHITECTURE_32
#    define OGRE_HASH128_FUNC MurmurHash3_x86_128
#else
#    define OGRE_HASH128_FUNC MurmurHash3_x64_128
#endif

namespace Ogre
{
    //-----------------------------------------------------------------------
    template <>
    GpuProgramManager *Singleton<GpuProgramManager>::msSingleton = 0;
    GpuProgramManager *GpuProgramManager::getSingletonPtr() { return msSingleton; }
    GpuProgramManager &GpuProgramManager::getSingleton()
    {
        assert( msSingleton );
        return ( *msSingleton );
    }
    //-----------------------------------------------------------------------
    GpuProgramPtr GpuProgramManager::getByName( const String &name, bool preferHighLevelPrograms )
    {
        return std::static_pointer_cast<GpuProgram>(
            getResourceByName( name, preferHighLevelPrograms ) );
    }
    //---------------------------------------------------------------------------
    GpuProgramManager::GpuProgramManager()
    {
        // Loading order
        mLoadOrder = 50.0f;
        // Resource type
        mResourceType = "GpuProgram";
        mSaveMicrocodesToCache = false;
        mCacheDirty = false;

        // subclasses should register with resource group manager
    }
    //---------------------------------------------------------------------------
    GpuProgramManager::~GpuProgramManager()
    {
        // subclasses should unregister with resource group manager
    }
    //---------------------------------------------------------------------------
    GpuProgramPtr GpuProgramManager::load( const String &name, const String &groupName,
                                           const String &filename, GpuProgramType gptype,
                                           const String &syntaxCode )
    {
        GpuProgramPtr prg;
        {
            OGRE_LOCK_AUTO_MUTEX;
            prg = getByName( name );
            if( !prg )
            {
                prg = createProgram( name, groupName, filename, gptype, syntaxCode );
            }
        }
        prg->load();
        return prg;
    }
    //---------------------------------------------------------------------------
    GpuProgramPtr GpuProgramManager::loadFromString( const String &name, const String &groupName,
                                                     const String &code, GpuProgramType gptype,
                                                     const String &syntaxCode )
    {
        GpuProgramPtr prg;
        {
            OGRE_LOCK_AUTO_MUTEX;
            prg = getByName( name );
            if( !prg )
            {
                prg = createProgramFromString( name, groupName, code, gptype, syntaxCode );
            }
        }
        prg->load();
        return prg;
    }
    //---------------------------------------------------------------------------
    ResourcePtr GpuProgramManager::create( const String &name, const String &group,
                                           GpuProgramType gptype, const String &syntaxCode,
                                           bool isManual, ManualResourceLoader *loader )
    {
        // Call creation implementation
        ResourcePtr ret = ResourcePtr(
            createImpl( name, getNextHandle(), group, isManual, loader, gptype, syntaxCode ) );

        addImpl( ret );
        // Tell resource group manager
        ResourceGroupManager::getSingleton()._notifyResourceCreated( ret );
        return ret;
    }
    //---------------------------------------------------------------------------
    GpuProgramPtr GpuProgramManager::createProgram( const String &name, const String &groupName,
                                                    const String &filename, GpuProgramType gptype,
                                                    const String &syntaxCode )
    {
        GpuProgramPtr prg =
            std::static_pointer_cast<GpuProgram>( create( name, groupName, gptype, syntaxCode ) );
        // Set all prarmeters (create does not set, just determines factory)
        prg->setType( gptype );
        prg->setSyntaxCode( syntaxCode );
        prg->setSourceFile( filename );
        return prg;
    }
    //---------------------------------------------------------------------------
    GpuProgramPtr GpuProgramManager::createProgramFromString( const String &name,
                                                              const String &groupName,
                                                              const String &code, GpuProgramType gptype,
                                                              const String &syntaxCode )
    {
        GpuProgramPtr prg =
            std::static_pointer_cast<GpuProgram>( create( name, groupName, gptype, syntaxCode ) );
        // Set all prarmeters (create does not set, just determines factory)
        prg->setType( gptype );
        prg->setSyntaxCode( syntaxCode );
        prg->setSource( code );
        return prg;
    }
    //---------------------------------------------------------------------------
    const GpuProgramManager::SyntaxCodes &GpuProgramManager::getSupportedSyntax() const
    {
        // Use the current render system
        RenderSystem *rs = Root::getSingleton().getRenderSystem();

        // Get the supported syntaxed from RenderSystemCapabilities
        return rs->getCapabilities()->getSupportedShaderProfiles();
    }

    //---------------------------------------------------------------------------
    bool GpuProgramManager::isSyntaxSupported( const String &syntaxCode ) const
    {
        // Use the current render system
        RenderSystem *rs = Root::getSingleton().getRenderSystem();

        // Get the supported syntax from RenderSystemCapabilities
        return rs->getCapabilities()->isShaderProfileSupported( syntaxCode );
    }
    //---------------------------------------------------------------------------
    ResourcePtr GpuProgramManager::getResourceByName( const String &name, bool preferHighLevelPrograms )
    {
        ResourcePtr ret;
        if( preferHighLevelPrograms )
        {
            ret = HighLevelGpuProgramManager::getSingleton().getResourceByName( name );
            if( ret )
                return ret;
        }
        return ResourceManager::getResourceByName( name );
    }
    //-----------------------------------------------------------------------------
    GpuProgramParametersSharedPtr GpuProgramManager::createParameters()
    {
        return GpuProgramParametersSharedPtr( OGRE_NEW GpuProgramParameters() );
    }
    //---------------------------------------------------------------------
    GpuSharedParametersPtr GpuProgramManager::createSharedParameters( const String &name )
    {
        if( mSharedParametersMap.find( name ) != mSharedParametersMap.end() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "The shared parameter set '" + name + "' already exists!",
                         "GpuProgramManager::createSharedParameters" );
        }
        GpuSharedParametersPtr ret( OGRE_NEW GpuSharedParameters( name ) );
        mSharedParametersMap[name] = ret;
        return ret;
    }
    //---------------------------------------------------------------------
    GpuSharedParametersPtr GpuProgramManager::getSharedParameters( const String &name ) const
    {
        SharedParametersMap::const_iterator i = mSharedParametersMap.find( name );
        if( i == mSharedParametersMap.end() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "No shared parameter set with name '" + name + "'!",
                         "GpuProgramManager::createSharedParameters" );
        }
        return i->second;
    }
    //---------------------------------------------------------------------
    const GpuProgramManager::SharedParametersMap &GpuProgramManager::getAvailableSharedParameters() const
    {
        return mSharedParametersMap;
    }
    //---------------------------------------------------------------------
    bool GpuProgramManager::getSaveMicrocodesToCache() { return mSaveMicrocodesToCache; }
    //---------------------------------------------------------------------
    bool GpuProgramManager::canGetCompiledShaderBuffer()
    {
        // Use the current render system
        RenderSystem *rs = Root::getSingleton().getRenderSystem();

        // Check if the supported
        return rs->getCapabilities()->hasCapability( RSC_CAN_GET_COMPILED_SHADER_BUFFER );
    }
    //---------------------------------------------------------------------
    void GpuProgramManager::setSaveMicrocodesToCache( const bool val )
    {
        // Check that saving shader microcode is supported
        if( !canGetCompiledShaderBuffer() )
            mSaveMicrocodesToCache = false;
        else
            mSaveMicrocodesToCache = val;
    }
    //---------------------------------------------------------------------
    bool GpuProgramManager::isCacheDirty() const { return mCacheDirty; }
    //---------------------------------------------------------------------
    GpuProgramManager::Hash GpuProgramManager::computeHashWithRenderSystemName( const String &source )
    {
        // Use the current render system
        RenderSystem *rs = Root::getSingleton().getRenderSystem();

        Hash hashVal[2];
        OGRE_HASH128_FUNC( source.c_str(), (int)source.size(), IdString::Seed, &hashVal[0] );
        OGRE_HASH128_FUNC( rs->getName().c_str(), (int)rs->getName().size(), IdString::Seed,
                           &hashVal[1] );

        Hash retVal;
        OGRE_HASH128_FUNC( hashVal, sizeof( hashVal ), IdString::Seed, &retVal );

        return retVal;
    }
    //---------------------------------------------------------------------
    bool GpuProgramManager::isMicrocodeAvailableInCache( const String &source ) const
    {
        return mMicrocodeCache.find( computeHashWithRenderSystemName( source ) ) !=
               mMicrocodeCache.end();
    }
    //---------------------------------------------------------------------
    const GpuProgramManager::Microcode &GpuProgramManager::getMicrocodeFromCache(
        const String &source ) const
    {
        return mMicrocodeCache.find( computeHashWithRenderSystemName( source ) )->second;
    }
    //---------------------------------------------------------------------
    bool GpuProgramManager::getMicrocodeFromCache( const String &source,
                                                   const Microcode **outMicrocode ) const
    {
        const GpuProgramManager::Hash hashKey = computeHashWithRenderSystemName( source );

        ScopedLock lock( mMicrocodeCacheMutex );
        MicrocodeMap ::const_iterator itor = mMicrocodeCache.find( hashKey );

        if( itor != mMicrocodeCache.end() )
        {
            *outMicrocode = &itor->second;
            return true;
        }
        else
        {
            *outMicrocode = nullptr;
            return false;
        }
    }
    //---------------------------------------------------------------------
    GpuProgramManager::Microcode GpuProgramManager::createMicrocode( const uint32 size ) const
    {
        return Microcode( OGRE_NEW MemoryDataStream( size ) );
    }
    //---------------------------------------------------------------------
    void GpuProgramManager::addMicrocodeToCache( const String &source,
                                                 const GpuProgramManager::Microcode &microcode )
    {
        const Hash hashKey = computeHashWithRenderSystemName( source );

        ScopedLock lock( mMicrocodeCacheMutex );
        MicrocodeMap::iterator foundIter = mMicrocodeCache.find( hashKey );

        if( foundIter == mMicrocodeCache.end() )
        {
            mMicrocodeCache.emplace( hashKey, microcode );
        }
        else
        {
            foundIter->second = microcode;
        }

        // if cache is modified, mark it as dirty.
        // We don't check foundIter->second == microcode
        mCacheDirty = true;
    }
    //---------------------------------------------------------------------
    void GpuProgramManager::removeMicrocodeFromCache( const String &source )
    {
        const Hash hashKey = computeHashWithRenderSystemName( source );

        ScopedLock lock( mMicrocodeCacheMutex );
        MicrocodeMap::iterator foundIter = mMicrocodeCache.find( hashKey );

        if( foundIter != mMicrocodeCache.end() )
        {
            mMicrocodeCache.erase( foundIter );
            mCacheDirty = true;
        }
    }
    //---------------------------------------------------------------------
    void GpuProgramManager::saveMicrocodeCache( DataStreamPtr stream ) const
    {
        ScopedLock lock( mMicrocodeCacheMutex );
        if( !mCacheDirty )
            return;

        if( !stream->isWriteable() )
        {
            OGRE_EXCEPT( Exception::ERR_CANNOT_WRITE_TO_FILE,
                         "Unable to write to stream " + stream->getName(),
                         "GpuProgramManager::saveMicrocodeCache" );
        }

        // write the size of the array
        uint32 sizeOfArray = static_cast<uint32>( mMicrocodeCache.size() );
        stream->write( &sizeOfArray, sizeof( uint32 ) );

        // loop the array and save it
        MicrocodeMap::const_iterator iter = mMicrocodeCache.begin();
        MicrocodeMap::const_iterator iterE = mMicrocodeCache.end();
        for( ; iter != iterE; ++iter )
        {
            // saves the hash of the shader
            {
                Hash hash = iter->first;
                stream->write( &hash, sizeof( Hash ) );
            }
            // saves the microcode
            {
                const Microcode &microcodeOfShader = iter->second;
                uint32 microcodeLength = static_cast<uint32>( microcodeOfShader->size() );
                stream->write( &microcodeLength, sizeof( uint32 ) );
                stream->write( microcodeOfShader->getPtr(), microcodeLength );
            }
        }
    }
    //---------------------------------------------------------------------
    void GpuProgramManager::loadMicrocodeCache( DataStreamPtr stream )
    {
        ScopedLock lock( mMicrocodeCacheMutex );
        mMicrocodeCache.clear();

        // write the size of the array
        uint32 sizeOfArray = 0;
        stream->read( &sizeOfArray, sizeof( uint32 ) );

        // loop the array and load it

        for( uint32 i = 0; i < sizeOfArray; i++ )
        {
            Hash shaderHash;
            // loads the hash of the shader
            stream->read( &shaderHash, sizeof( Hash ) );

            // loads the microcode
            uint32 microcodeLength = 0;
            stream->read( &microcodeLength, sizeof( uint32 ) );

            Microcode microcodeOfShader( OGRE_NEW MemoryDataStream( microcodeLength ) );
            microcodeOfShader->seek( 0 );
            stream->read( microcodeOfShader->getPtr(), microcodeLength );

            mMicrocodeCache.emplace( shaderHash, microcodeOfShader );
        }

        // if cache is not modified, mark it as clean.
        mCacheDirty = false;
    }
    //---------------------------------------------------------------------
    void GpuProgramManager::clearMicrocodeCache()
    {
        ScopedLock lock( mMicrocodeCacheMutex );
        mMicrocodeCache.clear();
        mCacheDirty = false;
    }
    //---------------------------------------------------------------------

}  // namespace Ogre

#undef OGRE_HASH128_FUNC
