# - finds the uWebsockets C++ libraries

set(Opus_HINTS
	/usr/local
	/usr
	${OpusPrefix}
)

# look for the root directory, first for the source-tree variant
find_path(Opus_ROOT_DIR 
	NAMES include/opus.h
	HINTS ${Opus_HINTS}
)
if(NOT Opus_ROOT_DIR)
	find_path(Opus_ROOT_DIR 
		NAMES include/opus/opus.h
		HINTS ${Opus_HINTS}
	)
	if(NOT Opus_ROOT_DIR)
		if(Opus_FIND_REQUIRED)
			message(FATAL_ERROR "Opus: Could not find opus install directory")
		endif()
		if(NOT Opus_FIND_QUIETLY)
			message(STATUS "Opus: Could not find opus install directory")
		endif()
		return()
	else()
		set(Opus_INSTALLED true)
		# poco was found with the make install directory structure
		message(STATUS "Assuming opus install directory structure at ${Opus_ROOT_DIR}.")
	endif()
endif()

if (Opus_INSTALLED)
	set(Opus_INCLUDE_DIRS ${Opus_ROOT_DIR}/include/opus CACHE PATH "The global include path for opus")
else()
	set(Opus_INCLUDE_DIRS ${Opus_ROOT_DIR}/include CACHE PATH "The global include path for opus")
endif()

find_library(
	Opus_LIBRARIES
	NAMES opus
	HINTS ${Opus_ROOT_DIR}
	PATH_SUFFIXES
		lib
)
if(DEFINED Opus_LIBRARIES)
	message(STATUS "Found Opus: ${Opus_LIBRARIES}")
	set(Opus_FOUND true)
endif()
