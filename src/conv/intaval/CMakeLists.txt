set(tgf_SRCS read_dra.cpp regtab.cpp tgf-g.cpp write_brl.cpp)
brlcad_addexec(tgf-g "${tgf_SRCS}" "libwdb;librt;libbu" FOLDER Conv)

set(
  intaval_ignore_files
  CMakeLists.txt
  glob.h
  read_dra.h
  readme.txt
  regtab.h
  write_brl.h
)
cmakefiles(${intaval_ignore_files})

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
