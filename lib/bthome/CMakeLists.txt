# Copyright (c) 2025 BTHome v2 for Zephyr
# SPDX-License-Identifier: Apache-2.0

# This is a Zephyr module CMakeLists.txt file
# It is included when BTHOME is enabled in Kconfig

if(CONFIG_BTHOME)
    # Create BTHome library as a Zephyr library
    zephyr_library()
    
    # Add source files
    zephyr_library_sources(src/bthome.c)
    
    # Add include directories
    zephyr_library_include_directories(include)
    
    # Add module-specific includes
    zephyr_include_directories(include)
    
    # Add compile definitions based on Kconfig
    zephyr_compile_definitions_ifdef(CONFIG_BTHOME_ENCRYPTION BTHOME_ENCRYPTION_ENABLED)
    zephyr_compile_definitions_ifdef(CONFIG_BTHOME_AUTO_MAC BTHOME_AUTO_MAC_ENABLED)
endif()