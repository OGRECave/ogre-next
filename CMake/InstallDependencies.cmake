#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

#####################################################
# Install dependencies 
#####################################################
if (NOT APPLE AND NOT WIN32)
  return()
endif()

# TODO - most of this file assumes a common dependencies root folder
# This is not robust, we should instead source dependencies from their individual locations
option(OGRE_INSTALL_DEPENDENCIES "Install dependency libs needed for samples" TRUE)
option(OGRE_COPY_DEPENDENCIES "Copy dependency libs to the build directory" TRUE)

macro(install_debug INPUT)
  if (EXISTS ${OGRE_DEP_DIR}/bin/debug/${INPUT})
    if (IS_DIRECTORY ${OGRE_DEP_DIR}/bin/debug/${INPUT})
      install(DIRECTORY ${OGRE_DEP_DIR}/bin/debug/${INPUT} DESTINATION bin/debug CONFIGURATIONS Debug)
    else ()
      install(FILES ${OGRE_DEP_DIR}/bin/debug/${INPUT} DESTINATION bin/debug CONFIGURATIONS Debug)
    endif ()
  else()
    message(send_error "${OGRE_DEP_DIR}/bin/debug/${INPUT} did not exist, can't install!")
  endif ()
endmacro()

macro(install_release INPUT)
  if (EXISTS ${OGRE_DEP_DIR}/bin/release/${INPUT})
    if (IS_DIRECTORY ${OGRE_DEP_DIR}/bin/release/${INPUT})
      install(DIRECTORY ${OGRE_DEP_DIR}/bin/release/${INPUT} DESTINATION bin/release CONFIGURATIONS Release None "")
      install(DIRECTORY ${OGRE_DEP_DIR}/bin/release/${INPUT} DESTINATION bin/relwithdebinfo CONFIGURATIONS RelWithDebInfo)
      install(DIRECTORY ${OGRE_DEP_DIR}/bin/release/${INPUT} DESTINATION bin/minsizerel CONFIGURATIONS MinSizeRel)
    else ()
      install(FILES ${OGRE_DEP_DIR}/bin/release/${INPUT} DESTINATION bin/release CONFIGURATIONS Release None "")
      install(FILES ${OGRE_DEP_DIR}/bin/release/${INPUT} DESTINATION bin/relwithdebinfo CONFIGURATIONS RelWithDebInfo)
      install(FILES ${OGRE_DEP_DIR}/bin/release/${INPUT} DESTINATION bin/minsizerel CONFIGURATIONS MinSizeRel)
    endif ()
  else()
    message(send_error "${OGRE_DEP_DIR}/bin/release/${INPUT} did not exist, can't install!")
  endif ()
endmacro()

macro(copy_debug INPUT)
  if (EXISTS ${OGRE_DEP_DIR}/lib/debug/${INPUT})
    if (MINGW OR NMAKE)
      configure_file(${OGRE_DEP_DIR}/lib/debug/${INPUT} ${PROJECT_BINARY_DIR}/lib/${INPUT} COPYONLY)
	else ()
      if (IS_DIRECTORY ${OGRE_DEP_DIR}/lib/debug/${INPUT})
        install(DIRECTORY ${OGRE_DEP_DIR}/lib/debug/${INPUT} DESTINATION lib/debug)
      else ()
        configure_file(${OGRE_DEP_DIR}/lib/debug/${INPUT} ${PROJECT_BINARY_DIR}/lib/debug/${INPUT} COPYONLY)
      endif ()
	endif ()
  endif ()
endmacro()

macro(copy_release INPUT)
  if (EXISTS ${OGRE_DEP_DIR}/lib/release/${INPUT})
    if (MINGW OR NMAKE)
      configure_file(${OGRE_DEP_DIR}/lib/release/${INPUT} ${PROJECT_BINARY_DIR}/lib/${INPUT} COPYONLY)
	else ()
      if (IS_DIRECTORY ${OGRE_DEP_DIR}/lib/release/${INPUT})
        install(DIRECTORY ${OGRE_DEP_DIR}/lib/release/${INPUT} DESTINATION lib/release CONFIGURATIONS Release None "")
        install(DIRECTORY ${OGRE_DEP_DIR}/lib/release/${INPUT} DESTINATION lib/relwithdebinfo CONFIGURATIONS RelWithDebInfo)
        install(DIRECTORY ${OGRE_DEP_DIR}/lib/release/${INPUT} DESTINATION lib/minsizerel CONFIGURATIONS MinSizeRel)
      else ()
        configure_file(${OGRE_DEP_DIR}/lib/release/${INPUT} ${PROJECT_BINARY_DIR}/lib/release/${INPUT} COPYONLY)
        configure_file(${OGRE_DEP_DIR}/lib/release/${INPUT} ${PROJECT_BINARY_DIR}/lib/relwithdebinfo/${INPUT} COPYONLY)
        configure_file(${OGRE_DEP_DIR}/lib/release/${INPUT} ${PROJECT_BINARY_DIR}/lib/minsizerel/${INPUT} COPYONLY)
      endif ()
	endif ()
  endif ()
endmacro ()

