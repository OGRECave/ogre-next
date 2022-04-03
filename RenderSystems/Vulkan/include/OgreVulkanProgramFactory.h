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

#ifndef _OgreVulkanProgramFactory_H_
#define _OgreVulkanProgramFactory_H_

#include "OgreVulkanPrerequisites.h"

#include "OgreHighLevelGpuProgramManager.h"

namespace Ogre
{
    /** Factory class for Vulkan programs. */
    class _OgreVulkanExport VulkanProgramFactory final : public HighLevelGpuProgramFactory
    {
    protected:
        String mLanguageName;
        VulkanDevice *mDevice;

        bool mGlslLangInitializer;

    public:
        VulkanProgramFactory( VulkanDevice *device, const char *languageName, bool glslLangInitializer );
        ~VulkanProgramFactory() override;
        /// Get the name of the language this factory creates programs for
        const String &getLanguage() const override;
        /// Create an instance of VulkanProgram
        HighLevelGpuProgram *create( ResourceManager *creator, const String &name, ResourceHandle handle,
                                     const String &group, bool isManual,
                                     ManualResourceLoader *loader ) override;
        void destroy( HighLevelGpuProgram *prog ) override;
    };
}  // namespace Ogre

#endif  // _OgreVulkanProgramFactory_H_
