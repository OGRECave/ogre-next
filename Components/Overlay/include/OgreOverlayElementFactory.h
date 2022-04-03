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
#ifndef __OverlayElementFactory_H__
#define __OverlayElementFactory_H__

#include "OgreOverlayPrerequisites.h"

#include "OgreBorderPanelOverlayElement.h"
#include "OgreOverlayElement.h"
#include "OgrePanelOverlayElement.h"
#include "OgreTextAreaOverlayElement.h"

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
        /** Defines the interface which all components wishing to
            supply OverlayElement subclasses must implement.
        @remarks
            To allow the OverlayElement types available for inclusion on
            overlays to be extended, OGRE allows external apps or plugins
            to register their ability to create custom OverlayElements with
            the OverlayManager, using the addOverlayElementFactory method. Classes
            wanting to do this must implement this interface.
        @par
            Each OverlayElementFactory creates a single type of OverlayElement,
            identified by a 'type name' which must be unique.
        */
        class _OgreOverlayExport OverlayElementFactory : public OgreAllocatedObj
        {
        public:
            /** Destroy the overlay element factory */
            virtual ~OverlayElementFactory() {}
            /** Creates a new OverlayElement instance with the name supplied. */
            virtual OverlayElement *createOverlayElement( const String &instanceName ) = 0;
            /** Destroys a OverlayElement which this factory created previously. */
            virtual void destroyOverlayElement( OverlayElement *pElement ) { delete pElement; }
            /** Gets the string uniquely identifying the type of element this factory creates. */
            virtual const String &getTypeName() const = 0;
        };

        /** Factory for creating PanelOverlayElement instances. */
        class _OgreOverlayExport PanelOverlayElementFactory final : public OverlayElementFactory
        {
        public:
            /** See OverlayElementFactory */
            OverlayElement *createOverlayElement( const String &instanceName ) override
            {
                return OGRE_NEW PanelOverlayElement( instanceName );
            }
            /** See OverlayElementFactory */
            const String &getTypeName() const override;
        };

        /** Factory for creating BorderPanelOverlayElement instances. */
        class _OgreOverlayExport BorderPanelOverlayElementFactory final : public OverlayElementFactory
        {
        public:
            /** See OverlayElementFactory */
            OverlayElement *createOverlayElement( const String &instanceName ) override
            {
                return OGRE_NEW BorderPanelOverlayElement( instanceName );
            }
            /** See OverlayElementFactory */
            const String &getTypeName() const override;
        };

        /** Factory for creating TextAreaOverlayElement instances. */
        class _OgreOverlayExport TextAreaOverlayElementFactory final : public OverlayElementFactory
        {
        public:
            /** See OverlayElementFactory */
            OverlayElement *createOverlayElement( const String &instanceName ) override
            {
                return OGRE_NEW TextAreaOverlayElement( instanceName );
            }
            /** See OverlayElementFactory */
            const String &getTypeName() const override;
        };
        /** @} */
        /** @} */

    }  // namespace v1
}  // namespace Ogre

#endif
