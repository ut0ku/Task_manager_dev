# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/untitled19_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/untitled19_autogen.dir/ParseCache.txt"
  "untitled19_autogen"
  )
endif()
