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

#include "OgreParticleSystemManager.h"

#include "OgreBillboardParticleRenderer.h"
#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreParticleAffectorFactory.h"
#include "OgreParticleEmitterFactory.h"
#include "OgreParticleSystem.h"
#include "OgreParticleSystemRenderer.h"
#include "OgreRoot.h"
#include "OgreScriptCompiler.h"

namespace Ogre
{
    //-----------------------------------------------------------------------
    // Shortcut to set up billboard particle renderer
    v1::BillboardParticleRendererFactory *mBillboardRendererFactory = 0;
    //-----------------------------------------------------------------------
    template <>
    ParticleSystemManager *Singleton<ParticleSystemManager>::msSingleton = 0;
    ParticleSystemManager *ParticleSystemManager::getSingletonPtr() { return msSingleton; }
    ParticleSystemManager &ParticleSystemManager::getSingleton()
    {
        assert( msSingleton );
        return ( *msSingleton );
    }
    //-----------------------------------------------------------------------
    ParticleSystemManager::ParticleSystemManager()
    {
        OGRE_LOCK_AUTO_MUTEX;
        mFactory = OGRE_NEW ParticleSystemFactory();
        Root::getSingleton().addMovableObjectFactory( mFactory );
    }
    //-----------------------------------------------------------------------
    ParticleSystemManager::~ParticleSystemManager()
    {
        OGRE_LOCK_AUTO_MUTEX;

        // Destroy all templates
        ParticleTemplateMap::iterator t;
        for( t = mSystemTemplates.begin(); t != mSystemTemplates.end(); ++t )
        {
            OGRE_DELETE t->second;
        }
        mSystemTemplates.clear();
        ResourceGroupManager::getSingleton()._unregisterScriptLoader( this );
        // delete billboard factory
        if( mBillboardRendererFactory )
        {
            OGRE_DELETE mBillboardRendererFactory;
            mBillboardRendererFactory = 0;
        }

        if( mFactory )
        {
            // delete particle system factory
            Root::getSingleton().removeMovableObjectFactory( mFactory );
            OGRE_DELETE mFactory;
            mFactory = 0;
        }
    }
    //-----------------------------------------------------------------------
    const StringVector &ParticleSystemManager::getScriptPatterns() const { return mScriptPatterns; }
    //-----------------------------------------------------------------------
    Real ParticleSystemManager::getLoadingOrder() const
    {
        /// Load late
        return 1000.0f;
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::parseScript( DataStreamPtr &stream, const String &groupName )
    {
        ScriptCompilerManager::getSingleton().parseScript( stream, groupName );
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::addEmitterFactory( ParticleEmitterFactory *factory )
    {
        OGRE_LOCK_AUTO_MUTEX;
        String name = factory->getName();
        mEmitterFactories[name] = factory;
        LogManager::getSingleton().logMessage( "Particle Emitter Type '" + name + "' registered" );
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::addAffectorFactory( ParticleAffectorFactory *factory )
    {
        OGRE_LOCK_AUTO_MUTEX;
        String name = factory->getName();
        mAffectorFactories[name] = factory;
        LogManager::getSingleton().logMessage( "Particle Affector Type '" + name + "' registered" );
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::addRendererFactory( ParticleSystemRendererFactory *factory )
    {
        OGRE_LOCK_AUTO_MUTEX;
        String name = factory->getType();
        mRendererFactories[name] = factory;
        LogManager::getSingleton().logMessage( "Particle Renderer Type '" + name + "' registered" );
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::addTemplate( const String &name, ParticleSystem *sysTemplate )
    {
        OGRE_LOCK_AUTO_MUTEX;
        // check name
        if( mSystemTemplates.find( name ) != mSystemTemplates.end() )
        {
            OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                         "ParticleSystem template with name '" + name + "' already exists.",
                         "ParticleSystemManager::addTemplate" );
        }

        mSystemTemplates[name] = sysTemplate;
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::removeTemplate( const String &name, bool deleteTemplate )
    {
        OGRE_LOCK_AUTO_MUTEX;
        ParticleTemplateMap::iterator itr = mSystemTemplates.find( name );
        if( itr == mSystemTemplates.end() )
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "ParticleSystem template with name '" + name + "' cannot be found.",
                         "ParticleSystemManager::removeTemplate" );

        if( deleteTemplate )
            OGRE_DELETE itr->second;

        mSystemTemplates.erase( itr );
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::removeAllTemplates( bool deleteTemplate )
    {
        OGRE_LOCK_AUTO_MUTEX;
        if( deleteTemplate )
        {
            ParticleTemplateMap::iterator itr;
            for( itr = mSystemTemplates.begin(); itr != mSystemTemplates.end(); ++itr )
                OGRE_DELETE itr->second;
        }

        mSystemTemplates.clear();
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::removeTemplatesByResourceGroup( const String &resourceGroup )
    {
        OGRE_LOCK_AUTO_MUTEX;

        ParticleTemplateMap::iterator i = mSystemTemplates.begin();
        while( i != mSystemTemplates.end() )
        {
            ParticleTemplateMap::iterator icur = i++;

            if( icur->second->getResourceGroupName() == resourceGroup )
            {
                delete icur->second;
                mSystemTemplates.erase( icur );
            }
        }
    }
    //-----------------------------------------------------------------------
    ParticleSystem *ParticleSystemManager::createTemplate( const String &name,
                                                           const String &resourceGroup )
    {
        OGRE_LOCK_AUTO_MUTEX;
        // check name
        if( mSystemTemplates.find( name ) != mSystemTemplates.end() )
        {
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT
            LogManager::getSingleton().logMessage( "ParticleSystem template with name '" + name +
                                                   "' already exists." );
            return NULL;
#else
            OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                         "ParticleSystem template with name '" + name + "' already exists.",
                         "ParticleSystemManager::createTemplate" );
#endif
        }

        ParticleSystem *tpl =
            OGRE_NEW ParticleSystem( Id::generateNewId<ParticleSystem>(), &mTemplatesObjectMemMgr,
                                     (SceneManager *)0, resourceGroup );
        tpl->setName( name );
        addTemplate( name, tpl );
        return tpl;
    }
    //-----------------------------------------------------------------------
    ParticleSystem *ParticleSystemManager::getTemplate( const String &name )
    {
        OGRE_LOCK_AUTO_MUTEX;
        ParticleTemplateMap::iterator i = mSystemTemplates.find( name );
        if( i != mSystemTemplates.end() )
        {
            return i->second;
        }
        else
        {
            return 0;
        }
    }
    //-----------------------------------------------------------------------
    ParticleSystem *ParticleSystemManager::createSystemImpl( IdType id,
                                                             ObjectMemoryManager *objectMemoryManager,
                                                             SceneManager *manager, size_t quota,
                                                             const String &resourceGroup )
    {
        ParticleSystem *sys = OGRE_NEW ParticleSystem( id, objectMemoryManager, manager, resourceGroup );
        sys->setParticleQuota( quota );
        return sys;
    }
    //-----------------------------------------------------------------------
    ParticleSystem *ParticleSystemManager::createSystemImpl( IdType id,
                                                             ObjectMemoryManager *objectMemoryManager,
                                                             SceneManager *manager,
                                                             const String &templateName )
    {
        // Look up template
        ParticleSystem *pTemplate = getTemplate( templateName );
        if( !pTemplate )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Cannot find required template '" + templateName + "'",
                         "ParticleSystemManager::createSystem" );
        }

        ParticleSystem *sys =
            createSystemImpl( id, objectMemoryManager, manager, pTemplate->getParticleQuota(),
                              pTemplate->getResourceGroupName() );
        // Copy template settings
        *sys = *pTemplate;
        return sys;
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::destroySystemImpl( ParticleSystem *sys ) { OGRE_DELETE sys; }
    //-----------------------------------------------------------------------
    ParticleEmitter *ParticleSystemManager::_createEmitter( const String &emitterType,
                                                            ParticleSystem *psys )
    {
        OGRE_LOCK_AUTO_MUTEX;
        // Locate emitter type
        ParticleEmitterFactoryMap::iterator pFact = mEmitterFactories.find( emitterType );

        if( pFact == mEmitterFactories.end() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Cannot find requested emitter type.",
                         "ParticleSystemManager::_createEmitter" );
        }

        return pFact->second->createEmitter( psys );
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::_destroyEmitter( ParticleEmitter *emitter )
    {
        if( !emitter )
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Cannot destroy a null ParticleEmitter.",
                         "ParticleSystemManager::_destroyEmitter" );

        OGRE_LOCK_AUTO_MUTEX;
        // Destroy using the factory which created it
        ParticleEmitterFactoryMap::iterator pFact = mEmitterFactories.find( emitter->getType() );

        if( pFact == mEmitterFactories.end() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Cannot find emitter factory to destroy emitter.",
                         "ParticleSystemManager::_destroyEmitter" );
        }

        pFact->second->destroyEmitter( emitter );
    }
    //-----------------------------------------------------------------------
    ParticleAffector *ParticleSystemManager::_createAffector( const String &affectorType,
                                                              ParticleSystem *psys )
    {
        OGRE_LOCK_AUTO_MUTEX;
        // Locate affector type
        ParticleAffectorFactoryMap::iterator pFact = mAffectorFactories.find( affectorType );

        if( pFact == mAffectorFactories.end() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Cannot find requested affector type.",
                         "ParticleSystemManager::_createAffector" );
        }

        return pFact->second->createAffector( psys );
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::_destroyAffector( ParticleAffector *affector )
    {
        if( !affector )
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Cannot destroy a null ParticleAffector.",
                         "ParticleSystemManager::_destroyAffector" );

        OGRE_LOCK_AUTO_MUTEX;
        // Destroy using the factory which created it
        ParticleAffectorFactoryMap::iterator pFact = mAffectorFactories.find( affector->getType() );

        if( pFact == mAffectorFactories.end() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Cannot find affector factory to destroy affector.",
                         "ParticleSystemManager::_destroyAffector" );
        }

        pFact->second->destroyAffector( affector );
    }
    //-----------------------------------------------------------------------
    ParticleSystemRenderer *ParticleSystemManager::_createRenderer( const String &rendererType,
                                                                    SceneManager *sceneManager )
    {
        OGRE_LOCK_AUTO_MUTEX;
        // Locate affector type
        ParticleSystemRendererFactoryMap::iterator pFact = mRendererFactories.find( rendererType );

        if( pFact == mRendererFactories.end() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Cannot find requested renderer type.",
                         "ParticleSystemManager::_createRenderer" );
        }

