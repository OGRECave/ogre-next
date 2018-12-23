/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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

#ifndef _OgreObjCmdBuffer_H_
#define _OgreObjCmdBuffer_H_

#include "OgrePrerequisites.h"
#include "OgreTextureBox.h"
#include "OgreImage2.h"
#include "OgreException.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    namespace TextureFilter
    {
        class FilterBase;
    }
    typedef FastArray<TextureFilter::FilterBase*> FilterBaseArray;

    class _OgreExport ObjCmdBuffer : public ResourceAlloc
    {
    public:
        class Cmd
        {
        public:
            virtual ~Cmd() {}
            virtual void execute(void) = 0;
        };

    protected:
        FastArray<uint8>    mCommandAllocator;
        FastArray<Cmd*>     mCommandBuffer;

        void* requestMemory( size_t sizeBytes );

    public:
        ~ObjCmdBuffer();
        void clear(void);
        void execute(void);

        template <typename T> T* addCommand()
        {
            T *newCmd = reinterpret_cast<T*>( requestMemory( sizeof(T) ) );
            mCommandBuffer.push_back( newCmd );
            return newCmd;
        }

        void commit(void);
        void swapCommitted( ObjCmdBuffer *workerThread );

        class TransitionToLoaded : public Cmd
        {
            TextureGpu  *texture;
            void        *sysRamCopy;
            GpuResidency::GpuResidency targetResidency;

        public:
            TransitionToLoaded( TextureGpu *_texture, void *_sysRamCopy,
                                GpuResidency::GpuResidency _targetResidency );
            virtual void execute(void);
        };

        class OutOfDateCache : public Cmd
        {
            TextureGpu  *texture;
            Image2      loadedImage;

        public:
            OutOfDateCache( TextureGpu *_texture, Image2 &image );
            virtual void execute(void);
        };

        class ExceptionThrown : public Cmd
        {
            TextureGpu  *texture;
            Exception   exception;

        public:
            ExceptionThrown( TextureGpu *_texture, const Exception &_exception );
            virtual void execute(void);
        };

        class UploadFromStagingTex : public Cmd
        {
            StagingTexture  *stagingTexture;
            TextureBox      box;
            TextureGpu      *dstTexture;
            TextureBox      dstBox;
            uint8           mipLevel;

        public:
            UploadFromStagingTex( StagingTexture *_stagingTexture,
                                  const TextureBox &_box,
                                  TextureGpu *_dstTexture,
                                  const TextureBox &_dstBox,
                                  uint8 _mipLevel );
            virtual void execute(void);
        };

        class NotifyDataIsReady : public Cmd
        {
            TextureGpu      *texture;
            FilterBaseArray filters;

        public:
            NotifyDataIsReady( TextureGpu *_textureGpu, FilterBaseArray &inOutFilters );
            virtual void execute(void);
        };
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
