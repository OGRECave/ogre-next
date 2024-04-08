/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2023 Torus Knot Software Ltd

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

#ifndef OgreBillboardSet2_H
#define OgreBillboardSet2_H

#include "OgrePrerequisites.h"

#include "ParticleSystem/OgreBillboard2.h"
#include "ParticleSystem/OgreParticleSystem2.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    /// A BillboardSet is just a ParticleSystemDef directly exposed to the user but without emitters,
    ///	affectors and time to live.
    ///
    /// Why isn't it the reverse? (i.e. a ParticleSystemDef should derive from BillboardSet2,
    /// making it a more specialized version with extra features).
    ///
    /// Well 2 simple reasons:
    ///
    ///	  1. The code started out as a ParticleSystemDef first.
    ///	  2. ParticleSystemDef are tied to shader generation (i.e. Hlms & RenderQueues) because
    ///		 geometry generation happens in shaders (instead of the CPU).
    ///		 This means it's easier for Hlms to downcast to ParticleSystemDef (which offers full
    ///      functionality) and generate the shader based on available features, rather than first
    ///		 downcast to a BillboardSet2 and then to ParticleSystemDef.
    ///
    /// Use setParticleQuota() to define how many billboards this set can contain.
    class _OgreExport BillboardSet : public ParticleSystemDef
    {
    public:
        using ParticleSystem::setMaterialName;

        Renderable *getAsRenderable() { return this; }

        Billboard allocBillboard();

        void deallocBillboard( Billboard billboard );
        void deallocBillboard( uint32 handle ) { deallocBillboard( Billboard( handle, this ) ); }

        BillboardSet( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *manager,
                      ParticleSystemManager2 *particleSystemManager ) :
            ParticleSystemDef( id, objectMemoryManager, manager, particleSystemManager, "", true )
        {
        }

    protected:
        // Hide these functions.
        uint32 allocParticle() { return ParticleSystemDef::allocParticle(); }
        void   deallocParticle( uint32 handle ) { ParticleSystemDef::deallocParticle( handle ); }
    };

    OGRE_ASSUME_NONNULL_END
}  // namespace Ogre
#include "OgreHeaderSuffix.h"

#endif
