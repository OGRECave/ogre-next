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

#ifndef __CompositorNode_H__
#define __CompositorNode_H__

#include "Compositor/OgreCompositorChannel.h"
#include "Compositor/OgreCompositorNamedBuffer.h"
#include "OgreId.h"
#include "OgreIdString.h"
#include "OgreResourceTransition.h"

#include "ogrestd/map.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class CompositorNodeDef;

    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Effects
     *  @{
     */

    struct BoundUav;

    /** Compositor nodes are the core subject of compositing.
        This is an instantiation. All const, shared parameters are in the definition
        (CompositorNodeDef) and we assume they don't change throughout the lifetime
        of our instance.
    @par
        The textures in mLocalTextures are managed by us and we're responsible for
        freeing them when they're no longer needed.
    @par
        Before nodes can be used, they have to be connected between each other,
        followed by a call to routeOutputs()
        Connections must be done in a very specific order, so let the manager
        take care of solving the dependencies. Basically the problem is
        that if the chain is like this: A -> B -> C; if we connect node
        B to C first, then there's a chance of giving null pointers to C
        instead of the valid ones that belong to A.
    @par
        To solve this problem, we first start with nodes that have no input,
        and then continue with those who have all of their input set; then repeat
        until there are no nodes to be processed.
        If there's still nodes with input left open; then those nodes can't be
        activated and the workspace is invalid.
    @par
        No Node can be valid if it has disconnected input channels left.
        Nodes can have no input because they either use passes that don't need it
        (eg. scene pass) or use global textures as means for sharing their work
        Similarly, Nodes may have no output because they use global textures.
    @par
        Nodes with feedback loops are not supported and may or may not work.
        A feedback loop is when A's output is used in B, B to C, then
        C is plugged back into A.
    @par
        It's possible to assign the same output to two different input channels,
        though it could work very unintuitively... (because two textures that may
        be intended to be hard copies are actually sharing the same memory)
    @remarks
        We own the local textures, so it's our job to destroy them
    @author
        Matias N. Goldberg
    @version
        1.0
    */
    class _OgreExport CompositorNode : public OgreAllocatedObj, public IdObject
    {
    protected:
        /// Unique name across the same workspace
        IdString mName;
        bool     mEnabled;

        /// Must be <= mInTextures.size(). Tracks how many pointers are not null in mInTextures
        size_t               mNumConnectedInputs;
        CompositorChannelVec mInTextures;
        CompositorChannelVec mLocalTextures;

        /// Contains pointers that are ither in mInTextures or mLocalTextures
        CompositorChannelVec mOutTextures;

        size_t                   mNumConnectedBufferInputs;
        CompositorNamedBufferVec mBuffers;

        CompositorPassVec mPasses;

        /// Nodes we're connected to. If we destroy our local textures, we need to inform them
        CompositorNodeVec mConnectedNodes;

        CompositorWorkspace *mWorkspace;

        RenderSystem *mRenderSystem;  /// Used to create/destroy MRTs

        /** Fills mOutTextures with the pointers from mInTextures & mLocalTextures according
            to CompositorNodeDef::mOutChannelMapping. Call this immediately after modifying
            mInTextures or mLocalTextures
        */
        void routeOutputs();

        /** Disconnects this node's output from all nodes we send our textures to. We only
            disconnect local textures.
        @remarks
            Textures that we got from our input channel and then pass it to the output channel
            are left untouched. This allows for some node to be plugged back & forth without
            making much mess and leaving everything else working.
        */
        void disconnectOutput();

        /// Makes global buffers visible to our passes. Must be done last in case
        /// there's an input/local buffer with the same name as a global buffer
        /// (local scope prevails over global scope)
        void populateGlobalBuffers();

        /** Called right after we create a pass. Derived
            classes may want to do something with it
        @param pass
            Newly created pass to toy with.
        */
        virtual void postInitializePass( CompositorPass *pass ) {}

    public:
        /** The Id must be unique across all engine so we can create unique named textures.
            The name is only unique across the workspace
        */
        CompositorNode( IdType id, IdString name, const CompositorNodeDef *definition,
                        CompositorWorkspace *workspace, RenderSystem *renderSys,
                        TextureGpu *finalTarget );
        virtual ~CompositorNode();

        void destroyAllPasses();

        IdString                 getName() const { return mName; }
        const CompositorNodeDef *getDefinition() const { return mDefinition; }

        RenderSystem *getRenderSystem() const { return mRenderSystem; }

        /** Enables or disables all instances of this node
        @remarks
            Note that we just won't execute our passes. It's your job to change the
            channel connections accordingly if you have to.
            A disabled node won't complain when its connections are incomplete in
            a workspace.
        @par
            This function is useful frequently toggling a compositor effect without having
            to recreate any API resource (which often would involve stalls).
        */
        void setEnabled( bool bEnabled );

        /// Returns if this instance is enabled. @See setEnabled
        bool getEnabled() const { return mEnabled; }

        /** Connects this node (let's call it node 'A') to node 'B', mapping the output
            channel from A into the input channel from B (buffer version)
        @param outChannelA
            Output to use from node A.
        @param inChannelB
            Input to connect the output from A.
        */
        void connectBufferTo( size_t outChannelA, CompositorNode *nodeB, size_t inChannelB );

        /** Connects this node (let's call it node 'A') to node 'B', mapping the output
            channel from A into the input channel from B (texture version)
        @param outChannelA
            Output to use from node A.
        @param inChannelB
            Input to connect the output from A.
        */
        void connectTo( size_t outChannelA, CompositorNode *nodeB, size_t inChannelB );

        /** Connects (injects) an external RT into the given channel. Usually used for
            the "connect_output" / "connect_external" directive for the RenderWindow.
        @param rt
            The RenderTarget.
        @param textures
            The Textures associated with the RT. Can be empty (eg. RenderWindow) but
            could cause crashes/exceptions if tried to use in PASS_QUAD passes.
        @param inChannelA
            In which channel number to inject to.
        */
        void connectExternalRT( TextureGpu *externalTexture, size_t inChannelA );

        /** Connects (injects) an external buffer into the given channel. Usually used for
            the 'connect_buffer_external' directive.
        @param buffer
            The buffer.
        @param inChannelA
            In which channel number to inject to.
        */
        void connectExternalBuffer( UavBufferPacked *buffer, size_t inChannelA );

        bool                        areAllInputsConnected() const;
        const CompositorChannelVec &getInputChannel() const { return mInTextures; }
        const CompositorChannelVec &getLocalTextures() const { return mLocalTextures; }

        /** Returns the texture pointer of a texture based on it's name & mrt index.
        @remarks
            The texture name must have been registered with
            CompositorNodeDef::addTextureSourceName
        @param textureName
            The name of the texture. This name may only be valid at node scope. It can
            refer to an input texture, a local texture, or a global one.
            If the global texture wasn't registered with addTextureSourceName,
            it will fail.
        @return
            Null if not found (or global texture not registered). The texture otherwise
        */
        TextureGpu *getDefinedTexture( IdString textureName ) const;

        /** Returns the buffer pointer of a buffer based on it's name.
        @remarks
            The buffer may come from a local buffer, an input buffer, or
            global (workspace).
        @param bufferName
            The name of the buffer. This name may only be valid at node scope. It can
            refer to an input buffer, a local buffer, or a global one.
            If a local or input buffer has the same name as a global one, the global
            one is ignored.
        @return
            Regular: The buffer. Throws if buffer wasn't found.
            No throw version: Null if not found. The buffer otherwise
        */
        UavBufferPacked *getDefinedBuffer( IdString bufferName ) const;
        UavBufferPacked *getDefinedBufferNoThrow( IdString bufferName ) const;

        /** Creates all passes based on our definition
        @remarks
            Call this function after connecting all channels (at least our input)
            otherwise we may bind null pointer RTs to the passes (and then crash)
            @See connectTo and @see connectFinalRT
        */
        void createPasses();

        const CompositorPassVec &_getPasses() const { return mPasses; }

        /** Calling this function every frame will cause us to execute all our passes (ie. render)
        @param lodCamera
            LOD Camera to be used by our passes. Pointer can be null, and note however passes can
            ignore this hint and use their own camera pointer for LOD (this parameter is mostly
            used for syncing shadow mapping).
        */
        void _update( const Camera *lodCamera, SceneManager *sceneManager );

        /** Call this function when you're replacing the textures from oldChannel with the
            ones in newChannel. Useful when recreating textures (i.e. resolution changed)
        @param oldChannel
            The old textures that are going to be removed. Pointers in it must be still valid
        @param newChannel
            The new replacement textures
        */
        void notifyRecreated( TextureGpu *channel );
        void notifyRecreated( const UavBufferPacked *oldBuffer, UavBufferPacked *newBuffer );

        /** Call this function when caller has destroyed a RenderTarget in which the callee
            may have a reference to that pointer, so that we can clean it up.
        @param channel
            Channel containing the pointer about to be destroyed (must still be valid)
        */
        void notifyDestroyed( TextureGpu *channel );
        void notifyDestroyed( const UavBufferPacked *buffer );

        /** Internal Use. Called when connections are all being zero'ed. We rely our
            caller is doing this to all nodes, hence we do not notify our @mConnectedNodes
            nodes. Failing to clear them too may leave dangling pointers or graphical glitches
        @remarks
            Destroys all of our passes.
        */
        void _notifyCleared();

        /** Called by CompositorManager2 when (i.e.) the RenderWindow was resized, thus our
            RTs that depend on their resolution need to be recreated.
        @remarks
            We inform all connected nodes and passes related to us of RenderTargets/Textures
            that may have been recreated (pointers could become danlging otherwise).
        @par
            This is divided in two steps: recreateResizableTextures01 & recreateResizableTextures02
            since in some cases in RenderPassDescriptor, setting up MRT and depth textures
            requires all textures to be up to date, otherwise validation errors would occur
            since we'll have partial data (e.g. MRT 0 is 1024x768 while MRT 1 is 800x600)
        @param finalTarget
            The Final Target (i.e. RenderWindow) from which we'll base our local textures'
            resolution.
        */
        virtual void finalTargetResized01( const TextureGpu *finalTarget );
        virtual void finalTargetResized02( const TextureGpu *finalTarget );

        /// @copydoc CompositorWorkspace::resetAllNumPassesLeft
        void resetAllNumPassesLeft();

        /// @copydoc CompositorPassDef::getPassNumber
        size_t getPassNumber( CompositorPass *pass ) const;

        /// Returns our parent workspace
        CompositorWorkspace *getWorkspace() { return mWorkspace; }

        /// Returns our parent workspace
        const CompositorWorkspace *getWorkspace() const { return mWorkspace; }

    private:
        CompositorNodeDef const *mDefinition;
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
