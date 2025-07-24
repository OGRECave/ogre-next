
#ifndef _Demo_GraphicsSystem_H_
#define _Demo_GraphicsSystem_H_

#include "OgreOverlayPrerequisites.h"
#include "OgrePrerequisites.h"

#include "BaseSystem.h"

#include "GameEntityManager.h"
#include "OgreColourValue.h"
#include "OgreOverlaySystem.h"
#include "SdlEmulationLayer.h"
#include "System/StaticPluginLoader.h"
#include "Threading/OgreUniformScalableTask.h"

#if OGRE_USE_SDL2
#    if defined( __clang__ )
#        pragma clang diagnostic push
#        pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#    elif defined( __GNUC__ )
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#    endif
#    include <SDL.h>
#    if defined( __clang__ )
#        pragma clang diagnostic pop
#    elif defined( __GNUC__ )
#        pragma GCC diagnostic pop
#    endif
#endif

namespace Demo
{
    class SdlInputHandler;

    class GraphicsSystem : public BaseSystem, public Ogre::UniformScalableTask
    {
    private:
        using BaseSystem::initialize;

    protected:
        BaseSystem *mLogicSystem;

#if OGRE_USE_SDL2
        SDL_Window      *mSdlWindow;
        SdlInputHandler *mInputHandler;
#endif

        Ogre::Root                *mRoot;
        Ogre::Window              *mRenderWindow;
        Ogre::SceneManager        *mSceneManager;
        Ogre::Camera              *mCamera;
        Ogre::CompositorWorkspace *mWorkspace;
        Ogre::String               mPluginsFolder;
        Ogre::String               mWriteAccessFolder;
        Ogre::String               mResourcePath;

        Ogre::v1::OverlaySystem *mOverlaySystem;

        StaticPluginLoader mStaticPluginLoader;

        /// Tracks the amount of elapsed time since we last
        /// heard from the LogicSystem finishing a frame
        float                mAccumTimeSinceLastLogicFrame;
        Ogre::uint32         mCurrentTransformIdx;
        GameEntityVec        mGameEntities[Ogre::NUM_SCENE_MEMORY_MANAGER_TYPES];
        GameEntityVec const *mThreadGameEntityToUpdate;
        float                mThreadWeight;

        bool              mQuit;
        bool              mAlwaysAskForConfig;
        bool              mUseHlmsDiskCache;
        bool              mUseMicrocodeCache;
        bool              mRequirePersistentDepthBuf;
        Ogre::ColourValue mBackgroundColour;

#if OGRE_USE_SDL2
        void handleWindowEvent( const SDL_Event &evt );
#endif

        bool isWriteAccessFolder( const Ogre::String &folderPath, const Ogre::String &fileToSave );

        /// @see MessageQueueSystem::processIncomingMessage
        void processIncomingMessage( Mq::MessageId messageId, const void *data ) override;

        static void addResourceLocation( const Ogre::String &archName, const Ogre::String &typeName,
                                         const Ogre::String &secName );

        void loadTextureCache();
        void saveTextureCache();
        void loadHlmsDiskCache();
        void saveHlmsDiskCache();

        virtual void setupResources();
        virtual void registerHlms();
        /// Optional override method where you can perform resource group loading
        /// Must at least do ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
        virtual void loadResources();
        virtual void chooseSceneManager();
        virtual void createCamera();
        /// Virtual so that advanced samples such as Sample_Compositor can override this
        /// method to change the default behavior if setupCompositor() is overridden, be
        /// aware @mBackgroundColour will be ignored
        virtual Ogre::CompositorWorkspace *setupCompositor();

        /// Called right before initializing Ogre's first window, so the params can be customized
        virtual void initMiscParamsListener( Ogre::NameValuePairList &params );

        /// Optional override method where you can create resource listeners (e.g. for loading screens)
        virtual void createResourceListener() {}

        void gameEntityAdded( const GameEntityManager::CreatedGameEntity *createdGameEntity );
        void gameEntityRemoved( GameEntity *toRemove );

    public:
        GraphicsSystem( GameState *gameState, Ogre::String resourcePath = Ogre::String( "" ),
                        Ogre::ColourValue backgroundColour = Ogre::ColourValue( 0.2f, 0.4f, 0.6f ) );
        ~GraphicsSystem() override;

        void _notifyLogicSystem( BaseSystem *logicSystem ) { mLogicSystem = logicSystem; }

        void initialize( const Ogre::String &windowTitle );
        void deinitialize() override;

        void update( float timeSinceLast );

        /** Updates the SceneNodes of all the game entities in the container,
            interpolating them according to weight, reading the transforms from
            mCurrentTransformIdx and mCurrentTransformIdx-1.
        @param gameEntities
            The container with entities to update.
        @param weight
            The interpolation weight, ideally in range [0; 1]
        */
        void updateGameEntities( const GameEntityVec &gameEntities, float weight );

        /// Overload Ogre::UniformScalableTask. @see updateGameEntities
        void execute( size_t threadId, size_t numThreads ) override;

        /// Returns the GameEntities that are ready to be rendered. May include entities
        /// that are scheduled to be removed (i.e. they are no longer updated by logic)
        const GameEntityVec &getGameEntities( Ogre::SceneMemoryMgrTypes type ) const
        {
            return mGameEntities[type];
        }

#if OGRE_USE_SDL2
        SdlInputHandler *getInputHandler() { return mInputHandler; }
#endif

        /// Creates an atmosphere and binds it to the SceneManager
        /// You can use SceneManager::getAtmosphere to retrieve it.
        ///
        /// The input light will be bound to the atmosphere component.
        /// Can be nullptr.
        void createAtmosphere( Ogre::Light *sunLight );

        void setQuit() { mQuit = true; }
        bool getQuit() const { return mQuit; }

        float getAccumTimeSinceLastLogicFrame() const { return mAccumTimeSinceLastLogicFrame; }

        Ogre::Root                *getRoot() const { return mRoot; }
        Ogre::Window              *getRenderWindow() const { return mRenderWindow; }
        Ogre::SceneManager        *getSceneManager() const { return mSceneManager; }
        Ogre::Camera              *getCamera() const { return mCamera; }
        Ogre::CompositorWorkspace *getCompositorWorkspace() const { return mWorkspace; }
        Ogre::v1::OverlaySystem   *getOverlaySystem() const { return mOverlaySystem; }

        void setAlwaysAskForConfig( bool alwaysAskForConfig );
        bool getAlwaysAskForConfig() const { return mAlwaysAskForConfig; }

        const Ogre::String &getPluginsFolder() const { return mPluginsFolder; }
        const Ogre::String &getWriteAccessFolder() const { return mWriteAccessFolder; }
        const Ogre::String &getResourcePath() const { return mResourcePath; }
        const char         *getMediaReadArchiveType() const;
        void                setRequiePersistentDepthBuf( bool require );

        virtual void stopCompositor();
        virtual void restartCompositor();
    };
}  // namespace Demo

#endif
