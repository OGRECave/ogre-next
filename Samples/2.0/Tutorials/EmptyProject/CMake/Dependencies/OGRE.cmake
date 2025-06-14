#-------------------------------------------------------------------
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Finds the string "#define RESULT_NAME" in OgreBuildSettings.h
# (provided as a string in an argument)
function( findOgreBuildSetting OGRE_BUILD_SETTINGS_STR RESULT_NAME )
	string( FIND "${OGRE_BUILD_SETTINGS_STR}" "#define ${RESULT_NAME}" TMP_RESULT )
	if( NOT TMP_RESULT EQUAL -1 )
		set( ${RESULT_NAME} 1 PARENT_SCOPE )
	else()
		unset( ${RESULT_NAME} PARENT_SCOPE )
	endif()
endfunction()

#----------------------------------------------------------------------------------------

# On Windows it's a regular copy.
# On Apple it does nothing
# On Linux, it copies both libOgreMain.so.2.1 and its symbolic link libOgreMain.so
function( copyWithSymLink SRC DST )
	if( NOT APPLE )
		if( UNIX )
			get_filename_component( RESOLVED_LIB_PATH ${SRC} REALPATH )
			file( COPY ${RESOLVED_LIB_PATH} DESTINATION ${DST} )
		endif()
		file( COPY ${SRC} DESTINATION ${DST} )
	endif()
endfunction()

#----------------------------------------------------------------------------------------

# Finds if Ogre has been built a library with LIBRARY_NAME.dll,
# and if so sets the string for Plugins.cfg in CFG_VARIABLE.
macro( findPluginAndSetPath BUILD_TYPE CFG_VARIABLE LIBRARY_NAME )
	set( REAL_LIB_PATH ${LIBRARY_NAME} )
	if( ${BUILD_TYPE} STREQUAL "Debug" )
		set( REAL_LIB_PATH ${REAL_LIB_PATH}_d )
	endif()

	if( WIN32 )
		set( REAL_LIB_PATH "${OGRE_BINARIES}/bin/${BUILD_TYPE}/${REAL_LIB_PATH}.dll" )
	else()
		set( REAL_LIB_PATH "${OGRE_BINARIES}/lib/${REAL_LIB_PATH}.so" )
	endif()

	if( EXISTS ${REAL_LIB_PATH} )
		# DLL Exists, set the variable for Plugins.cfg
		if( ${BUILD_TYPE} STREQUAL "Debug" )
			set( ${CFG_VARIABLE} "PluginOptional=${LIBRARY_NAME}_d" )
		else()
			set( ${CFG_VARIABLE} "PluginOptional=${LIBRARY_NAME}" )
		endif()

		# Copy the DLLs to the folders.
		copyWithSymLink( ${REAL_LIB_PATH} "${CMAKE_SOURCE_DIR}/bin/${BUILD_TYPE}/Plugins" )
	endif()
endmacro()

#----------------------------------------------------------------------------------------

