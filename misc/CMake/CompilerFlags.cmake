#             C O M P I L E R F L A G S . C M A K E
# BRL-CAD
#
# Copyright (c) 2011-2025 United States Government as represented by
# the U.S. Army Research Laboratory.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following
# disclaimer in the documentation and/or other materials provided
# with the distribution.
#
# 3. The name of the author may not be used to endorse or promote
# products derived from this software without specific prior written
# permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
###

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CMakeParseArguments)
include(CMakePushCheckState)

set(CMAKE_BUILD_TYPES DEBUG RELEASE)

# Debugging function to print all current flags
function(PRINT_BUILD_FLAGS)
  message("Current Build Flags (${ARGV0}):\n")
  message("CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
  message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
  message("CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}")
  message("CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
  foreach(BTYPE ${CMAKE_BUILD_TYPES})
    message(" ")
    message("CMAKE_C_FLAGS_${BTYPE}: ${CMAKE_C_FLAGS_${BTYPE}}")
    message("CMAKE_CXX_FLAGS_${BTYPE}: ${CMAKE_CXX_FLAGS_${BTYPE}}")
    message("CMAKE_SHARED_LINKER_FLAGS_${BTYPE}: ${CMAKE_SHARED_LINKER_FLAGS_${BTYPE}}")
    message("CMAKE_EXE_LINKER_FLAGS_${BTYPE}: ${CMAKE_EXE_LINKER_FLAGS_${BTYPE}}")
  endforeach(BTYPE ${CMAKE_BUILD_TYPES})
  message(" ")
endfunction(PRINT_BUILD_FLAGS)

# Clear all currently defined CMake compiler and linker flags.
# NOTE - we don't currently manage MSVC build flags directly,
# so in that case we leave the CMake defaults.
macro(CLEAR_BUILD_FLAGS)
  set(BUILD_FLAGS_TO_CLEAR CMAKE_C_FLAGS CMAKE_CXX_FLAGS CMAKE_SHARED_LINKER_FLAGS CMAKE_EXE_LINKER_FLAGS)
  if(NOT MSVC)
    foreach(bflag ${BUILD_FLAGS_TO_CLEAR})
      set(${bflag} "")
      unset(${bflag} CACHE)
      foreach(BTYPE ${CMAKE_BUILD_TYPES})
        set(${bflag}_${BTYPE} "")
        unset(${bflag}_${BTYPE} CACHE)
      endforeach(BTYPE ${CMAKE_BUILD_TYPES})
    endforeach(bflag ${BUILD_FLAGS_TO_CLEAR})

    set(CMAKE_C_FLAGS "$ENV{CFLAGS}")
    set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "$ENV{LDFLAGS}")
  endif(NOT MSVC)
endmacro(CLEAR_BUILD_FLAGS)

# To reduce verbosity in this file, determine up front which
# build configuration type (if any) we are using and stash
# the variables we want to assign flags to into a common
# variable that will be used for all routines.
macro(ADD_NEW_FLAG FLAG_TYPE NEW_FLAG CONFIG_LIST)
  if(${NEW_FLAG})
    if("${CONFIG_LIST}" STREQUAL "ALL")
      set(CMAKE_${FLAG_TYPE}_FLAGS "${CMAKE_${FLAG_TYPE}_FLAGS} ${${NEW_FLAG}}")
    elseif("${CONFIG_LIST}" STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE)
      set(CMAKE_${FLAG_TYPE}_FLAGS "${CMAKE_${FLAG_TYPE}_FLAGS} ${${NEW_FLAG}}")
    else("${CONFIG_LIST}" STREQUAL "ALL")
      foreach(CFG_TYPE ${CONFIG_LIST})
        set(VALID_CONFIG 1)
        if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "${CFG_TYPE}")
          set(VALID_CONFIG "-1")
        endif(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "${CFG_TYPE}")
        if(NOT "${VALID_CONFIG}" STREQUAL "-1")
          string(TOUPPER "${CFG_TYPE}" CFG_TYPE)
          if(CMAKE_${FLAG_TYPE}_FLAGS_${CFG_TYPE})
            set(CMAKE_${FLAG_TYPE}_FLAGS_${CFG_TYPE} "${CMAKE_${FLAG_TYPE}_FLAGS_${CFG_TYPE}} ${${NEW_FLAG}}")
          else(CMAKE_${FLAG_TYPE}_FLAGS_${CFG_TYPE})
            set(CMAKE_${FLAG_TYPE}_FLAGS_${CFG_TYPE} "${${NEW_FLAG}}")
          endif(CMAKE_${FLAG_TYPE}_FLAGS_${CFG_TYPE})
        endif(NOT "${VALID_CONFIG}" STREQUAL "-1")
      endforeach(CFG_TYPE ${CONFIG_LIST})
    endif("${CONFIG_LIST}" STREQUAL "ALL")
  endif(${NEW_FLAG})
