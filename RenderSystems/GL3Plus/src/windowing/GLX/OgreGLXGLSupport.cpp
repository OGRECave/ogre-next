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

#include "OgreException.h"
#include "OgreGL3PlusRenderSystem.h"
#include "OgreLogManager.h"
#include "OgreRoot.h"
#include "OgreStringConverter.h"

#include "OgreGLXGLSupport.h"
#include "OgreGLXUtils.h"
#include "OgreGLXWindow.h"

#include "OgreImage2.h"
#include "OgreTextureBox.h"

#ifndef Status
#    define Status int
#endif

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include <sstream>

static bool ctxErrorOccurred = false;
static int ctxErrorHandler( Display *dpy, XErrorEvent *ev )
{
    ctxErrorOccurred = true;
    return 0;
}

namespace Ogre
{
    //-------------------------------------------------------------------------------------------------//
    template <class C>
    void remove_duplicates( C &c )
    {
        std::sort( c.begin(), c.end() );
        typename C::iterator p = std::unique( c.begin(), c.end() );
        c.erase( p, c.end() );
    }

    //-------------------------------------------------------------------------------------------------//
    GLXGLSupport::GLXGLSupport() : mGLDisplay( 0 ), mXDisplay( 0 )
    {
        // A connection that might be shared with the application for GL rendering:
        mGLDisplay = getGLDisplay();

        // A connection that is NOT shared to enable independent event processing:
        mXDisplay = getXDisplay();

        int dummy;

        if( XQueryExtension( mXDisplay, "RANDR", &dummy, &dummy, &dummy ) )
        {
            XRRScreenConfiguration *screenConfig;

            screenConfig = XRRGetScreenInfo( mXDisplay, DefaultRootWindow( mXDisplay ) );

            if( screenConfig )
            {
                XRRScreenSize *screenSizes;
                int nSizes = 0;
                Rotation currentRotation;
                int currentSizeID = XRRConfigCurrentConfiguration( screenConfig, &currentRotation );

                screenSizes = XRRConfigSizes( screenConfig, &nSizes );

                mCurrentMode.first.first = static_cast<uint>( screenSizes[currentSizeID].width );
                mCurrentMode.first.second = static_cast<uint>( screenSizes[currentSizeID].height );
                mCurrentMode.second = XRRConfigCurrentRate( screenConfig );

                mOriginalMode = mCurrentMode;

                for( int sizeID = 0; sizeID < nSizes; sizeID++ )
                {
                    short *rates;
                    int nRates = 0;

                    rates = XRRConfigRates( screenConfig, sizeID, &nRates );

                    for( int rate = 0; rate < nRates; rate++ )
                    {
                        VideoMode mode;

                        mode.first.first = static_cast<uint>( screenSizes[sizeID].width );
                        mode.first.second = static_cast<uint>( screenSizes[sizeID].height );
                        mode.second = rates[rate];

                        mVideoModes.push_back( mode );
                    }
                }
                XRRFreeScreenConfigInfo( screenConfig );
            }
        }
        else
        {
            mCurrentMode.first.first =
                static_cast<uint>( DisplayWidth( mXDisplay, DefaultScreen( mXDisplay ) ) );
            mCurrentMode.first.second =
                static_cast<uint>( DisplayHeight( mXDisplay, DefaultScreen( mXDisplay ) ) );
            mCurrentMode.second = 0;

            mOriginalMode = mCurrentMode;

            mVideoModes.push_back( mCurrentMode );
        }

        GLXFBConfig *fbConfigs;
        int config, nConfigs = 0;

        fbConfigs = chooseFBConfig( NULL, &nConfigs );

        for( config = 0; config < nConfigs; config++ )
        {
            int caveat, samples;

            getFBConfigAttrib( fbConfigs[config], GLX_CONFIG_CAVEAT, &caveat );

            if( caveat != GLX_SLOW_CONFIG )
            {
                getFBConfigAttrib( fbConfigs[config], GLX_SAMPLES, &samples );
                mSampleLevels.push_back( StringConverter::toString( samples ) );
            }
        }

        XFree( fbConfigs );

        remove_duplicates( mSampleLevels );
    }

