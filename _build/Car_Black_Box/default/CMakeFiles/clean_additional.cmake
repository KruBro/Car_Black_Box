# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "/home/krubro/MPLABProjects/Car_Black_Box/out/Car_Black_Box/default.cmf"
  "/home/krubro/MPLABProjects/Car_Black_Box/out/Car_Black_Box/default.hex"
  "/home/krubro/MPLABProjects/Car_Black_Box/out/Car_Black_Box/default.hxl"
  "/home/krubro/MPLABProjects/Car_Black_Box/out/Car_Black_Box/default.mum"
  "/home/krubro/MPLABProjects/Car_Black_Box/out/Car_Black_Box/default.o"
  "/home/krubro/MPLABProjects/Car_Black_Box/out/Car_Black_Box/default.sdb"
  "/home/krubro/MPLABProjects/Car_Black_Box/out/Car_Black_Box/default.sym"
  )
endif()
