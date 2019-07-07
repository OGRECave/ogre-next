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

#ifndef _OgreDescriptorSetUav_H_
#define _OgreDescriptorSetUav_H_

#include "OgrePrerequisites.h"
#include "OgreCommon.h"
#include "OgreResourceTransition.h"
#include "OgrePixelFormatGpu.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    /** Descriptor sets describe what uavs should be bound together in one place.
    @remarks
        Do not create and destroy a set every frame. You should create it once, and reuse
        it until it is no longer necessary, only then you should destroy it.
    */
    struct _OgreExport DescriptorSetUav
    {
        enum SlotType
        {
            SlotTypeBuffer,
            SlotTypeTexture
        };
        struct _OgreExport BufferSlot
        {
            /// UAV buffer to bind
            UavBufferPacked *buffer;
            /// 0-based offset. It is possible to bind a region of the buffer.
            /// Offset needs to be aligned. You can query the RS capabilities for
            /// the alignment, however 256 bytes is the maximum allowed alignment
            /// per the OpenGL specification, making it a safe bet to hardcode.
            size_t offset;
            /// Size in bytes to bind the tex buffer. When zero,
            /// binds from offset until the end of the buffer.
            size_t sizeBytes;
            /// Access. Should match what the shader expects. Needed by Ogre to
            /// resolve memory barrier dependencies.
            ResourceAccess::ResourceAccess access;

            bool operator != ( const BufferSlot &other ) const
            {
                return  this->buffer != other.buffer ||
                        this->offset != other.offset ||
                        this->sizeBytes != other.sizeBytes ||
                        this->access != other.access;
            }

            bool operator < ( const BufferSlot &other ) const
            {
                if( this->buffer != other.buffer )
                    return this->buffer < other.buffer;
                if( this->offset != other.offset )
                    return this->offset < other.offset;
                if( this->sizeBytes != other.sizeBytes )
                    return this->sizeBytes < other.sizeBytes;
                if( this->access != other.access )
                    return this->access < other.access;

                return false;
            }

            static BufferSlot makeEmpty(void)
            {
                BufferSlot retVal;
                memset( &retVal, 0, sizeof( retVal ) );
                return retVal;
            }
        };
        struct _OgreExport TextureSlot
        {
            TextureGpu *texture;

            ResourceAccess::ResourceAccess access;
            uint8           mipmapLevel;
            uint16          textureArrayIndex;
            /// When left as PFG_UNKNOWN, we'll automatically use the TextureGpu's native format
            PixelFormatGpu  pixelFormat;

            bool operator != ( const TextureSlot &other ) const
            {
                return  this->texture != other.texture ||
                        this->access != other.access ||
                        this->mipmapLevel != other.mipmapLevel ||
                        this->textureArrayIndex != other.textureArrayIndex ||
                        this->pixelFormat != other.pixelFormat;
            }

            bool operator < ( const TextureSlot &other ) const
            {
                if( this->texture != other.texture )
                    return this->texture < other.texture;
                if( this->access != other.access )
                    return this->access < other.access;
                if( this->mipmapLevel != other.mipmapLevel )
                    return this->mipmapLevel < other.mipmapLevel;
                if( this->textureArrayIndex != other.textureArrayIndex )
                    return this->textureArrayIndex < other.textureArrayIndex;
                if( this->pixelFormat != other.pixelFormat )
                    return this->pixelFormat < other.pixelFormat;

                return false;
            }

            bool formatNeedsReinterpret(void) const;
            bool needsDifferentView(void) const;

            static TextureSlot makeEmpty(void)
            {
                TextureSlot retVal;
                memset( &retVal, 0, sizeof( retVal ) );
                return retVal;
            }
        };
        struct _OgreExport Slot
        {
            SlotType        slotType;
        protected:
            union
            {
                BufferSlot  buffer;
                TextureSlot texture;
            };
        public:
            Slot()
            {
                memset( this, 0, sizeof(*this) );
            }

            Slot( SlotType _slotType )
            {
                memset( this, 0, sizeof(*this) );
                slotType = _slotType;
            }

            bool empty(void) const
            {
                return buffer.buffer == 0 && texture.texture == 0;
            }

            bool isBuffer(void) const
            {
                return slotType == SlotTypeBuffer;
            }

            BufferSlot& getBuffer(void)
            {
                assert( slotType == SlotTypeBuffer );
                return buffer;
            }

            const BufferSlot& getBuffer(void) const
            {
                assert( slotType == SlotTypeBuffer );
                return buffer;
            }

            bool isTexture(void) const
            {
                return slotType == SlotTypeTexture;
            }

            TextureSlot& getTexture(void)
            {
                assert( slotType == SlotTypeTexture );
                return texture;
            }

            const TextureSlot& getTexture(void) const
            {
                assert( slotType == SlotTypeTexture );
                return texture;
            }

            bool operator != ( const Slot &other ) const
            {
                if( this->slotType != other.slotType )
                    return true;

                if( this->slotType == SlotTypeBuffer )
                {
                    return this->buffer != other.buffer;
                }
                else
                {
                    return this->texture != other.texture;
                }
            }

            bool operator < ( const Slot &other ) const
            {
                if( this->slotType != other.slotType )
                    return this->slotType < other.slotType;

                if( this->slotType == SlotTypeBuffer )
                {
                    return this->buffer < other.buffer;
                }
                else
                {
                    return this->texture < other.texture;
                }
            }
        };

        uint16          mRefCount;
        void            *mRsData;           /// Render-System specific data
        FastArray<Slot> mUavs;

        DescriptorSetUav() :
            mRefCount( 0 ),
            mRsData( 0 )
        {
        }

        /// Warning: This operator won't see changes in UAVs (i.e. data baked into mRsData).
        /// If you get notifyTextureChanged call, the UAV has changed and you must
        /// assume the DescriptorSetUav has changed.
        /// UAV = Unordered Access View.
        bool operator != ( const DescriptorSetUav &other ) const
        {
            const size_t thisNumUavs = mUavs.size();
            if( thisNumUavs != other.mUavs.size() )
                return true;

            for( size_t i=0; i<thisNumUavs; ++i )
            {
                if( this->mUavs[i] != other.mUavs[i] )
                    return true;
            }

            return false;
        }

        bool operator < ( const DescriptorSetUav &other ) const
        {
            const size_t thisNumUavs = mUavs.size();
            if( thisNumUavs != other.mUavs.size() )
                return thisNumUavs < other.mUavs.size();

            for( size_t i=0; i<thisNumUavs; ++i )
            {
                if( this->mUavs[i] != other.mUavs[i] )
                    return this->mUavs[i] < other.mUavs[i];
            }

            return false;
        }

        void checkValidity(void) const;
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
