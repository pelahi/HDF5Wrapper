cmake_minimum_required(VERSION 3.13)
project(hdf5wrapper)

set(default_build_type "Release")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 11)

#
# Options users can give on the command line via -D
#
macro(hdf5wrapper_option optname optdesc status)
	option(HDF5WRAPPER_${optname} "${optdesc}" "${status}")
endmacro()

# set default options
hdf5wrapper_option(USEMPI "Use MPI" OFF)
hdf5wrapper_option(ALLOWCOMPRESSIONHDF5 "Attempt to include HDF5 compression support " ON)
hdf5wrapper_option(ALLOWPARALLELHDF5 "Attempt to include parallel HDF5 support " ON)
hdf5wrapper_option(ALLOWCOMPRESSIONPARALLELHDF5 "Attempt to include parallel HDF5 compression support " OFF)

# find hdf5 macro where flags set if parallel, if particular version, if compression
macro(find_hdf5)
	find_package(HDF5 REQUIRED COMPONENTS C)
	if (HDF5_FOUND)
		include_directories(${HDF5_INCLUDE_DIRS})
		if (HDF5_IS_PARALLEL AND HDF5WRAPPER_ALLOWPARALLELHDF5)
			set (ENV{HDF5_PREFER_PARALLEL} true)
			set(HDF5WRAPPER_HAS_PARALLEL_HDF5 Yes)
			list(APPEND HDF5WRAPPER_DEFINES USEPARALLELHDF)
			if (HDF5_VERSION VERSION_GREATER "1.10.0" AND HDF5WRAPPER_ALLOWCOMPRESSIONPARALLELHDF5)
				set(HDF5WRAPPER_HAS_COMPRESSED_HDF5 Yes)
				list(APPEND HDF5WRAPPER_DEFINES USEHDFCOMPRESSION)
				list(APPEND HDF5WRAPPER_DEFINES PARALLELCOMPRESSIONACTIVE)
			endif()
		else()
			if (HDF5WRAPPER_ALLOWCOMPRESSIONHDF5)
				set(HDF5WRAPPER_HAS_COMPRESSED_HDF5 Yes)
				list(APPEND HDF5WRAPPER_DEFINES USEHDFCOMPRESSION)
			endif()
		endif()
    endif()
endmacro()

#
# How we find MPI and set it up
#
macro(find_mpi)
	find_package(MPI)
	if (MPI_FOUND)
        include_directories(${MPI_CXX_INCLUDE_PATH})
		list(APPEND HDF5_CONVERT_DEFINES USEMPI)
		set(HDF5WRAPPER_HAS_MPI Yes)
	endif()
endmacro()

set(HDF5WRAPPER_HAS_COMPRESSED_HDF5 No)
set(HDF5WRAPPER_HAS_PARALLEL_HDF5 No)
find_hdf5()
set(HDF5WRAPPER_HAS_MPI No)
if (HDF5WRAPPER_MPI)
	find_mpi()
endif()


if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    set (HDF5WRAPPER_USEOPENMP)
endif()

if (Verbose)
    ADD_DEFINITIONS(-D_VERBOSE_)
endif()

if (Timer)
    ADD_DEFINITIONS(-D_TIMER_)
endif()

#
# Tell the world what what we are doing
#
macro(hdf5wrapper_report feature)

	# Output feature name and underscore it in the next line
	message("\n${feature}")
	string(REGEX REPLACE "." "-" _underscores ${feature})
	message("${_underscores}\n")

	set(_args "${ARGN}")
	list(LENGTH _args _nargs)
	math(EXPR _nargs "${_nargs} - 1")
	foreach(_idx RANGE 0 ${_nargs} 2)

		# Items in the list come with a message first, then the variable name
		list(GET _args ${_idx} _msg)
		math(EXPR _idx2 "${_idx} + 1")
		list(GET _args ${_idx2} _varname)

		# We try to keep things up to 80 cols
		string(LENGTH ${_msg} _len)
		math(EXPR _nspaces "75 - ${_len}")
		string(RANDOM LENGTH ${_nspaces} _spaces)
		string(REGEX REPLACE "." " " _spaces "${_spaces}")
		string(CONCAT _msg "${_msg}" ${_spaces})
		message(" ${_msg} ${HDF5WRAPPER_HAS_${_varname}}")
	endforeach()
endmacro()

message("\nHDF Converter successfully configured with the following settings:")
hdf5wrapper_report("HDF5 Settings"
    "Compressed HDF5" COMPRESSED_HDF5
    "Parallel HDF5" PARALLEL_HDF5
)
if (HDF5WRAPPER_HAS_COMPRESSED_HDF5 AND HDF5WRAPPER_HAS_PARALLEL_HDF5)
	message("\n WARNING: Parallel Compression HDF5 active, use with caution as it is unstable!\n")
endif()
hdf5wrapper_report("Parallel settings"
    "OpenMP" OPENMP
    "MPI" MPI
)


set(LINK_LIBS ${LINK_LIBS} ${HDF5_LIBRARIES})

set(SOURCE_FILES
    ${SOURCE_FILES}
    src/HDF5Wrapper.cc
    )

add_library(hdf5wrapper ${SOURCE_FILES})
target_link_libraries(hdf5wrapper ${LINK_LIBS})
