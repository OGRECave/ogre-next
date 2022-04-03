/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2008 Renato Araujo Oliveira Filho <renatox@gmail.com>
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

#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"
#include "OgreRoot.h"

#include "OgreGLES2Prerequisites.h"
#include "OgreGLES2RenderSystem.h"

#include "OgreEGLSupport.h"
#include "OgreEGLWindow.h"
#include "OgreEGLRenderTexture.h"


namespace Ogre {


    EGLSupport::EGLSupport()
        : mGLDisplay(0),
          mNativeDisplay(0),
      mRandr(false)
    {
    }

    EGLSupport::~EGLSupport()
    {
    }

    void EGLSupport::addConfig()
    {
        ConfigOption optFullScreen;
        ConfigOption optVideoMode;
        ConfigOption optDisplayFrequency;
        ConfigOption optFSAA;
#if GL_OES_packed_depth_stencil
        ConfigOption optRTTMode;
#endif

        optFullScreen.name = "Full Screen";
        optFullScreen.immutable = false;

        optVideoMode.name = "Video Mode";
        optVideoMode.immutable = false;

        optDisplayFrequency.name = "Display Frequency";
        optDisplayFrequency.immutable = false;

        optFSAA.name = "FSAA";
        optFSAA.immutable = false;

#if GL_OES_packed_depth_stencil
        optRTTMode.name = "RTT Preferred Mode";
        optRTTMode.possibleValues.push_back("FBO");
        optRTTMode.possibleValues.push_back("Copy");
        optRTTMode.currentValue = "FBO";
        optRTTMode.immutable = false;
        optRTTMode.currentValue = optRTTMode.possibleValues[0];
#endif
        optFullScreen.possibleValues.push_back("No");
        optFullScreen.possibleValues.push_back("Yes");

        optFullScreen.currentValue = optFullScreen.possibleValues[1];

        VideoModes::const_iterator value = mVideoModes.begin();
        VideoModes::const_iterator end = mVideoModes.end();

        for (; value != end; value++)
        {
            String mode = StringConverter::toString(value->first.first,4) + " x " + StringConverter::toString(value->first.second,4);
            optVideoMode.possibleValues.push_back(mode);
        }
        removeDuplicates(optVideoMode.possibleValues);

        optVideoMode.currentValue = StringConverter::toString(mCurrentMode.first.first,4) + " x " + StringConverter::toString(mCurrentMode.first.second,4);

        refreshConfig();
        if (!mSampleLevels.empty())
        {
            StringVector::const_iterator sampleValue = mSampleLevels.begin();
            StringVector::const_iterator sampleEnd = mSampleLevels.end();

            for (; sampleValue != sampleEnd; sampleValue++)
            {
                optFSAA.possibleValues.push_back(*sampleValue);
            }

            optFSAA.currentValue = optFSAA.possibleValues[0];
        }


        mOptions[optFullScreen.name] = optFullScreen;
        mOptions[optVideoMode.name] = optVideoMode;
        mOptions[optDisplayFrequency.name] = optDisplayFrequency;
        mOptions[optFSAA.name] = optFSAA;
#if GL_OES_packed_depth_stencil
        mOptions[optRTTMode.name] = optRTTMode;
#endif
        refreshConfig();
    }

    void EGLSupport::refreshConfig()
    {
        ConfigOptionMap::iterator optVideoMode = mOptions.find("Video Mode");
        ConfigOptionMap::iterator optDisplayFrequency = mOptions.find("Display Frequency");

        if (optVideoMode != mOptions.end() && optDisplayFrequency != mOptions.end())
        {
            optDisplayFrequency->second.possibleValues.clear();

            VideoModes::const_iterator value = mVideoModes.begin();
            VideoModes::const_iterator end = mVideoModes.end();

            for (; value != end; value++)
            {
                String mode = StringConverter::toString(value->first.first,4) + " x " + StringConverter::toString(value->first.second,4);

                if (mode == optVideoMode->second.currentValue)
                {
                    String frequency = StringConverter::toString(value->second) + " MHz";

                    optDisplayFrequency->second.possibleValues.push_back(frequency);
                }
            }

            if (!optDisplayFrequency->second.possibleValues.empty())
            {
                optDisplayFrequency->second.currentValue = optDisplayFrequency->second.possibleValues[0];
            }
            else
            {
                optVideoMode->second.currentValue = StringConverter::toString(mVideoModes[0].first.first,4) + " x " + StringConverter::toString(mVideoModes[0].first.second,4);
                optDisplayFrequency->second.currentValue = StringConverter::toString(mVideoModes[0].second) + " MHz";
            }
        }
    }

    void EGLSupport::setConfigOption(const String &name, const String &value)
    {
        GLES2Support::setConfigOption(name, value);
        if (name == "Video Mode")
        {
            refreshConfig();
        }
    }

