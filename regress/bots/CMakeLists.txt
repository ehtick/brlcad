if(SH_EXEC AND TARGET mged)
  brlcad_add_test(NAME regress-bots COMMAND ${SH_EXEC} "${CMAKE_CURRENT_SOURCE_DIR}/bots.sh" ${CMAKE_SOURCE_DIR})
  brlcad_regression_test(regress-bots "mged;pixdiff;rt" TEST_DEFINED)
endif(SH_EXEC AND TARGET mged)

cmakefiles(
  bots.sh
)

# list of temporary files
set(
  bots_outfiles
  bots.diff.log
  bots.diff.pix
  bots.g
  bots.lh.pix
  bots.log
  bots.no.pix
  bots.rh.pix
  bots.rl.diff.pix
  bots.rn.diff.pix
  bots.rs.diff.pix
  bots.sph.pix
  bots.sync.pix
)

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${bots_outfiles}")
distclean(${bots_outfiles})

cmakefiles(CMakeLists.txt)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
