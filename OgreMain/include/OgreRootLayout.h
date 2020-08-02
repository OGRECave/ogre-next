/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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
#ifndef _OgreRootLayout_H_
#define _OgreRootLayout_H_

#include "OgrePrerequisites.h"

// Forward declaration for |Document|.
namespace rapidjson
{
    class CrtAllocator;
    template <typename>
    class MemoryPoolAllocator;
    template <typename>
    struct UTF8;

    template <typename BaseAllocator>
    class MemoryPoolAllocator;
    template <typename Encoding, typename>
    class GenericValue;
    typedef GenericValue<UTF8<char>, MemoryPoolAllocator<CrtAllocator> > Value;
}  // namespace rapidjson

namespace Ogre
{
#define OGRE_MAX_NUM_BOUND_DESCRIPTOR_SETS 4
    struct DescBindingRange
    {
        uint16 start;  // Inclusive
        uint16 end;    // Exclusive
        DescBindingRange();

        size_t getNumUsedSlots( void ) const { return end - start; }
        bool isInUse( void ) const { return start < end; }
        bool isValid( void ) const { return start <= end; }
    };

    namespace DescBindingTypes
    {
        /// The order is important as it affects compatibility between root layouts
        ///
        /// If the relative order is changed, then the following needs to be modified:
        ///     - VulkanRootLayout::bind
        ///     - RootLayout::findParamsBuffer (if ParamBuffer stops being the 1st)
        ///     - c_rootLayoutVarNames
        ///     - c_bufferTypes (Vulkan)
        enum DescBindingTypes
        {
            ParamBuffer,
            ConstBuffer,
            TexBuffer,
            Texture,
            Sampler,
            UavTexture,
            UavBuffer,
            NumDescBindingTypes
        };
    }  // namespace DescBindingTypes

    class _OgreExport RootLayout
    {
    public:
        bool mCompute;
        uint8 mParamsBuffStages;
        DescBindingRange mDescBindingRanges[OGRE_MAX_NUM_BOUND_DESCRIPTOR_SETS]
                                           [DescBindingTypes::NumDescBindingTypes];

    protected:
        void validate( const String &filename ) const;
        void parseSet( const rapidjson::Value &jsonValue, const size_t setIdx, const String &filename );

        size_t calculateNumUsedSets( void ) const;
        size_t calculateNumBindings( const size_t setIdx ) const;

    public:
        RootLayout();

        /** Copies all our parameters from 'other'
            Does NOT call validate()
        @param other
        */
        void copyFrom( const RootLayout &other );

        /** Parses a root layout definition from a JSON string
            The JSON string:

            @code
                {
                    "0" :
                    {
                        "has_params" : ["all", "vs", "gs", "hs", "ds", "ps", "cs"],
                        "const_buffers" : [0, 16],
                        "tex_buffers" : [1, 16],
                        "textures" : [0, 1],
                        "samplers" : [0, 1],
                        "uav_buffers" : [0, 16]
                        "uav_textures" : [16, 32]
                    }
                }
            @endcode

            has_params can establish which shader stages allow parameters.
                - "all" means all shader stages (vs through ps for graphics, cs for compute)

            Note that for compatibility with other APIs, textures and tex_buffers cannot overlap
            Same with uav_buffers and uav_textures
        @param rootLayout
            JSON string containing root layout
        @param bCompute
            True if this is meant for compute. False for graphics
        @param filename
            Filename for logging purposes if errors are found
        */
        void parseRootLayout( const char *rootLayout, const bool bCompute, const String &filename );

        /** Retrieves the set and binding idx of the params buffer
        @param shaderStage
            See GpuProgramType
        @param outSetIdx [out]
            Set in which it is located
            Value will not be modified if we return false
        @param outBindingIdx [out]
            Binding index in which it is located
            Value will not be modified if we return false
        @return
            True if there is a params buffer
            False otherwise and output params won't be modified
        */
        bool findParamsBuffer( uint32 shaderStage, size_t &outSetIdx, size_t &outBindingIdx ) const;

        const DescBindingRange *getDescBindingRanges( size_t setIdx ) const
        {
            return mDescBindingRanges[setIdx];
        }
    };
}  // namespace Ogre

#endif  // __Program_H__
