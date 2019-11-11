#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

macro( add_recursive dir retVal )
	file( GLOB_RECURSE ${retVal} ${dir}/*.h ${dir}/*.cpp ${dir}/*.c )
endmacro()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

include_directories(${CMAKE_SOURCE_DIR}/Components/Hlms/Common/include)
ogre_add_component_include_dir(Hlms/Pbs)

add_recursive( ./ SOURCE_FILES )

set(SAMPLE_NAME Sample_MorphAnimations)
ogre_add_executable(${SAMPLE_NAME} WIN32 MACOSX_BUNDLE ${SOURCE_FILES} ${SAMPLE_COMMON_RESOURCES})

target_link_libraries(${SAMPLE_NAME} ${OGRE_LIBRARIES} ${OGRE_SAMPLES_LIBRARIES})
ogre_config_sample_lib(${SAMPLE_NAME})

if( APPLE )
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.ogre3d.${SAMPLE_NAME}")
    set(MACOSX_BUNDLE_BUNDLE_NAME "${SAMPLE_NAME}")
	set(MACOSX_BUNDLE_EXECUTABLE "${SAMPLE_NAME}")
	set_target_properties(${SAMPLE_NAME} PROPERTIES RESOURCE "${TUTORIAL_RESOURCES}")
	set_target_properties(${SAMPLE_NAME} PROPERTIES RESOURCE "${SAMPLE_COMMON_RESOURCES}" )
	set_target_properties(${SAMPLE_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${SAMPLES_PLIST})


	if( CMAKE_GENERATOR STREQUAL "Xcode" )
        #if (IOS)
        #    set(APP_CONTENTS_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin/$(CONFIGURATION)/${SAMPLE_NAME}.app)
	#else ()
            set(APP_CONTENTS_PATH ${CMAKE_CURRENT_BINARY_DIR}/../../../../bin/$(CONFIGURATION)/${SAMPLE_NAME}.app/Contents)
        #endif ()
    else ()
        #if (IOS)
        #    set(APP_CONTENTS_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin/${SAMPLE_NAME}.app)
	#else ()
            set(APP_CONTENTS_PATH ${CMAKE_CURRENT_BINARY_DIR}/../../../../bin/${SAMPLE_NAME}.app/Contents)
        #endif ()
    endif ()
	
	if( NOT OGRE_BUILD_PLATFORM_APPLE_IOS )
		set(FRAMEWORKS
			Ogre
	        RenderSystem_GL3PLUS
			RenderSystem_Metal
	        OgreHlmsPbs
	        OgreHlmsUnlit
	        OgreOverlay
			Plugin_ParticleFX
	    )
		
	    foreach(FWK ${FRAMEWORKS})
			add_custom_command(TARGET ${SAMPLE_NAME} POST_BUILD
				COMMAND ditto ${CMAKE_CURRENT_BINARY_DIR}/../../../../lib/macosx/${FWK}.framework ${APP_CONTENTS_PATH}/Frameworks/${FWK}.framework
			)
	    endforeach(FWK)
		
		add_custom_command(TARGET ${SAMPLE_NAME} POST_BUILD
			COMMAND mkdir -p ${APP_CONTENTS_PATH}/Resources
		)
		
		foreach(RSC ${SAMPLE_COMMON_RESOURCES})
			add_custom_command(TARGET ${SAMPLE_NAME} POST_BUILD
				COMMAND cp -r ${RSC} ${APP_CONTENTS_PATH}/Resources/
			)
	    endforeach()
	endif()
endif()