# Outputs TRUE into RESULT_VARIABLE if Ogre was build as OgreNextMain.dll instead of OgreMain.dll
macro( isOgreNext RESULT_VARIABLE )
	if( WIN32 )
		if( EXISTS "${OGRE_BINARIES}/bin/Debug/OgreNextMain_d.dll" OR
			EXISTS "${OGRE_BINARIES}/bin/Release/OgreNextMain.dll" OR
			EXISTS "${OGRE_BINARIES}/bin/RelWithDebInfo/OgreNextMain.dll" OR
			EXISTS "${OGRE_BINARIES}/bin/MinSizeRel/OgreNextMain.dll" OR
			EXISTS "${OGRE_BINARIES}/lib/Debug/OgreNextMainStatic_d.lib" OR
			EXISTS "${OGRE_BINARIES}/lib/Release/OgreNextMainStatic.lib" OR
			EXISTS "${OGRE_BINARIES}/lib/RelWithDebInfo/OgreNextMainStatic.lib" OR
			EXISTS "${OGRE_BINARIES}/lib/MinSizeRel/OgreNextMainStatic.lib")
			set( ${RESULT_VARIABLE} TRUE )
		else()
			set( ${RESULT_VARIABLE} FALSE )
		endif()
	elseif( APPLE )
		if( APPLE_IOS )
			set( APPLE_FOLDER "iphoneos" )
		else()
			set( APPLE_FOLDER "macosx" )
		endif()
		if( EXISTS "${OGRE_BINARIES}/lib/${APPLE_FOLDER}/Debug/libOgreNextMain.so" OR
			EXISTS "${OGRE_BINARIES}/lib/${APPLE_FOLDER}/Release/libOgreNextMain.so" OR
			EXISTS "${OGRE_BINARIES}/lib/${APPLE_FOLDER}/RelWithDebInfo/libOgreNextMain.so" OR
			EXISTS "${OGRE_BINARIES}/lib/${APPLE_FOLDER}/MinSizeRel/libOgreNextMain.so" OR
			EXISTS "${OGRE_BINARIES}/lib/${APPLE_FOLDER}/Debug/libOgreNextMainStatic.a" OR
			EXISTS "${OGRE_BINARIES}/lib/${APPLE_FOLDER}/Release/libOgreNextMainStatic.a" OR
			EXISTS "${OGRE_BINARIES}/lib/${APPLE_FOLDER}/RelWithDebInfo/libOgreNextMainStatic.a" OR
			EXISTS "${OGRE_BINARIES}/lib/${APPLE_FOLDER}/MinSizeRel/libOgreNextMainStatic.a")
			set( ${RESULT_VARIABLE} TRUE )
		else()
			set( ${RESULT_VARIABLE} FALSE )
		endif()
	else()
		set( DEBUG_SUFFIX "" )
		if( CMAKE_BUILD_TYPE )
			if( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
				set( DEBUG_SUFFIX "_d" )
			endif()
		endif()
		if( EXISTS "${OGRE_BINARIES}/lib/libOgreNextMain${DEBUG_SUFFIX}.so" OR
			EXISTS "${OGRE_BINARIES}/lib/libOgreNextMainStatic${DEBUG_SUFFIX}.a")
			set( ${RESULT_VARIABLE} TRUE )
		else()
			set( ${RESULT_VARIABLE} FALSE )
		endif()
	endif()
endmacro()

#----------------------------------------------------------------------------------------

# Generates Plugins.cfg file out of user-editable Plugins.cfg.in file. Will automatically disable those plugins
# that were not built
# Copies all relevant DLLs: RenderSystem files, OgreOverlay, Hlms PBS & Unlit.
macro( setupPluginFileFromTemplate BUILD_TYPE OGRE_USE_SCENE_FORMAT OGRE_USE_PLANAR_REFLECTIONS )
	if( CMAKE_BUILD_TYPE )
		if( ${CMAKE_BUILD_TYPE} STREQUAL ${BUILD_TYPE} )
			set( OGRE_BUILD_TYPE_MATCHES 1 )
		endif()
	endif()

	# On non-Windows machines, we can only do Plugins for the current build.
	if( WIN32 OR OGRE_BUILD_TYPE_MATCHES )
		if( NOT APPLE )
			file( MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${BUILD_TYPE}/Plugins" )
		endif()

		findPluginAndSetPath( ${BUILD_TYPE} OGRE_PLUGIN_RS_D3D11	RenderSystem_Direct3D11 )
		findPluginAndSetPath( ${BUILD_TYPE} OGRE_PLUGIN_RS_GL3PLUS	RenderSystem_GL3Plus )
		findPluginAndSetPath( ${BUILD_TYPE} OGRE_PLUGIN_RS_VULKAN	RenderSystem_Vulkan )

		if( ${BUILD_TYPE} STREQUAL "Debug" )
			configure_file( ${CMAKE_SOURCE_DIR}/CMake/Templates/Plugins.cfg.in
							${CMAKE_SOURCE_DIR}/bin/${BUILD_TYPE}/plugins_d.cfg )
		else()
			configure_file( ${CMAKE_SOURCE_DIR}/CMake/Templates/Plugins.cfg.in
							${CMAKE_SOURCE_DIR}/bin/${BUILD_TYPE}/plugins.cfg )
		endif()

		# Copy
		# "${OGRE_BINARIES}/bin/${BUILD_TYPE}/OgreMain.dll" to "${CMAKE_SOURCE_DIR}/bin/${BUILD_TYPE}
		# and the other DLLs as well.

		# Lists of DLLs to copy
		set( OGRE_DLLS
				${OGRE_NEXT}Main
				${OGRE_NEXT}Overlay
				${OGRE_NEXT}HlmsPbs
				${OGRE_NEXT}HlmsUnlit
			)

		if( ${OGRE_USE_SCENE_FORMAT} )
			set( OGRE_DLLS ${OGRE_DLLS} ${OGRE_NEXT}SceneFormat )
		endif()

		if( NOT OGRE_BUILD_COMPONENT_ATMOSPHERE EQUAL -1 )
			set( OGRE_DLLS ${OGRE_DLLS} ${OGRE_NEXT}Atmosphere )
		endif()

		# Deal with OS and Ogre naming shenanigans:
		#	* OgreMain.dll vs libOgreMain.so
		#	* OgreMain_d.dll vs libOgreMain_d.so in Debug mode.
		if( WIN32 )
			set( DLL_OS_PREFIX "" )
			if( ${BUILD_TYPE} STREQUAL "Debug" )
				set( DLL_OS_SUFFIX "_d.dll" )
			else()
				set( DLL_OS_SUFFIX ".dll" )
			endif()
		else()
			set( DLL_OS_PREFIX "lib" )
			if( ${BUILD_TYPE} STREQUAL "Debug" )
				set( DLL_OS_SUFFIX "_d.so" )
			else()
				set( DLL_OS_SUFFIX ".so" )
			endif()
		endif()

		# On Windows DLLs are in build/bin/Debug & build/bin/Release;
		# On Linux DLLs are in build/Debug/lib.
		if( WIN32 )
			set( OGRE_DLL_PATH "${OGRE_BINARIES}/bin/${BUILD_TYPE}" )
		else()
			set( OGRE_DLL_PATH "${OGRE_BINARIES}/lib/" )
		endif()

		# Do not copy anything if we don't find OgreMain.dll (likely Ogre was not build)
		list( GET OGRE_DLLS 0 DLL_NAME )
		if( EXISTS "${OGRE_DLL_PATH}/${DLL_OS_PREFIX}${DLL_NAME}${DLL_OS_SUFFIX}" )
			foreach( DLL_NAME ${OGRE_DLLS} )
				copyWithSymLink( "${OGRE_DLL_PATH}/${DLL_OS_PREFIX}${DLL_NAME}${DLL_OS_SUFFIX}"
								 "${CMAKE_SOURCE_DIR}/bin/${BUILD_TYPE}" )
			endforeach()
		endif()

		unset( OGRE_PLUGIN_RS_D3D11 )
		unset( OGRE_PLUGIN_RS_GL3PLUS )
		unset( OGRE_PLUGIN_RS_VULKAN )
	endif()

	unset( OGRE_BUILD_TYPE_MATCHES )
endmacro()

#----------------------------------------------------------------------------------------

# Creates Resources.cfg out of user-editable CMake/Templates/Resources.cfg.in
function( setupResourceFileFromTemplate )
	message( STATUS "Generating ${CMAKE_SOURCE_DIR}/bin/Data/resources2.cfg from template
		${CMAKE_SOURCE_DIR}/CMake/Templates/Resources.cfg.in" )
	if( APPLE )
		set( OGRE_MEDIA_DIR "Contents/Resources/" )
	elseif( ANDROID )
		set( OGRE_MEDIA_DIR "" )
	else()
		set( OGRE_MEDIA_DIR "../" )
	endif()
	configure_file( ${CMAKE_SOURCE_DIR}/CMake/Templates/Resources.cfg.in ${CMAKE_SOURCE_DIR}/bin/Data/resources2.cfg )
endfunction()

#----------------------------------------------------------------------------------------

function( setupOgreSamplesCommon )
	message( STATUS "Copying OgreSamplesCommon cpp and header files to
		${CMAKE_SOURCE_DIR}/include/OgreCommon
		${CMAKE_SOURCE_DIR}/src/OgreCommon/" )
	include_directories( "${CMAKE_SOURCE_DIR}/include/OgreCommon/" )
	file( COPY "${OGRE_SOURCE}/Samples/2.0/Common/include/"	DESTINATION "${CMAKE_SOURCE_DIR}/include/OgreCommon/" )
	file( COPY "${OGRE_SOURCE}/Samples/2.0/Common/src/"		DESTINATION "${CMAKE_SOURCE_DIR}/src/OgreCommon/" )
endfunction()

#----------------------------------------------------------------------------------------

# Main call to setup Ogre.
macro( setupOgre OGRE_SOURCE, OGRE_BINARIES, OGRE_LIBRARIES_OUT,
		OGRE_USE_SCENE_FORMAT OGRE_USE_PLANAR_REFLECTIONS )

set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/Ogre/CMake/Packages" )

# Guess the paths.
set( OGRE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/Ogre" CACHE STRING "Path to OGRE-Next source code (see http://www.ogre3d.org/tikiwiki/tiki-index.php?page=CMake+Quick+Start+Guide)" )
if( WIN32 )
	set( OGRE_BINARIES "${OGRE_SOURCE}/build" CACHE STRING "Path to OGRE-Next's build folder generated by CMake" )
	link_directories( "${OGRE_BINARIES}/lib/$(ConfigurationName)" )
elseif( APPLE )
	set( OGRE_BINARIES "${OGRE_SOURCE}/build" CACHE STRING "Path to OGRE-Next's build folder generated by CMake" )
	link_directories( "${OGRE_BINARIES}/lib/$(PLATFORM_NAME)/$(CONFIGURATION)" )
elseif( ANDROID )
	set( OGRE_BINARIES "${OGRE_SOURCE}/build/Android/${CMAKE_BUILD_TYPE}" CACHE STRING "Path to OGRE-Next's build folder generated by CMake" )
	link_directories( "${OGRE_BINARIES}/lib" )
else()
	set( OGRE_BINARIES "${OGRE_SOURCE}/build/${CMAKE_BUILD_TYPE}" CACHE STRING "Path to OGRE-Next's build folder generated by CMake" )
	link_directories( "${OGRE_BINARIES}/lib" )
endif()

isOgreNext( OGRE_USE_NEW_NAME )
if( ${OGRE_USE_NEW_NAME} )
	set( OGRE_NEXT "OgreNext" )
else()
	set( OGRE_NEXT "Ogre" )
endif()
message( STATUS "OgreNext lib name prefix is ${OGRE_NEXT}" )

# Ogre config
include_directories( "${OGRE_SOURCE}/OgreMain/include" )

# Ogre includes
include_directories( "${OGRE_BINARIES}/include" )
include_directories( "${OGRE_SOURCE}/Components/Hlms/Common/include" )
include_directories( "${OGRE_SOURCE}/Components/Hlms/Unlit/include" )
include_directories( "${OGRE_SOURCE}/Components/Hlms/Pbs/include" )
include_directories( "${OGRE_SOURCE}/Components/Overlay/include" )
if( ${OGRE_USE_SCENE_FORMAT} )
	include_directories( "${OGRE_SOURCE}/Components/SceneFormat/include" )
endif()
if( ${OGRE_USE_PLANAR_REFLECTIONS} )
	include_directories( "${OGRE_SOURCE}/Components/PlanarReflections/include" )
endif()

# Parse OgreBuildSettings.h to see if it's a static build
set( OGRE_DEPENDENCY_LIBS "" )
file( READ "${OGRE_BINARIES}/include/OgreBuildSettings.h" OGRE_BUILD_SETTINGS_STR )
string( FIND "${OGRE_BUILD_SETTINGS_STR}" "#define OGRE_BUILD_COMPONENT_ATMOSPHERE" OGRE_BUILD_COMPONENT_ATMOSPHERE )
string( FIND "${OGRE_BUILD_SETTINGS_STR}" "#define OGRE_STATIC_LIB" OGRE_STATIC )
if( NOT OGRE_STATIC EQUAL -1 )
	message( STATUS "Detected static build of OgreNext" )
	set( OGRE_STATIC "Static" )

	# Static builds must link against its dependencies
	addStaticDependencies( OGRE_SOURCE, OGRE_BINARIES, OGRE_BUILD_SETTINGS_STR, OGRE_DEPENDENCY_LIBS )
else()
	message( STATUS "Detected DLL build of OgreNext" )
	unset( OGRE_STATIC )
endif()
findOgreBuildSetting( ${OGRE_BUILD_SETTINGS_STR} OGRE_BUILD_RENDERSYSTEM_GL3PLUS )
findOgreBuildSetting( ${OGRE_BUILD_SETTINGS_STR} OGRE_BUILD_RENDERSYSTEM_D3D11 )
findOgreBuildSetting( ${OGRE_BUILD_SETTINGS_STR} OGRE_BUILD_RENDERSYSTEM_METAL )
findOgreBuildSetting( ${OGRE_BUILD_SETTINGS_STR} OGRE_BUILD_RENDERSYSTEM_VULKAN )
unset( OGRE_BUILD_SETTINGS_STR )

if( NOT APPLE )
  # Create debug libraries with _d suffix
  set( OGRE_DEBUG_SUFFIX "_d" )
endif()

if( NOT IOS )
	set( CMAKE_PREFIX_PATH "${OGRE_SOURCE}/Dependencies ${CMAKE_PREFIX_PATH}" )
	find_package( SDL2 )
	if( NOT SDL2_FOUND )
		message( "Could not find SDL2. https://www.libsdl.org/" )
	else()
		message( STATUS "Found SDL2" )
		include_directories( ${SDL2_INCLUDE_DIR} )
		set( OGRE_DEPENDENCY_LIBS ${OGRE_DEPENDENCY_LIBS} ${SDL2_LIBRARY} )
	endif()
endif()

if( ${OGRE_USE_SCENE_FORMAT} )
	set( OGRE_LIBRARIES ${OGRE_LIBRARIES}
		debug ${OGRE_NEXT}SceneFormat${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
		optimized ${OGRE_NEXT}SceneFormat${OGRE_STATIC}
		)
endif()

if( ${OGRE_USE_PLANAR_REFLECTIONS} )
	set( OGRE_LIBRARIES ${OGRE_LIBRARIES}
		debug ${OGRE_NEXT}PlanarReflections${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
		optimized ${OGRE_NEXT}PlanarReflections${OGRE_STATIC}
		)
endif()

if( NOT OGRE_BUILD_COMPONENT_ATMOSPHERE EQUAL -1 )
	message( STATUS "Detected Atmosphere Component. Linking against it." )
	set( OGRE_LIBRARIES ${OGRE_LIBRARIES}
		debug ${OGRE_NEXT}Atmosphere${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
		optimized ${OGRE_NEXT}Atmosphere${OGRE_STATIC}
		)
	include_directories( "${OGRE_SOURCE}/Components/Atmosphere/include" )
endif()

if( OGRE_STATIC )
	if( OGRE_BUILD_RENDERSYSTEM_D3D11 )
		message( STATUS "Detected D3D11 RenderSystem. Linking against it." )
		set( OGRE_LIBRARIES
			${OGRE_LIBRARIES}
			debug RenderSystem_Direct3D11${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
			optimized RenderSystem_Direct3D11${OGRE_STATIC} )
		include_directories( "${OGRE_SOURCE}/RenderSystems/Direct3D11/include" )
	endif()
	if( OGRE_BUILD_RENDERSYSTEM_GL3PLUS )
		message( STATUS "Detected GL3+ RenderSystem. Linking against it." )
		set( OGRE_LIBRARIES
			${OGRE_LIBRARIES}
			debug RenderSystem_GL3Plus${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
			optimized RenderSystem_GL3Plus${OGRE_STATIC} )
		include_directories( "${OGRE_SOURCE}/RenderSystems/GL3Plus/include"
			"${OGRE_SOURCE}/RenderSystems/GL3Plus/include/GLSL")

		if( UNIX )
			set( OGRE_DEPENDENCY_LIBS ${OGRE_DEPENDENCY_LIBS} Xt Xrandr X11 GL )
		endif()
	endif()
	if( OGRE_BUILD_RENDERSYSTEM_METAL )
		message( STATUS "Detected Metal RenderSystem. Linking against it." )
		set( OGRE_LIBRARIES
			${OGRE_LIBRARIES}
			debug RenderSystem_Metal${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
			optimized RenderSystem_Metal${OGRE_STATIC} )
		include_directories( "${OGRE_SOURCE}/RenderSystems/Metal/include" )
	endif()
	if( OGRE_BUILD_RENDERSYSTEM_VULKAN )
		message( STATUS "Detected Vulkan RenderSystem. Linking against it." )
		set( OGRE_LIBRARIES
			${OGRE_LIBRARIES}
			debug RenderSystem_Vulkan${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
			optimized RenderSystem_Vulkan${OGRE_STATIC} )
		include_directories( "${OGRE_SOURCE}/RenderSystems/Vulkan/include" )

		set( OGRE_DEPENDENCY_LIBS ${OGRE_DEPENDENCY_LIBS} xcb X11-xcb xcb-randr )
	endif()
endif()

set( OGRE_LIBRARIES
	${OGRE_LIBRARIES}

	debug ${OGRE_NEXT}Overlay${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
	debug ${OGRE_NEXT}HlmsUnlit${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
	debug ${OGRE_NEXT}HlmsPbs${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}
	debug ${OGRE_NEXT}Main${OGRE_STATIC}${OGRE_DEBUG_SUFFIX}

	optimized ${OGRE_NEXT}Overlay${OGRE_STATIC}
	optimized ${OGRE_NEXT}HlmsUnlit${OGRE_STATIC}
	optimized ${OGRE_NEXT}HlmsPbs${OGRE_STATIC}
	optimized ${OGRE_NEXT}Main${OGRE_STATIC}

	${OGRE_DEPENDENCY_LIBS}
	)

set( OGRE_LIBRARIES_OUT ${OGRE_LIBRARIES} )

# Plugins.cfg
if( NOT APPLE )
	set( OGRE_PLUGIN_DIR "Plugins" )
endif()

message( STATUS "Copying Hlms data files from Ogre repository" )
file( COPY "${OGRE_SOURCE}/Samples/Media/Hlms/Common"	DESTINATION "${CMAKE_SOURCE_DIR}/bin/Data/Hlms" )
file( COPY "${OGRE_SOURCE}/Samples/Media/Hlms/Pbs"		DESTINATION "${CMAKE_SOURCE_DIR}/bin/Data/Hlms" )
file( COPY "${OGRE_SOURCE}/Samples/Media/Hlms/Unlit"	DESTINATION "${CMAKE_SOURCE_DIR}/bin/Data/Hlms" )

message( STATUS "Copying Common data files from Ogre repository" )
file( COPY "${OGRE_SOURCE}/Samples/Media/2.0/scripts/materials/Common"	DESTINATION "${CMAKE_SOURCE_DIR}/bin/Data/Materials" )
file( COPY "${OGRE_SOURCE}/Samples/Media/packs/DebugPack.zip"	DESTINATION "${CMAKE_SOURCE_DIR}/bin/Data" )

message( STATUS "Copying DLLs and generating Plugins.cfg for Debug" )
setupPluginFileFromTemplate( "Debug" ${OGRE_USE_SCENE_FORMAT} ${OGRE_USE_PLANAR_REFLECTIONS} )
message( STATUS "Copying DLLs and generating Plugins.cfg for Release" )
setupPluginFileFromTemplate( "Release" ${OGRE_USE_SCENE_FORMAT} ${OGRE_USE_PLANAR_REFLECTIONS} )
message( STATUS "Copying DLLs and generating Plugins.cfg for RelWithDebInfo" )
setupPluginFileFromTemplate( "RelWithDebInfo" ${OGRE_USE_SCENE_FORMAT} ${OGRE_USE_PLANAR_REFLECTIONS} )
message( STATUS "Copying DLLs and generating Plugins.cfg for MinSizeRel" )
setupPluginFileFromTemplate( "MinSizeRel" ${OGRE_USE_SCENE_FORMAT} ${OGRE_USE_PLANAR_REFLECTIONS} )

setupResourceFileFromTemplate()
setupOgreSamplesCommon()

endmacro()

#----------------------------------------------------------------------------------------

macro( addStaticDependencies OGRE_SOURCE, OGRE_BINARIES, OGRE_BUILD_SETTINGS_STR, OGRE_DEPENDENCY_LIBS )
	if( WIN32 )
		# Win32 seems to be the only one actually doing Debug builds for Dependencies w/ _d
		set( OGRE_DEP_DEBUG_SUFFIX "_d" )
	endif()

	if( WIN32 )
		set( OGRE_DEPENDENCIES "${OGRE_SOURCE}/Dependencies/lib/$(ConfigurationName)" CACHE STRING
			 "Path to OGRE-Next's dependencies folder. Only used in Static Builds" )
	elseif( IOS )
		set( OGRE_DEPENDENCIES "${OGRE_SOURCE}/iOSDependencies/lib/$(CONFIGURATION)" CACHE STRING
			 "Path to OGRE-Next's dependencies folder. Only used in Static Builds" )
	elseif( APPLE )
		set( OGRE_DEPENDENCIES "${OGRE_SOURCE}/Dependencies/lib/$(CONFIGURATION)" CACHE STRING
			 "Path to OGRE-Next's dependencies folder. Only used in Static Builds" )
	elseif( ANDROID )
		set( OGRE_DEPENDENCIES "${OGRE_SOURCE}/DependenciesAndroid/lib" CACHE STRING
			"Path to OGRE-Next's dependencies folder. Only used in Static Builds" )
	else()
		set( OGRE_DEPENDENCIES "${OGRE_SOURCE}/Dependencies/lib" CACHE STRING
			 "Path to OGRE-Next's dependencies folder. Only used in Static Builds" )
	endif()
	link_directories( ${OGRE_DEPENDENCIES} )

	string( FIND "${OGRE_BUILD_SETTINGS_STR}" "#define OGRE_NO_FREEIMAGE 0" OGRE_USES_FREEIMAGE )
	if( NOT OGRE_USES_FREEIMAGE EQUAL -1 )
		message( STATUS "Static lib needs FreeImage. Linking against it." )
		set( TMP_DEPENDENCY_LIBS ${TMP_DEPENDENCY_LIBS}
			debug FreeImage${OGRE_DEP_DEBUG_SUFFIX}
			optimized FreeImage )
	endif()

	string( FIND "${OGRE_BUILD_SETTINGS_STR}" "#define OGRE_BUILD_COMPONENT_OVERLAY" OGRE_USES_OVERLAYS )
	if( NOT OGRE_USES_OVERLAYS EQUAL -1 )
		message( STATUS "Static lib needs FreeType. Linking against it." )
		set( TMP_DEPENDENCY_LIBS ${TMP_DEPENDENCY_LIBS}
			debug freetype${OGRE_DEP_DEBUG_SUFFIX}
			optimized freetype )
	endif()

	string( FIND "${OGRE_BUILD_SETTINGS_STR}" "#define OGRE_NO_ZIP_ARCHIVE 0" OGRE_USES_ZIP )
	if( NOT OGRE_USES_ZIP EQUAL -1 )
		message( STATUS "Static lib needs zzip. Linking against it." )
		if( UNIX )
			set( ZZIPNAME zziplib )
		else()
			set( ZZIPNAME zzip )
		endif()
		set( TMP_DEPENDENCY_LIBS ${TMP_DEPENDENCY_LIBS}
			debug ${ZZIPNAME}${OGRE_DEP_DEBUG_SUFFIX}
			optimized ${ZZIPNAME} )
	endif()

	message( STATUS "Static lib needs freetype due to Overlays. Linking against it." )
	set( TMP_DEPENDENCY_LIBS ${TMP_DEPENDENCY_LIBS}
		debug freetype${OGRE_DEP_DEBUG_SUFFIX}
		optimized freetype )

	if( UNIX AND NOT APPLE )
		set( TMP_DEPENDENCY_LIBS ${TMP_DEPENDENCY_LIBS} dl Xt Xrandr X11 xcb Xaw )
	endif()

	set( OGRE_DEPENDENCY_LIBS ${TMP_DEPENDENCY_LIBS} )
endmacro()