        pFact->second->mCurrentSceneManager = sceneManager;
        return pFact->second->createInstance( rendererType );
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::_destroyRenderer( ParticleSystemRenderer *renderer )
    {
        if( !renderer )
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Cannot destroy a null ParticleSystemRenderer.",
                         "ParticleSystemManager::_destroyRenderer" );

        OGRE_LOCK_AUTO_MUTEX;
        // Destroy using the factory which created it
        ParticleSystemRendererFactoryMap::iterator pFact =
            mRendererFactories.find( renderer->getType() );

        if( pFact == mRendererFactories.end() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Cannot find renderer factory to destroy renderer.",
                         "ParticleSystemManager::_destroyRenderer" );
        }

        pFact->second->destroyInstance( renderer );
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::_initialise()
    {
        OGRE_LOCK_AUTO_MUTEX;
        // Create Billboard renderer factory
        mBillboardRendererFactory = OGRE_NEW v1::BillboardParticleRendererFactory();
        addRendererFactory( mBillboardRendererFactory );
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::parseNewEmitter( const String &type, DataStreamPtr &stream,
                                                 ParticleSystem *sys )
    {
        // Create new emitter
        ParticleEmitter *pEmit = sys->addEmitter( type );
        // Parse emitter details
        String line;

        while( !stream->eof() )
        {
            line = stream->getLine();
            // Ignore comments & blanks
            if( !( line.length() == 0 || line.substr( 0, 2 ) == "//" ) )
            {
                if( line == "}" )
                {
                    // Finished emitter
                    break;
                }
                else
                {
                    // Attribute
                    StringUtil::toLowerCase( line );
                    parseEmitterAttrib( line, pEmit );
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::parseNewAffector( const String &type, DataStreamPtr &stream,
                                                  ParticleSystem *sys )
    {
        // Create new affector
        ParticleAffector *pAff = sys->addAffector( type );
        // Parse affector details
        String line;

        while( !stream->eof() )
        {
            line = stream->getLine();
            // Ignore comments & blanks
            if( !( line.length() == 0 || line.substr( 0, 2 ) == "//" ) )
            {
                if( line == "}" )
                {
                    // Finished affector
                    break;
                }
                else
                {
                    // Attribute
                    StringUtil::toLowerCase( line );
                    parseAffectorAttrib( line, pAff );
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::parseAttrib( const String &line, ParticleSystem *sys )
    {
        // Split params on space
        vector<String>::type vecparams = StringUtil::split( line, "\t ", 1 );

        // Look up first param (command setting)
        if( !sys->setParameter( vecparams[0], vecparams[1] ) )
        {
            // Attribute not supported by particle system, try the renderer
            ParticleSystemRenderer *renderer = sys->getRenderer();
            if( renderer )
            {
                if( !renderer->setParameter( vecparams[0], vecparams[1] ) )
                {
                    LogManager::getSingleton().logMessage( "Bad particle system attribute line: '" +
                                                               line + "' in " + sys->getName() +
                                                               " (tried renderer)",
                                                           LML_CRITICAL );
                }
            }
            else
            {
                // BAD command. BAD!
                LogManager::getSingleton().logMessage( "Bad particle system attribute line: '" + line +
                                                           "' in " + sys->getName() + " (no renderer)",
                                                       LML_CRITICAL );
            }
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::parseEmitterAttrib( const String &line, ParticleEmitter *emit )
    {
        // Split params on first space
        vector<String>::type vecparams = StringUtil::split( line, "\t ", 1 );

        // Look up first param (command setting)
        if( !emit->setParameter( vecparams[0], vecparams[1] ) )
        {
            // BAD command. BAD!
            LogManager::getSingleton().logMessage(
                "Bad particle emitter attribute line: '" + line + "' for emitter " + emit->getType(),
                LML_CRITICAL );
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::parseAffectorAttrib( const String &line, ParticleAffector *aff )
    {
        // Split params on space
        vector<String>::type vecparams = StringUtil::split( line, "\t ", 1 );

        // Look up first param (command setting)
        if( !aff->setParameter( vecparams[0], vecparams[1] ) )
        {
            // BAD command. BAD!
            LogManager::getSingleton().logMessage(
                "Bad particle affector attribute line: '" + line + "' for affector " + aff->getType(),
                LML_CRITICAL );
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::skipToNextCloseBrace( DataStreamPtr &stream )
    {
        String line;
        while( !stream->eof() && line != "}" )
        {
            line = stream->getLine();
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::skipToNextOpenBrace( DataStreamPtr &stream )
    {
        String line;
        while( !stream->eof() && line != "{" )
        {
            line = stream->getLine();
        }
    }
    //-----------------------------------------------------------------------
    ParticleSystemManager::ParticleAffectorFactoryIterator
    ParticleSystemManager::getAffectorFactoryIterator()
    {
        return ParticleAffectorFactoryIterator( mAffectorFactories.begin(), mAffectorFactories.end() );
    }
    //-----------------------------------------------------------------------
    ParticleSystemManager::ParticleEmitterFactoryIterator
    ParticleSystemManager::getEmitterFactoryIterator()
    {
        return ParticleEmitterFactoryIterator( mEmitterFactories.begin(), mEmitterFactories.end() );
    }
    //-----------------------------------------------------------------------
    ParticleSystemManager::ParticleRendererFactoryIterator
    ParticleSystemManager::getRendererFactoryIterator()
    {
        return ParticleRendererFactoryIterator( mRendererFactories.begin(), mRendererFactories.end() );
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    String ParticleSystemFactory::FACTORY_TYPE_NAME = "ParticleSystem";
    //-----------------------------------------------------------------------
    MovableObject *ParticleSystemFactory::createInstanceImpl( IdType id,
                                                              ObjectMemoryManager *objectMemoryManager,
                                                              SceneManager *manager,
                                                              const NameValuePairList *params )
    {
        if( params != 0 )
        {
            NameValuePairList::const_iterator ni = params->find( "templateName" );
            if( ni != params->end() )
            {
                String templateName = ni->second;
                // create using manager
                return ParticleSystemManager::getSingleton().createSystemImpl( id, objectMemoryManager,
                                                                               manager, templateName );
            }
        }
        // Not template based, look for quota & resource name
        size_t quota = 500;
        String resourceGroup = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
        if( params != 0 )
        {
            NameValuePairList::const_iterator ni = params->find( "quota" );
            if( ni != params->end() )
            {
                quota = StringConverter::parseUnsignedInt( ni->second );
            }
            ni = params->find( "resourceGroup" );
            if( ni != params->end() )
            {
                resourceGroup = ni->second;
            }
        }
        // create using manager
        return ParticleSystemManager::getSingleton().createSystemImpl( id, objectMemoryManager, manager,
                                                                       quota, resourceGroup );
    }
    //-----------------------------------------------------------------------
    const String &ParticleSystemFactory::getType() const { return FACTORY_TYPE_NAME; }
    //-----------------------------------------------------------------------
    void ParticleSystemFactory::destroyInstance( MovableObject *obj )
    {
        // use manager
        ParticleSystemManager::getSingleton().destroySystemImpl( static_cast<ParticleSystem *>( obj ) );
    }
    //-----------------------------------------------------------------------
}  // namespace Ogre
