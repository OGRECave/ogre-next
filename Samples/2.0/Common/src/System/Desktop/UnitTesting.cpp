/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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

#include "System/Desktop/UnitTesting.h"

#include "GameState.h"
#include "GraphicsSystem.h"
#include "LogicSystem.h"
#include "SdlInputHandler.h"
#include "System/MainEntryPoints.h"

#include "OgreCamera.h"
#include "OgreFrameStats.h"
#include "OgreLwString.h"
#include "OgreRoot.h"
#include "OgreStringConverter.h"
#include "OgreTextureGpuManager.h"
#include "OgreWindow.h"

#if !OGRE_NO_JSON && defined( USE_JSON_UNIT_TESTING )
#    if defined( __GNUC__ ) && !defined( __clang__ )
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wclass-memaccess"
#    endif
#    if defined( __clang__ )
#        pragma clang diagnostic push
#        pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#        pragma clang diagnostic ignored "-Wdeprecated-copy"
#    endif
#    include "rapidjson/document.h"
#    include "rapidjson/error/en.h"
#    if defined( __clang__ )
#        pragma clang diagnostic pop
#    endif
#    if defined( __GNUC__ ) && !defined( __clang__ )
#        pragma GCC diagnostic pop
#    endif
#endif

#include <fstream>

namespace Demo
{
    UnitTest::KeyStroke::KeyStroke() : keycode( 0 ), scancode( 0u ), bReleased( false ) {}
    UnitTest::FrameActivity::FrameActivity( uint32_t _frameId ) :
        frameId( _frameId ),
        cameraPos( Ogre::Vector3::ZERO ),
        cameraRot( Ogre::Quaternion::IDENTITY ),
        screenshotRenderWindow( false )
    {
    }
    UnitTest::Params::Params() : bRecord( false ), bCompressDuration( false ), bSkipDump( false ) {}
    //-------------------------------------------------------------------------
    bool UnitTest::Params::isRecording() const { return bRecord && !recordPath.empty(); }
    //-------------------------------------------------------------------------
    bool UnitTest::Params::isPlayback() const { return !bRecord && !recordPath.empty(); }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    UnitTest::UnitTest() :
        mFrametime( 1.0 / 60.0 ),
        mFrameIdx( 0u ),
        mNumFrames( 0u ),
        mRealKeyboardListener( 0 ),
        mRealMouseListener( 0 ),
        mBlockInputForwarding( false )
    {
    }
    //-------------------------------------------------------------------------
    inline void UnitTest::flushLwString( Ogre::LwString &jsonStr, std::string &outJson )
    {
        outJson += jsonStr.c_str();
        jsonStr.clear();
    }
    //-------------------------------------------------------------------------
    void UnitTest::exportFrameActivity( const FrameActivity &frameActivity, Ogre::LwString &jsonStr,
                                        std::string &outJson )
    {
        jsonStr.a( "\n\t\t\t\"frame_id\" : ", frameActivity.frameId, "," );

        jsonStr.a( "\n\t\t\t\"camera_pos\" : [", frameActivity.cameraPos.x, ", ",
                   frameActivity.cameraPos.y, ", ", frameActivity.cameraPos.z, "]," );
        jsonStr.a( "\n\t\t\t\"camera_rot\" : [", frameActivity.cameraRot.w, ", ",
                   frameActivity.cameraRot.x, ", ", frameActivity.cameraRot.y, ", " );
        jsonStr.a( frameActivity.cameraRot.z, "]," );

        if( !frameActivity.keyStrokes.empty() )
        {
            jsonStr.a(
                "\n\t\t\t\"key_strokes\" :"
                "\n\t\t\t[" );

            std::vector<KeyStroke>::const_iterator itor = frameActivity.keyStrokes.begin();
            std::vector<KeyStroke>::const_iterator endt = frameActivity.keyStrokes.end();

            while( itor != endt )
            {
                jsonStr.a( "\n\t\t\t\t{ \"key_code\" : ", itor->keycode,
                           ", \"scan_code\" : ", itor->scancode );
                jsonStr.a( ", \"released\" : ", itor->bReleased ? "true }," : "false }," );
                ++itor;
            }

            jsonStr.resize( jsonStr.size() - 1u );  // Remove trailing comma
            jsonStr.a( "\n\t\t\t]," );

            flushLwString( jsonStr, outJson );
        }

        if( !frameActivity.targetsToScreenshot.empty() )
        {
            jsonStr.a( "\n\t\t\t\"targets\" : [" );

            Ogre::StringVector::const_iterator itor = frameActivity.targetsToScreenshot.begin();
            Ogre::StringVector::const_iterator endt = frameActivity.targetsToScreenshot.end();

            while( itor != endt )
            {
                jsonStr.a( "\"", itor->c_str(), "\", " );
                ++itor;
            }

            jsonStr.resize( jsonStr.size() - 2u );  // Remove trailing space and comma
            jsonStr.a( "\n\t\t\t]," );
            flushLwString( jsonStr, outJson );
        }

        jsonStr.a( "\n\t\t\t\"screenshot_render_window\" : ",
                   frameActivity.screenshotRenderWindow ? "true" : "false" );
    }
    //-------------------------------------------------------------------------
    void UnitTest::saveToJsonStr( std::string &outJson )
    {
        char tmpBuffer[4096];
        Ogre::LwString jsonStr( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

        jsonStr.a( "{" );
        jsonStr.a( "\n\t\"framerate\" : ", (float)( 1.0 / MainEntryPoints::Frametime ), "," );
        jsonStr.a( "\n\t\"num_frames\" : ", mNumFrames, "," );
        if( !mFrameActivity.empty() )
        {
            jsonStr.a( "\n\t\"frame_activity\" :\n\t[" );
            flushLwString( jsonStr, outJson );

            std::vector<FrameActivity>::const_iterator itor = mFrameActivity.begin();
            std::vector<FrameActivity>::const_iterator endt = mFrameActivity.end();

            while( itor != endt )
            {
                jsonStr.a( "\n\t\t{" );
                exportFrameActivity( *itor, jsonStr, outJson );
                jsonStr.a( "\n\t\t}," );
                flushLwString( jsonStr, outJson );
                ++itor;
            }

            outJson.resize( outJson.size() - 1u );  // Remove trailing comma

            jsonStr.a( "\n\t]" );
        }
        jsonStr.a( "\n}" );

        flushLwString( jsonStr, outJson );
    }
    //-------------------------------------------------------------------------
    /** Returns true if 'str' starts with the text contained in 'what'
    @param str
    @param what
    @param outStartIdx
        str[outStartIdx] points to the first letter (including null terminator)
        that diverged from 'what', even if we return false
    @return
        True if there's a match, false otherwise
    */
    static bool startsWith( const char *str, const char *what, size_t &outStartIdx )
    {
        const char *origStart = str;

        while( *str && *what && *str == *what )
        {
            ++str;
            ++what;
        }

        outStartIdx = static_cast<size_t>( str - origStart );

        return *what == '\0';
    }
    //-------------------------------------------------------------------------
    void UnitTest::parseCmdLine( int nargs, const char *const *argv )
    {
        for( int i = 1; i < nargs; ++i )
        {
            size_t startIdx;
            if( startsWith( argv[i], "--ut_record=", startIdx ) )
            {
                mParams.bRecord = true;
                mParams.recordPath = std::string( argv[i] + startIdx );
            }
            else if( startsWith( argv[i], "--ut_output=", startIdx ) )
            {
                mParams.outputPath = std::string( argv[i] + startIdx );
            }
            else if( startsWith( argv[i], "--ut_compress", startIdx ) )
            {
                mParams.bCompressDuration = true;
            }
            else if( startsWith( argv[i], "--ut_playback=", startIdx ) )
            {
                mParams.bRecord = false;
                mParams.recordPath = std::string( argv[i] + startIdx );
            }
            else if( startsWith( argv[i], "--ut_skip_dump", startIdx ) )
            {
                mParams.bSkipDump = true;
            }
        }
    }
    //-------------------------------------------------------------------------
    void UnitTest::startRecording( GraphicsSystem *graphicsSystem )
    {
        SdlInputHandler *inputHandler = graphicsSystem->getInputHandler();
        mRealKeyboardListener = inputHandler->getKeyboardListener();
        inputHandler->_overrideKeyboardListener( this );
    }
    //-------------------------------------------------------------------------
    void UnitTest::notifyRecordingNewFrame( Demo::GraphicsSystem *graphicsSystem )
    {
        Ogre::Camera *camera = graphicsSystem->getCamera();

        if( mFrameActivity.empty() )
            mFrameActivity.push_back( FrameActivity( mFrameIdx ) );

        // Save camera again if a record has already been created, or if the
        // camera changed from last known transform
        if( mFrameActivity.back().frameId == mFrameIdx ||
            camera->getRealPosition() != mFrameActivity.back().cameraPos ||
            camera->getRealOrientation() != mFrameActivity.back().cameraRot )
        {
            if( mFrameActivity.back().frameId != mFrameIdx )
                mFrameActivity.push_back( FrameActivity( mFrameIdx ) );

            mFrameActivity.back().cameraPos = camera->getRealPosition();
            mFrameActivity.back().cameraRot = camera->getRealOrientation();
        }

        ++mFrameIdx;
    }
    //-------------------------------------------------------------------------
    void UnitTest::saveToJson( const char *fullpath, const bool bCompressDuration )
    {
        if( bCompressDuration )
        {
            mNumFrames = static_cast<uint32_t>( mFrameActivity.size() + 3u );
            std::vector<FrameActivity>::iterator itor = mFrameActivity.begin();
            std::vector<FrameActivity>::iterator endt = mFrameActivity.end();

            uint32_t currFrame = 2u;
            while( itor != endt )
            {
                itor->frameId = currFrame++;
                ++itor;
            }
        }
        else
        {
            if( !mFrameActivity.empty() )
                mNumFrames = mFrameActivity.back().frameId + 1u;
        }

        std::string jsonStr;
        saveToJsonStr( jsonStr );

        std::ofstream outFile( fullpath, std::ios::binary | std::ios::out );
        if( !outFile.is_open() )
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_FILE_NOT_FOUND, "UnitTest::saveToJson",
                         "Could not write to file " + Ogre::String( fullpath ) );
        }

