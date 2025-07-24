/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2016 Torus Knot Software Ltd

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

#include <iostream>
#include "OgrePrerequisites.h"

#include "System/MainEntryPoints.h"

#include "System/Desktop/UnitTesting.h"
#include "TutorialGameState.h"

#include "GameState.h"
#include "GraphicsSystem.h"
#include "LogicSystem.h"
#include "SdlInputHandler.h"

#include "OgreTimer.h"
#include "OgreWindow.h"

#include "Threading/OgreThreads.h"

using namespace Demo;

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI Demo::MainEntryPoints::mainAppSingleThreaded( HINSTANCE hInst, HINSTANCE hPrevInstance,
                                                         LPSTR strCmdLine, INT nCmdShow )
#else
int Demo::MainEntryPoints::mainAppSingleThreaded( int argc, const char *argv[] )
#endif
{
    UnitTest unitTest;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    unitTest.parseCmdLine( __argc, __argv );
#else
    unitTest.parseCmdLine( argc, argv );
#endif

    if( unitTest.getParams().isPlayback() )
    {
        return unitTest.loadFromJson( unitTest.getParams().recordPath.c_str(),
                                      unitTest.getParams().outputPath );
    }

    GameState *graphicsGameState = 0;
    GraphicsSystem *graphicsSystem = 0;
    GameState *logicGameState = 0;
    LogicSystem *logicSystem = 0;

    MainEntryPoints::createSystems( &graphicsGameState, &graphicsSystem, &logicGameState, &logicSystem );
#ifdef AUTO_TESTING
    if( argv[1] )
    {
        graphicsSystem->setRendererParam( std::string( argv[1] ) );
    }
    if( TutorialGameState *tutorial = dynamic_cast<TutorialGameState *>( graphicsGameState ) )
    {
        tutorial->setSampleName( std::string( argv[0] ).substr( 2 ) );
    }
#endif
    try
    {
        graphicsSystem->initialize( getWindowTitle() );
        if( logicSystem )
            logicSystem->initialize();

        if( graphicsSystem->getQuit() )
        {
            if( logicSystem )
                logicSystem->deinitialize();
            graphicsSystem->deinitialize();

            MainEntryPoints::destroySystems( graphicsGameState, graphicsSystem, logicGameState,
                                             logicSystem );

            return 0;  // User cancelled config
        }

        if( unitTest.getParams().isRecording() )
            unitTest.startRecording( graphicsSystem );

        Ogre::Window *renderWindow = graphicsSystem->getRenderWindow();

        graphicsSystem->createScene01();
        if( logicSystem )
            logicSystem->createScene01();

        graphicsSystem->createScene02();
        if( logicSystem )
            logicSystem->createScene02();

#if OGRE_USE_SDL2
        // Do this after creating the scene for easier the debugging (the mouse doesn't hide itself)
        SdlInputHandler *inputHandler = graphicsSystem->getInputHandler();
        inputHandler->setGrabMousePointer( true );
        inputHandler->setMouseVisible( false );
        inputHandler->setMouseRelative( true );
#endif

        Ogre::Timer timer;
        Ogre::uint64 startTime = timer.getMicroseconds();
        double accumulator = MainEntryPoints::Frametime;

        double timeSinceLast = 1.0 / 60.0;

        while( !graphicsSystem->getQuit() )
        {
            while( accumulator >= MainEntryPoints::Frametime && logicSystem )
            {
                logicSystem->beginFrameParallel();
                logicSystem->update( static_cast<float>( MainEntryPoints::Frametime ) );
                logicSystem->finishFrameParallel();

                logicSystem->finishFrame();
                graphicsSystem->finishFrame();

                accumulator -= MainEntryPoints::Frametime;
            }

            graphicsSystem->beginFrameParallel();
            graphicsSystem->update( static_cast<float>( timeSinceLast ) );
            graphicsSystem->finishFrameParallel();
            if( !logicSystem )
                graphicsSystem->finishFrame();

            if( unitTest.getParams().isRecording() )
                unitTest.notifyRecordingNewFrame( graphicsSystem );

            if( !renderWindow->isVisible() )
            {
                // Don't burn CPU cycles unnecessary when we're minimized.
                Ogre::Threads::Sleep( 500 );
            }

            Ogre::uint64 endTime = timer.getMicroseconds();
            timeSinceLast = double( endTime - startTime ) / 1000000.0;
            timeSinceLast = std::min( 1.0, timeSinceLast );  // Prevent from going haywire.
            accumulator += timeSinceLast;
            startTime = endTime;
        }

        if( unitTest.getParams().isRecording() )
        {
            unitTest.saveToJson( unitTest.getParams().recordPath.c_str(),
                                 unitTest.getParams().bCompressDuration );
        }

        graphicsSystem->destroyScene();
        if( logicSystem )
        {
            logicSystem->destroyScene();
            logicSystem->deinitialize();
        }
        graphicsSystem->deinitialize();

        MainEntryPoints::destroySystems( graphicsGameState, graphicsSystem, logicGameState,
                                         logicSystem );
    }
    catch( Ogre::Exception &e )
    {
        MainEntryPoints::destroySystems( graphicsGameState, graphicsSystem, logicGameState,
                                         logicSystem );
        throw e;
    }
    catch( ... )
    {
        MainEntryPoints::destroySystems( graphicsGameState, graphicsSystem, logicGameState,
                                         logicSystem );
    }

    return 0;
}
