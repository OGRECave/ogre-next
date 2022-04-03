/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

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


#include "TestContext.h"
#include "SamplePlugin.h"
#include "TestResultWriter.h"
#include "HTMLWriter.h"
#include "CppUnitResultWriter.h"
#include "OgreConfigFile.h"
#include "OgrePlatform.h"
#include <iostream>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#if (OGRE_PLATFORM == OGRE_PLATFORM_APPLE) && __LP64__
#include <AppKit/AppKit.h>
static id mAppDelegate;
#endif

#ifdef OGRE_STATIC_LIB
#include "VTestPlugin.h"
#include "PlayPenTestPlugin.h"
#endif

TestContext::TestContext(int argc, char** argv) : mTimestep(0.01f), mOutputDir(BLANKSTRING), mCurrentTest(0), mBatch(0)
{
    Ogre::UnaryOptionList unOpt;
    Ogre::BinaryOptionList binOpt;

    // Prepopulate expected options.
    unOpt["-r"] = false;        // generate reference set
    unOpt["--no-html"] = false; // whether or not to generate HTML
    unOpt["-d"] = false;        // force config dialog
    unOpt["--nograb"] = false;  // do not grab mouse
    unOpt["-h"] = false;        // help, give usage details
    unOpt["--help"] = false;    // help, give usage details
    binOpt["-m"] = "";          // optional comment
    binOpt["-rp"] = "";         // optional specified reference set location
    binOpt["-od"] = "";         // directory to write output to
    binOpt["-ts"] = "VTests";   // name of the test set to use
    binOpt["-c"] = "Reference"; // name of batch to compare against
    binOpt["-n"] = "AUTO";      // name for this batch
    binOpt["-rs"] = "SAVED";    // rendersystem to use (default: use name from the config file/dialog)
    binOpt["-o"] = "NONE";      // path to output a summary file to (default: don't output a file)

    // Parse.
    Ogre::findCommandLineOpts(argc, argv, unOpt, binOpt);

    mReferenceSet = unOpt["-r"];
    mTestSetName = binOpt["-ts"];
    mComment = binOpt["-m"];
    mGenerateHtml = !unOpt["--no-html"];
    mBatchName = binOpt["-n"];
    mCompareWith = binOpt["-c"];
    mForceConfig = unOpt["-d"];
    mNoGrabMouse = unOpt["--nograb"];
    mOutputDir = binOpt["-od"];
    mRenderSystemName = binOpt["-rs"];
    mReferenceSetPath = binOpt["-rp"];
    mSummaryOutputDir = binOpt["-o"];
    mHelp = unOpt["-h"] || unOpt["--help"];

    if(mReferenceSetPath == BLANKSTRING)
        mReferenceSetPath = mOutputDir;
    
#ifdef INCLUDE_RTSHADER_SYSTEM
    mShaderGenerator     = NULL;
    mMaterialMgrListener = NULL;
#endif // INCLUDE_RTSHADER_SYSTEM
}
//-----------------------------------------------------------------------

TestContext::~TestContext()
{
    if (mBatch)
        delete mBatch;
}
//-----------------------------------------------------------------------

void TestContext::setup()
{
    // Standard setup.
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
    CGSize modeSize = [[UIScreen mainScreen] currentMode].size;
    uint w = modeSize.width / [UIScreen mainScreen].scale;
    uint h = modeSize.height / [UIScreen mainScreen].scale;

    UIInterfaceOrientation orientation = [UIApplication sharedApplication].statusBarOrientation;

    if (UIInterfaceOrientationIsPortrait(orientation))
        std::swap(w, h);

    mRoot->initialise(false, "OGRE Sample Browser");
    NameValuePairList miscParams;
    miscParams["retainedBacking"] = StringConverter::toString(true);
    mWindow = mRoot->createRenderWindow("OGRE Sample Browser", w, h, true, &miscParams);
#else
    mWindow = createWindow();
#endif

    mWindow->setDeactivateOnFocusChange(false);

    // Grab input, since moving the window seemed to change the results (in Linux anyways).
    setupInput(mNoGrabMouse);
    
    locateResources();

    createDummyScene();
#ifdef INCLUDE_RTSHADER_SYSTEM
    if (mRoot->getRenderSystem()->getCapabilities()->hasCapability(Ogre::RSC_FIXED_FUNCTION) == false)
    {
        Ogre::RTShader::ShaderGenerator::getSingletonPtr()->addSceneManager(mRoot->getSceneManager("DummyScene"));
    }
#endif // INCLUDE_RTSHADER_SYSTEM

    loadResources();
    Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);
    mRoot->addFrameListener(this);

