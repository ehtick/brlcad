set(DPLOT_READER dplot)

brlcad_find_package(PERPLEX REQUIRED NOINSTALL)
brlcad_find_package(LEMON REQUIRED NOINSTALL)
brlcad_find_package(RE2C REQUIRED NOINSTALL)

perplex_target(
  ${DPLOT_READER}_scanner
  ${DPLOT_READER}_scanner.perplex
  OUT_SRC_FILE ${DPLOT_READER}_scanner.c
  OUT_HDR_FILE ${DPLOT_READER}_scanner.h
)
lemon_target(
  ${DPLOT_READER}_parser
  ${DPLOT_READER}_parser.lemon
  OUT_SRC_FILE ${DPLOT_READER}_parser.c
  OUT_HDR_FILE ${DPLOT_READER}_parser.h
)
add_perplex_lemon_dependency(${DPLOT_READER}_scanner ${DPLOT_READER}_parser)
distclean(${CMAKE_CURRENT_BINARY_DIR}/${DPLOT_READER}_parser_${DPLOT_READER}_parser/${DPLOT_READER}_parser.lemon)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${BRLCAD_BINARY_DIR}/include
  ${BRLCAD_SOURCE_DIR}/include
  ${BU_INCLUDE_DIRS}
  ${GED_INCLUDE_DIRS}
  ${PERPLEX_${DPLOT_READER}_scanner_INCLUDE_DIR}
  ${LEMON_${DPLOT_READER}_parser_INCLUDE_DIR}
)

set(
  BREP_SRCS
  ${PERPLEX_${DPLOT_READER}_scanner_SRC}
  ${LEMON_${DPLOT_READER}_parser_SRC}
  brep.cpp
  conversion.cpp
  csg.cpp
  dplot.c
  geometry.cpp
  info.cpp
  intersect.cpp
  pick.cpp
  plot.cpp
  repair.cpp
  tikz.cpp
  topology.cpp
  valid.cpp
)

ged_plugin_library(ged-brep ${BREP_SRCS})
target_link_libraries(ged-brep libged libbu)

cmakefiles(
  CMakeLists.txt
  ${BREP_SRCS}
  dplot_parser.lemon
  dplot_reader.c
  dplot_reader.h
  dplot_scanner.perplex
)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
