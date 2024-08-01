
@echo off
SETLOCAL

set GENERATOR="Visual Studio 16 2019"
set PLATFORM=x64
set CONFIGURATION=Debug
set BUILD_FOLDER=%APPVEYOR_BUILD_FOLDER%

set CMAKE_BIN_x86="C:\Program Files (x86)\CMake\bin\cmake.exe"
set CMAKE_BIN_x64="C:\Program Files\CMake\bin\cmake.exe"
IF EXIST %CMAKE_BIN_x64% (
	echo CMake 64-bit detected
	set CMAKE_BIN=%CMAKE_BIN_x64%
) ELSE (
	IF EXIST %CMAKE_BIN_x86% (
		echo CMake 32-bit detected
		set CMAKE_BIN=%CMAKE_BIN_x86%
	) ELSE (
		echo Cannot detect either %CMAKE_BIN_x86% or
		echo %CMAKE_BIN_x64% make sure CMake is installed
		EXIT /B 1
	)
)
echo Using CMake at %CMAKE_BIN%

%CMAKE_BIN% --version
echo -- PATH --
echo %PATH%
echo -- PYTHONPATH --
echo %PYTHONPATH%

IF NOT EXIST %BUILD_FOLDER%\..\ogre-next-deps (
	mkdir %BUILD_FOLDER%\..\ogre-next-deps
	echo --- Cloning ogre-next-deps ---
	git clone --recurse-submodules --shallow-submodules https://github.com/OGRECave/ogre-next-deps %BUILD_FOLDER%\..\ogre-next-deps

	mkdir %BUILD_FOLDER%\..\ogre-next-deps\build
	cd %BUILD_FOLDER%\..\ogre-next-deps\build
	echo --- Building ogre-next-deps ---
	%CMAKE_BIN% -G %GENERATOR% -A %PLATFORM% %BUILD_FOLDER%\..\ogre-next-deps
	%CMAKE_BIN% --build . --config %CONFIGURATION%
	%CMAKE_BIN% --build . --target install --config %CONFIGURATION%
) ELSE (
	echo --- ogre-next-deps repo detected. Cloning skipped ---
	echo --- ogre-next-deps repo detected. Building skipped ---
)

cd %BUILD_FOLDER%
mkdir %BUILD_FOLDER%\build
cd %BUILD_FOLDER%\build
echo --- Running CMake configure ---
%CMAKE_BIN% -D OGRE_UNITY_BUILD=1 -D OGRE_USE_BOOST=0 -D OGRE_CONFIG_THREAD_PROVIDER=0 -D OGRE_CONFIG_THREADS=0 -D OGRE_BUILD_COMPONENT_SCENE_FORMAT=1 -D OGRE_BUILD_SAMPLES2=1 -D OGRE_BUILD_TESTS=1 -D OGRE_DEPENDENCIES_DIR=%BUILD_FOLDER%\..\ogre-next-deps\build\ogredeps -G %GENERATOR% -A %PLATFORM% %BUILD_FOLDER%
echo --- Building Ogre ---
%CMAKE_BIN% --build . --config %CONFIGURATION%
%CMAKE_BIN% --build . --target install --config %CONFIGURATION%

echo Done!

ENDLOCAL
