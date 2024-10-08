brlcad_find_package(PNG REQUIRED)

if(MSVC)
  add_definitions(-D_CONSOLE -DIF_WGL)
endif(MSVC)

if(BRLCAD_ENABLE_TK)
  brlcad_addexec(bombardier bombardier.c "libtclcad;libbu;${TK_LIBRARY};${TCL_LIBRARY}")
  brlcad_addexec(plot3-dm plot3-dm.c "libtclcad;libdm;libbv;libbu;${TK_LIBRARY};${TCL_LIBRARY};${M_LIBRARY}")
  add_target_deps(plot3-dm dm_plugins)
else(BRLCAD_ENABLE_TK)
  cmakefiles(
    bombardier.c
    plot3-dm.c
  )
endif(BRLCAD_ENABLE_TK)

brlcad_adddata(plot3-dm.c sample_applications)

if(LIBPC_DIR)
  brlcad_addexec(pc_test pc_test.c "libwdb;libpc" NO_INSTALL)
else(LIBPC_DIR)
  cmakefiles(pc_test.c)
endif(LIBPC_DIR)

brlcad_addexec(alias-pix alias-pix.c libbu FOLDER Util)
brlcad_addexec(ap-pix ap-pix.c libbu FOLDER Util)
brlcad_addexec(asc-plot3 asc-plot3.c "libbv;libbn;libbu" FOLDER Util)
brlcad_addexec(bw-a bw-a.c "libdm;libbu" FOLDER Util)
add_target_deps(bw-a dm_plugins)
brlcad_addexec(bw-pix bw-pix.c "libbu;libicv" FOLDER Util)
brlcad_addexec(bw-png bw-png.c "libdm;libbu;${PNG_LIBRARIES}" FOLDER Util)
add_target_deps(bw-png dm_plugins)
brlcad_addexec(bw-ps bw-ps.c libbu FOLDER Util)
brlcad_addexec(bwcrop bwcrop.c libbu FOLDER Util)
brlcad_addexec(bwdiff bwdiff.c libbu FOLDER Util)
brlcad_addexec(bwfilter bwfilter.c "libicv;libbu" FOLDER Util)
brlcad_addexec(bwhist bwhist.c libdm FOLDER Util)
add_target_deps(bwhist dm_plugins)
brlcad_addexec(bwhisteq bwhisteq.c libbu FOLDER Util)
brlcad_addexec(bwmod bwmod.c "libbu;${M_LIBRARY}" FOLDER Util)
brlcad_addexec(bwrect bwrect.c "libbu;libicv" FOLDER Util)
brlcad_addexec(bwrot bwrot.c "libbu;${M_LIBRARY}" FOLDER Util)
brlcad_addexec(bwscale bwscale.c libbu FOLDER Util)
brlcad_addexec(bwshrink bwshrink.c libbu FOLDER Util)
brlcad_addexec(bwstat bwstat.c "libbu;${M_LIBRARY}" FOLDER Util)
brlcad_addexec(bwthresh bwthresh.c libbu FOLDER Util)
brlcad_addexec(cv cv.c libbu FOLDER Util)
brlcad_addexec(decimate decimate.c libbu FOLDER Util)
brlcad_addexec(double-asc double-asc.c "libdm;libbu" FOLDER Util)
add_target_deps(double-asc dm_plugins)
brlcad_addexec(dpix-pix dpix-pix.c libbu FOLDER Util)
brlcad_addexec(gencolor gencolor.c libbu FOLDER Util)
brlcad_addexec(halftone halftone.c "librt;libdm;libbu" FOLDER Util)
add_target_deps(halftone dm_plugins)

# This is intended to replace all of the img-img utils eventually
brlcad_addexec(icv icv.cpp "libicv;libbu" FOLDER Util)

# Should (eventually) have the pixdiff functionality, but be more
# flexible about input and output formats...
brlcad_addexec(imgdiff imgdiff.cpp "libicv;libbu" FOLDER Util)

