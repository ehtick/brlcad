add_executable(lcheck licenses_check.cpp)

set(LOG_FILE "${CMAKE_CURRENT_BINARY_DIR}/regress-licenses.log")
set(STAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/regress-licenses.done")
set(ALL_FILES_LIST "${BRLCAD_BINARY_DIR}/cmakefiles.cmake")

add_custom_command(
  OUTPUT "${STAMP_FILE}"
  COMMAND "${CMAKE_COMMAND}"
           -DLCHECK_EXEC="$<TARGET_FILE:lcheck>" -DBRLCAD_SOURCE_DIR="${BRLCAD_SOURCE_DIR}"
	   -DL_FILE="${LOG_FILE}" -DS_FILE="${STAMP_FILE}" -DF_LIST="${ALL_FILES_LIST}"
	   -DW_DIR="${CMAKE_CURRENT_BINARY_DIR}"
           -P "${CMAKE_CURRENT_SOURCE_DIR}/regress-licenses.cmake"
  )
add_custom_target(regress-licenses DEPENDS ${STAMP_FILE})
set_target_properties(regress-licenses PROPERTIES FOLDER "BRL-CAD Regression Tests")
add_dependencies(regress regress-licenses)
add_dependencies(check regress-licenses)

add_test(NAME regress-licenses
  COMMAND "${CMAKE_COMMAND}"
           -DLCHECK_EXEC="$<TARGET_FILE:lcheck>" -DBRLCAD_SOURCE_DIR="${BRLCAD_SOURCE_DIR}"
	   -DL_FILE="${LOG_FILE}" -DS_FILE="${STAMP_FILE}" -DF_LIST="${ALL_FILES_LIST}"
	   -DW_DIR="${CMAKE_CURRENT_BINARY_DIR}"
           -P "${CMAKE_CURRENT_SOURCE_DIR}/regress-licenses.cmake"
  )
set_tests_properties(regress-licenses PROPERTIES LABELS "Regression")

DISTCLEAN(
  ${STAMP_FILE}
  ${LOG_FILE}
  )

set_target_properties(regress-licenses PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD 1)

CMAKEFILES(
  CMakeLists.txt
  licenses_check.cpp
  regress-licenses.cmake
  )
# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8