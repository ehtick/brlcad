set(
  ADRT_MASTER_SOURCES
  ../tienet.c
  compnet.c
  dispatcher.c
  master.c
  tienet_master.c
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../librender)

brlcad_addexec(adrt_master "${ADRT_MASTER_SOURCES}" "librt;${ZLIB_LIBRARIES}")

set(
  adrt_master_ignore_files
  CMakeLists.txt
  compnet.h
  dispatcher.h
  master.h
  tienet_master.h
)
cmakefiles(${adrt_master_ignore_files})

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
