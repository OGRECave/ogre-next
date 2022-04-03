/*-------------------------------------------------------------------------
This source file is a part of OGRE-Next
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
THE SOFTWARE
-------------------------------------------------------------------------*/

#ifndef _FontManager_H__
#define _FontManager_H__

#include "OgreOverlayPrerequisites.h"

#include "OgreFont.h"
#include "OgreResourceManager.h"
#include "OgreSingleton.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */
    /** Manages Font resources, parsing .fontdef files and generally organising them.*/
    class _OgreOverlayExport FontManager : public ResourceManager, public Singleton<FontManager>
    {
    public:
        FontManager();
        ~FontManager() override;

        /// Create a new font
        /// @see ResourceManager::createResource
        FontPtr create( const String &name, const String &group, bool isManual = false,
                        ManualResourceLoader *loader = 0, const NameValuePairList *createParams = 0 );

        /// Get a resource by name
        /// @see ResourceManager::getResourceByName
        FontPtr getByName(
            const String &name,
            const String &groupName = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );

        /** @copydoc ScriptLoader::parseScript */
        void parseScript( DataStreamPtr &stream, const String &groupName ) override;
        /** Override standard Singleton retrieval.
        @remarks
        Why do we do this? Well, it's because the Singleton
        implementation is in a .h file, which means it gets compiled
        into anybody who includes it. This is needed for the
        Singleton template to work, but we actually only want it
        compiled into the implementation of the class based on the
        Singleton, not all of them. If we don't change this, we get
        link errors when trying to use the Singleton-based class from
        an outside dll.
        @par
        This method just delegates to the template version anyway,
        but the implementation stays in this single compilation unit,
        preventing link errors.
        */
        static FontManager &getSingleton();
        /** Override standard Singleton retrieval.
        @remarks
        Why do we do this? Well, it's because the Singleton
        implementation is in a .h file, which means it gets compiled
        into anybody who includes it. This is needed for the
        Singleton template to work, but we actually only want it
        compiled into the implementation of the class based on the
        Singleton, not all of them. If we don't change this, we get
        link errors when trying to use the Singleton-based class from
        an outside dll.
        @par
        This method just delegates to the template version anyway,
        but the implementation stays in this single compilation unit,
        preventing link errors.
        */
        static FontManager *getSingletonPtr();

    protected:
        /// Internal methods
        Resource *createImpl( const String &name, ResourceHandle handle, const String &group,
                              bool isManual, ManualResourceLoader *loader,
                              const NameValuePairList *params ) override;

        void parseAttribute( const String &line, FontPtr &pFont );

        void logBadAttrib( const String &line, FontPtr &pFont );
    };
    /** @} */
    /** @} */
}  // namespace Ogre

#endif
