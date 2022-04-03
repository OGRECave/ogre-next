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
#include "OgreConfigDialog.h"
#include "OgreException.h"
#include "OgreImage2.h"
#include "OgreLogManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreTextureBox.h"

#include <cstdlib>
#include <iostream>

#include <string>

#define XTSTRINGDEFINES

#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <X11/X.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <X11/keysym.h>

#include <list>

namespace
{
/**
Backdrop image. This must be sized mWidth by mHeight, and produce a
RGB pixel format when loaded with Image::load .

You can easily generate your own backdrop with the following python script:

#!/usr/bin/python
import sys
pngstring=open(sys.argv[2], "rb").read()
print "const unsigned char %s[%i]={%s};" % (sys.argv[1],len(pngstring), ",".join([str(ord(x)) for x in
pngstring]))

Call this with
$ bintoheader.py GLX_backdrop_data GLX_backdrop.png > GLX_backdrop.h

*/
#include "GLX_backdrop.h"

}  // namespace

namespace Ogre
{
    /**
     * Single X window with image backdrop, making it possible to configure
     * OGRE in a graphical way.
     * XaW uses a not-very-smart widget positioning system, so I override it to use
     * fixed positions. This works great, but it means you need to define the various
     * positions manually.
     * Furthermore, it has no OptionMenu by default, so I simulate this with dropdown
     * buttons.
     */
    class GLXConfigurator
    {
        /* GUI constants */
        static const int wWidth = 600;   // Width of window
        static const int wHeight = 380;  // Height of window
        static const int col1x = 20;     // Starting x of column 1 (labels)
        static const int col2x = 220;    // Starting x of column 2 (options)
        static const int col1w = 180;    // Width of column 1 (labels)
        static const int col2w = 360;    // Width of column 2 (options)
        static const int ystart = 105;   // Starting y of option table rows
        static const int rowh = 20;      // Height of one row in the option table

    public:
        GLXConfigurator();
        virtual ~GLXConfigurator();

        bool CreateWindow();
        void Main();
        /**
         * Exit from main loop.
         */
        void Exit();

    protected:
        Display *mDisplay;
        ::Window mWindow;
        Pixmap mBackDrop;

        int mWidth, mHeight;
        // Xt
        XtAppContext appContext;
        Widget toplevel;

        /**
         * Create backdrop image, and return it as a Pixmap.
         */
        virtual Pixmap CreateBackdrop( ::Window rootWindow, uint32 depth );
        /**
         * Called after window initialisation.
         */
        virtual bool Init();
        /**
         * Called initially, and on expose.
         */
        virtual void Draw();

        void findMonitorIndexFromMouseCursor( ::Window rootWindow, int screen, int &outCenterW,
                                              int &outCenterH );

    public:
        /* Local */
        bool accept;
        /* Class that binds a callback to a RenderSystem */
        class RendererCallbackData
        {
        public:
            RendererCallbackData( GLXConfigurator *parent_, RenderSystem *renderer_,
                                  Widget optionmenu_ ) :
                parent( parent_ ),
                renderer( renderer_ ),
                optionmenu( optionmenu_ )
            {
            }
            GLXConfigurator *parent;
            RenderSystem *renderer;
            Widget optionmenu;
        };
        std::list<RendererCallbackData> mRendererCallbackData;

        RenderSystem *mRenderer;
        Widget box;                              // Box'o control widgets
        std::list<Widget> mRenderOptionWidgets;  // List of RenderSystem specific
                                                 // widgets for visibility management (cleared when
                                                 // another rendersystem is selected)
        /* Class that binds a callback to a certain configuration option/value */
        class ConfigCallbackData
        {
        public:
            ConfigCallbackData( GLXConfigurator *parent_, const String &optionName_,
                                const String &valueName_, Widget optionmenu_ ) :
                parent( parent_ ),
                optionName( optionName_ ),
                valueName( valueName_ ),
                optionmenu( optionmenu_ )
            {
            }
            GLXConfigurator *parent;
            String optionName, valueName;
            Widget optionmenu;
        };
        std::list<ConfigCallbackData> mConfigCallbackData;

        void SetRenderSystem( RenderSystem *sys ) { mRenderer = sys; }

