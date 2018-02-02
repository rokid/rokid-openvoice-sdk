# - finds the uWebsockets C++ libraries

set(UWS_HINTS
	/usr/local
	/usr
	${UWSPrefix}
)

# look for the root directory, first for the source-tree variant
find_path(UWS_ROOT_DIR 
	NAMES src/Hub.h
	HINTS ${UWS_HINTS}
)

if(NOT UWS_ROOT_DIR)
	# this means uWebsockets may have a different directory structure, maybe it was installed, let's check for that
	message(STATUS "Looking for uWebsockets install directory structure.")
	find_path(UWS_ROOT_DIR 
		NAMES include/uWS/Hub.h uWS/Hub.h
		HINTS ${UWS_HINTS}
	)
	if(NOT UWS_ROOT_DIR) 
		# uWebsockets was still not found -> Fail
		if(UWS_FIND_REQUIRED)
			message(FATAL_ERROR "UWS: Could not find uWebsockets install directory")
		endif()
		if(NOT UWS_FIND_QUIETLY)
			message(STATUS "UWS: Could not find uWebsockets install directory")
		endif()
		return()
	else()
		# poco was found with the make install directory structure
		message(STATUS "Assuming uWebsockets install directory structure at ${UWS_ROOT_DIR}.")
		set(UWS_INSTALLED true)
	endif()
endif()

# if installed directory structure, set full include dir
if(UWS_INSTALLED)
	set(UWS_INCLUDE_DIRS ${UWS_ROOT_DIR}/include CACHE PATH "The global include path for uWebsockets")
endif()

find_library(
	UWS_LIBRARIES
	NAMES uWS
	HINTS ${UWS_ROOT_DIR}
	PATH_SUFFIXES
		lib
)
if(DEFINED UWS_LIBRARIES)
	message(STATUS "Found UWS: ${UWS_LIBRARIES}")
	set(UWS_FOUND true)
endif()
