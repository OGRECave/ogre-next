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
#ifndef __HighLevelGpuProgram_H__
#define __HighLevelGpuProgram_H__

#include "OgrePrerequisites.h"

#include "OgreGpuProgram.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */
    /** Abstract base class representing a high-level program (a vertex or
        fragment program).
    @remarks
        High-level programs are vertex and fragment programs written in a high-level
        language such as Cg or HLSL, and as such do not require you to write assembler code
        like GpuProgram does. However, the high-level program does eventually
        get converted (compiled) into assembler and then eventually microcode which is
        what runs on the GPU. As well as the convenience, some high-level languages like Cg allow
        you to write a program which will operate under both Direct3D and OpenGL, something
        which you cannot do with just GpuProgram (which requires you to write 2 programs and
        use each in a Technique to provide cross-API compatibility). Ogre will be creating
        a GpuProgram for you based on the high-level program, which is compiled specifically
        for the API being used at the time, but this process is transparent.
    @par
        You cannot create high-level programs direct - use HighLevelGpuProgramManager instead.
        Plugins can register new implementations of HighLevelGpuProgramFactory in order to add
        support for new languages without requiring changes to the core Ogre API. To allow
        custom parameters to be set, this class extends StringInterface - the application
        can query on the available custom parameters and get/set them without having to
        link specifically with it.
    */
    class _OgreExport HighLevelGpuProgram : public GpuProgram
    {
    public:
        /// Command object for enabling include in shaders
        class _OgrePrivate CmdEnableIncludeHeader final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for using Hlms parser
        class _OgrePrivate CmdUseHlmsParser final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        static CmdEnableIncludeHeader msEnableIncludeHeaderCmd;
        static CmdUseHlmsParser       msUseHlmsParser;

    protected:
        /// Whether the high-level program (and it's parameter defs) is loaded
        bool mHighLevelLoaded;
        /// See setEnableIncludeHeader()
        bool mEnableIncludeHeader;
        /// See setUseHlmsParser()
        bool mUseHlmsParser;
        /// The underlying assembler program
        GpuProgramPtr mAssemblerProgram;
        /// Have we built the name->index parameter map yet?
        mutable bool mConstantDefsBuilt;

        /// Internal load high-level portion if not loaded
        virtual void loadHighLevel();
        /// Internal unload high-level portion if loaded
        virtual void unloadHighLevel();

        void dumpSourceIfHasIncludeEnabled();
        void parseIncludeFile( String &inOutSourceFile );
        /** Internal load implementation, loads just the high-level portion, enough to
            get parameters.
        */
        virtual void loadHighLevelImpl();
        /** Internal method for creating an appropriate low-level program from this
        high-level program, must be implemented by subclasses. */
        virtual void createLowLevelImpl() = 0;
        /// Internal unload implementation, must be implemented by subclasses
        virtual void unloadHighLevelImpl() = 0;
        /// Populate the passed parameters with name->index map
        virtual void populateParameterNames( GpuProgramParametersSharedPtr params );
        /** Build the constant definition map, must be overridden.
        @note The implementation must fill in the (inherited) mConstantDefs field at a minimum,
            and if the program requires that parameters are bound using logical
            parameter indexes then the mFloatLogicalToPhysical and mIntLogicalToPhysical
            maps must also be populated.
        */
        virtual void buildConstantDefinitions() const = 0;

        void setupBaseParamDictionary();

        /** @copydoc Resource::loadImpl */
        void loadImpl() override;
        /** @copydoc Resource::unloadImpl */
        void unloadImpl() override;

    public:
        /** Constructor, should be used only by factory classes. */
        HighLevelGpuProgram( ResourceManager *creator, const String &name, ResourceHandle handle,
                             const String &group, bool isManual = false,
                             ManualResourceLoader *loader = 0 );
        ~HighLevelGpuProgram() override;

        /** Creates a new parameters object compatible with this program definition.
        @remarks
            Unlike low-level assembly programs, parameters objects are specific to the
            program and therefore must be created from it rather than by the
            HighLevelGpuProgramManager. This method creates a new instance of a parameters
            object containing the definition of the parameters this program understands.
        */
        GpuProgramParametersSharedPtr createParameters() override;
        /** @copydoc GpuProgram::_getBindingDelegate */
        GpuProgram *_getBindingDelegate() override { return mAssemblerProgram.get(); }

        // This snippet must not be indented, in order for the documentation
        // of setEnableIncludeHeader to turn out looking right.  So
        // clang-format must be prevented from changing it.
        // clang-format off
        // [setEnableIncludeHeader-doc-snippet]
/*
    #include "MyFile.h" --> file will be included anyway.
*/
        // [setEnableIncludeHeader-doc-snippet]
        // clang-format on

        /** Whether we should parse the source code looking for include files and
            embedding the file. Disabled by default to avoid slowing down when
            `#include` is not used. Not needed if the API natively supports it (D3D11).
        @remarks
        Single line comments are supported:
            @code
            // #include "MyFile.h" --> won't be included.
            @endcode
        Block comment lines are not supported, but may not matter if
        the included file does not close the block:
            @snippet this setEnableIncludeHeader-doc-snippet
        Preprocessor macros are not supported, but should not matter:
            @code
            #if SOME_MACRO
                #include "MyFile.h" --> file will be included anyway.
            #endif
            @endcode
        @remarks
            Recursive includes are supported (e.g. header includes a header)
        @remarks
            Beware included files mess up error reporting (wrong lines)
        @param bEnable
            True to support `#include`. Must be toggled before loading the source file.
        */
        void setEnableIncludeHeader( bool bEnable );
        bool getEnableIncludeHeader() const;

        /** Whether we should run the shader through the Hlms parser.

            This parser doesn't yet have access to the entire PSO
            (i.e. Pixel Shader can't see Vertex Shader).

            However it's still useful, specially \@foreach()
        @remarks
            This is run after setEnableIncludeHeader
        @par
            Beware included files mess up error reporting (wrong lines)
        @param bUse
            True to run source code through Hlms parser.
            Must be toggled before loading the source file.
        */
        void setUseHlmsParser( bool bUse );
        bool getUseHlmsParser() const;

        /** Get the full list of GpuConstantDefinition instances.
        @note
        Only available if this parameters object has named parameters.
        */
        const GpuNamedConstants &getConstantDefinitions() const override;

        size_t calculateSize() const override;
    };
    /** @} */
    /** @} */

}  // namespace Ogre
#endif
