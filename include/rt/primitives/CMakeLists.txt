set(
  primitives_headers
  arb8.h
  annot.h
  bot.h
  brep.h
  cline.h
  dsp.h
  ell.h
  epa.h
  hf.h
  metaball.h
  nmg.h
  pg.h
  pipe.h
  rhc.h
  rpc.h
  script.h
  sketch.h
  tgc.h
  tor.h
)
brlcad_manage_files(primitives_headers ${INCLUDE_DIR}/brlcad/rt/primitives REQUIRED librt)

cmakefiles(CMakeLists.txt)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
