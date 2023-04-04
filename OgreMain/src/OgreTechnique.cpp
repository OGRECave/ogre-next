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

#include "OgreTechnique.h"

#include "OgreHlmsDatablock.h"
#include "OgreMaterial.h"
#include "OgreMaterialManager.h"
#include "OgrePass.h"
#include "OgreRenderSystem.h"
#include "OgreRoot.h"
#include "OgreString.h"

#include <sstream>

namespace Ogre
{
    //-----------------------------------------------------------------------------
    Technique::Technique( Material *parent ) :
        mParent( parent ),
        mIsSupported( false ),
        mLodIndex( 0 ),
        mSchemeIndex( 0 )
    {
        // See above, defaults to unsupported until examined
    }
    //-----------------------------------------------------------------------------
    Technique::Technique( Material *parent, const Technique &oth ) :
        mParent( parent ),
        mLodIndex( 0 ),
        mSchemeIndex( 0 )
    {
        // Copy using operator=
        *this = oth;
    }
    //-----------------------------------------------------------------------------
    Technique::~Technique() { removeAllPasses(); }
    //-----------------------------------------------------------------------------
    bool Technique::isSupported() const { return mIsSupported; }
    //-----------------------------------------------------------------------------
    size_t Technique::calculateSize() const
    {
        size_t memSize = 0;

        // Tally up passes
        for( Pass *pass : mPasses )
            memSize += pass->calculateSize();

        return memSize;
    }
    //-----------------------------------------------------------------------------
    String Technique::_compile( bool autoManageTextureUnits )
    {
        StringStream errors;

        mIsSupported = checkGPURules( errors );
        if( mIsSupported )
        {
            mIsSupported = checkHardwareSupport( autoManageTextureUnits, errors );
        }

        return errors.str();
    }
    //---------------------------------------------------------------------
    bool Technique::checkHardwareSupport( bool autoManageTextureUnits, StringStream &compileErrors )
    {
        // Go through each pass, checking requirements
        Passes::iterator i;
        unsigned short passNum = 0;
        const RenderSystemCapabilities *caps = Root::getSingleton().getRenderSystem()->getCapabilities();
        unsigned short numTexUnits = caps->getNumTextureUnits();
        for( i = mPasses.begin(); i != mPasses.end(); ++i, ++passNum )
        {
            Pass *currPass = *i;
            // Adjust pass index
            currPass->_notifyIndex( passNum );
            // Check texture unit requirements
            size_t numTexUnitsRequested = currPass->getNumTextureUnitStates();
            // Don't trust getNumTextureUnits for programmable
            if( !currPass->hasFragmentProgram() )
            {
#if defined( OGRE_PRETEND_TEXTURE_UNITS ) && OGRE_PRETEND_TEXTURE_UNITS > 0
                if( numTexUnits > OGRE_PRETEND_TEXTURE_UNITS )
                    numTexUnits = OGRE_PRETEND_TEXTURE_UNITS;
#endif
                if( numTexUnitsRequested > numTexUnits )
                {
                    if( !autoManageTextureUnits )
                    {
                        // The user disabled auto pass split
                        compileErrors << "Pass " << passNum
                                      << ": Too many texture units for the current hardware and no "
                                         "splitting allowed."
                                      << std::endl;
                        return false;
                    }
                    else if( currPass->hasVertexProgram() )
                    {
                        // Can't do this one, and can't split a programmable pass
                        compileErrors << "Pass " << passNum
                                      << ": Too many texture units for the current hardware and "
                                         "cannot split programmable passes."
                                      << std::endl;
                        return false;
                    }
                }
            }
            if( currPass->hasComputeProgram() )
            {
                // Check fragment program version
                if( !currPass->getComputeProgram()->isSupported() )
                {
                    // Can't do this one
                    compileErrors << "Pass " << passNum << ": Compute program "
                                  << currPass->getComputeProgram()->getName() << " cannot be used - ";
                    if( currPass->getComputeProgram()->hasCompileError() )
                        compileErrors << "compile error.";
                    else
                        compileErrors << "not supported.";

                    compileErrors << std::endl;
                    return false;
                }
            }
            if( currPass->hasVertexProgram() )
            {
                // Check vertex program version
                if( !currPass->getVertexProgram()->isSupported() )
                {
                    // Can't do this one
                    compileErrors << "Pass " << passNum << ": Vertex program "
                                  << currPass->getVertexProgram()->getName() << " cannot be used - ";
                    if( currPass->getVertexProgram()->hasCompileError() )
                        compileErrors << "compile error.";
                    else
                        compileErrors << "not supported.";

                    compileErrors << std::endl;
                    return false;
                }
            }
            if( currPass->hasTessellationHullProgram() )
            {
                // Check tessellation control program version
                if( !currPass->getTessellationHullProgram()->isSupported() )
                {
                    // Can't do this one
                    compileErrors << "Pass " << passNum << ": Tessellation Hull program "
                                  << currPass->getTessellationHullProgram()->getName()
                                  << " cannot be used - ";
                    if( currPass->getTessellationHullProgram()->hasCompileError() )
                        compileErrors << "compile error.";
                    else
                        compileErrors << "not supported.";

                    compileErrors << std::endl;
                    return false;
                }
            }
            if( currPass->hasTessellationDomainProgram() )
            {
                // Check tessellation control program version
                if( !currPass->getTessellationDomainProgram()->isSupported() )
                {
                    // Can't do this one
                    compileErrors << "Pass " << passNum << ": Tessellation Domain program "
                                  << currPass->getTessellationDomainProgram()->getName()
                                  << " cannot be used - ";
                    if( currPass->getTessellationDomainProgram()->hasCompileError() )
                        compileErrors << "compile error.";
                    else
                        compileErrors << "not supported.";

                    compileErrors << std::endl;
                    return false;
                }
            }
            if( currPass->hasGeometryProgram() )
            {
                // Check geometry program version
                if( !currPass->getGeometryProgram()->isSupported() )
                {
                    // Can't do this one
                    compileErrors << "Pass " << passNum << ": Geometry program "
                                  << currPass->getGeometryProgram()->getName() << " cannot be used - ";
                    if( currPass->getGeometryProgram()->hasCompileError() )
                        compileErrors << "compile error.";
                    else
                        compileErrors << "not supported.";

                    compileErrors << std::endl;
                    return false;
                }
            }
            if( currPass->hasFragmentProgram() )
            {
                // Check fragment program version
                if( !currPass->getFragmentProgram()->isSupported() )
                {
                    // Can't do this one
                    compileErrors << "Pass " << passNum << ": Fragment program "
                                  << currPass->getFragmentProgram()->getName() << " cannot be used - ";
                    if( currPass->getFragmentProgram()->hasCompileError() )
                        compileErrors << "compile error.";
                    else
                        compileErrors << "not supported.";

                    compileErrors << std::endl;
                    return false;
                }
            }
            else
            {
                // Check a few fixed-function options in texture layers
                Pass::TextureUnitStateIterator texi = currPass->getTextureUnitStateIterator();
                size_t texUnit = 0;
                while( texi.hasMoreElements() )
                {
                    TextureUnitState *tex = texi.getNext();
                    // Any Cube textures? NB we make the assumption that any
                    // card capable of running fragment programs can support
                    // cubic textures, which has to be true, surely?
                    if( tex->is3D() && !caps->hasCapability( RSC_CUBEMAPPING ) )
                    {
                        // Fail
                        compileErrors << "Pass " << passNum << " Tex " << texUnit
                                      << ": Cube maps not supported by current environment."
                                      << std::endl;
                        return false;
                    }
                    // Any 3D textures? NB we make the assumption that any
                    // card capable of running fragment programs can support
                    // 3D textures, which has to be true, surely?
                    if( ( ( tex->getTextureType() == TextureTypes::Type3D ) ||
                          ( tex->getTextureType() == TextureTypes::Type2DArray ) ) &&
                        !caps->hasCapability( RSC_TEXTURE_3D ) )
                    {
                        // Fail
                        compileErrors << "Pass " << passNum << " Tex " << texUnit
                                      << ": Volume textures not supported by current environment."
                                      << std::endl;
                        return false;
                    }
                    // Any Dot3 blending?
                    if( tex->getColourBlendMode().operation == LBX_DOTPRODUCT &&
                        !caps->hasCapability( RSC_DOT3 ) )
                    {
                        // Fail
                        compileErrors << "Pass " << passNum << " Tex " << texUnit
                                      << ": DOT3 blending not supported by current environment."
                                      << std::endl;
                        return false;
                    }
                    ++texUnit;
                }
            }
        }
        // If we got this far, we're ok
        return true;
    }
    //---------------------------------------------------------------------
    bool Technique::checkGPURules( StringStream &errors )
    {
        const RenderSystemCapabilities *caps = Root::getSingleton().getRenderSystem()->getCapabilities();

        StringStream includeRules;
        bool includeRulesPresent = false;
        bool includeRuleMatched = false;

        // Check vendors first
        for( GPUVendorRuleList::const_iterator i = mGPUVendorRules.begin(); i != mGPUVendorRules.end();
             ++i )
        {
            if( i->includeOrExclude == INCLUDE )
            {
                includeRulesPresent = true;
                includeRules << caps->vendorToString( i->vendor ) << " ";
                if( i->vendor == caps->getVendor() )
                    includeRuleMatched = true;
            }
            else  // EXCLUDE
            {
                if( i->vendor == caps->getVendor() )
                {
                    errors << "Excluded GPU vendor: " << caps->vendorToString( i->vendor ) << std::endl;
                    return false;
                }
            }
        }

        if( includeRulesPresent && !includeRuleMatched )
        {
            errors << "Failed to match GPU vendor: " << includeRules.str() << std::endl;
            return false;
        }

        // now check device names
        includeRules.str( BLANKSTRING );
        includeRulesPresent = false;
        includeRuleMatched = false;

        for( GPUDeviceNameRuleList::const_iterator i = mGPUDeviceNameRules.begin();
             i != mGPUDeviceNameRules.end(); ++i )
        {
            if( i->includeOrExclude == INCLUDE )
            {
                includeRulesPresent = true;
                includeRules << i->devicePattern << " ";
                if( StringUtil::match( caps->getDeviceName(), i->devicePattern, i->caseSensitive ) )
                    includeRuleMatched = true;
            }
            else  // EXCLUDE
            {
                if( StringUtil::match( caps->getDeviceName(), i->devicePattern, i->caseSensitive ) )
                {
                    errors << "Excluded GPU device: " << i->devicePattern << std::endl;
                    return false;
                }
            }
        }

        if( includeRulesPresent && !includeRuleMatched )
        {
            errors << "Failed to match GPU device: " << includeRules.str() << std::endl;
            return false;
        }

        // passed
        return true;
    }
    //-----------------------------------------------------------------------------
    Pass *Technique::createPass()
    {
        Pass *newPass = OGRE_NEW Pass( this, static_cast<unsigned short>( mPasses.size() ) );
        mPasses.push_back( newPass );
        return newPass;
    }
    //-----------------------------------------------------------------------------
    Pass *Technique::getPass( unsigned short index )
    {
        assert( index < mPasses.size() && "Index out of bounds" );
        return mPasses[index];
    }
    //-----------------------------------------------------------------------------
    Pass *Technique::getPass( const String &name )
    {
        Passes::iterator i = mPasses.begin();
        Passes::iterator iend = mPasses.end();
        Pass *foundPass = 0;

        // iterate through techniques to find a match
        while( i != iend )
        {
            if( ( *i )->getName() == name )
            {
                foundPass = ( *i );
                break;
            }
            ++i;
        }

        return foundPass;
    }
    //-----------------------------------------------------------------------------
    unsigned short Technique::getNumPasses() const
    {
        return static_cast<unsigned short>( mPasses.size() );
    }
    //-----------------------------------------------------------------------------
    void Technique::removePass( unsigned short index )
    {
        assert( index < mPasses.size() && "Index out of bounds" );
        Passes::iterator i = mPasses.begin() + index;
        OGRE_DELETE *i;
        i = mPasses.erase( i );
        // Adjust passes index
        for( ; i != mPasses.end(); ++i, ++index )
        {
            ( *i )->_notifyIndex( index );
        }
    }
    //-----------------------------------------------------------------------------
    void Technique::removeAllPasses()
    {
        for( Pass *pass : mPasses )
        {
            OGRE_DELETE pass;
        }
        mPasses.clear();
    }

