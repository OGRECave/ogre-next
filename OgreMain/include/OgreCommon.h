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
#ifndef __Common_H__
#define __Common_H__
// Common stuff

#include "OgrePrerequisites.h"

#include "Hash/MurmurHash3.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    typedef _StringBase String;

    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup General
     *  @{
     */

    /// Fast general hashing algorithm
    inline uint32 FastHash( const char *data, int len, uint32 hashSoFar = 0 )
    {
        uint32 ret;
        MurmurHash3_x86_32( data, len, hashSoFar, &ret );
        return ret;
    }
    /// Combine hashes with same style as boost::hash_combine
    template <typename T>
    uint32 HashCombine( uint32 hashSoFar, const T &data )
    {
        return FastHash( (const char *)&data, sizeof( T ), hashSoFar );
    }

    enum VertexPass
    {
        VpNormal,
        VpShadow,
        NumVertexPass
    };

    enum PrePassMode
    {
        /// This is a normal pass.
        PrePassNone,
        /// This is a depth pre-pass. Note: Implementations may write
        /// to colour too for hybrid deferred & forward rendering.
        PrePassCreate,
        /// This pass will be using the results of a previous pre-pass
        PrePassUse
    };

    enum IndexType
    {
        IT_16BIT,
        IT_32BIT
    };

    enum OperationType
    {
        /// A list of points, 1 vertex per point
        OT_POINT_LIST = 1,
        /// A list of lines, 2 vertices per line
        OT_LINE_LIST = 2,
        /// A strip of connected lines, 1 vertex per line plus 1 start vertex
        OT_LINE_STRIP = 3,
        /// A list of triangles, 3 vertices per triangle
        OT_TRIANGLE_LIST = 4,
        /// A strip of triangles, 3 vertices for the first triangle, and 1 per triangle after that
        OT_TRIANGLE_STRIP = 5,
        /// A fan of triangles, 3 vertices for the first triangle, and 1 per triangle after that
        OT_TRIANGLE_FAN = 6,
        /// Patch control point operations, used with tessellation stages
        OT_PATCH_1_CONTROL_POINT = 7,
        OT_PATCH_2_CONTROL_POINT = 8,
        OT_PATCH_3_CONTROL_POINT = 9,
        OT_PATCH_4_CONTROL_POINT = 10,
        OT_PATCH_5_CONTROL_POINT = 11,
        OT_PATCH_6_CONTROL_POINT = 12,
        OT_PATCH_7_CONTROL_POINT = 13,
        OT_PATCH_8_CONTROL_POINT = 14,
        OT_PATCH_9_CONTROL_POINT = 15,
        OT_PATCH_10_CONTROL_POINT = 16,
        OT_PATCH_11_CONTROL_POINT = 17,
        OT_PATCH_12_CONTROL_POINT = 18,
        OT_PATCH_13_CONTROL_POINT = 19,
        OT_PATCH_14_CONTROL_POINT = 20,
        OT_PATCH_15_CONTROL_POINT = 21,
        OT_PATCH_16_CONTROL_POINT = 22,
        OT_PATCH_17_CONTROL_POINT = 23,
        OT_PATCH_18_CONTROL_POINT = 24,
        OT_PATCH_19_CONTROL_POINT = 25,
        OT_PATCH_20_CONTROL_POINT = 26,
        OT_PATCH_21_CONTROL_POINT = 27,
        OT_PATCH_22_CONTROL_POINT = 28,
        OT_PATCH_23_CONTROL_POINT = 29,
        OT_PATCH_24_CONTROL_POINT = 30,
        OT_PATCH_25_CONTROL_POINT = 31,
        OT_PATCH_26_CONTROL_POINT = 32,
        OT_PATCH_27_CONTROL_POINT = 33,
        OT_PATCH_28_CONTROL_POINT = 34,
        OT_PATCH_29_CONTROL_POINT = 35,
        OT_PATCH_30_CONTROL_POINT = 36,
        OT_PATCH_31_CONTROL_POINT = 37,
        OT_PATCH_32_CONTROL_POINT = 38
    };

    /** Comparison functions used for the depth/stencil buffer operations and
        others. */
    enum CompareFunction
    {
        CMPF_ALWAYS_FAIL,
        CMPF_ALWAYS_PASS,
        CMPF_LESS,
        CMPF_LESS_EQUAL,
        CMPF_EQUAL,
        CMPF_NOT_EQUAL,
        CMPF_GREATER_EQUAL,
        CMPF_GREATER,
        NUM_COMPARE_FUNCTIONS,
    };

    /// Enum describing the various actions which can be taken on the stencil buffer
    enum StencilOperation
    {
        /// Leave the stencil buffer unchanged
        SOP_KEEP,
        /// Set the stencil value to zero
        SOP_ZERO,
        /// Set the stencil value to the reference value
        SOP_REPLACE,
        /// Increase the stencil value by 1, clamping at the maximum value
        SOP_INCREMENT,
        /// Decrease the stencil value by 1, clamping at 0
        SOP_DECREMENT,
        /// Increase the stencil value by 1, wrapping back to 0 when incrementing the maximum value
        SOP_INCREMENT_WRAP,
        /// Decrease the stencil value by 1, wrapping when decrementing 0
        SOP_DECREMENT_WRAP,
        /// Invert the bits of the stencil buffer
        SOP_INVERT
    };

    struct StencilStateOp
    {
        StencilOperation stencilFailOp;
        StencilOperation stencilPassOp;
        StencilOperation stencilDepthFailOp;
        CompareFunction  compareOp;

        StencilStateOp() :
            stencilFailOp( SOP_KEEP ),
            stencilPassOp( SOP_KEEP ),
            stencilDepthFailOp( SOP_KEEP ),
            compareOp( CMPF_ALWAYS_FAIL )
        {
        }

        bool operator<( const StencilStateOp &other ) const
        {
            if( this->stencilFailOp != other.stencilFailOp )
                return this->stencilFailOp < other.stencilFailOp;
            if( this->stencilPassOp != other.stencilPassOp )
                return this->stencilPassOp < other.stencilPassOp;
            if( this->stencilDepthFailOp != other.stencilDepthFailOp )
                return this->stencilDepthFailOp < other.stencilDepthFailOp;

            return this->compareOp < other.compareOp;
        }

        bool operator!=( const StencilStateOp &other ) const
        {
            return this->stencilFailOp != other.stencilFailOp ||
                   this->stencilPassOp != other.stencilPassOp ||
                   this->stencilDepthFailOp != other.stencilDepthFailOp ||
                   this->compareOp != other.compareOp;
        }
    };

    ///@see HlmsPso regarding padding.
    struct StencilParams
    {
        uint8          enabled;
        uint8          readMask;
        uint8          writeMask;
        uint8          padding;
        StencilStateOp stencilFront;
        StencilStateOp stencilBack;

        StencilParams() : enabled( false ), readMask( 0xFF ), writeMask( 0xFF ), padding( 0 ) {}

        bool operator<( const StencilParams &other ) const
        {
            if( this->enabled != other.enabled )
                return this->enabled < other.enabled;
            if( this->readMask != other.readMask )
                return this->readMask < other.readMask;
            if( this->stencilFront != other.stencilFront )
                return this->stencilFront < other.stencilFront;

            return this->stencilBack < other.stencilBack;
        }

        bool operator!=( const StencilParams &other ) const
        {
            return this->enabled != other.enabled ||            //
                   this->readMask != other.readMask ||          //
                   this->writeMask != other.writeMask ||        //
                   this->stencilFront != other.stencilFront ||  //
                   this->stencilBack != other.stencilBack;
        }
    };

    /** High-level filtering options providing shortcuts to settings the
        minification, magnification and mip filters. */
    enum TextureFilterOptions
    {
        /// Equal to: min=FO_POINT, mag=FO_POINT, mip=FO_NONE
        TFO_NONE,
        /// Equal to: min=FO_LINEAR, mag=FO_LINEAR, mip=FO_POINT
        TFO_BILINEAR,
        /// Equal to: min=FO_LINEAR, mag=FO_LINEAR, mip=FO_LINEAR
        TFO_TRILINEAR,
        /// Equal to: min=FO_ANISOTROPIC, max=FO_ANISOTROPIC, mip=FO_LINEAR
        TFO_ANISOTROPIC
    };

    enum FilterType
    {
        /// The filter used when shrinking a texture
        FT_MIN,
        /// The filter used when magnifying a texture
        FT_MAG,
        /// The filter used when determining the mipmap
        FT_MIP
    };
    /** Filtering options for textures / mipmaps. */
    enum FilterOptions
    {
        /// No filtering, used for FT_MIP to turn off mipmapping
        FO_NONE,
        /// Use the closest pixel
        FO_POINT,
        /// Average of a 2x2 pixel area, denotes bilinear for MIN and MAG, trilinear for MIP
        FO_LINEAR,
        /// Similar to FO_LINEAR, but compensates for the angle of the texture plane
        FO_ANISOTROPIC
    };

    /** Light shading modes. DEPRECATED */
    enum ShadeOptions
    {
        SO_FLAT,
        SO_GOURAUD,
        SO_PHONG
    };

    /** Fog modes. */
    enum FogMode
    {
        /// No fog. Duh.
        FOG_NONE,
        /// Fog density increases  exponentially from the camera (fog = 1/e^(distance * density))
        FOG_EXP,
        /// Fog density increases at the square of FOG_EXP, i.e. even quicker (fog = 1/e^(distance *
        /// density)^2)
        FOG_EXP2,
        /// Fog density increases linearly between the start and end distances
        FOG_LINEAR
    };

    /** Hardware culling modes based on vertex winding.
        This setting applies to how the hardware API culls triangles it is sent.
    @par
        A typical way for the rendering engine to cull triangles is based on the 'vertex winding' of
        triangles. Vertex winding refers to the direction in which the vertices are passed or indexed
        to in the rendering operation as viewed from the camera, and will wither be clockwise or
        anticlockwise (that's 'counterclockwise' for you Americans out there ;) The default is
        CULL_CLOCKWISE i.e. that only triangles whose vertices are passed/indexed in anticlockwise order
        are rendered - this is a common approach and is used in 3D studio models for example. You can
        alter this culling mode if you wish but it is not advised unless you know what you are doing.
    @par
        You may wish to use the CULL_NONE option for mesh data that you cull yourself where the vertex
        winding is uncertain.
    */
    enum CullingMode
    {
        /// Hardware never culls triangles and renders everything it receives.
        CULL_NONE = 1,
        /// Hardware culls triangles whose vertices are listed clockwise in the view (default).
        CULL_CLOCKWISE = 2,
        /// Hardware culls triangles whose vertices are listed anticlockwise in the view.
        CULL_ANTICLOCKWISE = 3
    };

    /** Enumerates the wave types usable with the Ogre engine. */
    enum WaveformType
    {
        /// Standard sine wave which smoothly changes from low to high and back again.
        WFT_SINE,
        /// An angular wave with a constant increase / decrease speed with pointed peaks.
        WFT_TRIANGLE,
        /// Half of the time is spent at the min, half at the max with instant transition between.
        WFT_SQUARE,
        /// Gradual steady increase from min to max over the period with an instant return to min at the
        /// end.
        WFT_SAWTOOTH,
        /// Gradual steady decrease from max to min over the period, with an instant return to max at the
        /// end.
        WFT_INVERSE_SAWTOOTH,
        /// Pulse Width Modulation. Works like WFT_SQUARE, except the high to low transition is
        /// controlled by duty cycle. With a duty cycle of 50% (0.5) will give the same output as
        /// WFT_SQUARE.
        WFT_PWM
    };

    /** The polygon mode to use when rasterising. */
    enum PolygonMode
    {
        /// Only points are rendered.
        PM_POINTS = 1,
        /// Wireframe models are rendered.
        PM_WIREFRAME = 2,
        /// Solid polygons are rendered.
        PM_SOLID = 3
    };

    /** An enumeration describing which material properties should track the vertex colours */
    typedef int TrackVertexColourType;
    enum TrackVertexColourEnum
    {
        TVC_NONE = 0x0,
        TVC_AMBIENT = 0x1,
        TVC_DIFFUSE = 0x2,
        TVC_SPECULAR = 0x4,
        TVC_EMISSIVE = 0x8
    };

    /** Sort mode for billboard-set and particle-system */
    enum SortMode
    {
        /** Sort by direction of the camera */
        SM_DIRECTION,
        /** Sort by distance from the camera */
        SM_DISTANCE
    };

    /** Defines the frame buffer types. */
    enum FrameBufferType
    {
        FBT_COLOUR = 0x1,
        FBT_DEPTH = 0x2,
        FBT_STENCIL = 0x4
    };

    /** Defines the colour buffer types. */
    enum ColourBufferType
    {
        CBT_BACK = 0x0,
        CBT_BACK_LEFT,
        CBT_BACK_RIGHT
    };

    /** Defines the stereo mode types. */
    enum StereoModeType
    {
        SMT_NONE = 0x0,
        SMT_FRAME_SEQUENTIAL
    };

    enum ShaderType
    {
        VertexShader,
        PixelShader,
        GeometryShader,
        HullShader,
        DomainShader,
        NumShaderTypes
    };

    static const uint8 c_allGraphicStagesMask = ( 1u << VertexShader ) | ( 1u << PixelShader ) |
                                                ( 1u << GeometryShader ) | ( 1u << HullShader ) |
                                                ( 1u << DomainShader );
    // NumShaderTypes == GPT_COMPUTE_PROGRAM
    static const uint8 c_computeStageMask = 1u << NumShaderTypes;

    /** Flags for the Instance Manager when calculating ideal number of instances per batch */
    enum InstanceManagerFlags
    {
        /** Forces an amount of instances per batch low enough so that vertices * numInst < 65535
            since usually improves performance. In HW instanced techniques, this flag is ignored
        */
        IM_USE16BIT = 0x0001,

        /** The num. of instances is adjusted so that as few pixels as possible are wasted
            in the vertex texture */
        IM_VTFBESTFIT = 0x0002,

        /** Use a limited number of skeleton animations shared among all instances.
        Update only that limited amount of animations in the vertex texture.*/
        IM_VTFBONEMATRIXLOOKUP = 0x0004,

        IM_USEBONEDUALQUATERNIONS = 0x0008,

        /** Use one weight per vertex when recommended (i.e. VTF). */
        IM_USEONEWEIGHT = 0x0010,

        /** All techniques are forced to one weight per vertex. */
        IM_FORCEONEWEIGHT = 0x0020,

        IM_USEALL = IM_USE16BIT | IM_VTFBESTFIT | IM_USEONEWEIGHT
    };

    /** The types of NodeMemoryManager & ObjectMemoryManagers
    @remarks
        By default all objects are dynamic. Static objects can save a lot of performance on CPU side
        (and sometimes GPU side, for example with some instancing techniques) by telling the engine
        they won't be changing often.
    @par
        What it means for Nodes:
            Nodes created with SCENE_STATIC won't update their derived position/rotation/scale every
            frame.
            This means that modifying (eg) a static node position won't actually take effect until
            SceneManager::notifyStaticDirty( mySceneNode ) is called or some other similar call.

            If the static scene node is child of a dynamic parent node, modifying the dynamic node
            will not cause the static one to notice the change until explicitly notifying the
            SceneManager that the child node should be updated.

            If a static scene node is child of another static scene node, explicitly notifying the
            SceneManager of the parent's change automatically causes the child to be updated as well

            Having a dynamic node to be child of a static node is perfectly pausible and encouraged,
            for example a moving pendulum hanging from a static clock.
            Having a static node being child of a dynamic node doesn't make much sense, and is probably
            a bug (unless the parent is the root node).
    @par
        What it means for Entities (and InstancedEntities, etc)
            Static entities are scheduled for culling and rendering like dynamic ones, but won't update
            their world AABB bounds (even if their scene node they're attached to changes)

            Static entities will update their aabb if user calls
            SceneManager::notifyStaticDirty( myEntity ) or the static node they're attached to was also
            flagged as dirty. Note that updating the node's position doesn't flag the node as dirty
            (it's not implicit) and hence the entity won't be updated either.

            Static entities can only be attached to static nodes, and dynamic entities can only be
            attached to dynamic nodes.
    @par
        Note that on most cases, changing a single static entity or node (or creating more) can cause
        a lot of other static objects to be scheduled to update, so don't do it often, and do it all
        in the same frame. An example is doing it at startup (i.e. during loading time)
    @par
        Entities & Nodes can switch between dynamic & static at runtime. However InstancedEntities can't.
        You need to destroy the InstancedEntity and create a new one if you wish to switch (which, by
        the way, isn't expensive because batches preallocate the instances)
        InstancedEntities with different SceneMemoryMgrTypes will never share the same batch.
    */
    enum SceneMemoryMgrTypes
    {
        SCENE_DYNAMIC = 0,
        SCENE_STATIC,
        NUM_SCENE_MEMORY_MANAGER_TYPES
    };

    /** A hashed vector.
     */
    template <typename T>
    class HashedVector
    {
    public:
#if OGRE_MEMORY_ALLOCATOR == OGRE_MEMORY_ALLOCATOR_NONE
        typedef std::vector<T> VectorImpl;
#else
        typedef std::vector<T, STLAllocator<T, AllocPolicy> > VectorImpl;
#endif

    protected:
        VectorImpl     mList;
        mutable uint32 mListHash;
        mutable bool   mListHashDirty;

        void addToHash( const T &newPtr ) const
        {
            mListHash = FastHash( (const char *)&newPtr, sizeof( T ), mListHash );
        }
        void recalcHash() const
        {
            mListHash = 0;
            for( const_iterator i = mList.begin(); i != mList.end(); ++i )
                addToHash( *i );
            mListHashDirty = false;
        }

    public:
        typedef typename VectorImpl::value_type             value_type;
        typedef typename VectorImpl::pointer                pointer;
        typedef typename VectorImpl::reference              reference;
        typedef typename VectorImpl::const_reference        const_reference;
        typedef typename VectorImpl::size_type              size_type;
        typedef typename VectorImpl::difference_type        difference_type;
        typedef typename VectorImpl::iterator               iterator;
        typedef typename VectorImpl::const_iterator         const_iterator;
        typedef typename VectorImpl::reverse_iterator       reverse_iterator;
        typedef typename VectorImpl::const_reverse_iterator const_reverse_iterator;

        void dirtyHash() { mListHashDirty = true; }
        bool isHashDirty() const { return mListHashDirty; }

        iterator begin()
        {
            // we have to assume that hash needs recalculating on non-const
            dirtyHash();
            return mList.begin();
        }
        iterator         end() { return mList.end(); }
        const_iterator   begin() const { return mList.begin(); }
        const_iterator   end() const { return mList.end(); }
        reverse_iterator rbegin()
        {
            // we have to assume that hash needs recalculating on non-const
            dirtyHash();
            return mList.rbegin();
        }
        reverse_iterator       rend() { return mList.rend(); }
        const_reverse_iterator rbegin() const { return mList.rbegin(); }
        const_reverse_iterator rend() const { return mList.rend(); }
        size_type              size() const { return mList.size(); }
        size_type              max_size() const { return mList.max_size(); }
        size_type              capacity() const { return mList.capacity(); }
        bool                   empty() const { return mList.empty(); }
        reference              operator[]( size_type n )
        {
            // we have to assume that hash needs recalculating on non-const
            dirtyHash();
            return mList[n];
        }
        const_reference operator[]( size_type n ) const { return mList[n]; }
        reference       at( size_type n )
        {
            // we have to assume that hash needs recalculating on non-const
            dirtyHash();
            return mList.const_iterator( n );
        }
        const_reference at( size_type n ) const { return mList.at( n ); }
        HashedVector() : mListHash( 0 ), mListHashDirty( false ) {}
        HashedVector( size_type n ) : mList( n ), mListHash( 0 ), mListHashDirty( n > 0 ) {}
        HashedVector( size_type n, const T &t ) : mList( n, t ), mListHash( 0 ), mListHashDirty( n > 0 )
        {
        }
        HashedVector( const HashedVector<T> &rhs ) :
            mList( rhs.mList ),
            mListHash( rhs.mListHash ),
            mListHashDirty( rhs.mListHashDirty )
        {
        }

        template <class InputIterator>
        HashedVector( InputIterator a, InputIterator b ) :
            mList( a, b ),
            mListHash( 0 ),
            mListHashDirty( false )
        {
            dirtyHash();
        }

        ~HashedVector() {}
        HashedVector<T> &operator=( const HashedVector<T> &rhs )
        {
            mList = rhs.mList;
            mListHash = rhs.mListHash;
            mListHashDirty = rhs.mListHashDirty;
            return *this;
        }

        void      reserve( size_t t ) { mList.reserve( t ); }
        reference front()
        {
            // we have to assume that hash needs recalculating on non-const
            dirtyHash();
            return mList.front();
        }
        const_reference front() const { return mList.front(); }
        reference       back()
        {
            // we have to assume that hash needs recalculating on non-const
            dirtyHash();
            return mList.back();
        }
        const_reference back() const { return mList.back(); }
        void            push_back( const T &t )
        {
            mList.push_back( t );
            // Quick progressive hash add
            if( !isHashDirty() )
                addToHash( t );
        }
        void pop_back()
        {
            mList.pop_back();
            dirtyHash();
        }
        void swap( HashedVector<T> &rhs )
        {
            mList.swap( rhs.mList );
            dirtyHash();
        }
        iterator insert( iterator pos, const T &t )
        {
            bool     recalc = ( pos != end() );
            iterator ret = mList.insert( pos, t );
            if( recalc )
                dirtyHash();
            else
                addToHash( t );
            return ret;
        }

        template <class InputIterator>
        void insert( iterator pos, InputIterator f, InputIterator l )
        {
            mList.insert( pos, f, l );
            dirtyHash();
        }

        void insert( iterator pos, size_type n, const T &x )
        {
            mList.insert( pos, n, x );
            dirtyHash();
        }

        iterator erase( iterator pos )
        {
            iterator ret = mList.erase( pos );
            dirtyHash();
            return ret;
        }
        iterator erase( iterator first, iterator last )
        {
            iterator ret = mList.erase( first, last );
            dirtyHash();
            return ret;
        }
        void clear()
        {
            mList.clear();
            mListHash = 0;
            mListHashDirty = false;
        }

        void resize( size_type n, const T &t = T() )
        {
            bool recalc = false;
            if( n != size() )
                recalc = true;

            mList.resize( n, t );
            if( recalc )
                dirtyHash();
        }

        bool operator==( const HashedVector<T> &b ) { return mListHash == b.mListHash; }

        bool operator<( const HashedVector<T> &b ) { return mListHash < b.mListHash; }

        /// Get the hash value
        uint32 getHash() const
        {
            if( isHashDirty() )
                recalcHash();

            return mListHash;
        }

    public:
    };

    class Light;
    typedef FastArray<Light *> LightArray;

    /// Used as the light list, sorted
    struct LightClosest
    {
        Light *light;
        /// Index to SceneManager::mGlobalLightList.
        /// globalIndex may be == SceneManager::mGlobalLightList.size() if
        /// it holds a static light (see CompositorShadowNode::setLightFixedToShadowMap)
        /// that is not currently in camera.
        size_t globalIndex;  // Index to SceneManager::mGlobalLightList
        Real   distance;
        bool   isStatic;
        bool   isDirty;

        LightClosest() :
            light( 0 ),
            globalIndex( 0 ),
            distance( 0.0f ),
            isStatic( false ),
            isDirty( false )
        {
        }
        LightClosest( Light *_light, size_t _globalIndex, Real _distance ) :
            light( _light ),
            globalIndex( _globalIndex ),
            distance( _distance ),
            isStatic( false ),
            isDirty( false )
        {
        }

        inline bool operator<( const LightClosest &right ) const
        {
            /*Shouldn't be necessary. distance is insanely low (big negative number)
            if( light->getType() == Light::LT_DIRECTIONAL &&
                right.light->getType() != Light::LT_DIRECTIONAL )
            {
                return true;
            }
            else if( light->getType() != Light::LT_DIRECTIONAL &&
                     right.light->getType() == Light::LT_DIRECTIONAL )
            {
                return false;
            }*/
            return distance < right.distance;
        }
    };
    /// Holds all lights in SoA after being culled over all frustums
    struct LightListInfo
    {
        LightArray lights;
        /// Copy from lights[i]->getVisibilityFlags(), this copy avoids one level of indirection
        uint32 *RESTRICT_ALIAS visibilityMask;
        Sphere *RESTRICT_ALIAS boundingSphere;

        LightListInfo() : visibilityMask( 0 ), boundingSphere( 0 ) {}
        ~LightListInfo()
        {
            OGRE_FREE_SIMD( visibilityMask, MEMCATEGORY_SCENE_CONTROL );
            OGRE_FREE_SIMD( boundingSphere, MEMCATEGORY_SCENE_CONTROL );
        }
    };
    typedef HashedVector<LightClosest> LightList;
    typedef FastArray<LightClosest>    LightClosestArray;

    /// Constant blank string, useful for returning by ref where local does not exist
    const String BLANKSTRING;

    typedef StdMap<String, bool>   UnaryOptionList;
    typedef StdMap<String, String> BinaryOptionList;

    /// Name / value parameter pair (first = name, second = value)
    typedef StdMap<String, String> NameValuePairList;

    /// Alias / Texture name pair (first = alias, second = texture name)
    typedef StdMap<String, String> AliasTextureNamePairList;

    template <typename T>
    struct TRect
    {
        T left, top, right, bottom;
        TRect() : left( 0 ), top( 0 ), right( 0 ), bottom( 0 ) {}
        TRect( T const &l, T const &t, T const &r, T const &b ) :
            left( l ),
            top( t ),
            right( r ),
            bottom( b )
        {
        }
        TRect( TRect const &o ) : left( o.left ), top( o.top ), right( o.right ), bottom( o.bottom ) {}
        TRect &operator=( TRect const &o )
        {
            left = o.left;
            top = o.top;
            right = o.right;
            bottom = o.bottom;
            return *this;
        }
        T      width() const { return right - left; }
        T      height() const { return bottom - top; }
        bool   isNull() const { return width() == 0 || height() == 0; }
        void   setNull() { left = right = top = bottom = 0; }
        TRect &merge( const TRect &rhs )
        {
            if( isNull() )
            {
                *this = rhs;
            }
            else if( !rhs.isNull() )
            {
                left = std::min( left, rhs.left );
                right = std::max( right, rhs.right );
                top = std::min( top, rhs.top );
                bottom = std::max( bottom, rhs.bottom );
            }

            return *this;
        }
        TRect intersect( const TRect &rhs ) const
        {
            TRect ret;
            if( isNull() || rhs.isNull() )
            {
                // empty
                return ret;
            }
            else
            {
                ret.left = std::max( left, rhs.left );
                ret.right = std::min( right, rhs.right );
                ret.top = std::max( top, rhs.top );
                ret.bottom = std::min( bottom, rhs.bottom );
            }

            if( ret.left > ret.right || ret.top > ret.bottom )
            {
                // no intersection, return empty
                ret.left = ret.top = ret.right = ret.bottom = 0;
            }

            return ret;
        }
    };
    /*template<typename T>
    std::ostream& operator<<(std::ostream& o, const TRect<T>& r)
    {
        o << "TRect<>(l:" << r.left << ", t:" << r.top << ", r:" << r.right << ", b:" << r.bottom << ")";
        return o;
    }*/

    /** Structure used to define a rectangle in a 2-D floating point space.
     */
    typedef TRect<float> FloatRect;

    /** Structure used to define a rectangle in a 2-D floating point space,
        subject to double / single floating point settings.
    */
    typedef TRect<Real> RealRect;

    /** Structure used to define a rectangle in a 2-D integer space.
     */
    typedef TRect<long> Rect;

    /** Structure used to define a box in a 3-D integer space.
        Note that the left, top, and front edges are included but the right,
        bottom and back ones are not.
     */
    struct Box
    {
        uint32 left, top, right, bottom, front, back;
        /// Parameterless constructor for setting the members manually
        Box() : left( 0 ), top( 0 ), right( 1 ), bottom( 1 ), front( 0 ), back( 1 ) {}
        /** Define a box from left, top, right and bottom coordinates
            This box will have depth one (front=0 and back=1).
            @param  l   x value of left edge
            @param  t   y value of top edge
            @param  r   x value of right edge
            @param  b   y value of bottom edge
            @note Note that the left, top, and front edges are included
                but the right, bottom and back ones are not.
        */
        Box( uint32 l, uint32 t, uint32 r, uint32 b ) :
            left( l ),
            top( t ),
            right( r ),
            bottom( b ),
            front( 0 ),
            back( 1 )
        {
            assert( right >= left && bottom >= top && back >= front );
        }
        /** Define a box from left, top, front, right, bottom and back
            coordinates.
            @param  l   x value of left edge
            @param  t   y value of top edge
            @param  ff  z value of front edge
            @param  r   x value of right edge
            @param  b   y value of bottom edge
            @param  bb  z value of back edge
            @note Note that the left, top, and front edges are included
                but the right, bottom and back ones are not.
        */
        Box( uint32 l, uint32 t, uint32 ff, uint32 r, uint32 b, uint32 bb ) :
            left( l ),
            top( t ),
            right( r ),
            bottom( b ),
            front( ff ),
            back( bb )
        {
            assert( right >= left && bottom >= top && back >= front );
        }

        /// Return true if the other box is a part of this one
        bool contains( const Box &def ) const
        {
            return ( def.left >= left && def.top >= top && def.front >= front && def.right <= right &&
                     def.bottom <= bottom && def.back <= back );
        }

        /// Get the width of this box
        uint32 getWidth() const { return right - left; }
        /// Get the height of this box
        uint32 getHeight() const { return bottom - top; }
        /// Get the depth of this box
        uint32 getDepth() const { return back - front; }
    };

    /** Locate command-line options of the unary form '-blah' and of the
        binary form '-blah foo', passing back the index of the next non-option.
    @param numargs, argv The standard parameters passed to the main method
    @param unaryOptList Map of unary options (i.e. those that do not require a parameter).
        Should be pre-populated with, for example '-e' in the key and false in the
        value. Options which are found will be set to true on return.
    @param binOptList Map of binary options (i.e. those that require a parameter
        e.g. '-e afile.txt').
        Should be pre-populated with, for example '-e' and the default setting.
        Options which are found will have the value updated.
    */
    int _OgreExport findCommandLineOpts( int numargs, char **argv, UnaryOptionList &unaryOptList,
                                         BinaryOptionList &binOptList );

    /// Generic result of clipping
    enum ClipResult
    {
        /// Nothing was clipped
        CLIPPED_NONE = 0,
        /// Partially clipped
        CLIPPED_SOME = 1,
        /// Everything was clipped away
        CLIPPED_ALL = 2
    };

    /** Specifies orientation mode.
     */
    enum OrientationMode
    {
        OR_DEGREE_0 = 0,
        /// Causes internal resolution to swap width and height
        OR_DEGREE_90 = 1,
        OR_DEGREE_180 = 2,
        /// Causes internal resolution to swap width and height
        OR_DEGREE_270 = 3,

        OR_PORTRAIT = OR_DEGREE_0,
        OR_LANDSCAPERIGHT = OR_DEGREE_90,
        OR_LANDSCAPELEFT = OR_DEGREE_270
    };

    namespace MsaaPatterns
    {
        enum MsaaPatterns
        {
            /// Let the GPU decide.
            Undefined,
            /// The subsample locations follow a fixed known mPattern.
            /// Call TextureGpu::getSubsampleLocations to get them.
            Standard,
            /// The subsample locations are centered in a grid.
            /// May not be supported by the GPU/API, in which case Standard will be used instead
            /// Call TextureGpu::isMsaaPatternSupported to check whether it will be honoured.
            Center,
            /// All subsamples are at 0, 0; effectively "disabling" msaa.
            CenterZero
        };
    }

    /// Opaque struct that holds effective FSAA (MSAA, CSAA, etc.) mode.
    ///
    /// Note that you can request a SampleDescription, but you may get the closest
    /// quality if that particular setting is not supported by the GPU.
    ///
    /// Additionally, device lost events can cause FSAA settings to be degraded on the fly
    /// Listen for TextureGpuListener::FsaaSettingAlteredByApi events to be notified of
    /// this
    struct _OgreExport SampleDescription
    {
    protected:
        uint8 mColourSamples;
        uint8 mCoverageSamples;
        uint8 mPattern;  ///< See MsaaPatterns::MsaaPatterns
        uint8 mPadding;

    public:
        SampleDescription( uint8                      msaa = 1u,
                           MsaaPatterns::MsaaPatterns pattern = MsaaPatterns::Undefined ) :
            mColourSamples( msaa ),
            mCoverageSamples( 0 ),
            mPattern( pattern ),
            mPadding( 0u )
        {
        }
        explicit SampleDescription( const String &fsaaSetting ) { parseString( fsaaSetting ); }

        bool operator==( const SampleDescription &rhs ) const
        {
            return mColourSamples == rhs.mColourSamples && mCoverageSamples == rhs.mCoverageSamples &&
                   mPattern == rhs.mPattern;
        }

        bool operator!=( const SampleDescription &rhs ) const
        {
            return mColourSamples != rhs.mColourSamples || mCoverageSamples != rhs.mCoverageSamples ||
                   mPattern != rhs.mPattern;
        }

        bool operator<( const SampleDescription &other ) const
        {
            if( this->mColourSamples != other.mColourSamples )
                return this->mColourSamples < other.mColourSamples;
            if( this->mCoverageSamples != other.mCoverageSamples )
                return this->mCoverageSamples < other.mCoverageSamples;

            return this->mPattern < other.mPattern;
        }

        bool isMultisample() const { return mColourSamples > 1u; }

        /// For internal use
        void _set( uint8 colourSamples, uint8 coverageSamples, MsaaPatterns::MsaaPatterns pattern );

        uint8 getColourSamples() const { return mColourSamples; }
        uint8 getCoverageSamples() const { return mCoverageSamples; }
        uint8 getMaxSamples() const { return std::max( mCoverageSamples, mColourSamples ); }
        MsaaPatterns::MsaaPatterns getMsaaPattern() const
        {
            return static_cast<MsaaPatterns::MsaaPatterns>( mPattern );
        }

        void setMsaa( uint8 msaa, MsaaPatterns::MsaaPatterns pattern = MsaaPatterns::Undefined );

        bool isMsaa() const;

        /** Set CSAA by NVIDIA's marketing names e.g.
                8x CSAA call setCsaa( 8u, false )
                8x CSAA (Quality) then call setCsaa( 8u, true )
                16x CSAA call setCsaa( 16u, false )
                16x CSAA (Quality) then call setCsaa( 16u, true )
        @param samples
            Marketing value. Can be 8 or 16
        @param bQuality
            True to use the 'quality' variation, false otherwise
        */
        void setCsaa( uint8 samples, bool bQuality );

        /** Returns true if this is CSAA, whether it's quality or not
        @remark
            There is some overlap between CSAA and EQAA modes, hence this
            function may return true even if setEqaa was called
        */
        bool isCsaa() const;

        /** Returns true if this is CSAA in quality mode
        @remark
            There is some overlap between CSAA and EQAA modes, hence this
            function may return true even if setEqaa was called
        */
        bool isCsaaQuality() const;

        /** Set EQAA by its marketing number (which coincides with its technical spec) e.g.
                2f4x EQAA call setEqaa( 2u, 4u )
                4f8x EQAA call setEqaa( 4u, 8u )
                8f16x EQAA call setEqaa( 8u, 16u )
        @param mColourSamples
        @param coverageSample
        */
        void setEqaa( uint8 colourSamples, uint8 coverageSamples );

        void parseString( const String &fsaaSetting );
        /// Appends the FSAA description to the string
        void getFsaaDesc( LwString &outFsaaSetting ) const;
        /// Appends the FSAA description to the string
        void getFsaaDesc( String &outFsaaSetting ) const;
    };

    struct _OgreExport RenderingMetrics
    {
        bool   mIsRecordingMetrics;
        size_t mBatchCount;
        size_t mFaceCount;
        size_t mVertexCount;
        size_t mDrawCount;
        size_t mInstanceCount;
        RenderingMetrics();
    };

    /// Render window container.
    typedef StdVector<Window *> WindowList;

    /** @} */
    /** @} */

    /** Used for efficient removal in std::vector and std::deque (like an std::list)
        However it assumes the order of elements in container is not important or
        something external to the container holds the index of an element in it
        (but still should be kept deterministically across machines)
        Basically it swaps the iterator with the last iterator, and pops back
        Returns the next iterator
    */
    template <typename T>
    typename T::iterator efficientVectorRemove( T &container, typename T::iterator &iterator )
    {
        const ptrdiff_t idx = iterator - container.begin();
        *iterator = container.back();
        container.pop_back();

        return container.begin() + idx;
    }

    /// Aligns the input 'offset' to the next multiple of 'alignment'.
    /// Alignment can be any value except 0. Some examples:
    ///
    /// alignToNextMultiple( 0, 4 ) = 0;
    /// alignToNextMultiple( 1, 4 ) = 4;
    /// alignToNextMultiple( 2, 4 ) = 4;
    /// alignToNextMultiple( 3, 4 ) = 4;
    /// alignToNextMultiple( 4, 4 ) = 4;
    /// alignToNextMultiple( 5, 4 ) = 8;
    ///
    /// alignToNextMultiple( 0, 3 ) = 0;
    /// alignToNextMultiple( 1, 3 ) = 3;
    template <typename T>
    T alignToNextMultiple( T offset, T alignment )
    {
        return ( ( offset + alignment - 1u ) / alignment ) * alignment;
    }

    /// This function has been purposedly not been named 'alignToPrevMultiple'
    /// to avoid easily confusing it with alignToNextMultiple
    inline size_t alignToPreviousMult( size_t offset, size_t alignment )
    {
        return ( offset / alignment ) * alignment;
    }

#if defined( __has_builtin )
#    define OGRE_HAS_BUILTIN( x ) __has_builtin( x )
#else
#    define OGRE_HAS_BUILTIN( x ) 0
#endif

    /// Performs the same as std::bit_cast
    /// i.e. the same as reinterpret_cast but without breaking strict aliasing rules
    template <class Dest, class Source>
#if OGRE_HAS_BUILTIN( __builtin_bit_cast ) || _MSC_VER >= 1928
    constexpr
#else
    inline
#endif
        Dest
        bit_cast( const Source &source )
    {
#if OGRE_HAS_BUILTIN( __builtin_bit_cast ) || _MSC_VER >= 1928
        return __builtin_bit_cast( Dest, source );
#else
        static_assert( sizeof( Dest ) == sizeof( Source ),
                       "bit_cast requires source and destination to be the same size" );
        static_assert( std::is_trivially_copyable<Dest>::value,
                       "bit_cast requires the destination type to be copyable" );
        static_assert( std::is_trivially_copyable<Source>::value,
                       "bit_cast requires the source type to be copyable" );
        Dest dest;
        memcpy( &dest, &source, sizeof( dest ) );
        return dest;
#endif
    }
}  // namespace Ogre

#include "OgreSilentMemory.h"

#include "OgreHeaderSuffix.h"

#endif