#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
    Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);
#endif

    // Get the path and list of test plugins from the config file.
    Ogre::ConfigFile testConfig;
    testConfig.load(mFSLayer->getConfigFilePath("tests.cfg"));
    mPluginDirectory = testConfig.getSetting("TestFolder");

#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE && OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
    if (mPluginDirectory.empty()) mPluginDirectory = ".";   // user didn't specify plugins folder, try current one
#endif

    // add slash or backslash based on platform
    char lastChar = mPluginDirectory[mPluginDirectory.length() - 1];
    if (lastChar != '/' && lastChar != '\\')
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || (OGRE_PLATFORM == OGRE_PLATFORM_WINRT)
        mPluginDirectory += "\\";
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
        mPluginDirectory += "/";
#endif
    }

    Ogre::ConfigFile::SectionIterator sections = testConfig.getSectionIterator();

    // Parse for the test sets and plugins that they're made up of.
    for (; sections.hasMoreElements(); sections.moveNext())
    {
        Ogre::String setName = sections.peekNextKey();
        if (setName != "")
        {
            mTestSets[setName] = Ogre::StringVector();
            Ogre::ConfigFile::SettingsMultiMap::iterator it = sections.peekNextValue()->begin();
            for (; it != sections.peekNextValue()->end(); ++it)
                mTestSets[setName].push_back(it->second);
        }
    }

#ifdef OGRE_STATIC_LIB
    mPluginNameMap["VTests"]       = (OgreBites::SamplePlugin *) OGRE_NEW VTestPlugin();
    mPluginNameMap["PlayPenTests"] = (OgreBites::SamplePlugin *) OGRE_NEW PlaypenTestPlugin();
#endif

    Ogre::String batchName = BLANKSTRING;
    time_t raw = time(0);

    // timestamp for the filename
    char temp[25];
    strftime(temp, 19, "%Y_%m_%d_%H%M_%S", gmtime(&raw));

    // A nicer formatted version for display.
    strftime(temp, 20, "%Y-%m-%d %H:%M:%S", gmtime(&raw));
    Ogre::String timestamp = Ogre::String(temp);

    if(mOutputDir == BLANKSTRING)
    {
        Ogre::String filestamp = Ogre::String(temp);
        // name for this batch (used for naming the directory, and uniquely identifying this batch)
        batchName = mTestSetName + "_" + filestamp;

        if (mReferenceSet)
            batchName = "Reference";
        else if (mBatchName != "AUTO")
            batchName = mBatchName;
    }

    // Set up output directories.
    setupDirectories(batchName);

    // An object storing info about this set.
    mBatch = new TestBatch(batchName, mTestSetName, timestamp,
                           mWindow->getWidth(), mWindow->getHeight(), mOutputDir + batchName + "/");
    mBatch->comment = mComment;

    OgreBites::Sample* firstTest = loadTests(mTestSetName);
    if (firstTest)
        runSample(firstTest);
}
//-----------------------------------------------------------------------

