#!/usr/bin/env python3

# Script that recursively runs clang-format on all files under g_folders
# then checkouts a different git commits; does the same; then compares
# both runs.
#
# The idea is that the new commit does not introduce changes that
# haven't been formatted by clang format.

import os
import subprocess
import sys
import threading

print("Arguments:\n\t{0}\n".format(sys.argv))

bCompareWithBase = False
bFixBrokenFiles = False

if len(sys.argv) == 2:
    print("Usage:\n\tpython3 run_clang_format.py base_commit_hash")
    print("Usage:\n\tpython3 run_clang_format.py --fix_broken_files")
    if sys.argv[1] == '--fix_broken_files':
        print("We will fix broken files")
        bFixBrokenFiles = True
    else:
        print("Comparing between two commits mode")
        bCompareWithBase = True

g_folders = [
    'Components/Atmosphere',
    'Components/Hlms',
    'Components/MeshLodGenerator',
    'Components/Overlay',
    'Components/PlanarReflections',
    'Components/SceneFormat',
    'OgreMain',
    'RenderSystems/Direct3D11',
    'RenderSystems/GL3Plus',
    'RenderSystems/Metal',
    'RenderSystems/NULL',
    'RenderSystems/Vulkan',
    'PlugIns/ParticleFX',
    'PlugIns/ParticleFX2',
    'Samples/2.0',
    'Tools/MaterialEditor',
]

g_exceptions = {'stb_image_write.h', 'stb_image.h',
                'wgl.h', 'khrplatform.h',
                'MurmurHash3.cpp', 'GLX_backdrop.h',
                'glcorearb.h', 'glext.h', 'wglext.h',
                'gl3w.h', 'glxtokens-backup.h', 'glxext.h',
                'gl3w.cpp',
                'OgreMetalPixelFormatToShaderType.inl',
                'OgreD3D11PixelFormatToShaderType.cpp',
                'OgreD3D11PixelFormatToShaderType.inl',
                'spirv.h', 'spirv_reflect.h',
                'LocalCubemapScene.h', 'LocalCubemapsManualProbesScene.h',
                'OgreScriptTranslator.cpp'}

