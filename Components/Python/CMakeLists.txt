set(Python_ADDITIONAL_VERSIONS 3.4) # allows using python3 on Ubuntu 14.04
find_package(PythonInterp)
find_package(PythonLibs)
macro_log_feature(PYTHONLIBS_FOUND "Python" "Language bindings to use OGRE from Python" "http://www.python.org/" FALSE "" "")

find_package(SWIG 3.0.8)

if(NOT PYTHONLIBS_FOUND OR NOT SWIG_FOUND)
    set(OGRE_BUILD_COMPONENT_PYTHON OFF CACHE BOOL "" FORCE)
    message(WARNING "Python ${PYTHONLIBS_FOUND} ${SWIG_FOUND}")
    message(WARNING "Python Component disabled because SWIG or Python was not found")
    return()
endif()

include_directories(${PYTHON_INCLUDE_PATH})
include_directories("${PROJECT_BINARY_DIR}/include" "${PROJECT_SOURCE_DIR}/OgreMain/include" "${PROJECT_SOURCE_DIR}/Components/Hlms/Common/include" "${PROJECT_SOURCE_DIR}/Components/Hlms/Pbs/include" "${PROJECT_SOURCE_DIR}/Components/Hlms/Unlit/include")
include(${SWIG_USE_FILE})

set(PYTHON_SITE_PACKAGES lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/dist-packages/)

set(CMAKE_SWIG_FLAGS -builtin)

if (APPLE AND NOT ANDROID)
    # we are using libc++ on OSX
    list(APPEND CMAKE_SWIG_FLAGS -D_LIBCPP_VERSION)
endif()

macro(ogre_python_module target)
    set_target_properties(${SWIG_MODULE_${target}_REAL_NAME} PROPERTIES 
                        DEBUG_POSTFIX "")
    install(TARGETS ${SWIG_MODULE_${target}_REAL_NAME} LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES})
    install(FILES ${PROJECT_BINARY_DIR}/Components/Python/${target}.py DESTINATION ${PYTHON_SITE_PACKAGES})
endmacro()

if(MSVC)
	# TODO: Set MSVC flags
	add_definitions(-DMS_NO_COREDLL)
else()
	add_definitions(-Wno-cast-qual -Wno-shadow -Wno-maybe-uninitialized -Wno-unused-label -Wno-unused-but-set-variable -Wno-missing-declarations)
endif()
set_source_files_properties(Ogre.i PROPERTIES CPLUSPLUS ON)
swig_add_module(Ogre python Ogre.i)
swig_link_libraries(Ogre OgreMain OgreHlmsPbs OgreHlmsUnlit ${PYTHON_LIBRARIES})
ogre_python_module(Ogre)

# if(OGRE_BUILD_COMPONENT_RTSHADERSYSTEM)
#     set_source_files_properties(../RTShaderSystem/include/OgreRTShader.i PROPERTIES CPLUSPLUS ON)
#     swig_add_module(OgreRTShader python ../RTShaderSystem/include/OgreRTShader.i)
#     swig_link_libraries(OgreRTShader OgreMain OgreRTShaderSystem ${PYTHON_LIBRARIES})
#     ogre_python_module(OgreRTShader)
# endif()
# 
# if(OGRE_BUILD_COMPONENT_OVERLAY)
#     set_source_files_properties(../Overlay/include/OgreOverlay.i PROPERTIES CPLUSPLUS ON)
#     swig_add_module(OgreOverlay python ../Overlay/include/OgreOverlay.i)
#     swig_link_libraries(OgreOverlay OgreMain OgreOverlay ${PYTHON_LIBRARIES})
#     ogre_python_module(OgreOverlay)
# endif()
