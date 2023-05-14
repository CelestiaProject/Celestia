# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.
#
# Initial version by Andrew Tribick

function(process_file src dest)
  get_filename_component(src_name "${src}" NAME)
  message("Copying ${src_name} to ${dest}")
  file(COPY_FILE "${src}" "${dest}/${src_name}")
endfunction()

if(NOT DEPS_DEST_DIR)
  get_filename_component(DEPS_DEST_DIR "${DEPS_TARGET_FILE}" DIRECTORY)
endif()

file(GET_RUNTIME_DEPENDENCIES
  RESOLVED_DEPENDENCIES_VAR resolved_deps
  CONFLICTING_DEPENDENCIES_PREFIX conflicts
  PRE_EXCLUDE_REGEXES "^api-ms" "^ext-ms-"
  POST_EXCLUDE_REGEXES ".*system32/.*\\.dll$"
  EXECUTABLES "${DEPS_TARGET_FILE}"
  DIRECTORIES "${DEPS_EXTRA_DIRS}"
)

if(DEPS_COPY_TARGET)
  process_file("${DEPS_TARGET_FILE}" "${DEPS_DEST_DIR}")
endif()

foreach(src_path IN LISTS resolved_deps)
  get_filename_component(dest_name "${src_path}" NAME)
  process_file("${src_path}" "${DEPS_DEST_DIR}")
endforeach()

foreach(conflict_name IN LISTS conflicts_FILENAMES)
  # In case of conflicts, use the first resolved path
  list(GET conflicts_${conflict_name} 0 src_path)
  process_file("${src_path}" "${DEPS_DEST_DIR}")
endforeach()