brlcad_addexec(imgdims imgdims.c "libdm;libbu" FOLDER Util)
add_target_deps(imgdims dm_plugins)
brlcad_addexec(loop loop.c libbu FOLDER Util)
brlcad_addexec(lowp lowp.c libbu FOLDER Util)
brlcad_addexec(mac-pix mac-pix.c libbu FOLDER Util)
brlcad_addexec(random random.c "libbn;libbu" FOLDER Util)
brlcad_addexec(pix-alias pix-alias.c libbu FOLDER Util)
brlcad_addexec(pix-bw pix-bw.c "libbu;libicv" FOLDER Util)
brlcad_addexec(pix-png pix-png.c "libdm;libbu;${PNG_LIBRARIES}" FOLDER Util)
add_target_deps(pix-png dm_plugins)
brlcad_addexec(pix-ppm pix-ppm.c "libdm;libbu" FOLDER Util)
add_target_deps(pix-ppm dm_plugins)
brlcad_addexec(pix-ps pix-ps.c libbu FOLDER Util)
brlcad_addexec(pix-spm pix-spm.c libbn FOLDER Util)
brlcad_addexec(pix-sun pix-sun.c libbu FOLDER Util)
brlcad_addexec(pix3filter pix3filter.c libbu FOLDER Util)
brlcad_addexec(pixbackgnd pixbackgnd.c libbu FOLDER Util)
brlcad_addexec(pixbgstrip pixbgstrip.c "libdm;libbu" FOLDER Util)
add_target_deps(pixbgstrip dm_plugins)
brlcad_addexec(pixblend pixblend.c libbu FOLDER Util)
brlcad_addexec(pixborder pixborder.c "libdm;libbn;libbu;${M_LIBRARY}" FOLDER Util)
add_target_deps(pixborder dm_plugins)
brlcad_addexec(pixbustup pixbustup.c libbu FOLDER Util)
brlcad_addexec(pixclump pixclump.c "libbn;libbu" FOLDER Util)
brlcad_addexec(pixcolors pixcolors.c libbu FOLDER Util)
brlcad_addexec(pixcount pixcount.cpp libbu FOLDER Util)
brlcad_addexec(pixcrop pixcrop.c libbu FOLDER Util)
brlcad_addexec(pixdiff pixdiff.c libbu FOLDER Util)
brlcad_addexec(pixelswap pixelswap.c libbu FOLDER Util)
brlcad_addexec(pixembed pixembed.c libbu FOLDER Util)
brlcad_addexec(pixfade pixfade.c "libicv;libbu" FOLDER Util)
brlcad_addexec(pixfilter pixfilter.c libbu FOLDER Util)
brlcad_addexec(pixhalve pixhalve.c "libdm;libbu" FOLDER Util)
add_target_deps(pixhalve dm_plugins)
brlcad_addexec(pixhist pixhist.c "libdm;libbu" FOLDER Util)
add_target_deps(pixhist dm_plugins)
brlcad_addexec(pixhist3d pixhist3d.c "libdm;libbu" FOLDER Util)
add_target_deps(pixhist3d dm_plugins)
brlcad_addexec(pixhist3d-plot3 pixhist3d-plot3.c "libbv;libbn;libbu" FOLDER Util)
brlcad_addexec(pixinterp2x pixinterp2x.c libbu FOLDER Util)
brlcad_addexec(pixmatte pixmatte.c libbu FOLDER Util)
brlcad_addexec(pixmerge pixmerge.c libbu FOLDER Util)
brlcad_addexec(pixmorph pixmorph.c "libdm;libbu;${M_LIBRARY}" FOLDER Util)
add_target_deps(pixmorph dm_plugins)
brlcad_addexec(pixrect pixrect.c "libbu;libicv" FOLDER Util)
brlcad_addexec(pixrot pixrot.c libbu FOLDER Util)
brlcad_addexec(pixsaturate pixsaturate.c libbu FOLDER Util)
brlcad_addexec(pixscale pixscale.c libbu FOLDER Util)
brlcad_addexec(pixshrink pixshrink.c libbu FOLDER Util)
brlcad_addexec(pixstat pixstat.c "libbu;${M_LIBRARY}" FOLDER Util)
brlcad_addexec(pixsubst pixsubst.c libbu FOLDER Util)
brlcad_addexec(pixtile pixtile.c libbu FOLDER Util)
brlcad_addexec(pixuntile pixuntile.c libbu FOLDER Util)
brlcad_addexec(plot3-asc plot3-asc.c "libbv;libbu" FOLDER Util)
brlcad_addexec(plot3-plot3 plot3-plot3.c "libbv;libbu" FOLDER Util)
brlcad_addexec(plot3-ps plot3-ps.c "libbv;libbu" FOLDER Util)
brlcad_addexec(plot3color plot3color.c "libbv;libbu" FOLDER Util)
brlcad_addexec(plot3getframe plot3getframe.c "libbv;libbu" FOLDER Util)
brlcad_addexec(plot3line2 plot3line2.c "libbv;libbu" FOLDER Util)
brlcad_addexec(plot3rot plot3rot.c "libbg;libbv;libbu;${M_LIBRARY}" FOLDER Util)
brlcad_addexec(plot3stat plot3stat.c "libbv;libbu" FOLDER Util)
brlcad_addexec(png-bw png-bw.c "libbn;libbu;${PNG_LIBRARIES}" FOLDER Util)
brlcad_addexec(png-pix png-pix.c "libbn;libbu;${PNG_LIBRARIES}" FOLDER Util)
brlcad_addexec(sun-pix sun-pix.c libbu FOLDER Util)
brlcad_addexec(terrain terrain.c "libbn;libbu;${M_LIBRARY}" FOLDER Util)
#FIXME - Need to look at https://github.com/microsoft/ntttcp to see if ttcp
# can be ported to Windows - ttcp should be cross platform
if(NOT WIN32)
  brlcad_addexec(ttcp ttcp.c "${SOCKET_LIBRARY};${NSL_LIBRARY};${NETWORK_LIBRARY}" FOLDER Util)
else(NOT WIN32)
  cmakefiles(ttcp.c)
endif(NOT WIN32)
brlcad_addexec(wavelet wavelet.c "libdm;libbn;libbu" FOLDER Util)
add_target_deps(wavelet dm_plugins)

install(PROGRAMS morphedit.tcl DESTINATION bin)

brlcad_addexec(roots_example roots_example.c "libbu;librt" NO_INSTALL)
brlcad_adddata(roots_example.c sample_applications)

cmakefiles(bombardier.h morphedit.tcl)
cmakefiles(CMakeLists.txt)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
