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
#ifndef _OgreHlmsPso_H_
#define _OgreHlmsPso_H_

#include "OgreGpuProgram.h"
#include "OgrePixelFormatGpu.h"
#include "Vao/OgreVertexBufferPacked.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */

    /** IT'S MEMBERS MUST BE KEPT POD (Otherwise HlmsPso needs to be modified).
    @par
        Padding bytes should be zeroed/copied for properly testing via memcmp.
    */
    struct HlmsPassPso
    {
        /// Stencil support
        StencilParams stencilParams;

        /// PF_NULL if no colour attachment is used.
        PixelFormatGpu colourFormat[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        /// PF_NULL if it won't be resolved (MSAA)
        PixelFormatGpu resolveColourFormat[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        /// PF_NULL if no depth buffer is used.
        PixelFormatGpu depthFormat;

        /// FSAA specific settings
        SampleDescription sampleDescription;

        /// For multi-GPU support
        uint32 adapterId;

        bool operator==( const HlmsPassPso &_r ) const
        {
            for( size_t i = 0u; i < OGRE_MAX_MULTIPLE_RENDER_TARGETS; ++i )
            {
                if( this->colourFormat[i] != _r.colourFormat[i] )
                    return false;
            }
            return !( this->stencilParams != _r.stencilParams ) &&     //
                   this->depthFormat == _r.depthFormat &&              //
                   this->sampleDescription == _r.sampleDescription &&  //
                   this->adapterId == _r.adapterId;
        }
        bool operator!=( const HlmsPassPso &_r ) const { return !( *this == _r ); }
        bool operator<( const HlmsPassPso &other ) const
        {
            if( this->stencilParams != other.stencilParams )
                return this->stencilParams < other.stencilParams;

            for( size_t i = 0u; i < OGRE_MAX_MULTIPLE_RENDER_TARGETS; ++i )
            {
                if( this->colourFormat[i] != other.colourFormat[i] )
                    return this->colourFormat[i] < other.colourFormat[i];
            }

            if( this->depthFormat != other.depthFormat )
                return this->depthFormat < other.depthFormat;
            if( this->sampleDescription != other.sampleDescription )
                return this->sampleDescription < other.sampleDescription;

            return this->adapterId < other.adapterId;
        }
    };

    /** Defines a PipelineStateObject as required by Vulkan, Metal & DX12.
    @remarks
        Some RenderSystem-specific quirks:
            In OpenGL, vertex-input data (vertexElements, operationType, enablePrimitiveRestart)
            is ignored, and controlled via the VertexArrayObject pointer.

            In the other APIs, vertex-input data is use, and VertexArrayObject pointers
            only control which vertex and index buffers are bound to the device.
    @par
        Padding bytes should be zeroed/copied for properly testing via memcmp.
    */
    struct HlmsPso
    {
        GpuProgramPtr vertexShader;
        GpuProgramPtr geometryShader;
        GpuProgramPtr tesselationHullShader;
        GpuProgramPtr tesselationDomainShader;
        GpuProgramPtr pixelShader;

        // Computed from the VertexArrayObject (or v1 equivalent)
        VertexElement2VecVec vertexElements;
        // --- POD part starts here ---
        OperationType operationType;
        bool          enablePrimitiveRestart;
        uint8         clipDistances;  // Bitmask. Only needed by GL.

        uint8                 strongBlocks;  // @see StrongBlocks enum.
        HlmsMacroblock const *macroblock;
        HlmsBlendblock const *blendblock;
        // No independent blenblocks for now
        //      HlmsBlendblock const    *blendblock[8];
        //      bool                    independentBlend;

        /// The values for strongBasicBlocks member
        // clang-format off
        enum StrongBlocks
        {
            HasStrongMacroblock = 1u << 0u, ///< If set, the macroblock was overridden and the HlmsPso holds a strong ref.
            HasStrongBlendblock = 1u << 1u  ///< If set, the blendblock was overridden and the HlmsPso holds a strong ref.
        };
        // clang-format on

        // TODO: Stream Out.
        //-dark_sylinc update: Stream Out seems to be dying.
        // It was hard to setup, applications are limited,
        // UAVs are easier and more flexible/powerful.

        // --- Begin Pass data ---
        uint32      sampleMask;  ///< Fixed to 0xffffffff for now
        HlmsPassPso pass;
        // --- End Pass data ---

        void *rsData;  ///< Render-System specific data

        HlmsPso() { initialize(); }

        HlmsPso( const HlmsPso &_r ) { *this = _r; }

        void initialize()
        {
            // Zero out POD part including padding bytes.
            memset( &operationType, 0,
                    static_cast<size_t>( (const uint8 *)this + sizeof( HlmsPso ) -
                                         (const uint8 *)&this->operationType ) );
            operationType = OT_POINT_LIST;
        }

        HlmsPso &operator=( const HlmsPso &_r )
        {
            // Copy non-POD part.
            this->vertexShader = _r.vertexShader;
            this->geometryShader = _r.geometryShader;
            this->tesselationHullShader = _r.tesselationHullShader;
            this->tesselationDomainShader = _r.tesselationDomainShader;
            this->pixelShader = _r.pixelShader;
            this->vertexElements = _r.vertexElements;

            // Copy POD part including padding bytes.
            memcpy( &operationType, &_r.operationType,
                    static_cast<size_t>( (const uint8 *)this + sizeof( HlmsPso ) -
                                         (const uint8 *)&this->operationType ) );
            return *this;
        }

        bool equalNonPod( const HlmsPso &_r ) const
        {
            return this->vertexShader == _r.vertexShader && this->geometryShader == _r.geometryShader &&
                   this->tesselationHullShader == _r.tesselationHullShader &&
                   this->tesselationDomainShader == _r.tesselationDomainShader &&
                   this->pixelShader == _r.pixelShader && this->vertexElements == _r.vertexElements;
        }
        int lessNonPod( const HlmsPso &_r ) const
        {
            if( this->vertexShader < _r.vertexShader )
                return -1;
            if( this->vertexShader != _r.vertexShader )
                return 1;
            if( this->geometryShader < _r.geometryShader )
                return -1;
            if( this->geometryShader != _r.geometryShader )
                return 1;
            if( this->tesselationHullShader < _r.tesselationHullShader )
                return -1;
            if( this->tesselationHullShader != _r.tesselationHullShader )
                return 1;
            if( this->tesselationDomainShader < _r.tesselationDomainShader )
                return -1;
            if( this->tesselationDomainShader != _r.tesselationDomainShader )
                return 1;
            if( this->pixelShader < _r.pixelShader )
                return -1;
            if( this->pixelShader != _r.pixelShader )
                return 1;
            if( this->vertexElements < _r.vertexElements )
                return -1;
            if( this->vertexElements != _r.vertexElements )
                return 1;

            return 0;
        }

        /// Compares if this == _r but only accounting data that is independent of a pass
        /// (and is typically part of a renderable with a material already assigned).
        bool equalExcludePassData( const HlmsPso &_r ) const
        {
            // Non-POD datatypes.
            return equalNonPod( _r ) &&
                   // POD datatypes
                   memcmp( &this->operationType, &_r.operationType,
                           static_cast<size_t>( (const uint8 *)&this->sampleMask -
                                                (const uint8 *)&this->operationType ) ) == 0;
        }
        /// Compares if this <= _r. See equalExcludePassData
        bool lessThanExcludePassData( const HlmsPso &_r ) const
        {
            // Non-POD datatypes.
            int nonPodResult = lessNonPod( _r );
            if( nonPodResult != 0 )
                return nonPodResult < 0;

            // POD datatypes
            return memcmp( &this->operationType, &_r.operationType,
                           static_cast<size_t>( (const uint8 *)&this->sampleMask -
                                                (const uint8 *)&this->operationType ) ) < 0;
        }
    };

    struct HlmsComputePso
    {
        GpuProgramPtr                 computeShader;
        GpuProgramParametersSharedPtr computeParams;

        /// XYZ. Metal needs the threads per group on C++ side. HLSL & GLSL want
        /// the thread count on shader side, thus we allow users to tell us
        /// the thread count to C++, and send it to the shaders via
        /// \@value( threads_per_group_x ); OR let the shader tell C++ the threadcount
        /// via \@pset( threads_per_group_x, 64 )
        /// (there's also threads_per_group_y & threads_per_group_z)
        /// @see HlmsComputeJob::setThreadsPerGroup
        uint32 mThreadsPerGroup[3];

        /// The number of thread groups to dispatch.
        /// eg. Typically:
        ///     mNumThreadGroups[0] = ceil( mThreadsPerGroup[0] / image.width );
        ///     mNumThreadGroups[1] = ceil( mThreadsPerGroup[1] / image.height );
        /// @see HlmsComputeJob::setNumThreadGroups
        uint32 mNumThreadGroups[3];

        void *rsData;  ///< Render-System specific data

        // No constructor on purpose. Performance implications
        //(could get called every object when looking up!)
        // see Hlms::getShaderCache
        // HlmsComputePso();

        void initialize()
        {
            memset( mThreadsPerGroup, 0, sizeof( mThreadsPerGroup ) );
            memset( mNumThreadGroups, 0, sizeof( mNumThreadGroups ) );
            rsData = 0;
        }
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
