
#ifndef _Demo_BaseSystem_H_
#define _Demo_BaseSystem_H_

#include "Threading/MessageQueueSystem.h"

namespace Demo
{
    class GameState;

    class BaseSystem : public Mq::MessageQueueSystem
    {
    protected:
        GameState *mCurrentGameState;

    public:
        BaseSystem( GameState *gameState );
        ~BaseSystem() override;

        virtual void initialize();
        virtual void deinitialize();

        virtual void createScene01();
        virtual void createScene02();

        virtual void destroyScene();

        void beginFrameParallel();
        void update( float timeSinceLast );
        void finishFrameParallel();
        void finishFrame();
    };
}  // namespace Demo

#endif
