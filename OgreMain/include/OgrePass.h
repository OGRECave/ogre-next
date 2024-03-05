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
#ifndef __Pass_H__
#define __Pass_H__

#include "OgrePrerequisites.h"

#include "OgreColourValue.h"
#include "OgreCommon.h"
#include "OgreLight.h"
#include "OgreTextureUnitState.h"
#include "OgreUserObjectBindings.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Materials
     *  @{
     */

    /** Class defining a single pass of a Technique (of a Material), i.e.
        a single rendering call.
        @remarks
        Rendering can be repeated with many passes for more complex effects.

        Note: Using multiple passes hasn't been tested in Ogre-Next 2.x at all
        and may no longer work. It definitely needs at least RenderQueue::V1_LEGACY
        @par
        Programmable passes are complex to define, because they require custom
        programs and you have to set all constant inputs to the programs (like
        the position of lights, any base material colours you wish to use etc), but
        they do give you much total flexibility over the algorithms used to render your
        pass, and you can create some effects which are impossible with a fixed-function pass.
    */
    class _OgreExport Pass : public OgreAllocatedObj
    {
    protected:
        /// Increments on the constructor, in order to create a unique datablock for each material pass
        static AtomicScalar<uint32> gId;

        uint32         mId;
        Technique     *mParent;
        unsigned short mIndex;  ///< Pass index
        String         mName;   ///< Optional name for the pass
        //-------------------------------------------------------------------------
        // Colour properties, only applicable if shader uses them
        ColourValue           mAmbient;
        ColourValue           mDiffuse;
        ColourValue           mSpecular;
        ColourValue           mEmissive;
        Real                  mShininess;
        TrackVertexColourType mTracking;
        //-------------------------------------------------------------------------
        HlmsLowLevelDatablock *mDatablock;

        // Alpha reject settings
        CompareFunction mAlphaRejectFunc;
        unsigned char   mAlphaRejectVal;

        //-------------------------------------------------------------------------

        /// Max simultaneous lights
        unsigned short mMaxSimultaneousLights;
        /// Starting light index
        unsigned short mStartLight;
        /// Run this pass once per light?
        bool mIteratePerLight;
        /// Iterate per how many lights?
        unsigned short mLightsPerIteration;
        /// Should it only be run for a certain light type?
        bool              mRunOnlyForOneLightType;
        Light::LightTypes mOnlyLightType;
        /// With a specific light mask?
        uint32 mLightMask;

        /// Shading options
        ShadeOptions mShadeOptions;
        /// Normalisation
        bool mPolygonModeOverrideable;
        //-------------------------------------------------------------------------
        // Fog
        bool        mFogOverride;
        FogMode     mFogMode;
        ColourValue mFogColour;
        Real        mFogStart;
        Real        mFogEnd;
        Real        mFogDensity;
        //-------------------------------------------------------------------------

        /// Storage of texture unit states
        typedef vector<TextureUnitState *>::type TextureUnitStates;
        TextureUnitStates                        mTextureUnitStates;

        /// Vertex program details
        GpuProgramUsage *mVertexProgramUsage;
        /// Fragment program details
        GpuProgramUsage *mFragmentProgramUsage;
        /// Geometry program details
        GpuProgramUsage *mGeometryProgramUsage;
        /// Tessellation hull program details
        GpuProgramUsage *mTessellationHullProgramUsage;
        /// Tessellation domain program details
        GpuProgramUsage *mTessellationDomainProgramUsage;
        /// Compute program details
        GpuProgramUsage *mComputeProgramUsage;
        /// Number of pass iterations to perform
        size_t mPassIterationCount;
        /// Point size, applies when not using per-vertex point size
        Real mPointSize;
        Real mPointMinSize;
        Real mPointMaxSize;
        bool mPointSpritesEnabled;
        bool mPointAttenuationEnabled;
        /// Constant, linear, quadratic coeffs
        Real mPointAttenuationCoeffs[3];
        /// Scissoring for the light?
        bool mLightScissoring;
        /// User clip planes for light?
        bool mLightClipPlanes;
        /// User objects binding.
        UserObjectBindings mUserObjectBindings;

    public:
        /// Used to get scene blending flags from a blending type
        static void _getBlendFlags( SceneBlendType type, SceneBlendFactor &source,
                                    SceneBlendFactor &dest );

    public:
        OGRE_STATIC_MUTEX( msDirtyHashListMutex );
        OGRE_STATIC_MUTEX( msPassGraveyardMutex );
        OGRE_MUTEX( mTexUnitChangeMutex );
        OGRE_MUTEX( mGpuProgramChangeMutex );
        /// Default constructor
        Pass( Technique *parent, unsigned short index );
        /// Copy constructor
        Pass( Technique *parent, unsigned short index, const Pass &oth );
        /// Operator = overload
        Pass &operator=( const Pass &oth );
        virtual ~Pass();

        uint32 getId() const { return mId; }

        /// Returns true if this pass is programmable i.e. includes either a vertex or fragment program.
        bool isProgrammable() const
        {
            return mVertexProgramUsage || mFragmentProgramUsage || mGeometryProgramUsage ||
                   mTessellationHullProgramUsage || mTessellationDomainProgramUsage ||
                   mComputeProgramUsage;
        }

        /// Returns true if this pass uses a programmable vertex pipeline
        bool hasVertexProgram() const { return mVertexProgramUsage != NULL; }
        /// Returns true if this pass uses a programmable fragment pipeline
        bool hasFragmentProgram() const { return mFragmentProgramUsage != NULL; }
        /// Returns true if this pass uses a programmable geometry pipeline
        bool hasGeometryProgram() const { return mGeometryProgramUsage != NULL; }
        /// Returns true if this pass uses a programmable tessellation control pipeline
        bool hasTessellationHullProgram() const { return mTessellationHullProgramUsage != NULL; }
        /// Returns true if this pass uses a programmable tessellation control pipeline
        bool hasTessellationDomainProgram() const { return mTessellationDomainProgramUsage != NULL; }
        /// Returns true if this pass uses a programmable compute pipeline
        bool hasComputeProgram() const { return mComputeProgramUsage != NULL; }

        size_t calculateSize() const;

        /// Gets the index of this Pass in the parent Technique
        unsigned short getIndex() const { return mIndex; }
        /* Set the name of the pass
           @remarks
           The name of the pass is optional.  Its useful in material scripts where a material could
           inherit from another material and only want to modify a particular pass.
        */
        void setName( const String &name );
        /// Get the name of the pass
        const String &getName() const { return mName; }

        /** Sets the ambient colour reflectance properties of this pass.
            @remarks
            The base colour of a pass is determined by how much red, green and blue light is reflects
            (provided texture layer #0 has a blend mode other than LBO_REPLACE). This property determines
           how much ambient light (directionless global light) is reflected. The default is full white,
           meaning objects are completely globally illuminated. Reduce this if you want to see diffuse or
           specular light effects, or change the blend of colours to make the object have a base colour
           other than white.
            @note
            This setting has no effect if dynamic lighting is disabled (see Pass::setLightingEnabled),
            or if this is a programmable pass.
        */
        void setAmbient( Real red, Real green, Real blue );

        /** Sets the ambient colour reflectance properties of this pass.
            @remarks
            The base colour of a pass is determined by how much red, green and blue light is reflects
            (provided texture layer #0 has a blend mode other than LBO_REPLACE). This property determines
           how much ambient light (directionless global light) is reflected. The default is full white,
           meaning objects are completely globally illuminated. Reduce this if you want to see diffuse or
           specular light effects, or change the blend of colours to make the object have a base colour
           other than white.
            @note
            This setting has no effect if dynamic lighting is disabled (see Pass::setLightingEnabled),
            or if this is a programmable pass.
        */

        void setAmbient( const ColourValue &ambient );

        /** Sets the diffuse colour reflectance properties of this pass.
            @remarks
            The base colour of a pass is determined by how much red, green and blue light is reflects
            (provided texture layer #0 has a blend mode other than LBO_REPLACE). This property determines
           how much diffuse light (light from instances of the Light class in the scene) is reflected.
           The default is full white, meaning objects reflect the maximum white light they can from Light
           objects.
            @note
            This setting has no effect if dynamic lighting is disabled (see Pass::setLightingEnabled),
            or if this is a programmable pass.
        */
        void setDiffuse( Real red, Real green, Real blue, Real alpha );

        /** Sets the diffuse colour reflectance properties of this pass.
            @remarks
            The base colour of a pass is determined by how much red, green and blue light is reflects
            (provided texture layer #0 has a blend mode other than LBO_REPLACE). This property determines
           how much diffuse light (light from instances of the Light class in the scene) is reflected.
           The default is full white, meaning objects reflect the maximum white light they can from Light
           objects.
            @note
            This setting has no effect if dynamic lighting is disabled (see Pass::setLightingEnabled),
            or if this is a programmable pass.
        */
        void setDiffuse( const ColourValue &diffuse );

        /** Sets the specular colour reflectance properties of this pass.
            @remarks
            The base colour of a pass is determined by how much red, green and blue light is reflects
            (provided texture layer #0 has a blend mode other than LBO_REPLACE). This property determines
           how much specular light (highlights from instances of the Light class in the scene) is
           reflected. The default is to reflect no specular light.
            @note
            The size of the specular highlights is determined by the separate 'shininess' property.
            @note
            This setting has no effect if dynamic lighting is disabled (see Pass::setLightingEnabled),
            or if this is a programmable pass.
        */
        void setSpecular( Real red, Real green, Real blue, Real alpha );

        /** Sets the specular colour reflectance properties of this pass.
            @remarks
            The base colour of a pass is determined by how much red, green and blue light is reflects
            (provided texture layer #0 has a blend mode other than LBO_REPLACE). This property determines
           how much specular light (highlights from instances of the Light class in the scene) is
           reflected. The default is to reflect no specular light.
            @note
            The size of the specular highlights is determined by the separate 'shininess' property.
            @note
            This setting has no effect if dynamic lighting is disabled (see Pass::setLightingEnabled),
            or if this is a programmable pass.
        */
        void setSpecular( const ColourValue &specular );

        /** Sets the shininess of the pass, affecting the size of specular highlights.
            @note
            This setting has no effect if dynamic lighting is disabled (see Pass::setLightingEnabled),
            or if this is a programmable pass.
        */
        void setShininess( Real val );

        /** Sets the amount of self-illumination an object has.
            @remarks
            If an object is self-illuminating, it does not need external sources to light it, ambient or
            otherwise. It's like the object has it's own personal ambient light. This property is rarely
           useful since you can already specify per-pass ambient light, but is here for completeness.
            @note
            This setting has no effect if dynamic lighting is disabled (see Pass::setLightingEnabled),
            or if this is a programmable pass.
        */
        void setSelfIllumination( Real red, Real green, Real blue );

        /** Sets the amount of self-illumination an object has.
            @see
            setSelfIllumination
        */
        void setEmissive( Real red, Real green, Real blue ) { setSelfIllumination( red, green, blue ); }

        /** Sets the amount of self-illumination an object has.
            @remarks
            If an object is self-illuminating, it does not need external sources to light it, ambient or
            otherwise. It's like the object has it's own personal ambient light. This property is rarely
           useful since you can already specify per-pass ambient light, but is here for completeness.
            @note
            This setting has no effect if dynamic lighting is disabled (see Pass::setLightingEnabled),
            or if this is a programmable pass.
        */
        void setSelfIllumination( const ColourValue &selfIllum );

        /** Sets the amount of self-illumination an object has.
            @see
            setSelfIllumination
        */
        void setEmissive( const ColourValue &emissive ) { setSelfIllumination( emissive ); }

        /** Sets which material properties follow the vertex colour
         */
        void setVertexColourTracking( TrackVertexColourType tracking );

        /** Gets the point size of the pass.
        @remarks
            This property determines what point size is used to render a point
            list.
        */
        Real getPointSize() const;

        /** Sets the point size of this pass.
        @remarks
            This setting allows you to change the size of points when rendering
            a point list, or a list of point sprites. The interpretation of this
            command depends on the Pass::setPointSizeAttenuation option - if it
            is off (the default), the point size is in screen pixels, if it is on,
            it expressed as normalised screen coordinates (1.0 is the height of
            the screen) when the point is at the origin.
        @note
            Some drivers have an upper limit on the size of points they support
            - this can even vary between APIs on the same card! Don't rely on
            point sizes that cause the point sprites to get very large on screen,
            since they may get clamped on some cards. Upper sizes can range from
            64 to 256 pixels.
        */
        void setPointSize( Real ps );

        /** Sets whether or not rendering points using OT_POINT_LIST will
            render point sprites (textured quads) or plain points (dots).
        @param enabled True enables point sprites, false returns to normal
            point rendering.
        */
        void setPointSpritesEnabled( bool enabled );

        /** Returns whether point sprites are enabled when rendering a
            point list.
        */
        bool getPointSpritesEnabled() const;

        /** Sets how points are attenuated with distance.
        @remarks
            When performing point rendering or point sprite rendering,
            point size can be attenuated with distance. The equation for
            doing this is attenuation = 1 / (constant + linear * dist + quadratic * d^2).
        @par
            For example, to disable distance attenuation (constant screensize)
            you would set constant to 1, and linear and quadratic to 0. A
            standard perspective attenuation would be 0, 1, 0 respectively.
        @note
            The resulting size is clamped to the minimum and maximum point
            size.
        @param enabled Whether point attenuation is enabled
        @param constant, linear, quadratic Parameters to the attenuation
            function defined above
        */
        void setPointAttenuation( bool enabled, Real constant = 0.0f, Real linear = 1.0f,
                                  Real quadratic = 0.0f );

        /** Returns whether points are attenuated with distance. */
        bool isPointAttenuationEnabled() const;

        /** Returns the constant coefficient of point attenuation. */
        Real getPointAttenuationConstant() const;
        /** Returns the linear coefficient of point attenuation. */
        Real getPointAttenuationLinear() const;
        /** Returns the quadratic coefficient of point attenuation. */
        Real getPointAttenuationQuadratic() const;

        /** Set the minimum point size, when point attenuation is in use. */
        void setPointMinSize( Real min );
        /** Get the minimum point size, when point attenuation is in use. */
        Real getPointMinSize() const;
        /** Set the maximum point size, when point attenuation is in use.
        @remarks Setting this to 0 indicates the max size supported by the card.
        */
        void setPointMaxSize( Real max );
        /** Get the maximum point size, when point attenuation is in use.
        @remarks 0 indicates the max size supported by the card.
        */
        Real getPointMaxSize() const;

        /** Gets the ambient colour reflectance of the pass.
         */
        const ColourValue &getAmbient() const;

        /** Gets the diffuse colour reflectance of the pass.
         */
        const ColourValue &getDiffuse() const;

        /** Gets the specular colour reflectance of the pass.
         */
        const ColourValue &getSpecular() const;

        /** Gets the self illumination colour of the pass.
         */
        const ColourValue &getSelfIllumination() const;

        /** Gets the self illumination colour of the pass.
            @see
            getSelfIllumination
        */
        const ColourValue &getEmissive() const { return getSelfIllumination(); }

        /** Gets the 'shininess' property of the pass (affects specular highlights).
         */
        Real getShininess() const;

        /** Gets which material properties follow the vertex colour
         */
        TrackVertexColourType getVertexColourTracking() const;

        /** Inserts a new TextureUnitState object into the Pass.
            @remarks
            This unit is is added on top of all previous units.
        */
        TextureUnitState *createTextureUnitState();
        /** Inserts a new TextureUnitState object into the Pass.
            @remarks
            This unit is is added on top of all previous units.
            @param textureName
            The basic name of the texture e.g. brickwall.jpg, stonefloor.png
            @param texCoordSet
            The index of the texture coordinate set to use.
        */
        OGRE_DEPRECATED_VER( 3.0 )
        TextureUnitState *createTextureUnitState( const String  &textureName,
                                                  unsigned short texCoordSet );

        /** Inserts a new TextureUnitState object into the Pass.
            @remarks
            This unit is is added on top of all previous units.
            @param textureName
            The basic name of the texture e.g. brickwall.jpg, stonefloor.png
            @param texCoordSet
            The index of the texture coordinate set to use.
        */
        TextureUnitState *createTextureUnitState( const String &textureName );

        /** Adds the passed in TextureUnitState, to the existing Pass.
        @param
        state The Texture Unit State to be attached to this pass.  It must not be attached to another
        pass.
        @note
            Throws an exception if the TextureUnitState is attached to another Pass.*/
        void addTextureUnitState( TextureUnitState *state );
        /** Retrieves a pointer to a texture unit state so it may be modified.
         */
        TextureUnitState *getTextureUnitState( size_t index );
        /** Retrieves the Texture Unit State matching name.
            Returns 0 if name match is not found.
        */
        TextureUnitState *getTextureUnitState( const String &name );
        /** Retrieves a const pointer to a texture unit state.
         */
        const TextureUnitState *getTextureUnitState( size_t index ) const;
        /** Retrieves the Texture Unit State matching name.
        Returns 0 if name match is not found.
        */
        const TextureUnitState *getTextureUnitState( const String &name ) const;

        /**  Retrieve the index of the Texture Unit State in the pass.
             @param
             state The Texture Unit State this is attached to this pass.
             @note
             Throws an exception if the state is not attached to the pass.
        */
        unsigned short getTextureUnitStateIndex( const TextureUnitState *state ) const;

        typedef VectorIterator<TextureUnitStates> TextureUnitStateIterator;
        /** Get an iterator over the TextureUnitStates contained in this Pass. */
        TextureUnitStateIterator getTextureUnitStateIterator();

        typedef ConstVectorIterator<TextureUnitStates> ConstTextureUnitStateIterator;
        /** Get an iterator over the TextureUnitStates contained in this Pass. */
        ConstTextureUnitStateIterator getTextureUnitStateIterator() const;

        /** Removes the indexed texture unit state from this pass.
        @remarks
            Note that removing a texture which is not the topmost will have a larger performance impact.
        */
        void removeTextureUnitState( unsigned short index );

        /** Removes all texture unit settings.
         */
        void removeAllTextureUnitStates();

        /** Returns the number of texture unit settings.
         */
        unsigned short getNumTextureUnitStates() const
        {
            return static_cast<unsigned short>( mTextureUnitStates.size() );
        }

        /** Returns true if this pass has some element of transparency. */
        bool isTransparent() const;

        /** Determines if colour buffer writing is enabled for this pass. */
        bool getColourWriteEnabled() const;

        /** Sets the maximum number of lights to be used by this pass.
            @remarks
            During rendering, if lighting is enabled (or if the pass uses an automatic
            program parameter based on a light) the engine will request the nearest lights
            to the object being rendered in order to work out which ones to use. This
            parameter sets the limit on the number of lights which should apply to objects
            rendered with this pass.
        */
        void setMaxSimultaneousLights( unsigned short maxLights );
        /** Gets the maximum number of lights to be used by this pass. */
        unsigned short getMaxSimultaneousLights() const;

        /** Sets the light index that this pass will start at in the light list.
        @remarks
            Normally the lights passed to a pass will start from the beginning
            of the light list for this object. This option allows you to make this
            pass start from a higher light index, for example if one of your earlier
            passes could deal with lights 0-3, and this pass dealt with lights 4+.
            This option also has an interaction with pass iteration, in that
            if you choose to iterate this pass per light too, the iteration will
            only begin from light 4.
        */
        void setStartLight( unsigned short startLight );
        /** Gets the light index that this pass will start at in the light list. */
        unsigned short getStartLight() const;

        /** Sets the light mask which can be matched to specific light flags to be handled by this pass
         */
        void setLightMask( uint32 mask );
        /** Gets the light mask controlling which lights are used for this pass */
        uint32 getLightMask() const;

        /** Sets the type of light shading required
            @note
            The default shading method is Gouraud shading.
        */
        void setShadingMode( ShadeOptions mode );

        /** Returns the type of light shading to be used.
         */
        ShadeOptions getShadingMode() const;

        /** Sets whether this pass's chosen detail level can be
            overridden (downgraded) by the camera setting.
        @param override true means that a lower camera detail will override this
            pass's detail level, false means it won't (default true).
        */
        virtual void setPolygonModeOverrideable( bool override ) { mPolygonModeOverrideable = override; }

        /** Gets whether this renderable's chosen detail level can be
            overridden (downgraded) by the camera setting.
        */
        virtual bool getPolygonModeOverrideable() const { return mPolygonModeOverrideable; }
        /** Sets the fogging mode applied to this pass.
            @remarks
            Fogging is an effect that is applied as polys are rendered. Sometimes, you want
            fog to be applied to an entire scene. Other times, you want it to be applied to a few
            polygons only. This pass-level specification of fog parameters lets you easily manage
            both.
            @par
            The SceneManager class also has a setFog method which applies scene-level fog. This method
            lets you change the fog behaviour for this pass compared to the standard scene-level fog.
            @param
            overrideScene If true, you authorise this pass to override the scene's fog params with it's
           own settings. If you specify false, so other parameters are necessary, and this is the default
           behaviour for passes.
            @param
            mode Only applicable if overrideScene is true. You can disable fog which is turned on for the
            rest of the scene by specifying FOG_NONE. Otherwise, set a pass-specific fog mode as
            defined in the enum FogMode.
            @param
            colour The colour of the fog. Either set this to the same as your viewport background colour,
            or to blend in with a skydome or skybox.
            @param
            expDensity The density of the fog in FOG_EXP or FOG_EXP2 mode, as a value between 0 and 1.
            The default is 0.001.
            @param
            linearStart Distance in world units at which linear fog starts to encroach.
            Only applicable if mode is FOG_LINEAR.
            @param
            linearEnd Distance in world units at which linear fog becomes completely opaque.
            Only applicable if mode is FOG_LINEAR.
        */
        OGRE_DEPRECATED_VER( 3 )
        void setFog( bool overrideScene, FogMode mode = FOG_NONE,
                     const ColourValue &colour = ColourValue::White, Real expDensity = Real( 0.001 ),
                     Real linearStart = 0.0, Real linearEnd = 1.0 );

        /** Returns true if this pass is to override the scene fog settings.
         */
        OGRE_DEPRECATED_VER( 3 ) bool getFogOverride() const;

        /** Returns the fog mode for this pass.
            @note
            Only valid if getFogOverride is true.
        */
        OGRE_DEPRECATED_VER( 3 ) FogMode getFogMode() const;

        /** Returns the fog colour for the scene.
         */
        OGRE_DEPRECATED_VER( 3 ) const ColourValue &getFogColour() const;

        /** Returns the fog start distance for this pass.
            @note
            Only valid if getFogOverride is true.
        */
        OGRE_DEPRECATED_VER( 3 ) Real getFogStart() const;

        /** Returns the fog end distance for this pass.
            @note
            Only valid if getFogOverride is true.
        */
        OGRE_DEPRECATED_VER( 3 ) Real getFogEnd() const;

        /** Returns the fog density for this pass.
            @note
            Only valid if getFogOverride is true.
        */
        OGRE_DEPRECATED_VER( 3 ) Real getFogDensity() const;

        /// Gets the internal datablock that acts as proxy for us
        HlmsDatablock *_getDatablock() const;

        /// Changes the current macroblock for a new one. Pointer can't be null
        void setMacroblock( const HlmsMacroblock &macroblock );

        /** Retrieves current macroblock. Don't const_cast the return value to modify it.
            See HlmsDatablock remarks.
        */
        const HlmsMacroblock *getMacroblock() const;

        /// Changes the current blendblock for a new one. Pointer can't be null
        void setBlendblock( const HlmsBlendblock &blendblock );

        /** Retrieves current blendblock. Don't const_cast the return value to modify it.
            See HlmsDatablock remarks.
        */
        const HlmsBlendblock *getBlendblock() const;

        /** Sets the alpha reject function. See setAlphaRejectSettings for more information.
         */
        void setAlphaRejectFunction( CompareFunction func );

        /** Gets the alpha reject value. See setAlphaRejectSettings for more information.
         */
        void setAlphaRejectValue( unsigned char val );

        /** Gets the alpha reject function. See setAlphaRejectSettings for more information.
         */
        CompareFunction getAlphaRejectFunction() const { return mAlphaRejectFunc; }

        /** Gets the alpha reject value. See setAlphaRejectSettings for more information.
         */
        unsigned char getAlphaRejectValue() const { return mAlphaRejectVal; }

        /** Sets whether or not this pass should iterate per light or number of
            lights which can affect the object being rendered.
        @remarks
            The default behaviour for a pass (when this option is 'false'), is
            for a pass to be rendered only once (or the number of times set in
            setPassIterationCount), with all the lights which could
            affect this object set at the same time (up to the maximum lights
            allowed in the render system, which is typically 8).
        @par
            Setting this option to 'true' changes this behaviour, such that
            instead of trying to issue render this pass once per object, it
            is run <b>per light</b>, or for a group of 'n' lights each time
            which can affect this object, the number of
            times set in setPassIterationCount (default is once). In
            this case, only light index 0 is ever used, and is a different light
            every time the pass is issued, up to the total number of lights
            which is affecting this object. This has 2 advantages:
            <ul><li>There is no limit on the number of lights which can be
            supported</li>
            <li>It's easier to write vertex / fragment programs for this because
            a single program can be used for any number of lights</li>
            </ul>
            However, this technique is more expensive, and typically you
            will want an additional ambient pass, because if no lights are
            affecting the object it will not be rendered at all, which will look
            odd even if ambient light is zero (imagine if there are lit objects
            behind it - the objects silhouette would not show up). Therefore,
            use this option with care, and you would be well advised to provide
            a less expensive fallback technique for use in the distance.
        @note
            The number of times this pass runs is still limited by the maximum
            number of lights allowed as set in setMaxSimultaneousLights, so
            you will never get more passes than this. Also, the iteration is
            started from the 'start light' as set in Pass::setStartLight, and
            the number of passes is the number of lights to iterate over divided
            by the number of lights per iteration (default 1, set by
            setLightCountPerIteration).
        @param enabled Whether this feature is enabled
        @param onlyForOneLightType If true, the pass will only be run for a single type
            of light, other light types will be ignored.
        @param lightType The single light type which will be considered for this pass
        */
        void setIteratePerLight( bool enabled, bool onlyForOneLightType = true,
                                 Light::LightTypes lightType = Light::LT_POINT );

        /** Does this pass run once for every light in range? */
        bool getIteratePerLight() const { return mIteratePerLight; }
        /** Does this pass run only for a single light type (if getIteratePerLight is true). */
        bool getRunOnlyForOneLightType() const { return mRunOnlyForOneLightType; }
        /** Gets the single light type this pass runs for if  getIteratePerLight and
            getRunOnlyForOneLightType are both true. */
        Light::LightTypes getOnlyLightType() const { return mOnlyLightType; }

        /** If light iteration is enabled, determine the number of lights per
            iteration.
        @remarks
            The default for this setting is 1, so if you enable light iteration
            (Pass::setIteratePerLight), the pass is rendered once per light. If
            you set this value higher, the passes will occur once per 'n' lights.
            The start of the iteration is set by Pass::setStartLight and the end
            by Pass::setMaxSimultaneousLights.
        */
        void setLightCountPerIteration( unsigned short c );
        /** If light iteration is enabled, determine the number of lights per
        iteration.
        */
        unsigned short getLightCountPerIteration() const;

        /// Gets the parent Technique
        Technique *getParent() const { return mParent; }

        /// Gets the resource group of the ultimate parent Material
        const String &getResourceGroup() const;

        /** Sets the details of the vertex program to use.
        @remarks
            Only applicable to programmable passes, this sets the details of
            the vertex program to use in this pass. The program will not be
            loaded until the parent Material is loaded.
        @param name The name of the program - this must have been
            created using GpuProgramManager by the time that this Pass
            is loaded. If this parameter is blank, any vertex program in this pass is disabled.
        @param resetParams
            If true, this will create a fresh set of parameters from the
            new program being linked, so if you had previously set parameters
            you will have to set them again. If you set this to false, you must
            be absolutely sure that the parameters match perfectly, and in the
            case of named parameters refers to the indexes underlying them,
            not just the names.
        */
        void setVertexProgram( const String &name, bool resetParams = true );
        /** Sets the vertex program parameters.
        @remarks
            Only applicable to programmable passes, and this particular call is
            designed for low-level programs; use the named parameter methods
            for setting high-level program parameters.
        */
        void setVertexProgramParameters( GpuProgramParametersSharedPtr params );
        /** Gets the name of the vertex program used by this pass. */
        const String &getVertexProgramName() const;
        /** Gets the vertex program parameters used by this pass. */
        GpuProgramParametersSharedPtr getVertexProgramParameters() const;
        /** Gets the vertex program used by this pass, only available after _load(). */
        const GpuProgramPtr &getVertexProgram() const;

        /** Sets the details of the fragment program to use.
        @remarks
            Only applicable to programmable passes, this sets the details of
            the fragment program to use in this pass. The program will not be
            loaded until the parent Material is loaded.
        @param name The name of the program - this must have been
            created using GpuProgramManager by the time that this Pass
            is loaded. If this parameter is blank, any fragment program in this pass is disabled.
        @param resetParams
            If true, this will create a fresh set of parameters from the
            new program being linked, so if you had previously set parameters
            you will have to set them again. If you set this to false, you must
            be absolutely sure that the parameters match perfectly, and in the
            case of named parameters refers to the indexes underlying them,
            not just the names.
        */
        void setFragmentProgram( const String &name, bool resetParams = true );
        /** Sets the fragment program parameters.
        @remarks
            Only applicable to programmable passes.
        */
        void setFragmentProgramParameters( GpuProgramParametersSharedPtr params );
        /** Gets the name of the fragment program used by this pass. */
        const String &getFragmentProgramName() const;
        /** Gets the fragment program parameters used by this pass. */
        GpuProgramParametersSharedPtr getFragmentProgramParameters() const;
        /** Gets the fragment program used by this pass, only available after _load(). */
        const GpuProgramPtr &getFragmentProgram() const;

        /** Sets the details of the geometry program to use.
        @remarks
            Only applicable to programmable passes, this sets the details of
            the geometry program to use in this pass. The program will not be
            loaded until the parent Material is loaded.
        @param name The name of the program - this must have been
            created using GpuProgramManager by the time that this Pass
            is loaded. If this parameter is blank, any geometry program in this pass is disabled.
        @param resetParams
            If true, this will create a fresh set of parameters from the
            new program being linked, so if you had previously set parameters
            you will have to set them again. If you set this to false, you must
            be absolutely sure that the parameters match perfectly, and in the
            case of named parameters refers to the indexes underlying them,
            not just the names.
        */
        void setGeometryProgram( const String &name, bool resetParams = true );
        /** Sets the geometry program parameters.
        @remarks
            Only applicable to programmable passes.
        */
        void setGeometryProgramParameters( GpuProgramParametersSharedPtr params );
        /** Gets the name of the geometry program used by this pass. */
        const String &getGeometryProgramName() const;
        /** Gets the geometry program parameters used by this pass. */
        GpuProgramParametersSharedPtr getGeometryProgramParameters() const;
        /** Gets the geometry program used by this pass, only available after _load(). */
        const GpuProgramPtr &getGeometryProgram() const;

        /** Internal method to adjust pass index. */
        void _notifyIndex( unsigned short index );

        /** Internal method for preparing to load this pass. */
        void _prepare();
        /** Internal method for undoing the load preparartion for this pass. */
        void _unprepare();
        /** Internal method for loading this pass. */
        void _load();
        /** Internal method for unloading this pass. */
        void _unload();
        /// Is this loaded?
        bool isLoaded() const;

        /** Update automatic parameters.
        @param source The source of the parameters
        @param variabilityMask A mask of GpuParamVariability which identifies which autos will need
        updating
        */
        void _updateAutoParams( const AutoParamDataSource *source, uint16 variabilityMask ) const;

        /** Gets the 'nth' texture which references the given content type.
        @remarks
            If the 'nth' texture unit which references the content type doesn't
            exist, then this method returns an arbitrary high-value outside the
            valid range to index texture units.
        */
        size_t _getTextureUnitWithContentTypeIndex( TextureUnitState::ContentType contentType,
                                                    size_t                        index ) const;

        /** Set samplerblock for every texture unit
            @note
            This property actually exists on the TextureUnitState class
            For simplicity, this method allows you to set these properties for
            every current TeextureUnitState, If you need more precision, retrieve the
            TextureUnitState instance and set the property there.
            @see TextureUnitState::setSamplerblock
        */
        void setSamplerblock( const HlmsSamplerblock &samplerblock );

        /** Returns whether this pass is ambient only.
         */
        bool isAmbientOnly() const;

        /** set the number of iterations that this pass
            should perform when doing fast multi pass operation.
            @remarks
            Only applicable for programmable passes.
            @param count number of iterations to perform fast multi pass operations.
            A value greater than 1 will cause the pass to be executed count number of
            times without changing the render state.  This is very useful for passes
            that use programmable shaders that have to iterate more than once but don't
            need a render state change.  Using multi pass can dramatically speed up rendering
            for materials that do things like fur, blur.
            A value of 1 turns off multi pass operation and the pass does
            the normal pass operation.
        */
        void setPassIterationCount( const size_t count ) { mPassIterationCount = count; }

        /** Gets the pass iteration count value.
         */
        size_t getPassIterationCount() const { return mPassIterationCount; }

        /** Applies texture names to Texture Unit State with matching texture name aliases.
            All Texture Unit States within the pass are checked.
            If matching texture aliases are found then true is returned.

            @param
            aliasList is a map container of texture alias, texture name pairs
            @param
            apply set true to apply the texture aliases else just test to see if texture alias matches
           are found.
            @return
            True if matching texture aliases were found in the pass.
        */
        bool applyTextureAliases( const AliasTextureNamePairList &aliasList,
                                  const bool                      apply = true ) const;

        /** Sets whether or not this pass will be clipped by a scissor rectangle
            encompassing the lights that are being used in it.
        @remarks
            In order to cut down on fillrate when you have a number of fixed-range
            lights in the scene, you can enable this option to request that
            during rendering, only the region of the screen which is covered by
            the lights is rendered. This region is the screen-space rectangle
            covering the union of the spheres making up the light ranges. Directional
            lights are ignored for this.
        @par
            This is only likely to be useful for multipass additive lighting
            algorithms, where the scene has already been 'seeded' with an ambient
            pass and this pass is just adding light in affected areas.
        @note
            When using SHADOWTYPE_STENCIL_ADDITIVE or SHADOWTYPE_TEXTURE_ADDITIVE,
            this option is implicitly used for all per-light passes and does
            not need to be specified. If you are not using shadows or are using
            a modulative or an integrated shadow technique then this could be useful.

        */
        void setLightScissoringEnabled( bool enabled ) { mLightScissoring = enabled; }
        /** Gets whether or not this pass will be clipped by a scissor rectangle
            encompassing the lights that are being used in it.
        */
        bool getLightScissoringEnabled() const { return mLightScissoring; }

        /** Gets whether or not this pass will be clipped by user clips planes
            bounding the area covered by the light.
        @remarks
            In order to cut down on the geometry set up to render this pass
            when you have a single fixed-range light being rendered through it,
            you can enable this option to request that during triangle setup,
            clip planes are defined to bound the range of the light. In the case
            of a point light these planes form a cube, and in the case of
            a spotlight they form a pyramid. Directional lights are never clipped.
        @par
            This option is only likely to be useful for multipass additive lighting
            algorithms, where the scene has already been 'seeded' with an ambient
            pass and this pass is just adding light in affected areas. In addition,
            it will only be honoured if there is exactly one non-directional light
            being used in this pass. Also, these clip planes override any user clip
            planes set on Camera.
        @note
            When using SHADOWTYPE_STENCIL_ADDITIVE or SHADOWTYPE_TEXTURE_ADDITIVE,
            this option is automatically used for all per-light passes if you
            enable SceneManager::setShadowUseLightClipPlanes and does
            not need to be specified. It is disabled by default since clip planes have
            a cost of their own which may not always exceed the benefits they give you.
        */
        void setLightClipPlanesEnabled( bool enabled ) { mLightClipPlanes = enabled; }
        /** Gets whether or not this pass will be clipped by user clips planes
            bounding the area covered by the light.
        */
        bool getLightClipPlanesEnabled() const { return mLightClipPlanes; }

        /** Return an instance of user objects binding associated with this class.
        You can use it to associate one or more custom objects with this class instance.
        @see UserObjectBindings::setUserAny.
        */
        UserObjectBindings &getUserObjectBindings() { return mUserObjectBindings; }

        /** Return an instance of user objects binding associated with this class.
        You can use it to associate one or more custom objects with this class instance.
        @see UserObjectBindings::setUserAny.
        */
        const UserObjectBindings &getUserObjectBindings() const { return mUserObjectBindings; }

        /// Support for shader model 5.0, hull and domain shaders
        /** Sets the details of the Tessellation control program to use.
        @remarks
            Only applicable to programmable passes, this sets the details of
            the Tessellation Hull program to use in this pass. The program will not be
            loaded until the parent Material is loaded.
        @param name The name of the program - this must have been
            created using GpuProgramManager by the time that this Pass
            is loaded. If this parameter is blank, any Tessellation Hull program in this pass is
        disabled.
        @param resetParams
            If true, this will create a fresh set of parameters from the
            new program being linked, so if you had previously set parameters
            you will have to set them again. If you set this to false, you must
            be absolutely sure that the parameters match perfectly, and in the
            case of named parameters refers to the indexes underlying them,
            not just the names.
        */
        void setTessellationHullProgram( const String &name, bool resetParams = true );
        /** Sets the Tessellation Hull program parameters.
        @remarks
            Only applicable to programmable passes.
        */
        void setTessellationHullProgramParameters( GpuProgramParametersSharedPtr params );
        /** Gets the name of the Tessellation Hull program used by this pass. */
        const String &getTessellationHullProgramName() const;
        /** Gets the Tessellation Hull program parameters used by this pass. */
        GpuProgramParametersSharedPtr getTessellationHullProgramParameters() const;
        /** Gets the Tessellation Hull program used by this pass, only available after _load(). */
        const GpuProgramPtr &getTessellationHullProgram() const;

        /** Sets the details of the Tessellation domain program to use.
        @remarks
            Only applicable to programmable passes, this sets the details of
            the Tessellation domain program to use in this pass. The program will not be
            loaded until the parent Material is loaded.
        @param name The name of the program - this must have been
            created using GpuProgramManager by the time that this Pass
            is loaded. If this parameter is blank, any Tessellation domain program in this pass is
        disabled.
        @param resetParams
            If true, this will create a fresh set of parameters from the
            new program being linked, so if you had previously set parameters
            you will have to set them again. If you set this to false, you must
            be absolutely sure that the parameters match perfectly, and in the
            case of named parameters refers to the indexes underlying them,
            not just the names.
        */
        void setTessellationDomainProgram( const String &name, bool resetParams = true );
        /** Sets the Tessellation Domain program parameters.
        @remarks
            Only applicable to programmable passes.
        */
        void setTessellationDomainProgramParameters( GpuProgramParametersSharedPtr params );
        /** Gets the name of the Domain Evaluation program used by this pass. */
        const String &getTessellationDomainProgramName() const;
        /** Gets the Tessellation Domain program parameters used by this pass. */
        GpuProgramParametersSharedPtr getTessellationDomainProgramParameters() const;
        /** Gets the Tessellation Domain program used by this pass, only available after _load(). */
        const GpuProgramPtr &getTessellationDomainProgram() const;

        /** Sets the details of the compute program to use.
        @remarks
            Only applicable to programmable passes, this sets the details of
            the compute program to use in this pass. The program will not be
            loaded until the parent Material is loaded.
        @param name The name of the program - this must have been
            created using GpuProgramManager by the time that this Pass
            is loaded. If this parameter is blank, any compute program in this pass is disabled.
        @param resetParams
            If true, this will create a fresh set of parameters from the
            new program being linked, so if you had previously set parameters
            you will have to set them again. If you set this to false, you must
            be absolutely sure that the parameters match perfectly, and in the
            case of named parameters refers to the indexes underlying them,
            not just the names.
        */
        void setComputeProgram( const String &name, bool resetParams = true );
        /** Sets the Tessellation Evaluation program parameters.
        @remarks
            Only applicable to programmable passes.
        */
        void setComputeProgramParameters( GpuProgramParametersSharedPtr params );
        /** Gets the name of the Tessellation Hull program used by this pass. */
        const String &getComputeProgramName() const;
        /** Gets the Tessellation Hull program parameters used by this pass. */
        GpuProgramParametersSharedPtr getComputeProgramParameters() const;
        /** Gets the Tessellation EHull program used by this pass, only available after _load(). */
        const GpuProgramPtr &getComputeProgram() const;
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#endif