    //-----------------------------------------------------------------------------
    bool Technique::movePass( const unsigned short sourceIndex, const unsigned short destinationIndex )
    {
        bool moveSuccessful = false;

        // don't move the pass if source == destination
        if( sourceIndex == destinationIndex )
            return true;

        if( ( sourceIndex < mPasses.size() ) && ( destinationIndex < mPasses.size() ) )
        {
            Passes::iterator i = mPasses.begin() + sourceIndex;
            // Passes::iterator DestinationIterator = mPasses.begin() + destinationIndex;

            Pass *pass = ( *i );
            mPasses.erase( i );

            i = mPasses.begin() + destinationIndex;

            mPasses.insert( i, pass );

            // Adjust passes index
            unsigned short beginIndex, endIndex;
            if( destinationIndex > sourceIndex )
            {
                beginIndex = sourceIndex;
                endIndex = destinationIndex;
            }
            else
            {
                beginIndex = destinationIndex;
                endIndex = sourceIndex;
            }
            for( unsigned short index = beginIndex; index <= endIndex; ++index )
            {
                mPasses[index]->_notifyIndex( index );
            }
            moveSuccessful = true;
        }

        return moveSuccessful;
    }

    //-----------------------------------------------------------------------------
    const Technique::PassIterator Technique::getPassIterator()
    {
        return PassIterator( mPasses.begin(), mPasses.end() );
    }
    //-----------------------------------------------------------------------------
    Technique &Technique::operator=( const Technique &rhs )
    {
        mName = rhs.mName;
        this->mIsSupported = rhs.mIsSupported;
        this->mLodIndex = rhs.mLodIndex;
        this->mSchemeIndex = rhs.mSchemeIndex;
        this->mShadowCasterMaterial = rhs.mShadowCasterMaterial;
        this->mShadowCasterMaterialName = rhs.mShadowCasterMaterialName;
        this->mGPUVendorRules = rhs.mGPUVendorRules;
        this->mGPUDeviceNameRules = rhs.mGPUDeviceNameRules;

        // copy passes
        removeAllPasses();
        for( Pass *rpass : rhs.mPasses )
        {
            Pass *p = OGRE_NEW Pass( this, rpass->getIndex(), *rpass );
            mPasses.push_back( p );
        }
        return *this;
    }
    //-----------------------------------------------------------------------------
    bool Technique::isTransparent() const
    {
        if( mPasses.empty() )
        {
            return false;
        }
        else
        {
            // Base decision on the transparency of the first pass
            return mPasses[0]->isTransparent();
        }
    }
    //-----------------------------------------------------------------------------
    bool Technique::isDepthWriteEnabled() const
    {
        if( mPasses.empty() )
        {
            return false;
        }
        else
        {
            // Base decision on the depth settings of the first pass
            return mPasses[0]->getMacroblock()->mDepthWrite;
        }
    }
    //-----------------------------------------------------------------------------
    bool Technique::isDepthCheckEnabled() const
    {
        if( mPasses.empty() )
        {
            return false;
        }
        else
        {
            // Base decision on the depth settings of the first pass
            return mPasses[0]->getMacroblock()->mDepthCheck;
        }
    }
    //-----------------------------------------------------------------------------
    bool Technique::hasColourWriteDisabled() const
    {
        if( mPasses.empty() )
        {
            return true;
        }
        else
        {
            // Base decision on the colour write settings of the first pass
            return !mPasses[0]->getColourWriteEnabled();
        }
    }
    //-----------------------------------------------------------------------------
    void Technique::_prepare()
    {
        assert( mIsSupported && "This technique is not supported" );
        // Load each pass
        for( Pass *pass : mPasses )
            pass->_prepare();
    }
    //-----------------------------------------------------------------------------
    void Technique::_unprepare()
    {
        // Unload each pass
        for( Pass *pass : mPasses )
            pass->_unprepare();
    }
    //-----------------------------------------------------------------------------
    void Technique::_load()
    {
        assert( mIsSupported && "This technique is not supported" );
        // Load each pass
        for( Pass *pass : mPasses )
            pass->_load();

        if( mShadowCasterMaterial )
        {
            mShadowCasterMaterial->load();
        }
        else if( !mShadowCasterMaterialName.empty() )
        {
            // in case we could not get material as it wasn't yet parsed/existent at that time.
            mShadowCasterMaterial =
                MaterialManager::getSingleton().getByName( mShadowCasterMaterialName );
            if( mShadowCasterMaterial )
                mShadowCasterMaterial->load();
        }
    }
    //-----------------------------------------------------------------------------
    void Technique::_unload()
    {
        // Unload each pass
        for( Pass *pass : mPasses )
            pass->_unload();
    }
    //-----------------------------------------------------------------------------
    bool Technique::isLoaded() const
    {
        // Only supported technique will be loaded
        return mParent->isLoaded() && mIsSupported;
    }
    //-----------------------------------------------------------------------
    void Technique::setPointSize( Real ps )
    {
        for( Pass *pass : mPasses )
            pass->setPointSize( ps );
    }
    //-----------------------------------------------------------------------
    void Technique::setAmbient( Real red, Real green, Real blue )
    {
        setAmbient( ColourValue( red, green, blue ) );
    }
    //-----------------------------------------------------------------------
    void Technique::setAmbient( const ColourValue &ambient )
    {
        for( Pass *pass : mPasses )
            pass->setAmbient( ambient );
    }
    //-----------------------------------------------------------------------
    void Technique::setDiffuse( Real red, Real green, Real blue, Real alpha )
    {
        for( Pass *pass : mPasses )
            pass->setDiffuse( red, green, blue, alpha );
    }
    //-----------------------------------------------------------------------
    void Technique::setDiffuse( const ColourValue &diffuse )
    {
        setDiffuse( diffuse.r, diffuse.g, diffuse.b, diffuse.a );
    }
    //-----------------------------------------------------------------------
    void Technique::setSpecular( Real red, Real green, Real blue, Real alpha )
    {
        for( Pass *pass : mPasses )
            pass->setSpecular( red, green, blue, alpha );
    }
    //-----------------------------------------------------------------------
    void Technique::setSpecular( const ColourValue &specular )
    {
        setSpecular( specular.r, specular.g, specular.b, specular.a );
    }
    //-----------------------------------------------------------------------
    void Technique::setShininess( Real val )
    {
        for( Pass *pass : mPasses )
            pass->setShininess( val );
    }
    //-----------------------------------------------------------------------
    void Technique::setSelfIllumination( Real red, Real green, Real blue )
    {
        setSelfIllumination( ColourValue( red, green, blue ) );
    }
    //-----------------------------------------------------------------------
    void Technique::setSelfIllumination( const ColourValue &selfIllum )
    {
        for( Pass *pass : mPasses )
            pass->setSelfIllumination( selfIllum );
    }
    //-----------------------------------------------------------------------
    void Technique::setShadingMode( ShadeOptions mode )
    {
        for( Pass *pass : mPasses )
            pass->setShadingMode( mode );
    }
    //-----------------------------------------------------------------------
    void Technique::setFog( bool overrideScene, FogMode mode, const ColourValue &colour, Real expDensity,
                            Real linearStart, Real linearEnd )
    {
        for( Pass *pass : mPasses )
            pass->setFog( overrideScene, mode, colour, expDensity, linearStart, linearEnd );
    }
    //-----------------------------------------------------------------------
    void Technique::setSamplerblock( const HlmsSamplerblock &samplerblock )
    {
        for( Pass *pass : mPasses )
            pass->setSamplerblock( samplerblock );
    }
    // --------------------------------------------------------------------
    void Technique::setMacroblock( const HlmsMacroblock &macroblock )
    {
        for( Pass *pass : mPasses )
            pass->setMacroblock( macroblock );
    }
    // --------------------------------------------------------------------
    void Technique::setBlendblock( const HlmsBlendblock &blendblock )
    {
        for( Pass *pass : mPasses )
            pass->setBlendblock( blendblock );
    }
    // --------------------------------------------------------------------
    void Technique::setName( const String &name ) { mName = name; }
    //-----------------------------------------------------------------------
    void Technique::_notifyNeedsRecompile() { mParent->_notifyNeedsRecompile(); }
    //-----------------------------------------------------------------------
    void Technique::setLodIndex( unsigned short index ) { mLodIndex = index; }
    //-----------------------------------------------------------------------
    void Technique::setSchemeName( const String &schemeName )
    {
        mSchemeIndex = MaterialManager::getSingleton()._getSchemeIndex( schemeName );
    }
    //-----------------------------------------------------------------------
    const String &Technique::getSchemeName() const
    {
        return MaterialManager::getSingleton()._getSchemeName( mSchemeIndex );
    }
    //-----------------------------------------------------------------------
    unsigned short Technique::_getSchemeIndex() const { return mSchemeIndex; }
    //-----------------------------------------------------------------------
    const String &Technique::getResourceGroup() const { return mParent->getGroup(); }

