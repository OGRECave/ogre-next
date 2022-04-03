/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2016 Torus Knot Software Ltd

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

#include "OgreMetalProgramFactory.h"

#include "OgreHighLevelGpuProgram.h"
#include "OgreMetalDevice.h"
#include "OgreMetalProgram.h"
#include "OgreRoot.h"

namespace Ogre
{
    //-----------------------------------------------------------------------
    String MetalProgramFactory::sLanguageName = "metal";
    //-----------------------------------------------------------------------
    MetalProgramFactory::MetalProgramFactory( MetalDevice *device ) : mDevice( device ) {}
    //-----------------------------------------------------------------------
    MetalProgramFactory::~MetalProgramFactory() {}
    //-----------------------------------------------------------------------
    const String &MetalProgramFactory::getLanguage() const { return sLanguageName; }
    //-----------------------------------------------------------------------
    HighLevelGpuProgram *MetalProgramFactory::create( ResourceManager *creator, const String &name,
                                                      ResourceHandle handle, const String &group,
                                                      bool isManual, ManualResourceLoader *loader )
    {
        return OGRE_NEW MetalProgram( creator, name, handle, group, isManual, loader, mDevice );
    }
    //-----------------------------------------------------------------------
    void MetalProgramFactory::destroy( HighLevelGpuProgram *prog ) { OGRE_DELETE prog; }
}
