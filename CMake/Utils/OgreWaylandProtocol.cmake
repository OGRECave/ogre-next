#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

function(ogre_generate_wayland_protocol PROTOCOL_XML GENERATED_DIR OUT_HEADER OUT_CODE)
  get_filename_component(PROTOCOL_NAME "${PROTOCOL_XML}" NAME_WE)

  set(GENERATED_HEADER "${GENERATED_DIR}/${PROTOCOL_NAME}-client-protocol.h")
  set(GENERATED_CODE "${GENERATED_DIR}/${PROTOCOL_NAME}-protocol.c")

  add_custom_command(
    OUTPUT "${GENERATED_HEADER}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GENERATED_DIR}"
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header "${PROTOCOL_XML}" "${GENERATED_HEADER}"
    DEPENDS "${PROTOCOL_XML}"
    VERBATIM
  )

  add_custom_command(
    OUTPUT "${GENERATED_CODE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GENERATED_DIR}"
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code "${PROTOCOL_XML}" "${GENERATED_CODE}"
    DEPENDS "${PROTOCOL_XML}"
    VERBATIM
  )

  set(${OUT_HEADER} "${GENERATED_HEADER}" PARENT_SCOPE)
  set(${OUT_CODE} "${GENERATED_CODE}" PARENT_SCOPE)
endfunction()
