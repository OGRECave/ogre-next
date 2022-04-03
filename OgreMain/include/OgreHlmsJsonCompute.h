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
#ifndef _OgreHlmsJsonCompute_H_
#define _OgreHlmsJsonCompute_H_

#include "OgreHlmsJson.h"

#if !OGRE_NO_JSON
#    include "OgreResourceTransition.h"

#    include "OgreHeaderPrefix.h"

namespace Ogre
{
    class ShaderParams;

    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */
    /** HLMS stands for "High Level Material System". */
    class _OgreExport HlmsJsonCompute : public OgreAllocatedObj
    {
        HlmsManager       *mHlmsManager;
        TextureGpuManager *mTextureManager;

        static uint8                          parseAccess( const char *value );
        static ResourceAccess::ResourceAccess parseAccess( const rapidjson::Value &json );

        void loadParams( const rapidjson::Value &jsonArray, ShaderParams &shaderParams,
                         const String &jobName );
        void loadTexture( const rapidjson::Value &json, const HlmsJson::NamedBlocks &blocks,
                          HlmsComputeJob *job, uint8 slotIdx, const String &resourceGroup );

        void loadBasedOnTextureOrUav( const rapidjson::Value &objValue, const String &jobName,
                                      HlmsComputeJob *job, int threadGroupsBasedOn );

    public:
        HlmsJsonCompute( HlmsManager *hlmsManager, TextureGpuManager *textureManager );

        void loadJobs( const rapidjson::Value &json, const HlmsJson::NamedBlocks &blocks,
                       const String &resourceGroup );

        void loadJob( const rapidjson::Value &json, const HlmsJson::NamedBlocks &blocks,
                      HlmsComputeJob *job, const String &jobName, const String &resourceGroup );
        void saveJob( const HlmsComputeJob *job, String &outString );
    };
    /** @} */
    /** @} */

}  // namespace Ogre

#    include "OgreHeaderSuffix.h"

#endif

#endif
