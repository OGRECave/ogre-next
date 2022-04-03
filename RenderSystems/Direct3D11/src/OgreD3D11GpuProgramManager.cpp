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
#include "OgreD3D11GpuProgramManager.h"

#include "OgreD3D11Device.h"
#include "OgreException.h"

namespace Ogre
{
    class _OgreD3D11Export D3D11UnsupportedGpuProgram : public GpuProgram
    {
    public:
        D3D11UnsupportedGpuProgram( ResourceManager *creator, const String &name, ResourceHandle handle,
                                    const String &group, bool isManual, ManualResourceLoader *loader ) :
            GpuProgram( creator, name, handle, group, isManual, loader )
        {
        }

        void throwException()
        {
            String message = "D3D11 doesn't support assembly shaders. Shader name:" + mName + "\n";
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, message,
                         "D3D11UnsupportedGpuProgram::loadFromSource" );
        }

    protected:
        void loadImpl() override { throwException(); }
        void loadFromSource() override { throwException(); }
        void unloadImpl() override {}
    };

    //-----------------------------------------------------------------------------
    D3D11GpuProgramManager::D3D11GpuProgramManager() : GpuProgramManager()
    {
        // Superclass sets up members

        // Register with resource group manager
        ResourceGroupManager::getSingleton()._registerResourceManager( mResourceType, this );
    }
    //-----------------------------------------------------------------------------
    D3D11GpuProgramManager::~D3D11GpuProgramManager()
    {
        // Unregister with resource group manager
        ResourceGroupManager::getSingleton()._unregisterResourceManager( mResourceType );
    }
    //-----------------------------------------------------------------------------
    Resource *D3D11GpuProgramManager::createImpl( const String &name, ResourceHandle handle,
                                                  const String &group, bool isManual,
                                                  ManualResourceLoader *loader,
                                                  const NameValuePairList *params )
    {
        NameValuePairList::const_iterator paramIt;

        if( !params || ( paramIt = params->find( "type" ) ) == params->end() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "You must supply a 'type' parameter",
                         "D3D11GpuProgramManager::createImpl" );
        }

        return new D3D11UnsupportedGpuProgram( this, name, handle, group, isManual, loader );
    }
    //-----------------------------------------------------------------------------
    Resource *D3D11GpuProgramManager::createImpl( const String &name, ResourceHandle handle,
                                                  const String &group, bool isManual,
                                                  ManualResourceLoader *loader, GpuProgramType gptype,
                                                  const String &syntaxCode )
    {
        return new D3D11UnsupportedGpuProgram( this, name, handle, group, isManual, loader );
    }
}  // namespace Ogre
