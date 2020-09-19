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

        bool isDirty( uint8 minDirtySlot ) const
        {
            return ( minDirtySlot >= start ) & ( minDirtySlot < end );
        }
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
            ReadOnlyBuffer,
            TexBuffer,
            Texture,
            Sampler,
            UavBuffer,
            UavTexture,
            NumDescBindingTypes
        };
    }  // namespace DescBindingTypes

    /**
    @brief The RootLayout class

        D3D11, OpenGL and Metal use a 'table' model for binding resources.
        While there are a couple differences between the APIs, if we look at HLSL's binding model:

            - There is a special const buffer for uniform parameters
            - All const buffers are in slots [b0; b8)
            - Textures and TexBuffers share the same slots, range [t0; t128)
            - Samplers have their own slots [s0; s16)
            - UAV textures and buffers share the same slots, range [u0; u8)

        Vulkan & D3D12 however don't follow this table model. Instead they distribute
        bindings into sets and bindings. Vulkan guarantees at least 4 sets will be available.

        A binding can contain ANYTHING: sampler, const buffer, textures, etc

        @code
            layout( set = 0, location = 0 ) texture2D myTex;
            layout( set = 0, location = 1 ) sampler mySampler;
            layout( set = 0, location = 2 ) buffer myBuffer { uint myUavArray[4096]; };
            layout( set = 0, location = ... ) ;

            layout( set = 1, location = 0 ) samplerBuffer myTexBuffer;
            layout( set = 1, location = 1 ) sampler anotherSampler;
            layout( set = 1, location = ... ) ;
        @endcode

        Since Ogre has historically followed the 'table' model and we want to reuse existing
        shaders as much as possible; this RootLayout tells Ogre which resources your shader
        is planning to use, so we can create the optimal DescriptorSets.

        For example an HLSL shader that ONLY declares and uses the following resources:

        @code
            cbuffer myConstBuffer0      : register(b4) {};
            cbuffer myConstBuffer1      : register(b5) {};
            cbuffer myConstBuffer2      : register(b6) {};
            Buffer<float4> myTexBuffer  : register(t1);
            Texture2D myTex0            : register(t3);
            Texture2D myTex1            : register(t4);
            SamplerState mySampler      : register(s1);
            RWTexture3D<float> myUavTex : register(u0);
            RWBuffer<float> myUavBuffer : register(u1);
        @endcode

        Would work using the following RootLayout:

        @code
            {
                "0" :
                {
                    "const_buffers"     : [4,7],
                    "tex_buffers"       : [1,2],
                    "textures"          : [3,4],
                    "samplers"          : [1,2],
                    "uav_buffers"       : [1,2],
                    "uav_textures"      : [0,1]
                }
            }
        @endcode

        That is, explicitly declare that you're using const buffer range [4; 7),
        tex buffer range [1; 2) etc.

        In Vulkan we will automatically generate macros for use in bindings
        (buffers are uppercase letter, textures lowercase):

        @code
            // Const buffers
            #define ogre_B4 set = 0, binding = 0
            #define ogre_B5 set = 0, binding = 1
            #define ogre_B6 set = 0, binding = 2

            // Texture buffers
            #define ogre_T1 set = 0, binding = 3

            // Textures
            #define ogre_t3 set = 0, binding = 4

            // Samplers
            #define ogre_s1 set = 0, binding = 5

            // UAV buffers
            #define ogre_U1 set = 0, binding = 6

            // UAV textures
            #define ogre_u0 set = 0, binding = 7
        @endcode

        Thus a GLSL shader can use it like this:
        @code
            layout( ogre_B4 ) uniform bufferName
            {
                uniform float myParam;
                uniform float2 myParam2;
            };

            layout( ogre_t3 ) uniform texture2D myTexture;
            // etc...
        @endcode

        Could you have used e.g. "const_buffers" : [0,7] instead of [4,7]?
        i.e. declare slots in range [0;4) even though they won't be used?

        Yes. But you would be consuming more memory.

        Note that if your vertex shader uses slots [0; 3) and pixel shader uses range
        [4; 7) then BOTH shaders must use a RootLayout that at least declares range [0; 7)
        so they can be paired together

        RootLayouts have a memory vs performance trade off:

            - Declaring unused slots helps reusing RootLayout. Reuse leads to fewer descriptor swapping
            - Unused slots waste RAM and VRAM

        That's why low level materials provide prefab RootLayouts, in order to maximize
        RootLayout reuse while also keeping reasonable memory consumption.
        See GpuProgram::setPrefabRootLayout

        Note that if two shaders were compiled with different RootLayouts, they may still
        be able to be used together if their layouts are compatible; and the 'bigger' RootLayout
        which can be used for both shaders will be selected.

        For example if vertex shader declared:

        @code
            {
                "0" :
                {
                    "const_buffers"     : [0,3]
                }
            }
        @endcode

        and the pixel shader declared:

        @code
            {
                "0" :
                {
                    "const_buffers"     : [0,3],
                    "textures"          : [2,4]
                }
            }
        @endcode

        Then they're compatible; and pixel shader's will be selected to be used for both shaders.

        That is because both layouts declared 3 const buffers (perfect match); and textures
        ALWAYS go after const buffers (THE ORDER is important and determined by
        DescBindingTypes::DescBindingTypes).

        Thus the pixel shader's RootLayout can be used for the vertex shader.

        However if the vertex shader had declared instead:

        @code
            {
                "0" :
                {
                    "const_buffers"     : [0,3],
                    "tex_buffers"       : [0,1]
                }
            }
        @endcode

        then both RootLayouts are incompatible.
        This is because vertex shader uses 3 const_buf slots, then 1 tex buffer slot:

            1. const
            2. const
            3. const
            4. tex buffer

        But where pixel shader uses 3 const_buf slots, then 2 texture slots:

            1. const
            2. const
            3. const
            4. texture --> ERROR. CONFLICT
            5. texture

        The 4th slot is in conflict because the vertex shader uses it for a tex buffer,
        and the pixel shader uses it for a texture. A slot can only be used for one type of resource.

        In order to fix this incompatibility, the pixel shader needs to declare a tex_buffer,
        even if it won't ever use the tex_buffer slot (the vertex shader will):

        @code
            {
                "0" :
                {
                    "const_buffers"     : [0,3],
                    "tex_buffers"       : [0,1],
                    "textures"          : [2,4]
                }
            }
        @endcode

        <b>Baked sets:</b>

        If you intend to use UAVs or you're writing a shader with the Hlms which uses
        DescriptorSetSampler, DescriptorSetTexture or DescriptorSetUav, then you need to use
        a baked set.

        For example if a low level material uses UAVs:

        @code
            cbuffer myConstBuffer0      : register(b4) {};
            cbuffer myConstBuffer1      : register(b5) {};
            cbuffer myConstBuffer2      : register(b6) {};
            Buffer<float4> myTexBuffer  : register(t1);
            Texture2D myTex0            : register(t3);
            Texture2D myTex1            : register(t4);
            SamplerState mySampler      : register(s1);
            RWTexture3D<float> myUavTex : register(u0);
            RWBuffer<float> myUavBuffer : register(u1);
        @endcode

        You'll need a set for baking the UAVs:
        @code
            {
                "0" :
                {
                    "const_buffers"     : [4,7],
                    "tex_buffers"       : [1,2],
                    "textures"          : [3,4],
                    "samplers"          : [1,2]
                },

                "1" :
                {
                    "baked" : true,
                    "uav_buffers"       : [1,2],
                    "uav_textures"      : [0,1]
                }
            }
        @endcode

        If this is a Compute Shader or another Hlms shader, then the textures and samplers also need
        to be in the baked set:

        @code
            {
                "0" :
                {
                    "const_buffers"     : [4,7],
                },

                "1" :
                {
                    "baked" : true,
                    "tex_buffers"       : [1,2],
                    "textures"          : [3,4],
                    "samplers"          : [1,2],
                    "uav_buffers"       : [1,2],
                    "uav_textures"      : [0,1]
                }
            }
        @endcode

        Note that you can use more than one set if e.g. you intend to swap a
        DescriptorSetTexture much more often than the rest:

        @code
            {
                "0" :
                {
                    "const_buffers"     : [4,7],
                },

                "1" :
                {
                    "baked" : true,
                    "samplers"          : [1,2],
                    "uav_buffers"       : [1,2],
                    "uav_textures"      : [0,1]
                },

                "2" :
                {
                    "baked" : true,
                    "tex_buffers"       : [1,2],
                    "textures"          : [3,4]
                }
            }
        @endcode

        For best performance, <b>the last set index should be the one that changes more frequently<b/>
    */
    class _OgreExport RootLayout
    {
    public:
        bool mCompute;
        uint8 mParamsBuffStages;
        bool mBaked[OGRE_MAX_NUM_BOUND_DESCRIPTOR_SETS];
        DescBindingRange mDescBindingRanges[OGRE_MAX_NUM_BOUND_DESCRIPTOR_SETS]
                                           [DescBindingTypes::NumDescBindingTypes];

        void validate( const String &filename ) const;

    protected:
        void parseSet( const rapidjson::Value &jsonValue, const size_t setIdx, const String &filename );

        inline static void flushLwString( LwString &jsonStr, String &outJson );

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
                        "uav_textures" : [16, 32],
                        "baked" : false
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

        /// Dumps the current RootLayout to a JSON string
        void dump( String &outJson ) const;

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
