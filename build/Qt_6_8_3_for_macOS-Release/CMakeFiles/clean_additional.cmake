# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles/backup_1_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/backup_1_autogen.dir/ParseCache.txt"
  "backup_1_autogen"
  )
endif()
