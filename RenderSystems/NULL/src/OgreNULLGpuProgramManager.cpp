/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2017 Torus Knot Software Ltd

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

#include "OgreNULLGpuProgramManager.h"

#include "OgreGpuProgram.h"
#include "OgreResourceGroupManager.h"

namespace Ogre
{
    /// The low level counterpart of OgreMain's NullProgram: it exists so a script can
    /// declare a program, and does nothing else.
    class NULLGpuProgram final : public GpuProgram
    {
    protected:
        void loadFromSource() override {}
        void unloadImpl() override {}

    public:
        NULLGpuProgram( ResourceManager *creator, const String &name, ResourceHandle handle,
                        const String &group, bool isManual, ManualResourceLoader *loader ) :
            GpuProgram( creator, name, handle, group, isManual, loader )
        {
        }
        ~NULLGpuProgram() override {}

        /// Overridden from GpuProgram - never supported, nothing was compiled
        bool isSupported() const override { return false; }

        size_t calculateSize() const override { return 0; }

        /// Overridden from StringInterface
        bool setParameter( const String & /*name*/, const String & /*value*/ ) override
        {
            // always silently ignore all parameters so as not to report errors on
            // unsupported platforms
            return true;
        }
    };
    //-----------------------------------------------------------------------------
    NULLGpuProgramManager::NULLGpuProgramManager() : GpuProgramManager()
    {
        // Superclass sets up members

        // Register with resource group manager
        ResourceGroupManager::getSingleton()._registerResourceManager( mResourceType, this );
    }
    //-----------------------------------------------------------------------------
    NULLGpuProgramManager::~NULLGpuProgramManager()
    {
        // Unregister with resource group manager
        ResourceGroupManager::getSingleton()._unregisterResourceManager( mResourceType );
    }
    //-----------------------------------------------------------------------------
    Resource *NULLGpuProgramManager::createImpl( const String &name, ResourceHandle handle,
                                                 const String &group, bool isManual,
                                                 ManualResourceLoader *loader,
                                                 const NameValuePairList * )
    {
        // The declaration is accepted whatever it says: syntax and type only matter to a
        // compiler, and there is none
        return new NULLGpuProgram( this, name, handle, group, isManual, loader );
    }
    //-----------------------------------------------------------------------------
    Resource *NULLGpuProgramManager::createImpl( const String &name, ResourceHandle handle,
                                                 const String &group, bool isManual,
                                                 ManualResourceLoader *loader, GpuProgramType,
                                                 const String & )
    {
        return new NULLGpuProgram( this, name, handle, group, isManual, loader );
    }
}  // namespace Ogre