    private:
        /* Callbacks that terminate modal dialog loop */
        static void acceptHandler( Widget w, GLXConfigurator *obj, XtPointer callData )
        {
            // Check if a renderer was selected, if not, don't accept
            if( !obj->mRenderer )
                return;
            obj->accept = true;
            obj->Exit();
        }
        static void cancelHandler( Widget w, GLXConfigurator *obj, XtPointer callData ) { obj->Exit(); }
        /* Callbacks that set a setting */
        static void renderSystemHandler( Widget w, RendererCallbackData *cdata, XtPointer callData )
        {
            // Set selected renderer its name
            XtVaSetValues( cdata->optionmenu, XtNlabel, cdata->renderer->getName().c_str(), 0, NULL );
            // Notify Configurator (and Ogre)
            cdata->parent->SetRenderer( cdata->renderer );
        }
        static void configOptionHandler( Widget w, ConfigCallbackData *cdata, XtPointer callData )
        {
            // Set selected renderer its name
            XtVaSetValues( cdata->optionmenu, XtNlabel, cdata->valueName.c_str(), 0, NULL );
            // Notify Configurator (and Ogre)
            cdata->parent->SetConfigOption( cdata->optionName, cdata->valueName );
        }

        /* Functions reacting to GUI */
        void SetRenderer( RenderSystem * );
        void SetConfigOption( const String &optionName, const String &valueName );
    };

    GLXConfigurator::GLXConfigurator() :
        mDisplay( 0 ),
        mWindow( 0 ),
        mBackDrop( 0 ),
        mWidth( wWidth ),
        mHeight( wHeight ),
        appContext( 0 ),
        toplevel( 0 ),

        accept( false ),
        mRenderer( 0 )
    {
    }
    GLXConfigurator::~GLXConfigurator()
    {
        if( mBackDrop )
            XFreePixmap( mDisplay, mBackDrop );
        if( toplevel )
        {
            XtUnrealizeWidget( toplevel );
            XtDestroyWidget( toplevel );
        }
        if( mDisplay )
        {
            XCloseDisplay( mDisplay );
        }
    }

    void GLXConfigurator::findMonitorIndexFromMouseCursor( ::Window rootWindow, int screen,
                                                           int &outCenterW, int &outCenterH )
    {
        int dummy = 0;
        if( XQueryExtension( mDisplay, "RANDR", &dummy, &dummy, &dummy ) )
        {
            XRRScreenResources *screenXRR = XRRGetScreenResources( mDisplay, rootWindow );

            ::Window mouseRootWindow;
            ::Window mouseRootWindowChild;
            int unused;
            unsigned int mask;
            int mouseX, mouseY;
            Bool result = XQueryPointer( mDisplay, rootWindow, &mouseRootWindow, &mouseRootWindowChild,
                                         &mouseX, &mouseY, &unused, &unused, &mask );
            if( result == True )
            {
                for( int i = 0; i < screenXRR->ncrtc; ++i )
                {
                    XRRCrtcInfo *crtcInfo = XRRGetCrtcInfo( mDisplay, screenXRR, screenXRR->crtcs[i] );
                    if( crtcInfo->x <= mouseX && mouseX < crtcInfo->x + (int)( crtcInfo->width ) &&
                        crtcInfo->y <= mouseY && mouseY < crtcInfo->y + (int)( crtcInfo->height ) )
                    {
                        outCenterW = crtcInfo->x + static_cast<int>( crtcInfo->width / 2u );
                        outCenterH = crtcInfo->y + static_cast<int>( crtcInfo->height / 2u );
                        XRRFreeCrtcInfo( crtcInfo );
                        XRRFreeScreenResources( screenXRR );
                        return;
                    }

                    XRRFreeCrtcInfo( crtcInfo );
                }
            }

            XRRFreeScreenResources( screenXRR );
        }

        // Couldn't find. Probably remote display.
        outCenterW = DisplayWidth( mDisplay, screen ) / 2;
        outCenterH = DisplayHeight( mDisplay, screen ) / 2;
    }