    //-------------------------------------------------------------------------------------------------//
    GLXGLSupport::~GLXGLSupport()
    {
        if( mXDisplay )
            XCloseDisplay( mXDisplay );

        if( !mIsExternalDisplay && mGLDisplay )
            XCloseDisplay( mGLDisplay );
    }

    //-------------------------------------------------------------------------------------------------//
    void GLXGLSupport::addConfig()
    {
        ConfigOption optFullScreen;
        ConfigOption optVideoMode;
        ConfigOption optColourDepth;
        ConfigOption optDisplayFrequency;
        ConfigOption optVSync;
        ConfigOption optFSAA;
        ConfigOption optRTTMode;
        ConfigOption optSRGB;
#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        ConfigOption optStereoMode;
#endif

        optFullScreen.name = "Full Screen";
        optFullScreen.immutable = false;

        optVideoMode.name = "Video Mode";
        optVideoMode.immutable = false;

        optDisplayFrequency.name = "Display Frequency";
        optDisplayFrequency.immutable = false;

        optColourDepth.name = "Colour Depth";
        optColourDepth.immutable = false;
        optColourDepth.currentValue.clear();

        optVSync.name = "VSync";
        optVSync.immutable = false;

        optFSAA.name = "FSAA";
        optFSAA.immutable = false;

        optRTTMode.name = "RTT Preferred Mode";
        optRTTMode.immutable = false;

        optSRGB.name = "sRGB Gamma Conversion";
        optSRGB.immutable = false;

        optFullScreen.possibleValues.push_back( "No" );
        optFullScreen.possibleValues.push_back( "Yes" );

        optFullScreen.currentValue = optFullScreen.possibleValues[1];

        VideoModes::const_iterator value = mVideoModes.begin();
        VideoModes::const_iterator end = mVideoModes.end();

        for( ; value != end; value++ )
        {
            String mode = StringConverter::toString( value->first.first, 4 ) + " x " +
                          StringConverter::toString( value->first.second, 4 );

            optVideoMode.possibleValues.push_back( mode );
        }

        remove_duplicates( optVideoMode.possibleValues );

        optVideoMode.currentValue = StringConverter::toString( mCurrentMode.first.first, 4 ) + " x " +
                                    StringConverter::toString( mCurrentMode.first.second, 4 );

        refreshConfig();

        optVSync.possibleValues.push_back( "No" );
        optVSync.possibleValues.push_back( "Yes" );
        optVSync.currentValue = optVSync.possibleValues[0];

        optRTTMode.possibleValues.push_back( "FBO" );
        optRTTMode.currentValue = optRTTMode.possibleValues[0];

        if( !mSampleLevels.empty() )
        {
            StringVector::const_iterator sampleLevel = mSampleLevels.begin();

            for( ; sampleLevel != mSampleLevels.end(); sampleLevel++ )
            {
                optFSAA.possibleValues.push_back( *sampleLevel );
            }

            optFSAA.currentValue = optFSAA.possibleValues[0];
        }

        optSRGB.possibleValues.push_back( "No" );
        optSRGB.possibleValues.push_back( "Yes" );

        optSRGB.currentValue = optSRGB.possibleValues[1];

#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        optStereoMode.name = "Stereo Mode";
        optStereoMode.possibleValues.push_back( StringConverter::toString( SMT_NONE ) );
        optStereoMode.possibleValues.push_back( StringConverter::toString( SMT_FRAME_SEQUENTIAL ) );
        optStereoMode.currentValue = optStereoMode.possibleValues[0];
        optStereoMode.immutable = false;

        mOptions[optStereoMode.name] = optStereoMode;
#endif

        mOptions[optFullScreen.name] = optFullScreen;
        mOptions[optVideoMode.name] = optVideoMode;
        mOptions[optColourDepth.name] = optColourDepth;
        mOptions[optDisplayFrequency.name] = optDisplayFrequency;
        mOptions[optVSync.name] = optVSync;
        mOptions[optRTTMode.name] = optRTTMode;
        mOptions[optFSAA.name] = optFSAA;
        mOptions[optSRGB.name] = optSRGB;

        refreshConfig();
    }

