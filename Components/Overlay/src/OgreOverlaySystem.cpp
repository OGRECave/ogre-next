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

#include "OgreOverlaySystem.h"

#include "OgreCamera.h"
#include "OgreFontManager.h"
#include "OgreOverlayElementFactory.h"
#include "OgreOverlayManager.h"
#include "OgreOverlayProfileSessionListener.h"
#include "OgreRoot.h"
#include "OgreViewport.h"

namespace Ogre
{
    namespace v1
    {
        //---------------------------------------------------------------------
        OverlaySystem::OverlaySystem()
        {
            RenderSystem::addSharedListener( this );

            mOverlayManager = OGRE_NEW OverlayManager();
            mOverlayManager->addOverlayElementFactory( OGRE_NEW PanelOverlayElementFactory() );

            mOverlayManager->addOverlayElementFactory( OGRE_NEW BorderPanelOverlayElementFactory() );

            mOverlayManager->addOverlayElementFactory( OGRE_NEW TextAreaOverlayElementFactory() );

            mFontManager = OGRE_NEW FontManager();
#if OGRE_PROFILING
            mProfileListener = new Ogre::v1::OverlayProfileSessionListener();
            Ogre::Profiler *prof = Ogre::Profiler::getSingletonPtr();
            if( prof )
            {
                prof->addListener( mProfileListener );
            }
#endif
        }
        //---------------------------------------------------------------------
        OverlaySystem::~OverlaySystem()
        {
            RenderSystem::removeSharedListener( this );

#if OGRE_PROFILING
            Ogre::Profiler *prof = Ogre::Profiler::getSingletonPtr();
            if( prof )
            {
                prof->removeListener( mProfileListener );
            }
            delete mProfileListener;
#endif
            OGRE_DELETE mOverlayManager;
            OGRE_DELETE mFontManager;
        }
        //---------------------------------------------------------------------
        void OverlaySystem::renderQueueStarted( RenderQueue *rq, uint8 queueGroupId,
                                                const String &invocation, bool &skipThisInvocation )
        {
            if( queueGroupId == mOverlayManager->mDefaultRenderQueueId )
            {
                Ogre::Viewport *vp =
                    Ogre::Root::getSingletonPtr()->getRenderSystem()->getCurrentRenderViewports();
                if( vp->getOverlaysEnabled() )
                {
                    OverlayManager::getSingleton()._queueOverlaysForRendering( rq, vp );
                }
            }
        }
        //---------------------------------------------------------------------
        void OverlaySystem::eventOccurred( const String &eventName, const NameValuePairList *parameters )
        {
            if( eventName == "DeviceLost" )
            {
                mOverlayManager->_releaseManualHardwareResources();
            }
            else if( eventName == "DeviceRestored" )
            {
                mOverlayManager->_restoreManualHardwareResources();
            }
        }
        //---------------------------------------------------------------------
    }  // namespace v1
}  // namespace Ogre
