add_subdirectory(images)

set(
  archer_TCLSCRIPTS
  Arb4EditFrame.tcl
  Arb5EditFrame.tcl
  Arb6EditFrame.tcl
  Arb7EditFrame.tcl
  Arb8EditFrame.tcl
  Archer.tcl
  ArcherCore.tcl
  AttrGroupsDisplayUtility.tcl
  BotEditFrame.tcl
  BotUtility.tcl
  BrepEditFrame.tcl
  CombEditFrame.tcl
  DataUtils.tcl
  EhyEditFrame.tcl
  EllEditFrame.tcl
  EpaEditFrame.tcl
  EtoEditFrame.tcl
  ExtrudeEditFrame.tcl
  GeometryEditFrame.tcl
  GripEditFrame.tcl
  HalfEditFrame.tcl
  HypEditFrame.tcl
  JointEditFrame.tcl
  LoadArcherLibs.tcl
  LODUtility.tcl
  MetaballEditFrame.tcl
  PartEditFrame.tcl
  PipeEditFrame.tcl
  Plugin.tcl
  RhcEditFrame.tcl
  RpcEditFrame.tcl
  ShaderEdit.tcl
  SketchEditFrame.tcl
  SphereEditFrame.tcl
  SuperellEditFrame.tcl
  TgcEditFrame.tcl
  TorusEditFrame.tcl
  Utility.tcl
  Wizard.tcl
  bgerror.tcl
  itk_redefines.tcl
  tabwindow.itk
)
if(BRLCAD_ENABLE_TCL)
  brlcad_adddata(archer_TCLSCRIPTS tclscripts/archer)
  pkgindex_build(tclscripts/archer)
  tclindex_build(tclscripts/archer)
endif(BRLCAD_ENABLE_TCL)

cmakefiles(
  CMakeLists.txt
  ${archer_TCLSCRIPTS}
)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
