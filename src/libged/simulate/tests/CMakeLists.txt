if(BRLCAD_ENABLE_BULLET)
  # There is a peculiarity about using Bullet libraries - by design, they are
  # intended to be used ONLY as static libraries and are not reliably viable
  # for use in a shared configuration:
  #
  # https://stackoverflow.com/a/37019528
  #
  # Additionally, when compiling Bullet to produce static libs on Ubuntu,
  # it proved necessary to add -DCMAKE_POSITION_INDEPENDENT_CODE=ON to
  # avoid linking errors.
  #
  # The need for static libs complicates searching for system installs, since
  # Linux package managers do provide shared versions (at least on Ubuntu,
  # there doesn't even seem to be an option for static libraries provided by
  # the OS?)
  #
  # To avoid finding versions that won't work for our build, we use a variation
  # on this trick:  https://www.scivision.dev/cmake-force-link-static-libs/
  set(CFLS_TMP ${CMAKE_FIND_LIBRARY_SUFFIXES})
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
  find_package(Bullet REQUIRED)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${CFLS_TMP})

  set(SIM_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/simulate ${BULLET_INCLUDE_DIR})

  # If bullet was built with USE_DOUBLE_PRECISION, we need this (see
  # https://pybullet.org/Bullet/phpBB3/viewtopic.php?t=9670 for more discussion.)
  # Hopefully we can find a way to detect this correctly at some point...
  add_definitions(-DBT_USE_DOUBLE_PRECISION=1)

  brlcad_addexec(simulate_test "simulate_test.cpp" "libged;${BULLET_LIBRARIES}" TEST)
  target_include_directories(simulate_test BEFORE PRIVATE ${SIM_INCLUDE_DIRS})
  brlcad_add_test(NAME simulate_test COMMAND simulate_test)
endif(BRLCAD_ENABLE_BULLET)

cmakefiles(simulate_test.cpp)
cmakefiles(CMakeLists.txt)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