        outFile.write( jsonStr.c_str(), static_cast<std::streamsize>( jsonStr.size() ) );
    }
    //-------------------------------------------------------------------------
    bool UnitTest::shouldRecordKey( const SDL_KeyboardEvent &arg )
    {
        // Ignore camera-changing keys
        return arg.keysym.scancode != SDL_SCANCODE_W && arg.keysym.scancode != SDL_SCANCODE_A &&
               arg.keysym.scancode != SDL_SCANCODE_S && arg.keysym.scancode != SDL_SCANCODE_D &&
               arg.keysym.scancode != SDL_SCANCODE_PAGEUP &&
               arg.keysym.scancode != SDL_SCANCODE_PAGEDOWN;
    }
    //-------------------------------------------------------------------------
    void UnitTest::keyPressed( const SDL_KeyboardEvent &arg )
    {
        if( !mParams.isPlayback() )
        {
            if( arg.keysym.sym == SDLK_F12 || arg.keysym.sym == SDLK_PRINTSCREEN )
            {
                // Ignore (see keyReleased)
            }
            else if( shouldRecordKey( arg ) )
            {
                if( mFrameActivity.empty() || mFrameActivity.back().frameId != mFrameIdx )
                    mFrameActivity.push_back( FrameActivity( mFrameIdx ) );

                KeyStroke keyStroke;
                keyStroke.keycode = arg.keysym.sym;
                keyStroke.scancode = arg.keysym.scancode;
                keyStroke.bReleased = arg.type == SDL_KEYUP;
                mFrameActivity.back().keyStrokes.push_back( keyStroke );
            }
        }

        if( !mBlockInputForwarding )
        {
            // Forward to application
            mRealKeyboardListener->keyPressed( arg );
        }
    }
    //-------------------------------------------------------------------------
    void UnitTest::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( !mParams.isPlayback() )
        {
            if( arg.keysym.sym == SDLK_F12 || arg.keysym.sym == SDLK_PRINTSCREEN )
            {
                if( mFrameActivity.empty() || mFrameActivity.back().frameId != mFrameIdx )
                    mFrameActivity.push_back( FrameActivity( mFrameIdx ) );

                mFrameActivity.back().screenshotRenderWindow = true;
            }
            else if( shouldRecordKey( arg ) )
            {
                if( mFrameActivity.empty() || mFrameActivity.back().frameId != mFrameIdx )
                    mFrameActivity.push_back( FrameActivity( mFrameIdx ) );

                KeyStroke keyStroke;
                keyStroke.keycode = arg.keysym.sym;
                keyStroke.scancode = arg.keysym.scancode;
                keyStroke.bReleased = arg.type == SDL_KEYUP;
                mFrameActivity.back().keyStrokes.push_back( keyStroke );
            }
        }

        if( !mBlockInputForwarding )
        {
            // Forward to application
            mRealKeyboardListener->keyReleased( arg );
        }
    }
    //-------------------------------------------------------------------------
    void UnitTest::mouseMoved( const SDL_Event &arg )
    {
        if( !mParams.isPlayback() )
            mRealMouseListener->mouseMoved( arg );
    }
    //-------------------------------------------------------------------------
    void UnitTest::mousePressed( const SDL_MouseButtonEvent &arg, Ogre::uint8 id )
    {
        if( !mParams.isPlayback() )
            mRealMouseListener->mousePressed( arg, id );
    }
    //-------------------------------------------------------------------------
    void UnitTest::mouseReleased( const SDL_MouseButtonEvent &arg, Ogre::uint8 id )
    {
        if( !mParams.isPlayback() )
            mRealMouseListener->mouseReleased( arg, id );
    }
    //-------------------------------------------------------------------------
    int UnitTest::loadFromJson( const char *fullpath, const Ogre::String &outputFolder )
    {
#if !OGRE_NO_JSON && defined( USE_JSON_UNIT_TESTING )
        rapidjson::Document d;

        std::ifstream inFile( fullpath, std::ios::binary | std::ios::in );
        if( !inFile.is_open() )
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_FILE_NOT_FOUND, "UnitTest::loadFromJson",
                         "Could not open file " + Ogre::String( fullpath ) );
        }

        std::vector<char> fileData;
        inFile.seekg( 0, std::ios::end );
        fileData.resize( static_cast<size_t>( inFile.tellg() ) );
        inFile.seekg( 0, std::ios::beg );

        inFile.read( &fileData[0], static_cast<std::streamsize>( fileData.size() ) );
        inFile.close();
        fileData.push_back( '\0' );  // Add null terminator just in case

        d.Parse( &fileData[0] );

        if( d.HasParseError() )
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_INVALIDPARAMS, "SceneFormatImporter::importScene",
                         "Invalid JSON string in file " + Ogre::String( fullpath ) + " at line " +
                             Ogre::StringConverter::toString( d.GetErrorOffset() ) +
                             " Reason: " + rapidjson::GetParseError_En( d.GetParseError() ) );
        }

        rapidjson::Value::ConstMemberIterator itor;

        itor = d.FindMember( "framerate" );
        if( itor != d.MemberEnd() && itor->value.IsDouble() )
        {
            const double framerate = itor->value.GetDouble();
            mFrametime = 1.0 / ( framerate == 0 ? 60.0 : itor->value.GetDouble() );
        }

        itor = d.FindMember( "num_frames" );
        if( itor != d.MemberEnd() && itor->value.IsUint() )
            mNumFrames = static_cast<uint32_t>( itor->value.GetUint() );

        itor = d.FindMember( "frame_activity" );
        if( itor != d.MemberEnd() && itor->value.IsArray() )
        {
            const rapidjson::Value &frameActObjArray = itor->value;
            const size_t arraySize = frameActObjArray.Size();

            mFrameActivity.reserve( arraySize );

            for( rapidjson::SizeType i = 0u; i < arraySize; ++i )
            {
                const rapidjson::Value &frameActObj = frameActObjArray[i];

                if( frameActObj.IsObject() )
                {
                    FrameActivity frameActivity( 0u );

                    itor = frameActObj.FindMember( "frame_id" );
                    if( itor != frameActObj.MemberEnd() && itor->value.IsUint() )
                        frameActivity.frameId = static_cast<uint32_t>( itor->value.GetUint() );

                    itor = frameActObj.FindMember( "camera_pos" );
                    if( itor != frameActObj.MemberEnd() && itor->value.IsArray() &&
                        itor->value.Size() == 3u &&   //
                        itor->value[0].IsDouble() &&  //
                        itor->value[1].IsDouble() &&  //
                        itor->value[2].IsDouble() )
                    {
                        frameActivity.cameraPos.x = static_cast<float>( itor->value[0].GetDouble() );
                        frameActivity.cameraPos.y = static_cast<float>( itor->value[1].GetDouble() );
                        frameActivity.cameraPos.z = static_cast<float>( itor->value[2].GetDouble() );
                    }

                    itor = frameActObj.FindMember( "camera_rot" );
                    if( itor != frameActObj.MemberEnd() && itor->value.IsArray() &&
                        itor->value.Size() == 4u &&   //
                        itor->value[0].IsDouble() &&  //
                        itor->value[1].IsDouble() &&  //
                        itor->value[2].IsDouble() &&  //
                        itor->value[3].IsDouble() )
                    {
                        frameActivity.cameraRot.w = static_cast<float>( itor->value[0].GetDouble() );
                        frameActivity.cameraRot.x = static_cast<float>( itor->value[1].GetDouble() );
                        frameActivity.cameraRot.y = static_cast<float>( itor->value[2].GetDouble() );
                        frameActivity.cameraRot.z = static_cast<float>( itor->value[3].GetDouble() );
                    }

                    itor = frameActObj.FindMember( "screenshot_render_window" );
                    if( itor != frameActObj.MemberEnd() && itor->value.IsBool() )
                        frameActivity.screenshotRenderWindow = itor->value.GetBool();

                    itor = frameActObj.FindMember( "key_strokes" );
                    if( itor != frameActObj.MemberEnd() && itor->value.IsArray() )
                    {
                        const rapidjson::Value &keyStrokeArray = itor->value;
                        const rapidjson::SizeType numCodes = keyStrokeArray.Size();
                        frameActivity.keyStrokes.reserve( numCodes );

                        for( rapidjson::SizeType j = 0u; j < numCodes; ++j )
                        {
                            if( keyStrokeArray[j].IsObject() )
                            {
                                const rapidjson::Value &keyStrokeObj = keyStrokeArray[j];
                                KeyStroke keyStroke;

                                itor = keyStrokeObj.FindMember( "key_code" );
                                if( itor != keyStrokeObj.MemberEnd() && itor->value.IsInt() )
                                    keyStroke.keycode = static_cast<int32_t>( itor->value.GetInt() );

                                itor = keyStrokeObj.FindMember( "scan_code" );
                                if( itor != keyStrokeObj.MemberEnd() && itor->value.IsUint() )
                                    keyStroke.scancode = static_cast<uint16_t>( itor->value.GetUint() );

                                itor = keyStrokeObj.FindMember( "released" );
                                if( itor != keyStrokeObj.MemberEnd() && itor->value.IsBool() )
                                    keyStroke.bReleased = static_cast<uint16_t>( itor->value.GetBool() );

                                frameActivity.keyStrokes.push_back( keyStroke );
                            }
                        }
                    }

                    itor = frameActObj.FindMember( "targets" );
                    if( itor != frameActObj.MemberEnd() && itor->value.IsArray() )
                    {
                        const rapidjson::SizeType numCodes = itor->value.Size();
                        frameActivity.targetsToScreenshot.reserve( numCodes );
                        for( rapidjson::SizeType j = 0u; j < numCodes; ++j )
                        {
                            if( itor->value[j].IsString() )
                            {
                                frameActivity.targetsToScreenshot.push_back(
                                    itor->value[j].GetString() );
                            }
                        }
                    }

                    mFrameActivity.push_back( frameActivity );
                }
            }
        }
