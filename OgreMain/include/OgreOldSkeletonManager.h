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
#ifndef _OldSkeletonManager_H_
#define _OldSkeletonManager_H_

#include "OgrePrerequisites.h"

#include "OgreResourceManager.h"
#include "OgreSingleton.h"

namespace Ogre
{
    namespace v1
    {
        /** \addtogroup Core
         *  @{
         */
        /** \addtogroup Animation
         *  @{
         */
        /** Handles the management of skeleton resources.
            @remarks
                This class deals with the runtime management of
                skeleton data; like other resource managers it handles
                the creation of resources (in this case skeleton data),
                working within a fixed memory budget.
        */
        class _OgreExport OldSkeletonManager final : public ResourceManager,
                                                     public Singleton<OldSkeletonManager>
        {
        public:
            /// Constructor
            OldSkeletonManager();
            ~OldSkeletonManager() override;

            /// Create a new skeleton
            /// @see ResourceManager::createResource
            SkeletonPtr create( const String &name, const String &group, bool isManual = false,
                                ManualResourceLoader    *loader = 0,
                                const NameValuePairList *createParams = 0 );

            /// Get a resource by name
            /// @see ResourceManager::getResourceByName
            SkeletonPtr getByName(
                const String &name,
                const String &groupName = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );

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
            static OldSkeletonManager &getSingleton();
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
            static OldSkeletonManager *getSingletonPtr();

        protected:
            /// @copydoc ResourceManager::createImpl
            Resource *createImpl( const String &name, ResourceHandle handle, const String &group,
                                  bool isManual, ManualResourceLoader *loader,
                                  const NameValuePairList *createParams ) override;
        };

        /** @} */
        /** @} */
    }  // namespace v1
}  // namespace Ogre

#endif
