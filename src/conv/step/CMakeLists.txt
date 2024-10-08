if(BRLCAD_ENABLE_STEP)
  brlcad_find_package(STEPCODE REQUIRED)
endif(BRLCAD_ENABLE_STEP)

# Because the exp2cxx outputs are used by both step-g and g-step,
# the logic both commands need is the same and is defined up front.

# Quiet MSVC warnings for all step converters due to SDAI sources and
# STEPcode headers used pervasively.  We intentionally do not quell
# all warnings since there are plenty of valid detectable issues also
# getting reported.
#
# FIXME: make STEPcode sources compile free of warnings
check_cxx_flag("wd4065") # switch contains default but no case labels
check_cxx_flag("wd4251") # exporting STL objects
check_cxx_flag("wd4800") # implicit conversions to bool

# Also quell a GCC warning for similar reasons.
check_cxx_flag("Wno-ignored-qualifiers")

find_package(STEPCODE)

macro(GENERATE_SCHEMA_INPUTS SCHEMA_FILE TARGET_SUFFIX)
  if(BRLCAD_ENABLE_STEP)
    # This Schema determines which version of STEP the command will support
    # TODO - see if there is some way to automatically support multiple versions
    # read the schema name from a line like 'SCHEMA AUTOMOTIVE_DESIGN;'
    file(STRINGS ${SCHEMA_FILE} SCHEMA_STATEMENT LIMIT_COUNT 1 REGEX "SCHEMA .*")
    string(REGEX REPLACE "^SCHEMA \(.*\)\;$" "\\1" SCHEMA_N ${SCHEMA_STATEMENT})
    string(TOUPPER ${SCHEMA_N} SCHEMA_NAME) #exp2cxx always uses upper case for file names

    # Because SCHEMA_OUT_DIR is the working directory for exp2cxx, it must be
    # created at configure time and be present when exp2cxx is run
    set(SCHEMA_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SCHEMA_NAME}_${TARGET_SUFFIX})
    set(${TARGET_SUFFIX}_SCHEMA_OUT_DIR ${SCHEMA_OUT_DIR})
    file(MAKE_DIRECTORY ${SCHEMA_OUT_DIR})
    distclean(${SCHEMA_OUT_DIR})

    # These files are the standard exp2cxx outputs that need to be compiled.
    set(
      express_srcs
      ${SCHEMA_OUT_DIR}/Sdai${SCHEMA_NAME}.cc
      ${SCHEMA_OUT_DIR}/Sdai${SCHEMA_NAME}.init.cc
      ${SCHEMA_OUT_DIR}/Sdai${SCHEMA_NAME}_unity_entities.cc
      ${SCHEMA_OUT_DIR}/Sdai${SCHEMA_NAME}_unity_types.cc
      ${SCHEMA_OUT_DIR}/SdaiAll.cc
      ${SCHEMA_OUT_DIR}/compstructs.cc
      ${SCHEMA_OUT_DIR}/schema.cc
    )

    # For the subdirectory add_custom_command definitions, need the generated
    # headers as well.
    set(
      EXPRESS_OUTPUT
      ${express_srcs}
      ${SCHEMA_OUT_DIR}/Sdai${SCHEMA_NAME}.h
      ${SCHEMA_OUT_DIR}/Sdai${SCHEMA_NAME}Names.h
      ${SCHEMA_OUT_DIR}/Sdai${SCHEMA_NAME}_unity_entities.h
      ${SCHEMA_OUT_DIR}/Sdai${SCHEMA_NAME}_unity_types.h
      ${SCHEMA_OUT_DIR}/Sdaiclasses.h
      ${SCHEMA_OUT_DIR}/schema.h
    )

    add_custom_command(
      OUTPUT ${EXPRESS_OUTPUT} ${SCHEMA_OUT_DIR}/step_express_${SCHEMA_NAME}.done
      COMMAND ${EXP2CXX_EXECUTABLE}
      ARGS ${SCHEMA_FILE} > ${SCHEMA_OUT_DIR}/step_express_${SCHEMA_NAME}.log 2>&1
      COMMAND ${CMAKE_COMMAND} -E touch ${SCHEMA_OUT_DIR}/step_express_${SCHEMA_NAME}.done
      DEPENDS ${SCHEMA_FILE} ${EXP2CXX_EXECUTABLE_TARGET}
      WORKING_DIRECTORY ${SCHEMA_OUT_DIR}
      COMMENT "Generating C++ code to express ${SCHEMA_NAME}..."
      VERBATIM
    )

    add_custom_target(
      step-express-${SCHEMA_NAME}-${TARGET_SUFFIX}
      DEPENDS ${SCHEMA_OUT_DIR}/step_express_${SCHEMA_NAME}.done
    )
    set_target_properties(step-express-${SCHEMA_NAME}-${TARGET_SUFFIX} PROPERTIES FOLDER "Compilation Utilities")

    set(
      clean_files
      ${EXPRESS_OUTPUT}
      ${SCHEMA_OUT_DIR}/step_express_${SCHEMA_NAME}.done
      ${SCHEMA_OUT_DIR}/step_express_${SCHEMA_NAME}.log
    )
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${clean_files}")
  endif(BRLCAD_ENABLE_STEP)
endmacro(GENERATE_SCHEMA_INPUTS SCHEMA_FILE)

# AP203
add_subdirectory(step-g)
add_subdirectory(g-step)

# Support for AP203e2, AP214 and AP242 is under development and not
# yet functional.  Since the large C++ files generated for these
# tools take a hideously long time to build on Windows, and are
# noticeable even on other platforms, wrap them with conditionals
# until they do something useful.  It may be that work in STEPcode
# to generate multiple files instead of single huge files will
# help alleviate the issue - something to check into.
add_subdirectory(g-ap203e2)
add_subdirectory(ap203e2-g)
add_subdirectory(ap214-g)
add_subdirectory(g-ap214)
add_subdirectory(ap242-g)
add_subdirectory(g-ap242)

# FIXME: the IFC converter is incomplete
add_subdirectory(ifc-g)

add_subdirectory(util)

cmakefiles(
  AP_Common.h
  BRLCADWrapper.h
  CMakeLists.txt
  STEPWrapper.h
  ap203_elements.txt
  ap203e2_elements.txt
  ap214e3_elements.txt
  ap_schema.h
)

distclean(${SCHEMA_OUT_DIR}/make_schema)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
