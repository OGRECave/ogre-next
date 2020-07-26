/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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

#include "OgreTextureUnitState.h"
#include "OgrePass.h"
#include "OgreMaterialManager.h"
#include "OgreControllerManager.h"
#include "OgreLogManager.h"
#include "OgreException.h"
#include "OgreRoot.h"
#include "OgreHlmsManager.h"
#include "OgreHlms.h"
#include "OgreTextureGpuManager.h"
#include "OgrePixelFormatGpuUtils.h"

namespace Ogre {

    //-----------------------------------------------------------------------
    TextureUnitState::TextureUnitState(Pass* parent)
        : mCurrentFrame(0)
        , mAnimDuration(0)
        , mCubic(false)
        , mAutomaticBatching(false)
        , mTextureType(TextureTypes::Type2D)
        , mTextureSrcMipmaps(1u)
        , mTextureCoordSetIndex(0)
        , mSamplerblock(0)
        , mTextureLoadFailed(false)
        , mIsAlpha(false)
        , mHwGamma(false)
        , mGamma(1)
        , mRecalcTexMatrix(false)
        , mUMod(0)
        , mVMod(0)
        , mUScale(1)
        , mVScale(1)
        , mRotate(0)
        , mTexModMatrix(Matrix4::IDENTITY)
        , mBindingType(BT_FRAGMENT)
        , mContentType(CONTENT_NAMED)
        , mParent(parent)
        , mAnimController(0)
    {
        mColourBlendMode.blendType = LBT_COLOUR;
        mAlphaBlendMode.operation = LBX_MODULATE;
        mAlphaBlendMode.blendType = LBT_ALPHA;
        mAlphaBlendMode.source1 = LBS_TEXTURE;
        mAlphaBlendMode.source2 = LBS_CURRENT;
        setColourOperation(LBO_MODULATE);

        HlmsManager *hlmsManager = parent->_getDatablock()->getCreator()->getHlmsManager();
        HlmsSamplerblock samplerblock;
        samplerblock.setAddressingMode( TAM_WRAP );
        mSamplerblock = hlmsManager->getSamplerblock( samplerblock );
    }
    //-----------------------------------------------------------------------
    TextureUnitState::TextureUnitState(Pass* parent, const TextureUnitState& oth )
    {
        mParent = parent;
        mAnimController = 0;
        *this = oth;
    }