endmacro(ADD_NEW_FLAG)

# This macro tests for a specified C or C++ compiler flag, setting the
# result in the specified variable.
if(NOT COMMAND CHECK_COMPILER_FLAG)
  macro(CHECK_COMPILER_FLAG FLAG_LANG NEW_FLAG RESULTVAR)
    if("${FLAG_LANG}" STREQUAL "C")
      check_c_compiler_flag(${NEW_FLAG} ${RESULTVAR})
    endif("${FLAG_LANG}" STREQUAL "C")
    if("${FLAG_LANG}" STREQUAL "CXX")
      check_cxx_compiler_flag(${NEW_FLAG} ${RESULTVAR})
    endif("${FLAG_LANG}" STREQUAL "CXX")
  endmacro(CHECK_COMPILER_FLAG LANG NEW_FLAG RESULTVAR)
endif(NOT COMMAND CHECK_COMPILER_FLAG)

# Synopsis:  CHECK_FLAG(LANG flag [BUILD_TYPES type1 type2 ...] [GROUPS group1 group2 ...] [VARS var1 var2 ...] )
#
# CHECK_FLAG is BRL-CAD's core macro for C/C++ flag testing.
# The first value is the language to test (C or C++ currently).  The
# second entry is the flag (without preliminary dash).
#
# If the first two mandatory options are the only ones provided, a
# successful test of the flag will result in its being assigned to
# *all* compilations using the appropriate global C/C++ CMake
# variable.  If optional parameters are included, they tell the macro
# what to do with the test results instead of doing the default global
# assignment.  Options include assigning the flag to one or more of
# the variable lists associated with build types (e.g. Debug or
# Release), appending the variable to a string that contains a group
# of variables, or assigning the flag to a variable if that variable
# does not already hold a value.  The assignments are not mutually
# exclusive - any or all of them may be used in a given command.
#
# For example, to test a flag and add it to the C Debug configuration
# flags:
#
# CHECK_FLAG(C ggdb3 BUILD_TYPES Debug)
#
# To assign a C flag to a unique variable:
#
# CHECK_FLAG(C c99 VARS C99_FLAG)
#
# To do all assignments at once, for multiple configs and vars:
#
# CHECK_FLAG(C ggdb3
#                   BUILD_TYPES Debug Release
#                   GROUPS DEBUG_FLAGS
#                   VARS DEBUG1 DEBUG2)
macro(CHECK_FLAG)
  # Set up some variables and names
  set(FLAG_LANG ${ARGV0})
  set(flag ${ARGV1})
  string(TOUPPER ${flag} UPPER_FLAG)
  string(REGEX REPLACE "[^a-zA-Z0-9]" "_" UPPER_FLAG ${UPPER_FLAG})
  set(NEW_FLAG "-${flag}")

  # Start processing arguments
  if(${ARGC} LESS 3)
    # Handle default (global) case
    check_compiler_flag(${FLAG_LANG} ${NEW_FLAG} ${UPPER_FLAG}_${FLAG_LANG}_FLAG_FOUND)
    if(${UPPER_FLAG}_${FLAG_LANG}_FLAG_FOUND)
      add_new_flag(${FLAG_LANG} NEW_FLAG ALL)
    endif(${UPPER_FLAG}_${FLAG_LANG}_FLAG_FOUND)
  else(${ARGC} LESS 3)
    # Parse extra arguments
    cmake_parse_arguments(FLAG "" "" "BUILD_TYPES;GROUPS;VARS" ${ARGN})

    # Iterate over listed Build types and append the flag to them if successfully tested.
    foreach(build_type ${FLAG_BUILD_TYPES})
      check_compiler_flag(${FLAG_LANG} "${NEW_FLAG}" ${UPPER_FLAG}_${FLAG_LANG}_FLAG_FOUND)
      if(${UPPER_FLAG}_${FLAG_LANG}_FLAG_FOUND)
        add_new_flag(${FLAG_LANG} NEW_FLAG "${build_type}")
      endif(${UPPER_FLAG}_${FLAG_LANG}_FLAG_FOUND)
    endforeach(build_type ${FLAG_BUILD_TYPES})

    # Append flag to a group of flags (this apparently needs to be
    # a string build, not a CMake list build.  Do this for all supplied
    # group variables.
    foreach(flag_group ${FLAG_GROUPS})
      check_compiler_flag(${FLAG_LANG} "${NEW_FLAG}" ${UPPER_FLAG}_${FLAG_LANG}_FLAG_FOUND)
      if(${UPPER_FLAG}_${FLAG_LANG}_FLAG_FOUND)
        if(${flag_group})
          set(${flag_group} "${${flag_group}} ${NEW_FLAG}")
        else(${flag_group})
          set(${flag_group} "${NEW_FLAG}")
        endif(${flag_group})
      endif(${UPPER_FLAG}_${FLAG_LANG}_FLAG_FOUND)
    endforeach(flag_group ${FLAG_GROUPS})

    # If a variable does not have a value, check the flag and if valid assign
    # the flag as the variable's value.  Do this for all supplied variables.
    foreach(flag_var ${FLAG_VARS})
      if(NOT ${flag_var})
        check_compiler_flag(${FLAG_LANG} "${NEW_FLAG}" ${UPPER_FLAG}_${FLAG_LANG}_FLAG_FOUND)
        if(${UPPER_FLAG}_${FLAG_LANG}_FLAG_FOUND AND NOT "${${flag_var}}")
          set(${flag_var} "${NEW_FLAG}")
        endif(${UPPER_FLAG}_${FLAG_LANG}_FLAG_FOUND AND NOT "${${flag_var}}")
      endif(NOT ${flag_var})
    endforeach(flag_var ${FLAG_VARS})
  endif(${ARGC} LESS 3)
