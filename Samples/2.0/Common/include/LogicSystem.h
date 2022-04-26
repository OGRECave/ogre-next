
#ifndef _Demo_LogicSystem_H_
#define _Demo_LogicSystem_H_

#include "BaseSystem.h"
#include "OgrePrerequisites.h"

#include <deque>

namespace Demo
{
    class GameEntityManager;

    class LogicSystem : public BaseSystem
    {
    protected:
        BaseSystem *       mGraphicsSystem;
        GameEntityManager *mGameEntityManager;

        Ogre::uint32             mCurrentTransformIdx;
        std::deque<Ogre::uint32> mAvailableTransformIdx;

        /// @see MessageQueueSystem::processIncomingMessage
        void processIncomingMessage( Mq::MessageId messageId, const void *data ) override;

    public:
        LogicSystem( GameState *gameState );
        ~LogicSystem() override;

        void _notifyGraphicsSystem( BaseSystem *graphicsSystem ) { mGraphicsSystem = graphicsSystem; }
        void _notifyGameEntityManager( GameEntityManager *mgr ) { mGameEntityManager = mgr; }

        void finishFrameParallel();

        GameEntityManager *getGameEntityManager() { return mGameEntityManager; }
        Ogre::uint32       getCurrentTransformIdx() const { return mCurrentTransformIdx; }
    };
}  // namespace Demo

#endif
