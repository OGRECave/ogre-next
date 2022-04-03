/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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
#ifndef __TextureUnitState_H__
#define __TextureUnitState_H__

#include "OgrePrerequisites.h"

#include "OgreBlendMode.h"
#include "OgreCommon.h"
#include "OgreIdString.h"
#include "OgreMatrix4.h"
#include "OgreTextureGpu.h"
#include "OgreTextureGpuListener.h"

#include "ogrestd/map.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Materials
     *  @{
     */
    /** Class representing the state of a single texture unit during a Pass of a
        Technique, of a Material.
    @remarks
        Texture units are pipelines for retrieving texture data for rendering onto
        your objects in the world. Using them is common to both the fixed-function and
        the programmable (vertex and fragment program) pipeline, but some of the
        settings will only have an effect in the fixed-function pipeline (for example,
        setting a texture rotation will have no effect if you use the programmable
        pipeline, because this is overridden by the fragment program). The effect
        of each setting as regards the 2 pipelines is commented in each setting.
    @par
        When I use the term 'fixed-function pipeline' I mean traditional rendering
        where you do not use vertex or fragment programs (shaders). Programmable
        pipeline means that for this pass you are using vertex or fragment programs.
    */
    class _OgreExport TextureUnitState : public OgreAllocatedObj, public TextureGpuListener
    {
        friend class RenderSystem;

    public:
        /** Definition of the broad types of texture effect you can apply to a texture unit.
        @note
            Note that these have no effect when using the programmable pipeline, since their
            effect is overridden by the vertex / fragment programs.
        */
        enum TextureEffectType
        {
            /// Generate all texture coords based on angle between camera and vertex.
            ET_ENVIRONMENT_MAP,
            /// Generate texture coords based on a frustum.
            ET_PROJECTIVE_TEXTURE,
            /// Constant u/v scrolling effect.
            ET_UVSCROLL,
            /// Constant u scrolling effect.
            ET_USCROLL,
            /// Constant u/v scrolling effect.
            ET_VSCROLL,
            /// Constant rotation.
            ET_ROTATE,
            /// More complex transform.
            ET_TRANSFORM

        };

        /** Enumeration to specify type of envmap.
        @note
            Note that these have no effect when using the programmable pipeline, since their
            effect is overridden by the vertex / fragment programs.
        */
        enum EnvMapType
        {
            /// Envmap based on vector from camera to vertex position, good for planar geometry.
            ENV_PLANAR,
            /// Envmap based on dot of vector from camera to vertex and vertex normal, good for curves.
            ENV_CURVED,
            /// Envmap intended to supply reflection vectors for cube mapping.
            ENV_REFLECTION,
            /// Envmap intended to supply normal vectors for cube mapping.
            ENV_NORMAL
        };

        /** Useful enumeration when dealing with procedural transforms.
        @note
            Note that these have no effect when using the programmable pipeline, since their
            effect is overridden by the vertex / fragment programs.
        */
        enum TextureTransformType
        {
            TT_TRANSLATE_U,
            TT_TRANSLATE_V,
            TT_SCALE_U,
            TT_SCALE_V,
            TT_ROTATE
        };

        /** Enum identifying the frame indexes for faces of a cube map (not the composite 3D type.
         */
        enum TextureCubeFace
        {
            CUBE_FRONT = 0,
            CUBE_BACK = 1,
            CUBE_LEFT = 2,
            CUBE_RIGHT = 3,
            CUBE_UP = 4,
            CUBE_DOWN = 5
        };

        /** Internal structure defining a texture effect.
         */
        struct TextureEffect
        {
            TextureEffectType type;
            int               subtype;
            Real              arg1, arg2;
            WaveformType      waveType;
            Real              base;
            Real              frequency;
            Real              phase;
            Real              amplitude;
            Controller<Real> *controller;
            const Frustum    *frustum;
        };

        /** Texture effects in a multimap paired array.
         */
        typedef multimap<TextureEffectType, TextureEffect>::type EffectMap;

        /** Default constructor.
         */
        TextureUnitState( Pass *parent );

        TextureUnitState( Pass *parent, const TextureUnitState &oth );

        TextureUnitState &operator=( const TextureUnitState &oth );

        /** Default destructor.
         */
        ~TextureUnitState() override;

        /** Name-based constructor.
        @param texName
            The basic name of the texture e.g. brickwall.jpg, stonefloor.png.
        @param texCoordSet
            The index of the texture coordinate set to use.
        */
        TextureUnitState( Pass *parent, const String &texName, unsigned int texCoordSet = 0 );

        /** Get the name of current texture image for this layer.
        @remarks
            This will either always be a single name for this layer,
            or will be the name of the current frame for an animated
            or otherwise multi-frame texture.
        @note
            Applies to both fixed-function and programmable pipeline.
        */
        const String &getTextureName() const;

        /** Sets this texture layer to use a single texture, given the
            name of the texture to use on this layer.
        @note
            Applies to both fixed-function and programmable pipeline.
        */
        void setTextureName( const String              &name,
                             TextureTypes::TextureTypes ttype = TextureTypes::Type2D );

        /** Sets this texture layer to use a single texture, given the
            pointer to the texture to use on this layer.
        @note
            Applies to both fixed-function and programmable pipeline.
        */
        void setTexture( TextureGpu *texPtr );

        /** Sets this texture layer to use a combination of 6 texture maps, each one relating to a face
        of a cube.
        @remarks
            Cubic textures are made up of 6 separate texture images. Each one of these is an orthogonal
        view of the world with a FOV of 90 degrees and an aspect ratio of 1:1. You can generate these
        from 3D Studio by rendering a scene to a reflection map of a transparent cube and saving the
        output files.
        @par
            Cubic maps can be used either for skyboxes (complete wrap-around skies, like space) or as
        environment maps to simulate reflections. The system deals with these 2 scenarios in different
        ways: <ol> <li> <p> for cubic environment maps, the 6 textures are combined into a single 'cubic'
        texture map which is then addressed using 3D texture coordinates. This is required because you
        don't know what face of the box you're going to need to address when you render an object, and
        typically you need to reflect more than one face on the one object, so all 6 textures are needed
        to be 'active' at once. Cubic environment maps are enabled by calling this method with the forUVW
            parameter set to true, and then calling setEnvironmentMap(true).
            </p>
            <p>
            Note that not all cards support cubic environment mapping.
            </p>
            </li>
            <li>
            <p>
            for skyboxes, the 6 textures are kept separate and used independently for each face of the
        skybox. This is done because not all cards support 3D cubic maps and skyboxes do not need to use
        3D texture coordinates so it is simpler to render each face of the box with 2D coordinates,
        changing texture between faces.
            </p>
            <p>
            Skyboxes are created by calling SceneManager::setSkyBox.
            </p>
            </li>
            </ol>
        @note
            Applies to both fixed-function and programmable pipeline.
        @param name
            The basic name of the texture e.g. brickwall.jpg, stonefloor.png. There must be 6 versions
            of this texture with the suffixes _fr, _bk, _up, _dn, _lf, and _rt (before the extension)
        which make up the 6 sides of the box. The textures must all be the same size and be powers of 2
        in width & height. If you can't make your texture names conform to this, use the alternative
        method of the same name which takes an array of texture names instead.
        @param forUVW
            Set to @c true if you want a single 3D texture addressable with 3D texture coordinates rather
        than 6 separate textures. Useful for cubic environment mapping.
        */
        void setCubicTextureName( const String &name, bool forUVW = false );

        /** Sets this texture layer to use a combination of 6 texture maps, each one relating to a face
        of a cube.
        @remarks
            Cubic textures are made up of 6 separate texture images. Each one of these is an orthogonal
        view of the world with a FOV of 90 degrees and an aspect ratio of 1:1. You can generate these
        from 3D Studio by rendering a scene to a reflection map of a transparent cube and saving the
        output files.
        @par
            Cubic maps can be used either for skyboxes (complete wrap-around skies, like space) or as
        environment maps to simulate reflections. The system deals with these 2 scenarios in different
        ways: <ol> <li> <p> For cubic environment maps, the 6 textures are combined into a single 'cubic'
        texture map which is then addressed using 3D texture coordinates. This is required because you
        don't know what face of the box you're going to need to address when you render an object, and
        typically you need to reflect more than one face on the one object, so all 6 textures are needed
        to be 'active' at once. Cubic environment maps are enabled by calling this method with the forUVW
            parameter set to @c true, and then calling setEnvironmentMap(true).
            </p>
            <p>
            Note that not all cards support cubic environment mapping.
            </p>
            </li>
            <li>
            <p>
            For skyboxes, the 6 textures are kept separate and used independently for each face of the
        skybox. This is done because not all cards support 3D cubic maps and skyboxes do not need to use
        3D texture coordinates so it is simpler to render each face of the box with 2D coordinates,
        changing texture between faces.
            </p>
            <p>
            Skyboxes are created by calling SceneManager::setSkyBox.
            </p>
            </li>
            </ol>
        @note
            Applies to both fixed-function and programmable pipeline.
        @param names
            The 6 names of the textures which make up the 6 sides of the box. The textures must all
            be the same size and be powers of 2 in width & height.
            Must be an Ogre::String array with a length of 6 unless forUVW is set to @c true.
        @param forUVW
            Set to @c true if you want a single 3D texture addressable with 3D texture coordinates rather
        than 6 separate textures. Useful for cubic environment mapping.
        */
        void setCubicTextureName( const String *const names, bool forUVW = false );

        /** Sets this texture layer to use a combination of 6 texture maps, each one relating to a face
        of a cube.
        @remarks
            Cubic textures are made up of 6 separate texture images. Each one of these is an orthogonal
        view of the world with a FOV of 90 degrees and an aspect ratio of 1:1. You can generate these
        from 3D Studio by rendering a scene to a reflection map of a transparent cube and saving the
        output files.
        @par
            Cubic maps can be used either for skyboxes (complete wrap-around skies, like space) or as
        environment maps to simulate reflections. The system deals with these 2 scenarios in different
        ways: <ol> <li> <p> for cubic environment maps, the 6 textures are combined into a single 'cubic'
        texture map which is then addressed using 3D texture coordinates. This is required because you
        don't know what face of the box you're going to need to address when you render an object, and
        typically you need to reflect more than one face on the one object, so all 6 textures are needed
        to be 'active' at once. Cubic environment maps are enabled by calling this method with the forUVW
            parameter set to true, and then calling setEnvironmentMap(true).
            </p>
            <p>
            Note that not all cards support cubic environment mapping.
            </p>
            </li>
            <li>
            <p>
            for skyboxes, the 6 textures are kept separate and used independently for each face of the
        skybox. This is done because not all cards support 3D cubic maps and skyboxes do not need to use
        3D texture coordinates so it is simpler to render each face of the box with 2D coordinates,
        changing texture between faces.
            </p>
            <p>
            Skyboxes are created by calling SceneManager::setSkyBox.
            </p>
            </li>
            </ol>
        @note
            Applies to both fixed-function and programmable pipeline.
        @param texPtrs
            The 6 pointers to the textures which make up the 6 sides of the box. The textures must all
            be the same size and be powers of 2 in width & height.
            Must be an Ogre::TextureGpu array with a length of 6 unless forUVW is set to @c true.
        @param forUVW
            Set to @c true if you want a single 3D texture addressable with 3D texture coordinates rather
        than 6 separate textures. Useful for cubic environment mapping.
        */
        void setCubicTexture( TextureGpu *const *texPtrs, bool forUVW = false );

        /** Sets the names of the texture images for an animated texture.
        @remarks
            Animated textures are just a series of images making up the frames of the animation. All the
        images must be the same size, and their names must have a frame number appended before the
        extension, e.g. if you specify a name of "wall.jpg" with 3 frames, the image names must be
        "wall_0.jpg", "wall_1.jpg" and "wall_2.jpg".
        @par
            You can change the active frame on a texture layer by calling the setCurrentFrame method.
        @note
            If you can't make your texture images conform to the naming standard laid out here, you
            can call the alternative setAnimatedTextureName method which takes an array of names instead.
        @note
            Applies to both fixed-function and programmable pipeline.
        @param name
            The base name of the textures to use e.g. wall.jpg for frames wall_0.jpg, wall_1.jpg etc.
        @param numFrames
            The number of frames in the sequence.
        @param duration
            The length of time it takes to display the whole animation sequence, in seconds.
            If 0, no automatic transition occurs.
        */
        void setAnimatedTextureName( const String &name, unsigned int numFrames, Real duration = 0 );

        /** Sets the names of the texture images for an animated texture.
        @remarks
            This an alternative method to the one where you specify a single name and let the system
        derive the names of each frame, incase your images can't conform to this naming standard.
        @par
            Animated textures are just a series of images making up the frames of the animation. All the
        images must be the same size, and you must provide their names as an array in the first
        parameter. You can change the active frame on a texture layer by calling the setCurrentFrame
        method.
        @note
            If you can make your texture images conform to a naming standard of basicName_frame.ext, you
            can call the alternative setAnimatedTextureName method which just takes a base name instead.
        @note
            Applies to both fixed-function and programmable pipeline.
        @param names
            Pointer to array of names of the textures to use, in frame order.
        @param numFrames
            The number of frames in the sequence.
        @param duration
            The length of time it takes to display the whole animation sequence, in seconds.
            If 0, no automatic transition occurs.
        */
        void setAnimatedTextureName( const String *const names, unsigned int numFrames,
                                     Real duration = 0 );

        /** Returns the width and height of the texture in the given frame.
         */
        std::pair<size_t, size_t> getTextureDimensions( unsigned int frame = 0 ) const;

        /** Changes the active frame in an animated or multi-image texture.
        @remarks
            An animated texture (or a cubic texture where the images are not combined for 3D use) is made
        up of a number of frames. This method sets the active frame.
        @note
            Applies to both fixed-function and programmable pipeline.
        */
        void setCurrentFrame( unsigned int frameNumber );

        /** Gets the active frame in an animated or multi-image texture layer.
        @note
            Applies to both fixed-function and programmable pipeline.
        */
        unsigned int getCurrentFrame() const;

        /** Gets the name of the texture associated with a frame number.
            Throws an exception if frameNumber exceeds the number of stored frames.
        @note
            Applies to both fixed-function and programmable pipeline.
        */
        const String &getFrameTextureName( unsigned int frameNumber ) const;

        /** Sets the name of the texture associated with a frame.
        @param name
            The name of the texture.
        @param frameNumber
            The frame the texture name is to be placed in.
        @note
            Throws an exception if frameNumber exceeds the number of stored frames.
            Applies to both fixed-function and programmable pipeline.
        */
        void setFrameTextureName( const String &name, unsigned int frameNumber );

        /** Add a Texture name to the end of the frame container.
        @param name
            The name of the texture.
        @note
            Applies to both fixed-function and programmable pipeline.
        */
        void addFrameTextureName( const String &name );
        /** Deletes a specific texture frame.  The texture used is not deleted but the
            texture will no longer be used by the Texture Unit.  An exception is raised
            if the frame number exceeds the number of actual frames.
        @param frameNumber
            The frame number of the texture to be deleted.
        @note
            Applies to both fixed-function and programmable pipeline.
        */
        void deleteFrameTextureName( const size_t frameNumber );
        /** Gets the number of frames for a texture.
        @note
            Applies to both fixed-function and programmable pipeline.
        */
        unsigned int getNumFrames() const;

        /** The type of unit to bind the texture settings to. */
        enum BindingType
        {
            /** Regular fragment processing unit - the default. */
            BT_FRAGMENT = 0,
            /** Vertex processing unit - indicates this unit will be used for
                a vertex texture fetch.
            */
            BT_VERTEX = 1,
            /// Geometry processing unit
            BT_GEOMETRY = 2,
            /// Tessellation control processing unit
            BT_TESSELLATION_HULL = 3,
            /// Tessellation evaluation processing unit
            BT_TESSELLATION_DOMAIN = 4
        };
        /** Enum identifying the type of content this texture unit contains.
         */
        enum ContentType
        {
            /// Normal texture identified by name
            CONTENT_NAMED = 0,
            /// A shadow texture, automatically bound by engine
            CONTENT_SHADOW = 1,
            /// A compositor texture, automatically linked to active viewport's chain
            CONTENT_COMPOSITOR = 2
        };

        /** Sets the type of unit these texture settings should be bound to.
        @remarks
            Some render systems, when implementing vertex texture fetch, separate
            the binding of textures for use in the vertex program versus those
            used in fragment programs. This setting allows you to target the
            vertex processing unit with a texture binding, in those cases. For
            rendersystems which have a unified binding for the vertex and fragment
            units, this setting makes no difference.
        */
        void setBindingType( BindingType bt );

        /** Gets the type of unit these texture settings should be bound to.
         */
        BindingType getBindingType() const;

        /** When a texture is used by both PBS datablocks and V1 materials it can lead to a few issues:
            1. V1 materials will often expect a Type2D texture to actually be Type2D.
            2. PBS datablocks will use TextureFlags::AutomaticBatching to group several textures of same
               resolution and format into the same Type2DArray, and thus lower CPU overhead. This
               means the 2D texture is not actually Type2D, but rather Type2DArray, and the slice
               information is also required.
        @par
            The default is to disable this option, which means the texture may be loaded *twice*: one
            for V1 materials, and another for PBS. If you do not want this, call
            setAutomaticBatching( true ); before loading the texture.
            Note that you will have to provide the slice index by hand and make sure the shader
            you wrote understands this is a 2D Array texture.
        @param automaticBatching
        */
        void setAutomaticBatching( bool automaticBatching );
        bool getAutomaticBatching() const;

        /** Set the type of content this TextureUnitState references.
        @remarks
            The default is to reference a standard named texture, but this unit
            can also reference automated content like a shadow texture.
        */
        void setContentType( ContentType ct );
        /** Get the type of content this TextureUnitState references. */
        ContentType getContentType() const;

        /** Returns true if this texture unit is either a series of 6 2D textures, each
            in it's own frame, or is a full 3D cube map. You can tell which by checking
            getTextureType.
        @note
            Applies to both fixed-function and programmable pipeline.
        */
        bool isCubic() const;

        /** Returns true if this texture layer uses a composite 3D cubic texture.
        @note
            Applies to both fixed-function and programmable pipeline.
        */
        bool is3D() const;

        /** Returns the type of this texture.
        @note
            Applies to both fixed-function and programmable pipeline.
        */
        TextureTypes::TextureTypes getTextureType() const;

        /** Sets how many mipmaps have been requested for the texture.
         */
        void setNumMipmaps( int numMipmaps );

        /** Gets how many mipmaps have been requested for the texture.
         */
        int getNumMipmaps() const;

        /** Sets whether this texture is requested to be loaded as alpha if single channel
         */
        void setIsAlpha( bool isAlpha );

        /** Gets whether this texture is requested to be loaded as alpha if single channel
         */
        bool getIsAlpha() const;

        /// @copydoc Texture::getGamma
        Real getGamma() const { return mGamma; }
        /// @copydoc Texture::setGamma
        void setGamma( Real gamma ) { mGamma = gamma; }

        /// @copydoc Texture::setHardwareGammaEnabled
        void setHardwareGammaEnabled( bool enabled );
        /// @copydoc Texture::isHardwareGammaEnabled
        bool isHardwareGammaEnabled() const;

        /** Gets the index of the set of texture co-ords this layer uses.
        @note
        Only applies to the fixed function pipeline and has no effect if a fragment program is used.
        */
        unsigned int getTextureCoordSet() const;

        /** Sets the index of the set of texture co-ords this layer uses.
        @note
            Default is 0 for all layers. Only change this if you have provided multiple texture co-ords
        per vertex.
        @note
        Only applies to the fixed function pipeline and has no effect if a fragment program is used.
        */
        void setTextureCoordSet( unsigned int set );

        /** Sets a matrix used to transform any texture coordinates on this layer.
        @remarks
            Texture coordinates can be modified on a texture layer to create effects like scrolling
            textures. A texture transform can either be applied to a layer which takes the source
        coordinates from a fixed set in the geometry, or to one which generates them dynamically (e.g.
        environment mapping).
        @par
            It's obviously a bit impractical to create scrolling effects by calling this method manually
        since you would have to call it every framw with a slight alteration each time, which is tedious.
        Instead you can use the ControllerManager class to create a Controller object which will manage
        the effect over time for you. See the ControllerManager::createTextureScroller and it's sibling
        methods for details.<BR> In addition, if you want to set the individual texture transformations
        rather than concatenating them yourself, use setTextureScroll, setTextureScale and
        setTextureRotate.
        @note
            Has no effect in the programmable pipeline.
        */
        void setTextureTransform( const Matrix4 &xform );

        /** Gets the current texture transformation matrix.
        @remarks
            Causes a reclaculation of the matrix if any parameters have been changed via
            setTextureScroll, setTextureScale and setTextureRotate.
        @note
            Has no effect in the programmable pipeline.
        */
        const Matrix4 &getTextureTransform() const;

        /** Sets the translation offset of the texture, ie scrolls the texture.
        @remarks
            This method sets the translation element of the texture transformation, and is easier to use
        than setTextureTransform if you are combining translation, scaling and rotation in your texture
        transformation. Again if you want to animate these values you need to use a Controller
        @note
            Has no effect in the programmable pipeline.
        @param u
            The amount the texture should be moved horizontally (u direction).
        @param v
            The amount the texture should be moved vertically (v direction).
        @see
            ControllerManager, Controller
        */
        void setTextureScroll( Real u, Real v );

        /** As setTextureScroll, but sets only U value.
        @note
            Has no effect in the programmable pipeline.
        */
        void setTextureUScroll( Real value );
        /// Get texture uscroll value.
        Real getTextureUScroll() const;

        /** As setTextureScroll, but sets only V value.
        @note
            Has no effect in the programmable pipeline.
        */
        void setTextureVScroll( Real value );
        /// Get texture vscroll value.
        Real getTextureVScroll() const;

        /** As setTextureScale, but sets only U value.
        @note
            Has no effect in the programmable pipeline.
        */
        void setTextureUScale( Real value );
        /// Get texture uscale value.
        Real getTextureUScale() const;

        /** As setTextureScale, but sets only V value.
        @note
            Has no effect in the programmable pipeline.
        */
        void setTextureVScale( Real value );
        /// Get texture vscale value.
        Real getTextureVScale() const;

        /** Sets the scaling factor applied to texture coordinates.
        @remarks
            This method sets the scale element of the texture transformation, and is easier to use than
            setTextureTransform if you are combining translation, scaling and rotation in your texture
        transformation. Again if you want to animate these values you need to use a Controller (see
        ControllerManager and it's methods for more information).
        @note
            Has no effect in the programmable pipeline.
        @param uScale
            The value by which the texture is to be scaled horizontally.
        @param vScale
            The value by which the texture is to be scaled vertically.
        */
        void setTextureScale( Real uScale, Real vScale );

        /** Sets the anticlockwise rotation factor applied to texture coordinates.
        @remarks
            This sets a fixed rotation angle - if you wish to animate this, see the
            ControllerManager::createTextureRotater method.
        @note
            Has no effect in the programmable pipeline.
        @param angle
            The angle of rotation (anticlockwise).
        */
        void setTextureRotate( const Radian &angle );
        /// Get texture rotation effects angle value.
        const Radian &getTextureRotate() const;

        /** Setting advanced blending options.
        @remarks
            This is an extended version of the TextureUnitState::setColourOperation method which allows
            extremely detailed control over the blending applied between this and earlier layers.
            See the IMPORTANT note below about the issues between mulitpass and multitexturing that
            using this method can create.
        @par
            Texture colour operations determine how the final colour of the surface appears when
            rendered. Texture units are used to combine colour values from various sources (ie. the
            diffuse colour of the surface from lighting calculations, combined with the colour of
            the texture). This method allows you to specify the 'operation' to be used, ie. the
            calculation such as adds or multiplies, and which values to use as arguments, such as
            a fixed value or a value from a previous calculation.
        @par
            The defaults for each layer are:
            <ul>
            <li>op = LBX_MODULATE</li>
            <li>source1 = LBS_TEXTURE</li>
            <li>source2 = LBS_CURRENT</li>
            </ul>
            ie. each layer takes the colour results of the previous layer, and multiplies them
            with the new texture being applied. Bear in mind that colours are RGB values from
            0.0 - 1.0 so multiplying them together will result in values in the same range,
            'tinted' by the multiply. Note however that a straight multiply normally has the
            effect of darkening the textures - for this reason there are brightening operations
            like LBO_MODULATE_X2. See the LayerBlendOperation and LayerBlendSource enumerated
            types for full details.
        @note
            Because of the limitations on some underlying APIs (Direct3D included)
            the LBS_TEXTURE argument can only be used as the first argument, not the second.
        @par
            The final 3 parameters are only required if you decide to pass values manually
            into the operation, i.e. you want one or more of the inputs to the colour calculation
            to come from a fixed value that you supply. Hence you only need to fill these in if
            you supply LBS_MANUAL to the corresponding source, or use the LBX_BLEND_MANUAL
            operation.
        @warning
            Ogre tries to use multitexturing hardware to blend texture layers
            together. However, if it runs out of texturing units (e.g. 2 of a GeForce2, 4 on a
            GeForce3) it has to fall back on multipass rendering, i.e. rendering the same object
            multiple times with different textures. This is both less efficient and there is a smaller
            range of blending operations which can be performed. For this reason, if you use this method
            you MUST also call TextureUnitState::setColourOpMultipassFallback to specify which effect you
            want to fall back on if sufficient hardware is not available.
        @note
            This has no effect in the programmable pipeline.
        @par
            If you wish to avoid having to do this, use the simpler TextureUnitState::setColourOperation
        method which allows less flexible blending options but sets up the multipass fallback
        automatically, since it only allows operations which have direct multipass equivalents.
        @param op
            The operation to be used, e.g. modulate (multiply), add, subtract.
        @param source1
            The source of the first colour to the operation e.g. texture colour.
        @param source2
            The source of the second colour to the operation e.g. current surface colour.
        @param arg1
            Manually supplied colour value (only required if source1 = LBS_MANUAL).
        @param arg2
            Manually supplied colour value (only required if source2 = LBS_MANUAL).
        @param manualBlend
            Manually supplied 'blend' value - only required for operations
            which require manual blend e.g. LBX_BLEND_MANUAL.
        */
        void setColourOperationEx( LayerBlendOperationEx op, LayerBlendSource source1 = LBS_TEXTURE,
                                   LayerBlendSource source2 = LBS_CURRENT,

                                   const ColourValue &arg1 = ColourValue::White,
                                   const ColourValue &arg2 = ColourValue::White,

                                   Real manualBlend = 0.0 );

        /** Determines how this texture layer is combined with the one below it (or the diffuse colour of
            the geometry if this is layer 0).
        @remarks
            This method is the simplest way to blend tetxure layers, because it requires only one
        parameter, gives you the most common blending types, and automatically sets up 2 blending
        methods: one for if single-pass multitexturing hardware is available, and another for if it is
        not and the blending must be achieved through multiple rendering passes. It is, however, quite
        limited and does not expose the more flexible multitexturing operations, simply because these
        can't be automatically supported in multipass fallback mode. If want to use the fancier options,
        use TextureUnitState::setColourOperationEx, but you'll either have to be sure that enough
        multitexturing units will be available, or you should explicitly set a fallback using
        TextureUnitState::setColourOpMultipassFallback.
        @note
            The default method is LBO_MODULATE for all layers.
        @note
            This option has no effect in the programmable pipeline.
        @param op
            One of the LayerBlendOperation enumerated blending types.
        */
        void setColourOperation( const LayerBlendOperation op );

        /** Sets the multipass fallback operation for this layer, if you used
        TextureUnitState::setColourOperationEx and not enough multitexturing hardware is available.
        @remarks
            Because some effects exposed using TextureUnitState::setColourOperationEx are only supported
        under multitexturing hardware, if the hardware is lacking the system must fallback on multipass
        rendering, which unfortunately doesn't support as many effects. This method is for you to specify
        the fallback operation which most suits you.
        @par
            You'll notice that the interface is the same as the Material::setSceneBlending method; this
        is because multipass rendering IS effectively scene blending, since each layer is rendered on top
            of the last using the same mechanism as making an object transparent, it's just being
        rendered in the same place repeatedly to get the multitexture effect.
        @par
            If you use the simpler (and hence less flexible) TextureUnitState::setColourOperation method
        you don't need to call this as the system sets up the fallback for you.
        @note
            This option has no effect in the programmable pipeline, because there is no multipass
        fallback and multitexture blending is handled by the fragment shader.
        */
        void setColourOpMultipassFallback( const SceneBlendFactor sourceFactor,
                                           const SceneBlendFactor destFactor );

        /** Get multitexturing colour blending mode.
         */
        const LayerBlendModeEx &getColourBlendMode() const;

        /** Get multitexturing alpha blending mode.
         */
        const LayerBlendModeEx &getAlphaBlendMode() const;

        /** Get the multipass fallback for colour blending operation source factor.
         */
        SceneBlendFactor getColourBlendFallbackSrc() const;

        /** Get the multipass fallback for colour blending operation destination factor.
         */
        SceneBlendFactor getColourBlendFallbackDest() const;

        /** Sets the alpha operation to be applied to this texture.
        @remarks
            This works in exactly the same way as setColourOperation, except
            that the effect is applied to the level of alpha (i.e. transparency)
            of the texture rather than its colour. When the alpha of a texel (a pixel
            on a texture) is 1.0, it is opaque, whereas it is fully transparent if the
            alpha is 0.0. Please refer to the setColourOperation method for more info.
        @param op
            The operation to be used, e.g. modulate (multiply), add, subtract
        @param source1
            The source of the first alpha value to the operation e.g. texture alpha
        @param source2
            The source of the second alpha value to the operation e.g. current surface alpha
        @param arg1
            Manually supplied alpha value (only required if source1 = LBS_MANUAL)
        @param arg2
            Manually supplied alpha value (only required if source2 = LBS_MANUAL)
        @param manualBlend
            Manually supplied 'blend' value - only required for operations
            which require manual blend e.g. LBX_BLEND_MANUAL
        @see
            setColourOperation
        @note
            This option has no effect in the programmable pipeline.
        */
        void setAlphaOperation( LayerBlendOperationEx op, LayerBlendSource source1 = LBS_TEXTURE,
                                LayerBlendSource source2 = LBS_CURRENT, Real arg1 = 1.0, Real arg2 = 1.0,
                                Real manualBlend = 0.0 );

        /** Generic method for setting up texture effects.
        @remarks
            Allows you to specify effects directly by using the TextureEffectType enumeration. The
            arguments that go with it depend on the effect type. Only one effect of
            each type can be applied to a texture layer.
        @par
            This method is used internally by Ogre but it is better generally for applications to use the
            more intuitive specialised methods such as setEnvironmentMap and setScroll.
        @note
            This option has no effect in the programmable pipeline.
        */
        void addEffect( TextureEffect &effect );

        /** Turns on/off texture coordinate effect that makes this layer an environment map.
        @remarks
            Environment maps make an object look reflective by using the object's vertex normals relative
            to the camera view to generate texture coordinates.
        @par
            The vectors generated can either be used to address a single 2D texture which
            is a 'fish-eye' lens view of a scene, or a 3D cubic environment map which requires 6 textures
            for each side of the inside of a cube. The type depends on what texture you set up - if you
        use the setTextureName method then a 2D fisheye lens texture is required, whereas if you used
        setCubicTextureName then a cubic environment map will be used.
        @par
            This effect works best if the object has lots of gradually changing normals. The texture also
            has to be designed for this effect - see the example spheremap.png included with the sample
            application for a 2D environment map; a cubic map can be generated by rendering 6 views of a
            scene to each of the cube faces with orthogonal views.
        @note
            Enabling this disables any other texture coordinate generation effects.
            However it can be combined with texture coordinate modification functions, which then operate
        on the generated coordinates rather than static model texture coordinates.
        @param enable
            True to enable, false to disable
        @param envMapType
            The type of environment mapping to perform. Planar, curved, reflection or normal. @see
        EnvMapType
        @note
            This option has no effect in the programmable pipeline.
        */
        void setEnvironmentMap( bool enable, EnvMapType envMapType = ENV_CURVED );

        /** Sets up an animated scroll for the texture layer.
        @note
            Useful for creating constant scrolling effects on a texture layer (for varying scrolls, see
        setTransformAnimation).
        @param uSpeed
            The number of horizontal loops per second (+ve=moving right, -ve = moving left).
        @param vSpeed
            The number of vertical loops per second (+ve=moving up, -ve= moving down).
        @note
            This option has no effect in the programmable pipeline.
        */
        void setScrollAnimation( Real uSpeed, Real vSpeed );

        /** Sets up an animated texture rotation for this layer.
        @note
            Useful for constant rotations (for varying rotations, see setTransformAnimation).
        @param speed
            The number of complete anticlockwise revolutions per second (use -ve for clockwise)
        @note
            This option has no effect in the programmable pipeline.
        */
        void setRotateAnimation( Real speed );

        /** Sets up a general time-relative texture modification effect.
        @note
            This can be called multiple times for different values of ttype, but only the latest effect
            applies if called multiple time for the same ttype.
        @param ttype
            The type of transform, either translate (scroll), scale (stretch) or rotate (spin).
        @param waveType
            The shape of the wave, see WaveformType enum for details.
        @param base
            The base value for the function (range of output = {base, base + amplitude}).
        @param frequency
            The speed of the wave in cycles per second.
        @param phase
            The offset of the start of the wave, e.g. 0.5 to start half-way through the wave.
        @param amplitude
            Scales the output so that instead of lying within 0..1 it lies within 0..1*amplitude for
        exaggerated effects.
        @note
            This option has no effect in the programmable pipeline.
        */
        void setTransformAnimation( const TextureTransformType ttype, const WaveformType waveType,
                                    Real base = 0, Real frequency = 1, Real phase = 0,
                                    Real amplitude = 1 );

        /** Enables or disables projective texturing on this texture unit.
        @remarks
            Projective texturing allows you to generate texture coordinates
            based on a Frustum, which gives the impression that a texture is
            being projected onto the surface. Note that once you have called
            this method, the texture unit continues to monitor the Frustum you
            passed in and the projection will change if you can alter it. It also
            means that you must ensure that the Frustum object you pass a pointer
            to remains in existence for as long as this TextureUnitState does.
        @par
            This effect cannot be combined with other texture generation effects,
            such as environment mapping. It also has no effect on passes which
            have a vertex program enabled - projective texturing has to be done
            in the vertex program instead.
        @param enabled
            Whether to enable / disable.
        @param projectionSettings
            The Frustum which will be used to derive the
            projection parameters.
        */
        void setProjectiveTexturing( bool enabled, const Frustum *projectionSettings = 0 );

        /** Removes all effects applied to this texture layer.
         */
        void removeAllEffects();

        /** Removes a single effect applied to this texture layer.
        @note
            Because you can only have 1 effect of each type (e.g. 1 texture coordinate generation)
        applied to a layer, only the effect type is required.
        */
        void removeEffect( const TextureEffectType type );

        /** Determines if this texture layer is currently blank.
        @note
            This can happen if a texture fails to load or some other non-fatal error. Worth checking
        after setting texture name.
        */
        bool isBlank() const;

        /** Sets this texture layer to be blank.
         */
        void setBlank();

        /** Tests if the texture associated with this unit has failed to load.
         */
        bool isTextureLoadFailing() const { return mTextureLoadFailed; }

        /** Tells the unit to retry loading the texture if it had failed to load.
         */
        void retryTextureLoad() { mTextureLoadFailed = false; }

        /// Changes the current samplerblock for a new one.
        void setSamplerblock( const HlmsSamplerblock &samplerblock );

        /// Changes the current samplerblock for a new one.
        /// MUST'VE BEEN CREATED VIA HlmsManager!!!
        /// INCREASES THE REFERENCE COUNT OF THE SAMPLERBLOCK
        void _setSamplerblock( const HlmsSamplerblock *samplerblock );

        /** Retrieves current samplerblock. Don't const_cast the return value to modify it.
            @See HlmsDatablock remarks.
        */
        const HlmsSamplerblock *getSamplerblock() const;

        /// Get texture effects in a multimap paired array.
        const EffectMap &getEffects() const;
        /// Get the animated-texture animation duration.
        Real getAnimationDuration() const;

        /** Set the compositor reference for this texture unit state.
        @remarks
            Only valid when content type is compositor.
        @param textureName
            The name of the texture to reference.
        */
        void setCompositorReference( const String &textureName );

        /** Gets the name of the texture in the compositor that this texture references. */
        IdString getReferencedTextureName() const { return mCompositorRefTexName; }

        /// Gets the parent Pass object.
        Pass *getParent() const { return mParent; }

        /** Internal method for preparing this object for load, as part of Material::prepare. */
        void _prepare();
        /** Internal method for undoing the preparation this object as part of Material::unprepare. */
        void _unprepare();
        /** Internal method for loading this object as part of Material::load. */
        void _load();
        /** Internal method for unloading this object as part of Material::unload. */
        void _unload();
        /// Returns whether this unit has texture coordinate generation that depends on the camera.
        bool hasViewRelativeTextureCoordinateGeneration() const;

        /// Is this loaded?
        bool isLoaded() const;

        /** Set the name of the Texture Unit State.
        @remarks
            The name of the Texture Unit State is optional.  Its useful in material scripts where a
        material could inherit from another material and only want to modify a particalar Texture Unit
        State.
        */
        void setName( const String &name );
        /// Get the name of the Texture Unit State.
        const String &getName() const { return mName; }

        /** Set the alias name used for texture frame names.
        @param name
            Can be any sequence of characters and does not have to be unique.
        */
        void setTextureNameAlias( const String &name );
        /** Gets the Texture Name Alias of the Texture Unit.
         */
        const String &getTextureNameAlias() const { return mTextureNameAlias; }

        /** Applies texture names to Texture Unit State with matching texture name aliases.
            If no matching aliases are found then the TUS state does not change.
        @remarks
            Cubic, 1d, 2d, and 3d textures are determined from current state of the Texture Unit.
            Assumes animated frames are sequentially numbered in the name.
            If matching texture aliases are found then true is returned.

        @param aliasList
            A map container of texture alias, texture name pairs.
        @param apply
            Set @c true to apply the texture aliases else just test to see if texture alias matches are
        found.
        @return
            True if matching texture aliases were found in the Texture Unit State.
        */
        bool applyTextureAliases( const AliasTextureNamePairList &aliasList, const bool apply = true );

        /** Notify this object that its parent has changed. */
        void _notifyParent( Pass *parent );

        /** Get the texture pointer for the current frame. */
        TextureGpu *_getTexturePtr() const;
        /** Get the texture pointer for a given frame. */
        TextureGpu *_getTexturePtr( size_t frame ) const;

        /** Set the texture pointer for the current frame (internal use only!). */
        void _setTexturePtr( TextureGpu *texptr );
        /** Set the texture pointer for a given frame (internal use only!). */
        void _setTexturePtr( TextureGpu *texptr, size_t frame );

        size_t calculateSize() const;

        /** Gets the animation controller (as created because of setAnimatedTexture)
            if it exists.
        */
        Controller<Real> *_getAnimController() const { return mAnimController; }

    protected:
        // State
        /// The current animation frame.
        unsigned int mCurrentFrame;

        /// Duration of animation in seconds.
        Real mAnimDuration;
        bool mCubic;  /// Is this a series of 6 2D textures to make up a cube?

        bool                       mAutomaticBatching;
        TextureTypes::TextureTypes mTextureType;
        int                        mTextureSrcMipmaps;  /// Request number of mipmaps.

        unsigned int            mTextureCoordSetIndex;
        HlmsSamplerblock const *mSamplerblock;

        LayerBlendModeEx mColourBlendMode;
        SceneBlendFactor mColourBlendFallbackSrc;
        SceneBlendFactor mColourBlendFallbackDest;

        LayerBlendModeEx mAlphaBlendMode;
        mutable bool     mTextureLoadFailed;
        bool             mIsAlpha;
        bool             mHwGamma;
        Real             mGamma;

        mutable bool    mRecalcTexMatrix;
        Real            mUMod, mVMod;
        Real            mUScale, mVScale;
        Radian          mRotate;
        mutable Matrix4 mTexModMatrix;

        /// Binding type (fragment, vertex, tessellation hull and domain pipeline).
        BindingType mBindingType;
        /// Content type of texture (normal loaded texture, auto-texture).
        ContentType mContentType;

        //-----------------------------------------------------------------------------
        // Complex members (those that can't be copied using memcpy) are at the end to
        // allow for fast copying of the basic members.
        //
        vector<String>::type               mFrames;
        mutable vector<TextureGpu *>::type mFramePtrs;
        String                             mName;              ///< Optional name for the TUS.
        String                             mTextureNameAlias;  ///< Optional alias for texture frames.
        EffectMap                          mEffects;
        /// The data that references the compositor.
        IdString mCompositorRefTexName;
        //-----------------------------------------------------------------------------

        //-----------------------------------------------------------------------------
        // Pointer members (those that can't be copied using memcpy), and MUST
        // preserving even if assign from others
        //
        Pass             *mParent;
        Controller<Real> *mAnimController;
        //-----------------------------------------------------------------------------

        /** Internal method for calculating texture matrix.
         */
        void recalcTextureMatrix() const;

        /** Internal method for creating animation controller.
         */
        void createAnimController();

        /** Internal method for creating texture effect controller.
         */
        void createEffectController( TextureEffect &effect );

        /** Internal method for ensuring the texture for a given frame is prepared. */
        void ensurePrepared( size_t frame ) const;
        /** Internal method for ensuring the texture for a given frame is loaded. */
        void ensureLoaded( size_t frame ) const;

        void cleanFramePtrs();

        /// TextureGpuListener overload
        void notifyTextureChanged( TextureGpu *texture, TextureGpuListener::Reason reason,
                                   void *extraData ) override;
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif  // __TextureUnitState_H__