    String EGLSupport::validateConfig()
    {
        // TODO
        return BLANKSTRING;
    }

    EGLDisplay EGLSupport::getGLDisplay()
    {
        EGLint major = 0, minor = 0;

        mGLDisplay = eglGetDisplay(mNativeDisplay);
        EGL_CHECK_ERROR

        if(mGLDisplay == EGL_NO_DISPLAY)
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "Couldn`t open EGLDisplay " + getDisplayName(),
                        "EGLSupport::getGLDisplay");
        }

        if (eglInitialize(mGLDisplay, &major, &minor) == EGL_FALSE)
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "Couldn`t initialize EGLDisplay ",
                        "EGLSupport::getGLDisplay");
        }
        EGL_CHECK_ERROR
        return mGLDisplay;
    }


    String EGLSupport::getDisplayName()
    {
        return "todo";
    }

    EGLConfig* EGLSupport::chooseGLConfig(const GLint *attribList, GLint *nElements)
    {
        EGLConfig *configs;

        if (eglChooseConfig(mGLDisplay, attribList, NULL, 0, nElements) == EGL_FALSE)
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "Failed to choose config",
                        __FUNCTION__);

            *nElements = 0;
            return 0;
        }
        EGL_CHECK_ERROR
        configs = (EGLConfig*) malloc(*nElements * sizeof(EGLConfig));
        if (eglChooseConfig(mGLDisplay, attribList, configs, *nElements, nElements) == EGL_FALSE)
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "Failed to choose config",
                        __FUNCTION__);

            *nElements = 0;
            free(configs);
            return 0;
        }
        EGL_CHECK_ERROR
        return configs;
    }

    EGLConfig* EGLSupport::getConfigs(GLint *nElements)
    {
        EGLConfig *configs;

        if (eglGetConfigs(mGLDisplay, NULL, 0, nElements) == EGL_FALSE)
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "Failed to choose config",
                        __FUNCTION__);

            *nElements = 0;
            return 0;
        }
        EGL_CHECK_ERROR
        configs = (EGLConfig*) malloc(*nElements * sizeof(EGLConfig));
        if (eglGetConfigs(mGLDisplay, configs, *nElements, nElements) == EGL_FALSE)
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "Failed to choose config",
                        __FUNCTION__);

            *nElements = 0;
            free(configs);
            return 0;
        }
        EGL_CHECK_ERROR
        return configs;
    }

    EGLBoolean EGLSupport::getGLConfigAttrib(EGLConfig glConfig, GLint attribute, GLint *value)
    {
        EGLBoolean status;

        status = eglGetConfigAttrib(mGLDisplay, glConfig, attribute, value);
        EGL_CHECK_ERROR
        return status;
    }

    void* EGLSupport::getProcAddress(const char* procname) const
    {
        return (void*)eglGetProcAddress(procname);
    }

    ::EGLConfig EGLSupport::getGLConfigFromContext(::EGLContext context)
    {
        ::EGLConfig glConfig = 0;

        if (eglQueryContext(mGLDisplay, context, EGL_CONFIG_ID, (EGLint *) &glConfig) == EGL_FALSE)
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "Fail to get config from context",
                        __FUNCTION__);
            return 0;
        }
        EGL_CHECK_ERROR
        return glConfig;
    }

    ::EGLConfig EGLSupport::getGLConfigFromDrawable(::EGLSurface drawable,
                                                    unsigned int *w, unsigned int *h)
    {
        ::EGLConfig glConfig = 0;

        if (eglQuerySurface(mGLDisplay, drawable, EGL_CONFIG_ID, (EGLint *) &glConfig) == EGL_FALSE)
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "Fail to get config from drawable",
                        __FUNCTION__);
            return 0;
        }
        EGL_CHECK_ERROR
        eglQuerySurface(mGLDisplay, drawable, EGL_WIDTH, (EGLint *) w);
        EGL_CHECK_ERROR
        eglQuerySurface(mGLDisplay, drawable, EGL_HEIGHT, (EGLint *) h);
        EGL_CHECK_ERROR
        return glConfig;
    }

    //------------------------------------------------------------------------
    // A helper class for the implementation of selectFBConfig
    //------------------------------------------------------------------------
    class GLConfigAttribs
    {
        public:
            GLConfigAttribs(const int* attribs)
            {
                fields[EGL_CONFIG_CAVEAT] = EGL_NONE;

                for (int i = 0; attribs[2*i] != EGL_NONE; i++)
                {
                    fields[attribs[2*i]] = attribs[2*i+1];
                }
            }

            void load(EGLSupport* const glSupport, EGLConfig glConfig)
            {
                std::map<int,int>::iterator it;

                for (it = fields.begin(); it != fields.end(); it++)
                {
                    it->second = EGL_NONE;

                    glSupport->getGLConfigAttrib(glConfig, it->first, &it->second);
                }
            }

            bool operator>(GLConfigAttribs& alternative)
            {
                // Caveats are best avoided, but might be needed for anti-aliasing
                if (fields[EGL_CONFIG_CAVEAT] != alternative.fields[EGL_CONFIG_CAVEAT])
                {
                    if (fields[EGL_CONFIG_CAVEAT] == EGL_SLOW_CONFIG)
                    {
                        return false;
                    }

                    if (fields.find(EGL_SAMPLES) != fields.end() &&
                        fields[EGL_SAMPLES] < alternative.fields[EGL_SAMPLES])
                    {
                        return false;
                    }
                }

                std::map<int,int>::iterator it;

                for (it = fields.begin(); it != fields.end(); it++)
                {
                    if (it->first != EGL_CONFIG_CAVEAT &&
                        fields[it->first] > alternative.fields[it->first])
                    {
                        return true;
                    }
                }

                return false;
            }

            std::map<int,int> fields;
    };

    ::EGLConfig EGLSupport::selectGLConfig(const int* minAttribs, const int *maxAttribs)
    {
        EGLConfig *glConfigs;
        EGLConfig glConfig = 0;
        int config, nConfigs = 0;

        glConfigs = chooseGLConfig(minAttribs, &nConfigs);

        if (!nConfigs)
        {
            glConfigs = getConfigs(&nConfigs);
        }

        if (!nConfigs)
        {
            return 0;
        }

        glConfig = glConfigs[0];

        if (maxAttribs)
        {
            GLConfigAttribs maximum(maxAttribs);
            GLConfigAttribs best(maxAttribs);
            GLConfigAttribs candidate(maxAttribs);

            best.load(this, glConfig);

            for (config = 1; config < nConfigs; config++)
            {
                candidate.load(this, glConfigs[config]);

                if (candidate > maximum)
                {
                    continue;
                }

                if (candidate > best)
                {
                    glConfig = glConfigs[config];

                    best.load(this, glConfig);
                }
            }
        }

        free(glConfigs);
        return glConfig;
    }

    void EGLSupport::switchMode()
    {
        return switchMode(mOriginalMode.first.first,
                          mOriginalMode.first.second, mOriginalMode.second);
    }

    RenderWindow* EGLSupport::createWindow(bool autoCreateWindow,
                                           GLES2RenderSystem* renderSystem,
                                           const String& windowTitle)
    {
        RenderWindow *window = 0;

        if (autoCreateWindow)
        {
            ConfigOptionMap::iterator opt;
            ConfigOptionMap::iterator end = mOptions.end();
            NameValuePairList miscParams;

            bool fullscreen = false;
            uint w = 640, h = 480;

            if ((opt = mOptions.find("Full Screen")) != end)
            {
                fullscreen = (opt->second.currentValue == "Yes");
            }

            if ((opt = mOptions.find("Display Frequency")) != end)
            {
                miscParams["displayFrequency"] = opt->second.currentValue;
            }

            if ((opt = mOptions.find("Video Mode")) != end)
            {
                String val = opt->second.currentValue;
                String::size_type pos = val.find('x');

                if (pos != String::npos)
                {
                    w = StringConverter::parseUnsignedInt(val.substr(0, pos));
                    h = StringConverter::parseUnsignedInt(val.substr(pos + 1));
                }
            }

            if ((opt = mOptions.find("FSAA")) != end)
            {
                miscParams["FSAA"] = opt->second.currentValue;
            }

            window = renderSystem->_createRenderWindow(windowTitle, w, h, fullscreen, &miscParams);
        }

        return window;
    }

    ::EGLContext EGLSupport::createNewContext(EGLDisplay eglDisplay,
                          ::EGLConfig glconfig,
                                              ::EGLContext shareList) const 
    {
        EGLint contextAttrs[] = {
#if OGRE_NO_GLES3_SUPPORT == 0
            EGL_CONTEXT_CLIENT_VERSION, 3,
#else
            EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
            EGL_NONE, EGL_NONE
        };
        ::EGLContext context = ((::EGLContext) 0);
        if (eglDisplay == ((EGLDisplay) 0))
        {
            context = eglCreateContext(mGLDisplay, glconfig, shareList, contextAttrs);
            EGL_CHECK_ERROR
        }
        else
        {
            context = eglCreateContext(eglDisplay, glconfig, 0, contextAttrs);
            EGL_CHECK_ERROR
        }

        if (context == ((::EGLContext) 0))
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "Fail to create New context",
                        __FUNCTION__);
            return 0;
        }

        return context;
    }

    void EGLSupport::start()
    {
    }

    void EGLSupport::stop()
    {
        eglTerminate(mGLDisplay);
        EGL_CHECK_ERROR
    }

    void EGLSupport::setGLDisplay( EGLDisplay val )
    {
        mGLDisplay = val;
    }
}