if (OGRE_INSTALL_DEPENDENCIES)
  if (OGRE_STATIC)
    # for static builds, projects must link against all Ogre dependencies themselves, so copy full include and lib dir
    if (EXISTS ${OGRE_DEP_DIR}/include/)
      install(DIRECTORY ${OGRE_DEP_DIR}/include/ DESTINATION include)
    endif ()
    
    if (EXISTS ${OGRE_DEP_DIR}/lib/)
        install(DIRECTORY ${OGRE_DEP_DIR}/lib/ DESTINATION lib)
    endif ()
  endif () # OGRE_STATIC
    
  if(WIN32)
	if( OGRE_PROFILING_PROVIDER STREQUAL "remotery" )
	  install_debug(Remotery_d.dll)
	  install_release(Remotery.dll)
	endif ()

	if( OGRE_BUILD_SAMPLES2 )
	  install_debug(SDL2.dll)
	  install_release(SDL2.dll)
	  install_debug(openvr_api.dll)
	  install_release(openvr_api.dll)
	  install_debug(openvr_api.pdb)
	  install_release(openvr_api.pdb)
	endif ()

	if( OGRE_CONFIG_AMD_AGS )
		if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
			install_debug(amd_ags_x64.dll)
			install_release(amd_ags_x64.dll)
		else()
			install_debug(amd_ags_x86.dll)
			install_release(amd_ags_x86.dll)
		endif()
	endif()

    # install GLES dlls
    if (OGRE_BUILD_RENDERSYSTEM_GLES)
      install_debug(libgles_cm.dll)
      install_release(libgles_cm.dll)
    endif ()

    # install GLES2 dlls
    if (OGRE_BUILD_RENDERSYSTEM_GLES2)
      install_debug(libGLESv2.dll)
      install_release(libEGL.dll)
    endif ()
  endif () # WIN32
endif () # OGRE_INSTALL_DEPENDENCIES

if (OGRE_COPY_DEPENDENCIES)

  if (WIN32)
    # copy the required DLLs to the build directory (configure_file is the only copy-like op I found in CMake)
	if( (OGRE_BUILD_SAMPLES2 OR OGRE_BUILD_TESTS) )
		if(EXISTS ${SDL2_BINARY_DBG} AND EXISTS ${SDL2_BINARY_REL})
		  file(COPY ${SDL2_BINARY_DBG} DESTINATION ${PROJECT_BINARY_DIR}/bin/debug)
		  file(COPY ${SDL2_BINARY_REL} DESTINATION ${PROJECT_BINARY_DIR}/bin/release)
		  file(COPY ${SDL2_BINARY_REL} DESTINATION ${PROJECT_BINARY_DIR}/bin/relwithdebinfo)
		  file(COPY ${SDL2_BINARY_REL} DESTINATION ${PROJECT_BINARY_DIR}/bin/minsizerel)
		endif()
		if(EXISTS ${OpenVR_BINARY_DBG} AND EXISTS ${OpenVR_BINARY_REL})
		  file(COPY ${OpenVR_BINARY_DBG} DESTINATION ${PROJECT_BINARY_DIR}/bin/debug)
		  file(COPY ${OpenVR_BINARY_REL} DESTINATION ${PROJECT_BINARY_DIR}/bin/release)
		  file(COPY ${OpenVR_BINARY_REL} DESTINATION ${PROJECT_BINARY_DIR}/bin/relwithdebinfo)
		  file(COPY ${OpenVR_BINARY_REL} DESTINATION ${PROJECT_BINARY_DIR}/bin/minsizerel)
		endif()
	endif()

	if( OGRE_PROFILING_PROVIDER STREQUAL "remotery" )
		if(EXISTS ${Remotery_BINARY_DBG} AND EXISTS ${Remotery_BINARY_REL})
		  file(COPY ${Remotery_BINARY_DBG} DESTINATION ${PROJECT_BINARY_DIR}/bin/debug)
		  file(COPY ${Remotery_BINARY_REL} DESTINATION ${PROJECT_BINARY_DIR}/bin/release)
		  file(COPY ${Remotery_BINARY_REL} DESTINATION ${PROJECT_BINARY_DIR}/bin/relwithdebinfo)
		  file(COPY ${Remotery_BINARY_REL} DESTINATION ${PROJECT_BINARY_DIR}/bin/minsizerel)
		endif()
	endif()

	if( OGRE_CONFIG_AMD_AGS )
		if(EXISTS ${AMDAGS_BINARY_DBG} AND EXISTS ${AMDAGS_BINARY_REL})
		  file(COPY ${AMDAGS_BINARY_DBG} DESTINATION ${PROJECT_BINARY_DIR}/bin/debug)
		  file(COPY ${AMDAGS_BINARY_REL} DESTINATION ${PROJECT_BINARY_DIR}/bin/release)
		  file(COPY ${AMDAGS_BINARY_REL} DESTINATION ${PROJECT_BINARY_DIR}/bin/relwithdebinfo)
		  file(COPY ${AMDAGS_BINARY_REL} DESTINATION ${PROJECT_BINARY_DIR}/bin/minsizerel)
		endif()
	endif()
   
    if (OGRE_BUILD_RENDERSYSTEM_GLES)
      copy_debug(libgles_cm.dll)
      copy_release(libgles_cm.dll)
    endif ()
    
    if (OGRE_BUILD_RENDERSYSTEM_GLES2)	
      copy_debug(libEGL.dll)
      copy_debug(libGLESv2.dll)
      copy_release(libEGL.dll)
      copy_release(libGLESv2.dll)
    endif ()
  endif ()

endif ()
