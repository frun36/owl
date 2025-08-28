set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(PARTITION_TABLE_FILENAME partitions.csv)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(owl)

spiffs_create_partition_image(spiffs data FLASH_IN_PROJECT)
