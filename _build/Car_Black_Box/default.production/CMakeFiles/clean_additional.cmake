# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "/home/krubro/MPLABProjects/Car_Black_Box/out/Car_Black_Box/production/default-production.cmf"
  "/home/krubro/MPLABProjects/Car_Black_Box/out/Car_Black_Box/production/default-production.hex"
  "/home/krubro/MPLABProjects/Car_Black_Box/out/Car_Black_Box/production/default-production.hxl"
  "/home/krubro/MPLABProjects/Car_Black_Box/out/Car_Black_Box/production/default-production.mum"
  "/home/krubro/MPLABProjects/Car_Black_Box/out/Car_Black_Box/production/default-production.o"
  "/home/krubro/MPLABProjects/Car_Black_Box/out/Car_Black_Box/production/default-production.sdb"
  "/home/krubro/MPLABProjects/Car_Black_Box/out/Car_Black_Box/production/default-production.sym"
  )
endif()
