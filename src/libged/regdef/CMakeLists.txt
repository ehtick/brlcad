include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${BRLCAD_BINARY_DIR}/include
  ${BRLCAD_SOURCE_DIR}/include
  ${BU_INCLUDE_DIRS}
  ${GED_INCLUDE_DIRS}
)

add_definitions(-DGED_PLUGIN)
ged_plugin_library(ged-regdef SHARED regdef.c)
target_link_libraries(ged-regdef libged libbu)
set_property(TARGET ged-regdef APPEND PROPERTY COMPILE_DEFINITIONS BRLCADBUILD HAVE_CONFIG_H)
validate_style(ged-regdef regdef.c)
plugin_setup(ged-regdef ged)

cmakefiles(
  CMakeLists.txt
  regdef.c
)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
