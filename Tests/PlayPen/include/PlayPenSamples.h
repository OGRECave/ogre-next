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
#ifndef __PlayPenSamples_H__
#define __PlayPenSamples_H__

#include "PlayPen.h"

//---------------------------------------------------------------------
class _OgreSampleClassExport PlayPen_testManualBlend : public PlayPenBase
{
public:
    PlayPen_testManualBlend();
protected:
    void setupContent();
};
//---------------------------------------------------------------------
class _OgreSampleClassExport PlayPen_testProjectSphere : public PlayPenBase
{
public:
    PlayPen_testProjectSphere();
    bool frameStarted(const Ogre::FrameEvent& evt);
protected:
    Sphere* mProjectionSphere;
    ManualObject* mScissorRect;
    void setupContent();
};
//---------------------------------------------------------------------
class _OgreSampleClassExport PlayPen_testCameraSetDirection : public PlayPenBase
{
public:
    PlayPen_testCameraSetDirection();

    void buttonHit(OgreBites::Button* button);
    void checkBoxToggled(OgreBites::CheckBox* box);
protected:
    bool mUseParentNode;
    bool mUseFixedYaw;
    SceneNode* mParentNode;
    Vector3 mFocus;
    void setupContent();
    void toggleParentNode();
    void toggleFixedYaw();
    void track();

};
#ifdef OGRE_BUILD_COMPONENT_MESHLODGENERATOR
//---------------------------------------------------------------------
class _OgreSampleClassExport PlayPen_testManualLOD : public PlayPenBase
{
public:
    PlayPen_testManualLOD();
protected:
    void setupContent();
    String getLODMesh();
};
//---------------------------------------------------------------------
class _OgreSampleClassExport PlayPen_testManualLODFromFile : public PlayPen_testManualLOD
{
public:
    PlayPen_testManualLODFromFile();
protected:
    String getLODMesh();
};
#endif
//---------------------------------------------------------------------
class _OgreSampleClassExport PlayPen_testFullScreenSwitch : public PlayPenBase
{
public:
    PlayPen_testFullScreenSwitch();

    void buttonHit(OgreBites::Button* button);
protected:
    void setupContent();

    OgreBites::Button* m640x480w;
    OgreBites::Button* m800x600w;
    OgreBites::Button* m1024x768w;
    OgreBites::Button* m640x480fs;
    OgreBites::Button* m800x600fs;
    OgreBites::Button* m1024x768fs;

};

//---------------------------------------------------------------------
class _OgreSampleClassExport PlayPen_testMorphAnimationWithNormals : public PlayPenBase
{
public:
    PlayPen_testMorphAnimationWithNormals();
protected:
    void setupContent();
};
//---------------------------------------------------------------------
class _OgreSampleClassExport PlayPen_testMorphAnimationWithoutNormals : public PlayPenBase
{
public:
    PlayPen_testMorphAnimationWithoutNormals();
protected:
    void setupContent();
};
//---------------------------------------------------------------------
class _OgreSampleClassExport PlayPen_testPoseAnimationWithNormals : public PlayPenBase
{
public:
    PlayPen_testPoseAnimationWithNormals();
protected:
    void setupContent();
};
//---------------------------------------------------------------------
class _OgreSampleClassExport PlayPen_testPoseAnimationWithoutNormals : public PlayPenBase
{
public:
    PlayPen_testPoseAnimationWithoutNormals();
protected:
    void setupContent();
};



#endif
