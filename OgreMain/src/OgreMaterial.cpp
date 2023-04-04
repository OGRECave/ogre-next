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

#include "OgreMaterial.h"

#include "OgreLodStrategy.h"
#include "OgreLodStrategyManager.h"
#include "OgreLogManager.h"
#include "OgreMaterialManager.h"
#include "OgreTechnique.h"

#include <sstream>

namespace Ogre
{
    //-----------------------------------------------------------------------
    Material::Material( ResourceManager *creator, const String &name, ResourceHandle handle,
                        const String &group, bool isManual, ManualResourceLoader *loader ) :
        Resource( creator, name, handle, group, isManual, loader ),
        mReceiveShadows( true ),
        mTransparencyCastsShadows( false ),
        mCompilationRequired( true )
    {
        // Override isManual, not applicable for Material (we always want to call loadImpl)
        if( isManual )
        {
            mIsManual = false;
            LogManager::getSingleton().logMessage(
                "Material " + name +
                " was requested with isManual=true, but this is not applicable "
                "for materials; the flag has been reset to false" );
        }

        mLodValues.push_back( LodStrategyManager::getSingleton().getDefaultStrategy()->getBaseValue() );

        applyDefaults();

        /* For consistency with StringInterface, but we don't add any parameters here
        That's because the Resource implementation of StringInterface is to
        list all the options that need to be set before loading, of which
        we have none as such. Full details can be set through scripts.
        */
        createParamDictionary( "Material" );
    }
    //-----------------------------------------------------------------------
    Material::~Material()
    {
        removeAllTechniques();
        // have to call this here rather than in Resource destructor
        // since calling virtual methods in base destructors causes crash
        unload();
    }
    //-----------------------------------------------------------------------
    Material &Material::operator=( const Material &rhs )
    {
        mName = rhs.mName;
        mGroup = rhs.mGroup;
        mCreator = rhs.mCreator;
        mIsManual = rhs.mIsManual;
        mLoader = rhs.mLoader;
        mHandle = rhs.mHandle;
        mSize = rhs.mSize;
        mReceiveShadows = rhs.mReceiveShadows;
        mTransparencyCastsShadows = rhs.mTransparencyCastsShadows;

        mLoadingState = rhs.mLoadingState;
        mIsBackgroundLoaded = rhs.mIsBackgroundLoaded;

        // Copy Techniques
        this->removeAllTechniques();
        for( Technique *rtech : rhs.mTechniques )
        {
            Technique *t = this->createTechnique();
            *t = *rtech;
            if( rtech->isSupported() )
            {
                insertSupportedTechnique( t );
            }
        }

        // Also copy LOD information
        mUserLodValues = rhs.mUserLodValues;
        mLodValues = rhs.mLodValues;
        mCompilationRequired = rhs.mCompilationRequired;
        // illumination passes are not compiled right away so
        // mIsLoaded state should still be the same as the original material
        assert( isLoaded() == rhs.isLoaded() );

        return *this;
    }

