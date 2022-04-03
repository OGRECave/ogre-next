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
#ifndef _OgreCbTexture_H_
#define _OgreCbTexture_H_

#include "CommandBuffer/OgreCbCommon.h"

namespace Ogre
{
    struct _OgreExport CbTexture : public CbBase
    {
        uint16                  texUnit;
        bool                    bDepthReadOnly;
        TextureGpu             *texture;
        HlmsSamplerblock const *samplerBlock;

        CbTexture( uint16 _texUnit, TextureGpu *_texture, const HlmsSamplerblock *_samplerBlock = 0,
                   bool _bDepthReadOnly = false );
    };

    struct _OgreExport CbTextures : public CbBase
    {
        uint16                      texUnit;
        uint16                      hazardousTexIdx;
        const DescriptorSetTexture *descSet;

        CbTextures( uint16 _texUnit, uint16 _hazardousTexIdx, const DescriptorSetTexture *_descSet );
    };

    struct _OgreExport CbSamplers : public CbBase
    {
        uint16                      texUnit;
        const DescriptorSetSampler *descSet;

        CbSamplers( uint16 _texUnit, const DescriptorSetSampler *_descSet );
    };
}  // namespace Ogre

#endif
