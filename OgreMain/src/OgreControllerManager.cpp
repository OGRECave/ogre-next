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

#include "OgreControllerManager.h"

#include "OgrePredefinedControllers.h"
#include "OgreRoot.h"

namespace Ogre
{
    //-----------------------------------------------------------------------
    template <>
    ControllerManager *Singleton<ControllerManager>::msSingleton = 0;
    ControllerManager *ControllerManager::getSingletonPtr() { return msSingleton; }
    ControllerManager &ControllerManager::getSingleton()
    {
        assert( msSingleton );
        return ( *msSingleton );
    }
    //-----------------------------------------------------------------------
    ControllerManager::ControllerManager() :
        mFrameTimeController( OGRE_NEW FrameTimeControllerValue() ),
        mPassthroughFunction( OGRE_NEW PassthroughControllerFunction() ),
        mLastFrameNumber( 0 )
    {
    }
    //-----------------------------------------------------------------------
    ControllerManager::~ControllerManager() { clearControllers(); }
    //-----------------------------------------------------------------------
    Controller<Real> *ControllerManager::createController( const ControllerValueRealPtr &src,
                                                           const ControllerValueRealPtr &dest,
                                                           const ControllerFunctionRealPtr &func )
    {
        Controller<Real> *c = OGRE_NEW Controller<Real>( src, dest, func );

        mControllers.insert( c );
        return c;
    }
    //-----------------------------------------------------------------------
    Controller<Real> *ControllerManager::createFrameTimePassthroughController(
        const ControllerValueRealPtr &dest )
    {
        return createController( getFrameTimeSource(), dest, getPassthroughControllerFunction() );
    }
    //-----------------------------------------------------------------------
    void ControllerManager::updateAllControllers()
    {
        // Only update once per frame
        unsigned long thisFrameNumber = Root::getSingleton().getNextFrameNumber();
        if( thisFrameNumber != mLastFrameNumber )
        {
            ControllerList::const_iterator ci;
            for( ci = mControllers.begin(); ci != mControllers.end(); ++ci )
            {
                ( *ci )->update();
            }
            mLastFrameNumber = thisFrameNumber;
        }
    }
    //-----------------------------------------------------------------------
    void ControllerManager::clearControllers()
    {
        ControllerList::iterator ci;
        for( ci = mControllers.begin(); ci != mControllers.end(); ++ci )
        {
            OGRE_DELETE *ci;
        }
        mControllers.clear();
    }
    //-----------------------------------------------------------------------
    const ControllerValueRealPtr &ControllerManager::getFrameTimeSource() const
    {
        return mFrameTimeController;
    }
    //-----------------------------------------------------------------------
    const ControllerFunctionRealPtr &ControllerManager::getPassthroughControllerFunction() const
    {
        return mPassthroughFunction;
    }
    //-----------------------------------------------------------------------
    Controller<Real> *ControllerManager::createTextureAnimator( TextureUnitState *layer,
                                                                Real sequenceTime )
    {
        SharedPtr<ControllerValue<Real> > texVal( OGRE_NEW TextureFrameControllerValue( layer ) );
        SharedPtr<ControllerFunction<Real> > animFunc(
            OGRE_NEW AnimationControllerFunction( sequenceTime ) );

        return createController( mFrameTimeController, texVal, animFunc );
    }
    //-----------------------------------------------------------------------
    Controller<Real> *ControllerManager::createTextureUVScroller( TextureUnitState *layer, Real speed )
    {
        Controller<Real> *ret = 0;

        if( speed != 0 )
        {
            SharedPtr<ControllerValue<Real> > val;
            SharedPtr<ControllerFunction<Real> > func;

            // We do both scrolls with a single controller
            val.reset( OGRE_NEW TexCoordModifierControllerValue( layer, true, true ) );
            // Create function: use -speed since we're altering texture coords so they have reverse
            // effect
            func.reset( OGRE_NEW ScaleControllerFunction( -speed, true ) );
            ret = createController( mFrameTimeController, val, func );
        }

        return ret;
    }
    //-----------------------------------------------------------------------
    Controller<Real> *ControllerManager::createTextureUScroller( TextureUnitState *layer, Real uSpeed )
    {
        Controller<Real> *ret = 0;

        if( uSpeed != 0 )
        {
            SharedPtr<ControllerValue<Real> > uVal;
            SharedPtr<ControllerFunction<Real> > uFunc;

            uVal.reset( OGRE_NEW TexCoordModifierControllerValue( layer, true ) );
            // Create function: use -speed since we're altering texture coords so they have reverse
            // effect
            uFunc.reset( OGRE_NEW ScaleControllerFunction( -uSpeed, true ) );
            ret = createController( mFrameTimeController, uVal, uFunc );
        }

        return ret;
    }
    //-----------------------------------------------------------------------
    Controller<Real> *ControllerManager::createTextureVScroller( TextureUnitState *layer, Real vSpeed )
    {
        Controller<Real> *ret = 0;

        if( vSpeed != 0 )
        {
            SharedPtr<ControllerValue<Real> > vVal;
            SharedPtr<ControllerFunction<Real> > vFunc;

            // Set up a second controller for v scroll
            vVal.reset( OGRE_NEW TexCoordModifierControllerValue( layer, false, true ) );
            // Create function: use -speed since we're altering texture coords so they have reverse
            // effect
            vFunc.reset( OGRE_NEW ScaleControllerFunction( -vSpeed, true ) );
            ret = createController( mFrameTimeController, vVal, vFunc );
        }

        return ret;
    }
    //-----------------------------------------------------------------------
    Controller<Real> *ControllerManager::createTextureRotater( TextureUnitState *layer, Real speed )
    {
        SharedPtr<ControllerValue<Real> > val;
        SharedPtr<ControllerFunction<Real> > func;

        // Target value is texture coord rotation
        val.reset( OGRE_NEW TexCoordModifierControllerValue( layer, false, false, false, false, true ) );
        // Function is simple scale (seconds * speed)
        // Use -speed since altering texture coords has the reverse visible effect
        func.reset( OGRE_NEW ScaleControllerFunction( -speed, true ) );

        return createController( mFrameTimeController, val, func );
    }
    //-----------------------------------------------------------------------
    Controller<Real> *ControllerManager::createTextureWaveTransformer(
        TextureUnitState *layer, TextureUnitState::TextureTransformType ttype, WaveformType waveType,
        Real base, Real frequency, Real phase, Real amplitude )
    {
        SharedPtr<ControllerValue<Real> > val;
        SharedPtr<ControllerFunction<Real> > func;

        switch( ttype )
        {
        case TextureUnitState::TT_TRANSLATE_U:
            // Target value is a u scroll
            val.reset( OGRE_NEW TexCoordModifierControllerValue( layer, true ) );
            break;
        case TextureUnitState::TT_TRANSLATE_V:
            // Target value is a v scroll
            val.reset( OGRE_NEW TexCoordModifierControllerValue( layer, false, true ) );
            break;
        case TextureUnitState::TT_SCALE_U:
            // Target value is a u scale
            val.reset( OGRE_NEW TexCoordModifierControllerValue( layer, false, false, true ) );
            break;
        case TextureUnitState::TT_SCALE_V:
            // Target value is a v scale
            val.reset( OGRE_NEW TexCoordModifierControllerValue( layer, false, false, false, true ) );
            break;
        case TextureUnitState::TT_ROTATE:
            // Target value is texture coord rotation
            val.reset(
                OGRE_NEW TexCoordModifierControllerValue( layer, false, false, false, false, true ) );
            break;
        }
        // Create new wave function for alterations
        func.reset(
            OGRE_NEW WaveformControllerFunction( waveType, base, frequency, phase, amplitude, true ) );

        return createController( mFrameTimeController, val, func );
    }
    //-----------------------------------------------------------------------
    Controller<Real> *ControllerManager::createGpuProgramTimerParam(
        GpuProgramParametersSharedPtr params, size_t paramIndex, Real timeFactor )
    {
        SharedPtr<ControllerValue<Real> > val;
        SharedPtr<ControllerFunction<Real> > func;

        val.reset( OGRE_NEW FloatGpuParameterControllerValue( params, paramIndex ) );
        func.reset( OGRE_NEW ScaleControllerFunction( timeFactor, true ) );

        return createController( mFrameTimeController, val, func );
    }
    //-----------------------------------------------------------------------
    void ControllerManager::destroyController( Controller<Real> *controller )
    {
        ControllerList::iterator i = mControllers.find( controller );
        if( i != mControllers.end() )
        {
            mControllers.erase( i );
            OGRE_DELETE controller;
        }
    }
    //-----------------------------------------------------------------------
    Real ControllerManager::getTimeFactor() const
    {
        return static_cast<const FrameTimeControllerValue *>( mFrameTimeController.get() )
            ->getTimeFactor();
    }
    //-----------------------------------------------------------------------
    void ControllerManager::setTimeFactor( Real tf )
    {
        static_cast<FrameTimeControllerValue *>( mFrameTimeController.get() )
            ->setTimeFactor( tf );
    }
    //-----------------------------------------------------------------------
    Real ControllerManager::getFrameDelay() const
    {
        return static_cast<const FrameTimeControllerValue *>( mFrameTimeController.get() )
            ->getFrameDelay();
    }
    //-----------------------------------------------------------------------
    void ControllerManager::setFrameDelay( Real fd )
    {
        static_cast<FrameTimeControllerValue *>( mFrameTimeController.get() )
            ->setFrameDelay( fd );
    }
    //-----------------------------------------------------------------------
    Real ControllerManager::getElapsedTime() const
    {
        return static_cast<const FrameTimeControllerValue *>( mFrameTimeController.get() )
            ->getElapsedTime();
    }
    //-----------------------------------------------------------------------
    void ControllerManager::setElapsedTime( Real elapsedTime )
    {
        static_cast<FrameTimeControllerValue *>( mFrameTimeController.get() )
            ->setElapsedTime( elapsedTime );
    }
}  // namespace Ogre
