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
#ifndef __D3D11HLSLProgram_H__
#define __D3D11HLSLProgram_H__

#include "OgreD3D11Prerequisites.h"

#include "OgreD3D11DeviceResource.h"
#include "OgreHighLevelGpuProgram.h"
#include "OgreString.h"
#include "Vao/OgreVertexBufferPacked.h"

namespace Ogre
{
    typedef vector<byte>::type MicroCode;

    /** Specialization of HighLevelGpuProgram to provide support for D3D11
    High-Level Shader Language (HLSL).
    @remarks
    Note that the syntax of D3D11 HLSL is identical to nVidia's Cg language, therefore
    unless you know you will only ever be deploying on Direct3D, or you have some specific
    reason for not wanting to use the Cg plugin, I suggest you use Cg instead since that
    can produce programs for OpenGL too.
    */
    class _OgreD3D11Export D3D11HLSLProgram final : public HighLevelGpuProgram,
                                                    protected D3D11DeviceResource
    {
    public:
        /// Command object for setting entry point
        class CmdEntryPoint final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for setting target assembler
        class CmdTarget final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for setting macro defines
        class CmdPreprocessorDefines final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for setting matrix packing in column-major order
        class CmdColumnMajorMatrices final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for setting backwards compatibility
        class CmdEnableBackwardsCompatibility final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

    protected:
        static CmdEntryPoint                   msCmdEntryPoint;
        static CmdTarget                       msCmdTarget;
        static CmdPreprocessorDefines          msCmdPreprocessorDefines;
        static CmdColumnMajorMatrices          msCmdColumnMajorMatrices;
        static CmdEnableBackwardsCompatibility msCmdEnableBackwardsCompatibility;

        void notifyDeviceLost( D3D11Device *device ) override;
        void notifyDeviceRestored( D3D11Device *device, unsigned pass ) override;

        /** Internal method for creating an appropriate low-level program from this
        high-level program, must be implemented by subclasses. */
        void createLowLevelImpl() override;
        /// Internal unload implementation, must be implemented by subclasses
        void unloadHighLevelImpl() override;

        // Recursive utility method for populateParameterNames
        void processParamElement( String prefix, LPCSTR pName, ID3D11ShaderReflectionType *varRefType );

        void populateDef( D3D11_SHADER_TYPE_DESC &d3dDesc, GpuConstantDefinition &def ) const;

        void getDefines( String &stringBuffer, vector<D3D_SHADER_MACRO>::type &defines,
                         const String &definesString );

        String mTarget;
        String mEntryPoint;
        String mPreprocessorDefines;
        bool   mColumnMajorMatrices;
        bool   mEnableBackwardsCompatibility;

        bool      mErrorsInCompile;
        MicroCode mMicroCode;

        D3D11Device &mDevice;

        ComPtr<ID3D11VertexShader>   mVertexShader;
        ComPtr<ID3D11PixelShader>    mPixelShader;
        ComPtr<ID3D11GeometryShader> mGeometryShader;
        ComPtr<ID3D11DomainShader>   mDomainShader;
        ComPtr<ID3D11HullShader>     mHullShader;
        ComPtr<ID3D11ComputeShader>  mComputeShader;

        struct ShaderVarWithPosInBuf
        {
            mutable String name;
            size_t         size;
            size_t         startOffset;

            ShaderVarWithPosInBuf &operator=( const ShaderVarWithPosInBuf &var )
            {
                name = var.name;
                size = var.size;
                startOffset = var.startOffset;
                return *this;
            }
        };
        typedef vector<ShaderVarWithPosInBuf>::type ShaderVars;
        typedef ShaderVars::iterator                ShaderVarsIter;
        typedef ShaderVars::const_iterator          ShaderVarsConstIter;

        // A hack for cg to get the "original name" of the var in the "auto comments"
        // that cg adds to the hlsl 4 output. This is to solve the issue that
        // in some cases cg changes the name of the var to a new name.
        void fixVariableNameFromCg( const ShaderVarWithPosInBuf &newVar );

        struct BufferInfo
        {
            ComPtr<ID3D11Buffer> mConstBuffer;
            ShaderVars           mShaderVars;
        };

        // Map to store interface slot position.
        // Number of interface slots is size of this map.
        typedef std::map<String, unsigned int>                 SlotMap;
        typedef std::map<String, unsigned int>::const_iterator SlotIterator;

        SlotMap mSlotMap;

        typedef vector<D3D11_SIGNATURE_PARAMETER_DESC>::type D3d11ShaderParameters;
        typedef D3d11ShaderParameters::iterator              D3d11ShaderParametersIter;

        typedef vector<D3D11_SHADER_VARIABLE_DESC>::type D3d11ShaderVariables;
        typedef D3d11ShaderVariables::iterator           D3d11ShaderVariablesIter;

        struct GpuConstantDefinitionWithName : GpuConstantDefinition
        {
            LPCSTR Name;
        };
        typedef vector<GpuConstantDefinitionWithName>::type D3d11ShaderVariableSubparts;
        typedef D3d11ShaderVariableSubparts::iterator       D3d11ShaderVariableSubpartsIter;

        struct MemberTypeName
        {
            LPCSTR Name;
        };

        vector<String *>::type mSerStrings;

        typedef vector<D3D11_SHADER_BUFFER_DESC>::type D3d11ShaderBufferDescs;
        typedef vector<D3D11_SHADER_TYPE_DESC>::type   D3d11ShaderTypeDescs;
        typedef vector<UINT>::type                     InterfaceSlots;
        typedef vector<MemberTypeName>::type           MemberTypeNames;

        UINT                        mConstantBufferSize;
        UINT                        mConstantBufferNr;
        UINT                        mNumSlots;
        ShaderVars                  mShaderVars;
        D3d11ShaderParameters       mD3d11ShaderInputParameters;
        D3d11ShaderParameters       mD3d11ShaderOutputParameters;
        D3d11ShaderVariables        mD3d11ShaderVariables;
        D3d11ShaderVariableSubparts mD3d11ShaderVariableSubparts;
        D3d11ShaderBufferDescs      mD3d11ShaderBufferDescs;
        D3d11ShaderVariables        mVarDescBuffer;
        D3d11ShaderVariables        mVarDescPointer;
        D3d11ShaderTypeDescs        mD3d11ShaderTypeDescs;
        D3d11ShaderTypeDescs        mMemberTypeDesc;
        enum DefaultBufferTypes
        {
            BufferGlobal,
            BufferParam,
            NumDefaultBufferTypes
        };
        // D3D11_SHADER_INPUT_BIND_DESC    mDefaultBufferBindPoints[NumDefaultBufferTypes];
        UINT            mDefaultBufferBindPoint;
        BufferInfo      mDefaultBuffers[NumDefaultBufferTypes];
        MemberTypeNames mMemberTypeName;
        InterfaceSlots  mInterfaceSlots;

        void analizeMicrocode();
        void getMicrocodeFromCache( const void *microcode );
        void compileMicrocode();

    public:
        D3D11HLSLProgram( ResourceManager *creator, const String &name, ResourceHandle handle,
                          const String &group, bool isManual, ManualResourceLoader *loader,
                          D3D11Device &device );
        ~D3D11HLSLProgram() override;

        /** Sets the entry point for this program ie the first method called. */
        void setEntryPoint( const String &entryPoint ) { mEntryPoint = entryPoint; }
        /** Gets the entry point defined for this program. */
        const String &getEntryPoint() const { return mEntryPoint; }
        /** Sets the shader target to compile down to, e.g. 'vs_1_1'. */
        void setTarget( const String &target );
        /** Gets the shader target to compile down to, e.g. 'vs_1_1'. */
        const String &getTarget() const { return mTarget; }
        /** Gets the shader target promoted to the first compatible, e.g. 'vs_4_0' or 'ps_4_0' if
         * backward compatibility is enabled. */
        const String &getCompatibleTarget() const;

        /** Sets the preprocessor defines use to compile the program. */
        void setPreprocessorDefines( const String &defines ) { mPreprocessorDefines = defines; }
        /** Sets the preprocessor defines use to compile the program. */
        const String &getPreprocessorDefines() const { return mPreprocessorDefines; }
        /** Sets whether matrix packing in column-major order. */
        void setColumnMajorMatrices( bool columnMajor ) { mColumnMajorMatrices = columnMajor; }
        /** Gets whether matrix packed in column-major order. */
        bool getColumnMajorMatrices() const { return mColumnMajorMatrices; }
        /** Sets whether backwards compatibility is enabled. */
        void setEnableBackwardsCompatibility( bool enableBackwardsCompatibility )
        {
            mEnableBackwardsCompatibility = enableBackwardsCompatibility;
        }
        /** Gets whether backwards compatibility is enabled. */
        bool getEnableBackwardsCompatibility() const { return mEnableBackwardsCompatibility; }
        /// Overridden from GpuProgram
        bool isSupported() const override;
        /// Overridden from GpuProgram
        GpuProgramParametersSharedPtr createParameters() override;
        /// Overridden from GpuProgram
        const String &getLanguage() const override;

        void                  buildConstantDefinitions() const override;
        ID3D11VertexShader   *getVertexShader() const;
        ID3D11PixelShader    *getPixelShader() const;
        ID3D11GeometryShader *getGeometryShader() const;
        ID3D11DomainShader   *getDomainShader() const;
        ID3D11HullShader     *getHullShader() const;
        ID3D11ComputeShader  *getComputeShader() const;
        const MicroCode      &getMicroCode() const;

        /// buffers must have a capacity of 2, i.e. ID3D11Buffer *buffers[2];
        void getConstantBuffers( ID3D11Buffer **buffers, UINT &outSlotStart, UINT &outNumBuffers,
                                 GpuProgramParametersSharedPtr params, uint16 variabilityMask );

        // Get slot for a specific interface
        unsigned int getSubroutineSlot( const String &subroutineSlotName ) const;

        ComPtr<ID3D11InputLayout> getLayoutForPso( const VertexElement2VecVec &vertexElements );

        void CreateVertexShader();
        void CreatePixelShader();
        void CreateGeometryShader();
        void CreateDomainShader();
        void CreateHullShader();
        void CreateComputeShader();

        /** Internal load implementation, must be implemented by subclasses.
         */
        void loadFromSource() override;

        void reinterpretGSForStreamOut();
        bool mReinterpretingGS;

        size_t getNumInputs() const;
        size_t getNumOutputs() const;

        String getNameForMicrocodeCache();

        const D3D11_SIGNATURE_PARAMETER_DESC &getInputParamDesc( size_t index ) const;
        const D3D11_SIGNATURE_PARAMETER_DESC &getOutputParamDesc( size_t index ) const;
    };
}  // namespace Ogre

#endif