g_thresholds = {
    '../../OgreMain/include/OgreTextureUnitState.h': 12,
    '../../OgreMain/include/OgreWorkarounds.h': 3,
    '../../OgreMain/include/OgreBitset.inl': 747,
    '../../OgreMain/include/OgreMovableObject.inl': 174,
    '../../OgreMain/include/OgreLight.inl': 42,
    '../../OgreMain/include/OgreLight.h': 3,
    '../../OgreMain/include/OgreHlmsDatablock.h': 3,
    '../../OgreMain/include/Animation/OgreBone.inl': 63,
    '../../OgreMain/include/Math/Array/OgreNodeMemoryManager.h': 3,
    '../../OgreMain/include/Math/Array/C/OgreArrayVector3C.inl': 855,
    '../../OgreMain/include/Math/Array/C/OgreArraySphereC.inl': 24,
    '../../OgreMain/include/Math/Array/C/OgreArrayMatrix4C.inl': 528,
    '../../OgreMain/include/Math/Array/C/OgreMathlibC.inl': 33,
    '../../OgreMain/include/Math/Array/C/OgreArrayAabbC.inl': 114,
    '../../OgreMain/include/Math/Array/C/OgreBooleanMaskC.inl': 54,
    '../../OgreMain/include/Math/Array/C/OgreArrayQuaternionC.inl': 702,
    '../../OgreMain/include/Math/Array/C/OgreArrayMatrixAf4x3C.inl': 1053,
    '../../OgreMain/include/Math/Array/NEON/Single/OgreArrayVector3NEON.inl': 1386,
    '../../OgreMain/include/Math/Array/NEON/Single/OgreArraySphereNEON.inl': 30,
    '../../OgreMain/include/Math/Array/NEON/Single/OgreArrayMatrix4NEON.inl': 1419,
    '../../OgreMain/include/Math/Array/NEON/Single/OgreMathlibNEON.inl': 408,
    '../../OgreMain/include/Math/Array/NEON/Single/OgreArrayAabbNEON.inl': 216,
    '../../OgreMain/include/Math/Array/NEON/Single/OgreBooleanMaskNEON.inl': 141,
    '../../OgreMain/include/Math/Array/NEON/Single/OgreArrayQuaternionNEON.inl': 1170,
    '../../OgreMain/include/Math/Array/NEON/Single/OgreArrayMatrixAf4x3NEON.inl': 1791,
    '../../OgreMain/include/Math/Array/SSE2/Single/OgreArrayVector3SSE2.inl': 1245,
    '../../OgreMain/include/Math/Array/SSE2/Single/OgreArraySphereSSE2.inl': 30,
    '../../OgreMain/include/Math/Array/SSE2/Single/OgreArrayMatrix4SSE2.inl': 1530,
    '../../OgreMain/include/Math/Array/SSE2/Single/OgreArrayAabbSSE2.inl': 219,
    '../../OgreMain/include/Math/Array/SSE2/Single/OgreBooleanMaskSSE2.inl': 126,
    '../../OgreMain/include/Math/Array/SSE2/Single/OgreArrayQuaternionSSE2.inl': 1074,
    '../../OgreMain/include/Math/Array/SSE2/Single/OgreArrayMatrixAf4x3SSE2.inl': 1929,
    '../../OgreMain/include/Compositor/OgreCompositorManager2.h': 3,
    '../../OgreMain/include/Compositor/Pass/OgreCompositorPassDef.h': 3,
    '../../OgreMain/src/OgrePlatformInformation.cpp': 2274,
    '../../OgreMain/src/OgreImageResampler.h': 3792,
    '../../OgreMain/src/OgreRenderSystemCapabilitiesSerializer.cpp': 3123,
    '../../OgreMain/src/OgreUnifiedHighLevelGpuProgram.cpp': 162,
    '../../OgreMain/src/OgreOptimisedUtilSSE.cpp': 7899,
    '../../OgreMain/src/OgreImageDownsampler.cpp': 408,
    '../../OgreMain/src/OgreFrustum.cpp': 18,
    '../../OgreMain/src/OgreGpuProgramParams.cpp': 9,
    '../../OgreMain/src/OgreOptimisedUtilGeneral.cpp': 990,
    '../../OgreMain/src/OgreHlmsDiskCache.cpp': 6,
    '../../OgreMain/src/OgreControllerManager.cpp': 6,
    '../../OgreMain/src/OgreRenderSystemCapabilitiesManager.cpp': 189,
    '../../OgreMain/src/OgreOptimisedUtil.cpp': 600,
    '../../OgreMain/src/OgreSmallVector.cpp': 135,
    '../../OgreMain/src/OgreRenderSystemCapabilities.cpp': 2133,
    '../../OgreMain/src/OgreImage2.cpp': 3,
    '../../OgreMain/src/OgreResourceGroupManager.cpp': 12,
    '../../OgreMain/src/OgrePrefabFactory.cpp': 1869,
    '../../OgreMain/src/WIN32/resource.h': 96,
    '../../RenderSystems/Metal/src/OgreMetalRenderSystem.mm': 42,
    '../../RenderSystems/Metal/src/Vao/OgreMetalVaoManager.mm': 21,
    '../../Samples/2.0/ApiUsage/DynamicGeometry/DynamicGeometryGameState.cpp': 63,
    '../../Samples/2.0/Tutorials/Tutorial03_DeterministicLoop/iOS/TutorialViewController.mm': 18,
    '../../Samples/2.0/Tutorials/Tutorial02_VariableFramerate/iOS/TutorialViewController.mm': 18,
    '../../Samples/2.0/Tutorials/Tutorial_Terrain/include/Terra/Terra.h': 18,
    '../../Samples/2.0/Tutorials/Tutorial_Terrain/include/Terra/TerraShadowMapper.h': 36,
    '../../Samples/2.0/Tutorials/Tutorial_Terrain/include/Terra/Hlms/OgreHlmsTerra.h': 66,
    '../../Samples/2.0/Tutorials/Tutorial_Terrain/include/Terra/Hlms/PbsListener/OgreHlmsPbsTerraShadows.h': 12,
    '../../Samples/2.0/Tutorials/Tutorial01_Initialization/iOS/TutorialViewController.mm': 18,
    '../../Samples/2.0/Tests/Restart/iOS/RestartViewController.mm': 18,
    '../../Samples/2.0/Common/include/GameEntity.h': 12,
    '../../Samples/2.0/Common/include/SdlInputHandler.h': 24,
    '../../Samples/2.0/Common/include/GraphicsSystem.h': 66,
    '../../Samples/2.0/Common/include/GameEntityManager.h': 12,
    '../../Samples/2.0/Common/include/SdlEmulationLayer.h': 3273,
    '../../Samples/2.0/Common/include/LogicSystem.h': 6,
    '../../Samples/2.0/Common/include/System/Desktop/UnitTesting.h': 6,
    '../../Samples/2.0/Common/include/System/Android/AndroidSystems.h': 6,
    '../../Samples/2.0/Common/src/System/iOS/GameViewController.mm': 18,
    '../../Samples/2.0/Common/src/System/iOS/AppDelegate.mm': 18
}

