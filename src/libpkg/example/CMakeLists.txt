# Include directories needed by libpkg users
set(PKG_EXAMPLE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${BU_INCLUDE_DIRS} ${PKG_INCLUDE_DIRS})
list(REMOVE_DUPLICATES PKG_EXAMPLE_INCLUDES)
include_directories(${PKG_EXAMPLE_INCLUDES})

if(TARGET libbu)
  brlcad_addexec(pkg_test_server server.c "libbu;libpkg" TEST)
  brlcad_addexec(pkg_test_client client.c "libbu;libpkg" TEST)
endif()

cmakefiles(
  CMakeLists.txt
  ncp.h
  server.c
  client.c
)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
