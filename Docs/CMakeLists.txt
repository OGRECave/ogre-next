#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

###############################################################
# Install documentation
# Build API docs, if doxygen is available
###############################################################

# Set docs target directory
if (WIN32 OR APPLE)
  set(OGRE_DOCS_PATH "Docs")
elseif (UNIX)
  set(OGRE_DOCS_PATH "share/${OGRE_NEXT_PREFIX}/docs")
endif ()
  
# Build and install API documentation if doxygen is available
if(DOXYGEN_FOUND)
    if (OGRE_INSTALL_DOCS)
      install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
	DESTINATION ${OGRE_DOCS_PATH}
	PATTERN ".hg" EXCLUDE
	PATTERN "src" EXCLUDE
      )
    endif ()
    
	file(GLOB MANUAL
		${PROJECT_SOURCE_DIR}/Docs/src/manual/*.md
			${PROJECT_SOURCE_DIR}/Docs/src/manual/Introduction/*.md
			${PROJECT_SOURCE_DIR}/Docs/src/manual/Migrating/*.md
			${PROJECT_SOURCE_DIR}/Docs/src/manual/Performance/*.md
			${PROJECT_SOURCE_DIR}/Docs/src/manual/Plugins/*.md
			${PROJECT_SOURCE_DIR}/Docs/src/manual/Rendering/*.md
			${PROJECT_SOURCE_DIR}/Docs/src/manual/Scripts/*.md
			${PROJECT_SOURCE_DIR}/Docs/src/SettingUpOgre/*.md
			${PROJECT_SOURCE_DIR}/Docs/src/UsingOgre/*.md
		)

	file(GLOB COMPONENTS ${PROJECT_SOURCE_DIR}/Components/*/include ${PROJECT_SOURCE_DIR}/Components/Hlms/*/include)
	file(GLOB PLUGINS ${PROJECT_SOURCE_DIR}/PlugIns/*/include)
	file(GLOB SAMPLES ${PROJECT_SOURCE_DIR}/Samples/Media/particle/*.particle)

    set(CMAKE_DOXYGEN_INPUT_LIST 
        \"${PROJECT_SOURCE_DIR}/OgreMain/include
        ${COMPONENTS}
        ${PLUGINS}
        ${PROJECT_SOURCE_DIR}/PlugIns
        ${PROJECT_SOURCE_DIR}/RenderSystems
        ${PROJECT_SOURCE_DIR}/Docs/src/apimainpage.md
        ${MANUAL}\")
    string(REPLACE ";" "\" \\\n\"" CMAKE_DOXYGEN_INPUT_LIST "${CMAKE_DOXYGEN_INPUT_LIST}")
	configure_file(${OGRE_TEMPLATES_DIR}/html.cfg.in ${PROJECT_BINARY_DIR}/html.cfg)
	add_custom_target(OgreDoc COMMAND 
        ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/html.cfg
        DEPENDS ${PROJECT_BINARY_DIR}/html.cfg
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/Docs/src/ 
        COMMENT "Building documentation" VERBATIM)
	file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/api/)
	if (OGRE_INSTALL_DOCS)
		install(DIRECTORY ${PROJECT_BINARY_DIR}/api DESTINATION ${OGRE_DOCS_PATH})
	endif()
endif()
