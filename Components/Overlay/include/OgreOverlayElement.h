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

#ifndef __OverlayElement_H__
#define __OverlayElement_H__

#include "OgreOverlayPrerequisites.h"

#include "OgreColourValue.h"
#include "OgreOverlayElementCommands.h"
#include "OgreRenderable.h"
#include "OgreStringInterface.h"
#include "OgreUTFString.h"

namespace Ogre
{
    namespace v1
    {
        /** \addtogroup Core
         *  @{
         */
        /** \addtogroup Overlays
         *  @{
         */

#if OGRE_UNICODE_SUPPORT
        typedef UTFString DisplayString;
#    define OGRE_DEREF_DISPLAYSTRING_ITERATOR( it ) it.getCharacter()
#else
        typedef String DisplayString;
#    define OGRE_DEREF_DISPLAYSTRING_ITERATOR( it ) *it
#endif
        /** Enum describing how the position / size of an element is to be recorded.
         */
        enum GuiMetricsMode
        {
            /// 'left', 'top', 'height' and 'width' are parametrics from 0.0 to 1.0
            GMM_RELATIVE,
            /// Positions & sizes are in absolute pixels
            GMM_PIXELS,
            /// Positions & sizes are in virtual pixels
            GMM_RELATIVE_ASPECT_ADJUSTED
        };

        /** Enum describing where '0' is in relation to the parent in the horizontal dimension.
        @remarks Affects how 'left' is interpreted.
        */
        enum GuiHorizontalAlignment
        {
            GHA_LEFT,
            GHA_CENTER,
            GHA_RIGHT
        };
        /** Enum describing where '0' is in relation to the parent in the vertical dimension.
        @remarks Affects how 'top' is interpreted.
        */
        enum GuiVerticalAlignment
        {
            GVA_TOP,
            GVA_CENTER,
            GVA_BOTTOM
        };

