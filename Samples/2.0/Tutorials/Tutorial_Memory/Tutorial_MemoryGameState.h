
#ifndef _Demo_MemoryGameState_H_
#define _Demo_MemoryGameState_H_

#include "OgrePrerequisites.h"
#include "OgreOverlayPrerequisites.h"
#include "OgreOverlay.h"
#include "OgreGpuResource.h"
#include "OgreTextureGpuManagerListener.h"
#include "OgreTextureGpuManager.h"
#include "TutorialGameState.h"


namespace Demo
{
    class SceneFormatGameState;

    class MemoryGameState : public TutorialGameState, public Ogre::DefaultTextureGpuManagerListener
    {
        SceneFormatGameState *mSceneGenerator;
        Ogre::TextureGpuManager::BudgetEntryVec mDefaultBudget;

        bool mTightMemoryBudget;

        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

        void createCleanupScene();
        void destroyCleanupScene();
        bool isSceneLoaded() const;

        template <typename T, size_t MaxNumTextures>
        void unloadTexturesFromUnusedMaterials( Ogre::HlmsDatablock *datablock,
                                                std::set<Ogre::TextureGpu*> &usedTex,
                                                std::set<Ogre::TextureGpu*> &unusedTex );
        /// Unloads textures that are only bound to unused materials/datablock
        void unloadTexturesFromUnusedMaterials();

        /// Unloads textures that are currently not bound to anything (i.e. not even to a material)
        /// Use with caution, as it may unload textures that are used for something else
        /// and the code may not account for it.
        void unloadUnusedTextures();

        void minimizeMemory();
        void setTightMemoryBudget();
        void setRelaxedMemoryBudget();

    public:
        MemoryGameState( const Ogre::String &helpDescription );

        virtual size_t getNumSlicesFor( Ogre::TextureGpu *texture,
                                        Ogre::TextureGpuManager *textureManager );

        virtual void createScene01();
        virtual void destroyScene();

        virtual void update( float timeSinceLast );

        virtual void keyReleased( const SDL_KeyboardEvent &arg );
    };
}

#endif
