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

#ifndef _OgreHlmsJsonUnlit_H_
#define _OgreHlmsJsonUnlit_H_

#include "OgreHlmsUnlitPrerequisites.h"

#if !OGRE_NO_JSON

#    include "OgreHlmsJson.h"
#    include "OgreHlmsUnlitDatablock.h"

#    include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Component
     *  @{
     */
    /** \addtogroup Material
     *  @{
     */

    class _OgreHlmsUnlitExport HlmsJsonUnlit
    {
        HlmsManager       *mHlmsManager;
        TextureGpuManager *mTextureManager;

        UnlitBlendModes parseBlendMode( const char *value );
        void            parseAnimation( const rapidjson::Value &jsonArray, Matrix4 &mat );

        void loadTexture( const rapidjson::Value &json, const HlmsJson::NamedBlocks &blocks,
                          uint8 textureType, HlmsUnlitDatablock *datablock,
                          const String &resourceGroup );

        void saveTexture( const char *blockName, uint8 textureType, const HlmsUnlitDatablock *datablock,
                          String &outString, bool writeTexture = true );

    public:
        HlmsJsonUnlit( HlmsManager *hlmsManager, TextureGpuManager *textureManager );

        void loadMaterial( const rapidjson::Value &json, const HlmsJson::NamedBlocks &blocks,
                           HlmsDatablock *datablock, const String &resourceGroup );
        void saveMaterial( const HlmsDatablock *datablock, String &outString );

        static void collectSamplerblocks( const HlmsDatablock                 *datablock,
                                          set<const HlmsSamplerblock *>::type &outSamplerblocks );
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#    include "OgreHeaderSuffix.h"

#endif

#endif
