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
#ifndef __HardwareVertexBuffer__
#define __HardwareVertexBuffer__

#include "OgrePrerequisites.h"

// Precompiler options
#include "OgreColourValue.h"
#include "OgreCommon.h"
#include "OgreHardwareBuffer.h"
#include "OgreSharedPtr.h"
#include "Vao/OgreVertexBufferPacked.h"

#include "ogrestd/list.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    namespace v1
    {
        class HardwareBufferManagerBase;

        /** \addtogroup Core
         *  @{
         */
        /** \addtogroup RenderSystem
         *  @{
         */
        /** Specialisation of HardwareBuffer for a vertex buffer. */
        class _OgreExport HardwareVertexBuffer : public HardwareBuffer
        {
        protected:
            HardwareBufferManagerBase *mMgr;

            size_t mNumVertices;
            size_t mVertexSize;
            bool   mIsInstanceData;
            size_t mInstanceDataStepRate;
            /// Checks if vertex instance data is supported by the render system
            virtual bool checkIfVertexInstanceDataIsSupported();

        public:
            /// Should be called by HardwareBufferManager
            HardwareVertexBuffer( HardwareBufferManagerBase *mgr, size_t vertexSize, size_t numVertices,
                                  HardwareBuffer::Usage usage, bool useSystemMemory,
                                  bool useShadowBuffer );
            ~HardwareVertexBuffer();
            /// Return the manager of this buffer, if any
            HardwareBufferManagerBase *getManager() const { return mMgr; }
            /// Gets the size in bytes of a single vertex in this buffer
            size_t getVertexSize() const { return mVertexSize; }
            /// Get the number of vertices in this buffer
            size_t getNumVertices() const { return mNumVertices; }
            /// Get if this vertex buffer is an "instance data" buffer (per instance)
            bool getIsInstanceData() const { return mIsInstanceData; }
            /// Set if this vertex buffer is an "instance data" buffer (per instance)
            void setIsInstanceData( const bool val );
            /// Get the number of instances to draw using the same per-instance data before advancing in
            /// the buffer by one element.
            size_t getInstanceDataStepRate() const;
            /// Set the number of instances to draw using the same per-instance data before advancing in
            /// the buffer by one element.
            void setInstanceDataStepRate( const size_t val );

            // NB subclasses should override lock, unlock, readData, writeData
        };

        /** Shared pointer implementation used to share vertex buffers. */
        class _OgreExport HardwareVertexBufferSharedPtr : public SharedPtr<HardwareVertexBuffer>
        {
        public:
            HardwareVertexBufferSharedPtr() : SharedPtr<HardwareVertexBuffer>() {}
            explicit HardwareVertexBufferSharedPtr( HardwareVertexBuffer *buf );
        };

        /** Locking helper. */
        typedef HardwareBufferLockGuard
            HardwareVertexBufferLockGuard;  // deprecated, use HardwareBufferLockGuard directly
    }                                       // namespace v1

    namespace v1
    {
        /** This class declares the usage of a single vertex buffer as a component
            of a complete VertexDeclaration.
            @remarks
            Several vertex buffers can be used to supply the input geometry for a
            rendering operation, and in each case a vertex buffer can be used in
            different ways for different operations; the buffer itself does not
            define the semantics (position, normal etc), the VertexElement
            class does.
        */
        class _OgreExport VertexElement : public OgreAllocatedObj
        {
        protected:
            /// The source vertex buffer, as bound to an index using VertexBufferBinding
            unsigned short mSource;
            /// The offset in the buffer that this element starts at
            size_t mOffset;
            /// The type of element
            VertexElementType mType;
            /// The meaning of the element
            VertexElementSemantic mSemantic;
            /// Index of the item, only applicable for some elements like texture coords
            unsigned short mIndex;

        public:
            /// Constructor, should not be called directly, only needed because of list
            VertexElement() {}
            /// Constructor, should not be called directly, call VertexDeclaration::addElement
            VertexElement( unsigned short source, size_t offset, VertexElementType theType,
                           VertexElementSemantic semantic, unsigned short index = 0 );
            /// Gets the vertex buffer index from where this element draws it's values
            unsigned short getSource() const { return mSource; }
            /// Gets the offset into the buffer where this element starts
            size_t getOffset() const { return mOffset; }
            /// Gets the data format of this element
            VertexElementType getType() const { return mType; }
            /// Gets the meaning of this element
            VertexElementSemantic getSemantic() const { return mSemantic; }
            /// Gets the index of this element, only applicable for repeating elements
            unsigned short getIndex() const { return mIndex; }
            /// Gets the size of this element in bytes
            uint32 getSize() const;
            /// Utility method for helping to calculate offsets
            static uint32 getTypeSize( VertexElementType etype );
            /// Utility method which returns the count of values in a given type
            static uint8 getTypeCount( VertexElementType etype );
            /// Utility method which returns true if the type is SNORM or NORM
            static bool isTypeNormalized( VertexElementType etype );
            /** Simple converter function which will turn a single-value type into a
                multi-value type based on a parameter.
            */
            static VertexElementType multiplyTypeCount( VertexElementType baseType,
                                                        unsigned short    count );
            /** Simple converter function which will a type into it's single-value
                equivalent - makes switches on type easier.
            */
            static VertexElementType getBaseType( VertexElementType multiType );

            /** Utility method for converting colour from
                one packed 32-bit colour type to another.
            @param srcType The source type
            @param dstType The destination type
            @param ptr Read / write value to change
            */
            static void convertColourValue( VertexElementType srcType, VertexElementType dstType,
                                            uint32 *ptr );

            /** Utility method for converting colour to
                a packed 32-bit colour type.
            @param src source colour
            @param dst The destination type
            */
            static uint32 convertColourValue( const ColourValue &src, VertexElementType dst );

            /** Utility method to get the most appropriate packed colour vertex element format. */
            static VertexElementType getBestColourVertexElementType();

            inline bool operator==( const VertexElement &rhs ) const
            {
                if( mType != rhs.mType || mIndex != rhs.mIndex || mOffset != rhs.mOffset ||
                    mSemantic != rhs.mSemantic || mSource != rhs.mSource )
                    return false;
                else
                    return true;
            }
            /** Adjusts a pointer to the base of a vertex to point at this element.
            @remarks
                This variant is for void pointers, passed as a parameter because we can't
                rely on covariant return types.
            @param pBase Pointer to the start of a vertex in this buffer.
            @param pElem Pointer to a pointer which will be set to the start of this element.
            */
            inline void baseVertexPointerToElement( void *pBase, void **pElem ) const
            {
                // The only way we can do this is to cast to char* in order to use byte offset
                // then cast back to void*.
                *pElem = static_cast<void *>( static_cast<unsigned char *>( pBase ) + mOffset );
            }
            /** Adjusts a pointer to the base of a vertex to point at this element.
            @remarks
                This variant is for float pointers, passed as a parameter because we can't
                rely on covariant return types.
            @param pBase Pointer to the start of a vertex in this buffer.
            @param pElem Pointer to a pointer which will be set to the start of this element.
            */
            inline void baseVertexPointerToElement( void *pBase, float **pElem ) const
            {
                // The only way we can do this is to cast to char* in order to use byte offset
                // then cast back to float*. However we have to go via void* because casting
                // directly is not allowed
                *pElem = static_cast<float *>(
                    static_cast<void *>( static_cast<unsigned char *>( pBase ) + mOffset ) );
            }

            /** Adjusts a pointer to the base of a vertex to point at this element.
            @remarks
                This variant is for RGBA pointers, passed as a parameter because we can't
                rely on covariant return types.
            @param pBase Pointer to the start of a vertex in this buffer.
            @param pElem Pointer to a pointer which will be set to the start of this element.
            */
            inline void baseVertexPointerToElement( void *pBase, RGBA **pElem ) const
            {
                *pElem = static_cast<RGBA *>(
                    static_cast<void *>( static_cast<unsigned char *>( pBase ) + mOffset ) );
            }
            /** Adjusts a pointer to the base of a vertex to point at this element.
            @remarks
                This variant is for char pointers, passed as a parameter because we can't
                rely on covariant return types.
            @param pBase Pointer to the start of a vertex in this buffer.
            @param pElem Pointer to a pointer which will be set to the start of this element.
            */
            inline void baseVertexPointerToElement( void *pBase, unsigned char **pElem ) const
            {
                *pElem = static_cast<unsigned char *>( pBase ) + mOffset;
            }

            /** Adjusts a pointer to the base of a vertex to point at this element.
            @remarks
            This variant is for ushort pointers, passed as a parameter because we can't
            rely on covariant return types.
            @param pBase Pointer to the start of a vertex in this buffer.
            @param pElem Pointer to a pointer which will be set to the start of this element.
            */
            inline void baseVertexPointerToElement( void *pBase, unsigned short **pElem ) const
            {
                *pElem = static_cast<unsigned short *>(
                    static_cast<void *>( static_cast<unsigned char *>( pBase ) + mOffset ) );
            }
        };
        /** This class declares the format of a set of vertex inputs, which
            can be issued to the rendering API through a RenderOperation.
        @remarks
        You should be aware that the ordering and structure of the
        VertexDeclaration can be very important on DirectX with older
        cards,so if you want to maintain maximum compatibility with
        all render systems and all cards you should be careful to follow these
        rules:<ol>
        <li>VertexElements should be added in the following order, and the order of the
        elements within a shared buffer should be as follows:
        position, blending weights, normals, diffuse colours, specular colours,
                texture coordinates (in order, with no gaps)</li>
        <li>You must not have unused gaps in your buffers which are not referenced
        by any VertexElement</li>
        <li>You must not cause the buffer & offset settings of 2 VertexElements to overlap</li>
        </ol>
        Whilst GL and more modern graphics cards in D3D will allow you to defy these rules,
        sticking to them will ensure that your buffers have the maximum compatibility.
        @par
        Like the other classes in this functional area, these declarations should be created and
        destroyed using the HardwareBufferManager.
        */
        class _OgreExport VertexDeclaration : public OgreAllocatedObj
        {
        public:
            /// Defines the list of vertex elements that makes up this declaration
            typedef list<VertexElement>::type VertexElementList;
            /// Sort routine for vertex elements
            static bool vertexElementLess( const VertexElement &e1, const VertexElement &e2 );
            static bool vertexElementLessForV2( const VertexElement &e1, const VertexElement &e2 );

        protected:
            HardwareBufferManagerBase *mCreator;

            VertexElementList mElementList;
            /// See VertexArrayObject::mInputLayoutId
            /// This value does not contain the OperationType in the last 6 bits, thus it needs
            /// to be added later
            uint16 mBaseInputLayoutId;
            bool   mInputLayoutDirty;

            void vertexLayoutDirty();

        public:
            /// Standard constructor, not you should use HardwareBufferManager::createVertexDeclaration
            VertexDeclaration( HardwareBufferManagerBase *creator );
            virtual ~VertexDeclaration();

            /** VertexDeclaration do not store OperationType, which is why we cache just the base 10 bits

                This function is not const as it will modify mBaseInputLayoutId if the layout is dirty

                @see    HlmsManager::_getInputLayoutId
                @see    VertexDeclaration::mBaseInputLayoutId
            @param hlmsManager
            @param opType
            @return
            */
            uint16 _getInputLayoutId( HlmsManager *hlmsManager, OperationType opType );

            /** Get the number of elements in the declaration. */
            size_t getElementCount() const { return mElementList.size(); }
            /** Gets read-only access to the list of vertex elements. */
            const VertexElementList &getElements() const;
            /** Get a single element. */
            const VertexElement *getElement( unsigned short index ) const;

            /** Sorts the elements in this list to be compatible with the maximum
                number of rendering APIs / graphics cards.
            @remarks
                Older graphics cards require vertex data to be presented in a more
                rigid way, as defined in the main documentation for this class. As well
                as the ordering being important, where shared source buffers are used, the
                declaration must list all the elements for each source in turn.
            */
            void sort();

            void sortForV2();

            /** Remove any gaps in the source buffer list used by this declaration.
            @remarks
                This is useful if you've modified a declaration and want to remove
                any gaps in the list of buffers being used. Note, however, that if this
                declaration is already being used with a VertexBufferBinding, you will
                need to alter that too. This method is mainly useful when reorganising
                buffers based on an altered declaration.
            @note
                This will cause the vertex declaration to be re-sorted.
            */
            void closeGapsInSource();

            /** Generates a new VertexDeclaration for optimal usage based on the current
                vertex declaration, which can be used with VertexData::reorganiseBuffers later
                if you wish, or simply used as a template.
            @remarks
                Different buffer organisations and buffer usages will be returned
                depending on the parameters passed to this method.
            @param skeletalAnimation Whether this vertex data is going to be
                skeletally animated
            @param vertexAnimation Whether this vertex data is going to be vertex animated
            @param vertexAnimationNormals Whether vertex data animation is going to include normals
            animation
            */
            VertexDeclaration *getAutoOrganisedDeclaration( bool skeletalAnimation, bool vertexAnimation,
                                                            bool vertexAnimationNormals ) const;

            /** Gets the index of the highest source value referenced by this declaration. */
            unsigned short getMaxSource() const;

            /** Adds a new VertexElement to this declaration.
            @remarks
                This method adds a single element (positions, normals etc) to the end of the
                vertex declaration. <b>Please read the information in VertexDeclaration about
            the importance of ordering and structure for compatibility with older D3D drivers</b>.
            @param source The binding index of HardwareVertexBuffer which will provide the source for
            this element. See VertexBufferBinding for full information.
            @param offset The offset in bytes where this element is located in the buffer
            @param theType The data format of the element (3 floats, a colour etc)
            @param semantic The meaning of the data (position, normal, diffuse colour etc)
            @param index Optional index for multi-input elements like texture coordinates
            @return A reference to the VertexElement added.
            */
            virtual const VertexElement &addElement( unsigned short source, size_t offset,
                                                     VertexElementType     theType,
                                                     VertexElementSemantic semantic,
                                                     unsigned short        index = 0 );
            /** Inserts a new VertexElement at a given position in this declaration.
            @remarks
            This method adds a single element (positions, normals etc) at a given position in this
            vertex declaration. <b>Please read the information in VertexDeclaration about
            the importance of ordering and structure for compatibility with older D3D drivers</b>.
            @param source The binding index of HardwareVertexBuffer which will provide the source for
            this element. See VertexBufferBinding for full information.
            @param offset The offset in bytes where this element is located in the buffer
            @param theType The data format of the element (3 floats, a colour etc)
            @param semantic The meaning of the data (position, normal, diffuse colour etc)
            @param index Optional index for multi-input elements like texture coordinates
            @return A reference to the VertexElement added.
            */
            virtual const VertexElement &insertElement( unsigned short atPosition, unsigned short source,
                                                        size_t offset, VertexElementType theType,
                                                        VertexElementSemantic semantic,
                                                        unsigned short        index = 0 );

            /** Remove the element at the given index from this declaration. */
            virtual void removeElement( unsigned short elem_index );

            /** Remove the element with the given semantic and usage index.
            @remarks
                In this case 'index' means the usage index for repeating elements such
                as texture coordinates. For other elements this will always be 0 and does
                not refer to the index in the vector.
            */
            virtual void removeElement( VertexElementSemantic semantic, unsigned short index = 0 );

            /** Remove all elements. */
            virtual void removeAllElements();

            /** Modify an element in-place, params as addElement.
           @remarks
           <b>Please read the information in VertexDeclaration about
            the importance of ordering and structure for compatibility with older D3D drivers</b>.
         */
            virtual void modifyElement( unsigned short elem_index, unsigned short source, size_t offset,
                                        VertexElementType theType, VertexElementSemantic semantic,
                                        unsigned short index = 0 );

            /** Finds a VertexElement with the given semantic, and index if there is more than
                one element with the same semantic.
            @remarks
                If the element is not found, this method returns null.
            */
            virtual const VertexElement *findElementBySemantic( VertexElementSemantic sem,
                                                                unsigned short        index = 0 ) const;
            /** Based on the current elements, gets the size of the vertex for a given buffer source.
            @param source The buffer binding index for which to get the vertex size.
            */

            /** Gets a list of elements which use a given source.
            @remarks
                Note that the list of elements is returned by value therefore is separate from
                the declaration as soon as this method returns.
            */
            virtual VertexElementList findElementsBySource( unsigned short source ) const;

            /** Gets the vertex size defined by this declaration for a given source. */
            virtual size_t getVertexSize( unsigned short source ) const;

            /** Return the index of the next free texture coordinate set which may be added
                to this declaration.
            */
            virtual unsigned short getNextFreeTextureCoordinate() const;

            /** Clones this declaration.
            @param mgr Optional HardwareBufferManager to use for creating the clone
                (if null, use the current default).
            */
            virtual VertexDeclaration *clone( HardwareBufferManagerBase *mgr = 0 ) const;

            bool isSortedForV2() const;

            VertexElement2VecVec convertToV2();

            void convertFromV2( const VertexElement2Vec &v2Decl );
            void convertFromV2( const VertexElement2VecVec &v2Decl );

            inline bool operator==( const VertexDeclaration &rhs ) const
            {
                if( mElementList.size() != rhs.mElementList.size() )
                    return false;

                VertexElementList::const_iterator i, iend, rhsi, rhsiend;
                iend = mElementList.end();
                rhsiend = rhs.mElementList.end();
                rhsi = rhs.mElementList.begin();
                for( i = mElementList.begin(); i != iend && rhsi != rhsiend; ++i, ++rhsi )
                {
                    if( !( *i == *rhsi ) )
                        return false;
                }

                return true;
            }
            inline bool operator!=( const VertexDeclaration &rhs ) const { return !( *this == rhs ); }
        };

        /** Records the state of all the vertex buffer bindings required to provide a vertex declaration
            with the input data it needs for the vertex elements.
        @remarks
            Why do we have this binding list rather than just have VertexElement referring to the
            vertex buffers direct? Well, in the underlying APIs, binding the vertex buffers to an
            index (or 'stream') is the way that vertex data is linked, so this structure better
            reflects the realities of that. In addition, by separating the vertex declaration from
            the list of vertex buffer bindings, it becomes possible to reuse bindings between
        declarations and vice versa, giving opportunities to reduce the state changes required to perform
        rendering.
        @par
            Like the other classes in this functional area, these binding maps should be created and
            destroyed using the HardwareBufferManager.
        */
        class _OgreExport VertexBufferBinding : public OgreAllocatedObj
        {
        public:
            /// Defines the vertex buffer bindings used as source for vertex declarations
            typedef map<unsigned short, HardwareVertexBufferSharedPtr>::type VertexBufferBindingMap;

        protected:
            VertexBufferBindingMap mBindingMap;
            mutable unsigned short mHighIndex;

        public:
            /// Constructor, should not be called direct, use
            /// HardwareBufferManager::createVertexBufferBinding
            VertexBufferBinding();
            virtual ~VertexBufferBinding();
            /** Set a binding, associating a vertex buffer with a given index.
            @remarks
                If the index is already associated with a vertex buffer,
                the association will be replaced. This may cause the old buffer
                to be destroyed if nothing else is referring to it.
                You should assign bindings from 0 and not leave gaps, although you can
                bind them in any order.
            */
            virtual void setBinding( unsigned short index, const HardwareVertexBufferSharedPtr &buffer );
            /** Removes an existing binding. */
            virtual void unsetBinding( unsigned short index );

            /** Removes all the bindings. */
            virtual void unsetAllBindings();

            /// Gets a read-only version of the buffer bindings
            virtual const VertexBufferBindingMap &getBindings() const;

            /// Gets the buffer bound to the given source index
            virtual const HardwareVertexBufferSharedPtr &getBuffer( unsigned short index ) const;
            /// Gets whether a buffer is bound to the given source index
            virtual bool isBufferBound( unsigned short index ) const;

            virtual size_t getBufferCount() const { return mBindingMap.size(); }

            /** Gets the highest index which has already been set, plus 1.
            @remarks
                This is to assist in binding the vertex buffers such that there are
                not gaps in the list.
            */
            virtual unsigned short getNextIndex() const { return mHighIndex++; }

            /** Gets the last bound index.
             */
            virtual unsigned short getLastBoundIndex() const;

            typedef map<ushort, ushort>::type BindingIndexMap;

            /** Check whether any gaps in the bindings.
             */
            virtual bool hasGaps() const;

            /** Remove any gaps in the bindings.
            @remarks
                This is useful if you've removed vertex buffer from this vertex buffer
                bindings and want to remove any gaps in the bindings. Note, however,
                that if this bindings is already being used with a VertexDeclaration,
                you will need to alter that too. This method is mainly useful when
                reorganising buffers manually.
            @param
                bindingIndexMap To be retrieve the binding index map that used to
                translation old index to new index; will be cleared by this method
                before fill-in.
            */
            virtual void closeGaps( BindingIndexMap &bindingIndexMap );

            /// Returns true if this binding has an element that contains instance data
            virtual bool getHasInstanceData() const;
        };
        /** @} */
        /** @} */

    }  // namespace v1
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
