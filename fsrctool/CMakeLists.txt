
add_executable (fsrctool main.c wave.c)

include_directories (${FSRC_SOURCE_DIR}/libfsrc) 
#link_directories (${FSRC_BINARY_DIR}) 

IF (MSVC)
	SET_TARGET_PROPERTIES(fsrctool PROPERTIES COMPILE_FLAGS "/TP")
ENDIF (MSVC)

target_link_libraries(fsrctool fsrc)




