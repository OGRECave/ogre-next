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

#include "CommandBuffer/OgreCommandBuffer.h"
#include "CommandBuffer/OgreCbTexture.h"

#include "OgreRenderSystem.h"

namespace Ogre
{
    CbTexture::CbTexture( uint16 _texUnit, TextureGpu *_texture,
                          const HlmsSamplerblock *_samplerBlock ) :
        CbBase( CB_SET_TEXTURE ),
        texUnit( _texUnit ),
        texture( _texture ),
        samplerBlock( _samplerBlock )
    {
    }

    void CommandBuffer::execute_setTexture( CommandBuffer *_this, const CbBase * RESTRICT_ALIAS _cmd )
    {
        const CbTexture *cmd = static_cast<const CbTexture*>( _cmd );
        _this->mRenderSystem->_setTexture( cmd->texUnit, cmd->texture );

        if( cmd->samplerBlock )
            _this->mRenderSystem->_setHlmsSamplerblock( cmd->texUnit, cmd->samplerBlock );
    }

    CbTextures::CbTextures( uint16 _texUnit, uint16 _hazardousTexIdx,
                            const DescriptorSetTexture *_descSet ) :
        CbBase( CB_SET_TEXTURES ),
        texUnit( _texUnit ),
        hazardousTexIdx( _hazardousTexIdx ),
        descSet( _descSet )
    {
    }

    void CommandBuffer::execute_setTextures( CommandBuffer *_this, const CbBase * RESTRICT_ALIAS _cmd )
    {
        const CbTextures *cmd = static_cast<const CbTextures*>( _cmd );
        _this->mRenderSystem->_setTextures( cmd->texUnit, cmd->descSet, cmd->hazardousTexIdx );
    }

    CbSamplers::CbSamplers( uint16 _texUnit, const DescriptorSetSampler *_descSet ) :
        CbBase( CB_SET_SAMPLERS ),
        texUnit( _texUnit ),
        descSet( _descSet )
    {
    }

    void CommandBuffer::execute_setSamplers( CommandBuffer *_this, const CbBase * RESTRICT_ALIAS _cmd )
    {
        const CbSamplers *cmd = static_cast<const CbSamplers*>( _cmd );
        _this->mRenderSystem->_setSamplers( cmd->texUnit, cmd->descSet );
    }
}
