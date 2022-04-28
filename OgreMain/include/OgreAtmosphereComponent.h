/*-------------------------------------------------------------------------
This source file is a part of OGRE-Next
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
THE SOFTWARE

You may alternatively use this source under the terms of a specific version of
the OGRE Unrestricted License provided you have obtained such a license from
Torus Knot Software Ltd.
-------------------------------------------------------------------------*/

#ifndef OgreAtmosphereComponent_H_
#define OgreAtmosphereComponent_H_

#include "OgrePrerequisites.h"

namespace Ogre
{
    class _OgreExport AtmosphereComponent : public OgreAllocatedObj
    {
    public:
        virtual ~AtmosphereComponent();

        /// Indicates Hlms implementations this component has Hlms integration
        virtual bool providesHlmsCode() const { return true; }

        /// Returns getNumConstBuffersSlots
        virtual uint32 preparePassHash( Hlms *hlms, size_t constBufferSlot ) = 0;

        /// How many additional const buffers are required in our Hlms integration
        virtual uint32 getNumConstBuffersSlots() const = 0;

        /// Tells the component to bind the buffers
        /// Returns getNumConstBuffersSlots
        virtual uint32 bindConstBuffers( CommandBuffer *commandBuffer, size_t slotIdx ) = 0;

        /// Called when the scene manager wants to render and needs the component
        /// to update its buffers
        virtual void _update( SceneManager *sceneManager, Camera *camera ) = 0;
    };
}  // namespace Ogre

#endif
