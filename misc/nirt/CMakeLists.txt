set(
  nirt_format_files
  csv.nrt
  csv-gap.nrt
  default.nrt
  diff.nrt
  entryexit.nrt
  gap1.nrt
  gap2.nrt
  ged.nrt
)
if(TARGET libged OR TARGET nirt)
  brlcad_adddata(nirt_format_files nirt)
  if(TARGET libged)
    add_dependencies(libged nirt_format_files_cp)
  endif(TARGET libged)
  if(TARGET nirt)
    add_dependencies(nirt nirt_format_files_cp)
  endif(TARGET nirt)
endif(TARGET libged OR TARGET nirt)

cmakefiles(
  CMakeLists.txt
  ${nirt_format_files}
)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
