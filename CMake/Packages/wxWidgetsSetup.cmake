#-------------------------------------------------------------------
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Call this macro early to setup necessary variables on to setup wxWidgets.
macro( findWxWidgets )
	if( WIN32 )
		set( wxWidgets_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/wxWidgets_3_0" CACHE STRING "Path to wxWidgets 3.0.0 source code" )
		if( EXISTS ${wxWidgets_SOURCE} )
			set( wxWidgets_FOUND TRUE )
			set( wxWidgets_INCLUDE_DIRS "${wxWidgets_SOURCE}/include" )
		else()
			set( wxWidgets_FOUND FALSE )
		endif()
	else()
		if( APPLE )
			set( wxWidgets_CONFIG_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/wxWidgets_3_0/build-release/wx-config" CACHE STRING "Path to wx-config" )
			if( EXISTS ${wxWidgets_CONFIG_EXECUTABLE} )
				set( wxWidgets_FOUND TRUE )
				find_package( wxWidgets COMPONENTS core base aui adv REQUIRED )
			else()
				set( wxWidgets_FOUND FALSE )
			endif()
		else()
			find_package( wxWidgets COMPONENTS core base aui adv )
		endif()
	endif()

	if( UNIX AND wxWidgets_FOUND )
		# Use the package PkgConfig to detect GTK+ headers/library files
		find_package( PkgConfig REQUIRED )
		pkg_check_modules( GTK3 REQUIRED gtk+-3.0 )
	endif()
endmacro()

# Call this macro for every project that needs wxWidgets. Must have called findWxWidgets first.
macro( setupProjectForWxWidgets wxWidgets_LIBRARIES )
	if( wxWidgets_FOUND )
		if( WIN32 )
			include_directories( "${wxWidgets_INCLUDE_DIRS}" )
			    # Technically we should include mswu & mswud but we assume they're equal.
				# (CMake can't do per target includes... grr....)
				if( PLATFORM_X64 )
					include_directories( "${wxWidgets_SOURCE}/lib/vc_x64_lib/mswu" )
					link_directories( "${wxWidgets_SOURCE}/lib/vc_x64_lib" )
				else()
					include_directories( "${wxWidgets_SOURCE}/lib/vc_lib/mswu" )
					link_directories( "${wxWidgets_SOURCE}/lib/vc_lib" )
				endif()

			set( wxWidgets_LIBRARIES
				debug wxbase30ud
				debug wxmsw30ud_core
				debug wxmsw30ud_adv
				debug wxmsw30ud_aui
				debug wxpngd
				debug wxzlibd

				optimized wxbase30u
				optimized wxmsw30u_core
				optimized wxmsw30u_adv
				optimized wxmsw30u_aui
				optimized wxpng
				optimized wxzlib

				comctl32 Rpcrt4
			    )
		else()
			include( ${wxWidgets_USE_FILE} )
			include_directories( ${wxWidgets_INCLUDE_DIRS} )
			add_definitions( -DwxUSE_GUI=1 )
			set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${wxWidgets_CXX_FLAGS}" )
		endif()
	endif()
endmacro()
