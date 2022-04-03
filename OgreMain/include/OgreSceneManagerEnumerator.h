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
#ifndef __SceneManagerEnumerator_H__
#define __SceneManagerEnumerator_H__

#include "OgrePrerequisites.h"

#include "OgreSceneManager.h"
#include "OgreSingleton.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Scene
     *  @{
     */
    /// Factory for default scene manager
    class _OgreExport DefaultSceneManagerFactory final : public SceneManagerFactory
    {
    protected:
        void initMetaData() const override;

    public:
        DefaultSceneManagerFactory() {}
        ~DefaultSceneManagerFactory() override {}
        /// Factory type name
        static const String FACTORY_TYPE_NAME;
        SceneManager *createInstance( const String &instanceName, size_t numWorkerThreads ) override;
        void          destroyInstance( SceneManager *instance ) override;
    };
    /// Default scene manager
    class _OgreExport DefaultSceneManager final : public SceneManager
    {
    public:
        DefaultSceneManager( const String &name, size_t numWorkerThreads );
        ~DefaultSceneManager() override;
        const String &getTypeName() const override;
    };

    /** Enumerates the SceneManager classes available to applications.
        @remarks
            As described in the SceneManager class, SceneManagers are responsible
            for organising the scene and issuing rendering commands to the
            RenderSystem. Certain scene types can benefit from different
            rendering approaches, and it is intended that subclasses will
            be created to special case this.
        @par
            In order to give applications easy access to these implementations,
            this class has a number of methods to create or retrieve a SceneManager
            which is appropriate to the scene type.
        @par
            SceneManagers are created by SceneManagerFactory instances. New factories
            for new types of SceneManager can be registered with this class to make
            them available to clients.
        @par
            Note that you can still plug in your own custom SceneManager without
            using a factory, should you choose, it's just not as flexible that way.
            Just instantiate your own SceneManager manually and use it directly.
    */
    class _OgreExport SceneManagerEnumerator : public Singleton<SceneManagerEnumerator>,
                                               public OgreAllocatedObj
    {
    public:
        /// Scene manager instances, indexed by instance name
        typedef map<String, SceneManager *>::type Instances;
        /// List of available scene manager types as meta data
        typedef vector<const SceneManagerMetaData *>::type MetaDataList;

    private:
        /// Scene manager factories
        typedef list<SceneManagerFactory *>::type Factories;

        Factories mFactories;
        Instances mInstances;
        /// Stored separately to allow iteration
        MetaDataList mMetaDataList;
        /// Factory for default scene manager
        DefaultSceneManagerFactory mDefaultFactory;
        /// Count of creations for auto-naming
        unsigned long mInstanceCreateCount;
        /// Currently assigned render system
        RenderSystem *mCurrentRenderSystem;

    public:
        SceneManagerEnumerator();
        ~SceneManagerEnumerator();

        /** Register a new SceneManagerFactory.
        @remarks
            Plugins should call this to register as new SceneManager providers.
        */
        void addFactory( SceneManagerFactory *fact );

        /** Remove a SceneManagerFactory.
         */
        void removeFactory( SceneManagerFactory *fact );

        /** Get more information about a given type of SceneManager.
        @remarks
            The metadata returned tells you a few things about a given type
            of SceneManager, which can be created using a factory that has been
            registered already.
        @param typeName The type name of the SceneManager you want to enquire on.
            If you don't know the typeName already, you can iterate over the
            metadata for all types using getMetaDataIterator.
        */
        const SceneManagerMetaData *getMetaData( const String &typeName ) const;

        typedef ConstVectorIterator<MetaDataList> MetaDataIterator;
        /** Iterate over all types of SceneManager available for construction,
            providing some information about each one.
        */
        MetaDataIterator getMetaDataIterator() const;

        /** Create a SceneManager instance of a given type.
        @remarks
            You can use this method to create a SceneManager instance of a
            given specific type. You may know this type already, or you may
            have discovered it by looking at the results from getMetaDataIterator.
        @note
            This method throws an exception if the named type is not found.
        @param typeName String identifying a unique SceneManager type
        @param instanceName Optional name to given the new instance that is
            created. If you leave this blank, an auto name will be assigned.
        */
        SceneManager *createSceneManager( const String &typeName, size_t numWorkerThreads,
                                          const String &instanceName = BLANKSTRING );

        /** Create a SceneManager instance based on scene type support.
        @remarks
            Creates an instance of a SceneManager which supports the scene types
            identified in the parameter. If more than one type of SceneManager
            has been registered as handling that combination of scene types,
            in instance of the last one registered is returned.
        @note This method always succeeds, if a specific scene manager is not
            found, the default implementation is always returned.
        @param typeMask A mask containing one or more SceneType flags
        @param instanceName Optional name to given the new instance that is
            created. If you leave this blank, an auto name will be assigned.
        */
        SceneManager *createSceneManager( SceneTypeMask typeMask, size_t numWorkerThreads,
                                          const String &instanceName = BLANKSTRING );

        /** Destroy an instance of a SceneManager. */
        void destroySceneManager( SceneManager *sm );

        /** Get an existing SceneManager instance that has already been created,
            identified by the instance name.
        @param instanceName The name of the instance to retrieve.
        */
        SceneManager *getSceneManager( const String &instanceName ) const;

        /** Identify if a SceneManager instance already exists.
        @param instanceName The name of the instance to retrieve.
        */
        bool hasSceneManager( const String &instanceName ) const;

        typedef MapIterator<Instances> SceneManagerIterator;
        /** Get an iterator over all the existing SceneManager instances. */
        SceneManagerIterator getSceneManagerIterator();

        /** Notifies all SceneManagers of the destination rendering system.
         */
        void setRenderSystem( RenderSystem *rs );

        /// Utility method to control shutdown of the managers
        void shutdownAll();
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
        static SceneManagerEnumerator &getSingleton();
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
        static SceneManagerEnumerator *getSingletonPtr();
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
