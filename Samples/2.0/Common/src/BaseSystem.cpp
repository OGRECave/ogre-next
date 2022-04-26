
#include "BaseSystem.h"
#include "GameState.h"

namespace Demo
{
    BaseSystem::BaseSystem( GameState *gameState ) : mCurrentGameState( gameState ) {}
    //-----------------------------------------------------------------------------------
    BaseSystem::~BaseSystem() {}
    //-----------------------------------------------------------------------------------
    void BaseSystem::initialize() { mCurrentGameState->initialize(); }
    //-----------------------------------------------------------------------------------
    void BaseSystem::deinitialize() { mCurrentGameState->deinitialize(); }
    //-----------------------------------------------------------------------------------
    void BaseSystem::createScene01() { mCurrentGameState->createScene01(); }
    //-----------------------------------------------------------------------------------
    void BaseSystem::createScene02() { mCurrentGameState->createScene02(); }
    //-----------------------------------------------------------------------------------
    void BaseSystem::destroyScene() { mCurrentGameState->destroyScene(); }
    //-----------------------------------------------------------------------------------
    void BaseSystem::beginFrameParallel() { this->processIncomingMessages(); }
    //-----------------------------------------------------------------------------------
    void BaseSystem::update( float timeSinceLast ) { mCurrentGameState->update( timeSinceLast ); }
    //-----------------------------------------------------------------------------------
    void BaseSystem::finishFrameParallel()
    {
        mCurrentGameState->finishFrameParallel();

        this->flushQueuedMessages();
    }
    //-----------------------------------------------------------------------------------
    void BaseSystem::finishFrame() { mCurrentGameState->finishFrame(); }
}  // namespace Demo