OgreBites::Sample* TestContext::loadTests(Ogre::String set)
{
    OgreBites::Sample* startSample = 0;
    Ogre::StringVector sampleList;

    // Need support for static loading in here!!!!

    // load all of the plugins in the set
    for (Ogre::StringVector::iterator it = mTestSets[set].begin(); it !=
             mTestSets[set].end(); ++it)
    {
        Ogre::String plugin = *it;

        // try loading up the test plugin
        try
        {
#ifdef OGRE_STATIC_LIB
            // in debug, remove any suffix
            if(StringUtil::endsWith(plugin, "_d"))
                plugin = plugin.substr(0, plugin.length()-2);

            OgreBites::SamplePlugin *pluginInstance = (OgreBites::SamplePlugin *) mPluginNameMap[plugin];
            if(pluginInstance)
            {
                //                OgreBites::SamplePlugin* sp = OGRE_NEW OgreBites::SamplePlugin(pluginInstance->getInfo()["Title"]);

                //                sp->addSample(pluginInstance);
                mRoot->installPlugin(pluginInstance);
            }
#else
            mRoot->loadPlugin(mPluginDirectory + plugin);
#endif
        }
        // if it fails, just return right away
        catch (Ogre::Exception e)
        {
            return 0;
        }

        // grab the plugin and cast to SamplePlugin
        Ogre::Plugin* p = mRoot->getInstalledPlugins().back();
        OgreBites::SamplePlugin* sp = dynamic_cast<OgreBites::SamplePlugin*>(p);

        // if something has gone wrong return null
        if (!sp)
            return 0;

        // go through every sample (test) in the plugin...
        OgreBites::SampleSet newSamples = sp->getSamples();
        for (OgreBites::SampleSet::iterator j = newSamples.begin(); j != newSamples.end(); j++)
        {
            // skip it if using wrong rendersystem
            Ogre::String rs = (*j)->getRequiredRenderSystem();
            if(!rs.empty() && rs != mRoot->getRenderSystem()->getName())
                continue;

            // capability check
            try
            {
                (*j)->testCapabilities(mRoot->getRenderSystem()->getCapabilities());
            }
            catch(Ogre::Exception e)
            {
                continue;
            }

            mTests.push_back(*j);
            Ogre::NameValuePairList& info = (*j)->getInfo();   // acquire custom sample info
            Ogre::NameValuePairList::iterator k;

            // give sample default title and category if none found
            k= info.find("Title");
            if (k == info.end() || k->second.empty()) info["Title"] = "Untitled";
            k = info.find("Category");
            if (k == info.end() || k->second.empty()) info["Category"] = "Unsorted";
            k = info.find("Thumbnail");
            if (k == info.end() || k->second.empty()) info["Thumbnail"] = "thumb_error.png";
        }
    }

    // start with the first one on the list
    if (!mTests.empty())
    {
        startSample = mTests.front();
        return startSample;
    }
    else
        return 0;    
}
//-----------------------------------------------------------------------

bool TestContext::frameStarted(const Ogre::FrameEvent& evt)
{
    captureInputDevices();

    // pass a fixed timestep along to the tests
    Ogre::FrameEvent fixed_evt = Ogre::FrameEvent();
    fixed_evt.timeSinceLastFrame = mTimestep;
    fixed_evt.timeSinceLastEvent = mTimestep;

    if (mCurrentTest) // if a test is running
    {
        // track frame number for screenshot purposes
        ++mCurrentFrame;

        // regular update function
        return mCurrentTest->frameStarted(fixed_evt);
    }
    else if (mCurrentSample) // if a generic sample is running
    {
        return mCurrentSample->frameStarted(evt);
    }
    else
    {
        // if no more tests are queued, generate output and exit
        finishedTests();
        return false;
    }
}
//-----------------------------------------------------------------------

bool TestContext::frameEnded(const Ogre::FrameEvent& evt)
{
    // pass a fixed timestep along to the tests
    Ogre::FrameEvent fixed_evt = Ogre::FrameEvent();
    fixed_evt.timeSinceLastFrame = mTimestep;
    fixed_evt.timeSinceLastEvent = mTimestep;

    if (mCurrentTest) // if a test is running
    {
        if (mCurrentTest->isScreenshotFrame(mCurrentFrame))
        {
            // take a screenshot
            Ogre::String filename = mOutputDir + mBatch->name + "/" +
                mCurrentTest->getInfo()["Title"] + "_" +
                Ogre::StringConverter::toString(mCurrentFrame) + ".png";
            // remember the name of the shot, for later comparison purposes
            mBatch->images.push_back(mCurrentTest->getInfo()["Title"] + "_" +
                                     Ogre::StringConverter::toString(mCurrentFrame));
            mWindow->writeContentsToFile(filename);
        }

        if (mCurrentTest->isDone())
        {
#ifdef INCLUDE_RTSHADER_SYSTEM
            mShaderGenerator->removeAllShaderBasedTechniques(); // clear techniques from the RTSS
#endif

            createDummyScene();

            // continue onto the next test
            runSample(0);

            return true;
        }

        // standard update function
        return mCurrentTest->frameEnded(fixed_evt);
    }
    else if (mCurrentSample) // if a generic sample is running
    {
        return mCurrentSample->frameEnded(evt);
    }
    else
    {
        // if no more tests are queued, generate output and exit
        finishedTests();
        return false;
    }
}
//-----------------------------------------------------------------------

