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

#ifndef __PanelOverlayElement_H__
#define __PanelOverlayElement_H__

#include "OgreOverlayContainer.h"
#include "OgreRenderOperation.h"

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
        /** OverlayElement representing a flat, single-material (or transparent) panel which can contain
        other elements.
        @remarks
            This class subclasses OverlayContainer because it can contain other elements. Like other
            containers, if hidden it's contents are also hidden, if moved it's contents also move etc.
            The panel itself is a 2D rectangle which is either completely transparent, or is rendered
            with a single material. The texture(s) on the panel can be tiled depending on your
        requirements.
        @par
            This component is suitable for backgrounds and grouping other elements. Note that because
            it has a single repeating material it cannot have a discrete border (unless the texture has
        one and the texture is tiled only once). For a bordered panel, see it's subclass
        BorderPanelOverlayElement.
        @par
            Note that the material can have all the usual effects applied to it like multiple texture
            layers, scrolling / animated textures etc. For multiple texture layers, you have to set
            the tiling level for each layer.
        */
        class _OgreOverlayExport PanelOverlayElement : public OverlayContainer
        {
        public:
            /** Constructor. */
            PanelOverlayElement( const String &name );
            ~PanelOverlayElement() override;

            /** Initialise */
            void initialise() override;

            /** @copydoc OverlayElement::_releaseManualHardwareResources. */
            void _releaseManualHardwareResources() override;
            /** @copydoc OverlayElement::_restoreManualHardwareResources. */
            void _restoreManualHardwareResources() override;

            /** Sets the number of times textures should repeat.
            @param x The number of times the texture should repeat horizontally
            @param y The number of times the texture should repeat vertically
            @param layer The texture layer to specify (only needs to be altered if
                you're using a multi-texture layer material)
            */
            void setTiling( Real x, Real y, ushort layer = 0 );

            Real getTileX( ushort layer = 0 ) const;
            /** Gets the number of times the texture should repeat vertically.
            @param layer The texture layer to specify (only needs to be altered if
                you're using a multi-texture layer material)
            */
            Real getTileY( ushort layer = 0 ) const;

            /** Sets the texture coordinates for the panel. */
            void setUV( Real u1, Real v1, Real u2, Real v2 );

            /** Get the uv coordinates for the panel*/
            void getUV( Real &u1, Real &v1, Real &u2, Real &v2 ) const;

            /** Sets whether this panel is transparent (used only as a grouping level), or
                if it is actually rendered.
            */
            void setTransparent( bool isTransparent );

            /** Returns whether this panel is transparent. */
            bool isTransparent() const;

            /** See OverlayElement. */
            const String &getTypeName() const override;
            /** See Renderable. */
            void getRenderOperation( v1::RenderOperation &op, bool casterPass ) override;
            /** Overridden from OverlayElement */
            void setMaterialName( const String &matName ) override;
            /** Overridden from OverlayContainer */
            void _updateRenderQueue( RenderQueue *queue, Camera *camera,
                                     const Camera *lodCamera ) override;

            /** Command object for specifying tiling (see ParamCommand).*/
            class _OgrePrivate CmdTiling final : public ParamCommand
            {
            public:
                String doGet( const void *target ) const override;
                void   doSet( void *target, const String &val ) override;
            };
            /** Command object for specifying transparency (see ParamCommand).*/
            class _OgrePrivate CmdTransparent final : public ParamCommand
            {
            public:
                String doGet( const void *target ) const override;
                void   doSet( void *target, const String &val ) override;
            };
            /** Command object for specifying UV coordinates (see ParamCommand).*/
            class _OgrePrivate CmdUVCoords final : public ParamCommand
            {
            public:
                String doGet( const void *target ) const override;
                void   doSet( void *target, const String &val ) override;
            };

        protected:
            /// Flag indicating if this panel should be visual or just group things
            bool mTransparent;
            // Texture tiling
            Real   mTileX[/*OGRE_MAX_TEXTURE_LAYERS*/ 16];
            Real   mTileY[/*OGRE_MAX_TEXTURE_LAYERS*/ 16];
            size_t mNumTexCoordsInBuffer;
            Real   mU1, mV1, mU2, mV2;

            v1::RenderOperation mRenderOp;

            /// Internal method for setting up geometry, called by OverlayElement::update
            void updatePositionGeometry() override;

            /// Called to update the texture coords when layers change
            void updateTextureGeometry() override;

            /// Method for setting up base parameters for this class
            void addBaseParameters() override;

            static String msTypeName;

            // Command objects
            static CmdTiling      msCmdTiling;
            static CmdTransparent msCmdTransparent;
            static CmdUVCoords    msCmdUVCoords;
        };
        /** @} */
        /** @} */

    }  // namespace v1
}  // namespace Ogre

#endif
