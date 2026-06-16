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
#ifndef __RenderSystem_H_
#define __RenderSystem_H_

// Precompiler options
#include "OgrePrerequisites.h"

#include "OgreCommon.h"
#include "OgreTextureUnitState.h"

#include "OgreConfigOptionMap.h"
#include "OgreGpuProgram.h"
#include "OgreHardwareVertexBuffer.h"
#include "OgrePixelFormatGpu.h"
#include "OgrePlane.h"
#include "OgreRenderSystemCapabilities.h"
#include "OgreResourceTransition.h"
#include "OgreViewport.h"

#include "OgreHeaderPrefix.h"

struct RENDERDOC_API_1_4_1;

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup RenderSystem
     *  @{
     */

    typedef vector<TextureGpu *>::type       TextureGpuVec;
    typedef map<uint16, TextureGpuVec>::type DepthBufferMap2;
    typedef map<TextureGpu *, uint16>::type  DepthBufferRefMap;
    typedef set<TextureGpu *>::type          TextureGpuSet;

    /// Enum describing the ways to generate texture coordinates
    enum TexCoordCalcMethod
    {
        /// No calculated texture coordinates
        TEXCALC_NONE,
        /// Environment map based on vertex normals
        TEXCALC_ENVIRONMENT_MAP,
        /// Environment map based on vertex positions
        TEXCALC_ENVIRONMENT_MAP_PLANAR,
        TEXCALC_ENVIRONMENT_MAP_REFLECTION,
        TEXCALC_ENVIRONMENT_MAP_NORMAL,
        /// Projective texture
        TEXCALC_PROJECTIVE_TEXTURE
    };

    /// Render window creation parameters.
    struct RenderWindowDescription
    {
        String            name;
        unsigned int      width;
        unsigned int      height;
        bool              useFullScreen;
        NameValuePairList miscParams;
    };

    struct BoundUav
    {
        GpuTrackedResource            *rttOrBuffer;
        ResourceAccess::ResourceAccess boundAccess;
    };

    /// Render window creation parameters container.
    typedef vector<RenderWindowDescription>::type RenderWindowDescriptionList;

    /** Defines the functionality of a 3D API
    @remarks
    The RenderSystem class provides a base interface
    which abstracts the general functionality of the 3D API
    e.g. Direct3D or OpenGL. Whilst a few of the general
    methods have implementations, most of this class is
    abstract, requiring a subclass based on a specific API
    to be constructed to provide the full functionality.
    Note there are 2 levels to the interface - one which
    will be used often by the caller of the Ogre library,
    and one which is at a lower level and will be used by the
    other classes provided by Ogre. These lower level
    methods are prefixed with '_' to differentiate them.
    The advanced user of the library may use these lower
    level methods to access the 3D API at a more fundamental
    level (dealing direct with render states and rendering
    primitives), but still benefiting from Ogre's abstraction
    of exactly which 3D API is in use.
    @author
    Steven Streeting
    @version
    1.0
    */
    class _OgreExport RenderSystem : public OgreAllocatedObj
    {
    public:
        /** Default Constructor.
         */
        RenderSystem();

        /** Destructor.
         */
        virtual ~RenderSystem();

        /** Returns the name of the rendering system.
         */
        virtual const String &getName() const = 0;

        /** Returns the friendly name of the render system
         */
        virtual const String &getFriendlyName() const = 0;

        /** Returns the details of this API's configuration options
        @remarks
        Each render system must be able to inform the world
        of what options must/can be specified for it's
        operation.
        @par
        These are passed as strings for portability, but
        grouped into a structure (_ConfigOption) which includes
        both options and current value.
        @par
        Note that the settings returned from this call are
        affected by the options that have been set so far,
        since some options are interdependent.
        @par
        This routine is called automatically by the default
        configuration dialogue produced by Root::showConfigDialog
        or may be used by the caller for custom settings dialogs
        @return
        A 'map' of options, i.e. a list of options which is also
        indexed by option name.
        */
        virtual ConfigOptionMap &getConfigOptions() = 0;

        /** Sets an option for this API
        @remarks
        Used to confirm the settings (normally chosen by the user) in
        order to make the renderer able to initialise with the settings as required.
        This may be video mode, D3D driver, full screen / windowed etc.
        Called automatically by the default configuration
        dialog, and by the restoration of saved settings.
        These settings are stored and only activated when
        RenderSystem::initialise or RenderSystem::reinitialise
        are called.
        @par
        If using a custom configuration dialog, it is advised that the
        caller calls RenderSystem::getConfigOptions
        again, since some options can alter resulting from a selection.
        @param
        name The name of the option to alter.
        @param
        value The value to set the option to.
        */
        virtual void setConfigOption( const String &name, const String &value ) = 0;

        /** Some options depend on other options. Therefore it's best to call
        RenderSystem::setConfigOption in order
        @param idx
            Value must be in range [0; getNumPriorityConfigOptions)

            Options must be set in ascending order,
            i.e. idx = 0 must be called before idx = 1
        @return
            The name to use in setConfigOption( name, value )
        */
        virtual const char *getPriorityConfigOption( size_t idx ) const;

        /** Number of priority config options in RenderSystem::getPriorityConfigOption
        @remarks
            IMPORTANT: The return value can change after calls to setConfigOption,
            it can even return higher or lower values than before.

            Therefore a proper loop would call getNumPriorityConfigOptions on every iteration:

            @code
                // GOOD:
                for( size_t i=0; i < rs->getNumPriorityConfigOptions(); ++i )
                    rs->setConfigOption( rs->getPriorityConfigOption( i ), value );

                // BAD:
                const size_t cachedNumOptions = rs->getNumPriorityConfigOptions();
                for( size_t i=0; i < cachedNumOptions; ++i )
                    rs->setConfigOption( rs->getPriorityConfigOption( i ), value );
            @endcode
        */
        virtual size_t getNumPriorityConfigOptions() const;

        /// Returns true if RenderSystem supports multithreaded shader and PSO compilation.
        /// Support depends on the API, our implementation, and CMake setting
        /// OGRE_SHADER_COMPILATION_THREADING_MODE with which OgreNext was built.
        virtual bool supportsMultithreadedShaderCompilation() const;

        /** Create an object for performing hardware occlusion queries.
         */
        virtual HardwareOcclusionQuery *createHardwareOcclusionQuery() = 0;

        /** Destroy a hardware occlusion query object.
         */
        virtual void destroyHardwareOcclusionQuery( HardwareOcclusionQuery *hq );

        /** Validates the options set for the rendering system, returning a message if there are
        problems.
        @note
        If the returned string is empty, there are no problems.
        */
        virtual String validateConfigOptions() = 0;

        /** Start up the renderer using the settings selected (Or the defaults if none have been
        selected).
        @remarks
        Called by Root::setRenderSystem. Shouldn't really be called
        directly, although  this can be done if the app wants to.
        @param
        autoCreateWindow If true, creates a render window
        automatically, based on settings chosen so far. This saves
        an extra call to _createRenderWindow
        for the main render window.
        @param
        windowTitle Sets the app window title
        @return
        A pointer to the automatically created window, if requested, otherwise null.
        */
        virtual Window *_initialise( bool          autoCreateWindow,
                                     const String &windowTitle = "OGRE Render Window" );

        /*
        Returns whether under the current render system buffers marked as TU_STATIC can be locked for
        update
        @remarks
        Needed in the implementation of DirectX9 with DirectX9Ex driver
        */
        virtual bool isStaticBufferLockable() const { return true; }

        /** Query the real capabilities of the GPU and driver in the RenderSystem*/
        virtual RenderSystemCapabilities *createRenderSystemCapabilities() const = 0;

        /** Get a pointer to the current capabilities being used by the RenderSystem.
        @remarks
        The capabilities may be modified using this pointer, this will only have an effect
        before the RenderSystem has been initialised. It's intended use is to allow a
        listener of the RenderSystemCapabilitiesCreated event to customise the capabilities
        on the fly before the RenderSystem is initialised.
        */
        RenderSystemCapabilities *getMutableCapabilities() { return mCurrentCapabilities; }

        /** Force the render system to use the special capabilities. Can only be called
         *    before the render system has been fully initializer (before createWindow is called)
         *   @param
         *        capabilities has to be a subset of the real capabilities and the caller is
         *        responsible for deallocating capabilities.
         */
        virtual void useCustomRenderSystemCapabilities( RenderSystemCapabilities *capabilities );

        /** Restart the renderer (normally following a change in settings).
         */
        virtual void reinitialise() = 0;

        /** Shutdown the renderer and cleanup resources.
         */
        virtual void shutdown();

        /** Returns true if device was lost and the application should destroy and recreate
            the device and all device depended resources using validateDevice() call. Note,
            that initial device lost condition detection is asynchronous, and can be reported
            by any 3D API call, resulting in RenderingApiException that should be catched.
            Device lost conditions are reported by 3D APIs as
            - Direct3D 9: D3DERR_DEVICELOST
            - Direct3D 10+: DXGI_ERROR_DEVICE_REMOVED/_RESET/_HUNG/etc
            - Vulkan: VK_ERROR_DEVICE_LOST
            - OpenGL[ES]: GL_CONTEXT_LOST
            - Metal: MTLCommandBufferErrorDeviceRemoved/AccessRevoked, MTLDeviceWasRemovedNotification
         */
        virtual bool isDeviceLost() { return false; }

        /** Checks if device was lost and recreates it with all depended resources.
            Optionally elects best physical device even if current device was not lost.
            Without forceDeviceElection param this function is pretty lightweight
            and is automatically called by Ogre on start of each frame.
         */
        virtual bool validateDevice( bool forceDeviceElection = false ) { return true; }

        /** Sets whether or not W-buffers are enabled if they are available for this renderer.
        @param
        enabled If true and the renderer supports them W-buffers will be used.  If false
        W-buffers will not be used even if available.  W-buffers are enabled by default
        for 16bit depth buffers and disabled for all other depths.
        */
        void setWBufferEnabled( bool enabled );

        /** Returns true if the renderer will try to use W-buffers when available.
         */
        bool getWBufferEnabled() const;

        /** Returns supported sample description for requested FSAA mode, with graceful downgrading.
        @note
            Depth/Stencil-only buffer formats combined with TextureFlags::Uav is valid
            but a rarely-supported combination. Don't ask for it unless that's truly what you want.
        @param sampleDesc
            Reqiested sample description.
        @param format
            Use PF_NULL returns to query support for framebuffer-less rendering.
        @param textureFlags
            See TextureFlags::TextureFlags.
            Relevant flags are:
                NotTexture
                RenderToTexture
                Uav
                RenderWindowSpecific
        @return
            Supported sample description for requested FSAA mode, with graceful downgrading.
        */
        virtual SampleDescription validateSampleDescription( const SampleDescription &sampleDesc,
                                                             PixelFormatGpu           format,
                                                             uint32                   textureFlags );

        /** Creates a new rendering window.
        @remarks
        This method creates a new rendering window as specified
        by the paramteters. The rendering system could be
        responible for only a single window (e.g. in the case
        of a game), or could be in charge of multiple ones (in the
        case of a level editor). The option to create the window
        as a child of another is therefore given.
        This method will create an appropriate subclass of
        RenderWindow depending on the API and platform implementation.
        @par
        After creation, this window can be retrieved using getRenderTarget().
        @param
        name The name of the window. Used in other methods
        later like setRenderTarget and getRenderTarget.
        @param
        width The width of the new window.
        @param
        height The height of the new window.
        @param
        fullScreen Specify true to make the window full screen
        without borders, title bar or menu bar.
        @param
        miscParams A NameValuePairList describing the other parameters for the new rendering window.
        Options are case sensitive. Unrecognised parameters will be ignored silently.
        These values might be platform dependent, but these are present for all platforms unless
        indicated otherwise:
        <table>
        <tr>
            <td><b>Key</b></td>
            <td><b>Type/Values</b></td>
            <td><b>Default</b></td>
            <td><b>Description</b></td>
            <td><b>Notes</b></td>
        </tr>
        <tr>
            <td>title</td>
            <td>Any string</td>
            <td>RenderTarget name</td>
            <td>The title of the window that will appear in the title bar</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>colourDepth</td>
            <td>16, 32</td>
            <td>Desktop depth</td>
            <td>Colour depth of the resulting rendering window; only applies if fullScreen</td>
            <td>Win32 Specific</td>
        </tr>
        <tr>
            <td>left</td>
            <td>Positive integers</td>
            <td>Centred</td>
            <td>Screen x coordinate from left</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>top</td>
            <td>Positive integers</td>
            <td>Centred</td>
            <td>Screen y coordinate from left</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>depthBuffer</td>
            <td>true, false</td>
            <td>true</td>
            <td>Use depth buffer</td>
            <td>DirectX9 specific</td>
        </tr>
        <tr>
            <td>externalWindowHandle</td>
            <td>Win32: HWND as integer<br/>
                GLX: poslong:posint:poslong (display*:screen:windowHandle) or
        poslong:posint:poslong:poslong (display*:screen:windowHandle:XVisualInfo*)<br/> OS X Cocoa:
        OgreGLView address as an integer. You can pass NSView or NSWindow too, but should perform
        OgreGLView callbacks into the Ogre manually. iOS: UIWindow address as an integer
            </td>
            <td>0 (none)</td>
            <td>External window handle, for embedding the OGRE render in an existing window</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>externalGLControl</td>
            <td>true, false</td>
            <td>false</td>
            <td>Let the external window control OpenGL i.e. don't select a pixel format for the window,
            do not change v-sync and do not swap buffer. When set to true, the calling application
            is responsible of OpenGL initialization and buffer swapping. It should also create an
            OpenGL context for its own rendering, Ogre will create one for its use. Then the calling
            application must also enable Ogre OpenGL context before calling any Ogre function and
            restore its OpenGL context after these calls.</td>
            <td>OpenGL specific</td>
        </tr>
        <tr>
            <td>externalGLContext</td>
            <td>Context as Unsigned Long</td>
            <td>0 (create own context)</td>
            <td>Use an externally created GL context</td>
            <td>OpenGL Specific</td>
        </tr>
        <tr>
            <td>parentWindowHandle</td>
            <td>Win32: HWND as integer<br/>
                GLX: poslong:posint:poslong (display*:screen:windowHandle) or
        poslong:posint:poslong:poslong (display*:screen:windowHandle:XVisualInfo*)</td> <td>0 (none)</td>
            <td>Parent window handle, for embedding the OGRE in a child of an external window</td>
            <td>&nbsp;</td>
        </tr>
         <tr>
             <td>contentScalingFactor</td>
             <td>Positive Float greater than 1.0</td>
             <td>The default content scaling factor of the screen</td>
             <td>Specifies the CAEAGLLayer content scaling factor.  Only supported on iOS 4 or greater.
                 This can be useful to limit the resolution of the OpenGL ES backing store.  For example,
        the iPhone 4's native resolution is 960 x 640.  Windows are always 320 x 480, if you would like
        to limit the display to 720 x 480, specify 1.5 as the scaling factor.
             </td>
             <td>iOS Specific</td>
             <td>&nbsp;</td>
         </tr>
         <tr>
             <td>externalViewHandle</td>
             <td>UIView pointer as an integer</td>
             <td>0</td>
             <td>External view handle, for rendering OGRE render in an existing view</td>
             <td>iOS Specific</td>
             <td>&nbsp;</td>
         </tr>
         <tr>
             <td>externalViewControllerHandle</td>
             <td>UIViewController pointer as an integer</td>
             <td>0</td>
             <td>External view controller handle, for embedding OGRE in an existing view controller</td>
             <td>iOS Specific</td>
             <td>&nbsp;</td>
         </tr>
         <tr>
             <td>externalSharegroup</td>
             <td>EAGLSharegroup pointer as an integer</td>
             <td>0</td>
             <td>External sharegroup, used to shared GL resources between contexts</td>
             <td>iOS Specific</td>
             <td>&nbsp;</td>
         </tr>
         <tr>
             <td>Full Screen</td>
             <td>true, false</td>
             <td>false</td>
             <td>Specify whether to create the window in full screen mode</td>
             <td>OS X Specific</td>
             <td>&nbsp;</td>
         </tr>
         <tr>
            <td>MSAA</td>
            <td>Positive integer (usually 1, 2, 4, 8, 16)</td>
            <td>1</td>
            <td>Multisample antialiasing factor</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>MSAA_Quality</td>
            <td>Depends on RenderSystem and hardware. Currently supports:<br/>
            0 ... infinite number (depends on HW)</td>
            <td>Blank</td>
            <td>Full screen antialiasing hint</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>displayFrequency</td>
            <td>Refresh rate in Hertz (e.g. 60, 75, 100)</td>
            <td>Desktop vsync rate</td>
            <td>Display frequency rate, for fullscreen mode</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>vsync</td>
            <td>true, false</td>
            <td>false</td>
            <td>Synchronize buffer swaps to monitor vsync, eliminating tearing at the expense of a fixed
        frame rate</td> <td>&nbsp;</td>
        </tr>
        <tr>
            <td>vsyncInterval</td>
            <td>1, 2, 3, 4</td>
            <td>1</td>
            <td>If vsync is enabled, the minimum number of vertical blanks that should occur between
        renders. For example if vsync is enabled, the refresh rate is 60 and this is set to 2, then the
            frame rate will be locked at 30.</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>border</td>
            <td>none, fixed, resize</td>
            <td>resize</td>
            <td>The type of window border (in windowed mode)</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>outerDimensions</td>
            <td>true, false</td>
            <td>false</td>
            <td>Whether the width/height is expressed as the size of the
            outer window, rather than the content area</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>useNVPerfHUD</td>
            <td>true, false</td>
            <td>false</td>
            <td>Enable the use of nVidia NVPerfHUD</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>gamma</td>
            <td>true, false</td>
            <td>false</td>
            <td>Enable hardware conversion from linear colour space to gamma
            colour space on rendering to the window.</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>enableDoubleClick</td>
            <td>true, false</td>
            <td>false</td>
            <td>Enable the window to keep track and transmit double click messages.</td>
            <td>Win32 Specific</td>
        </tr>
        <tr>
            <td>MSAA</td>
            <td>Positive integer (usually 0, 2, 4, 8, 16)</td>
            <td>0</td>
            <td>Full screen antialiasing factor</td>
            <td>Android Specific</td>
        </tr>
        <tr>
            <td>CSAA</td>
            <td>Positive integer (usually 0, 2, 4, 8, 16)</td>
            <td>0</td>
            <td>Coverage sampling factor
        (https://www.khronos.org/registry/egl/extensions/NV/EGL_NV_coverage_sample.txt)</td> <td>Android
        Specific</td>
        </tr>
        <tr>
            <td>maxColourBufferSize</td>
            <td>Positive integer (usually 16, 32)</td>
            <td>32</td>
            <td>Max EGL_BUFFER_SIZE</td>
            <td>Android Specific</td>
        </tr>
        <tr>
            <td>minColourBufferSize</td>
            <td>Positive integer (usually 16, 32)</td>
            <td>16</td>
            <td>Min EGL_BUFFER_SIZE</td>
            <td>Android Specific</td>
        </tr>
        <tr>
            <td>maxStencilBufferSize</td>
            <td>Positive integer (usually 0, 8)</td>
            <td>0</td>
            <td>EGL_STENCIL_SIZE</td>
            <td>Android Specific</td>
        </tr>
        <tr>
            <td>maxDepthBufferSize</td>
            <td>Positive integer (usually 0, 16, 24)</td>
            <td>16</td>
            <td>EGL_DEPTH_SIZE</td>
            <td>Android Specific</td>
        </tr>
        */
        virtual Window *_createRenderWindow( const String &name, uint32 width, uint32 height,
                                             bool                     fullScreen,
                                             const NameValuePairList *miscParams = 0 ) = 0;

        /** Creates multiple rendering windows.
        @param
        renderWindowDescriptions Array of structures containing the descriptions of each render window.
        The structure's members are the same as the parameters of _createRenderWindow:
        + name
        + width
        + height
        + fullScreen
        + miscParams
        .
        See _createRenderWindow for details about each member.
        @param
        createdWindows This array will hold the created render windows.
        @return
        true on success.
        */
        virtual bool _createRenderWindows( const RenderWindowDescriptionList &renderWindowDescriptions,
                                           WindowList                        &createdWindows );

        /** Destroys a render window */
        virtual void destroyRenderWindow( Window *window );

        /** Returns a description of an error code.
         */
        virtual String getErrorDescription( long errorNumber ) const = 0;

        /** Returns the global instance vertex buffer.
         */
        v1::HardwareVertexBufferSharedPtr getGlobalInstanceVertexBuffer() const;
        /** Sets the global instance vertex buffer.
         */
        void setGlobalInstanceVertexBuffer( const v1::HardwareVertexBufferSharedPtr &val );
        /** Gets vertex declaration for the global vertex buffer for the global instancing
         */
        v1::VertexDeclaration *getGlobalInstanceVertexBufferVertexDeclaration() const;
        /** Sets vertex declaration for the global vertex buffer for the global instancing
         */
        void setGlobalInstanceVertexBufferVertexDeclaration( v1::VertexDeclaration *val );
        /** Gets the global number of instances.
         */
        size_t getGlobalNumberOfInstances() const;
        /** Sets the global number of instances.
         */
        void setGlobalNumberOfInstances( const size_t val );

        // ------------------------------------------------------------------------
        //                     Internal Rendering Access
        // All methods below here are normally only called by other OGRE classes
        // They can be called by library user if required
        // ------------------------------------------------------------------------

        /** Tells the rendersystem to use the attached set of lights (and no others)
        up to the number specified (this allows the same list to be used with different
        count limits) */
        virtual void _useLights( const LightList &lights, unsigned short limit ) = 0;
        /** Are fixed-function lights provided in view space? Affects optimisation.
         */
        virtual bool areFixedFunctionLightsInViewSpace() const { return false; }
        /** Sets the world transform matrix. */
        virtual void _setWorldMatrix( const Matrix4 &m ) = 0;
        /** Sets multiple world matrices (vertex blending). */
        virtual void _setWorldMatrices( const Matrix4 *m, unsigned short count );
        /** Sets the view transform matrix */
        virtual void _setViewMatrix( const Matrix4 &m ) = 0;
        /** Sets the projection transform matrix */
        virtual void _setProjectionMatrix( const Matrix4 &m ) = 0;
        /** Utility function for setting all the properties of a texture unit at once.
        This method is also worth using over the individual texture unit settings because it
        only sets those settings which are different from the current settings for this
        unit, thus minimising render state changes.
        */
        virtual void _setTextureUnitSettings( size_t texUnit, TextureUnitState &tl );
        /** Set texture unit binding type */
        virtual void _setBindingType( TextureUnitState::BindingType bindigType );
        /** Sets the surface properties to be used for future rendering.

        This method sets the the properties of the surfaces of objects
        to be rendered after it. In this context these surface properties
        are the amount of each type of light the object reflects (determining
        it's colour under different types of light), whether it emits light
        itself, and how shiny it is. Textures are not dealt with here,
        see the _setTetxure method for details.
        This method is used by _setMaterial so does not need to be called
        direct if that method is being used.

        @param ambient The amount of ambient (sourceless and directionless)
        light an object reflects. Affected by the colour/amount of ambient light in the scene.
        @param diffuse The amount of light from directed sources that is
        reflected (affected by colour/amount of point, directed and spot light sources)
        @param specular The amount of specular light reflected. This is also
        affected by directed light sources but represents the colour at the
        highlights of the object.
        @param emissive The colour of light emitted from the object. Note that
        this will make an object seem brighter and not dependent on lights in
        the scene, but it will not act as a light, so will not illuminate other
        objects. Use a light attached to the same SceneNode as the object for this purpose.
        @param shininess A value which only has an effect on specular highlights (so
        specular must be non-black). The higher this value, the smaller and crisper the
        specular highlights will be, imitating a more highly polished surface.
        This value is not constrained to 0.0-1.0, in fact it is likely to
        be more (10.0 gives a modest sheen to an object).
        @param tracking A bit field that describes which of the ambient, diffuse, specular
        and emissive colours follow the vertex colour of the primitive. When a bit in this field is set
        its ColourValue is ignored. This is a combination of TVC_AMBIENT, TVC_DIFFUSE, TVC_SPECULAR(note
        that the shininess value is still taken from shininess) and TVC_EMISSIVE. TVC_NONE means that
        there will be no material property tracking the vertex colours.
        */
        virtual void _setSurfaceParams( const ColourValue &ambient, const ColourValue &diffuse,
                                        const ColourValue &specular, const ColourValue &emissive,
                                        Real shininess, TrackVertexColourType tracking = TVC_NONE ) = 0;

        /** Sets whether or not rendering points using OT_POINT_LIST will
        render point sprites (textured quads) or plain points.
        @param enabled True enables point sprites, false returns to normal
        point rendering.
        */
        virtual void _setPointSpritesEnabled( bool enabled ) = 0;

        /** Sets the size of points and how they are attenuated with distance.
        @remarks
        When performing point rendering or point sprite rendering,
        point size can be attenuated with distance. The equation for
        doing this is attenuation = 1 / (constant + linear * dist + quadratic * d^2) .
        @par
        For example, to disable distance attenuation (constant screensize)
        you would set constant to 1, and linear and quadratic to 0. A
        standard perspective attenuation would be 0, 1, 0 respectively.
        */
        virtual void _setPointParameters( Real size, bool attenuationEnabled, Real constant, Real linear,
                                          Real quadratic, Real minSize, Real maxSize ) = 0;

        /**
        Sets the texture to bind to a given texture unit.

        User processes would not normally call this direct unless rendering
        primitives themselves.

        @param unit The index of the texture unit to modify. Multitexturing
        hardware can support multiple units (see
        RenderSystemCapabilites::getNumTextureUnits)
        @param enabled Boolean to turn the unit on/off
        @param texPtr Pointer to the texture to use.
        @param bDepthReadOnly
            true if the texture is also attached as a depth buffer but is read only
        */
        virtual void _setTexture( size_t unit, TextureGpu *texPtr, bool bDepthReadOnly ) = 0;

        /** Because Ogre doesn't (yet) have the notion of a 'device' or 'GL context',
            this function lets Ogre know which device should be used by providing
            a texture.
        @param texture
            Cannot be null.
        */
        virtual void _setCurrentDeviceFromTexture( TextureGpu *texture ) = 0;

        virtual RenderPassDescriptor *createRenderPassDescriptor() = 0;
        void destroyRenderPassDescriptor( RenderPassDescriptor *renderPassDesc );

        RenderPassDescriptor *getCurrentPassDescriptor() { return mCurrentRenderPassDescriptor; }
        Viewport             &_getCurrentRenderViewport() { return mCurrentRenderViewport[0]; }
        Viewport             *getCurrentRenderViewports() { return mCurrentRenderViewport; }
        uint32                getMaxBoundViewports() { return mMaxBoundViewports; }

        /** When the descriptor is set to Load clear, two possible things may happen:
                1. The region is cleared.
                2. The whole texture is cleared.
            What actually happens is undefined (depends on the API). But calling
            "beginRenderPassDescriptor( desc, viewportSettings );" with the same
            descriptor but different viewports (without changing the desc)
            guarantees that each region is cleared:
                1. Each time the subregion is switched
                2. Only once (the whole texture), when the first viewport was set.
        @par
            When switching between render targets, two beginRenderPassDescriptor
            in a row automatically implies calls endRenderPassDescriptor.
            In fact this is faster to perfom than calling beginRenderPassDescriptor -
            endRenderPassDescriptor in pairs, because we can smartly flush only what
            needs to be flushed.
            endRenderPassDescriptor only needs to be called when no other
            beginRenderPassDescriptor will follow (i.e. at the end of the frame or
            when starting compute jobs)
        @param desc
        @param anyTarget
            Contains the first valid texture in mRenderPassDesc, to be used for reference
            (e.g. width, height, etc). Could be colour, depth, stencil, or nullptr.
        @param mipLevel
            Mip at which anyTarget is bound
        @param viewportSize
        @param warnIfRtvWasFlushed
            See CompositorPassDef::mWarnIfRtvWasFlushed
        */
        virtual void beginRenderPassDescriptor( RenderPassDescriptor *desc, TextureGpu *anyTarget,
                                                uint8 mipLevel, const Vector4 *viewportSizes,
                                                const Vector4 *scissors, uint32 numViewports,
                                                bool overlaysEnabled, bool warnIfRtvWasFlushed );
        /// Metal needs to delay RenderCommand creation to the last minute, because
        /// we can't issue blit operations (e.g. buffer copies) which a lot of v1
        /// code relies on otherwise the RenderCommand gets canceled.
        /// Even if we were to get rid of v1 operations, the user may want to hook
        /// listeners for _renderPhase02 and perform forbidden operations.
        /// Therefore it's easier to split the process done in beginRenderPassDescriptor
        /// in two steps (beginRenderPassDescriptor and executeRenderPassDescriptorDelayedActions)
        /// for Metal.
        virtual void executeRenderPassDescriptorDelayedActions();
        virtual void endRenderPassDescriptor();

    protected:
        void debugLogPso( const HlmsPso *pso );
        void debugLogPso( const HlmsComputePso *pso );

        /// Reads DepthBuffer::AvailableDepthFormats and alters DepthBuffer::DefaultDepthBufferFormat
        void selectDepthBufferFormat( const uint8 supportedFormats );

        virtual TextureGpu *createDepthBufferFor( TextureGpu *colourTexture, bool preferDepthTexture,
                                                  PixelFormatGpu depthBufferFormat, uint16 poolId );

        /// Detroys a depth buffer associated in the pool. If no texture is found the it skips.
        void destroySharedDepthBuffer( TextureGpu *depthTexture );
        void referenceSharedDepthBuffer( TextureGpu *depthBuffer );

    public:
        void _cleanupDepthBuffers();
        /// Releases the reference count on a shared depth buffer.
        /// Does nothing if input is not a shared depth buffer.
        void _dereferenceSharedDepthBuffer( TextureGpu *depthBuffer );

        virtual TextureGpu *getDepthBufferFor( TextureGpu *colourTexture, uint16 poolId,
                                               bool           preferDepthTexture,
                                               PixelFormatGpu depthBufferFormat );

        /** In Direct3D11, UAV & RenderTargets share the same slots. Because of this,
            we enforce the same behavior on all RenderSystems.
            An unfortunate consequence is that if you attach an MRT consisting of 3 RTs;
            the UAV needs to set at slot 3; not slot 0.
            This setting lets you tell Ogre the starting slot; so queueBindUAVs( descSet )
            goes from slot 3 onwards if you call setUavStartingSlot( 3 )
        @par
            Ogre will raise an exception in D3D11 if the starting slot is lower than
            the number of attached RTs, but will let it pass if you're using GL3+
            [TODO: Make this behavior consistent?]
        @remarks
            Will not take effect until the next call to flushUAVs or setting a new RTT.
        @param startingSlot
            Default value: 1.
        */
        void setUavStartingSlot( uint32 startingSlot );

        /** Queues the binding of an UAV to the binding point/slot.
            It won't actually take effect until you flush the UAVs or set another RTT.
        @remarks
            Internal Developer Notes:
            D3D11 keeps UAVs that affect rendering separate from UAVs that affect Compute Shaders.
            Hence queueBindUAVs & _setTextureCS are independent.

            OpenGL however, does not make this distinction. Hence once we switch back to
            3D rendering, we need to restore UAVs set via queueBindUAV.
        */
        void queueBindUAVs( const DescriptorSetUav *descSetUav );

        BoundUav getBoundUav( size_t slot ) const;

        /**
        @param slotStart
        @param set
        @param hazardousTexIdx
            When hazardousTexIdx < set->mTextures.size(); it means that we need to check
            if set->mTextures[hazardousTexIdx] is not the same as the currently bound RTT.
        */
        virtual void _setTextures( uint32 slotStart, const DescriptorSetTexture *set,
                                   uint32 hazardousTexIdx ) = 0;
        virtual void _setTextures( uint32 slotStart, const DescriptorSetTexture2 *set ) = 0;
        virtual void _setSamplers( uint32 slotStart, const DescriptorSetSampler *set ) = 0;
        virtual void _setTexturesCS( uint32 slotStart, const DescriptorSetTexture *set ) = 0;
        virtual void _setTexturesCS( uint32 slotStart, const DescriptorSetTexture2 *set ) = 0;
        virtual void _setSamplersCS( uint32 slotStart, const DescriptorSetSampler *set ) = 0;
        virtual void _setUavCS( uint32 slotStart, const DescriptorSetUav *set ) = 0;

        /// Required when caller will soon start analyzing barriers (e.g. use BarrierSolver)
        /// Ogre will flush any pending resource transitions.
        ///
        /// Otherwise BarrierSolver will see that a Resource is in a particular state or layout,
        /// then when calling executeResourceTransition, the pending resource layout
        /// will be flushed, and now the resource transition resolved by BarrierSolver
        /// will have the wrong 'old' layout
        virtual void endCopyEncoder() {}

        virtual void executeResourceTransition( const ResourceTransitionArray &rstCollection ) {}

        /** PSO creation on Vulkan could be skipped after exhausting per-frame time budget.

            Every frame we have a time budget for compiling PSOs. For example, 8ms.
            If we spend more than 8ms compiling shaders and PSOs, we abort those and will attempt to
            compile them on the next frame. Any object that uses such uncompiled PSO will not appear
            on screen.

            Example: if each PSO takes 1ms to compile and we have 10 PSOs left, then we'll
            compile 8 PSOs, and the other 2 will be compiled on the next frame.
            If a single PSO takes 20ms, then only that PSO will be compiled (we can't stop mid-compile).
            If each PSO takes 1.2ms, we'll compile 7 PSOs. Because at the 6th PSO we will have taken
            7.2ms, which is below the deadline, and after the 7th PSO we will have taken 8.4ms.
            This happens because we can't actually predict how long a PSO will take.

            This feature reduces stutter by allocating an aproximate maximum time we spend compiling. But
            it trades off in visual glitches/artifacts until all PSOs are done.
            This feature is important on Android to reduce the number of ANRs.
        @remark
            IMPORTANT: Techniques that rely on running a shader once (e.g. to fill a texture) may end
            up uninitialized if the PSO doesn't compile on time. It's best to disable this feature
            when doing such tasks or to monitor getIncompletePsoRequestsCounter() and retry.
        @remark
            When PSOs are deferred due to this feature, it will be logged. Keep an eye on Ogre.log.
        @param ms
            Time deadline in milliseconds.
        */
        void   setPsoRequestsTimeout( uint32 ms ) { mPsoRequestsTimeout = ms; }
        uint32 getPsoRequestsTimeout() const { return mPsoRequestsTimeout; }

        /// The amount of PSOs that remain uncompiled because they've exceeded the time budget.
        /// See setPsoRequestsTimeout(). This value gets reset to 0 at the beginning of every frame.
        uint32 getIncompletePsoRequestsCounter() const { return mIncompletePsoRequestsCounterFrame; }
        void   _notifyIncompletePsoRequests( uint32 count );

        /// return false for recoverable errors, for example for exhausted per-frame time budget
        virtual bool _hlmsPipelineStateObjectCreated( HlmsPso *newPso, uint64 deadline = UINT64_MAX )
        {
            return true;
        }
        virtual void _hlmsPipelineStateObjectDestroyed( HlmsPso *pso ) {}
        virtual void _hlmsMacroblockCreated( HlmsMacroblock *newBlock ) {}
        virtual void _hlmsMacroblockDestroyed( HlmsMacroblock *block ) {}
        virtual void _hlmsBlendblockCreated( HlmsBlendblock *newBlock ) {}
        virtual void _hlmsBlendblockDestroyed( HlmsBlendblock *block ) {}
        virtual void _hlmsSamplerblockCreated( HlmsSamplerblock *newBlock ) {}
        virtual void _hlmsSamplerblockDestroyed( HlmsSamplerblock *block ) {}
        virtual void _descriptorSetTextureCreated( DescriptorSetTexture *newSet ) {}
        virtual void _descriptorSetTextureDestroyed( DescriptorSetTexture *set ) {}
        virtual void _descriptorSetTexture2Created( DescriptorSetTexture2 *newSet ) {}
        virtual void _descriptorSetTexture2Destroyed( DescriptorSetTexture2 *set ) {}
        virtual void _descriptorSetSamplerCreated( DescriptorSetSampler *newSet ) {}
        virtual void _descriptorSetSamplerDestroyed( DescriptorSetSampler *set ) {}
        virtual void _descriptorSetUavCreated( DescriptorSetUav *newSet ) {}
        virtual void _descriptorSetUavDestroyed( DescriptorSetUav *set ) {}

        virtual void _setIndirectBuffer( IndirectBufferPacked *indirectBuffer ) = 0;

        virtual void _hlmsComputePipelineStateObjectCreated( HlmsComputePso *newPso ) {}
        virtual void _hlmsComputePipelineStateObjectDestroyed( HlmsComputePso *newPso ) {}

        /** Binds a texture to a vertex, geometry, compute, tessellation hull
        or tessellation domain sampler.
        @remarks
        Not all rendersystems support separate vertex samplers. For those that
        do, you can set a texture for them, separate to the regular texture
        samplers, using this method. For those that don't, you should use the
        regular texture samplers which are shared between the vertex and
        fragment units; calling this method will throw an exception.
        @see RenderSystemCapabilites::getVertexTextureUnitsShared
        */
        virtual void _setVertexTexture( size_t unit, TextureGpu *tex );
        virtual void _setGeometryTexture( size_t unit, TextureGpu *tex );
        virtual void _setTessellationHullTexture( size_t unit, TextureGpu *tex );
        virtual void _setTessellationDomainTexture( size_t unit, TextureGpu *tex );

        /**
        Sets a method for automatically calculating texture coordinates for a stage.
        Should not be used by apps - for use by Ogre only.
        @param unit Texture unit as above
        @param m Calculation method to use
        @param frustum Optional Frustum param, only used for projective effects
        */
        virtual void _setTextureCoordCalculation( size_t unit, TexCoordCalcMethod m,
                                                  const Frustum *frustum = 0 ) = 0;

        /** Sets the texture blend modes from a TextureUnitState record.
        Meant for use internally only - apps should use the Material
        and TextureUnitState classes.
        @param unit Texture unit as above
        @param bm Details of the blending mode
        */
        virtual void _setTextureBlendMode( size_t unit, const LayerBlendModeEx &bm ) = 0;

        /** Sets the texture coordinate transformation matrix for a texture unit.
        @param unit Texture unit to affect
        @param xform The 4x4 matrix
        */
        virtual void _setTextureMatrix( size_t unit, const Matrix4 &xform ) = 0;

        /** Notify the rendersystem that it should adjust texture projection to be
            relative to a different origin.
        */
        virtual void _setTextureProjectionRelativeTo( bool enabled, const Vector3 &pos );

        /// Signifies the beginning of the main frame. i.e. will only be called once per frame,
        /// not per viewport
        virtual void _beginFrameOnce();
        /// Called once per frame, regardless of how many active workspaces there are.
        /// Gets called AFTER all RenderWindows have been swapped.
        virtual void _endFrameOnce();

        /**
         * Signifies the beginning of a frame, i.e. the start of rendering on a single viewport. Will
         * occur several times per complete frame if multiple viewports exist.
         */
        virtual void _beginFrame() = 0;

        // Dummy structure for render system contexts - implementing RenderSystems can extend
        // as needed
        struct RenderSystemContext
        {
        };
        /**
         * Pause rendering for a frame. This has to be called after _beginFrame and before _endFrame.
         * Will usually be called by the SceneManager, don't use this manually unless you know what
         * you are doing.
         */
        virtual RenderSystemContext *_pauseFrame();
        /**
         * Resume rendering for a frame. This has to be called after a _pauseFrame call
         * Will usually be called by the SceneManager, don't use this manually unless you know what
         * you are doing.
         * @param context the render system context, as returned by _pauseFrame
         */
        virtual void _resumeFrame( RenderSystemContext *context );

        /**
         * Ends rendering of a frame to the current viewport.
         */
        virtual void _endFrame() = 0;

        /// Called once per frame, regardless of how many active workspaces there are
        void _update();

        /// This gives the renderer a chance to perform the compositor update in a special way.
        /// When the render system is ready to perform the actual update it should just
        /// compositorManager->_updateImplementation.
        virtual void updateCompositorManager( CompositorManager2 *compositorManager );

        /// See RenderSystem::updateCompositorManager
        virtual void compositorWorkspaceBegin( CompositorWorkspace *workspace,
                                               const bool           forceBeginFrame );
        /// See RenderSystem::updateCompositorManager
        virtual void compositorWorkspaceUpdate( CompositorWorkspace *workspace );
        /// See RenderSystem::updateCompositorManager
        virtual void compositorWorkspaceEnd( CompositorWorkspace *workspace, const bool forceEndFrame );

        /// See HlmsSamplerblock. This function MUST be called after _setTexture, not before.
        /// Otherwise not all APIs may see the change.
        virtual void _setHlmsSamplerblock( uint8 texUnit, const HlmsSamplerblock *Samplerblock ) = 0;

        /// @see HlmsPso
        virtual void _setPipelineStateObject( const HlmsPso *pso );

        /// Unlike _setPipelineStateObject, the RenderSystem will check if the PSO
        /// has changed to avoid redundant state changes (since it's hard to do it
        /// at Hlms level)
        virtual void _setComputePso( const HlmsComputePso *pso ) = 0;

        void _resetMetrics();
        void _addMetrics( const RenderingMetrics &newMetrics );

        void setMetricsRecordingEnabled( bool bEnable );

        const RenderingMetrics &getMetrics() const;

        /** Generates a packed data version of the passed in ColourValue suitable for
        use as with this RenderSystem.
        @remarks
        Since different render systems have different colour data formats (eg
        RGBA for GL, ARGB for D3D) this method allows you to use 1 method for all.
        @param colour The colour to convert
        @param pDest Pointer to location to put the result.
        */
        virtual void convertColourValue( const ColourValue &colour, uint32 *pDest );
        /** Get the native VertexElementType for a compact 32-bit colour value
        for this rendersystem.
        */
        virtual VertexElementType getColourVertexElementType() const = 0;

        /** Reverts the compare order e.g. greater_equal becomes less_equal
            Used by reverse depth
        @param depthFunc
        @return
        */
        static CompareFunction reverseCompareFunction( CompareFunction depthFunc );

        /** Takes a regular source projection matrix in range [-1; 1] and converts it to a projection
            matrix in 'dest' with reverse Z range [1; 0]

            _convertProjectionMatrix does the same thing but is more generic. This version
            assumes a standard projection matrix (i.e. not oblique) to maximize precision
        @param matrix
        @param dest
        @param nearPlane
        @param farPlane
        @param projectionType
        */
        virtual void _makeRsProjectionMatrix( const Matrix4 &matrix, Matrix4 &dest, Real nearPlane,
                                              Real farPlane, ProjectionType projectionType );

        /** Converts a uniform projection matrix to suitable for this render system.
        @remarks
        Because different APIs have different requirements (some incompatible) for the
        projection matrix, this method allows each to implement their own correctly and pass
        back a generic OGRE matrix for storage in the engine.
        */
        virtual void _convertProjectionMatrix( const Matrix4 &matrix, Matrix4 &dest );

        /** Converts an OpenVR projection matrix to have the proper depth range and
            reverse Z settings
        @param matrix
        @param dest
        */
        virtual void _convertOpenVrProjectionMatrix( const Matrix4 &matrix, Matrix4 &dest );

        /// OpenGL depth is in range [-1;1] so it returns 2.0f;
        /// D3D11 & Metal are in range [0;1] so it returns 1.0f;
        ///
        /// Note OpenGL may behave like D3D11, and thus we'll return 1.0f too.
        /// This is decided at runtime, not at compile time.
        virtual Real getRSDepthRange() const { return 1.0f; }

        /** This method allows you to set all the stencil buffer parameters in one call.
        @remarks
        The stencil buffer is used to mask out pixels in the render target, allowing
        you to do effects like mirrors, cut-outs, stencil shadows and more. Each of
        your batches of rendering is likely to ignore the stencil buffer,
        update it with new values, or apply it to mask the output of the render.
        The stencil test is:<PRE>
        (Reference Value & Mask) CompareFunction (Stencil Buffer Value & Mask)</PRE>
        The result of this will cause one of 3 actions depending on whether the test fails,
        succeeds but with the depth buffer check still failing, or succeeds with the
        depth buffer check passing too.
        @par
        Unlike other render states, stencilling is left for the application to turn
        on and off when it requires. This is because you are likely to want to change
        parameters between batches of arbitrary objects and control the ordering yourself.
        In order to batch things this way, you'll want to use OGRE's separate render queue
        groups (see RenderQueue) and register a RenderQueueListener to get notifications
        between batches.
        @param refValue
            The reference value used in the comparison (dynamic)
        @param stencilParams
            The static parameters that involve more expensive state changes.
            Ogre dev implementors note: Should check if the stencilParams are different from before
        */
        virtual void setStencilBufferParams( uint32 refValue, const StencilParams &stencilParams );

        const StencilParams &getStencilBufferParams() const { return mStencilParams; }

        /**
        Render something to the active viewport.

        Low-level rendering interface to perform rendering
        operations. Unlikely to be used directly by client
        applications, since the SceneManager and various support
        classes will be responsible for calling this method.
        Can only be called between _beginScene and _endScene

        @param op A rendering operation instance, which contains
        details of the operation to be performed.
        */
        virtual void _render( const v1::RenderOperation &op );

        virtual void _dispatch( const HlmsComputePso &pso ) = 0;

        /** Part of the low level rendering interface. Tells the RS which VAO will be bound now.
            (i.e. Vertex Formats, buffers being bound, etc.)
            You don't need to rebind if the VAO's mRenderQueueId is the same as previous call.
        @remarks
            Assumes _setPipelineStateObject has already been called.
        */
        virtual void _setVertexArrayObject( const VertexArrayObject *vao ) = 0;

        /// Renders the VAO. Assumes _setVertexArrayObject has already been called.
        virtual void _render( const CbDrawCallIndexed *cmd ) = 0;
        virtual void _render( const CbDrawCallStrip *cmd ) = 0;
        virtual void _renderEmulated( const CbDrawCallIndexed *cmd ) = 0;
        virtual void _renderEmulated( const CbDrawCallStrip *cmd ) = 0;
        virtual void _renderEmulatedNoBaseInstance( const CbDrawCallIndexed *cmd ) {}
        virtual void _renderEmulatedNoBaseInstance( const CbDrawCallStrip *cmd ) {}

        /// May override the current VertexArrayObject!
        virtual void _startLegacyV1Rendering() {}
        virtual void _setRenderOperation( const v1::CbRenderOp *cmd ) = 0;
        /// Renders a V1 RenderOperation. Assumes _setRenderOperation has already been called.
        virtual void _render( const v1::CbDrawCallIndexed *cmd ) = 0;
        virtual void _render( const v1::CbDrawCallStrip *cmd ) = 0;
        virtual void _renderNoBaseInstance( const v1::CbDrawCallIndexed *cmd ) {}
        virtual void _renderNoBaseInstance( const v1::CbDrawCallStrip *cmd ) {}

        /** Gets the capabilities of the render system. */
        const RenderSystemCapabilities *getCapabilities() const { return mCurrentCapabilities; }

        /** Returns the driver version.
         */
        virtual const DriverVersion &getDriverVersion() const { return mDriverVersion; }

        /** Returns the default material scheme used by the render system.
            Systems that use the RTSS to emulate a fixed function pipeline
            (e.g. OpenGL ES 2, GL3+, DX11) need to override this function to return
            the default material scheme of the RTSS ShaderGenerator.

            This is currently only used to set the default material scheme for
            viewports.  It is a necessary step on these render systems for
            render textures to be rendered into properly.
        */
        virtual const String &_getDefaultViewportMaterialScheme() const;

        /** Bind Gpu program parameters.
        @param gptype The type of program to bind the parameters to
        @param params The parameters to bind
        @param variabilityMask A mask of GpuParamVariability identifying which params need binding
        */
        virtual void bindGpuProgramParameters( GpuProgramType                gptype,
                                               GpuProgramParametersSharedPtr params,
                                               uint16                        variabilityMask ) = 0;

        /** Only binds Gpu program parameters used for passes that have more than one iteration rendering
         */
        virtual void bindGpuProgramPassIterationParameters( GpuProgramType gptype ) = 0;

        /** Returns whether or not a Gpu program of the given type is currently bound. */
        virtual bool isGpuProgramBound( GpuProgramType gptype );

        VaoManager *getVaoManager() const { return mVaoManager; }

        TextureGpuManager *getTextureGpuManager() const { return mTextureGpuManager; }

        /**
         * Gets the native shading language version for this render system.
         * Formatted so that it can be used within a shading program.
         * For example, OpenGL 3.2 would return 150, while 3.3 would return 330
         */
        uint16 getNativeShadingLanguageVersion() const { return mNativeShadingLanguageVersion; }

        /** Sets the user clipping region.
         */
        virtual void setClipPlanes( const PlaneList &clipPlanes );

        /** Add a user clipping plane. */
        virtual void addClipPlane( const Plane &p );
        /** Add a user clipping plane. */
        virtual void addClipPlane( Real A, Real B, Real C, Real D );

        /** Clears the user clipping region.
         */
        virtual void resetClipPlanes();

        /** Sets whether or not vertex windings set should be inverted; this can be important
        for rendering reflections. */
        void setInvertVertexWinding( bool invert );

        /** Indicates whether or not the vertex windings set will be inverted for the current render
        (e.g. reflections)
        @see RenderSystem::setInvertVertexWinding
        */
        virtual bool getInvertVertexWinding() const;

        /** Immediately clears the whole frame buffer on the selected RenderPassDescriptor.
            Prefer clearing using the LoadAction semantics in the RenderPassDescriptor.
            This function is provided for two reasons:
                1. Backwards compatibility (i.e. easier porting from 2.1)
                2. Non-tilers desktop GPUs may be faster to clear the whole framebuffer at once.
        @remarks
            Will break an existing RenderPassDescriptor set via beginRenderPassDescriptor.
        @param renderPassDesc
            RenderPassDescriptor filled with LoadActions set to clear and StoreActions
            set to Store.
        */
        virtual void clearFrameBuffer( RenderPassDescriptor *renderPassDesc, TextureGpu *anyTarget,
                                       uint8 mipLevel ) = 0;

        /** Returns the horizontal texel offset value required for mapping
        texel origins to pixel origins in this rendersystem.
        @remarks
        Since rendersystems sometimes disagree on the origin of a texel,
        mapping from texels to pixels can sometimes be problematic to
        implement generically. This method allows you to retrieve the offset
        required to map the origin of a texel to the origin of a pixel in
        the horizontal direction.
        */
        virtual Real getHorizontalTexelOffset() = 0;
        /** Returns the vertical texel offset value required for mapping
        texel origins to pixel origins in this rendersystem.
        @remarks
        Since rendersystems sometimes disagree on the origin of a texel,
        mapping from texels to pixels can sometimes be problematic to
        implement generically. This method allows you to retrieve the offset
        required to map the origin of a texel to the origin of a pixel in
        the vertical direction.
        */
        virtual Real getVerticalTexelOffset() = 0;

        /** Gets the minimum (closest) depth value to be used when rendering
        using identity transforms.
        @remarks
        When using identity transforms you can manually set the depth
        of a vertex; however the input values required differ per
        rendersystem. This method lets you retrieve the correct value.
        @see Renderable::getUseIdentityView, Renderable::getUseIdentityProjection
        */
        virtual Real getMinimumDepthInputValue() = 0;
        /** Gets the maximum (farthest) depth value to be used when rendering
        using identity transforms.
        @remarks
        When using identity transforms you can manually set the depth
        of a vertex; however the input values required differ per
        rendersystem. This method lets you retrieve the correct value.
        @see Renderable::getUseIdentityView, Renderable::getUseIdentityProjection
        */
        virtual Real getMaximumDepthInputValue() = 0;
        /** set the current multi pass count value.  This must be set prior to
        calling _render() if multiple renderings of the same pass state are
        required.
        @param count Number of times to render the current state.
        */
        virtual void setCurrentPassIterationCount( const size_t count )
        {
            mCurrentPassIterationCount = count;
        }

        /** Tell the render system whether to derive a depth bias on its own based on
        the values passed to it in setCurrentPassIterationCount.
        The depth bias set will be baseValue + iteration * multiplier
        @param derive True to tell the RS to derive this automatically
        @param baseValue The base value to which the multiplier should be
        added
        @param multiplier The amount of depth bias to apply per iteration
        @param slopeScale The constant slope scale bias for completeness
        */
        virtual void setDeriveDepthBias( bool derive, float baseValue = 0.0f, float multiplier = 0.0f,
                                         float slopeScale = 0.0f )
        {
            mDerivedDepthBias = derive;
            mDerivedDepthBiasBase = baseValue;
            mDerivedDepthBiasMultiplier = multiplier;
            mDerivedDepthBiasSlopeScale = slopeScale;
        }

        /** Defines a listener on the custom events that this render system
        can raise.
        @see RenderSystem::addListener
        */
        class _OgreExport Listener
        {
        public:
            Listener() {}
            virtual ~Listener();

            /** A rendersystem-specific event occurred.
            @param eventName The name of the event which has occurred
            @param parameters A list of parameters that may belong to this event,
            may be null if there are no parameters
            */
            virtual void eventOccurred( const String            &eventName,
                                        const NameValuePairList *parameters = 0 ) = 0;
        };

        /** Sets shared listener.
        @remarks
        Shared listener could be set even if no render system is selected yet.
        This listener will receive "RenderSystemChanged" event on each Root::setRenderSystem call.
        */
        static void addSharedListener( Listener *listener );
        /** Remove shared listener to the custom events that this render system can raise.
         */
        static void removeSharedListener( Listener *listener );

        static void fireSharedEvent( const String &name, const NameValuePairList *params = 0 );

        /** Adds a listener to the custom events that this render system can raise.
        @remarks
        Some render systems have quite specific, internally generated events
        that the application may wish to be notified of. Many applications
        don't have to worry about these events, and can just trust OGRE to
        handle them, but if you want to know, you can add a listener here.
        @par
        Events are raised very generically by string name. Perhaps the most
        common example of a render system specific event is the loss and
        restoration of a device in DirectX; which OGRE deals with, but you
        may wish to know when it happens.
        @see RenderSystem::getRenderSystemEvents
        */
        virtual void addListener( Listener *l );
        /** Remove a listener to the custom events that this render system can raise.
         */
        virtual void removeListener( Listener *l );

        /** Gets a list of the rendersystem specific events that this rendersystem
        can raise.
        @see RenderSystem::addListener
        */
        virtual const StringVector &getRenderSystemEvents() const { return mEventNames; }

        /** Tell the rendersystem to perform any prep tasks it needs to directly
        before other threads which might access the rendering API are registered.
        @remarks
        Call this from your main thread before starting your other threads
        (which themselves should call registerThread()). Note that if you
        start your own threads, there is a specific startup sequence which
        must be respected and requires synchronisation between the threads:
        <ol>
        <li>[Main thread]Call preExtraThreadsStarted</li>
        <li>[Main thread]Start other thread, wait</li>
        <li>[Other thread]Call registerThread, notify main thread & continue</li>
        <li>[Main thread]Wake up & call postExtraThreadsStarted</li>
        </ol>
        Once this init sequence is completed the threads are independent but
        this startup sequence must be respected.
        */
        virtual void preExtraThreadsStarted() = 0;

        /* Tell the rendersystem to perform any tasks it needs to directly
        after other threads which might access the rendering API are registered.
        @see RenderSystem::preExtraThreadsStarted
        */
        virtual void postExtraThreadsStarted() = 0;

        /** Register the an additional thread which may make calls to rendersystem-related
        objects.
        @remarks
        This method should only be called by additional threads during their
        initialisation. If they intend to use hardware rendering system resources
        they should call this method before doing anything related to the render system.
        Some rendering APIs require a per-thread setup and this method will sort that
        out. It is also necessary to call unregisterThread before the thread shuts down.
        @note
        This method takes no parameters - it must be called from the thread being
        registered and that context is enough.
        */
        virtual void registerThread() = 0;

        /** Unregister an additional thread which may make calls to rendersystem-related objects.
        @see RenderSystem::registerThread
        */
        virtual void unregisterThread() = 0;

        /**
        * Gets the number of display monitors.
        @see Root::getDisplayMonitorCount
        */
        virtual unsigned int getDisplayMonitorCount() const = 0;

        /**
         * This marks the beginning of an event for GPU profiling.
         */
        virtual void beginProfileEvent( const String &eventName ) = 0;

        /**
         * Ends the currently active GPU profiling event.
         */
        virtual void endProfileEvent() = 0;

        /**
         * Marks an instantaneous event for graphics profilers.
         * This is equivalent to calling beginProfileEvent() and endProfileEvent() back to back.
         */
        virtual void markProfileEvent( const String &event ) = 0;

        /// Specifically meant to mark passes in RenderDoc.
        /// See https://renderdoc.org/docs/how/how_annotate_capture.html
        ///
        /// In many cases it will use the same interface as beginProfileEvent / endProfileEvent
        ///
        /// Note: For security & performance reasons this feature is only enabled when
        /// OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        virtual void debugAnnotationPush( const String &event ) {}
        virtual void debugAnnotationPop() {}

        virtual void initGPUProfiling() = 0;
        virtual void deinitGPUProfiling() = 0;
        virtual void beginGPUSampleProfile( const String &name, uint32 *hashCache ) = 0;
        virtual void endGPUSampleProfile( const String &name ) = 0;

        /** Programmatically performs a GPU capture when attached to a GPU debugger like
            RenderDoc or Metal Graphics Debugger

            You must call RenderSystem::endGpuDebuggerFrameCapture

        @remarks
            Very big captures (e.g. lots of API calls between start/endGpuDebuggerFrameCapture)
            may result in the system running out of memory.

        @param window
            Can be NULL for automatic capture.

            See pRENDERDOC_StartFrameCapture's documentation
        @return
            False if we know for certain we failed to capture (e.g. RenderDoc is not running,
            we were build without integration support, etc)

            True if we believe the frame capture started. Note a capture may still
            fail regardless due to other more technical reasons, as rendering APIs are complex.
        */
        virtual bool startGpuDebuggerFrameCapture( Window *window );

        /// See RenderSystem::startGpuDebuggerFrameCapture
        /// Call this function when you're done capturing a frame.
        virtual void endGpuDebuggerFrameCapture( Window *window, const bool bDiscard = false );

        /// Explicitly loads RenderDoc. It is not necessary to call this function
        /// unless you want to use RenderSystem::getRenderDocApi before we
        /// are required to load RenderDoc.
        bool loadRenderDocApi();

        /// Returns the RenderDoc API handle in case you want to do more advanced functionality
        /// than what we expose.
        ///
        /// Note we may return nullptr if RenderDoc was not yet loaded.
        ///
        /// Call RenderSystem::loadRenderDocApi first if you wish the returned ptr
        /// to not be null (it may still be null if we fail to load RenderDoc)
        RENDERDOC_API_1_4_1 *getRenderDocApi() { return mRenderDocApi; }

        /** Determines if the system has anisotropic mip map filter support
         */
        virtual bool hasAnisotropicMipMapFilter() const = 0;

        /** Gets a custom (maybe platform-specific) attribute.
        @remarks This is a nasty way of satisfying any API's need to see platform-specific details.
        @param name The name of the attribute.
        @param pData Pointer to memory of the right kind of structure to receive the info.
        */
        virtual void getCustomAttribute( const String &name, void *pData );

        /**
         * Sets the colour buffer that the render system will to draw. If the render system
         * implementation or configuration does not support a particular value, then false will be
         * returned and the current colour buffer value will not be modified.
         *
         * @param
         *     colourBuffer Specifies the colour buffer that will be drawn into.
         */
        virtual bool setDrawBuffer( ColourBufferType colourBuffer ) { return false; };

        /// Checks for the presense of an API-specific extension (eg. Vulkan, GL)
        virtual bool checkExtension( const String &ext ) const { return false; }

        /** Instructs the RenderSystem to compile shaders without optimizations
            and with debug information, for easier debugging on APIs that support it.
            Default is true if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH, else false

            This setting takes effect for shaders compiled afterwards. Already
            compiled shaders won't change unless you manually rebuild them.

            It is highly recommended you disable the Microcode cache before changing
            the default, or else debug shaders may contaminate your cache, or
            alternatively a shader from the cache may be used which may have been
            compiled with a different setting.
        */
        void setDebugShaders( bool bDebugShaders );
        bool getDebugShaders() const { return mDebugShaders; }

        bool isReverseDepth() const { return mReverseDepth; }

        /// +Y is downwards in NDC (Normalized Device Coordinates). Only Vulkan has this problem.
        bool getInvertedClipSpaceY() const { return mInvertedClipSpaceY; }

        /** Returns true if 'a' and 'b' internally map to the same layout and should be
            considered equivalent for a given texture
        @param bIsDebugCheck
            When true, we're calling this as a consistency check (e.g. asserts if
            layouts changed externally outside the BarrierSolver).
            Non-explicit APIs may return too many false negatives triggering the
            assert, thus this flag prevents false crashes
        */
        virtual bool isSameLayout( ResourceLayout::Layout a, ResourceLayout::Layout b,
                                   const TextureGpu *texture, bool bIsDebugCheck ) const;

        /// On D3D11 calls ClearState followed by Flush().
        /// On GL3+ it calls glFlush
        ///
        /// Do not call this function while inside rendering internals, as it will clear
        /// the device state, thus leaving it inconsistent with what we think it is set to.
        virtual void _clearStateAndFlushCommandBuffer();

        virtual void flushCommands() = 0;

        virtual const PixelFormatToShaderType *getPixelFormatToShaderType() const = 0;

        BarrierSolver &getBarrierSolver() { return mBarrierSolver; }

        /** Loads the pipeline cache from disk.
        @param stream The source stream, optional, starts with struct PipelineCachePrefixHeader
        */
        virtual void loadPipelineCache( DataStreamPtr stream );
        /** Saves the pipeline cache to disk.
        @param stream The destination stream
        */
        virtual void savePipelineCache( DataStreamPtr stream ) const;

        /// When serializing pipeline cache data to the file, we use a header that is filled with
        /// enough information to be able to validate the data, with the pipeline cache data following
        /// immediately afterwards. https://zeux.io/2019/07/17/serializing-pipeline-cache/#fn:1
        struct PipelineCachePrefixHeader
        {
            uint32_t magic;     // an arbitrary magic header to make sure this is actually our file
            uint32_t dataSize;  // equal to *pDataSize returned by vkGetPipelineCacheData
            uint64_t dataHash;  // a hash of pipeline cache data, including the header

            uint32_t vendorID;       // equal to VkPhysicalDeviceProperties::vendorID
            uint32_t deviceID;       // equal to VkPhysicalDeviceProperties::deviceID
            uint32_t driverVersion;  // equal to VkPhysicalDeviceProperties::driverVersion
            uint32_t driverABI;      // equal to sizeof(void*)

            uint8_t uuid[16];  // equal to VkPhysicalDeviceProperties::pipelineCacheUUID
        };

    protected:
        void destroyAllRenderPassDescriptors();

        BarrierSolver mBarrierSolver;

        DepthBufferMap2   mDepthBufferPool2;
        DepthBufferRefMap mSharedDepthBufferRefs;
        TextureGpuSet     mSharedDepthBufferZeroRefCandidates;

        typedef set<RenderPassDescriptor *>::type RenderPassDescriptorSet;

        RenderPassDescriptorSet mRenderPassDescs;
        RenderPassDescriptor   *mCurrentRenderPassDescriptor;
        Viewport                mCurrentRenderViewport[16];
        uint32                  mMaxBoundViewports;

        typedef set<Window *>::type WindowSet;
        WindowSet                   mWindows;

        StencilParams mStencilParams;

        /** The Active GPU programs and gpu program parameters*/
        GpuProgramParametersSharedPtr mActiveVertexGpuProgramParameters;
        GpuProgramParametersSharedPtr mActiveGeometryGpuProgramParameters;
        GpuProgramParametersSharedPtr mActiveFragmentGpuProgramParameters;
        GpuProgramParametersSharedPtr mActiveTessellationHullGpuProgramParameters;
        GpuProgramParametersSharedPtr mActiveTessellationDomainGpuProgramParameters;
        GpuProgramParametersSharedPtr mActiveComputeGpuProgramParameters;

        VaoManager        *mVaoManager;
        TextureGpuManager *mTextureGpuManager;

        bool mDebugShaders;
        bool mWBuffer;

        RenderingMetrics mMetrics;

        /// Saved manual colour blends
        ColourValue mManualBlendColours[OGRE_MAX_TEXTURE_LAYERS][2];

        bool mInvertVertexWinding;

        /// Texture units from this upwards are disabled
        size_t mDisabledTexUnitsFrom;

        /// number of times to render the current state
        size_t mCurrentPassIterationCount;
        size_t mCurrentPassIterationNum;
        /// Whether to update the depth bias per render call
        bool  mDerivedDepthBias;
        float mDerivedDepthBiasBase;
        float mDerivedDepthBiasMultiplier;
        float mDerivedDepthBiasSlopeScale;

        bool                    mUavRenderingDirty;
        uint32                  mUavStartingSlot;
        DescriptorSetUav const *mUavRenderingDescSet;

        /// a global vertex buffer for global instancing
        v1::HardwareVertexBufferSharedPtr mGlobalInstanceVertexBuffer;
        /// a vertex declaration for the global vertex buffer for the global instancing
        v1::VertexDeclaration *mGlobalInstanceVertexBufferVertexDeclaration;
        /// the number of global instances (this number will be multiply by the render op instance
        /// number)
        size_t mGlobalNumberOfInstances;

        /** updates pass iteration rendering state including bound gpu program parameter
        pass iteration auto constant entry
        @return True if more iterations are required
        */
        bool updatePassIterationRenderState();

        RENDERDOC_API_1_4_1 *mRenderDocApi;

        /// List of names of events this rendersystem may raise
        StringVector mEventNames;

        /// Internal method for firing a rendersystem event
        virtual void fireEvent( const String &name, const NameValuePairList *params = 0 );

        typedef list<Listener *>::type ListenerList;

        ListenerList        mEventListeners;
        static ListenerList msSharedEventListeners;

        typedef list<HardwareOcclusionQuery *>::type HardwareOcclusionQueryList;

        HardwareOcclusionQueryList mHwOcclusionQueries;

        bool mVertexProgramBound;
        bool mGeometryProgramBound;
        bool mFragmentProgramBound;
        bool mTessellationHullProgramBound;
        bool mTessellationDomainProgramBound;
        bool mComputeProgramBound;

        // Recording user clip planes
        PlaneList mClipPlanes;
        // Indicator that we need to re-set the clip planes on next render call
        bool mClipPlanesDirty;

        /// Used to store the capabilities of the graphics card
        RenderSystemCapabilities *mRealCapabilities;
        RenderSystemCapabilities *mCurrentCapabilities;
        bool                      mUseCustomCapabilities;

        /// Internal method used to set the underlying clip planes when needed
        virtual void setClipPlanesImpl( const PlaneList &clipPlanes ) = 0;

        /** Initialize the render system from the capabilities*/
        virtual void initialiseFromRenderSystemCapabilities( RenderSystemCapabilities *caps,
                                                             Window                   *primary ) = 0;

        DriverVersion mDriverVersion;
        uint16        mNativeShadingLanguageVersion;

        bool    mTexProjRelative;
        Vector3 mTexProjRelativeOrigin;

        bool mReverseDepth;
        bool mInvertedClipSpaceY;

        uint32 mPsoRequestsTimeout;  // ms, per frame, or 0 to disable
        /// Amount of PSO incomplete requests during this frame.
        uint32 mIncompletePsoRequestsCounterFrame;
    };
    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