#else
        OGRE_EXCEPT( Ogre::Exception::ERR_INVALID_CALL, "UnitTest::loadFromJson",
                     "UnitTest playback requires compiling Ogre with OGRE_CONFIG_ENABLE_JSON" );
#endif

        return runLoop( outputFolder );
    }
    //-------------------------------------------------------------------------
    int UnitTest::runLoop( Ogre::String outputFolder )
    {
        if( outputFolder.empty() )
            outputFolder = "./";
        else if( outputFolder[outputFolder.size() - 1u] != '/' )
            outputFolder += "/";

        GameState *graphicsGameState = 0;
        GraphicsSystem *graphicsSystem = 0;
        GameState *logicGameState = 0;
        LogicSystem *logicSystem = 0;

        MainEntryPoints::createSystems( &graphicsGameState, &graphicsSystem, &logicGameState,
                                        &logicSystem );

        try
        {
            graphicsSystem->setAlwaysAskForConfig( false );
            graphicsSystem->initialize( Demo::MainEntryPoints::getWindowTitle() );
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

            Ogre::Window *renderWindow = graphicsSystem->getRenderWindow();

            graphicsSystem->createScene01();
            if( logicSystem )
                logicSystem->createScene01();

            graphicsSystem->createScene02();
            if( logicSystem )
                logicSystem->createScene02();

            SdlInputHandler *inputHandler = graphicsSystem->getInputHandler();
            mRealKeyboardListener = inputHandler->getKeyboardListener();
            mRealMouseListener = inputHandler->getMouseListener();
            inputHandler->_overrideKeyboardListener( this );
            inputHandler->_overrideMouseListener( this );

            renderWindow->setWantsToDownload( true );
            renderWindow->setManualSwapRelease( true );

            const size_t numFrames = mNumFrames;
            MainEntryPoints::Frametime = mFrametime;

            Ogre::Root *root = graphicsSystem->getRoot();
            Ogre::TextureGpuManager *textureManager = root->getRenderSystem()->getTextureGpuManager();

            mBlockInputForwarding = true;

            std::vector<FrameActivity>::const_iterator frameActivity = mFrameActivity.begin();

            for( size_t frameIdx = 0u; frameIdx < numFrames; ++frameIdx )
            {
                mFrameIdx = static_cast<uint32_t>( frameIdx );

                if( frameActivity != mFrameActivity.end() && frameIdx == frameActivity->frameId )
                {
                    Ogre::Camera *camera = graphicsSystem->getCamera();

                    // Camera may be attached to a node, which is why we call it twice.
                    // We need to ensure the derived position is respected
                    camera->setPosition( frameActivity->cameraPos );
                    camera->setPosition( frameActivity->cameraPos + frameActivity->cameraPos -
                                         camera->getRealPosition() );

                    camera->setOrientation( camera->getParentNode()->_getDerivedOrientation().Inverse() *
                                            frameActivity->cameraRot );
                }

                if( logicSystem )
                {
                    logicSystem->beginFrameParallel();
                    logicSystem->update( static_cast<float>( MainEntryPoints::Frametime ) );
                    logicSystem->finishFrameParallel();

                    logicSystem->finishFrame();
                    graphicsSystem->finishFrame();
                }

                const Ogre::FrameStats *frameStats = root->getFrameStats();
                // A bit hacky to const_cast, but we're intentionally tampering something we shouldn't
                // in order to force deterministic output
                Ogre::FrameStats *frameStatsNonConst = const_cast<Ogre::FrameStats *>( frameStats );
                frameStatsNonConst->reset( 0 );

                graphicsSystem->beginFrameParallel();
                if( frameActivity != mFrameActivity.end() && frameIdx == frameActivity->frameId )
                {
                    SDL_Event evt;
                    memset( &evt, 0, sizeof( evt ) );

                    mBlockInputForwarding = false;
                    std::vector<KeyStroke>::const_iterator itor = frameActivity->keyStrokes.begin();
                    std::vector<KeyStroke>::const_iterator endt = frameActivity->keyStrokes.end();

                    while( itor != endt )
                    {
                        if( !itor->bReleased )
                            evt.type = SDL_KEYDOWN;
                        else
                            evt.type = SDL_KEYUP;
                        evt.key.keysym.sym = itor->keycode;
                        evt.key.keysym.scancode = static_cast<SDL_Scancode>( itor->scancode );
                        inputHandler->_handleSdlEvents( evt );
                        ++itor;
                    }
                    mBlockInputForwarding = true;
                }
                graphicsSystem->update( static_cast<float>( MainEntryPoints::Frametime ) );
                graphicsSystem->finishFrameParallel();
                if( !logicSystem )
                    graphicsSystem->finishFrame();

                if( !renderWindow->isVisible() )
                    renderWindow->setFocused( true );

                // We must do this to ensure determinism, though it may mean
                // we miss some coverage due to race conditions
                textureManager->waitForStreamingCompletion();

                if( frameActivity != mFrameActivity.end() && frameIdx == frameActivity->frameId )
                {
                    if( !mParams.bSkipDump )
                    {
                        const Ogre::String frameIdxStr( Ogre::StringConverter::toString( frameIdx ) +
                                                        "_" );
                        if( frameActivity->screenshotRenderWindow && renderWindow->canDownloadData() )
                        {
                            Ogre::Image2 img;
                            Ogre::TextureGpu *texture;

                            texture = renderWindow->getTexture();
                            img.convertFromTexture( texture, 0u, texture->getNumMipmaps() - 1u );
                            img.save( outputFolder + frameIdxStr + "RenderWindow_colour.png", 0u,
                                      texture->getNumMipmaps() );

                            /*texture = renderWindow->getDepthBuffer();
                            img.convertFromTexture( texture, 0u, texture->getNumMipmaps() - 1u );
                            img.save( outputFolder + frameIdxStr + "RenderWindow_depth.exr", 0u,
                                      texture->getNumMipmaps() );*/
                        }

                        Ogre::StringVector::const_iterator itor =
                            frameActivity->targetsToScreenshot.begin();
                        Ogre::StringVector::const_iterator endt =
                            frameActivity->targetsToScreenshot.end();

                        while( itor != endt )
                        {
                            Ogre::TextureGpu *texture = textureManager->findTextureNoThrow( *itor );
                            if( !texture )
                            {
                                OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND, "UnitTest::runLoop",
                                             "Could not find texture " + *itor );
                            }

                            Ogre::Image2 img;
                            img.convertFromTexture( texture, 0u, texture->getNumMipmaps() - 1u );
                            img.save( outputFolder + frameIdxStr + *itor + ".oitd", 0u,
                                      texture->getNumMipmaps() );

                            ++itor;
                        }
                    }
                    ++frameActivity;
                }

                renderWindow->performManualRelease();
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
}  // namespace Demo
