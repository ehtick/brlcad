brlcad_find_package(OSMESA REQUIRED)

set(SWRAST_SRCS dm-swrast.cpp fb-swrast.cpp ../dm-gl.c ../dm-gl_lod.cpp)

# Qt is used for the stand-alone FB window, but other than that should not be
# used by swrast code.  If we are building without Qt, swrast can still be used
# in a "headless" configuration, with screengrab able to retrieve and save
# images of the drawn scene.
if(BRLCAD_ENABLE_QT)
  find_package_qt(REQUIRED)

  add_definitions(-DSWRAST_QT)

  set(SWRAST_SRCS ${SWRAST_SRCS} swrastwin.cpp)
  if(Qt6Widgets_FOUND)
    qt6_wrap_cpp(swrast_moc_srcs swrastwin.h)
  else()
    qt5_wrap_cpp(swrast_moc_srcs swrastwin.h)
  endif(Qt6Widgets_FOUND)

  distclean(moc_swrastwin.cpp_parameters)
endif(BRLCAD_ENABLE_QT)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${BRLCAD_BINARY_DIR}/include
  ${BRLCAD_SOURCE_DIR}/include
  ${BU_INCLUDE_DIRS}
  ${OSMESA_INCLUDE_DIR}
)

add_definitions(-DDM_PLUGIN)

# Use the Mesa mechanism for mangling names so we don't collide with any
# system OpenGL libs
add_definitions(-DUSE_MGL_NAMESPACE)

set(SWRAST_SRCS ${SWRAST_SRCS} ${swrast_moc_srcs})

dm_plugin_library(dm-swrast SHARED ${SWRAST_SRCS})
if(BRLCAD_ENABLE_QT)
  if(Qt6Widgets_FOUND)
    target_link_libraries(
      dm-swrast
      libqtcad
      libdm
      libbu
      ${OSMESA_LIBRARY}
      Qt6::Core
      Qt6::Widgets
    )
  else()
    target_link_libraries(
      dm-swrast
      libqtcad
      libdm
      libbu
      ${OSMESA_LIBRARY}
      Qt5::Core
      Qt5::Widgets
    )
  endif(Qt6Widgets_FOUND)
else(BRLCAD_ENABLE_QT)
  target_link_libraries(dm-swrast libdm libbu ${OSMESA_LIBRARY})
endif(BRLCAD_ENABLE_QT)
set_property(TARGET dm-swrast APPEND PROPERTY COMPILE_DEFINITIONS BRLCADBUILD HAVE_CONFIG_H OSMESA)
validate_style(dm-swrast "${SWRAST_SRCS}")

plugin_setup(dm-swrast dm)

cmakefiles(
  CMakeLists.txt
  ${SWRAST_SRCS}
  swrastwin.cpp
  dm-swrast.h
  swrastwin.h
)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