    bool GLXConfigurator::CreateWindow()
    {
        const char *bla[] = { "Rendering Settings", "-bg", "honeydew3", "-fg", "black", "-bd",
                              "darkseagreen4" };
        int argc = sizeof( bla ) / sizeof( *bla );

        toplevel = XtVaOpenApplication( &appContext, "OGRE", NULL, 0, &argc, const_cast<char **>( bla ),
                                        NULL, sessionShellWidgetClass, XtNwidth, mWidth, XtNheight,
                                        mHeight, XtNminWidth, mWidth, XtNmaxWidth, mWidth, XtNminHeight,
                                        mHeight, XtNmaxHeight, mHeight, XtNallowShellResize, False,
                                        XtNborderWidth, 0, XtNoverrideRedirect, False, NULL, NULL );

        /* Find out display and screen used */
        mDisplay = XtDisplay( toplevel );
        int screen = DefaultScreen( mDisplay );
        ::Window rootWindow = RootWindow( mDisplay, screen );

        /* Move to center of display */
        int centerX = 0, centerY = 0;
        findMonitorIndexFromMouseCursor( rootWindow, screen, centerX, centerY );
        XtVaSetValues( toplevel, XtNx, centerX - mWidth / 2, XtNy, centerY - mHeight / 2, 0, NULL );

        /* Backdrop stuff */
        mBackDrop =
            CreateBackdrop( rootWindow, static_cast<uint32>( DefaultDepth( mDisplay, screen ) ) );

        /* Create toplevel */
        box = XtVaCreateManagedWidget( "box", formWidgetClass, toplevel, XtNbackgroundPixmap, mBackDrop,
                                       0, NULL );

        /* Create renderer selection */
        int cury = ystart + 0 * rowh;

        XtVaCreateManagedWidget( "topLabel", labelWidgetClass, box, XtNlabel, "Select Renderer",
                                 XtNborderWidth, 0, XtNwidth, col1w,  // Fixed width
                                 XtNheight, 18, XtNleft, XawChainLeft, XtNtop, XawChainTop, XtNright,
                                 XawChainLeft, XtNbottom, XawChainTop, XtNhorizDistance, col1x,
                                 XtNvertDistance, cury, XtNjustify, XtJustifyLeft, NULL );

        const char *curRenderName = " Select One ";  // Name of current renderer, or hint to select one
        if( mRenderer )
            curRenderName = mRenderer->getName().c_str();
        Widget mb1 = XtVaCreateManagedWidget(
            "Menu", menuButtonWidgetClass, box, XtNlabel, curRenderName, XtNresize, false, XtNresizable,
            false, XtNwidth, col2w,  // Fixed width
            XtNheight, 18, XtNleft, XawChainLeft, XtNtop, XawChainTop, XtNright, XawChainLeft, XtNbottom,
            XawChainTop, XtNhorizDistance, col2x, XtNvertDistance, cury, NULL );

        Widget menu = XtVaCreatePopupShell( "menu", simpleMenuWidgetClass, mb1, 0, NULL );

        const RenderSystemList &renderers = Root::getSingleton().getAvailableRenderers();
        for( RenderSystemList::const_iterator pRend = renderers.begin(); pRend != renderers.end();
             pRend++ )
        {
            // Create callback data
            mRendererCallbackData.push_back( RendererCallbackData( this, *pRend, mb1 ) );

            Widget entry = XtVaCreateManagedWidget( "menuentry", smeBSBObjectClass, menu, XtNlabel,
                                                    ( *pRend )->getName().c_str(), 0, NULL );
            XtAddCallback( entry, XtNcallback, (XtCallbackProc)&GLXConfigurator::renderSystemHandler,
                           &mRendererCallbackData.back() );
        }

        Widget bottomPanel = XtVaCreateManagedWidget(
            "bottomPanel", formWidgetClass, box, XtNsensitive, True, XtNborderWidth, 0, XtNwidth,
            150,  // Fixed width
            XtNleft, XawChainLeft, XtNtop, XawChainTop, XtNright, XawChainLeft, XtNbottom, XawChainTop,
            XtNhorizDistance, mWidth - 160, XtNvertDistance, mHeight - 40, NULL );

        Widget helloButton = XtVaCreateManagedWidget( "cancelButton", commandWidgetClass, bottomPanel,
                                                      XtNlabel, " Cancel ", NULL );
        XtAddCallback( helloButton, XtNcallback, (XtCallbackProc)&GLXConfigurator::cancelHandler, this );

        Widget exitButton =
            XtVaCreateManagedWidget( "acceptButton", commandWidgetClass, bottomPanel, XtNlabel,
                                     " Accept ", XtNfromHoriz, helloButton, NULL );
        XtAddCallback( exitButton, XtNcallback, (XtCallbackProc)&GLXConfigurator::acceptHandler, this );

        XtRealizeWidget( toplevel );

        if( mRenderer )
            /* There was already a renderer selected; display its options */
            SetRenderer( mRenderer );

        return true;
    }

