# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bthome_counter)

# Add BTHome library
add_subdirectory(../../lib/bthome bthome)

# Link BTHome library and add include directory
target_link_libraries(app PRIVATE bthome)
target_include_directories(app PRIVATE ../../lib/bthome/include)

target_sources(app PRIVATE src/main.c)