/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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

#ifndef _Demo_UnitTesting_H_
#define _Demo_UnitTesting_H_

#include "OgrePrerequisites.h"

#include "InputListeners.h"

#include "OgreQuaternion.h"
#include "OgreStringVector.h"
#include "OgreVector3.h"

#include <stdint.h>

namespace Demo
{
    class GraphicsSystem;
    class KeyboardListener;

    /**
    Usage:
        To record:
            Start app with --ut_record=/home/username/Ogre/pbs.json --ut_compress
            Move around and press F12 to take a picture
        To playback:
            --ut_playback=/home/username/Ogre/pbs.json --ut_output=/home/username/Ogre/

        ut_compress is optional (recommended)
        ut_output is where the the app should dump the captures (i.e. where you pressed F12)
        ut_skip_dump can be used in playback to skip dumping (i.e. to debug an error)
    */
    class UnitTest : public KeyboardListener, public MouseListener
    {
        struct KeyStroke
        {
            int32_t keycode;    // SDL_Keycode
            uint16_t scancode;  // SDL_Scancode
            bool bReleased;
            KeyStroke();
        };

        struct FrameActivity
        {
            uint32_t frameId;
            Ogre::Vector3 cameraPos;
            Ogre::Quaternion cameraRot;
            std::vector<KeyStroke> keyStrokes;
            bool screenshotRenderWindow;
            Ogre::StringVector targetsToScreenshot;
            FrameActivity( uint32_t _frameId );
        };

    public:
        struct Params
        {
            bool bRecord;
            bool bCompressDuration;
            bool bSkipDump;  // Useful for debugging a record showing something that is broken
            std::string recordPath;
            std::string outputPath;
            Params();

            bool isRecording() const;
            bool isPlayback() const;
        };

    protected:
        double mFrametime;
        uint32_t mFrameIdx;
        uint32_t mNumFrames;
        KeyboardListener *mRealKeyboardListener;
        MouseListener *mRealMouseListener;

        std::vector<FrameActivity> mFrameActivity;

        Params mParams;
        bool mBlockInputForwarding;

        inline static void flushLwString( Ogre::LwString &jsonStr, std::string &outJson );

        void exportFrameActivity( const FrameActivity &frameActivity, Ogre::LwString &jsonStr,
                                  std::string &outJson );
        void saveToJsonStr( std::string &outJson );

        static bool shouldRecordKey( const SDL_KeyboardEvent &arg );
        static Ogre::Vector3 getCameraRecordPosition( Ogre::Camera *camera );

    public:
        UnitTest();

        void parseCmdLine( int nargs, const char *const *argv );

        const Params &getParams( void ) const { return mParams; }

        void startRecording( Demo::GraphicsSystem *graphicsSystem );
        void notifyRecordingNewFrame( Demo::GraphicsSystem *graphicsSystem );

        /** Saves the current recording to JSON, for later playback
        @param fullpath
            Full path where to save the JSON file
        @param bCompressDuration
            When true, we assume mNumFrames can be reduced to a minimum. Thus if recording
            took 70 seconds but there's only 8 FrameActivity, then it will be
            compressed to roughly 11 frames.
        */
        void saveToJson( const char *fullpath, const bool bCompressDuration );

        virtual void keyPressed( const SDL_KeyboardEvent &arg );
        virtual void keyReleased( const SDL_KeyboardEvent &arg );
        virtual void mouseMoved( const SDL_Event &arg );
        virtual void mousePressed( const SDL_MouseButtonEvent &arg, Ogre::uint8 id );
        virtual void mouseReleased( const SDL_MouseButtonEvent &arg, Ogre::uint8 id );

        /** Loads JSON from fullpath and plays it back, saving the results to outputFolder
            Return value is the return value for main()
        */
        int loadFromJson( const char *fullpath, const Ogre::String &outputFolder );

    protected:
        int runLoop( Ogre::String outputFolder );
    };
}  // namespace Demo

#endif
