cmake_minimum_required (VERSION 2.6) 

PROJECT (FSRC C) 

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")


IF (CMAKE_COMPILER_IS_GNUCC)
	 SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")

ENDIF(CMAKE_COMPILER_IS_GNUCC)


IF (WIN32)

	add_definitions(
		-D_USE_MATH_DEFINES
		-D_CRT_SECURE_NO_WARNINGS
		-D_CRT_NONSTDC_NO_WARNINGS
		-DUNICODE
		-D_UNICODE
		-D_WIN32_WINNT=0x0500 
		-DWINVER=0x0500
		)

ENDIF(WIN32)


INCLUDE (CheckIncludeFiles)

INCLUDE (CheckFunctionExists)

INCLUDE (CheckTypeSize)
INCLUDE (CheckCSourceCompiles)

check_c_source_compiles("int main(int argc) { return __builtin_ctz(argc); }" HAVE_BUILTIN_CTZ)
check_c_source_compiles("int main(int argc) { return __builtin_clz(argc); }" HAVE_BUILTIN_CLZ)
check_c_source_compiles("int main(int argc) { return __builtin_popcount(argc); }" HAVE_BUILTIN_POPCOUNT)

check_c_source_compiles("int main(int argc) { return __sync_fetch_and_add(&argc, 1); }" HAVE_SYNC_FETCH_AND_ADD)



SET(CMAKE_EXTRA_INCLUDE_FILES math.h)

CHECK_FUNCTION_EXISTS(lrint HAVE_LRINT)

SET(CMAKE_EXTRA_INCLUDE_FILES)



CHECK_INCLUDE_FILES (malloc.h HAVE_MALLOC_H)

CHECK_INCLUDE_FILES (stdint.h HAVE_STDINT_H)



SET(CMAKE_EXTRA_INCLUDE_FILES stddef.h)

CHECK_TYPE_SIZE("short" SIZEOF_SHORT)

CHECK_TYPE_SIZE("int" SIZEOF_INT)

CHECK_TYPE_SIZE("long" SIZEOF_LONG)

CHECK_TYPE_SIZE("long long" SIZEOF_LONGLONG)

CHECK_TYPE_SIZE("size_t" SIZEOF_SIZE_T)

CHECK_TYPE_SIZE("ptrdiff_t" SIZEOF_PTRDIFF_T)

CHECK_TYPE_SIZE("void *" SIZEOF_PVOID)

SET(CMAKE_EXTRA_INCLUDE_FILES)



CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/config.h.in ${PROJECT_BINARY_DIR}/config.h)



include_directories(${PROJECT_BINARY_DIR})



#IF (MSVC)

#	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /TP")

#ENDIF (MSVC)



SET(CMAKE_DEBUG_POSTFIX d)



OPTION (BUILD_SHARED_LIBS "Build Shared Libraries" ON)



IF (BUILD_SHARED_LIBS)

	add_definitions (-DLIBFSRC_DLL)

ENDIF (BUILD_SHARED_LIBS)



add_subdirectory (libfsrc) 

add_subdirectory (fsrctool)