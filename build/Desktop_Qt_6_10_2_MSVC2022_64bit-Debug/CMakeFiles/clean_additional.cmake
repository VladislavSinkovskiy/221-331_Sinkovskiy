# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\LR_1_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\LR_1_autogen.dir\\ParseCache.txt"
  "LR_1_autogen"
  )
endif()