    //-----------------------------------------------------------------------
    TextureUnitState::TextureUnitState( Pass* parent, const String& texName, unsigned int texCoordSet)
        : mCurrentFrame(0)
        , mAnimDuration(0)
        , mCubic(false)
        , mTextureType(TextureTypes::Type2D)
        , mTextureSrcMipmaps(1u)
        , mTextureCoordSetIndex(0)
        , mTextureLoadFailed(false)
        , mIsAlpha(false)
        , mHwGamma(false)
        , mGamma(1)
        , mRecalcTexMatrix(false)
        , mUMod(0)
        , mVMod(0)
        , mUScale(1)
        , mVScale(1)
        , mRotate(0)
        , mTexModMatrix(Matrix4::IDENTITY)
        , mBindingType(BT_FRAGMENT)
        , mContentType(CONTENT_NAMED)
        , mParent(parent)
        , mAnimController(0)
    {
        mColourBlendMode.blendType = LBT_COLOUR;
        mAlphaBlendMode.operation = LBX_MODULATE;
        mAlphaBlendMode.blendType = LBT_ALPHA;
        mAlphaBlendMode.source1 = LBS_TEXTURE;
        mAlphaBlendMode.source2 = LBS_CURRENT;
        setColourOperation(LBO_MODULATE);

        setTextureName(texName);
        setTextureCoordSet(texCoordSet);

        HlmsManager *hlmsManager = parent->_getDatablock()->getCreator()->getHlmsManager();
        HlmsSamplerblock samplerblock;
        samplerblock.setAddressingMode( TAM_WRAP );
        mSamplerblock = hlmsManager->getSamplerblock( samplerblock );
    }
    //-----------------------------------------------------------------------
    TextureUnitState::~TextureUnitState()
    {
        // Unload ensure all controllers destroyed
        _unload();

        HlmsManager *hlmsManager = mParent->_getDatablock()->getCreator()->getHlmsManager();
        hlmsManager->destroySamplerblock( mSamplerblock );
        mSamplerblock = 0;
    }
    //-----------------------------------------------------------------------
    TextureUnitState & TextureUnitState::operator = ( 
        const TextureUnitState &oth )
    {
        assert(mAnimController == 0);
        removeAllEffects();

        // copy basic members (int's, real's)
        memcpy( this, &oth, (const uchar *)(&oth.mFrames) - (const uchar *)(&oth) );
        // copy complex members
        mFrames  = oth.mFrames;
        mFramePtrs = oth.mFramePtrs;
        mName    = oth.mName;
        mEffects = oth.mEffects;

        {
            vector<TextureGpu*>::type::iterator itor = mFramePtrs.begin();
            vector<TextureGpu*>::type::iterator end  = mFramePtrs.end();
            while( itor != end )
            {
                if( *itor )
                    (*itor)->addListener( this );
                ++itor;
            }
        }

        mSamplerblock = oth.mSamplerblock;
        HlmsManager *hlmsManager = oth.mParent->_getDatablock()->getCreator()->getHlmsManager();
        hlmsManager->addReference( mSamplerblock );

        mTextureNameAlias = oth.mTextureNameAlias;
        mCompositorRefTexName = oth.mCompositorRefTexName;
        // Can't sharing controllers with other TUS, reset to null to avoid potential bug.
        for (EffectMap::iterator j = mEffects.begin(); j != mEffects.end(); ++j)
        {
            j->second.controller = 0;
        }

        // Load immediately if Material loaded
        if (isLoaded())
        {
            _load();
        }

        return *this;
    }
    //-----------------------------------------------------------------------
    const String& TextureUnitState::getTextureName(void) const
    {
        // Return name of current frame
        if (mCurrentFrame < mFrames.size())
            return mFrames[mCurrentFrame];
        else
            return BLANKSTRING;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureName( const String& name, TextureTypes::TextureTypes texType )
    {
        setContentType(CONTENT_NAMED);
        mTextureLoadFailed = false;

        if (texType == TextureTypes::TypeCube)
        {
            // delegate to cubic texture implementation
            setCubicTextureName(name, true);
        }
        else
        {
            cleanFramePtrs();
            mFrames.resize(1);
            mFramePtrs.resize(1);
            mFrames[0] = name;
            mFramePtrs[0] = 0;
            // defer load until used, so don't grab pointer yet
            mCurrentFrame = 0;
            mCubic = false;
            mTextureType = texType;
            if (name.empty())
            {
                return;
            }

            
            // Load immediately ?
            if (isLoaded())
            {
                _load(); // reload
            }
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTexture( TextureGpu *texPtr )
    {
        if( !texPtr )
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
                "Texture Pointer is empty.",
                "TextureUnitState::setTexture");
        }

        setContentType(CONTENT_NAMED);
        mTextureLoadFailed = false;

        if (texPtr->getTextureType() == TextureTypes::TypeCube)
        {
            // delegate to cubic texture implementation
            setCubicTexture(&texPtr, true);
        }
        else
        {
            cleanFramePtrs();
            mFrames.resize(1);
            mFramePtrs.resize(1);
            mFrames[0] = texPtr->getNameStr();
            mFramePtrs[0] = texPtr;
            texPtr->addListener( this );
            // defer load until used, so don't grab pointer yet
            mCurrentFrame = 0;
            mCubic = false;
            mTextureType = texPtr->getTextureType();

            // Load immediately ?
            if (isLoaded())
            {
                _load(); // reload
            }
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setBindingType(TextureUnitState::BindingType bt)
    {
        mBindingType = bt;

    }
    //-----------------------------------------------------------------------
    TextureUnitState::BindingType TextureUnitState::getBindingType(void) const
    {
        return mBindingType;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setAutomaticBatching( bool automaticBatching )
    {
        mAutomaticBatching = automaticBatching;
    }
    //-----------------------------------------------------------------------
    bool TextureUnitState::getAutomaticBatching(void) const
    {
        return mAutomaticBatching;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setContentType(TextureUnitState::ContentType ct)
    {
        if( mContentType != ct && mParent )
        {
            if( mContentType == CONTENT_SHADOW )
                mParent->removeShadowContentTypeLookup( mParent->getTextureUnitStateIndex( this ) );
            else if( ct == CONTENT_SHADOW )
                mParent->insertShadowContentTypeLookup( mParent->getTextureUnitStateIndex( this ) );
        }

        mContentType = ct;
        if (ct == CONTENT_SHADOW || ct == CONTENT_COMPOSITOR)
        {
            cleanFramePtrs();
            // Clear out texture frames, not applicable
            mFrames.clear();
            // One reference space, set manually through _setTexturePtr
            mFramePtrs.resize(1);
            mFramePtrs[0] = 0;
        }
    }
    //-----------------------------------------------------------------------
    TextureUnitState::ContentType TextureUnitState::getContentType(void) const
    {
        return mContentType;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setCubicTextureName( const String& name, bool forUVW)
    {
        if (forUVW)
        {
            setCubicTextureName(&name, forUVW);
        }
        else
        {
            setContentType(CONTENT_NAMED);
            mTextureLoadFailed = false;
            String ext;
            String suffixes[6] = {"_fr", "_bk", "_lf", "_rt", "_up", "_dn"};
            String baseName;
            String fullNames[6];

            size_t pos = name.find_last_of(".");
            if( pos != String::npos )
            {
                baseName = name.substr(0, pos);
                ext = name.substr(pos);
            }
            else
                baseName = name;

            for (int i = 0; i < 6; ++i)
            {
                fullNames[i] = baseName + suffixes[i] + ext;
            }

            setCubicTextureName(fullNames, forUVW);
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setCubicTextureName(const String* const names, bool forUVW)
    {
        setContentType(CONTENT_NAMED);
        mTextureLoadFailed = false;
        cleanFramePtrs();
        mFrames.resize(forUVW ? 1 : 6);
        // resize pointers, but don't populate until asked for
        mFramePtrs.resize(forUVW ? 1 : 6);
        mAnimDuration = 0;
        mCurrentFrame = 0;
        mCubic = true;
        mTextureType = forUVW ? TextureTypes::TypeCube : TextureTypes::Type2D;

        for (unsigned int i = 0; i < mFrames.size(); ++i)
        {
            mFrames[i] = names[i];
            mFramePtrs[i] = 0;
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setCubicTexture( TextureGpu * const *texPtrs, bool forUVW )
    {
        setContentType(CONTENT_NAMED);
        mTextureLoadFailed = false;
        cleanFramePtrs();
        mFrames.resize(forUVW ? 1 : 6);
        // resize pointers, but don't populate until asked for
        mFramePtrs.resize(forUVW ? 1 : 6);
        mAnimDuration = 0;
        mCurrentFrame = 0;
        mCubic = true;
        mTextureType = forUVW ? TextureTypes::TypeCube : TextureTypes::Type2D;

        for (unsigned int i = 0; i < mFrames.size(); ++i)
        {
            mFrames[i] = texPtrs[i]->getNameStr();
            mFramePtrs[i] = texPtrs[i];
            if( mFramePtrs[i] )
                mFramePtrs[i]->addListener( this );
        }
    }
    //-----------------------------------------------------------------------
    bool TextureUnitState::isCubic(void) const
    {
        return mCubic;
    }
    //-----------------------------------------------------------------------
    bool TextureUnitState::is3D(void) const
    {
        return mTextureType == TextureTypes::TypeCube;
    }
    //-----------------------------------------------------------------------
    TextureTypes::TextureTypes TextureUnitState::getTextureType(void) const
    {
        return mTextureType;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setFrameTextureName(const String& name, unsigned int frameNumber)
    {
        mTextureLoadFailed = false;
        if (frameNumber < mFrames.size())
        {
            cleanFramePtrs();
            mFrames[frameNumber] = name;
            // reset pointer (don't populate until requested)
            mFramePtrs[frameNumber] = 0;

            if (isLoaded())
            {
                _load(); // reload
            }
        }
        else // raise exception for frameNumber out of bounds
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "frameNumber parameter value exceeds number of stored frames.",
                "TextureUnitState::setFrameTextureName");
        }
    }

    //-----------------------------------------------------------------------
    void TextureUnitState::addFrameTextureName(const String& name)
    {
        setContentType(CONTENT_NAMED);
        mTextureLoadFailed = false;

        mFrames.push_back(name);
        // Add blank pointer, load on demand
        mFramePtrs.push_back( 0 );

        // Load immediately if Material loaded
        if (isLoaded())
        {
            _load();
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::deleteFrameTextureName(const size_t frameNumber)
    {
        mTextureLoadFailed = false;
        if (frameNumber < mFrames.size())
        {
            if( mFramePtrs[frameNumber] )
                mFramePtrs[frameNumber]->removeListener( this );

            mFrames.erase(mFrames.begin() + frameNumber);
            mFramePtrs.erase(mFramePtrs.begin() + frameNumber);

            if (isLoaded())
            {
                _load();
            }
        }
        else
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "frameNumber parameter value exceeds number of stored frames.",
                "TextureUnitState::deleteFrameTextureName");
        }
    }

    //-----------------------------------------------------------------------
    void TextureUnitState::setAnimatedTextureName( const String& name, unsigned int numFrames, Real duration)
    {
        setContentType(CONTENT_NAMED);
        mTextureLoadFailed = false;

        String ext;
        String baseName;

        size_t pos = name.find_last_of(".");
        baseName = name.substr(0, pos);
        ext = name.substr(pos);

        cleanFramePtrs();
        mFrames.resize(numFrames);
        // resize pointers, but don't populate until needed
        mFramePtrs.resize(numFrames);
        mAnimDuration = duration;
        mCurrentFrame = 0;
        mCubic = false;

        for (unsigned int i = 0; i < mFrames.size(); ++i)
        {
            mFrames[i] = baseName + "_" + StringConverter::toString( i ) + ext;
            mFramePtrs[i] = 0;
        }

        // Load immediately if Material loaded
        if (isLoaded())
        {
            _load();
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setAnimatedTextureName(const String* const names, unsigned int numFrames, Real duration)
    {
        setContentType(CONTENT_NAMED);
        mTextureLoadFailed = false;

        cleanFramePtrs();
        mFrames.resize(numFrames);
        // resize pointers, but don't populate until needed
        mFramePtrs.resize(numFrames);
        mAnimDuration = duration;
        mCurrentFrame = 0;
        mCubic = false;

        for (unsigned int i = 0; i < mFrames.size(); ++i)
        {
            mFrames[i] = names[i];
            mFramePtrs[i] = 0;
        }

        // Load immediately if Material loaded
        if (isLoaded())
        {
            _load();
        }
    }
    //-----------------------------------------------------------------------
    std::pair< size_t, size_t > TextureUnitState::getTextureDimensions( unsigned int frame ) const
    {
        
        TextureGpu *tex = _getTexturePtr(frame);
        if( !tex )
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Could not find texture " + mFrames[ frame ],
            "TextureUnitState::getTextureDimensions" );

        return std::pair< size_t, size_t >( tex->getWidth(), tex->getHeight() );
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setCurrentFrame(unsigned int frameNumber)
    {
        if (frameNumber < mFrames.size())
        {
            mCurrentFrame = frameNumber;
        }
        else
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "frameNumber parameter value exceeds number of stored frames.",
                "TextureUnitState::setCurrentFrame");
        }

    }
    //-----------------------------------------------------------------------
    unsigned int TextureUnitState::getCurrentFrame(void) const
    {
        return mCurrentFrame;
    }
    //-----------------------------------------------------------------------
    unsigned int TextureUnitState::getNumFrames(void) const
    {
        return (unsigned int)mFrames.size();
    }
    //-----------------------------------------------------------------------
    const String& TextureUnitState::getFrameTextureName(unsigned int frameNumber) const
    {
        if (frameNumber >= mFrames.size())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "frameNumber parameter value exceeds number of stored frames.",
                "TextureUnitState::getFrameTextureName");
        }

        return mFrames[frameNumber];
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setNumMipmaps(int numMipmaps)
    {
        mTextureSrcMipmaps = numMipmaps;
    }
    //-----------------------------------------------------------------------
    int TextureUnitState::getNumMipmaps(void) const
    {
        return mTextureSrcMipmaps;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setIsAlpha(bool isAlpha)
    {
        mIsAlpha = isAlpha;
    }
    //-----------------------------------------------------------------------
    bool TextureUnitState::getIsAlpha(void) const
    {
        return mIsAlpha;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setHardwareGammaEnabled(bool g)
    {
        mHwGamma = g;
    }
    //-----------------------------------------------------------------------
    bool TextureUnitState::isHardwareGammaEnabled() const
    {
        return mHwGamma;
    }
    //-----------------------------------------------------------------------
    unsigned int TextureUnitState::getTextureCoordSet(void) const
    {
        return mTextureCoordSetIndex;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureCoordSet(unsigned int set)
    {
        mTextureCoordSetIndex = set;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setColourOperationEx(LayerBlendOperationEx op,
        LayerBlendSource source1,
        LayerBlendSource source2,
        const ColourValue& arg1,
        const ColourValue& arg2,
        Real manualBlend)
    {
        mColourBlendMode.operation = op;
        mColourBlendMode.source1 = source1;
        mColourBlendMode.source2 = source2;
        mColourBlendMode.colourArg1 = arg1;
        mColourBlendMode.colourArg2 = arg2;
        mColourBlendMode.factor = manualBlend;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setColourOperation(LayerBlendOperation op)
    {
        // Set up the multitexture and multipass blending operations
        switch (op)
        {
        case LBO_REPLACE:
            setColourOperationEx(LBX_SOURCE1, LBS_TEXTURE, LBS_CURRENT);
            setColourOpMultipassFallback(SBF_ONE, SBF_ZERO);
            break;
        case LBO_ADD:
            setColourOperationEx(LBX_ADD, LBS_TEXTURE, LBS_CURRENT);
            setColourOpMultipassFallback(SBF_ONE, SBF_ONE);
            break;
        case LBO_MODULATE:
            setColourOperationEx(LBX_MODULATE, LBS_TEXTURE, LBS_CURRENT);
            setColourOpMultipassFallback(SBF_DEST_COLOUR, SBF_ZERO);
            break;
        case LBO_ALPHA_BLEND:
            setColourOperationEx(LBX_BLEND_TEXTURE_ALPHA, LBS_TEXTURE, LBS_CURRENT);
            setColourOpMultipassFallback(SBF_SOURCE_ALPHA, SBF_ONE_MINUS_SOURCE_ALPHA);
            break;
        }


    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setColourOpMultipassFallback(SceneBlendFactor sourceFactor, SceneBlendFactor destFactor)
    {
        mColourBlendFallbackSrc = sourceFactor;
        mColourBlendFallbackDest = destFactor;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setAlphaOperation(LayerBlendOperationEx op,
        LayerBlendSource source1,
        LayerBlendSource source2,
        Real arg1,
        Real arg2,
        Real manualBlend)
    {
        mAlphaBlendMode.operation = op;
        mAlphaBlendMode.source1 = source1;
        mAlphaBlendMode.source2 = source2;
        mAlphaBlendMode.alphaArg1 = arg1;
        mAlphaBlendMode.alphaArg2 = arg2;
        mAlphaBlendMode.factor = manualBlend;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::addEffect(TextureEffect& effect)
    {
        // Ensure controller pointer is null
        effect.controller = 0;

        if (effect.type == ET_ENVIRONMENT_MAP 
            || effect.type == ET_UVSCROLL
            || effect.type == ET_USCROLL
            || effect.type == ET_VSCROLL
            || effect.type == ET_ROTATE
            || effect.type == ET_PROJECTIVE_TEXTURE)
        {
            // Replace - must be unique
            // Search for existing effect of this type
            EffectMap::iterator i = mEffects.find(effect.type);
            if (i != mEffects.end())
            {
                // Destroy old effect controller if exist
                if (i->second.controller)
                {
                    ControllerManager::getSingleton().destroyController(i->second.controller);
                }

                mEffects.erase(i);
            }
        }

        if (isLoaded())
        {
            // Create controller
            createEffectController(effect);
        }

        // Record new effect
        mEffects.insert(EffectMap::value_type(effect.type, effect));

    }
    //-----------------------------------------------------------------------
    void TextureUnitState::removeAllEffects(void)
    {
        // Iterate over effects to remove controllers
        EffectMap::iterator i, iend;
        iend = mEffects.end();
        for (i = mEffects.begin(); i != iend; ++i)
        {
            if (i->second.controller)
            {
                ControllerManager::getSingleton().destroyController(i->second.controller);
            }
        }

        mEffects.clear();
    }

    //-----------------------------------------------------------------------
    bool TextureUnitState::isBlank(void) const
    {
        if (mFrames.empty())
            return true;
        else
            return mFrames[0].empty() || mTextureLoadFailed;
    }

    //-----------------------------------------------------------------------
    SceneBlendFactor TextureUnitState::getColourBlendFallbackSrc(void) const
    {
        return mColourBlendFallbackSrc;
    }
    //-----------------------------------------------------------------------
    SceneBlendFactor TextureUnitState::getColourBlendFallbackDest(void) const
    {
        return mColourBlendFallbackDest;
    }
    //-----------------------------------------------------------------------
    const LayerBlendModeEx& TextureUnitState::getColourBlendMode(void) const
    {
        return mColourBlendMode;
    }
    //-----------------------------------------------------------------------
    const LayerBlendModeEx& TextureUnitState::getAlphaBlendMode(void) const
    {
        return mAlphaBlendMode;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setEnvironmentMap(bool enable, EnvMapType envMapType)
    {
        if (enable)
        {
            TextureEffect eff;
            eff.type = ET_ENVIRONMENT_MAP;

            eff.subtype = envMapType;
            addEffect(eff);
        }
        else
        {
            removeEffect(ET_ENVIRONMENT_MAP);
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::removeEffect(TextureEffectType type)
    {
        // Get range of items matching this effect
        std::pair< EffectMap::iterator, EffectMap::iterator > remPair = 
            mEffects.equal_range( type );
        // Remove controllers
        for (EffectMap::iterator i = remPair.first; i != remPair.second; ++i)
        {
            if (i->second.controller)
            {
                ControllerManager::getSingleton().destroyController(i->second.controller);
            }
        }
        // Erase         
        mEffects.erase( remPair.first, remPair.second );
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setBlank(void)
    {
        setTextureName(BLANKSTRING);
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureTransform(const Matrix4& xform)
    {
        mTexModMatrix = xform;
        mRecalcTexMatrix = false;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureScroll(Real u, Real v)
    {
        mUMod = u;
        mVMod = v;
        mRecalcTexMatrix = true;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureScale(Real uScale, Real vScale)
    {
        mUScale = uScale;
        mVScale = vScale;
        mRecalcTexMatrix = true;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureRotate(const Radian& angle)
    {
        mRotate = angle;
        mRecalcTexMatrix = true;
    }
    //-----------------------------------------------------------------------
    const Matrix4& TextureUnitState::getTextureTransform() const
    {
        if (mRecalcTexMatrix)
            recalcTextureMatrix();
        return mTexModMatrix;

    }
    //-----------------------------------------------------------------------
    void TextureUnitState::recalcTextureMatrix() const
    {
        // Assumption: 2D texture coords
        Matrix4 xform;

        xform = Matrix4::IDENTITY;
        if (mUScale != 1 || mVScale != 1)
        {
            // Offset to center of texture
            xform[0][0] = 1/mUScale;
            xform[1][1] = 1/mVScale;
            // Skip matrix concat since first matrix update
            xform[0][3] = (-0.5f * xform[0][0]) + 0.5f;
            xform[1][3] = (-0.5f * xform[1][1]) + 0.5f;
        }

        if (mUMod || mVMod)
        {
            Matrix4 xlate = Matrix4::IDENTITY;

            xlate[0][3] = mUMod;
            xlate[1][3] = mVMod;

            xform = xlate * xform;
        }

        if (mRotate != Radian(0))
        {
            Matrix4 rot = Matrix4::IDENTITY;
            Radian theta ( mRotate );
            Real cosTheta = Math::Cos(theta);
            Real sinTheta = Math::Sin(theta);

            rot[0][0] = cosTheta;
            rot[0][1] = -sinTheta;
            rot[1][0] = sinTheta;
            rot[1][1] = cosTheta;
            // Offset center of rotation to center of texture
            rot[0][3] = 0.5f + ( (-0.5f * cosTheta) - (-0.5f * sinTheta) );
            rot[1][3] = 0.5f + ( (-0.5f * sinTheta) + (-0.5f * cosTheta) );

            xform = rot * xform;
        }

        mTexModMatrix = xform;
        mRecalcTexMatrix = false;

    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureUScroll(Real value)
    {
        mUMod = value;
        mRecalcTexMatrix = true;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureVScroll(Real value)
    {
        mVMod = value;
        mRecalcTexMatrix = true;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureUScale(Real value)
    {
        mUScale = value;
        mRecalcTexMatrix = true;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureVScale(Real value)
    {
        mVScale = value;
        mRecalcTexMatrix = true;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setScrollAnimation(Real uSpeed, Real vSpeed)
    {
        // Remove existing effects
        removeEffect(ET_UVSCROLL);
        removeEffect(ET_USCROLL);
        removeEffect(ET_VSCROLL);

        // don't create an effect if the speeds are both 0
        if(uSpeed == 0.0f && vSpeed == 0.0f) 
        {
          return;
        }

        // Create new effect
        TextureEffect eff;
    if(uSpeed == vSpeed) 
    {
        eff.type = ET_UVSCROLL;
        eff.arg1 = uSpeed;
        addEffect(eff);
    }
    else
    {
        if(uSpeed)
        {
            eff.type = ET_USCROLL;
        eff.arg1 = uSpeed;
        addEffect(eff);
    }
        if(vSpeed)
        {
            eff.type = ET_VSCROLL;
            eff.arg1 = vSpeed;
            addEffect(eff);
        }
    }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setRotateAnimation(Real speed)
    {
        // Remove existing effect
        removeEffect(ET_ROTATE);
        // don't create an effect if the speed is 0
        if(speed == 0.0f) 
        {
          return;
        }
        // Create new effect
        TextureEffect eff;
        eff.type = ET_ROTATE;
        eff.arg1 = speed;
        addEffect(eff);
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTransformAnimation(TextureTransformType ttype,
        WaveformType waveType, Real base, Real frequency, Real phase, Real amplitude)
    {
        // Remove existing effect
        // note, only remove for subtype, not entire ET_TRANSFORM
        // otherwise we won't be able to combine subtypes
        // Get range of items matching this effect
        for (EffectMap::iterator i = mEffects.begin(); i != mEffects.end(); ++i)
        {
            if (i->second.type == ET_TRANSFORM && i->second.subtype == ttype)
            {
                if (i->second.controller)
                {
                    ControllerManager::getSingleton().destroyController(i->second.controller);
                }
                mEffects.erase(i);

                // should only be one, so jump out
                break;
            }
        }

    // don't create an effect if the given values are all 0
    if(base == 0.0f && phase == 0.0f && frequency == 0.0f && amplitude == 0.0f) 
    {
      return;
    }
        // Create new effect
        TextureEffect eff;
        eff.type = ET_TRANSFORM;
        eff.subtype = ttype;
        eff.waveType = waveType;
        eff.base = base;
        eff.frequency = frequency;
        eff.phase = phase;
        eff.amplitude = amplitude;
        addEffect(eff);
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::_prepare(void)
    {
        // Unload first
        //_unload();

        // Load textures
        for (unsigned int i = 0; i < mFrames.size(); ++i)
        {
            ensurePrepared(i);
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::_load(void)
    {

        // Load textures
        for (unsigned int i = 0; i < mFrames.size(); ++i)
        {
            ensureLoaded(i);
        }
        // Animation controller
        if (mAnimDuration != 0)
        {
            createAnimController();
        }
        // Effect controllers
        for (EffectMap::iterator it = mEffects.begin(); it != mEffects.end(); ++it)
        {
            createEffectController(it->second);
        }

    }
    //-----------------------------------------------------------------------
    TextureGpu* TextureUnitState::_getTexturePtr(void) const
    {
        return _getTexturePtr(mCurrentFrame);
    }
    //-----------------------------------------------------------------------
    TextureGpu* TextureUnitState::_getTexturePtr(size_t frame) const
    {
        if (mContentType == CONTENT_NAMED)
        {
            if (frame < mFrames.size() && !mTextureLoadFailed)
            {
                ensureLoaded(frame);
                return mFramePtrs[frame];
            }
            else
            {
                // Silent fail with empty texture for internal method
                return 0;
            }
        }
        else
        {
            // Manually bound texture, no name or loading
            assert(frame < mFramePtrs.size());
            return mFramePtrs[frame];
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::_setTexturePtr( TextureGpu *texptr )
    {
        _setTexturePtr(texptr, mCurrentFrame);
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::_setTexturePtr( TextureGpu *texptr, size_t frame )
    {
        assert(frame < mFramePtrs.size());

        if (mFramePtrs[frame] != texptr)
        {
            if (mFramePtrs[frame])
            {
                mFramePtrs[frame]->removeListener(this);
            }

            mFramePtrs[frame] = texptr;

            mFramePtrs[frame]->addListener(this);
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::ensurePrepared(size_t frame) const
    {
        if (!mFrames[frame].empty() && !mTextureLoadFailed)
        {
            // Ensure texture is loaded, specified number of mipmaps and
            // priority
            if( !mFramePtrs[frame] )
            {
                try
                {
                    uint32 textureFlags = 0;
                    String aliasName = mFrames[frame];
                    if( mTextureType == TextureTypes::Type2D && mAutomaticBatching )
                        textureFlags |= TextureFlags::AutomaticBatching;
                    else
                        aliasName += "/V1_Material";

                    if( mHwGamma )
                        textureFlags |= TextureFlags::PrefersLoadingFromFileAsSRGB;
                    TextureGpuManager *textureManager = Root::getSingleton().
                                                        getRenderSystem()->getTextureGpuManager();

                    //First check if we need the alias: there could be a texture
                    //with the original name that satisfies our needs
                    TextureGpu *texture = textureManager->findTextureNoThrow( mFrames[frame] );

                    if( texture && texture->hasAutomaticBatching() == mAutomaticBatching )
                        mFramePtrs[frame] = texture;
                    else
                    {
                        mFramePtrs[frame] =
                                textureManager->createOrRetrieveTexture( mFrames[frame], aliasName,
                                                                         GpuPageOutStrategy::Discard,
                                                                         textureFlags,
                                                                         mTextureType,
                                                                         mParent->getResourceGroup() );
                    }

                    //You think this is ugly? So do I.
                    //But I'm not the #@!# who made "ensurePrepared" const
                    mFramePtrs[frame]->addListener( const_cast<TextureUnitState*>( this ) );
                }
                catch (Exception &e)
                {
                    String msg;
                    msg = msg + "Error loading texture " + mFrames[frame]  +
                          ". Texture layer will be blank. Loading the texture "
                          "failed with the following exception: "
                          + e.getFullDescription();
                    LogManager::getSingleton().logMessage( msg, LML_CRITICAL );
                    mTextureLoadFailed = true;
                }   
            }
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::ensureLoaded(size_t frame) const
    {
        if (!mFrames[frame].empty() && !mTextureLoadFailed)
        {
            // Ensure texture is loaded, specified number of mipmaps and
            // priority
            if( !mFramePtrs[frame] )
            {
                try
                {
                    uint32 textureFlags = 0;
                    String aliasName = mFrames[frame];
                    if( mTextureType == TextureTypes::Type2D && mAutomaticBatching )
                        textureFlags |= TextureFlags::AutomaticBatching;
                    else
                        aliasName += "/V1_Material";

                    if( mHwGamma )
                        textureFlags |= TextureFlags::PrefersLoadingFromFileAsSRGB;
                    TextureGpuManager *textureManager = Root::getSingleton().
                                                        getRenderSystem()->getTextureGpuManager();

                    //First check if we need the alias: there could be a texture
                    //with the original name that satisfies our needs
                    TextureGpu *texture = textureManager->findTextureNoThrow( mFrames[frame] );

                    if( texture && texture->hasAutomaticBatching() == mAutomaticBatching )
                        mFramePtrs[frame] = texture;
                    else
                    {
                        mFramePtrs[frame] =
                                textureManager->createOrRetrieveTexture( mFrames[frame], aliasName,
                                                                         GpuPageOutStrategy::Discard,
                                                                         textureFlags,
                                                                         mTextureType,
                                                                         mParent->getResourceGroup() );
                    }

                    if( mFramePtrs[frame]->getNextResidencyStatus() != GpuResidency::Resident )
                    {
                        mFramePtrs[frame]->unsafeScheduleTransitionTo( GpuResidency::Resident );
                        mFramePtrs[frame]->waitForData();
                    }

                    //You think this is ugly? So do I.
                    //But I'm not the #@!# who made "ensurePrepared" const
                    mFramePtrs[frame]->addListener( const_cast<TextureUnitState*>( this ) );
                }
                catch (Exception &e)
                {
                    String msg;
                    msg = msg + "Error loading texture " + mFrames[frame]  +
                          ". Texture layer will be blank. Loading the texture "
                          "failed with the following exception: "
                          + e.getFullDescription();
                    LogManager::getSingleton().logMessage(msg, LML_CRITICAL);
                    mTextureLoadFailed = true;
                }
            }
            else
            {
                // Just ensure existing pointer is loaded
                if( mFramePtrs[frame]->getNextResidencyStatus() != GpuResidency::Resident )
                {
                    mFramePtrs[frame]->unsafeScheduleTransitionTo( GpuResidency::Resident );
                    mFramePtrs[frame]->waitForData();
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::createAnimController(void)
    {
        if (mAnimController)
        {
            ControllerManager::getSingleton().destroyController(mAnimController);
            mAnimController = 0;
        }
        mAnimController = ControllerManager::getSingleton().createTextureAnimator(this, mAnimDuration);

    }
    //-----------------------------------------------------------------------
    void TextureUnitState::createEffectController(TextureEffect& effect)
    {
        if (effect.controller)
        {
            ControllerManager::getSingleton().destroyController(effect.controller);
            effect.controller = 0;
        }
        ControllerManager& cMgr = ControllerManager::getSingleton();
        switch (effect.type)
        {
        case ET_UVSCROLL:
            effect.controller = cMgr.createTextureUVScroller(this, effect.arg1);
            break;
        case ET_USCROLL:
            effect.controller = cMgr.createTextureUScroller(this, effect.arg1);
            break;
        case ET_VSCROLL:
            effect.controller = cMgr.createTextureVScroller(this, effect.arg1);
            break;
        case ET_ROTATE:
            effect.controller = cMgr.createTextureRotater(this, effect.arg1);
            break;
        case ET_TRANSFORM:
            effect.controller = cMgr.createTextureWaveTransformer(this, (TextureUnitState::TextureTransformType)effect.subtype, effect.waveType, effect.base,
                effect.frequency, effect.phase, effect.amplitude);
            break;
        case ET_ENVIRONMENT_MAP:
            break;
        default:
            break;
        }
    }
    //-----------------------------------------------------------------------
    Real TextureUnitState::getTextureUScroll(void) const
    {
        return mUMod;
    }

    //-----------------------------------------------------------------------
    Real TextureUnitState::getTextureVScroll(void) const
    {
        return mVMod;
    }

    //-----------------------------------------------------------------------
    Real TextureUnitState::getTextureUScale(void) const
    {
        return mUScale;
    }

    //-----------------------------------------------------------------------
    Real TextureUnitState::getTextureVScale(void) const
    {
        return mVScale;
    }

    //-----------------------------------------------------------------------
    const Radian& TextureUnitState::getTextureRotate(void) const
    {
        return mRotate;
    }
    
    //-----------------------------------------------------------------------
    Real TextureUnitState::getAnimationDuration(void) const
    {
        return mAnimDuration;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setSamplerblock( const HlmsSamplerblock &samplerblock )
    {
        HlmsManager *hlmsManager = mParent->_getDatablock()->getCreator()->getHlmsManager();
        const HlmsSamplerblock *oldSamplerblock = mSamplerblock;
        mSamplerblock = hlmsManager->getSamplerblock( samplerblock );

        if( oldSamplerblock )
            hlmsManager->destroySamplerblock( oldSamplerblock );
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::_setSamplerblock( const HlmsSamplerblock *samplerblock )
    {
        HlmsManager *hlmsManager = mParent->_getDatablock()->getCreator()->getHlmsManager();
        const HlmsSamplerblock *oldSamplerblock = mSamplerblock;
        hlmsManager->addReference( samplerblock );
        mSamplerblock = samplerblock;

        if( oldSamplerblock )
            hlmsManager->destroySamplerblock( oldSamplerblock );
    }
    //-----------------------------------------------------------------------
    const HlmsSamplerblock* TextureUnitState::getSamplerblock(void) const
    {
        return mSamplerblock;
    }
    //-----------------------------------------------------------------------
    const TextureUnitState::EffectMap& TextureUnitState::getEffects(void) const
    {
        return mEffects;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::_unprepare(void)
    {
        cleanFramePtrs();
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::_unload(void)
    {
        // Destroy animation controller
        if (mAnimController)
        {
            ControllerManager::getSingleton().destroyController(mAnimController);
            mAnimController = 0;
        }

        // Destroy effect controllers
        for (EffectMap::iterator i = mEffects.begin(); i != mEffects.end(); ++i)
        {
            if (i->second.controller)
            {
                ControllerManager::getSingleton().destroyController(i->second.controller);
                i->second.controller = 0;
            }
        }

        cleanFramePtrs();
    }
    //-----------------------------------------------------------------------------
    bool TextureUnitState::isLoaded(void) const
    {
        return mParent->isLoaded();
    }
    //-----------------------------------------------------------------------
    bool TextureUnitState::hasViewRelativeTextureCoordinateGeneration(void) const
    {
        // Right now this only returns true for reflection maps

        EffectMap::const_iterator i, iend;
        iend = mEffects.end();
        
        for(i = mEffects.find(ET_ENVIRONMENT_MAP); i != iend; ++i)
        {
            if (i->second.subtype == ENV_REFLECTION)
                return true;
        }

        if(mEffects.find(ET_PROJECTIVE_TEXTURE) != iend)
        {
            return true;
        }

        return false;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setProjectiveTexturing(bool enable, 
        const Frustum* projectionSettings)
    {
        if (enable)
        {
            TextureEffect eff;
            eff.type = ET_PROJECTIVE_TEXTURE;
            eff.frustum = projectionSettings;
            addEffect(eff);
        }
        else
        {
            removeEffect(ET_PROJECTIVE_TEXTURE);
        }

    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setName(const String& name)
    {
        mName = name;
        if (mTextureNameAlias.empty())
            mTextureNameAlias = mName;
    }

    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureNameAlias(const String& name)
    {
        mTextureNameAlias = name;
    }

    //-----------------------------------------------------------------------
    bool TextureUnitState::applyTextureAliases(const AliasTextureNamePairList& aliasList, const bool apply)
    {
        bool testResult = false;
        // if TUS has an alias see if its in the alias container
        if (!mTextureNameAlias.empty())
        {
            AliasTextureNamePairList::const_iterator aliasEntry =
                aliasList.find(mTextureNameAlias);

            if (aliasEntry != aliasList.end())
            {
                // match was found so change the texture name in mFrames
                testResult = true;

                if (apply)
                {
                    // currently assumes animated frames are sequentially numbered
                    // cubic, 1d, 2d, and 3d textures are determined from current TUS state
                    
                    // if cubic or 3D
                    if (mCubic)
                    {
                        setCubicTextureName(aliasEntry->second, mTextureType == TextureTypes::TypeCube);
                    }
                    else
                    {
                        // if more than one frame then assume animated frames
                        if (mFrames.size() > 1)
                            setAnimatedTextureName(aliasEntry->second, 
                                static_cast<unsigned int>(mFrames.size()), mAnimDuration);
                        else
                            setTextureName(aliasEntry->second, mTextureType);
                    }
                }
                
            }
        }

        return testResult;
    }
    //-----------------------------------------------------------------------------
    void TextureUnitState::_notifyParent(Pass* parent)
    {
        mParent = parent;
    }
    //-----------------------------------------------------------------------------
    void TextureUnitState::setCompositorReference( const String& textureName )
    {  
        cleanFramePtrs();
        mFrames.resize(1);
        mFramePtrs.resize(1);
        mCompositorRefTexName = textureName;
    }
    //-----------------------------------------------------------------------
    size_t TextureUnitState::calculateSize(void) const
    {
        size_t memSize = 0;

        memSize += sizeof(TextureUnitState);

        memSize += mFrames.size() * sizeof(String);
        memSize += mFramePtrs.size() * sizeof(TextureGpu*);
        memSize += mEffects.size() * sizeof(TextureEffect);

        return memSize;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::cleanFramePtrs(void)
    {
        vector<TextureGpu*>::type::iterator itor = mFramePtrs.begin();
        vector<TextureGpu*>::type::iterator end  = mFramePtrs.end();
        while( itor != end )
        {
            if( *itor )
                (*itor)->removeListener( this );
            *itor = 0;
            ++itor;
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::notifyTextureChanged( TextureGpu *texture, TextureGpuListener::Reason reason,
                                                 void *extraData )
    {
        if( reason == TextureGpuListener::Deleted )
        {
            vector<TextureGpu*>::type::iterator itor = mFramePtrs.begin();
            vector<TextureGpu*>::type::iterator end  = mFramePtrs.end();

            while( itor != end )
            {
                if( *itor == texture )
                {
                    texture->removeListener( this );
                    *itor = 0;
                }
                ++itor;
            }
        }
    }
}
