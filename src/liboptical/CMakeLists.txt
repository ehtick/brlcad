set(
  LIBOPTICAL_SOURCES
  init.c
  material.c
  photonmap.c
  refract.c
  sh_air.c
  sh_billboard.c
  sh_brdf.c
  sh_camo.c
  sh_cloud.c
  sh_cook.c
  sh_fbm.c
  sh_fire.c
  sh_flat.c
  sh_gauss.c
  sh_grass.c
  sh_light.c
  sh_noise.c
  sh_null.c
  sh_plastic.c
  sh_points.c
  sh_prj.c
  sh_rtrans.c
  sh_scloud.c
  sh_spm.c
  sh_stack.c
  sh_stxt.c
  sh_text.c
  sh_toon.c
  sh_toyota.c
  sh_treetherm.c
  sh_wood.c
  sh_xxx.c
  shade.c
  turb.c
  vers.c
  wray.c
)

if(BRLCAD_ENABLE_TCL)
  set(TCL_ENABLE_TK OFF)
  find_package_tcl(REQUIRED)
  set(LIBOPTICAL_SOURCES ${LIBOPTICAL_SOURCES} sh_tcl.c)
  set(LOCAL_OPTICAL_INCLUDE_DIRS ${LOCAL_OPTICAL_INCLUDE_DIRS} ${TCL_INCLUDE_PATH})
endif(BRLCAD_ENABLE_TCL)

if(BRLCAD_ENABLE_OSL)
  ##########################################
  # Build OSL-renderer Library
  ##########################################

  # Find dependencies (TODO: build this altogether)
  include(FindOSL)

  # NOTE(boulos): Boost can only detect that you've disabled RTTI for
  # gcc >= 4.3. My poor mac doesn't have that.
  add_definitions("-fno-rtti -DBOOST_NO_RTTI -DBOOST_NO_TYPEID -DOSL_ENABLED")

  message("Oslexec: ${OSLEXEC_LIBRARY}")
  message("Oslcomp: ${OSLCOMP_LIBRARY}")
  message("Oslquery: ${OSLQUERY_LIBRARY}")

  set(OSLRT_SOURCES liboslrend.cpp render_svc.cpp)

  #set (OSLRT_LIBS ${OSLEXEC_LIBRARY} ${OSLCOMP_LIBRARY} ${OSLQUERY_LIBRARY} ${OPENIMAGEIO_LIBRARY} ${Boost_LIBRARIES})
  message("OSLRT_SOURCES: ${OSLRT_SOURCES}")
  #find_package(Boost)

  # Try to find BOOST
  set(Boost_COMPONENTS filesystem regex system thread)
  find_package(Boost 1.46 REQUIRED COMPONENTS ${Boost_COMPONENTS})
  # If it was not found, check if BOOST_ROOT was set
  if(NOT Boost_FILESYSTEM_LIBRARY)
    message("Checking if BOOST ROOT was FOUND")
  endif()

  message("Boost filesystem: ${Boost_FILESYSTEM_LIBRARY}")
  message("Boost regex: ${Boost_REGEX_LIBRARY}")
  message("Boost system: ${Boost_SYSTEM_LIBRARY}")
  message("Boost thread: ${Boost_THREAD_LIBRARY}")

  set(
    OSLRT_LIBS
    ${OSLEXEC_LIBRARY}
    ${OSLCOMP_LIBRARY}
    ${OSLQUERY_LIBRARY}
    ${OPENIMAGEIO_LIBRARY}
    ${Boost_LIBRARIES}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_REGEX_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
  )

  set(
    OSL_LOCAL_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OSL_INCLUDES}
    ${OPENIMAGEIO_INCLUDES}
  )

  brlcad_addlib(liboslrend "${OSLRT_SOURCES}" "${OSLRT_LIBS}" "" "${OSL_LOCAL_INCLUDE_DIRS}")

  ##########################################
  # Build OSL Shader
  ##########################################

  # Add the osl shader to the list of shaders
  set(LIBOPTICAL_SOURCES ${LIBOPTICAL_SOURCES} sh_osl.cpp)
endif(BRLCAD_ENABLE_OSL)

# Note - liboptical_deps is defined by ${BRLCAD_SOURCE_DIR}/src/source_dirs.cmake
set(olibs "${liboptical_deps}")
if(TARGET liboslrend)
  set(olibs "${olibs};liboslrend")
endif(TARGET liboslrend)
if(BRLCAD_ENABLE_TCL)
  set(olibs "${olibs};${TCL_LIBRARY}")
endif(BRLCAD_ENABLE_TCL)

brlcad_addlib(liboptical "${LIBOPTICAL_SOURCES}" "${olibs}" "" "${LOCAL_OPTICAL_INCLUDE_DIRS}")

cmakefiles(
  CMakeLists.txt
  liboslrend.cpp
  oslexec_pvt.h
  render_svc.h
  sh_osl.cpp
  sh_tcl.c
  liboslrend.h
  constantpool.h
  render_svc.cpp
)

set_target_properties(liboptical PROPERTIES VERSION 20.0.1 SOVERSION 20)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