    //-----------------------------------------------------------------------
    bool Technique::applyTextureAliases( const AliasTextureNamePairList &aliasList,
                                         const bool apply ) const
    {
        // iterate through passes and apply texture alias
        bool testResult = false;

        for( Pass *pass : mPasses )
        {
            if( pass->applyTextureAliases( aliasList, apply ) )
                testResult = true;
        }

        return testResult;
    }
    //-----------------------------------------------------------------------
    Ogre::MaterialPtr Technique::getShadowCasterMaterial() const { return mShadowCasterMaterial; }
    //-----------------------------------------------------------------------
    void Technique::setShadowCasterMaterial( Ogre::MaterialPtr val )
    {
        if( val )
        {
            mShadowCasterMaterial = val;
            mShadowCasterMaterialName = val->getName();
        }
        else
        {
            mShadowCasterMaterial.reset();
            mShadowCasterMaterialName.clear();
        }
    }
    //-----------------------------------------------------------------------
    void Technique::setShadowCasterMaterial( const Ogre::String &name )
    {
        mShadowCasterMaterialName = name;
        mShadowCasterMaterial = MaterialManager::getSingleton().getByName( name );
    }
    //---------------------------------------------------------------------
    void Technique::addGPUVendorRule( GPUVendor vendor, Technique::IncludeOrExclude includeOrExclude )
    {
        addGPUVendorRule( GPUVendorRule( vendor, includeOrExclude ) );
    }
    //---------------------------------------------------------------------
    void Technique::addGPUVendorRule( const Technique::GPUVendorRule &rule )
    {
        // remove duplicates
        removeGPUVendorRule( rule.vendor );
        mGPUVendorRules.push_back( rule );
    }
    //---------------------------------------------------------------------
    void Technique::removeGPUVendorRule( GPUVendor vendor )
    {
        for( GPUVendorRuleList::iterator i = mGPUVendorRules.begin(); i != mGPUVendorRules.end(); )
        {
            if( i->vendor == vendor )
                i = mGPUVendorRules.erase( i );
            else
                ++i;
        }
    }
    //---------------------------------------------------------------------
    Technique::GPUVendorRuleIterator Technique::getGPUVendorRuleIterator() const
    {
        return GPUVendorRuleIterator( mGPUVendorRules.begin(), mGPUVendorRules.end() );
    }
    //---------------------------------------------------------------------
    void Technique::addGPUDeviceNameRule( const String &devicePattern,
                                          Technique::IncludeOrExclude includeOrExclude,
                                          bool caseSensitive )
    {
        addGPUDeviceNameRule( GPUDeviceNameRule( devicePattern, includeOrExclude, caseSensitive ) );
    }
    //---------------------------------------------------------------------
    void Technique::addGPUDeviceNameRule( const Technique::GPUDeviceNameRule &rule )
    {
        // remove duplicates
        removeGPUDeviceNameRule( rule.devicePattern );
        mGPUDeviceNameRules.push_back( rule );
    }
    //---------------------------------------------------------------------
    void Technique::removeGPUDeviceNameRule( const String &devicePattern )
    {
        for( GPUDeviceNameRuleList::iterator i = mGPUDeviceNameRules.begin();
             i != mGPUDeviceNameRules.end(); )
        {
            if( i->devicePattern == devicePattern )
                i = mGPUDeviceNameRules.erase( i );
            else
                ++i;
        }
    }
    //---------------------------------------------------------------------
    Technique::GPUDeviceNameRuleIterator Technique::getGPUDeviceNameRuleIterator() const
    {
        return GPUDeviceNameRuleIterator( mGPUDeviceNameRules.begin(), mGPUDeviceNameRules.end() );
    }
    //---------------------------------------------------------------------

}  // namespace Ogre
