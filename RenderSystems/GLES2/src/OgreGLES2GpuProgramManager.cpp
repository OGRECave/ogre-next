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

#include "OgreGLES2GpuProgramManager.h"
#include "OgreGLSLESShader.h"
#include "OgreLogManager.h"

namespace Ogre {
    GLES2GpuProgramManager::GLES2GpuProgramManager()
    {
        // Superclass sets up members

        // Register with resource group manager
        ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);
    }

    GLES2GpuProgramManager::~GLES2GpuProgramManager()
    {
        // Unregister with resource group manager
        ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
    }

    bool GLES2GpuProgramManager::registerProgramFactory(const String& syntaxCode,
                                                     CreateGpuProgramCallback createFn)
    {
        return mProgramMap.insert(ProgramMap::value_type(syntaxCode, createFn)).second;
    }

    bool GLES2GpuProgramManager::unregisterProgramFactory(const String& syntaxCode)
    {
        return mProgramMap.erase(syntaxCode) != 0;
    }

    Resource* GLES2GpuProgramManager::createImpl(const String& name,
                                                ResourceHandle handle,
                                                const String& group, bool isManual,
                                                ManualResourceLoader* loader,
                                                const NameValuePairList* params)
    {
        NameValuePairList::const_iterator paramSyntax, paramType;

        if (!params || (paramSyntax = params->find("syntax")) == params->end() ||
            (paramType = params->find("type")) == params->end())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                "You must supply 'syntax' and 'type' parameters",
                "GLES2GpuProgramManager::createImpl");
        }

        ProgramMap::const_iterator iter = mProgramMap.find(paramSyntax->second);
        if(iter == mProgramMap.end())
        {
            // No factory, this is an unsupported syntax code, probably for another rendersystem
            // Create a basic one, it doesn't matter what it is since it won't be used
            // we have to forward the syntax code though
            GpuProgram* ret =  new GLSLESShader(this, name, handle, group, isManual, loader);
            ret->setSyntaxCode(paramSyntax->second);
            return  ret;
        }

        GpuProgramType gpt;
        if (paramType->second == "vertex_program")
        {
            gpt = GPT_VERTEX_PROGRAM;
        }
        else
        {
            gpt = GPT_FRAGMENT_PROGRAM;
        }

        return (iter->second)(this, name, handle, group, isManual,
                              loader, gpt, paramSyntax->second);
    }

    Resource* GLES2GpuProgramManager::createImpl(const String& name,
                                                ResourceHandle handle,
                                                const String& group, bool isManual,
                                                ManualResourceLoader* loader,
                                                GpuProgramType gptype,
                                                const String& syntaxCode)
    {
        ProgramMap::const_iterator iter = mProgramMap.find(syntaxCode);
        if(iter == mProgramMap.end())
        {
            // No factory, this is an unsupported syntax code, probably for another rendersystem
            // Create a basic one, it doesn't matter what it is since it won't be used
            // we have to forward the syntax code though
            GpuProgram* ret =  new GLSLESShader(this, name, handle, group, isManual, loader);
            ret->setSyntaxCode(syntaxCode);
            return  ret;
        }
        
        return (iter->second)(this, name, handle, group, isManual, loader, gptype, syntaxCode);
    }
}