void TestContext::runSample(OgreBites::Sample* s)
{
    // reset frame timing
    Ogre::ControllerManager::getSingleton().setFrameDelay(0);
    Ogre::ControllerManager::getSingleton().setTimeFactor(1.f);
    mCurrentFrame = 0;
    destroyDummyScene();

    OgreBites::Sample* sampleToRun = s;

    // If a valid test is passed, then run it
    // If null, grab the next one from the deque
    if (!sampleToRun && !mTests.empty())
    {
        mTests.pop_front();
        if (!mTests.empty())
            sampleToRun = mTests.front();
    }

    // Check if this is a VisualTest
    mCurrentTest = static_cast<VisualTest*>(sampleToRun);

    // Set things up to be deterministic
    if (mCurrentTest)
    {
        // Seed rand with a predictable value
        srand(5); // 5 is completely arbitrary, the idea is simply to use a constant

        // Give a fixed timestep for particles and other time-dependent things in OGRE
        Ogre::ControllerManager::getSingleton().setFrameDelay(mTimestep);
        LogManager::getSingleton().logMessage("----- Running Visual Test " + mCurrentTest->getInfo()["Title"] + " -----");
    }

#ifdef INCLUDE_RTSHADER_SYSTEM
    if (sampleToRun) {
        sampleToRun->setShaderGenerator(mShaderGenerator);
    }
#endif

    SampleContext::runSample(sampleToRun);
}
//-----------------------------------------------------------------------

void TestContext::createRoot()
{
    // note that we use a separate config file here
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
    mRoot = Ogre::Root::getSingletonPtr();
#else
    Ogre::String pluginsPath = Ogre::BLANKSTRING;
#ifndef OGRE_STATIC_LIB
    pluginsPath = mFSLayer->getConfigFilePath("plugins.cfg");
#endif
    // we use separate config and log files for the tests
    mRoot = OGRE_NEW Ogre::Root(pluginsPath, mFSLayer->getWritablePath("ogretests.cfg"),
                                mFSLayer->getWritablePath("ogretests.log"));
#endif

#ifdef OGRE_STATIC_LIB
    mStaticPluginLoader.load();
#endif
    mOverlaySystem = OGRE_NEW Ogre::OverlaySystem();
}
//-----------------------------------------------------------------------

void TestContext::go(OgreBites::Sample* initialSample)
{
    // Either start up as usual or print usage details.
    if (!mHelp)
    {
        SampleContext::go(initialSample);
    }
    else
    {
        std::cout<<"\nOgre Visual Testing Context:\n";
        std::cout<<"Runs sets of visual test scenes, taking screenshots, and running comparisons.\n\n";
        std::cout<<"Usage: TestContext [opts]\n\n";
        std::cout<<"Options:\n";
        std::cout<<"\t-r           Generate reference set.\n";
        std::cout<<"\t--no-html    Suppress html output.\n";
        std::cout<<"\t-d           Force config dialog.\n";
        std::cout<<"\t-h, --help   Show usage details.\n";
        std::cout<<"\t-m [comment] Optional comment.\n";
        std::cout<<"\t-ts [name]   Name of the test set to use (defined in tests.cfg).\n";
        std::cout<<"\t-c [name]    Name of the test result batch to compare against.\n";
        std::cout<<"\t-n [name]    Name for this result image set.\n";
        std::cout<<"\t-rs [name]   Render system to use.\n";
        std::cout<<"\t-o [path]    Path to output a simple summary file to.\n";
        std::cout<<"\t--nograb     Do not restrict mouse to window (warning: may affect results).\n\n";
    }
}
//-----------------------------------------------------------------------

bool TestContext::oneTimeConfig()
{
    // if forced, just do it and return
    if(mForceConfig)
    {
        bool temp = mRoot->showConfigDialog();
        if(!temp)
            mRoot->setRenderSystem(0);
        return temp;
    }

    // try restore
    bool success = mRoot->restoreConfig();

    // if restoring failed, show the dialog
    if(!success)
        success = mRoot->showConfigDialog();

    // set render system if user-defined
    if(success && mRenderSystemName != "SAVED" && mRoot->getRenderSystemByName(mRenderSystemName))
        mRoot->setRenderSystem(mRoot->getRenderSystemByName(mRenderSystemName));
    else if(!success)
        mRoot->setRenderSystem(0);

    mRenderSystemName = mRoot->getRenderSystem() ? mRoot->getRenderSystem()->getName() : "";

    return success;
}
//-----------------------------------------------------------------------