g_threadCount = os.cpu_count()
print("System has " + str(g_threadCount) + " CPU threads")
# g_threadCount *= 2


def split_list(alist, wanted_parts=1):
    """
    Utility function to split an array into multiple arrays
    """
    length = len(alist)
    return [alist[i*length // wanted_parts: (i+1)*length // wanted_parts]
            for i in range(wanted_parts)]


def collectCppFilesForFormatting():
    """
    Collects & returns a list of all files we need to analyze
    """
    global g_folders
    global g_exceptions
    pathsToParse = []
    for folder in g_folders:
        for root, dirs, filenames in os.walk(os.path.join('../../', folder)):
            for fileName in filenames:
                fullpath = os.path.join(root, fileName)
                ext = os.path.splitext(fileName)[-1].lower()
                if (ext == '.cpp' or ext == '.h' or ext == '.inl' or ext == '.mm'):
                    if (fileName not in g_exceptions):
                        pathsToParse.append(fullpath)
    return pathsToParse


def runClangThread(pathsToParseByThisThread, outChangelist, bFixProblems):
    """
    Thread that runs clang format on the list of files given to it
    """
    for fullpath in pathsToParseByThisThread:
        command = ['clang-format-18', '--dry-run', fullpath]
        if bFixProblems:
            command = ['clang-format-18', '-i', fullpath]
            print("[CLANG FORMAT INFO]: Fixing: {0}".format(fullpath))
        process = subprocess.Popen(
            command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        (output, err) = process.communicate()
        numLines = err.count("\n") + 0
        # print(fullpath + ' line changes: ' + str(numLines))
        outChangelist.append((fullpath, numLines))


def runClangMultithreaded(pathsToParse, bFixProblems):
    """
    Dispatches one thread for each block files
    """
    global g_threadCount
    changeList = [None] * g_threadCount
    for i in range(g_threadCount):
        changeList[i] = []
    pathsToParseByEachThread = split_list(pathsToParse, g_threadCount)
    threads = []
    for i, pathsToParseByThread in enumerate(pathsToParseByEachThread):
        newThread = threading.Thread(
            target=runClangThread, args=(pathsToParseByThread, changeList[i], bFixProblems))
        newThread.start()
        threads.append(newThread)

    # Wait for threads to finish
    for thread in threads:
        thread.join()

    fullChangeList = {}
    for i in range(g_threadCount):
        for change in changeList[i]:
            fullChangeList[change[0]] = change[1]
    return fullChangeList


pathsToParse = collectCppFilesForFormatting()
prChangeList = runClangMultithreaded(pathsToParse, False)

bHasErrors = False
if bCompareWithBase:
    # Change to base
    process = subprocess.Popen(['git', 'checkout', sys.argv[1]])

    pathsToParse = collectCppFilesForFormatting()
    baseChangeList = runClangMultithreaded(pathsToParse)

    for fullpath, prNumLines in prChangeList.items():
        try:
            baseNumLines = baseChangeList[fullpath]
            if prNumLines > baseNumLines:
                print("[CLANG FORMAT]: {0} has more lines to format PR: {1} vs BASE: {2}".format(
                    fullpath, prNumLines, baseNumLines))
                bHasErrors = True
        except IndexError:
            if prNumLines != 0:
                print("[CLANG FORMAT]: {0} is a new file with lines to format PR: {1}".format(
                    fullpath, prNumLines))
                bHasErrors = True
else:
    pathsToParse = []
    for fullpath, prNumLines in prChangeList.items():
        if prNumLines > 0:
            if fullpath in g_thresholds:
                thresh = g_thresholds[fullpath]
                if prNumLines > thresh:
                    print("[CLANG FORMAT ERROR]: {0} has lines: {1} > threshold: {2}".format(
                        fullpath, prNumLines, thresh))
                    pathsToParse.append(fullpath)
                    bHasErrors = True
                else:
                    print("[CLANG FORMAT INFO]: {0} has lines: {1} <= threshold: {2}".format(
                        fullpath, prNumLines, thresh))
            else:
                print("[CLANG FORMAT ERROR]: NOT in threshold {0} has lines: {1}".format(
                    fullpath, prNumLines))
                pathsToParse.append(fullpath)
                bHasErrors = True

    if bFixBrokenFiles:
        runClangMultithreaded(pathsToParse, True)

if bHasErrors:
    if bFixBrokenFiles:
        print("Clang Format Script had Failures and tried to fix them")
    else:
        print("Clang Format Script Failure")
    exit(1)
else:
    print("Clang Format Script Success!")
