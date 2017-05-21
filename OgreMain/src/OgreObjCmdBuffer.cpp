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

#include "OgreObjCmdBuffer.h"
#include "OgreTextureGpu.h"
#include "OgreStagingTexture.h"

#include "OgreId.h"
#include "OgreLwString.h"
#include "OgreCommon.h"

#include "Vao/OgreVaoManager.h"
#include "OgreResourceGroupManager.h"
#include "OgreImage2.h"

#include "OgreException.h"

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
    TransitionToResident::TransitionToResident( TextureGpu *_texture, void *_sysRamCopy ) :
        texture( _texture ),
        sysRamCopy( _sysRamCopy )
    {
    }
    //-----------------------------------------------------------------------------------
    void ObjCmdBuffer::TransitionToResident::execute(void)
    {
        texture->_transitionTo( GpuResidency::Resident, reinterpret_cast<uint8*>( sysRamCopy ) );
    }
    //-----------------------------------------------------------------------------------
    ObjCmdBuffer::
    UploadFromStagingTex::UploadFromStagingTex( StagingTexture *_stagingTexture, const TextureBox &_box,
                                                TextureGpu *_dstTexture, uint8 _mipLevel ) :
        stagingTexture( _stagingTexture ),
        box( _box ),
        dstTexture( _dstTexture ),
        mipLevel( _mipLevel )
    {
    }
    //-----------------------------------------------------------------------------------
    void ObjCmdBuffer::UploadFromStagingTex::execute(void)
    {
        stagingTexture->upload( box, dstTexture, mipLevel );
    }
    //-----------------------------------------------------------------------------------
    ObjCmdBuffer::
    NotifyDataIsReady::NotifyDataIsReady( TextureGpu *_textureGpu ) :
        texture( _textureGpu )
    {
    }
    //-----------------------------------------------------------------------------------
    void ObjCmdBuffer::NotifyDataIsReady::execute(void)
    {
        texture->notifyDataIsReady();
    }
}