    Pixmap GLXConfigurator::CreateBackdrop( ::Window rootWindow, uint32 depth )
    {
        uint32 bpl;
        /* Find out number of bytes per pixel */
        switch( depth )
        {
        default:
            LogManager::getSingleton().logMessage( "GLX backdrop: Unsupported bit depth" );
            /* Unsupported bit depth */
            return 0;
        case 15:
        case 16:
            bpl = 2;
            break;
        case 24:
        case 32:
            bpl = 4;
            break;
        }
        /* Create background pixmap */
        unsigned char *data = 0;  // Must be allocated with malloc

        const uint32 uWidth = static_cast<uint32>( mWidth );
        const uint32 uHeight = static_cast<uint32>( mHeight );

        try
        {
            String imgType = "png";
            Image2 img;
            MemoryDataStream *imgStream;
            DataStreamPtr imgStreamPtr;

            // Load backdrop image using OGRE
            imgStream = new MemoryDataStream( const_cast<unsigned char *>( GLX_backdrop_data ),
                                              sizeof( GLX_backdrop_data ), false );
            imgStreamPtr = DataStreamPtr( imgStream );
            img.load( imgStreamPtr, imgType );

            TextureBox srcBox = img.getData( 0 );

            // Center the logo in the X axis
            const PixelFormatGpu intermediateFormat = PFG_BGRA8_UNORM;
            TextureBox dstBox(
                srcBox.width, srcBox.height, 1u, 1u,
                PixelFormatGpuUtils::getBytesPerPixel( intermediateFormat ),
                (uint32)PixelFormatGpuUtils::getSizeBytes( uWidth, 1u, 1u, 1u, intermediateFormat, 1u ),
                PixelFormatGpuUtils::getSizeBytes( uWidth, uHeight, 1u, 1u, intermediateFormat, 1u ) );
            dstBox.x = ( uWidth - srcBox.width ) >> 1u;
            dstBox.y = 12u;
            dstBox.data = (unsigned char *)malloc( uWidth * uHeight * 4u );
            memset( dstBox.data, 0, uWidth * uHeight * 4u );

            PixelFormatGpuUtils::bulkPixelConversion( srcBox, img.getPixelFormat(), dstBox,
                                                      intermediateFormat );

            dstBox.x = 0u;
            dstBox.y = 0u;
            dstBox.width = uWidth;
            dstBox.height = uHeight;

            // Blend a gradient into the final image (and use alpha blending)
            data = (unsigned char *)malloc( uWidth * uHeight * bpl );  // Must be allocated with malloc
            const PixelFormatGpu dstFormat = bpl == 2 ? PFG_B5G6R5_UNORM : PFG_BGRA8_UNORM;
            TextureBox finalBox(
                uWidth, uHeight, 1u, 1u, PixelFormatGpuUtils::getBytesPerPixel( dstFormat ),
                (uint32)PixelFormatGpuUtils::getSizeBytes( uWidth, 1u, 1u, 1u, dstFormat, 1u ),
                PixelFormatGpuUtils::getSizeBytes( uWidth, uHeight, 1u, 1u, dstFormat, 1u ) );
            finalBox.data = data;

            const size_t height = (size_t)mHeight;
            const size_t width = (size_t)mWidth;
            for( size_t y = 0u; y < height; ++y )
            {
                const float yGradient = sqrtf( y / (float)height );
                for( size_t x = 0u; x < width; ++x )
                {
                    ColourValue colour = dstBox.getColourAt( x, y, 0u, intermediateFormat );
                    colour =
                        Math::lerp( ColourValue( 0.40f, 0.439f, 0.345f ) * yGradient, colour, colour.a );
                    finalBox.setColourAt( colour, x, y, 0u, dstFormat );
                }
            }

            free( dstBox.data );
        }
        catch( Exception &e )
        {
            // Could not find image; never mind
            LogManager::getSingleton().logMessage(
                "WARNING: Can not load backdrop for config dialog. " + e.getDescription(), LML_TRIVIAL );
            return 0;
        }

        GC context = XCreateGC( mDisplay, rootWindow, 0, NULL );

        /* put my pixmap data into the client side X image data structure */
        XImage *image = XCreateImage( mDisplay, NULL, depth, ZPixmap, 0, (char *)data, uWidth, uHeight,
                                      8, mWidth * static_cast<int>( bpl ) );
#if OGRE_ENDIAN == OGRE_ENDIAN_BIG
        image->byte_order = MSBFirst;
#else
        image->byte_order = LSBFirst;
#endif

        /* tell server to start managing my pixmap */
        Pixmap rv = XCreatePixmap( mDisplay, rootWindow, uWidth, uHeight, depth );

        /* copy from client to server */
        XPutImage( mDisplay, rv, context, image, 0, 0, 0, 0, uWidth, uHeight );

        /* free up the client side pixmap data area */
        XDestroyImage( image );  // also cleans data
        XFreeGC( mDisplay, context );

        return rv;
    }
    bool GLXConfigurator::Init()
    {
        // Init misc resources
        return true;
    }
    void GLXConfigurator::Draw() {}
    void GLXConfigurator::Main() { XtAppMainLoop( appContext ); }
    void GLXConfigurator::Exit() { XtAppSetExitFlag( appContext ); }

