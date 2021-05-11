/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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

#include "OgreResourceTransition.h"

#include "OgreRenderSystem.h"
#include "OgreTextureGpu.h"
#include "OgreTextureGpuManager.h"

#include "ogrestd/map.h"

namespace Ogre
{
    namespace ResourceAccess
    {
        // clang-format off
        static const char* resourceAccessTable[] =
        {
            "Undefined",
            "Read",
            "Write",
            "ReadWrite"
        };
        // clang-format on

        const char *toString( ResourceAccess value ) { return resourceAccessTable[value]; }
    }  // namespace ResourceAccess

    GpuTrackedResource::~GpuTrackedResource() {}
    //-------------------------------------------------------------------------
    const ResourceStatusMap &BarrierSolver::getResourceStatus( void ) { return mResourceStatus; }
    //-------------------------------------------------------------------------
    void BarrierSolver::reset() { mResourceStatus.clear(); }
    //-------------------------------------------------------------------------
    /**
    @brief BarrierSolver::checkDivergingTransition
        Checks if there's already a transition for this texture and whether
        we're trying to transition to two different layouts (which is impossible)
    @param resourceTransitions
    @param texture
    @param newLayout
    @return
        True if there is already an entry barrier in the barrier to transition this texture
        False otherwise
    */
    bool BarrierSolver::checkDivergingTransition( const ResourceTransitionArray &resourceTransitions,
                                                  const TextureGpu *texture,
                                                  ResourceLayout::Layout newLayout )
    {
        ResourceTransitionArray::const_iterator itor = resourceTransitions.begin();
        ResourceTransitionArray::const_iterator endt = resourceTransitions.end();

        while( itor != endt && itor->resource != texture )
            ++itor;

        if( itor != endt )
        {
            OGRE_ASSERT( itor->newLayout == newLayout &&
                         "Trying to transition a texture to 2 different layouts at the same time" );
        }

        return itor != endt;
    }
    //-------------------------------------------------------------------------
    void BarrierSolver::resolveTransition( ResourceTransitionArray &resourceTransitions,
                                           TextureGpu *texture, ResourceLayout::Layout newLayout,
                                           ResourceAccess::ResourceAccess access, uint8 stageMask )
    {
        OGRE_ASSERT_MEDIUM( newLayout != ResourceLayout::Undefined );
        OGRE_ASSERT_MEDIUM( access != ResourceAccess::Undefined );

        OGRE_ASSERT_MEDIUM(
            ( newLayout == ResourceLayout::Texture || newLayout == ResourceLayout::Uav ||
              newLayout == ResourceLayout::RenderTargetReadOnly || stageMask == 0u ) &&
            "stageMask must be 0 when layouts aren't Texture, Uav or RenderTargetReadOnly" );

        OGRE_ASSERT_MEDIUM(
            ( ( newLayout != ResourceLayout::Texture && newLayout != ResourceLayout::Uav ) ||
              stageMask != 0u ) &&
            "stageMask can't be 0 when layouts are Texture, Uav or RenderTargetReadOnly" );

        OGRE_ASSERT_MEDIUM(
            ( ( newLayout != ResourceLayout::Texture &&
                newLayout != ResourceLayout::RenderTargetReadOnly &&
                newLayout != ResourceLayout::ResolveDest && newLayout != ResourceLayout::CopySrc &&  //
                newLayout != ResourceLayout::CopyDst &&                                              //
                newLayout != ResourceLayout::MipmapGen ) ||
              ( newLayout == ResourceLayout::Texture && access == ResourceAccess::Read ) ||
              ( newLayout == ResourceLayout::CopySrc && access == ResourceAccess::Read ) ||
              ( newLayout == ResourceLayout::CopyDst && access == ResourceAccess::Write ) ||
              ( newLayout == ResourceLayout::MipmapGen && access == ResourceAccess::ReadWrite ) ||
              ( newLayout == ResourceLayout::ResolveDest && access == ResourceAccess::Write ) ||
              ( newLayout == ResourceLayout::RenderTargetReadOnly &&
                access == ResourceAccess::Read ) ) &&
            "Invalid Layout-access pair" );

        OGRE_ASSERT_MEDIUM( access != ResourceAccess::Undefined );

        ResourceStatusMap::iterator itor = mResourceStatus.find( texture );

        if( itor == mResourceStatus.end() )
        {
            ResourceStatus status;
            status.layout = newLayout;
            status.access = access;
            status.stageMask = stageMask;
            mResourceStatus.insert( ResourceStatusMap::value_type( texture, status ) );

            ResourceTransition resTrans;
            resTrans.resource = texture;
            if( texture->isDiscardableContent() )
            {
                resTrans.oldLayout = ResourceLayout::Undefined;
                if( access == ResourceAccess::Read )
                {
                    OGRE_EXCEPT(
                        Exception::ERR_INVALID_STATE,
                        "Transitioning texture " + texture->getNameStr() +
                            " from Undefined to a read-only layout. Perhaps you didn't want to set "
                            "TextureFlags::DiscardableContent / aka keep_content in compositor?",
                        "BarrierSolver::resolveTransition" );
                }
            }
            else
                resTrans.oldLayout = texture->getCurrentLayout();
            resTrans.oldAccess = ResourceAccess::Undefined;
            resTrans.newLayout = newLayout;
            resTrans.newAccess = access;
            resTrans.oldStageMask = 0;
            resTrans.newStageMask = stageMask;
            resourceTransitions.push_back( resTrans );
        }
        else
        {
            RenderSystem *renderSystem = texture->getTextureManager()->getRenderSystem();

            OGRE_ASSERT_MEDIUM( itor->second.layout != ResourceLayout::CopyEncoderManaged &&
                                "Call RenderSystem::endCopyEncoder first!" );

            OGRE_ASSERT_MEDIUM(
                ( checkDivergingTransition( resourceTransitions, texture, newLayout ) ||
                  renderSystem->isSameLayout( itor->second.layout, texture->getCurrentLayout(), texture,
                                              true ) ) &&
                "Layout was altered outside BarrierSolver! "
                "Common reasons are copyTo and _autogenerateMipmaps" );

            if( !renderSystem->isSameLayout( itor->second.layout, newLayout, texture, false ) ||
                ( newLayout == ResourceLayout::Uav &&  //
                  ( access != ResourceAccess::Read ||  //
                    itor->second.access != ResourceAccess::Read ) ) )
            {
                ResourceTransition resTrans;
                resTrans.resource = texture;
                resTrans.oldLayout = itor->second.layout;
                resTrans.newLayout = newLayout;
                resTrans.oldAccess = itor->second.access;
                resTrans.newAccess = access;
                resTrans.oldStageMask = itor->second.stageMask;
                resTrans.newStageMask = stageMask;

                resourceTransitions.push_back( resTrans );

                // After a barrier, the stageMask should be reset
                itor->second.stageMask = 0u;
            }

            itor->second.layout = newLayout;
            itor->second.access = access;
            itor->second.stageMask |= stageMask;
        }
    }
    //-------------------------------------------------------------------------
    void BarrierSolver::resolveTransition( ResourceTransitionArray &resourceTransitions,
                                           GpuTrackedResource *bufferRes,
                                           ResourceAccess::ResourceAccess access, uint8 stageMask )
    {
        OGRE_ASSERT_MEDIUM( access != ResourceAccess::Undefined );

        ResourceStatusMap::iterator itor = mResourceStatus.find( bufferRes );

        if( itor == mResourceStatus.end() )
        {
            ResourceStatus status;
            status.layout = ResourceLayout::Undefined;
            status.access = access;
            status.stageMask = stageMask;
            mResourceStatus.insert( ResourceStatusMap::value_type( bufferRes, status ) );

            // No transition. There's nothing to wait for and unlike textures,
            // buffers have no layout to transition to
        }
        else
        {
            if( access != ResourceAccess::Read || itor->second.access != ResourceAccess::Read )
            {
                ResourceTransition resTrans;
                resTrans.resource = bufferRes;
                resTrans.oldLayout = ResourceLayout::Undefined;
                resTrans.newLayout = ResourceLayout::Undefined;
                resTrans.oldAccess = itor->second.access;
                resTrans.newAccess = access;
                resTrans.oldStageMask = itor->second.stageMask;
                resTrans.newStageMask = stageMask;

                resourceTransitions.push_back( resTrans );

                // After a barrier, the stageMask should be reset
                itor->second.stageMask = 0u;
            }

            itor->second.access = access;
            itor->second.stageMask |= stageMask;
        }
    }
    //-------------------------------------------------------------------------
    void BarrierSolver::assumeTransition( TextureGpu *texture, ResourceLayout::Layout newLayout,
                                          ResourceAccess::ResourceAccess access, uint8 stageMask )
    {
        OGRE_ASSERT_MEDIUM(
            ( ( newLayout != ResourceLayout::Texture &&
                newLayout != ResourceLayout::RenderTargetReadOnly &&
                newLayout != ResourceLayout::CopySrc &&  //
                newLayout != ResourceLayout::CopyDst &&  //
                newLayout != ResourceLayout::MipmapGen ) ||
              ( newLayout == ResourceLayout::Texture && access == ResourceAccess::Read ) ||
              ( newLayout == ResourceLayout::CopySrc && access == ResourceAccess::Read ) ||
              ( newLayout == ResourceLayout::CopyDst && access == ResourceAccess::Write ) ||
              ( newLayout == ResourceLayout::MipmapGen && access == ResourceAccess::ReadWrite ) ||
              ( newLayout == ResourceLayout::RenderTargetReadOnly &&
                access == ResourceAccess::Read ) ) &&
            "Invalid Layout-access pair" );

        ResourceStatus &status = mResourceStatus[texture];
        status.layout = newLayout;
        status.access = access;
        status.stageMask = stageMask;
    }
    //-------------------------------------------------------------------------
    void BarrierSolver::assumeTransitions( ResourceStatusMap &resourceStatus )
    {
        mResourceStatus.insert( resourceStatus.begin(), resourceStatus.end() );
    }
    //-------------------------------------------------------------------------
    void BarrierSolver::textureDeleted( TextureGpu *texture )
    {
        ResourceStatusMap::iterator itor = mResourceStatus.find( texture );

        if( itor != mResourceStatus.end() )
            mResourceStatus.erase( itor );
    }
}  // namespace Ogre
