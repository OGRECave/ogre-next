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

#include "OgreHighLevelGpuProgramManager.h"

#include "OgreUnifiedHighLevelGpuProgram.h"

namespace Ogre
{
    String sNullLang = "null";
    class NullProgram final : public HighLevelGpuProgram
    {
    protected:
        /** Internal load implementation, must be implemented by subclasses.
         */
        void loadFromSource() override {}
        /** Internal method for creating an appropriate low-level program from this
        high-level program, must be implemented by subclasses. */
        void createLowLevelImpl() override {}
        /// Internal unload implementation, must be implemented by subclasses
        void unloadHighLevelImpl() override {}
        /// Populate the passed parameters with name->index map, must be overridden
        void populateParameterNames( GpuProgramParametersSharedPtr params ) override
        {
            // Skip the normal implementation
            // Ensure we don't complain about missing parameter names
            params->setIgnoreMissingParams( true );
        }
        void buildConstantDefinitions() const override
        {
            // do nothing
        }

    public:
        NullProgram( ResourceManager *creator, const String &name, ResourceHandle handle,
                     const String &group, bool isManual, ManualResourceLoader *loader ) :
            HighLevelGpuProgram( creator, name, handle, group, isManual, loader )
        {
        }
        ~NullProgram() override {}
        /// Overridden from GpuProgram - never supported
        bool isSupported() const override { return false; }
        /// Overridden from GpuProgram
        const String &getLanguage() const override { return sNullLang; }
        size_t calculateSize() const override { return 0; }

        /// Overridden from StringInterface
        bool setParameter( const String & /*name*/, const String & /*value*/ ) override
        {
            // always silently ignore all parameters so as not to report errors on
            // unsupported platforms
            return true;
        }
    };
    class NullProgramFactory final : public HighLevelGpuProgramFactory
    {
    public:
        NullProgramFactory() {}
        ~NullProgramFactory() override {}
        /// Get the name of the language this factory creates programs for
        const String &getLanguage() const override { return sNullLang; }
        HighLevelGpuProgram *create( ResourceManager *creator, const String &name, ResourceHandle handle,
                                     const String &group, bool isManual,
                                     ManualResourceLoader *loader ) override
        {
            return OGRE_NEW NullProgram( creator, name, handle, group, isManual, loader );
        }
        void destroy( HighLevelGpuProgram *prog ) override { OGRE_DELETE prog; }
    };
    //-----------------------------------------------------------------------
    template <>
    HighLevelGpuProgramManager *Singleton<HighLevelGpuProgramManager>::msSingleton = 0;
    HighLevelGpuProgramManager *HighLevelGpuProgramManager::getSingletonPtr() { return msSingleton; }
    HighLevelGpuProgramManager &HighLevelGpuProgramManager::getSingleton()
    {
        assert( msSingleton );
        return ( *msSingleton );
    }
    //-----------------------------------------------------------------------
    HighLevelGpuProgramManager::HighLevelGpuProgramManager()
    {
        // Loading order
        mLoadOrder = 50.0f;
        // Resource type
        mResourceType = "HighLevelGpuProgram";

        ResourceGroupManager::getSingleton()._registerResourceManager( mResourceType, this );

        mNullFactory = OGRE_NEW NullProgramFactory();
        addFactory( mNullFactory );
        mUnifiedFactory = OGRE_NEW UnifiedHighLevelGpuProgramFactory();
        addFactory( mUnifiedFactory );
    }
    //-----------------------------------------------------------------------
    HighLevelGpuProgramManager::~HighLevelGpuProgramManager()
    {
        OGRE_DELETE mUnifiedFactory;
        OGRE_DELETE mNullFactory;
        ResourceGroupManager::getSingleton()._unregisterResourceManager( mResourceType );
    }
    //---------------------------------------------------------------------------
    void HighLevelGpuProgramManager::addFactory( HighLevelGpuProgramFactory *factory )
    {
        // deliberately allow later plugins to override earlier ones
        mFactories[factory->getLanguage()] = factory;
    }
    //---------------------------------------------------------------------------
    void HighLevelGpuProgramManager::removeFactory( HighLevelGpuProgramFactory *factory )
    {
        // Remove only if equal to registered one, since it might overridden
        // by other plugins
        FactoryMap::iterator it = mFactories.find( factory->getLanguage() );
        if( it != mFactories.end() && it->second == factory )
        {
            mFactories.erase( it );
        }
    }
    //---------------------------------------------------------------------------
    HighLevelGpuProgramFactory *HighLevelGpuProgramManager::getFactory( const String &language )
    {
        FactoryMap::iterator i = mFactories.find( language );

        if( i == mFactories.end() )
        {
            // use the null factory to create programs that will never be supported
            i = mFactories.find( sNullLang );
        }
        return i->second;
    }
    //---------------------------------------------------------------------
    bool HighLevelGpuProgramManager::isLanguageSupported( const String &lang )
    {
        FactoryMap::iterator i = mFactories.find( lang );

        return i != mFactories.end();
    }
    //---------------------------------------------------------------------------
    Resource *HighLevelGpuProgramManager::createImpl( const String &name, ResourceHandle handle,
                                                      const String &group, bool isManual,
                                                      ManualResourceLoader *loader,
                                                      const NameValuePairList *params )
    {
        NameValuePairList::const_iterator paramIt;

        if( !params || ( paramIt = params->find( "language" ) ) == params->end() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "You must supply a 'language' parameter",
                         "HighLevelGpuProgramManager::createImpl" );
        }

        return getFactory( paramIt->second )
            ->create( this, name, getNextHandle(), group, isManual, loader );
    }
    //-----------------------------------------------------------------------
    HighLevelGpuProgramPtr HighLevelGpuProgramManager::getByName( const String &name,
                                                                  const String &groupName )
    {
        return std::static_pointer_cast<HighLevelGpuProgram>( getResourceByName( name, groupName ) );
    }
    //---------------------------------------------------------------------------
    HighLevelGpuProgramPtr HighLevelGpuProgramManager::createProgram( const String &name,
                                                                      const String &groupName,
                                                                      const String &language,
                                                                      GpuProgramType gptype )
    {
        ResourcePtr ret = ResourcePtr(
            getFactory( language )->create( this, name, getNextHandle(), groupName, false, 0 ) );

        HighLevelGpuProgramPtr prg = std::static_pointer_cast<HighLevelGpuProgram>( ret );
        prg->setType( gptype );
        prg->setSyntaxCode( language );

        addImpl( ret );
        // Tell resource group manager
        ResourceGroupManager::getSingleton()._notifyResourceCreated( ret );
        return prg;
    }
    //---------------------------------------------------------------------------
    HighLevelGpuProgramFactory::~HighLevelGpuProgramFactory() {}
}  // namespace Ogre
