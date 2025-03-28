#                   D O C B O O K . C M A K E
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
#
# In principle, DocBook conversion and validation can be accomplished
# with multiple programs.  BRL-CAD's CMake logic uses variables to
# hold the "active" tools for each conversion operation, but will set
# defaults if the user does not manually set them.

# If a user wishes to use their own validation and/or conversion
# tools, they can set the following variables to their executable
# names and create <exec_name>.cmake.in files in misc/CMake.  To work,
# the cmake.in files will need to produce the same validity "stamp"
# files and fatal errors as the default tools upon success or failure.
# For a worked example, see rnv.cmake.in - to test it, install rnv
# from http://sourceforge.net/projects/rnv/ and configure BRL-CAD as
# follows:
#
# cmake .. -DBRLCAD_EXTRADOCS_VALIDATE=ON -DVALIDATE_EXECUTABLE=rnv
#
# Note that rnv must be in the system path for this to work.

if(NOT COMMAND ADD_DOCBOOK)
  find_program(XSLTPROC_EXECUTABLE xsltproc HINTS ${BRLCAD_EXT_NOINSTALL_DIR}/${BIN_DIR})
  mark_as_advanced(XSLTPROC_EXECUTABLE)
  find_program(XMLLINT_EXECUTABLE xmllint HINTS ${BRLCAD_EXT_NOINSTALL_DIR}/${BIN_DIR})
  mark_as_advanced(XMLLINT_EXECUTABLE)

  # Handle default exec and sanity checking for XML validation
  if(BRLCAD_ENABLE_STRICT)
    if(NOT DEFINED VALIDATE_EXECUTABLE)
      set(VALIDATE_EXECUTABLE "xmllint")
    else(NOT DEFINED VALIDATE_EXECUTABLE)
      if(NOT EXISTS "${BRLCAD_SOURCE_DIR}/misc/CMake/${VALIDATE_EXECUTABLE}.cmake.in")
        message(
          FATAL_ERROR
          "Specified ${VALIDATE_EXECUTABLE} for DocBook validation, but \"${BRLCAD_SOURCE_DIR}/misc/CMake/${VALIDATE_EXECUTABLE}.cmake.in\" does not exist.  To use ${VALIDATE_EXECUTABLE} a ${VALIDATE_EXECUTABLE}.cmake.in file must be defined."
        )
      endif(NOT EXISTS "${BRLCAD_SOURCE_DIR}/misc/CMake/${VALIDATE_EXECUTABLE}.cmake.in")
    endif(NOT DEFINED VALIDATE_EXECUTABLE)
  endif(BRLCAD_ENABLE_STRICT)

  # Handle default exec and sanity checking for XSLT
  if(NOT DEFINED XSLT_EXECUTABLE)
    set(XSLT_EXECUTABLE "xsltproc")
  else(NOT DEFINED XSLT_EXECUTABLE)
    if(NOT EXISTS "${BRLCAD_SOURCE_DIR}/misc/CMake/${XSLT_EXECUTABLE}.cmake.in")
      message(
        FATAL_ERROR
        "Specified ${XSLT_EXECUTABLE} for DocBook processing, but \"${BRLCAD_SOURCE_DIR}/misc/CMake/${XSLT_EXECUTABLE}.cmake.in\" does not exist.  To use ${XSLT_EXECUTABLE} a ${XSLT_EXECUTABLE}.cmake.in file must be defined."
      )
    endif(NOT EXISTS "${BRLCAD_SOURCE_DIR}/misc/CMake/${XSLT_EXECUTABLE}.cmake.in")
  endif(NOT DEFINED XSLT_EXECUTABLE)

  # Handle default exec and sanity checking for PDF conversion
  if(NOT DEFINED PDF_CONV_EXECUTABLE)
    set(PDF_CONV_EXECUTABLE "fop")
  else(NOT DEFINED PDF_CONF_EXECUTABLE)
    if(NOT EXISTS "${BRLCAD_SOURCE_DIR}/misc/CMake/${PDF_CONF_EXECUTABLE}.cmake.in")
      message(
        FATAL_ERROR
        "Specified ${PDF_CONF_EXECUTABLE} for DocBook pdf conversion, but \"${BRLCAD_SOURCE_DIR}/misc/CMake/${PDF_CONF_EXECUTABLE}.cmake.in\" does not exist.  To use ${PDF_CONF_EXECUTABLE} a ${PDF_CONF_EXECUTABLE}.cmake.in file must be defined."
      )
    endif(NOT EXISTS "${BRLCAD_SOURCE_DIR}/misc/CMake/${PDF_CONF_EXECUTABLE}.cmake.in")
  endif(NOT DEFINED PDF_CONV_EXECUTABLE)

  # xsltproc is finicky about slashes in names - do some sanity
  # scrubbing of the full root path string in preparation for generating
  # DocBook scripts
  string(REGEX REPLACE "/+" "/" ${CMAKE_BINARY_DIR} "${${CMAKE_BINARY_DIR}}")
  string(REGEX REPLACE "/$" "" ${CMAKE_BINARY_DIR} "${${CMAKE_BINARY_DIR}}")

  set(OUTPUT_FORMATS)
  if(BRLCAD_EXTRADOCS_HTML)
    set(OUTPUT_FORMATS ${OUTPUT_FORMATS} HTML)
  endif(BRLCAD_EXTRADOCS_HTML)
  if(BRLCAD_EXTRADOCS_PHP)
    set(OUTPUT_FORMATS ${OUTPUT_FORMATS} PHP)
  endif(BRLCAD_EXTRADOCS_PHP)
  if(BRLCAD_EXTRADOCS_PPT)
    set(OUTPUT_FORMATS ${OUTPUT_FORMATS} PPT)
  endif(BRLCAD_EXTRADOCS_PPT)
  if(BRLCAD_EXTRADOCS_MAN)
    set(OUTPUT_FORMATS ${OUTPUT_FORMATS} MAN1)
    set(OUTPUT_FORMATS ${OUTPUT_FORMATS} MAN3)
    set(OUTPUT_FORMATS ${OUTPUT_FORMATS} MAN5)
    set(OUTPUT_FORMATS ${OUTPUT_FORMATS} MANN)
  endif(BRLCAD_EXTRADOCS_MAN)
  if(BRLCAD_EXTRADOCS_PDF)
    set(OUTPUT_FORMATS ${OUTPUT_FORMATS} PDF)
  endif(BRLCAD_EXTRADOCS_PDF)

  set(HTML_EXTENSION "html")
  set(PHP_EXTENSION "php")
  set(PPT_EXTENSION "ppt.html")
  set(MAN1_EXTENSION "1")
  set(MAN3_EXTENSION "3")
  set(MAN5_EXTENSION "5")
  set(MANN_EXTENSION "nged")
  set(PDF_EXTENSION "pdf")

  set(HTML_DIR "${DOC_DIR}/html/")
  set(PHP_DIR "${DOC_DIR}/html/")
  set(PPT_DIR "${DOC_DIR}/html/")
  set(MAN1_DIR "${DOC_DIR}/../man/")
  set(MAN3_DIR "${DOC_DIR}/../man/")
  set(MAN5_DIR "${DOC_DIR}/../man/")
  set(MANN_DIR "${DOC_DIR}/../man/")
  set(PDF_DIR "${DOC_DIR}/pdf/")

  # TODO - is there some way generator expressions could be used to
  # improve this?  Maybe pass the build time location of these programs
  # to the scripts as -D arguments?
  set(XMLLINT_EXEC "${XMLLINT_EXECUTABLE}")
  set(XSLTPROC_EXEC "${XSLTPROC_EXECUTABLE}")

  # Convenience target to launch all DocBook builds
  add_custom_target(docbook ALL)
  set_target_properties(docbook PROPERTIES FOLDER "DocBook")
  if(TARGET brlcad_css)
    add_dependencies(docbook brlcad_css)
  endif(TARGET brlcad_css)

  function(ADD_DOCBOOK fmts in_xml_files outdir deps_list)
    cmake_parse_arguments(A "" "" "REQUIRED" ${ARGN})

    if(A_REQUIRED)
      foreach(rdep ${A_REQUIRED})
        if(NOT TARGET ${rdep})
          return()
        endif(NOT TARGET ${rdep})
      endforeach(rdep ${A_REQUIRED})
    endif(A_REQUIRED)

    # If we got the name of a list or an explicit list, translate into
    # the form we need.
    list(GET ${in_xml_files} 0 xml_files)
    if("${xml_files}" MATCHES "NOTFOUND")
      set(xml_files ${in_xml_files})
      get_filename_component(abs_xml_file_path "${xml_files}" ABSOLUTE)
      get_filename_component(dname_root1 "${abs_xml_file_path}" NAME_WE)
      get_filename_component(dname_path1 "${abs_xml_file_path}" PATH)
      get_filename_component(dname_root2 "${dname_path1}" NAME_WE)
      get_filename_component(dname_path2 "${dname_path1}" PATH)
      get_filename_component(dname_root3 "${dname_path2}" NAME_WE)
      set(target_root "${dname_root3}-${dname_root2}-${dname_root1}")
    else("${xml_files}" MATCHES "NOTFOUND")
      get_filename_component(abs_xml_file_path "${xml_files}" ABSOLUTE)
      get_filename_component(abs_path "${abs_xml_file_path}" PATH)
      get_filename_component(dname_root1 "${abs_path}" NAME_WE)
      get_filename_component(dname_path1 "${abs_path}" PATH)
      get_filename_component(dname_root2 "${dname_path1}" NAME_WE)
      get_filename_component(dname_path2 "${dname_path1}" PATH)
      get_filename_component(dname_root3 "${dname_path2}" NAME_WE)
      set(inc_num 0)
      set(target_root "${dname_root3}-${dname_root2}-${dname_root1}")
      while(TARGET docbook-${target_root})
        math(EXPR inc_num "${inc_num} + 1")
        set(target_root "${dname_root3}-${dname_root2}-${dname_root1}-${inc_num}")
      endwhile(TARGET docbook-${target_root})

      # Unpack the list
      set(xml_files ${${in_xml_files}})
    endif("${xml_files}" MATCHES "NOTFOUND")

    if(BRLCAD_EXTRADOCS)
      set(all_outfiles)
      set(errors 0)

      # Each file gets its own script file and custom command, which
      # handle all the outputs to be produced from that file.
      foreach(fname ${xml_files})
        get_filename_component(fname_root "${fname}" NAME_WE)

        # Used in cmake.in scripts
        get_filename_component(filename "${fname}" ABSOLUTE)

        # Find out which outputs we're actually going to produce,
        # between what's currently enabled and what the command says the
        # target *can* produce
        set(CURRENT_OUTPUT_FORMATS)
        foreach(fmt ${fmts})
          list(FIND OUTPUT_FORMATS "${fmt}" IN_LIST)
          if(NOT "${IN_LIST}" STREQUAL "-1")
            set(CURRENT_OUTPUT_FORMATS ${CURRENT_OUTPUT_FORMATS} ${fmt})
          endif(NOT "${IN_LIST}" STREQUAL "-1")
        endforeach(fmt ${fmts})

        # Now that we know our formats, prepare output paths
        set(outputs)
        foreach(fmt ${fmts})
          list(FIND OUTPUT_FORMATS "${fmt}" IN_LIST)
          if(NOT "${IN_LIST}" STREQUAL "-1")
            set(${fmt}_OUTFILE "${CMAKE_BINARY_DIR}/${${fmt}_DIR}${outdir}/${fname_root}.${${fmt}_EXTENSION}")
            set(outputs ${outputs} ${${fmt}_OUTFILE})
            install(FILES "${${fmt}_OUTFILE}" DESTINATION ${${fmt}_DIR}${outdir})
          endif(NOT "${IN_LIST}" STREQUAL "-1")
        endforeach(fmt ${OUTPUT_FORMATS})

        # If we have more outputs than the default, they need to be
        # handled here.
        foreach(fmt ${fmts})
          list(FIND OUTPUT_FORMATS "${fmt}" IN_LIST)
          if(NOT "${IN_LIST}" STREQUAL "-1")
            set(${fmt}_EXTRAS)
            get_property(EXTRA_OUTPUTS SOURCE ${fname} PROPERTY EXTRA_${fmt}_OUTPUTS)
            foreach(extra_out ${EXTRA_OUTPUTS})
              # Pass the file name to the script's extras list, in case
              # the script has to manually place the file in the correct
              # directory...
              set(${fmt}_EXTRAS ${${fmt}_EXTRAS} "${extra_out}")

              set(${fmt}_EXTRA "${CMAKE_BINARY_DIR}/${${fmt}_DIR}${outdir}/${extra_out}")
              set(outputs ${outputs} ${${fmt}_EXTRA})
              install(FILES "${${fmt}_EXTRA}" DESTINATION ${${fmt}_DIR}${outdir})
            endforeach(extra_out ${EXTRA_OUTPUTS})
          endif(NOT "${IN_LIST}" STREQUAL "-1")
        endforeach(fmt ${fmts})

        set(all_outfiles ${all_outfiles} ${outputs})

        # Long as we're outputting, we have a target to produce
        if(NOT "${outputs}" STREQUAL "")
          string(MD5 path_md5 "${CMAKE_CURRENT_SOURCE_DIR}/${fname}")
          configure_file(
            ${BRLCAD_CMAKE_DIR}/docbook.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/dbp_${fname_root}-${path_md5}.cmake
            @ONLY
          )
          distclean("${CMAKE_CURRENT_BINARY_DIR}/dbp_${fname_root}-${path_md5}.cmake")
          add_custom_command(
            OUTPUT ${outputs}
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/dbp_${fname_root}-${path_md5}.cmake
            DEPENDS
              ${fname}
              ${XMLLINT_EXECUTABLE_TARGET}
              ${XSLTPROC_EXECUTABLE_TARGET}
              ${DOCBOOK_RESOURCE_FILES}
              ${deps_list}
          )

          # For now, we'll skip generating per-input-file build targets
          # - that's not normally how the docbook targets are built.

          #add_custom_target(docbook-${fname_root}-${path_md5} DEPENDS ${outputs})
          #set_target_properties(docbook-${fname_root}-${path_md5} PROPERTIES FOLDER "DocBook")
          #if (TARGET brlcad_css)
          #  add_dependencies(docbook-${fname_root}-${path_md5} brlcad_css)
          #endif (TARGET brlcad_css)
        endif(NOT "${outputs}" STREQUAL "")
      endforeach(fname ${xml_files})

      if(errors GREATER "0")
        message(FATAL_ERROR "Halting build due to previous DocBook error(s).")
      endif(errors GREATER "0")

      if(NOT "${all_outfiles}" STREQUAL "")
        if(NOT TARGET docbook-${target_root})
          add_custom_target(docbook-${target_root} ALL DEPENDS ${all_outfiles})
        else(NOT TARGET docbook-${target_root})
          add_dependencies(docbook-${target_root} ${all_outfiles})
        endif(NOT TARGET docbook-${target_root})
        set_target_properties(docbook-${target_root} PROPERTIES FOLDER "DocBook")
        add_dependencies(docbook docbook-${target_root})
      endif(NOT "${all_outfiles}" STREQUAL "")
    endif(BRLCAD_EXTRADOCS)
  endfunction(ADD_DOCBOOK)
endif(NOT COMMAND ADD_DOCBOOK)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