    //-------------------------------------------------------------------------------------------------//
    void GLXGLSupport::refreshConfig()
    {
        ConfigOptionMap::iterator optVideoMode = mOptions.find( "Video Mode" );
        ConfigOptionMap::iterator optDisplayFrequency = mOptions.find( "Display Frequency" );
        ConfigOptionMap::iterator optFullScreen = mOptions.find( "Full Screen" );

        bool isFullscreen = false;
        if( optFullScreen != mOptions.end() && optFullScreen->second.currentValue == "Yes" )
            isFullscreen = true;

        if( optVideoMode != mOptions.end() && optDisplayFrequency != mOptions.end() )
        {
            optDisplayFrequency->second.possibleValues.clear();
            if( !isFullscreen )
            {
                optDisplayFrequency->second.possibleValues.push_back( "N/A" );
            }
            else
            {
                VideoModes::const_iterator value = mVideoModes.begin();
                VideoModes::const_iterator end = mVideoModes.end();

                for( ; value != end; value++ )
                {
                    String mode = StringConverter::toString( value->first.first, 4 ) + " x " +
                                  StringConverter::toString( value->first.second, 4 );

                    if( mode == optVideoMode->second.currentValue && isFullscreen )
                    {
                        String frequency = StringConverter::toString( value->second ) + " Hz";

                        optDisplayFrequency->second.possibleValues.push_back( frequency );
                    }
                }
            }

            if( !optDisplayFrequency->second.possibleValues.empty() )
            {
                optDisplayFrequency->second.currentValue = optDisplayFrequency->second.possibleValues[0];
            }
            else
            {
                if( !mVideoModes.empty() )
                {
                    optVideoMode->second.currentValue =
                        StringConverter::toString( mVideoModes[0].first.first, 4 ) + " x " +
                        StringConverter::toString( mVideoModes[0].first.second, 4 );
                    optDisplayFrequency->second.currentValue =
                        StringConverter::toString( mVideoModes[0].second ) + " Hz";
                }
                else
                {
                    LogManager::getSingleton().logMessage(
                        "Could not find any suitable video mode. Is no monitor connected?" );
                    optVideoMode->second.currentValue = "800 x 600";
                    optDisplayFrequency->second.currentValue = "60 Hz";
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------//
    void GLXGLSupport::setConfigOption( const String &name, const String &value )
    {
        ConfigOptionMap::iterator option = mOptions.find( name );

        if( option == mOptions.end() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Option named " + name + " does not exist.",
                         "GLXGLSupport::setConfigOption" );
        }
        else
        {
            option->second.currentValue = value;
        }

        if( name == "Video Mode" || name == "Full Screen" )
            refreshConfig();
    }

    //-------------------------------------------------------------------------------------------------//
    String GLXGLSupport::validateConfig()
    {
        // TODO
        return BLANKSTRING;
    }

    //-------------------------------------------------------------------------------------------------//
    Window *GLXGLSupport::createWindow( bool autoCreateWindow, GL3PlusRenderSystem *renderSystem,
                                        const String &windowTitle )
    {
        Window *window = 0;

        if( autoCreateWindow )
        {
            ConfigOptionMap::iterator opt;
            ConfigOptionMap::iterator end = mOptions.end();
            NameValuePairList miscParams;

            bool fullscreen = false;
            uint w = 800, h = 600;

            if( ( opt = mOptions.find( "Full Screen" ) ) != end )
                fullscreen = ( opt->second.currentValue == "Yes" );

            if( ( opt = mOptions.find( "Display Frequency" ) ) != end )
                miscParams["displayFrequency"] = opt->second.currentValue;

            if( ( opt = mOptions.find( "Video Mode" ) ) != end )
            {
                String val = opt->second.currentValue;
                String::size_type pos = val.find( 'x' );

                if( pos != String::npos )
                {
                    w = StringConverter::parseUnsignedInt( val.substr( 0, pos ) );
                    h = StringConverter::parseUnsignedInt( val.substr( pos + 1 ) );
                }
            }

            if( ( opt = mOptions.find( "FSAA" ) ) != end )
                miscParams["FSAA"] = opt->second.currentValue;

            if( ( opt = mOptions.find( "VSync" ) ) != end )
                miscParams["vsync"] = opt->second.currentValue;

            if( ( opt = mOptions.find( "sRGB Gamma Conversion" ) ) != end )
                miscParams["gamma"] = opt->second.currentValue;

#if OGRE_NO_QUAD_BUFFER_STEREO == 0
            opt = mOptions.find( "Stereo Mode" );
            if( opt == mOptions.end() )
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Can't find stereo enabled options!",
                             "GLXGLSupport::createWindow" );
            miscParams["stereoMode"] = opt->second.currentValue;
#endif

            window = renderSystem->_createRenderWindow( windowTitle, w, h, fullscreen, &miscParams );
        }

        return window;
    }

    //-------------------------------------------------------------------------------------------------//
    Window *GLXGLSupport::newWindow( const String &name, uint32 width, uint32 height, bool fullscreen,
                                     const NameValuePairList *miscParams )
    {
        GLXWindow *window =
            new GLXWindow( name, width, height, fullscreen, PFG_UNKNOWN, miscParams, this );
        return window;
    }
    //-------------------------------------------------------------------------------------------------//
    void GLXGLSupport::start()
    {
        LogManager::getSingleton().logMessage(
            "******************************\n"
            "*** Starting GLX Subsystem ***\n"
            "******************************" );
    }

    //-------------------------------------------------------------------------------------------------//
    void GLXGLSupport::stop()
    {
        LogManager::getSingleton().logMessage(
            "******************************\n"
            "*** Stopping GLX Subsystem ***\n"
            "******************************" );
    }

    //-------------------------------------------------------------------------------------------------//
    void *GLXGLSupport::getProcAddress( const char *procname ) const
    {
        return (void *)glXGetProcAddressARB( (const GLubyte *)procname );
    }

    //-------------------------------------------------------------------------------------------------//
    void GLXGLSupport::initialiseExtensions()
    {
        assert( mGLDisplay );

        GL3PlusSupport::initialiseExtensions();

        const char *extensionsString;

        // This is more realistic than using glXGetClientString:
        extensionsString = glXQueryExtensionsString( mGLDisplay, DefaultScreen( mGLDisplay ) );

        LogManager::getSingleton().stream() << "Supported GLX extensions: " << extensionsString;

        StringStream ext;
        String instr;

        ext << extensionsString;

        while( ext >> instr )
        {
            extensionList.insert( instr );
        }
    }

    //-------------------------------------------------------------------------------------------------//
    // Returns the FBConfig behind a GLXContext

    GLXFBConfig GLXGLSupport::getFBConfigFromContext( ::GLXContext context )
    {
        GLXFBConfig fbConfig = 0;

        int fbConfigAttrib[] = { GLX_FBCONFIG_ID, 0, None };
        GLXFBConfig *fbConfigs;
        int nElements = 0;

        glXQueryContext( mGLDisplay, context, GLX_FBCONFIG_ID, &fbConfigAttrib[1] );
        fbConfigs =
            glXChooseFBConfig( mGLDisplay, DefaultScreen( mGLDisplay ), fbConfigAttrib, &nElements );

        if( nElements )
        {
            fbConfig = fbConfigs[0];
            XFree( fbConfigs );
        }

        return fbConfig;
    }

    //-------------------------------------------------------------------------------------------------//
    // Returns the FBConfig behind a GLXDrawable, or returns 0 when
    //   missing GLX_SGIX_fbconfig and drawable is Window (unlikely), OR
    //   missing GLX_VERSION_1_3 and drawable is a GLXPixmap (possible).

    GLXFBConfig GLXGLSupport::getFBConfigFromDrawable( GLXDrawable drawable, unsigned int *width,
                                                       unsigned int *height )
    {
        GLXFBConfig fbConfig = 0;

        int fbConfigAttrib[] = { GLX_FBCONFIG_ID, 0, None };
        GLXFBConfig *fbConfigs;
        int nElements = 0;

        glXQueryDrawable( mGLDisplay, drawable, GLX_FBCONFIG_ID, (unsigned int *)&fbConfigAttrib[1] );

        fbConfigs =
            glXChooseFBConfig( mGLDisplay, DefaultScreen( mGLDisplay ), fbConfigAttrib, &nElements );

        if( nElements )
        {
            fbConfig = fbConfigs[0];
            XFree( fbConfigs );

            glXQueryDrawable( mGLDisplay, drawable, GLX_WIDTH, width );
            glXQueryDrawable( mGLDisplay, drawable, GLX_HEIGHT, height );
        }

        if( !fbConfig )
        {
            XWindowAttributes windowAttrib;

            if( XGetWindowAttributes( mGLDisplay, drawable, &windowAttrib ) )
            {
                VisualID visualid = XVisualIDFromVisual( windowAttrib.visual );

                fbConfig = getFBConfigFromVisualID( visualid );

                *width = static_cast<unsigned int>( windowAttrib.width );
                *height = static_cast<unsigned int>( windowAttrib.height );
            }
        }

        return fbConfig;
    }

    //-------------------------------------------------------------------------------------------------//
    // Finds a GLXFBConfig compatible with a given VisualID.

    GLXFBConfig GLXGLSupport::getFBConfigFromVisualID( VisualID visualid )
    {
        GLXFBConfig fbConfig = 0;

        XVisualInfo visualInfo;

        visualInfo.screen = DefaultScreen( mGLDisplay );
        visualInfo.depth = DefaultDepth( mGLDisplay, DefaultScreen( mGLDisplay ) );
        visualInfo.visualid = visualid;

        fbConfig = glXGetFBConfigFromVisualSGIX( mGLDisplay, &visualInfo );

        if( !fbConfig )
        {
            int minAttribs[] = { GLX_DRAWABLE_TYPE,
                                 GLX_WINDOW_BIT | GLX_PIXMAP_BIT,
                                 GLX_RENDER_TYPE,
                                 GLX_RGBA_BIT,
                                 GLX_RED_SIZE,
                                 1,
                                 GLX_BLUE_SIZE,
                                 1,
                                 GLX_GREEN_SIZE,
                                 1,
                                 None };
            int nConfigs = 0;

            GLXFBConfig *fbConfigs = chooseFBConfig( minAttribs, &nConfigs );

            for( int i = 0; i < nConfigs && !fbConfig; i++ )
            {
                XVisualInfo *vInfo = getVisualFromFBConfig( fbConfigs[i] );

                if( vInfo->visualid == visualid )
                    fbConfig = fbConfigs[i];

                XFree( vInfo );
            }

            XFree( fbConfigs );
        }

        return fbConfig;
    }

    //-------------------------------------------------------------------------------------------------//
    // A helper class for the implementation of selectFBConfig

    class FBConfigAttribs
    {
    public:
        FBConfigAttribs( const int *attribs )
        {
            fields[GLX_CONFIG_CAVEAT] = GLX_NONE;

            for( int i = 0; attribs[2 * i]; i++ )
            {
                fields[attribs[2 * i]] = attribs[2 * i + 1];
            }
        }

        void load( GLXGLSupport *const glSupport, GLXFBConfig fbConfig )
        {
            std::map<int, int>::iterator it;

            for( it = fields.begin(); it != fields.end(); it++ )
            {
                it->second = 0;

                glSupport->getFBConfigAttrib( fbConfig, it->first, &it->second );
            }
        }

        bool operator>( FBConfigAttribs &alternative )
        {
            // Caveats are best avoided, but might be needed for anti-aliasing

            if( fields[GLX_CONFIG_CAVEAT] != alternative.fields[GLX_CONFIG_CAVEAT] )
            {
                if( fields[GLX_CONFIG_CAVEAT] == GLX_SLOW_CONFIG )
                    return false;

                if( fields.find( GLX_SAMPLES ) != fields.end() &&
                    fields[GLX_SAMPLES] < alternative.fields[GLX_SAMPLES] )
                    return false;
            }

            std::map<int, int>::iterator it;

            for( it = fields.begin(); it != fields.end(); it++ )
            {
                if( it->first != GLX_CONFIG_CAVEAT && fields[it->first] > alternative.fields[it->first] )
                    return true;
            }

            return false;
        }

        std::map<int, int> fields;
    };

    //-------------------------------------------------------------------------------------------------//
    // Finds an FBConfig that possesses each of minAttribs and gets as close
    // as possible to each of the maxAttribs without exceeding them.
    // Resembles glXChooseFBConfig, but is forgiving to platforms
    // that do not support the attributes listed in the maxAttribs.

    GLXFBConfig GLXGLSupport::selectFBConfig( const int *minAttribs, const int *maxAttribs )
    {
        GLXFBConfig *fbConfigs;
        GLXFBConfig fbConfig = 0;
        int config, nConfigs = 0;

        fbConfigs = chooseFBConfig( minAttribs, &nConfigs );

        // this is a fix for cases where chooseFBConfig is not supported.
        // On the 10/2010 chooseFBConfig was not supported on VirtualBox
        // http://www.virtualbox.org/ticket/7195
        if( !nConfigs )
        {
            fbConfigs = glXGetFBConfigs( mGLDisplay, DefaultScreen( mGLDisplay ), &nConfigs );
        }

        if( !nConfigs )
            return 0;

        fbConfig = fbConfigs[0];

        if( maxAttribs )
        {
            FBConfigAttribs maximum( maxAttribs );
            FBConfigAttribs best( maxAttribs );
            FBConfigAttribs candidate( maxAttribs );

            best.load( this, fbConfig );

            for( config = 1; config < nConfigs; config++ )
            {
                candidate.load( this, fbConfigs[config] );

                if( candidate > maximum )
                    continue;

                if( candidate > best )
                {
                    fbConfig = fbConfigs[config];

                    best.load( this, fbConfig );
                }
            }
        }

        XFree( fbConfigs );
        return fbConfig;
    }

    //-------------------------------------------------------------------------------------------------//
    bool GLXGLSupport::loadIcon( const String &name, Pixmap *pixmap, Pixmap *bitmap )
    {
        Image2 image;
        uint32 width, height;
        char *imageData;

        if( !Ogre::ResourceGroupManager::getSingleton().resourceExists(
                ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, name ) )
        {
            return false;
        }

        try
        {
            // Try to load image
            image.load( name, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

            if( image.getPixelFormat() != PFG_RGBA8_UNORM )
            {
                // Image format must be RGBA
                return false;
            }

            width = image.getWidth();
            height = image.getHeight();
            imageData = (char *)image.getData( 0 ).data;
        }
        catch( Exception &e )
        {
            // Could not find image; never mind
            return false;
        }

        uint32 bitmapLineLength = ( width + 7 ) / 8;
        uint32 pixmapLineLength = 4 * width;

        char *bitmapData = (char *)malloc( bitmapLineLength * height );
        char *pixmapData = (char *)malloc( pixmapLineLength * height );

        int sptr = 0, dptr = 0;

        for( uint32 y = 0; y < height; y++ )
        {
            for( uint32 x = 0; x < width; x++ )
            {
                if( ImageByteOrder( mXDisplay ) == MSBFirst )
                {
                    pixmapData[dptr + 0] = 0;
                    pixmapData[dptr + 1] = imageData[sptr + 0];
                    pixmapData[dptr + 2] = imageData[sptr + 1];
                    pixmapData[dptr + 3] = imageData[sptr + 2];
                }
                else
                {
                    pixmapData[dptr + 3] = 0;
                    pixmapData[dptr + 2] = imageData[sptr + 0];
                    pixmapData[dptr + 1] = imageData[sptr + 1];
                    pixmapData[dptr + 0] = imageData[sptr + 2];
                }

                if( ( (unsigned char)imageData[sptr + 3] ) < 128 )
                {
                    bitmapData[y * bitmapLineLength + ( x >> 3 )] &= ~( 1 << ( x & 7 ) );
                }
                else
                {
                    bitmapData[y * bitmapLineLength + ( x >> 3 )] |= 1 << ( x & 7 );
                }
                sptr += 4;
                dptr += 4;
            }
        }

        // Create bitmap on server and copy over bitmapData
        *bitmap = XCreateBitmapFromData( mXDisplay, DefaultRootWindow( mXDisplay ), bitmapData, width,
                                         height );

        free( bitmapData );

        // Create pixmap on server and copy over pixmapData (via pixmapXImage)
        *pixmap = XCreatePixmap( mXDisplay, DefaultRootWindow( mXDisplay ), width, height, 24 );

        GC gc = XCreateGC( mXDisplay, DefaultRootWindow( mXDisplay ), 0, NULL );
        XImage *pixmapXImage = XCreateImage( mXDisplay, NULL, 24, ZPixmap, 0, pixmapData, width, height,
                                             8, static_cast<int>( width * 4 ) );
        XPutImage( mXDisplay, *pixmap, gc, pixmapXImage, 0, 0, 0, 0, width, height );
        XDestroyImage( pixmapXImage );
        XFreeGC( mXDisplay, gc );

        return true;
    }

    //-------------------------------------------------------------------------------------------------//
    Display *GLXGLSupport::getGLDisplay()
    {
        if( !mGLDisplay )
        {
            //                  glXGetCurrentDisplay =
            //                  (PFNGLXGETCURRENTDISPLAYPROC)getProcAddress("glXGetCurrentDisplay");

            mGLDisplay = glXGetCurrentDisplay();
            mIsExternalDisplay = true;

            if( !mGLDisplay )
            {
                mGLDisplay = XOpenDisplay( 0 );
                mIsExternalDisplay = false;
            }

            if( !mGLDisplay )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "Couldn`t open X display " + String( (const char *)XDisplayName( 0 ) ),
                             "GLXGLSupport::getGLDisplay" );
            }
        }

        return mGLDisplay;
    }

    //-------------------------------------------------------------------------------------------------//
    Display *GLXGLSupport::getXDisplay()
    {
        if( !mXDisplay )
        {
            char *displayString = mGLDisplay ? DisplayString( mGLDisplay ) : 0;

            mXDisplay = XOpenDisplay( displayString );

            if( !mXDisplay )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "Couldn`t open X display " + String( (const char *)displayString ),
                             "GLXGLSupport::getXDisplay" );
            }

            mAtomDeleteWindow = XInternAtom( mXDisplay, "WM_DELETE_WINDOW", True );
            mAtomFullScreen = XInternAtom( mXDisplay, "_NET_WM_STATE_FULLSCREEN", True );
            mAtomState = XInternAtom( mXDisplay, "_NET_WM_STATE", True );
        }

