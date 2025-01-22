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

#include "OgrePsoCacheHelper.h"

#include "OgreRenderPassDescriptor.h"
#include "OgreRenderSystem.h"

namespace Ogre
{
    // clang-format off
    const uint32 PsoCacheHelper::RenderableBits = 22u;
    const uint32 PsoCacheHelper::PassBits       = 10u;
    const uint32 PsoCacheHelper::RenderableMask = (1u << RenderableBits) - 1u;
    const uint32 PsoCacheHelper::PassMask       = (1u << PassBits) - 1u;
    // clang-format on

    PsoCacheHelper::PsoCacheHelper( RenderSystem *renderSystem ) :
        mPassHashCounter( 0 ),
        mRenderableHashCounter( 0 ),
        mCurrentPassHash( std::numeric_limits<uint32>::max() ),
        mLastFinalHash( std::numeric_limits<uint32>::max() ),
        mLastPso( 0 ),
        mRenderSystem( renderSystem )
    {
        mCurrentState.initialize();
    }
    //-----------------------------------------------------------------------------------
    PsoCacheHelper::~PsoCacheHelper()
    {
        PsoCacheEntryVec::iterator itor = mPsoCache.begin();
        PsoCacheEntryVec::iterator endt = mPsoCache.end();

        while( itor != endt )
        {
            mRenderSystem->_hlmsPipelineStateObjectDestroyed( &itor->pso );
            ++itor;
        }

        mPsoCache.clear();
    }
    //-----------------------------------------------------------------------------------
    void PsoCacheHelper::clearState()
    {
        mCurrentPassHash = std::numeric_limits<uint32>::max();
        mLastFinalHash = std::numeric_limits<uint32>::max();
        mLastPso = 0;
        mCurrentState.initialize();
    }
    //-----------------------------------------------------------------------------------
    uint32 PsoCacheHelper::getPassHash()
    {
        PassCacheEntry entry;
        entry.passKey = mCurrentState.pass;
        PassCacheEntryVec::iterator itor =
            std::lower_bound( mPassCache.begin(), mPassCache.end(), entry );

        if( itor == mPassCache.end() || itor->passKey != entry.passKey )
        {
            entry.hashToMainCache = mPassHashCounter++;
            const ptrdiff_t idx = itor - mPassCache.begin();
            mPassCache.insert( itor, 1u, entry );
            itor = mPassCache.begin() + idx;
        }

        return itor->hashToMainCache;
    }
    //-----------------------------------------------------------------------------------
    uint32 PsoCacheHelper::getRenderableHash()
    {
        RenderableCacheEntry entry;
        entry.psoRenderableKey = mCurrentState;
        RenderableCacheEntryVec::iterator itor =
            std::lower_bound( mRenderableCache.begin(), mRenderableCache.end(), entry );

        if( itor == mRenderableCache.end() ||
            !itor->psoRenderableKey.equalExcludePassData( entry.psoRenderableKey ) )
        {
            entry.hashToMainCache = mRenderableHashCounter++;
            const ptrdiff_t idx = itor - mRenderableCache.begin();
            mRenderableCache.insert( itor, 1u, entry );
            itor = mRenderableCache.begin() + idx;
        }

        return itor->hashToMainCache;
    }
    //-----------------------------------------------------------------------------------
    void PsoCacheHelper::setRenderTarget( const RenderPassDescriptor *renderPassDesc )
    {
        mLastFinalHash = std::numeric_limits<uint32>::max();
        mLastPso = 0;
        mCurrentState.pass.stencilParams = mRenderSystem->getStencilBufferParams();

        for( int i = 0; i < OGRE_MAX_MULTIPLE_RENDER_TARGETS; ++i )
        {
            if( renderPassDesc->mColour[i].texture )
            {
                mCurrentState.pass.colourFormat[i] =
                    renderPassDesc->mColour[i].texture->getPixelFormat();
                mCurrentState.pass.sampleDescription =
                    renderPassDesc->mColour[i].texture->getSampleDescription();
            }
            else
                mCurrentState.pass.colourFormat[i] = PFG_NULL;
        }

        mCurrentState.pass.depthFormat = PFG_NULL;
        if( renderPassDesc->mDepth.texture )
        {
            mCurrentState.pass.depthFormat = renderPassDesc->mDepth.texture->getPixelFormat();
            mCurrentState.pass.sampleDescription =
                renderPassDesc->mDepth.texture->getSampleDescription();
        }

        mCurrentState.pass.adapterId = 1;       // TODO: Ask RenderSystem current adapter ID.
        mCurrentState.sampleMask = 0xffffffff;  // TODO

        mCurrentPassHash = getPassHash();
    }
    //-----------------------------------------------------------------------------------
    void PsoCacheHelper::setVertexFormat( const VertexElement2VecVec &vertexElements,
                                          OperationType operationType, bool enablePrimitiveRestart )
    {
        if( mCurrentState.operationType != operationType )
        {
            mCurrentState.operationType = operationType;
            mLastFinalHash = std::numeric_limits<uint32>::max();
            mLastPso = 0;
        }
        if( mCurrentState.enablePrimitiveRestart != enablePrimitiveRestart )
        {
            mCurrentState.enablePrimitiveRestart = enablePrimitiveRestart;
            mLastFinalHash = std::numeric_limits<uint32>::max();
            mLastPso = 0;
        }
        if( !mLastPso || mCurrentState.vertexElements != vertexElements )
        {
            mCurrentState.vertexElements = vertexElements;
            mLastFinalHash = std::numeric_limits<uint32>::max();
            mLastPso = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void PsoCacheHelper::setMacroblock( const HlmsMacroblock *macroblock )
    {
        if( macroblock != mCurrentState.macroblock )
        {
            mCurrentState.macroblock = macroblock;
            mLastFinalHash = std::numeric_limits<uint32>::max();
            mLastPso = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void PsoCacheHelper::setBlendblock( const HlmsBlendblock *blendblock )
    {
        if( blendblock != mCurrentState.blendblock )
        {
            mCurrentState.blendblock = blendblock;
            mLastFinalHash = std::numeric_limits<uint32>::max();
            mLastPso = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void PsoCacheHelper::setVertexShader( GpuProgramPtr &shader )
    {
        if( mCurrentState.vertexShader != shader )
        {
            mCurrentState.vertexShader = shader;
            mLastFinalHash = std::numeric_limits<uint32>::max();
            mLastPso = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void PsoCacheHelper::setPixelShader( GpuProgramPtr &shader )
    {
        if( mCurrentState.pixelShader != shader )
        {
            mCurrentState.pixelShader = shader;
            mLastFinalHash = std::numeric_limits<uint32>::max();
            mLastPso = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    HlmsPso *PsoCacheHelper::getPso( uint32 renderableHash, bool renderableCacheAlreadySet )
    {
        assert( mCurrentPassHash != std::numeric_limits<uint32>::max() &&
                "You must have called setRenderTarget first!!!" );

        const uint32 finalHash =
            ( ( mCurrentPassHash & PassMask ) << RenderableBits ) | ( renderableHash & RenderableMask );

        if( mLastFinalHash != finalHash )
        {
            PsoCacheEntryVec::iterator itor =
                std::lower_bound( mPsoCache.begin(), mPsoCache.end(), finalHash, PsoCacheEntry() );

            if( itor == mPsoCache.end() || itor->hash != finalHash )
            {
                if( !renderableCacheAlreadySet )
                {
                    RenderableCacheEntryVec::const_iterator it = mRenderableCache.begin();
                    RenderableCacheEntryVec::const_iterator en = mRenderableCache.end();

                    while( it != en && it->hashToMainCache != renderableHash )
                        ++it;

                    assert( it != en &&
                            "Bad / corrupted renderableHash!!! "
                            "Maybe it belongs to a different PsoCacheHelper?" );

                    HlmsPassPso savedPassPart = mCurrentState.pass;
                    mCurrentState = it->psoRenderableKey;
                    mCurrentState.pass = savedPassPart;
                }

                // Create the PSO
                PsoCacheEntry entry;
                entry.hash = finalHash;
                entry.pso = mCurrentState;
                const ptrdiff_t idx = itor - mPsoCache.begin();
                mPsoCache.insert( itor, 1u, entry );
                itor = mPsoCache.begin() + idx;

                mRenderSystem->_hlmsPipelineStateObjectCreated( &itor->pso, (uint64)-1 );
            }

            mLastFinalHash = finalHash;
            mLastPso = &itor->pso;
        }

        return mLastPso;
    }
    //-----------------------------------------------------------------------------------
    HlmsPso *PsoCacheHelper::getPso()
    {
        if( !mLastPso )
        {
            const uint32 renderableHash = getRenderableHash();
            mLastPso = getPso( renderableHash, true );
        }

        return mLastPso;
    }
}  // namespace Ogre