    void GLXConfigurator::SetRenderer( RenderSystem *r )
    {
        mRenderer = r;

        // Destroy each widget of GUI of previously selected renderer
        for( std::list<Widget>::iterator i = mRenderOptionWidgets.begin();
             i != mRenderOptionWidgets.end(); i++ )
            XtDestroyWidget( *i );
        mRenderOptionWidgets.clear();
        mConfigCallbackData.clear();

        // Create option GUI
        int cury = ystart + 1 * rowh + 10;

        ConfigOptionMap options = mRenderer->getConfigOptions();
        // Process each option and create an optionmenu widget for it
        for( ConfigOptionMap::iterator it = options.begin(); it != options.end(); it++ )
        {
            // if the config option does not have any possible value, then skip it.
            // if we create a popup with zero entries, it will crash when you click
            // on it.
            if( it->second.possibleValues.empty() )
                continue;

            Widget lb1 = XtVaCreateManagedWidget(
                "topLabel", labelWidgetClass, box, XtNlabel, it->second.name.c_str(), XtNborderWidth, 0,
                XtNwidth, col1w,  // Fixed width
                XtNheight, 18, XtNleft, XawChainLeft, XtNtop, XawChainTop, XtNright, XawChainLeft,
                XtNbottom, XawChainTop, XtNhorizDistance, col1x, XtNvertDistance, cury, XtNjustify,
                XtJustifyLeft, NULL );
            mRenderOptionWidgets.push_back( lb1 );
            Widget mb1 = XtVaCreateManagedWidget(
                "Menu", menuButtonWidgetClass, box, XtNlabel, it->second.currentValue.c_str(), XtNresize,
                false, XtNresizable, false, XtNwidth, col2w,  // Fixed width
                XtNheight, 18, XtNleft, XawChainLeft, XtNtop, XawChainTop, XtNright, XawChainLeft,
                XtNbottom, XawChainTop, XtNhorizDistance, col2x, XtNvertDistance, cury, NULL );
            mRenderOptionWidgets.push_back( mb1 );

            Widget menu = XtVaCreatePopupShell( "menu", simpleMenuWidgetClass, mb1, 0, NULL );

            // Process each choice
            StringVector::iterator opt_it;
            for( opt_it = it->second.possibleValues.begin(); opt_it != it->second.possibleValues.end();
                 opt_it++ )
            {
                // Create callback data
                mConfigCallbackData.push_back(
                    ConfigCallbackData( this, it->second.name, *opt_it, mb1 ) );

                Widget entry = XtVaCreateManagedWidget( "menuentry", smeBSBObjectClass, menu, XtNlabel,
                                                        ( *opt_it ).c_str(), 0, NULL );
                XtAddCallback( entry, XtNcallback, (XtCallbackProc)&GLXConfigurator::configOptionHandler,
                               &mConfigCallbackData.back() );
            }
            cury += rowh;
        }
    }

    void GLXConfigurator::SetConfigOption( const String &optionName, const String &valueName )
    {
        if( !mRenderer )
            // No renderer set -- how can this be called?
            return;
        mRenderer->setConfigOption( optionName, valueName );
        SetRenderer( mRenderer );
    }

    //------------------------------------------------------------------------------------//
    ConfigDialog::ConfigDialog() : mSelectedRenderSystem( 0 ) {}

    //------------------------------------------------------------------------------------//
    bool ConfigDialog::display()
    {
        GLXConfigurator test;
        /* Select previously selected rendersystem */
        if( Root::getSingleton().getRenderSystem() )
            test.SetRenderSystem( Root::getSingleton().getRenderSystem() );
        /* Attempt to create the window */
        if( !test.CreateWindow() )
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "Could not create configuration dialog",
                         "GLXConfig::display" );

        // Modal loop
        test.Main();
        if( !test.accept )  // User did not accept
            return false;

        /* All done */
        Root::getSingleton().setRenderSystem( test.mRenderer );

        return true;
    }
}  // namespace Ogre
