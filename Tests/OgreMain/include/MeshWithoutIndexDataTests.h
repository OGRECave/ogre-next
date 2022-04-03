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

#ifndef __MeshWithoutIndexDataTests_H__
#define __MeshWithoutIndexDataTests_H__

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace Ogre;

class MeshWithoutIndexDataTests : public CppUnit::TestFixture
{
    // CppUnit macros for setting up the test suite
    CPPUNIT_TEST_SUITE(MeshWithoutIndexDataTests);
    CPPUNIT_TEST(testCreateSimpleLine);
    CPPUNIT_TEST(testCreateLineList);
    CPPUNIT_TEST(testCreateLineStrip);
    CPPUNIT_TEST(testCreatePointList);
    CPPUNIT_TEST(testCreateLineWithMaterial);
    CPPUNIT_TEST(testCreateMesh);
    CPPUNIT_TEST(testCloneMesh);
    CPPUNIT_TEST(testEdgeList);
    CPPUNIT_TEST(testGenerateExtremes);
    CPPUNIT_TEST(testBuildTangentVectors);
    CPPUNIT_TEST(testGenerateLodLevels);
    CPPUNIT_TEST_SUITE_END();

protected:
    HardwareBufferManager* mBufMgr;
    MeshManager* mMeshMgr;
    ArchiveManager* mArchiveMgr;

public:
    void setUp();
    void tearDown();

    void testCreateSimpleLine();
    void testCreateLineList();
    void testCreateLineStrip();
    void testCreatePointList();
    void testCreateLineWithMaterial();
    void testCreateMesh();
    void testCloneMesh();
    void testEdgeList();
    void testGenerateExtremes();
    void testBuildTangentVectors();
    void testGenerateLodLevels();
};
#endif