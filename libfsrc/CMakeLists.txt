SET(FSRC_SOURCES
	alloc.c
	bits.c
	cache.c
	converter.c
	design.c
	enum_factors.c
	factors.c
	fft.c
	file_ioi.c
	fir_irls.c
	fir_minphase.c
	formats.c
	fsrc.c
	lpf_design.c
	nearest.c
	ols_src.c
	pps_src.c
	qfactors.c
	qpermute.c
	ratio.c
	toeplitz_pcg.c
	vupart.c
	xblas.c
)

ADD_LIBRARY (fsrc ${FSRC_SOURCES})

IF (MSVC)
	SET_TARGET_PROPERTIES(fsrc PROPERTIES COMPILE_FLAGS "/TP")
ENDIF (MSVC)

SET(FFT_LIB "FFTW" CACHE STRING "FFT Library")

IF (FFT_LIB STREQUAL "FFTW")
	find_package(FFTW REQUIRED)	
	include_directories(${FFTW_INCLUDE_DIRS})
	target_link_libraries(fsrc ${FFTW_LIBRARIES})
	SET_TARGET_PROPERTIES (fsrc PROPERTIES DEFINE_SYMBOL  "LIBFSRC_USE_FFTW")
ELSE (FFT_LIB STREQUAL "FFTW")
	message(SEND_ERROR "Unsupported FFT Library")	
ENDIF (FFT_LIB STREQUAL "FFTW")



