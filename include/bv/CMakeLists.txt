set(
  bv_headers
  adc.h
  defines.h
  events.h
  faceplate.h
  lod.h
  plot3.h
  polygon.h
  snap.h
  tcl_data.h
  tig.h
  util.h
  vectfont.h
  vlist.h
  view_sets.h
)
brlcad_manage_files(bv_headers ${INCLUDE_DIR}/brlcad/bv REQUIRED libbv)

cmakefiles(CMakeLists.txt)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
