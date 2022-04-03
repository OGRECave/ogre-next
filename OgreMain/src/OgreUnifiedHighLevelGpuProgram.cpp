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

#include "OgreException.h"
#include "OgreGpuProgramManager.h"
#include "OgreUnifiedHighLevelGpuProgram.h"

namespace Ogre
{
    //-----------------------------------------------------------------------
    UnifiedHighLevelGpuProgram::CmdDelegate UnifiedHighLevelGpuProgram::msCmdDelegate;
    static const String sLanguage = "unified";
    std::map<String, int> UnifiedHighLevelGpuProgram::mLanguagePriorities;

    int UnifiedHighLevelGpuProgram::getPriority( String shaderLanguage )
    {
        std::map<String, int>::iterator it = mLanguagePriorities.find( shaderLanguage );
        if( it == mLanguagePriorities.end() )
            return -1;
        else
            return ( *it ).second;
    }

    void UnifiedHighLevelGpuProgram::setPrioriry( String shaderLanguage, int priority )
    {
        mLanguagePriorities[shaderLanguage] = priority;
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    UnifiedHighLevelGpuProgram::UnifiedHighLevelGpuProgram( ResourceManager *creator, const String &name,
                                                            ResourceHandle handle, const String &group,
                                                            bool isManual,
                                                            ManualResourceLoader *loader ) :
        HighLevelGpuProgram( creator, name, handle, group, isManual, loader )
    {
        if( createParamDictionary( "UnifiedHighLevelGpuProgram" ) )
        {
            setupBaseParamDictionary();

            ParamDictionary *dict = getParamDictionary();

            dict->addParameter(
                ParameterDef( "delegate", "Additional delegate programs containing implementations.",
                              PT_STRING ),
                &msCmdDelegate );
        }
    }
    //-----------------------------------------------------------------------
    UnifiedHighLevelGpuProgram::~UnifiedHighLevelGpuProgram() {}
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::chooseDelegate() const
    {
        OGRE_LOCK_AUTO_MUTEX;

        mChosenDelegate.reset();

        HighLevelGpuProgramPtr tmpDelegate;
        tmpDelegate.reset();
        int tmpPriority = -1;

        for( StringVector::const_iterator i = mDelegateNames.begin(); i != mDelegateNames.end(); ++i )
        {
            HighLevelGpuProgramPtr deleg = HighLevelGpuProgramManager::getSingleton().getByName( *i );

            // Silently ignore missing links
            if( deleg && deleg->isSupported() )
            {
                int priority = getPriority( deleg->getLanguage() );
                // Find the delegate with the highest prioriry
                if( priority >= tmpPriority )
                {
                    tmpDelegate = deleg;
                    tmpPriority = priority;
                }
            }
        }

        mChosenDelegate = tmpDelegate;
    }
    //-----------------------------------------------------------------------
    const HighLevelGpuProgramPtr &UnifiedHighLevelGpuProgram::_getDelegate() const
    {
        if( !mChosenDelegate )
        {
            chooseDelegate();
        }
        return mChosenDelegate;
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::addDelegateProgram( const String &name )
    {
        OGRE_LOCK_AUTO_MUTEX;

        mDelegateNames.push_back( name );

        // reset chosen delegate
        mChosenDelegate.reset();
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::clearDelegatePrograms()
    {
        OGRE_LOCK_AUTO_MUTEX;

        mDelegateNames.clear();
        mChosenDelegate.reset();
    }
    //-----------------------------------------------------------------------------
    size_t UnifiedHighLevelGpuProgram::calculateSize() const
    {
        size_t memSize = 0;

        memSize += HighLevelGpuProgram::calculateSize();

        // Delegate Names
        for( StringVector::const_iterator i = mDelegateNames.begin(); i != mDelegateNames.end(); ++i )
            memSize += ( *i ).size() * sizeof( char );

        return memSize;
    }
    //-----------------------------------------------------------------------
    const String &UnifiedHighLevelGpuProgram::getLanguage() const { return sLanguage; }
    //-----------------------------------------------------------------------
    GpuProgramParametersSharedPtr UnifiedHighLevelGpuProgram::createParameters()
    {
        if( isSupported() )
        {
            return _getDelegate()->createParameters();
        }
        else
        {
            // return a default set
            GpuProgramParametersSharedPtr params = GpuProgramManager::getSingleton().createParameters();
            // avoid any errors on parameter names that don't exist
            params->setIgnoreMissingParams( true );
            return params;
        }
    }
    //-----------------------------------------------------------------------
    GpuProgram *UnifiedHighLevelGpuProgram::_getBindingDelegate()
    {
        if( auto& d = _getDelegate() )
            return d->_getBindingDelegate();
        else
            return 0;
    }
    //-----------------------------------------------------------------------
    bool UnifiedHighLevelGpuProgram::isSupported() const
    {
        // Supported if one of the delegates is
        return _getDelegate() != nullptr;
    }
    //-----------------------------------------------------------------------
    bool UnifiedHighLevelGpuProgram::isSkeletalAnimationIncluded() const
    {
        if( auto& d = _getDelegate() )
            return d->isSkeletalAnimationIncluded();
        else
            return false;
    }
    //-----------------------------------------------------------------------
    bool UnifiedHighLevelGpuProgram::isMorphAnimationIncluded() const
    {
        if( auto& d = _getDelegate() )
            return d->isMorphAnimationIncluded();
        else
            return false;
    }
    //-----------------------------------------------------------------------
    bool UnifiedHighLevelGpuProgram::isPoseAnimationIncluded() const
    {
        if( auto& d = _getDelegate() )
            return d->isPoseAnimationIncluded();
        else
            return false;
    }
    //-----------------------------------------------------------------------
    ushort UnifiedHighLevelGpuProgram::getNumberOfPosesIncluded() const
    {
        if( auto& d = _getDelegate() )
            return d->getNumberOfPosesIncluded();
        else
            return 0;
    }
    //-----------------------------------------------------------------------
    bool UnifiedHighLevelGpuProgram::isVertexTextureFetchRequired() const
    {
        if( auto& d = _getDelegate() )
            return d->isVertexTextureFetchRequired();
        else
            return false;
    }
    //-----------------------------------------------------------------------
    GpuProgramParametersSharedPtr UnifiedHighLevelGpuProgram::getDefaultParameters()
    {
        if( auto& d = _getDelegate() )
            return d->getDefaultParameters();
        else
            return GpuProgramParametersSharedPtr();
    }
    //-----------------------------------------------------------------------
    bool UnifiedHighLevelGpuProgram::hasDefaultParameters() const
    {
        if( auto& d = _getDelegate() )
            return d->hasDefaultParameters();
        else
            return false;
    }
    //-----------------------------------------------------------------------
    bool UnifiedHighLevelGpuProgram::getPassSurfaceAndLightStates() const
    {
        if( auto& d = _getDelegate() )
            return d->getPassSurfaceAndLightStates();
        else
            return HighLevelGpuProgram::getPassSurfaceAndLightStates();
    }
    //---------------------------------------------------------------------
    bool UnifiedHighLevelGpuProgram::getPassFogStates() const
    {
        if( auto& d = _getDelegate() )
            return d->getPassFogStates();
        else
            return HighLevelGpuProgram::getPassFogStates();
    }
    //---------------------------------------------------------------------
    bool UnifiedHighLevelGpuProgram::getPassTransformStates() const
    {
        if( auto& d = _getDelegate() )
            return d->getPassTransformStates();
        else
            return HighLevelGpuProgram::getPassTransformStates();
    }
    //-----------------------------------------------------------------------
    bool UnifiedHighLevelGpuProgram::hasCompileError() const
    {
        if( auto& d = _getDelegate() )
            return d->hasCompileError();
        else
            return false;
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::resetCompileError()
    {
        if( auto& d = _getDelegate() )
            d->resetCompileError();
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::load( bool backgroundThread )
    {
        if( auto& d = _getDelegate() )
            d->load( backgroundThread );
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::reload( LoadingFlags flags )
    {
        if( auto& d = _getDelegate() )
            d->reload( flags );
    }
    //-----------------------------------------------------------------------
    bool UnifiedHighLevelGpuProgram::isReloadable() const
    {
        if( auto& d = _getDelegate() )
            return d->isReloadable();
        else
            return true;
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::unload()
    {
        if( auto& d = _getDelegate() )
            d->unload();
    }
    //-----------------------------------------------------------------------
    bool UnifiedHighLevelGpuProgram::isLoaded() const
    {
        if( auto& d = _getDelegate() )
            return d->isLoaded();
        else
            return false;
    }
    //-----------------------------------------------------------------------
    bool UnifiedHighLevelGpuProgram::isLoading() const
    {
        if( auto& d = _getDelegate() )
            return d->isLoading();
        else
            return false;
    }
    //-----------------------------------------------------------------------
    Resource::LoadingState UnifiedHighLevelGpuProgram::getLoadingState() const
    {
        if( auto& d = _getDelegate() )
            return d->getLoadingState();
        else
            return Resource::LOADSTATE_UNLOADED;
    }
    //-----------------------------------------------------------------------
    size_t UnifiedHighLevelGpuProgram::getSize() const
    {
        if( auto& d = _getDelegate() )
            return d->getSize();
        else
            return 0;
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::touch()
    {
        if( auto& d = _getDelegate() )
            d->touch();
    }
    //-----------------------------------------------------------------------
    bool UnifiedHighLevelGpuProgram::isBackgroundLoaded() const
    {
        if( auto& d = _getDelegate() )
            return d->isBackgroundLoaded();
        else
            return false;
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::setBackgroundLoaded( bool bl )
    {
        if( auto& d = _getDelegate() )
            d->setBackgroundLoaded( bl );
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::escalateLoading()
    {
        if( auto& d = _getDelegate() )
            d->escalateLoading();
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::addListener( Resource::Listener *lis )
    {
        if( auto& d = _getDelegate() )
            d->addListener( lis );
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::removeListener( Resource::Listener *lis )
    {
        if( auto& d = _getDelegate() )
            d->removeListener( lis );
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::createLowLevelImpl()
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "This method should never get called!",
                     "UnifiedHighLevelGpuProgram::createLowLevelImpl" );
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::unloadHighLevelImpl()
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "This method should never get called!",
                     "UnifiedHighLevelGpuProgram::unloadHighLevelImpl" );
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::buildConstantDefinitions() const
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "This method should never get called!",
                     "UnifiedHighLevelGpuProgram::buildConstantDefinitions" );
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::loadFromSource()
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "This method should never get called!",
                     "UnifiedHighLevelGpuProgram::loadFromSource" );
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    String UnifiedHighLevelGpuProgram::CmdDelegate::doGet( const void *target ) const
    {
        // Can't do this (not one delegate), shouldn't matter
        return BLANKSTRING;
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::CmdDelegate::doSet( void *target, const String &val )
    {
        static_cast<UnifiedHighLevelGpuProgram *>( target )->addDelegateProgram( val );
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    UnifiedHighLevelGpuProgramFactory::UnifiedHighLevelGpuProgramFactory() {}
    //-----------------------------------------------------------------------
    UnifiedHighLevelGpuProgramFactory::~UnifiedHighLevelGpuProgramFactory() {}
    //-----------------------------------------------------------------------
    const String &UnifiedHighLevelGpuProgramFactory::getLanguage() const { return sLanguage; }
    //-----------------------------------------------------------------------
    HighLevelGpuProgram *UnifiedHighLevelGpuProgramFactory::create( ResourceManager *creator,
                                                                    const String &name,
                                                                    ResourceHandle handle,
                                                                    const String &group, bool isManual,
                                                                    ManualResourceLoader *loader )
    {
        return OGRE_NEW UnifiedHighLevelGpuProgram( creator, name, handle, group, isManual, loader );
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgramFactory::destroy( HighLevelGpuProgram *prog ) { OGRE_DELETE prog; }
    //-----------------------------------------------------------------------

}  // namespace Ogre
