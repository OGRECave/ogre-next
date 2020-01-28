/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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

#include "LightProfiles/OgreLightProfiles.h"

#include "LightProfiles/OgreIesLoader.h"
#include "OgreHlmsPbs.h"

#include "OgreResourceGroupManager.h"
#include "OgreTextureBox.h"
#include "OgreTextureGpu.h"
#include "OgreTextureGpuManager.h"

namespace Ogre
{
    LightProfiles::LightProfiles( HlmsPbs *hlmsPbs, TextureGpuManager *textureGpuManager ) :
        mLightProfilesTexture( 0 ),
        mHlmsPbs( hlmsPbs ),
        mTextureGpuManager( textureGpuManager )
    {
    }
    //-------------------------------------------------------------------------
    LightProfiles::~LightProfiles()
    {
        destroyTexture();

        FastArray<IesLoader *>::const_iterator itor = mIesData.begin();
        FastArray<IesLoader *>::const_iterator endt = mIesData.end();

        while( itor != endt )
            delete *itor++;
    }
    //-------------------------------------------------------------------------
    void LightProfiles::destroyTexture( void )
    {
        if( !mLightProfilesTexture )
            return;
        mLightProfilesTexture->getTextureManager()->destroyTexture( mLightProfilesTexture );
        mLightProfilesTexture = 0;

        if( mHlmsPbs )
            mHlmsPbs->setLightProfilesTexture( 0 );
    }
    //-------------------------------------------------------------------------
    void LightProfiles::loadIesProfile( const String &filename, const String &resourceGroup,
                                        bool throwOnDuplicate )
    {
        if( mIesNameMap.find( filename ) != mIesNameMap.end() )
        {
            if( throwOnDuplicate )
            {
                OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM, "IES file '" + filename + "' already loaded",
                             "LightProfiles::loadIesProfile" );
            }
            else
            {
                return;
            }
        }

        DataStreamPtr dataStream =
            ResourceGroupManager::getSingleton().openResource( filename, resourceGroup );

        String dataStr = dataStream->getAsString();
        mIesNameMap[filename] = mIesData.size();
        IesLoader *iesLoader = OGRE_NEW IesLoader( filename, dataStr.c_str() );
        mIesData.push_back( iesLoader );
    }
    //-------------------------------------------------------------------------
    /// Return closest power of two not smaller than given number
    static uint32 ClosestPow2( uint32 x )
    {
        if( !( x & ( x - 1u ) ) )
            return x;
        while( x & ( x + 1u ) )
            x |= ( x + 1u );
        return x + 1u;
    }
    void LightProfiles::build( void )
    {
        uint32 suggestedWidth = 0u;

        FastArray<IesLoader *>::const_iterator itor = mIesData.begin();
        FastArray<IesLoader *>::const_iterator endt = mIesData.end();

        while( itor != endt )
        {
            IesLoader *iesLoader = *itor;
            suggestedWidth = std::max( iesLoader->getSuggestedTexWidth(), suggestedWidth );
            ++itor;
        }

        suggestedWidth = ClosestPow2( suggestedWidth );
        suggestedWidth = std::min<uint32>( suggestedWidth, 4096u );

        // We always need one more, as lights without profile will use that one (which is pure white)
        const uint32 texHeight = ClosestPow2( static_cast<uint32>( mIesData.size() + 1u ) );

        if( !mLightProfilesTexture || texHeight > mLightProfilesTexture->getHeight() ||
            suggestedWidth != mLightProfilesTexture->getWidth() )
        {
            destroyTexture();

            mLightProfilesTexture =
                mTextureGpuManager->createTexture( "LightProfilesTexture", GpuPageOutStrategy::Discard,
                                                   TextureFlags::ManualTexture, TextureTypes::Type2D );
            mLightProfilesTexture->setResolution( suggestedWidth, texHeight );
            mLightProfilesTexture->setPixelFormat( PFG_R16_FLOAT );
            mLightProfilesTexture->scheduleTransitionTo( GpuResidency::Resident );

            if( mHlmsPbs )
                mHlmsPbs->setLightProfilesTexture( mLightProfilesTexture );

            Root::getSingleton()._setLightProfilesInvHeight( 1.0f / texHeight );
        }

        Image2 iesImageData;
        iesImageData.createEmptyImageLike( mLightProfilesTexture );

        uint32 row = 0u;
        // Set first row to white, which is used by lights without profile
        for( size_t x = 0u; x < suggestedWidth; ++x )
            iesImageData.setColourAt( ColourValue( 1.0f ), x, row, 0u );

        ++row;
        itor = mIesData.begin();

        while( itor != endt )
        {
            IesLoader *iesLoader = *itor;
            iesLoader->convertToImage1D( iesImageData, row );
            ++row;
            ++itor;
        }

        iesImageData.uploadTo( mLightProfilesTexture, 0u, 0u );
    }
    //-------------------------------------------------------------------------
    void LightProfiles::assignProfile( IdString profileName, Light *light )
    {
        if( profileName == IdString() || profileName == IdString( "" ) )
            light->_setLightProfileIdx( 0u );
        else
        {
            map<IdString, size_t>::type::const_iterator itor = mIesNameMap.find( profileName );
            if( itor == mIesNameMap.end() )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                             "Light Profile '" + profileName.getFriendlyText() +
                                 "' not found. Did you call loadIesProfile?",
                             "LightProfiles::assignProfile" );
            }

            light->_setLightProfileIdx( itor->second + 1u );
        }
    }
    //-------------------------------------------------------------------------
    const IesLoader *LightProfiles::getProfile( IdString profileName ) const
    {
        const IesLoader *retVal = 0;
        map<IdString, size_t>::type::const_iterator itor = mIesNameMap.find( profileName );
        if( itor != mIesNameMap.end() )
            retVal = mIesData[itor->second];
        return retVal;
    }
    //-------------------------------------------------------------------------
    const IesLoader *LightProfiles::getProfile( Light *light ) const
    {
        const IesLoader *retVal = 0;
        const uint16 profileIdx = light->getLightProfileIdx() - 1u;
        if( profileIdx < mIesData.size() )
            retVal = mIesData[profileIdx];
        return retVal;
    }
    //-------------------------------------------------------------------------
    const String &LightProfiles::getProfileName( Light *light ) const
    {
        const IesLoader *iesLoader = getProfile( light );
        if( iesLoader )
            return iesLoader->getName();
        else
            return BLANKSTRING;
    }
}  // namespace Ogre