endmacro(CHECK_FLAG)

# This macro checks whether a specified C flag is available.  See
# CHECK_FLAG() for arguments.
macro(CHECK_C_FLAG)
  check_flag(C ${ARGN})
endmacro(CHECK_C_FLAG)

# This macro checks whether a specified C++ flag is available.  See
# CHECK_FLAG() for arguments.
macro(CHECK_CXX_FLAG)
  check_flag(CXX ${ARGN})
endmacro(CHECK_CXX_FLAG)

# Disable any compilation warning flags currently set
macro(DISABLE_WARNINGS)
  # borland-style
  if(NOT NOWARN_CFLAG)
    check_c_flag("w-" VARS NOWARN_CFLAG)
  endif(NOT NOWARN_CFLAG)
  if(NOT NOWARN_CXXFLAG)
    check_cxx_flag("w-" VARS NOWARN_CXXFLAG)
  endif(NOT NOWARN_CXXFLAG)

  # msvc-style (must test before gcc's -w test)
  if(NOT NOWARN_CFLAG)
    check_c_flag("W0" VARS NOWARN_CFLAG)

    if(NOWARN_CFLAG)
      # replace msvc-style warning level C flags, disable with W0
      string(REGEX REPLACE "[/-][wW][1-4]" "/W0" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
      foreach(BTYPE ${CMAKE_BUILD_TYPES})
        string(REGEX REPLACE "[/-][wW][1-4]" "/W0" CMAKE_C_FLAGS_${BTYPE} "${CMAKE_C_FLAGS_${BTYPE}}")
      endforeach(BTYPE ${CMAKE_BUILD_TYPES})
    endif(NOWARN_CFLAG)
  endif(NOT NOWARN_CFLAG)

  if(NOT NOWARN_CXXFLAG)
    check_cxx_flag("W0" VARS NOWARN_CXXFLAG)

    if(NOWARN_CXXFLAG)
      # replace msvc-style warning level C++ flags, disable with W0
      string(REGEX REPLACE "[/-][wW][1-4]" "/W0" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
      foreach(BTYPE ${CMAKE_BUILD_TYPES})
        string(REGEX REPLACE "[/-][wW][1-4]" "/W0" CMAKE_CXX_FLAGS_${BTYPE} "${CMAKE_CXX_FLAGS_${BTYPE}}")
      endforeach(BTYPE ${CMAKE_BUILD_TYPES})
    endif(NOWARN_CXXFLAG)
  endif(NOT NOWARN_CXXFLAG)

  # gcc/llvm-style
  if(NOT NOWARN_CFLAG)
    check_c_flag("w" VARS NOWARN_CFLAG)
  endif(NOT NOWARN_CFLAG)
  if(NOT NOWARN_CXXFLAG)
    check_cxx_flag("w" VARS NOWARN_CXXFLAG)
  endif(NOT NOWARN_CXXFLAG)

  # turn off warnings in the current scope
  if(NOWARN_CFLAG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${NOWARN_CFLAG}")
  endif(NOWARN_CFLAG)
  if(NOWARN_CXXFLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${NOWARN_CXXFLAG}")
  endif(NOWARN_CXXFLAG)
endmacro(DISABLE_WARNINGS)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
