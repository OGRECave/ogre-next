/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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
#ifndef _OgreVulkanProgram_H_
#define _OgreVulkanProgram_H_

#include "OgreVulkanPrerequisites.h"

#include "OgreHardwareVertexBuffer.h"
#include "OgreHighLevelGpuProgram.h"

struct VkVertexInputBindingDescription;
struct VkVertexInputAttributeDescription;
struct SpvReflectDescriptorBinding;
struct SpvReflectDescriptorSet;
struct SpvReflectShaderModule;

struct TBuiltInResource;

namespace Ogre
{
    enum ShaderSyntax
    {
        GLSL,
        HLSL
    };

    struct _OgreVulkanExport VulkanConstantDefinitionBindingParam
    {
        size_t offset;
        size_t size;
    };

    /** Specialisation of HighLevelGpuProgram to provide support for Vulkan
        Shader Language.
    @remarks
        Vulkan has no target assembler or entry point specification like DirectX 9 HLSL.
        Vertex and Fragment shaders only have one entry point called "main".
        When a shader is compiled, microcode is generated but can not be accessed by
        the application.
        Vulkan also does not provide assembler low level output after compiling.  The Vulkan Render
        system assumes that the Gpu program is a Vulkan Gpu program so VulkanProgram will create a
        VulkanGpuProgram that is subclassed from VulkanGpuProgram for the low level implementation.
        The VulkanProgram class will create a shader object and compile the source but will
        not create a program object.  It's up to VulkanGpuProgram class to request a program object
        to link the shader object to.
    */
    class _OgreVulkanExport VulkanProgram final : public HighLevelGpuProgram
    {
    public:
        /// Command object for setting macro defines
        class CmdPreprocessorDefines final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void doSet( void *target, const String &val ) override;
        };

        VulkanProgram( ResourceManager *creator, const String &name, ResourceHandle handle,
                       const String &group, bool isManual, ManualResourceLoader *loader,
                       VulkanDevice *device, String &languageName );
        ~VulkanProgram() override;

        /// Overridden
        bool getPassTransformStates() const override;
        bool getPassSurfaceAndLightStates() const override;
        bool getPassFogStates() const override;

        void setRootLayout( GpuProgramType type, const RootLayout &rootLayout ) override;
        void unsetRootLayout() override;
        void setAutoReflectArrayBindingsInRootLayout( bool bReflectArrayRootLayouts ) override;

        /// Sets the preprocessor defines use to compile the program.
        void setPreprocessorDefines( const String &defines ) { mPreprocessorDefines = defines; }
        /// Sets the preprocessor defines use to compile the program.
        const String &getPreprocessorDefines() const { return mPreprocessorDefines; }

        void setReplaceVersionMacro( bool bReplace ) override;

        /// Overridden from GpuProgram
        const String &getLanguage() const override;
        /// Overridden from GpuProgram
        GpuProgramParametersSharedPtr createParameters() override;

        void debugDump( String &outString );

        /// Compile source into shader object
        bool compile( const bool checkErrors, const bool bReflectingArrays = false );

        void fillPipelineShaderStageCi( VkPipelineShaderStageCreateInfo &pssCi );

        /// In bytes.
        uint32 getBufferRequiredSize() const;
        /// dstData must be able to hold at least getBufferRequiredSize
        void updateBuffers( const GpuProgramParametersSharedPtr &params, uint8 *RESTRICT_ALIAS dstData );

        const std::vector<uint32> &getSpirv() const { return mSpirv; }

        const map<uint32, VulkanConstantDefinitionBindingParam>::type &getConstantDefsBindingParams()
            const
        {
            return mConstantDefsBindingParams;
        }

        VulkanRootLayout *getRootLayout() { return mRootLayout; }

        void getLayoutForPso( const VertexElement2VecVec &vertexElements,
                              FastArray<VkVertexInputBindingDescription> &outBufferBindingDescs,
                              FastArray<VkVertexInputAttributeDescription> &outVertexInputs ) const;

        uint32 getDrawIdLocation() const { return mDrawIdLocation; }

    protected:
        static CmdPreprocessorDefines msCmdPreprocessorDefines;

        uint32 getEshLanguage() const;

        String getNameForMicrocodeCache( const String &preamble ) const;

        /// Returns true if successfully extracted Root Layout from source
        bool extractRootLayoutFromSource();

        static void initGlslResources( TBuiltInResource &resources );

        /** Internal load implementation, must be implemented by subclasses.
         */
        void loadFromSource() override;

        void replaceVersionMacros();
        void getPreamble( String &preamble ) const;
        void addVertexSemanticsToPreamble( String &inOutPreamble ) const;
        void addPreprocessorToPreamble( String &inOutPreamble ) const;

        /** Internal method for creating a dummy low-level program for this
        high-level program. Vulkan does not give access to the low level implementation of the
        shader so this method creates an object sub-classed from VulkanGpuProgram just to be
        compatible with VulkanRenderSystem.
        */
        void createLowLevelImpl() override;
        /// Internal unload implementation, must be implemented by subclasses
        void unloadHighLevelImpl() override;
        /// Overridden from HighLevelGpuProgram
        void unloadImpl() override;

        /// Populate the passed parameters with name->index map
        void populateParameterNames( GpuProgramParametersSharedPtr params ) override;
        /// Populate the passed parameters with name->index map, must be overridden
        void buildConstantDefinitions() const override;

        static const SpvReflectDescriptorBinding *findBinding(
            const FastArray<SpvReflectDescriptorSet *> &sets, size_t setIdx, size_t bindingIdx );

        void gatherVertexInputs( SpvReflectShaderModule &module );

        /** Reflects the SPIRV looking for array bindings (e.g. uniform texture2D myTex[123])
            to patch the Root Layout

            See GpuProgram::setAutoReflectArrayBindingsInRootLayout
        @param bValidating
            When true, we won't patch Root Layout but rather warn if the Root Layout
            is wrong (doesn't match declared arrays)
        */
        void gatherArrayedDescs( const bool bValidating );

        /// In order to compile a shader, we need a RootLayout.
        /// However if the shader uses arrayed bindings (e.g. uniform texture2D myArray[5])
        /// we must patch the RootLayout, creating a derivative
        void gatherArrayedDescs( const FastArray<SpvReflectDescriptorSet *> &sets,
                                 const bool bValidating );

    private:
        VulkanDevice *mDevice;

        VulkanRootLayout *mRootLayout;

        std::vector<uint32> mSpirv;
        VkShaderModule mShaderModule;

        typedef map<uint32, VkVertexInputAttributeDescription>::type VertexInputByLocationIdxMap;
        VertexInputByLocationIdxMap mVertexInputs;
        uint8 mNumSystemGenVertexInputs;  // System-generated inputs like gl_VertexIndex

        bool mCustomRootLayout;
        bool mReflectArrayRootLayouts;
        bool mReplaceVersionMacro;

        /// Flag indicating if shader object successfully compiled
        bool mCompiled;
        /// Preprocessor options
        String mPreprocessorDefines;

        vector<GpuConstantDefinition>::type mConstantDefsSorted;
        map<uint32, VulkanConstantDefinitionBindingParam>::type mConstantDefsBindingParams;
        uint32 mConstantsBytesToWrite;

        ShaderSyntax mShaderSyntax;
        uint32 mDrawIdLocation;
    };
}  // namespace Ogre

#endif  // __VulkanProgram_H__