void TestContext::setupDirectories(Ogre::String batchName)
{
    // ensure there's a root directory for visual tests
    if(mOutputDir == BLANKSTRING)
    {
        mOutputDir = mFSLayer->getWritablePath("VisualTests/");
        static_cast<Ogre::FileSystemLayer*>(mFSLayer)->createDirectory(mOutputDir);

        // make sure there's a directory for the test set
        mOutputDir += mTestSetName + "/";
        static_cast<Ogre::FileSystemLayer*>(mFSLayer)->createDirectory(mOutputDir);

        // add a directory for the render system
        Ogre::String rsysName = Ogre::Root::getSingleton().getRenderSystem()->getName();
        // strip spaces from render system name
        for (unsigned int i = 0;i < rsysName.size(); ++i)
            if (rsysName[i] != ' ')
                mOutputDir += rsysName[i];
        mOutputDir += "/";
        static_cast<Ogre::FileSystemLayer*>(mFSLayer)->createDirectory(mOutputDir);
    }

    if(mSummaryOutputDir != "NONE")
    {
        static_cast<Ogre::FileSystemLayer*>(mFSLayer)->createDirectory(mSummaryOutputDir);
    }

    // and finally a directory for the test batch itself
    static_cast<Ogre::FileSystemLayer*>(mFSLayer)->createDirectory(mOutputDir
                                                                   + batchName + "/");
}
//-----------------------------------------------------------------------

void TestContext::finishedTests()
{
    if ((mGenerateHtml || mSummaryOutputDir != "NONE") && !mReferenceSet)
    {
        const TestBatch* compareTo = 0;
        TestBatchSetPtr batches;
        TestBatchSet::iterator i;

        Ogre::ConfigFile info;
        bool foundReference = true;
        TestBatch* ref = 0;

        // look for a reference set first (either "Reference" or a user-specified image set)
        try
        {
            info.load(mReferenceSetPath + mCompareWith + "/info.cfg");
        }
        catch (Ogre::FileNotFoundException e)
        {
            // if no luck, just grab the most recent compatible set
            foundReference = false;
            batches = TestBatch::loadTestBatches(mOutputDir);
            
            for (i = batches->begin(); i != batches->end(); ++i)
            {
                if (mBatch->canCompareWith((*i)))
                {
                    compareTo = &(*i);
                    break;
                }
            }
        }

        if (foundReference)
        {
            ref = OGRE_NEW TestBatch(info, mReferenceSetPath + mCompareWith);
            if (mBatch->canCompareWith(*ref))
                compareTo = ref;
        }

        if (compareTo)
        {
            ComparisonResultVectorPtr results = mBatch->compare(*compareTo);

            if(mGenerateHtml)
            {
                HtmlWriter writer(*compareTo, *mBatch, results);

                // we save a generally named "out.html" that gets overwritten each run,
                // plus a uniquely named one for this run
                writer.writeToFile(mOutputDir + "out.html");
                writer.writeToFile(mOutputDir + "TestResults_" + mBatch->name + ".html");
            }

            // also save a summary file for CTest to parse, if required
            if(mSummaryOutputDir != "NONE")
            {
                Ogre::String rs;
                for(size_t j = 0; j < mRenderSystemName.size(); ++j)
                    if(mRenderSystemName[j]!=' ')
                        rs += mRenderSystemName[j];

                CppUnitResultWriter cppunitWriter(*compareTo, *mBatch, results);
                cppunitWriter.writeToFile(mSummaryOutputDir + "/TestResults_" + rs + ".xml");
            }
        }

        OGRE_DELETE ref;
    }

    // write this batch's config file
    mBatch->writeConfig();
}
//-----------------------------------------------------------------------

Ogre::Real TestContext::getTimestep()
{
    return mTimestep;
}
//-----------------------------------------------------------------------

void TestContext::setTimestep(Ogre::Real timestep)
{
    // ensure we're getting a positive value
    mTimestep = timestep >= 0.f ? timestep : mTimestep;
}
//-----------------------------------------------------------------------

