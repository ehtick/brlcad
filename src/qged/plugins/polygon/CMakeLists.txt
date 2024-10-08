if(BRLCAD_ENABLE_QT)
  include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${BRLCAD_BINARY_DIR}/${INCLUDE_DIR}
    ${BRLCAD_SOURCE_DIR}/include
    ${QTCAD_INCLUDE_DIRS}
    ${GED_INCLUDE_DIRS}
    ${RT_INCLUDE_DIRS}
    ${BU_INCLUDE_DIRS}
  )
endif(BRLCAD_ENABLE_QT)

set(polygon_srcs polygon_tool.cpp QPolyCreate.cpp QPolyMod.cpp QPolySettings.cpp)

set(qth_names QPolyCreate QPolyMod QPolySettings)
foreach(qh ${qth_names})
  set(polygon_moc_hdrs ${polygon_moc_hdrs} ${qh}.h)
endforeach(qh ${qth_names})

if(BRLCAD_ENABLE_QT)
  if(Qt6Widgets_FOUND)
    qt6_wrap_cpp(polygon_moc_srcs ${polygon_moc_hdrs})
    qt6_add_resources(icon_qrc icon.qrc)
  else()
    qt5_wrap_cpp(polygon_moc_srcs ${polygon_moc_hdrs})
    qt5_add_resources(icon_qrc icon.qrc)
  endif(Qt6Widgets_FOUND)

  qged_plugin(qged_polygon SHARED ${polygon_srcs} ${polygon_moc_srcs} ${icon_qrc})
  if(Qt6Widgets_FOUND)
    target_link_libraries(
      qged_polygon
      libqtcad
      libged
      librt
      libbu
      Qt6::Widgets
    )
  else()
    target_link_libraries(
      qged_polygon
      libqtcad
      libged
      librt
      libbu
      Qt5::Widgets
    )
  endif(Qt6Widgets_FOUND)
  set_property(TARGET qged_polygon APPEND PROPERTY COMPILE_DEFINITIONS BRLCADBUILD HAVE_CONFIG_H)
  validate_style(qged_polygon polygon_tool.cpp polygon_control.cpp)
  plugin_setup(qged_polygon qged)

  foreach(qh ${qth_names})
    distclean(moc_${qh}.cpp_parameters)
  endforeach(qh ${qth_names})
  distclean(icon.qrc.depends)
endif(BRLCAD_ENABLE_QT)

set(
  ignore_srcs
  CMakeLists.txt
  ${polygon_srcs}
  ${polygon_moc_hdrs}
  icon.qrc
  circle.svg
  ellipse.svg
  poly_create.svg
  poly_modify.svg
  polygon.svg
  rectangle.svg
  square.svg
)

cmakefiles(${ignore_srcs})

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
