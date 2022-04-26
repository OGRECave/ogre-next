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
#ifndef __UnifiedHighLevelGpuProgram_H__
#define __UnifiedHighLevelGpuProgram_H__

#include "OgrePrerequisites.h"

#include "OgreHighLevelGpuProgram.h"
#include "OgreHighLevelGpuProgramManager.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */
    /** Specialisation of HighLevelGpuProgram which just delegates its implementation
        to one other high level program, allowing a single program definition
        to represent one supported program from a number of options
    @remarks
        Whilst you can use Technique to implement several ways to render an object
        depending on hardware support, if the only reason to need multiple paths is
        because of the high-level shader language supported, this can be
        cumbersome. For example you might want to implement the same shader
        in HLSL and GLSL for portability but apart from the implementation detail,
        the shaders do the same thing and take the same parameters. If the materials
        in question are complex, duplicating the techniques just to switch language
        is not optimal, so instead you can define high-level programs with a
        syntax of 'unified', and list the actual implementations in order of
        preference via repeated use of the 'delegate' parameter, which just points
        at another program name. The first one which has a supported syntax
        will be used.
    */
    class _OgreExport UnifiedHighLevelGpuProgram : public HighLevelGpuProgram
    {
    private:
        static std::map<String, int> mLanguagePriorities;

    public:
        /// Command object for setting delegate (can set more than once)
        class CmdDelegate final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        static void setPrioriry( String shaderLanguage, int priority );
        static int  getPriority( String shaderLanguage );

    protected:
        static CmdDelegate msCmdDelegate;

        /// Ordered list of potential delegates
        StringVector mDelegateNames;
        /// The chosen delegate
        mutable HighLevelGpuProgramPtr mChosenDelegate;

        /// Choose the delegate to use
        void chooseDelegate() const;

        void createLowLevelImpl() override;
        void unloadHighLevelImpl() override;
        void buildConstantDefinitions() const override;
        void loadFromSource() override;

    public:
        /** Constructor, should be used only by factory classes. */
        UnifiedHighLevelGpuProgram( ResourceManager *creator, const String &name, ResourceHandle handle,
                                    const String &group, bool isManual = false,
                                    ManualResourceLoader *loader = 0 );
        ~UnifiedHighLevelGpuProgram() override;

        size_t calculateSize() const override;

        /** Adds a new delegate program to the list.
        @remarks
            Delegates are tested in order so earlier ones are preferred.
        */
        void addDelegateProgram( const String &name );

        /// Remove all delegate programs
        void clearDelegatePrograms();

        /// Get the chosen delegate
        const HighLevelGpuProgramPtr &_getDelegate() const;

        /** @copydoc GpuProgram::getLanguage */
        const String &getLanguage() const override;

        /** Creates a new parameters object compatible with this program definition.
        @remarks
        Unlike low-level assembly programs, parameters objects are specific to the
        program and therefore must be created from it rather than by the
        HighLevelGpuProgramManager. This method creates a new instance of a parameters
        object containing the definition of the parameters this program understands.
        */
        GpuProgramParametersSharedPtr createParameters() override;
        /** @copydoc GpuProgram::_getBindingDelegate */
        GpuProgram *_getBindingDelegate() override;

        // All the following methods must delegate to the implementation

        /** @copydoc GpuProgram::isSupported */
        bool isSupported() const override;

        /** @copydoc GpuProgram::isSkeletalAnimationIncluded */
        bool isSkeletalAnimationIncluded() const override;

        bool isMorphAnimationIncluded() const override;

        bool   isPoseAnimationIncluded() const override;
        ushort getNumberOfPosesIncluded() const override;

        bool                          isVertexTextureFetchRequired() const override;
        GpuProgramParametersSharedPtr getDefaultParameters() override;
        bool                          hasDefaultParameters() const override;
        bool                          getPassSurfaceAndLightStates() const override;
        bool                          getPassFogStates() const override;
        bool                          getPassTransformStates() const override;
        bool                          hasCompileError() const override;
        void                          resetCompileError() override;

        void         load( bool backgroundThread = false ) override;
        void         reload( LoadingFlags flags = LF_DEFAULT ) override;
        bool         isReloadable() const override;
        bool         isLoaded() const override;
        bool         isLoading() const override;
        LoadingState getLoadingState() const override;
        void         unload() override;
        size_t       getSize() const override;
        void         touch() override;
        bool         isBackgroundLoaded() const override;
        void         setBackgroundLoaded( bool bl ) override;
        void         escalateLoading() override;
        void         addListener( Listener *lis ) override;
        void         removeListener( Listener *lis ) override;
    };

    /** Factory class for Unified programs. */
    class UnifiedHighLevelGpuProgramFactory : public HighLevelGpuProgramFactory
    {
    public:
        UnifiedHighLevelGpuProgramFactory();
        ~UnifiedHighLevelGpuProgramFactory() override;
        /// Get the name of the language this factory creates programs for
        const String &getLanguage() const override;

        HighLevelGpuProgram *create( ResourceManager *creator, const String &name, ResourceHandle handle,
                                     const String &group, bool isManual,
                                     ManualResourceLoader *loader ) override;

        void destroy( HighLevelGpuProgram *prog ) override;
    };

    /** @} */
    /** @} */

}  // namespace Ogre
#endif
