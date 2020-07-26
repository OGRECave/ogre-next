#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure CmgenToCubemap

macro( add_recursive dir retVal )
	file( GLOB_RECURSE ${retVal} ${dir}/*.h ${dir}/*.cpp ${dir}/*.c )
endmacro()

add_recursive( ./ SOURCE_FILES )

ogre_add_executable(OgreCmgenToCubemap ${SOURCE_FILES})

target_link_libraries(OgreCmgenToCubemap ${OGRE_LIBRARIES})

if (APPLE)
    set_target_properties(OgreCmgenToCubemap PROPERTIES
        LINK_FLAGS "-framework Carbon -framework Cocoa")
endif ()

ogre_config_tool(OgreCmgenToCubemap)