    //-----------------------------------------------------------------------
    void Material::prepareImpl()
    {
        // compile if required
        if( mCompilationRequired )
            compile();

        // Load all supported techniques
        for( Technique *tech : mSupportedTechniques )
            tech->_prepare();
    }
    //-----------------------------------------------------------------------
    void Material::unprepareImpl()
    {
        // Load all supported techniques
        for( Technique *tech : mSupportedTechniques )
            tech->_unprepare();
    }
    //-----------------------------------------------------------------------
    void Material::loadImpl()
    {
        // Load all supported techniques
        for( Technique *tech : mSupportedTechniques )
            tech->_load();
    }
    //-----------------------------------------------------------------------
    void Material::unloadImpl()
    {
        // Unload all supported techniques
        for( Technique *tech : mSupportedTechniques )
            tech->_unload();
    }
    //-----------------------------------------------------------------------
    size_t Material::calculateSize() const
    {
        size_t memSize = 0;

        // Tally up techniques
        for( Technique *tech : mTechniques )
            memSize += tech->calculateSize();

        memSize += sizeof( bool ) * 3;
        memSize += mUnsupportedReasons.size() * sizeof( char );
        memSize += sizeof( LodStrategy );

        memSize += Resource::calculateSize();

        return memSize;
    }
    //-----------------------------------------------------------------------
    MaterialPtr Material::clone( const String &newName, bool changeGroup, const String &newGroup ) const
    {
        MaterialPtr newMat;
        if( changeGroup )
        {
            newMat = MaterialManager::getSingleton().create( newName, newGroup );
        }
        else
        {
            newMat = MaterialManager::getSingleton().create( newName, mGroup );
        }

        // Keep handle (see below, copy overrides everything)
        ResourceHandle newHandle = newMat->getHandle();
        // Assign values from this
        *newMat = *this;
        // Restore new group if required, will have been overridden by operator
        if( changeGroup )
        {
            newMat->mGroup = newGroup;
        }

        // Correct the name & handle, they get copied too
        newMat->mName = newName;
        newMat->mHandle = newHandle;

        return newMat;
    }
    //-----------------------------------------------------------------------
    void Material::copyDetailsTo( MaterialPtr &mat ) const
    {
        // Keep handle (see below, copy overrides everything)
        ResourceHandle savedHandle = mat->mHandle;
        String savedName = mat->mName;
        String savedGroup = mat->mGroup;
        ManualResourceLoader *savedLoader = mat->mLoader;
        bool savedManual = mat->mIsManual;
        // Assign values from this
        *mat = *this;
        // Correct the name & handle, they get copied too
        mat->mName = savedName;
        mat->mHandle = savedHandle;
        mat->mGroup = savedGroup;
        mat->mIsManual = savedManual;
        mat->mLoader = savedLoader;
    }
    //-----------------------------------------------------------------------
    void Material::applyDefaults()
    {
        MaterialPtr defaults = MaterialManager::getSingleton().getDefaultSettings();

        if( defaults )
        {
            // save name & handle
            String savedName = mName;
            String savedGroup = mGroup;
            ResourceHandle savedHandle = mHandle;
            ManualResourceLoader *savedLoader = mLoader;
            bool savedManual = mIsManual;
            *this = *defaults;
            // restore name & handle
            mName = savedName;
            mHandle = savedHandle;
            mGroup = savedGroup;
            mLoader = savedLoader;
            mIsManual = savedManual;
        }
        mCompilationRequired = true;
    }
    //-----------------------------------------------------------------------
    Technique *Material::createTechnique()
    {
        Technique *t = OGRE_NEW Technique( this );
        mTechniques.push_back( t );
        mCompilationRequired = true;
        return t;
    }
    //-----------------------------------------------------------------------
    Technique *Material::getTechnique( unsigned short index )
    {
        assert( index < mTechniques.size() && "Index out of bounds." );
        return mTechniques[index];
    }
    //-----------------------------------------------------------------------
    Technique *Material::getTechnique( const String &name )
    {
        Techniques::iterator i = mTechniques.begin();
        Techniques::iterator iend = mTechniques.end();
        Technique *foundTechnique = 0;

        // iterate through techniques to find a match
        while( i != iend )
        {
            if( ( *i )->getName() == name )
            {
                foundTechnique = ( *i );
                break;
            }
            ++i;
        }

        return foundTechnique;
    }
    //-----------------------------------------------------------------------
    unsigned short Material::getNumTechniques() const
    {
        return static_cast<unsigned short>( mTechniques.size() );
    }
    //-----------------------------------------------------------------------
    Technique *Material::getSupportedTechnique( unsigned short index )
    {
        assert( index < mSupportedTechniques.size() && "Index out of bounds." );
        return mSupportedTechniques[index];
    }
    //-----------------------------------------------------------------------
    unsigned short Material::getNumSupportedTechniques() const
    {
        return static_cast<unsigned short>( mSupportedTechniques.size() );
    }
    //-----------------------------------------------------------------------
    unsigned short Material::getNumLodLevels( unsigned short schemeIndex ) const
    {
        // Safety check - empty list?
        if( mBestTechniquesBySchemeList.empty() )
            return 0;

        BestTechniquesBySchemeList::const_iterator i = mBestTechniquesBySchemeList.find( schemeIndex );
        if( i == mBestTechniquesBySchemeList.end() )
        {
            // get the first item, will be 0 (the default) if default
            // scheme techniques exist, otherwise the earliest defined
            i = mBestTechniquesBySchemeList.begin();
        }

        return static_cast<unsigned short>( i->second->size() );
    }
    //-----------------------------------------------------------------------
    unsigned short Material::getNumLodLevels( const String &schemeName ) const
    {
        return getNumLodLevels( MaterialManager::getSingleton()._getSchemeIndex( schemeName ) );
    }
    //-----------------------------------------------------------------------
    void Material::insertSupportedTechnique( Technique *t )
    {
        mSupportedTechniques.push_back( t );
        // get scheme
        unsigned short schemeIndex = t->_getSchemeIndex();
        BestTechniquesBySchemeList::iterator i = mBestTechniquesBySchemeList.find( schemeIndex );
        LodTechniques *lodtechs = 0;
        if( i == mBestTechniquesBySchemeList.end() )
        {
            lodtechs = OGRE_NEW_T( LodTechniques, MEMCATEGORY_RESOURCE );
            mBestTechniquesBySchemeList[schemeIndex] = lodtechs;
        }
        else
        {
            lodtechs = i->second;
        }

        // Insert won't replace if supported technique for this scheme/lod is
        // already there, which is what we want
        lodtechs->insert( LodTechniques::value_type( t->getLodIndex(), t ) );
    }
    //-----------------------------------------------------------------------------
    Technique *Material::getBestTechnique( unsigned short lodIndex, const Renderable *rend )
    {
        if( mSupportedTechniques.empty() )
        {
            return NULL;
        }
        else
        {
            Technique *ret = 0;
            MaterialManager &matMgr = MaterialManager::getSingleton();
            // get scheme
            BestTechniquesBySchemeList::iterator si =
                mBestTechniquesBySchemeList.find( matMgr._getActiveSchemeIndex() );
            // scheme not found?
            if( si == mBestTechniquesBySchemeList.end() )
            {
                // listener specified alternative technique available?
                ret = matMgr._arbitrateMissingTechniqueForActiveScheme( this, lodIndex, rend );
                if( ret )
                    return ret;

                // Nope, use default
                // get the first item, will be 0 (the default) if default
                // scheme techniques exist, otherwise the earliest defined
                si = mBestTechniquesBySchemeList.begin();
            }

            // get LOD
            LodTechniques::iterator li = si->second->find( lodIndex );
            // LOD not found?
            if( li == si->second->end() )
            {
                // Use the next LOD level up
                for( LodTechniques::reverse_iterator rli = si->second->rbegin();
                     rli != si->second->rend(); ++rli )
                {
                    if( rli->second->getLodIndex() < lodIndex )
                    {
                        ret = rli->second;
                        break;
                    }
                }
                if( !ret )
                {
                    // shouldn't ever hit this really, unless user defines no LOD 0
                    // pick the first LOD we have (must be at least one to have a scheme entry)
                    ret = si->second->begin()->second;
                }
            }
            else
            {
                // LOD found
                ret = li->second;
            }

            return ret;
        }
    }
    //-----------------------------------------------------------------------
    void Material::removeTechnique( unsigned short index )
    {
        assert( index < mTechniques.size() && "Index out of bounds." );
        Techniques::iterator i = mTechniques.begin() + index;
        OGRE_DELETE( *i );
        mTechniques.erase( i );
        mSupportedTechniques.clear();
        clearBestTechniqueList();
        mCompilationRequired = true;
    }
    //-----------------------------------------------------------------------
    void Material::removeAllTechniques()
    {
        for( Technique *tech : mTechniques )
        {
            OGRE_DELETE( tech );
        }
        mTechniques.clear();
        mSupportedTechniques.clear();
        clearBestTechniqueList();
        mCompilationRequired = true;
    }
    //-----------------------------------------------------------------------
    Material::TechniqueIterator Material::getTechniqueIterator()
    {
        return TechniqueIterator( mTechniques.begin(), mTechniques.end() );
    }
    //-----------------------------------------------------------------------
    Material::TechniqueIterator Material::getSupportedTechniqueIterator()
    {
        return TechniqueIterator( mSupportedTechniques.begin(), mSupportedTechniques.end() );
    }
    //-----------------------------------------------------------------------
    bool Material::isTransparent() const
    {
        // Check each technique
        for( Technique *tech : mTechniques )
        {
            if( tech->isTransparent() )
                return true;
        }
        return false;
    }
    //-----------------------------------------------------------------------
    void Material::compile( bool autoManageTextureUnits )
    {
        // Compile each technique, then add it to the list of supported techniques
        mSupportedTechniques.clear();
        clearBestTechniqueList();
        mUnsupportedReasons.clear();

        size_t techNo = 0;
        for( Technique *tech : mTechniques )
        {
            String compileMessages = tech->_compile( autoManageTextureUnits );
            if( tech->isSupported() )
            {
                insertSupportedTechnique( tech );
            }
            else
            {
                // Log informational
                StringStream str;
                str << "Material " << mName << " Technique " << techNo;
                if( !tech->getName().empty() )
                    str << "(" << tech->getName() << ")";
                str << " is not supported. " << compileMessages;
                LogManager::getSingleton().logMessage( str.str(), LML_TRIVIAL );
                mUnsupportedReasons += compileMessages;
            }
            ++techNo;
        }

        mCompilationRequired = false;

        // Did we find any?
        if( mSupportedTechniques.empty() )
        {
            LogManager::getSingleton().logMessage(
                "WARNING: material " + mName +
                    " has no supportable Techniques and will be blank. Explanation: \n" +
                    mUnsupportedReasons,
                LML_CRITICAL );
        }
    }
    //-----------------------------------------------------------------------
    void Material::clearBestTechniqueList()
    {
        for( BestTechniquesBySchemeList::iterator i = mBestTechniquesBySchemeList.begin();
             i != mBestTechniquesBySchemeList.end(); ++i )
        {
            OGRE_DELETE_T( i->second, LodTechniques, MEMCATEGORY_RESOURCE );
        }
        mBestTechniquesBySchemeList.clear();
    }
    //-----------------------------------------------------------------------
    void Material::setPointSize( Real ps )
    {
        for( Technique *tech : mTechniques )
            tech->setPointSize( ps );
    }
    //-----------------------------------------------------------------------
    void Material::setAmbient( Real red, Real green, Real blue )
    {
        setAmbient( ColourValue( red, green, blue ) );
    }
    //-----------------------------------------------------------------------
    void Material::setAmbient( const ColourValue &ambient )
    {
        for( Technique *tech : mTechniques )
            tech->setAmbient( ambient );
    }
    //-----------------------------------------------------------------------
    void Material::setDiffuse( Real red, Real green, Real blue, Real alpha )
    {
        for( Technique *tech : mTechniques )
            tech->setDiffuse( red, green, blue, alpha );
    }
    //-----------------------------------------------------------------------
    void Material::setDiffuse( const ColourValue &diffuse )
    {
        setDiffuse( diffuse.r, diffuse.g, diffuse.b, diffuse.a );
    }
    //-----------------------------------------------------------------------
    void Material::setSpecular( Real red, Real green, Real blue, Real alpha )
    {
        for( Technique *tech : mTechniques )
            tech->setSpecular( red, green, blue, alpha );
    }
    //-----------------------------------------------------------------------
    void Material::setSpecular( const ColourValue &specular )
    {
        setSpecular( specular.r, specular.g, specular.b, specular.a );
    }
    //-----------------------------------------------------------------------
    void Material::setShininess( Real val )
    {
        for( Technique *tech : mTechniques )
            tech->setShininess( val );
    }
    //-----------------------------------------------------------------------
    void Material::setSelfIllumination( Real red, Real green, Real blue )
    {
        setSelfIllumination( ColourValue( red, green, blue ) );
    }
    //-----------------------------------------------------------------------
    void Material::setSelfIllumination( const ColourValue &selfIllum )
    {
        for( Technique *tech : mTechniques )
            tech->setSelfIllumination( selfIllum );
    }
    //-----------------------------------------------------------------------
    void Material::setMacroblock( const HlmsMacroblock &macroblock )
    {
        for( Technique *tech : mTechniques )
            tech->setMacroblock( macroblock );
    }
    // --------------------------------------------------------------------
    void Material::setBlendblock( const HlmsBlendblock &blendblock )
    {
        for( Technique *tech : mTechniques )
            tech->setBlendblock( blendblock );
    }
    //-----------------------------------------------------------------------
    void Material::setShadingMode( ShadeOptions mode )
    {
        for( Technique *tech : mTechniques )
            tech->setShadingMode( mode );
    }
    //-----------------------------------------------------------------------
    void Material::setFog( bool overrideScene, FogMode mode, const ColourValue &colour, Real expDensity,
                           Real linearStart, Real linearEnd )
    {
        for( Technique *tech : mTechniques )
            tech->setFog( overrideScene, mode, colour, expDensity, linearStart, linearEnd );
    }
    //-----------------------------------------------------------------------
    void Material::setSamplerblock( const HlmsSamplerblock &samplerblock )
    {
        for( Technique *tech : mTechniques )
            tech->setSamplerblock( samplerblock );
    }
    // --------------------------------------------------------------------
    void Material::_notifyNeedsRecompile()
    {
        mCompilationRequired = true;
        // Also need to unload to ensure we loaded any new items
        if( isLoaded() )  // needed to stop this being called in 'loading' state
            unload();
    }
    // --------------------------------------------------------------------
    void Material::setLodLevels( const LodValueArray &lodValues )
    {
        LodStrategy *lodStrategy = LodStrategyManager::getSingleton().getDefaultStrategy();

        // Square the distances for the internal list
        LodValueArray::const_iterator i, iend;
        iend = lodValues.end();
        // First, clear and add single zero entry
        mLodValues.clear();
        mUserLodValues.clear();
        mUserLodValues.push_back( 0 );
        mLodValues.push_back( LodStrategyManager::getSingleton().getDefaultStrategy()->getBaseValue() );
        for( i = lodValues.begin(); i != iend; ++i )
        {
            mUserLodValues.push_back( *i );
            mLodValues.push_back( lodStrategy->transformUserValue( *i ) );
        }
    }
    // --------------------------------------------------------------------
    Material::LodValueIterator Material::getLodValueIterator() const
    {
        return LodValueIterator( mLodValues.begin(), mLodValues.end() );
    }
    // --------------------------------------------------------------------
    Material::LodValueIterator Material::getUserLodValueIterator() const
    {
        return LodValueIterator( mUserLodValues.begin(), mUserLodValues.end() );
    }

    //-----------------------------------------------------------------------
    bool Material::applyTextureAliases( const AliasTextureNamePairList &aliasList,
                                        const bool apply ) const
    {
        // iterate through all techniques and apply texture aliases
        bool testResult = false;

        for( Technique *tech : mTechniques )
        {
            if( tech->applyTextureAliases( aliasList, apply ) )
                testResult = true;
        }

        return testResult;
    }
    //---------------------------------------------------------------------
}  // namespace Ogre
