add_executable(fftc fftc.c splitditc.c)
target_include_directories(fftc BEFORE PRIVATE ${CMAKE_BINARY_DIR}/include ${CMAKE_SOURCE_DIR}/include)
set_property(TARGET fftc APPEND PROPERTY COMPILE_DEFINITIONS BRLCADBUILD HAVE_CONFIG_H)
settargetfolder(fftc "Compilation Utilities")
target_link_libraries(fftc ${M_LIBRARY})
cmakefiles(fftc.c splitditc.c)

add_executable(ifftc ifftc.c ditsplitc.c)
target_include_directories(ifftc BEFORE PRIVATE ${CMAKE_BINARY_DIR}/include ${CMAKE_SOURCE_DIR}/include)
set_property(TARGET ifftc APPEND PROPERTY COMPILE_DEFINITIONS BRLCADBUILD HAVE_CONFIG_H)
settargetfolder(ifftc "Compilation Utilities")
target_link_libraries(ifftc ${M_LIBRARY})
cmakefiles(ifftc.c ditsplitc.c)

# We need to generate some of libfft's source files
function(fft_gen numlist targetdir outfiles)
  file(MAKE_DIRECTORY "${targetdir}")
  foreach(filenum ${numlist})
    add_custom_command(
      OUTPUT ${targetdir}/rfft${filenum}.c
      COMMAND fftc ${filenum} > ${targetdir}/rfft${filenum}.c
      DEPENDS fftc
    )
    set(output_files "${output_files};${targetdir}/rfft${filenum}.c")
    add_custom_command(
      OUTPUT ${targetdir}/irfft${filenum}.c
      COMMAND ifftc ${filenum} > ${targetdir}/irfft${filenum}.c
      DEPENDS ifftc
    )
    set(output_files "${output_files};${targetdir}/irfft${filenum}.c")
  endforeach(filenum ${FFT_NUMLIST})
  set(${outfiles} "${output_files}" PARENT_SCOPE)
endfunction(fft_gen numlist targetdir outfiles)

# 512 and 1024 are tough in optimized compiles, don't
# go that high until there's a demonstrated need. Only
# use in BRL-CAD right now is 256.
set(FFT_NUMLIST "16;32;64;128;256")

set(LIBFFT_SRCS fftfast.c splitdit.c ditsplit.c)
fft_gen("${FFT_NUMLIST}" "${CMAKE_CURRENT_BINARY_DIR}/shared" FFT_GEN_SHARED_SRCS)
fft_gen("${FFT_NUMLIST}" "${CMAKE_CURRENT_BINARY_DIR}/static" FFT_GEN_STATIC_SRCS)

# Note - libfft_deps is defined by ${BRLCAD_SOURCE_DIR}/src/source_dirs.cmake
brlcad_addlib(libfft "${LIBFFT_SRCS}" "${libfft_deps};${M_LIBRARY}"
  "" "${CMAKE_BINARY_DIR}/include;${CMAKE_SOURCE_DIR}/include"
  SHARED_SRCS "${FFT_GEN_SHARED_SRCS}" STATIC_SRCS "${FFT_GEN_STATIC_SRCS}"
)

set_target_properties(libfft PROPERTIES LINKER_LANGUAGE C)
set_target_properties(libfft PROPERTIES VERSION 20.0.1 SOVERSION 20)

add_executable(fftest fftest.c)
target_include_directories(fftest BEFORE PRIVATE ${CMAKE_BINARY_DIR}/include ${CMAKE_SOURCE_DIR}/include)
set_property(TARGET fftest APPEND PROPERTY COMPILE_DEFINITIONS BRLCADBUILD HAVE_CONFIG_H)
settargetfolder(fftest "Compilation Utilities")
target_link_libraries(fftest libfft ${M_LIBRARY})
cmakefiles(fftest.c)
cmakefiles(CMakeLists.txt)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