void TestContext::createDummyScene()
{
    //Test multithreading which is prone to errors (regardless of number of cores the system has)
    const size_t numThreads = 3;
    Ogre::InstancingTheadedCullingMethod threadedCullingMethod = Ogre::INSTANCING_CULLING_THREADED;

    Ogre::SceneManager* sm = mRoot->createSceneManager(Ogre::ST_GENERIC, numThreads,
                                                        threadedCullingMethod, "DummyScene");
    sm->addRenderQueueListener(mOverlaySystem);
#ifdef INCLUDE_RTSHADER_SYSTEM
    // Initialize shader generator.
    // Must be before resource loading in order to allow parsing extended material attributes.
    bool success = initialiseRTShaderSystem(sm);
    if (!success)
    {
        OGRE_EXCEPT(Ogre::Exception::ERR_FILE_NOT_FOUND,
                    "Shader Generator Initialization failed - Core shader libs path not found",
                    "SampleBrowser::createDummyScene");
    }
    if(mRoot->getRenderSystem()->getCapabilities()->hasCapability(Ogre::RSC_FIXED_FUNCTION) == false)
    {
        //newViewport->setMaterialScheme(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);

        // creates shaders for base material BaseWhite using the RTSS
        Ogre::MaterialPtr baseWhite = Ogre::MaterialManager::getSingleton().getByName("BaseWhite", Ogre::ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME);
        baseWhite->setLightingEnabled(false);
        mShaderGenerator->createShaderBasedTechnique(
            "BaseWhite",
            Ogre::MaterialManager::DEFAULT_SCHEME_NAME,
            Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
        mShaderGenerator->validateMaterial(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME,
                                           "BaseWhite");
        if(baseWhite->getNumTechniques() > 1)
        {
            baseWhite->getTechnique(0)->getPass(0)->setVertexProgram(
                baseWhite->getTechnique(1)->getPass(0)->getVertexProgram()->getName());
            baseWhite->getTechnique(0)->getPass(0)->setFragmentProgram(
                baseWhite->getTechnique(1)->getPass(0)->getFragmentProgram()->getName());
        }

        // creates shaders for base material BaseWhiteNoLighting using the RTSS
        mShaderGenerator->createShaderBasedTechnique(
            "BaseWhiteNoLighting",
            Ogre::MaterialManager::DEFAULT_SCHEME_NAME,
            Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
        mShaderGenerator->validateMaterial(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME,
                                           "BaseWhiteNoLighting");
        Ogre::MaterialPtr baseWhiteNoLighting = Ogre::MaterialManager::getSingleton().getByName("BaseWhiteNoLighting", Ogre::ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME);
        if(baseWhite->getNumTechniques() > 1)
        {
            baseWhiteNoLighting->getTechnique(0)->getPass(0)->setVertexProgram(
                baseWhiteNoLighting->getTechnique(1)->getPass(0)->getVertexProgram()->getName());
            baseWhiteNoLighting->getTechnique(0)->getPass(0)->setFragmentProgram(
                baseWhiteNoLighting->getTechnique(1)->getPass(0)->getFragmentProgram()->getName());
        }
    }
#endif // INCLUDE_RTSHADER_SYSTEM
}

void TestContext::destroyDummyScene()
{
    Ogre::SceneManager*  dummyScene = mRoot->getSceneManager("DummyScene");
#ifdef INCLUDE_RTSHADER_SYSTEM
    mShaderGenerator->removeSceneManager(dummyScene);
#endif
    dummyScene->removeRenderQueueListener(mOverlaySystem);
    mWindow->removeAllViewports();
    mRoot->destroySceneManager(dummyScene);
}

#ifdef INCLUDE_RTSHADER_SYSTEM

/*-----------------------------------------------------------------------------
  | Initialize the RT Shader system.
  -----------------------------------------------------------------------------*/
bool TestContext::initialiseRTShaderSystem(Ogre::SceneManager* sceneMgr)
{
    if (Ogre::RTShader::ShaderGenerator::initialize())
    {
        mShaderGenerator = Ogre::RTShader::ShaderGenerator::getSingletonPtr();

        mShaderGenerator->addSceneManager(sceneMgr);

#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID && OGRE_PLATFORM != OGRE_PLATFORM_NACL && OGRE_PLATFORM != OGRE_PLATFORM_WINRT
        // Setup core libraries and shader cache path.
        Ogre::StringVector groupVector = Ogre::ResourceGroupManager::getSingleton().getResourceGroups();
        Ogre::StringVector::iterator itGroup = groupVector.begin();
        Ogre::StringVector::iterator itGroupEnd = groupVector.end();
        Ogre::String shaderCoreLibsPath;
        Ogre::String shaderCachePath;

        for (; itGroup != itGroupEnd; ++itGroup)
        {
            Ogre::ResourceGroupManager::LocationList resLocationsList = Ogre::ResourceGroupManager::getSingleton().getResourceLocationList(*itGroup);
            Ogre::ResourceGroupManager::LocationList::iterator it = resLocationsList.begin();
            Ogre::ResourceGroupManager::LocationList::iterator itEnd = resLocationsList.end();
            bool coreLibsFound = false;

            // Try to find the location of the core shader lib functions and use it
            // as shader cache path as well - this will reduce the number of generated files
            // when running from different directories.
            for (; it != itEnd; ++it)
            {
                if ((*it)->archive->getName().find("RTShaderLib") != Ogre::String::npos)
                {
                    shaderCoreLibsPath = (*it)->archive->getName() + "/cache/";
                    shaderCachePath = shaderCoreLibsPath;
                    coreLibsFound = true;
                    break;
                }
            }
            // Core libs path found in the current group.
            if (coreLibsFound)
                break;
        }

        // Core shader libs not found -> shader generating will fail.
        if (shaderCoreLibsPath.empty())
            return false;

#ifdef _RTSS_WRITE_SHADERS_TO_DISK
        // Set shader cache path.
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        shaderCachePath = Ogre::macCachePath();
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        shaderCachePath = Ogre::macCachePath() + "/org.ogre3d.RTShaderCache";
#endif
        mShaderGenerator->setShaderCachePath(shaderCachePath);
#endif
#endif
        // Create and register the material manager listener if it doesn't exist yet.
        if (mMaterialMgrListener == NULL) {
            mMaterialMgrListener = new ShaderGeneratorTechniqueResolverListener(mShaderGenerator);
            Ogre::MaterialManager::getSingleton().addListener(mMaterialMgrListener);
        }
    }

    return true;
}

/*-----------------------------------------------------------------------------
  | Destroy the RT Shader system.
  -----------------------------------------------------------------------------*/
void TestContext::finaliseRTShaderSystem()
{
    // Restore default scheme.
    Ogre::MaterialManager::getSingleton().setActiveScheme(Ogre::MaterialManager::DEFAULT_SCHEME_NAME);

    // Unregister the material manager listener.
    if (mMaterialMgrListener != NULL)
    {
        Ogre::MaterialManager::getSingleton().removeListener(mMaterialMgrListener);
        delete mMaterialMgrListener;
        mMaterialMgrListener = NULL;
    }

    // Destroy RTShader system.
    if (mShaderGenerator != NULL)
    {
        Ogre::RTShader::ShaderGenerator::destroy();
        mShaderGenerator = NULL;
    }
}
#endif // INCLUDE_RTSHADER_SYSTEM

// main, platform-specific stuff is copied from SampleBrowser and not guaranteed to work...

// since getting commandline args out of WinMain isn't pretty, just use plain main for now...
//#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
//INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, INT)
//#else
int main(int argc, char *argv[])
//#endif
{
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    int retVal = UIApplicationMain(argc, argv, @"UIApplication", @"AppDelegate");
    [pool release];
    return retVal;
#elif (OGRE_PLATFORM == OGRE_PLATFORM_APPLE) && __LP64__
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    mAppDelegate = [[AppDelegate alloc] init];
    [[NSApplication sharedApplication] setDelegate:mAppDelegate];
    int retVal = NSApplicationMain(argc, (const char **) argv);

    [pool release];

    return retVal;
#else

    try
    {
        TestContext tc = TestContext(argc, argv);
        tc.go();
    }
    catch (Ogre::Exception& e)
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        MessageBoxA(NULL, e.getFullDescription().c_str(), "An exception has occurred!", MB_ICONERROR | MB_TASKMODAL);
#else
        std::cerr << "An exception has occurred: " << e.getFullDescription().c_str() << std::endl;
#endif
    }

#endif
    return 0;
}