        /** Abstract definition of a 2D element to be displayed in an Overlay.
        @remarks
        This class abstracts all the details of a 2D element which will appear in
        an overlay. In fact, not all OverlayElement instances can be directly added to an
        Overlay, only those which are OverlayContainer instances (a subclass of this class).
        OverlayContainer objects can contain any OverlayElement however. This is just to
        enforce some level of grouping on widgets.
        @par
        OverlayElements should be managed using OverlayManager. This class is responsible for
        instantiating / deleting elements, and also for accepting new types of element
        from plugins etc.
        @par
        Note that positions / dimensions of 2D screen elements are expressed as parametric
        values (0.0 - 1.0) because this makes them resolution-independent. However, most
        screen resolutions have an aspect ratio of 1.3333:1 (width : height) so note that
        in physical pixels 0.5 is wider than it is tall, so a 0.5x0.5 panel will not be
        square on the screen (but it will take up exactly half the screen in both dimensions).
        @par
        Because this class is designed to be extensible, it subclasses from StringInterface
        so its parameters can be set in a generic way.
        */
        class _OgreOverlayExport OverlayElement : public StringInterface,
                                                  public Renderable,
                                                  public OgreAllocatedObj
        {
        public:
        protected:
            // Command object for setting / getting parameters
            static OverlayElementCommands::CmdLeft            msLeftCmd;
            static OverlayElementCommands::CmdTop             msTopCmd;
            static OverlayElementCommands::CmdWidth           msWidthCmd;
            static OverlayElementCommands::CmdHeight          msHeightCmd;
            static OverlayElementCommands::CmdMaterial        msMaterialCmd;
            static OverlayElementCommands::CmdCaption         msCaptionCmd;
            static OverlayElementCommands::CmdMetricsMode     msMetricsModeCmd;
            static OverlayElementCommands::CmdHorizontalAlign msHorizontalAlignCmd;
            static OverlayElementCommands::CmdVerticalAlign   msVerticalAlignCmd;
            static OverlayElementCommands::CmdVisible         msVisibleCmd;

            String        mName;
            bool          mVisible;
            bool          mCloneable;
            Real          mLeft;
            Real          mTop;
            Real          mWidth;
            Real          mHeight;
            String        mMaterialName;
            MaterialPtr   mMaterial;
            DisplayString mCaption;
            ColourValue   mColour;
            RealRect      mClippingRegion;

            GuiMetricsMode         mMetricsMode;
            GuiHorizontalAlignment mHorzAlign;
            GuiVerticalAlignment   mVertAlign;

            // metric-mode positions, used in GMM_PIXELS & GMM_RELATIVE_ASPECT_ADJUSTED mode.
            Real mPixelTop;
            Real mPixelLeft;
            Real mPixelWidth;
            Real mPixelHeight;
            Real mPixelScaleX;
            Real mPixelScaleY;

            /// Parent pointer
            OverlayContainer *mParent;
            /// Overlay attached to
            Overlay *mOverlay;

            // Derived positions from parent
            Real mDerivedLeft;
            Real mDerivedTop;
            bool mDerivedOutOfDate;

            /// Flag indicating if the vertex positions need recalculating
            bool mGeomPositionsOutOfDate;
            /// Flag indicating if the vertex uvs need recalculating
            bool mGeomUVsOutOfDate;

            /// Is element enabled?
            bool mEnabled;

            /// Is element initialised?
            bool mInitialised;

            /// Used to see if this element is created from a Template
            OverlayElement *mSourceTemplate;

            /** Internal method which is triggered when the positions of the element get updated,
            meaning the element should be rebuilding it's mesh positions. Abstract since
            subclasses must implement this.
            */
            virtual void updatePositionGeometry() = 0;
            /** Internal method which is triggered when the UVs of the element get updated,
            meaning the element should be rebuilding it's mesh UVs. Abstract since
            subclasses must implement this.
            */
            virtual void updateTextureGeometry() = 0;

            /** Internal method for setting up the basic parameter definitions for a subclass.
            @remarks
            Because StringInterface holds a dictionary of parameters per class, subclasses need to
            call this to ask the base class to add it's parameters to their dictionary as well.
            Can't do this in the constructor because that runs in a non-virtual context.
            @par
            The subclass must have called it's own createParamDictionary before calling this method.
            */
            virtual void addBaseParameters();

        public:
            /// Constructor: do not call direct, use OverlayManager::createElement
            OverlayElement( const String &name );
            ~OverlayElement() override;

            /** Initialise gui element */
            virtual void initialise() = 0;

            /** Notifies that hardware resources were lost */
            virtual void _releaseManualHardwareResources() {}
            /** Notifies that hardware resources should be restored */
            virtual void _restoreManualHardwareResources() {}

            /** Gets the name of this overlay. */
            const String &getName() const;

            /** Shows this element if it was hidden. */
            virtual void show();

            /** Hides this element if it was visible. */
            virtual void hide();

            /** Returns whether or not the element is visible. */
            bool isVisible() const;

            bool         isEnabled() const;
            virtual void setEnabled( bool b );

            /** Sets the dimensions of this element in relation to the screen (1.0 = screen
             * width/height). */
            void setDimensions( Real width, Real height );

            /** Sets the position of the top-left corner of the element, relative to the screen size
            (1.0 = screen width / height) */
            void setPosition( Real left, Real top );

            /** Sets the width of this element in relation to the screen (where 1.0 = screen width) */
            void setWidth( Real width );
            /** Gets the width of this element in relation to the screen (where 1.0 = screen width) */
            Real getWidth() const;

            /** Sets the height of this element in relation to the screen (where 1.0 = screen height) */
            void setHeight( Real height );
            /** Gets the height of this element in relation to the screen (where 1.0 = screen height) */
            Real getHeight() const;

            /** Sets the left of this element in relation to the screen (where 0 = far left, 1.0 = far
             * right) */
            void setLeft( Real left );
            /** Gets the left of this element in relation to the screen (where 0 = far left, 1.0 = far
             * right)  */
            Real getLeft() const;

            /** Sets the top of this element in relation to the screen (where 0 = top, 1.0 = bottom) */
            void setTop( Real Top );
            /** Gets the top of this element in relation to the screen (where 0 = top, 1.0 = bottom)  */
            Real getTop() const;

            /** Gets the left of this element in relation to the screen (where 0 = far left, 1.0 = far
             * right)  */
            Real _getLeft() const { return mLeft; }
            /** Gets the top of this element in relation to the screen (where 0 = far left, 1.0 = far
             * right)  */
            Real _getTop() const { return mTop; }
            /** Gets the width of this element in relation to the screen (where 1.0 = screen width)  */
            Real _getWidth() const { return mWidth; }
            /** Gets the height of this element in relation to the screen (where 1.0 = screen height)  */
            Real _getHeight() const { return mHeight; }
            /** Sets the left of this element in relation to the screen (where 1.0 = screen width) */
            void _setLeft( Real left );
            /** Sets the top of this element in relation to the screen (where 1.0 = screen width) */
            void _setTop( Real top );
            /** Sets the width of this element in relation to the screen (where 1.0 = screen width) */
            void _setWidth( Real width );
            /** Sets the height of this element in relation to the screen (where 1.0 = screen width) */
            void _setHeight( Real height );
            /** Sets the left and top of this element in relation to the screen (where 1.0 = screen
             * width) */
            void _setPosition( Real left, Real top );
            /** Sets the width and height of this element in relation to the screen (where 1.0 = screen
             * width) */
            void _setDimensions( Real width, Real height );

            /** Gets the name of the material this element uses. */
            virtual const String &getMaterialName() const;

            /** Sets the name of the material this element will use.
            @remarks
            Different elements will use different materials. One constant about them
            all though is that a Material used for a OverlayElement must have it's depth
            checking set to 'off', which means it always gets rendered on top. OGRE
            will set this flag for you if necessary. What it does mean though is that
            you should not use the same Material for rendering OverlayElements as standard
            scene objects. It's fine to use the same textures, just not the same
            Material.
            */
            virtual void setMaterialName( const String &matName );

            // --- Renderable Overrides ---
            /** See Renderable */
            const MaterialPtr &getMaterial() const;

            // NB getRenderOperation not implemented, still abstract here

            /** See Renderable */
            void getWorldTransforms( Matrix4 *xform ) const override;

            /** Tell the object to recalculate */
            virtual void _positionsOutOfDate();

            /** Internal method to update the element based on transforms applied. */
            virtual void _update();

            /** Updates this elements transform based on it's parent. */
            virtual void _updateFromParent();

            /** Internal method for notifying the GUI element of it's parent and ultimate overlay. */
            virtual void _notifyParent( OverlayContainer *parent, Overlay *overlay );

            /** Gets the 'left' position as derived from own left and that of parents. */
            virtual Real _getDerivedLeft();

            /** Gets the 'top' position as derived from own left and that of parents. */
            virtual Real _getDerivedTop();

            /** Gets the 'width' as derived from own width and metrics mode. */
            virtual Real _getRelativeWidth();
            /** Gets the 'height' as derived from own height and metrics mode. */
            virtual Real _getRelativeHeight();

            /** Gets the clipping region of the element */
            virtual void _getClippingRegion( RealRect &clippingRegion );

            /** Internal method to notify the element when the viewport
             of parent overlay has changed.
            */
            virtual void _notifyViewport();

            /** Internal method to put the contents onto the render queue. */
            virtual void _updateRenderQueue( RenderQueue *queue, Camera *camera,
                                             const Camera *lodCamera );

            /** Gets the type name of the element. All concrete subclasses must implement this. */
            virtual const String &getTypeName() const = 0;

            /** Sets the caption on elements that support it.
            @remarks
            This property doesn't do something on all elements, just those that support it.
            However, being a common requirement it is in the top-level interface to avoid
            having to set it via the StringInterface all the time.
            */
            virtual void setCaption( const DisplayString &text );
            /** Gets the caption for this element. */
            virtual const DisplayString &getCaption() const;
            /** Sets the colour on elements that support it.
            @remarks
            This property doesn't do something on all elements, just those that support it.
            However, being a common requirement it is in the top-level interface to avoid
            having to set it via the StringInterface all the time.
            */
            virtual void setColour( const ColourValue &col );

            /** Gets the colour for this element. */
            virtual const ColourValue &getColour() const;

            /** Tells this element how to interpret the position and dimension values it is given.
            @remarks
            By default, OverlayElements are positioned and sized according to relative dimensions
            of the screen. This is to ensure portability between different resolutions when you
            want things to be positioned and sized the same way across all resolutions. However,
            sometimes you want things to be sized according to fixed pixels. In order to do this,
            you can call this method with the parameter GMM_PIXELS. Note that if you then want
            to place your element relative to the center, right or bottom of it's parent, you will
            need to use the setHorizontalAlignment and setVerticalAlignment methods.
            */
            virtual void setMetricsMode( GuiMetricsMode gmm );
            /** Retrieves the current settings of how the element metrics are interpreted. */
            virtual GuiMetricsMode getMetricsMode() const;
            /** Sets the horizontal origin for this element.
            @remarks
            By default, the horizontal origin for a OverlayElement is the left edge of the parent
            container (or the screen if this is a root element). You can alter this by calling this
            method, which is especially useful when you want to use pixel-based metrics (see
            setMetricsMode) since in this mode you can't use relative positioning.
            @par
            For example, if you were using GMM_PIXELS metrics mode, and you wanted to place a 30x30 pixel
            crosshair in the center of the screen, you would use GHA_CENTER with a 'left' property of
            -15.
            @par
            Note that neither GHA_CENTER or GHA_RIGHT alter the position of the element based
            on it's width, you have to alter the 'left' to a negative number to do that; all this
            does is establish the origin. This is because this way you can align multiple things
            in the center and right with different 'left' offsets for maximum flexibility.
            */
            virtual void setHorizontalAlignment( GuiHorizontalAlignment gha );
            /** Gets the horizontal alignment for this element. */
            virtual GuiHorizontalAlignment getHorizontalAlignment() const;
            /** Sets the vertical origin for this element.
            @remarks
            By default, the vertical origin for a OverlayElement is the top edge of the parent container
            (or the screen if this is a root element). You can alter this by calling this method, which
            is especially useful when you want to use pixel-based metrics (see setMetricsMode) since in
            this mode you can't use relative positioning.
            @par
            For example, if you were using GMM_PIXELS metrics mode, and you wanted to place a 30x30 pixel
            crosshair in the center of the screen, you would use GHA_CENTER with a 'top' property of -15.
            @par
            Note that neither GVA_CENTER or GVA_BOTTOM alter the position of the element based
            on it's height, you have to alter the 'top' to a negative number to do that; all this
            does is establish the origin. This is because this way you can align multiple things
            in the center and bottom with different 'top' offsets for maximum flexibility.
            */
            virtual void setVerticalAlignment( GuiVerticalAlignment gva );
            /** Gets the vertical alignment for this element. */
            virtual GuiVerticalAlignment getVerticalAlignment() const;

            /** Returns true if xy is within the constraints of the component */
            virtual bool contains( Real x, Real y ) const;

            /** Returns true if xy is within the constraints of the component */
            virtual OverlayElement *findElementAt( Real x, Real y );  // relative to parent

            /**
             * returns false as this class is not a container type
             */
            inline virtual bool isContainer() const { return false; }

            inline virtual bool isKeyEnabled() const { return false; }

            inline virtual bool isCloneable() const { return mCloneable; }

            inline virtual void setCloneable( bool c ) { mCloneable = c; }

            /**
             * Returns the parent container.
             */
            OverlayContainer *getParent();
            void              _setParent( OverlayContainer *parent ) { mParent = parent; }

            /** @copydoc Renderable::getLights */
            const LightList &getLights() const override
            {
                // Overlayelements should not be lit by the scene, this will not get called
                static LightList ll;
                return ll;
            }

            virtual void            copyFromTemplate( OverlayElement *templateOverlay );
            virtual OverlayElement *clone( const String &instanceName );

            /// Returns the SourceTemplate for this element
            const OverlayElement *getSourceTemplate() const { return mSourceTemplate; }
        };

        /** @} */
        /** @} */
    }  // namespace v1
}  // namespace Ogre

#endif
