set(ASC_SRCS asc.cpp asc_v4.cpp asc_v5.cpp)

gcv_plugin_library(gcv-asc SHARED ${ASC_SRCS})
target_link_libraries(gcv-asc libwdb librt libbu)
set_property(TARGET gcv-asc APPEND PROPERTY COMPILE_DEFINITIONS BRLCADBUILD HAVE_CONFIG_H)
validate_style(gcv-asc "${ASC_SRCS}")
plugin_setup(gcv-asc gcv)

cmakefiles(
  ${ASC_SRCS}
  CMakeLists.txt
)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