        return mXDisplay;
    }

    //-------------------------------------------------------------------------------------------------//
    String GLXGLSupport::getDisplayName()
    {
        return String( (const char *)XDisplayName( DisplayString( mGLDisplay ) ) );
    }

    //-------------------------------------------------------------------------------------------------//
    GLXFBConfig *GLXGLSupport::chooseFBConfig( const GLint *attribList, GLint *nElements )
    {
        GLXFBConfig *fbConfigs;

        fbConfigs = glXChooseFBConfig( mGLDisplay, DefaultScreen( mGLDisplay ), attribList, nElements );

        return fbConfigs;
    }

    //-------------------------------------------------------------------------------------------------//
    ::GLXContext GLXGLSupport::createNewContext( GLXFBConfig fbConfig, GLint renderType,
                                                 ::GLXContext shareList, GLboolean direct ) const
    {
        ::GLXContext glxContext = NULL;
        int context_attribs[] = {
            GLX_CONTEXT_MAJOR_VERSION_ARB,
            4,
            GLX_CONTEXT_MINOR_VERSION_ARB,
            5,
            GLX_CONTEXT_PROFILE_MASK_ARB,
            GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
#if OGRE_DEBUG_MODE
            GLX_CONTEXT_FLAGS_ARB,
            GLX_CONTEXT_DEBUG_BIT_ARB,
#endif
            None
        };

        ctxErrorOccurred = false;
        int ( *oldHandler )( Display *, XErrorEvent * ) = XSetErrorHandler( &ctxErrorHandler );

        PFNGLXCREATECONTEXTATTRIBSARBPROC _glXCreateContextAttribsARB;
        _glXCreateContextAttribsARB =
            ( PFNGLXCREATECONTEXTATTRIBSARBPROC ) const_cast<GLXGLSupport *>( this )->getProcAddress(
                "glXCreateContextAttribsARB" );

        while( !glxContext && ( context_attribs[1] >= 3 ) )
        {
            ctxErrorOccurred = false;
            glxContext =
                _glXCreateContextAttribsARB( mGLDisplay, fbConfig, shareList, direct, context_attribs );
            // Sync to ensure any errors generated are processed.
            XSync( mGLDisplay, False );
            if( !ctxErrorOccurred && glxContext )
            {
                LogManager::getSingleton().logMessage(
                    "Created GL " + StringConverter::toString( context_attribs[1] ) + "." +
                    StringConverter::toString( context_attribs[3] ) + " context" );
            }
            else
            {
                if( context_attribs[3] == 0 )
                {
                    context_attribs[1] -= 1;
                    context_attribs[3] = 5;
                }
                else
                {
                    context_attribs[3] -= 1;
                }
            }
        }
        // Sync to ensure any errors generated are processed.
        XSync( mGLDisplay, False );

        // Restore the original error handler
        XSetErrorHandler( oldHandler );

        if( ctxErrorOccurred || !glxContext )
            LogManager::getSingleton().logMessage( "Failed to create an OpenGL 3+ context",
                                                   LML_CRITICAL );

        return glxContext;
    }

    //-------------------------------------------------------------------------------------------------//
    GLint GLXGLSupport::getFBConfigAttrib( GLXFBConfig fbConfig, GLint attribute, GLint *value )
    {
        GLint status;

        status = glXGetFBConfigAttrib( mGLDisplay, fbConfig, attribute, value );

        return status;
    }

    //-------------------------------------------------------------------------------------------------//
    XVisualInfo *GLXGLSupport::getVisualFromFBConfig( GLXFBConfig fbConfig )
    {
        XVisualInfo *visualInfo;

        visualInfo = glXGetVisualFromFBConfig( mGLDisplay, fbConfig );

        return visualInfo;
    }

    //-------------------------------------------------------------------------------------------------//
    void GLXGLSupport::switchMode( uint32 width, uint32 height, short frequency )
    {
        int size = 0;
        int newSize = -1;

        VideoModes::iterator mode;
        VideoModes::iterator end = mVideoModes.end();
        VideoMode *newMode = 0;

        for( mode = mVideoModes.begin(); mode != end; size++ )
        {
            if( mode->first.first >= width && mode->first.second >= height )
            {
                if( !newMode || mode->first.first < newMode->first.first ||
                    mode->first.second < newMode->first.second )
                {
                    newSize = size;
                    newMode = &( *mode );
                }
            }

            VideoMode *lastMode = &( *mode );

            while( ++mode != end && mode->first == lastMode->first )
            {
                if( lastMode == newMode && mode->second == frequency )
                {
                    newMode = &( *mode );
                }
            }
        }

        if( newMode && *newMode != mCurrentMode )
        {
            XRRScreenConfiguration *screenConfig =
                XRRGetScreenInfo( mXDisplay, DefaultRootWindow( mXDisplay ) );

            if( screenConfig )
            {
                Rotation currentRotation;

                XRRConfigCurrentConfiguration( screenConfig, &currentRotation );

                XRRSetScreenConfigAndRate( mXDisplay, screenConfig, DefaultRootWindow( mXDisplay ),
                                           newSize, currentRotation, newMode->second, CurrentTime );

                XRRFreeScreenConfigInfo( screenConfig );

                mCurrentMode = *newMode;

                LogManager::getSingleton().logMessage(
                    "Entered video mode " + StringConverter::toString( mCurrentMode.first.first ) + "x" +
                    StringConverter::toString( mCurrentMode.first.second ) + " @ " +
                    StringConverter::toString( mCurrentMode.second ) + "Hz" );
            }
        }
    }

    //-------------------------------------------------------------------------------------------------//
    void GLXGLSupport::switchMode()
    {
        return switchMode( mOriginalMode.first.first, mOriginalMode.first.second, mOriginalMode.second );
    }
}  // namespace Ogre
