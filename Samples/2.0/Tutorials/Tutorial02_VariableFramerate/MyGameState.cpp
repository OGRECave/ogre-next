
#include "MyGameState.h"
#include "GraphicsSystem.h"

#include "OgreItem.h"
#include "OgreSceneManager.h"

#include "OgreTextAreaOverlayElement.h"

using namespace Demo;

namespace Demo
{
    MyGameState::MyGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mSceneNode( 0 ),
        mDisplacement( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    void MyGameState::createScene01()
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        Ogre::Item *item = sceneManager->createItem(
            "Cube_d.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
            Ogre::SCENE_DYNAMIC );

        mSceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                         ->createChildSceneNode( Ogre::SCENE_DYNAMIC );

        mSceneNode->attachObject( item );

        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
    void MyGameState::update( float timeSinceLast )
    {
        const Ogre::Vector3 origin( -5.0f, 0.0f, 0.0f );

        mDisplacement += timeSinceLast * 4.0f;
        mDisplacement = fmodf( mDisplacement, 10.0f );

        mSceneNode->setPosition( origin + Ogre::Vector3::UNIT_X * mDisplacement );

        TutorialGameState::update( timeSinceLast );
    }
}  // namespace Demo
