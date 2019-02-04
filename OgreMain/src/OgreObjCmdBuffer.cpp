/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2017 Torus Knot Software Ltd

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

#include "OgreObjCmdBuffer.h"
#include "OgreTextureGpu.h"
#include "OgreStagingTexture.h"
#include "OgreTextureGpuManager.h"

#include "OgreId.h"
#include "OgreLwString.h"
#include "OgreCommon.h"

#include "Vao/OgreVaoManager.h"
#include "OgreResourceGroupManager.h"
#include "OgreImage2.h"
#include "OgreTextureFilters.h"

#include "OgreException.h"
#include "OgreProfiler.h"

namespace Ogre
{
    ObjCmdBuffer::~ObjCmdBuffer()
    {
        clear();
    }
    //-----------------------------------------------------------------------------------
    void* ObjCmdBuffer::requestMemory( size_t sizeBytes )
    {
        void *retVal = 0;
        const size_t newSize = mCommandAllocator.size() + sizeBytes;

        if( newSize > mCommandAllocator.capacity() )
        {
            const uint8 *prevBuffStart = mCommandAllocator.begin();
            mCommandAllocator.resize( newSize );
            uint8 *newBuffStart = mCommandAllocator.begin();

            FastArray<Cmd*>::iterator itor = mCommandBuffer.begin();
            FastArray<Cmd*>::iterator end  = mCommandBuffer.end();

            while( itor != end )
            {
                const uintptr_t ptrDiff = reinterpret_cast<uint8*>( *itor ) - prevBuffStart;
                *itor = reinterpret_cast<Cmd*>( newBuffStart + ptrDiff );
                ++itor;
            }
        }
        else
        {
            mCommandAllocator.resize( newSize );
        }

        retVal = mCommandAllocator.end() - sizeBytes;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void ObjCmdBuffer::clear(void)
    {
        FastArray<Cmd*>::const_iterator itor = mCommandBuffer.begin();
        FastArray<Cmd*>::const_iterator end  = mCommandBuffer.end();

        while( itor != end )
        {
            (*itor)->~Cmd();
            ++itor;
        }

        mCommandBuffer.clear();
        mCommandAllocator.clear();
    }
    //-----------------------------------------------------------------------------------
    void ObjCmdBuffer::execute(void)
    {
        FastArray<Cmd*>::const_iterator itor = mCommandBuffer.begin();
        FastArray<Cmd*>::const_iterator end  = mCommandBuffer.end();

        while( itor != end )
        {
            (*itor)->execute();
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    ObjCmdBuffer::
    TransitionToLoaded::TransitionToLoaded( TextureGpu *_texture, void *_sysRamCopy,
                                            GpuResidency::GpuResidency _targetResidency ) :
        texture( _texture ),
        sysRamCopy( _sysRamCopy ),
        targetResidency( _targetResidency )
    {
        OGRE_ASSERT_MEDIUM( targetResidency != GpuResidency::OnStorage );
    }
    //-----------------------------------------------------------------------------------
    void ObjCmdBuffer::TransitionToLoaded::execute(void)
    {
        texture->_transitionTo( targetResidency, reinterpret_cast<uint8*>( sysRamCopy ) );
        OGRE_ASSERT_MEDIUM( !texture->isManualTexture() );

        //Do not update metadata cache when loading from OnStorage to OnSystemRam as
        //it may have tainted (incomplete, mostly mipmaps) data. Only when going Resident.
        if( targetResidency == GpuResidency::Resident )
            texture->getTextureManager()->_updateMetadataCache( texture );
    }
    //-----------------------------------------------------------------------------------
    ObjCmdBuffer::
    OutOfDateCache::OutOfDateCache( TextureGpu *_texture, Image2 &image ) :
        texture( _texture ),
        loadedImage()
    {
        image._setAutoDelete( false );
        loadedImage = image;
        loadedImage._setAutoDelete( true );
    }
    //-----------------------------------------------------------------------------------
    void ObjCmdBuffer::OutOfDateCache::execute(void)
    {
        OGRE_ASSERT_MEDIUM( texture->getResidencyStatus() == GpuResidency::Resident );

        TextureGpuManager *textureManager = texture->getTextureManager();
        textureManager->_setIgnoreScheduledTasks( true );
        texture->notifyAllListenersTextureChanged( TextureGpuListener::MetadataCacheOutOfDate );
        texture->_addPendingResidencyChanges( 1u );
        texture->_transitionTo( GpuResidency::OnStorage, 0 );
        texture->getTextureManager()->_removeMetadataCacheEntry( texture );
        loadedImage._setAutoDelete( false );
        Image2 *image = new Image2( loadedImage );
        image->_setAutoDelete( true );
        texture->unsafeScheduleTransitionTo( GpuResidency::Resident, image, true );
        textureManager->_setIgnoreScheduledTasks( false );
    }
    //-----------------------------------------------------------------------------------
    ObjCmdBuffer::
    ExceptionThrown::ExceptionThrown( TextureGpu *_texture, const Exception &_exception ) :
        texture( _texture ),
        exception( _exception )
    {
    }
    //-----------------------------------------------------------------------------------
    void ObjCmdBuffer::ExceptionThrown::execute(void)
    {
        texture->notifyAllListenersTextureChanged( TextureGpuListener::ExceptionThrown, &exception );
    }
    //-----------------------------------------------------------------------------------
    ObjCmdBuffer::
    UploadFromStagingTex::UploadFromStagingTex( StagingTexture *_stagingTexture, const TextureBox &_box,
                                                TextureGpu *_dstTexture, const TextureBox &_dstBox,
                                                uint8 _mipLevel ) :
        stagingTexture( _stagingTexture ),
        box( _box ),
        dstTexture( _dstTexture ),
        dstBox( _dstBox ),
        mipLevel( _mipLevel )
    {
    }
    //-----------------------------------------------------------------------------------
    void ObjCmdBuffer::UploadFromStagingTex::execute(void)
    {
        OgreProfileExhaustive( "ObjCmdBuffer::UploadFromStagingTex::execute" );
        stagingTexture->upload( box, dstTexture, mipLevel, &dstBox, &dstBox );
    }
    //-----------------------------------------------------------------------------------
    ObjCmdBuffer::
    NotifyDataIsReady::NotifyDataIsReady( TextureGpu *_textureGpu, FilterBaseArray &inOutFilters ) :
        texture( _textureGpu )
    {
        filters.swap( inOutFilters );
    }
    //-----------------------------------------------------------------------------------
    void ObjCmdBuffer::NotifyDataIsReady::execute(void)
    {
        OgreProfileExhaustive( "ObjCmdBuffer::NotifyDataIsReady::execute" );

        FilterBaseArray::const_iterator itor = filters.begin();
        FilterBaseArray::const_iterator end  = filters.end();

        while( itor != end )
        {
            (*itor)->_executeSerial( texture );
            OGRE_DELETE *itor;
            ++itor;
        }
        filters.destroy(); //Destroy manually as ~NotifyDataIsReady won't be called.

        texture->notifyDataIsReady();
    }
}
