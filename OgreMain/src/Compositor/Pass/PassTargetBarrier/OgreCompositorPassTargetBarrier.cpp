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

#include "Compositor/Pass/PassTargetBarrier/OgreCompositorPassTargetBarrier.h"

#include "Compositor/OgreCompositorNode.h"
#include "Compositor/Pass/PassTargetBarrier/OgreCompositorPassTargetBarrierDef.h"
#include "OgreRenderSystem.h"

namespace Ogre
{
    CompositorPassTargetBarrierDef::CompositorPassTargetBarrierDef(
        CompositorTargetDef *parentTargetDef ) :
        CompositorPassDef( PASS_TARGET_BARRIER, parentTargetDef )
    {
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    CompositorPassTargetBarrier::CompositorPassTargetBarrier(
        const CompositorPassTargetBarrierDef *definition, CompositorNode *parentNode,
        const size_t numPasses ) :
        CompositorPass( definition, parentNode )
    {
        initialize( 0, true );
        mPasses.reserve( numPasses );
    }
    //-----------------------------------------------------------------------------------
    CompositorPassTargetBarrier::~CompositorPassTargetBarrier() {}
    //-----------------------------------------------------------------------------------
    void CompositorPassTargetBarrier::addPass( CompositorPass *pass ) { mPasses.push_back( pass ); }
    //-----------------------------------------------------------------------------------
    void CompositorPassTargetBarrier::execute( const Camera * )
    {
        // Execute a limited number of times?
        if( mNumPassesLeft != std::numeric_limits<uint32>::max() )
        {
            if( !mNumPassesLeft )
                return;
            --mNumPassesLeft;
        }

        profilingBegin();

        notifyPassEarlyPreExecuteListeners();

        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        renderSystem->endRenderPassDescriptor();

        // Fire the listener in case it wants to change anything
        notifyPassPreExecuteListeners();

        mResourceTransitions.clear();
        FastArray<CompositorPass *>::const_iterator itor = mPasses.begin();
        FastArray<CompositorPass *>::const_iterator endt = mPasses.end();

        while( itor != endt )
        {
            ResourceTransitionArray &barriers = ( *itor )->_getResourceTransitionsNonConst();
            barriers.clear();
            barriers.swap( mResourceTransitions );
            ( *itor )->analyzeBarriers( false );
            barriers.swap( mResourceTransitions );
            ++itor;
        }
        executeResourceTransitions();

        notifyPassPosExecuteListeners();

        profilingEnd();
    }
}  // namespace Ogre
