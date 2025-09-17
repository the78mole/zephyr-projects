# Copyright (c) 2025 BTHome v2 for Zephyr
# SPDX-License-Identifier: Apache-2.0

# Check if we are building within Zephyr context
if(CONFIG_BTHOME)
    # Zephyr module mode
    zephyr_library()
    zephyr_library_sources(src/bthome.c)
    zephyr_library_include_directories(include)
else()
    # Standalone library mode
    add_library(bthome src/bthome.c)
    target_include_directories(bthome PUBLIC include)
    
    # Make sure we have Zephyr context
    if(TARGET zephyr_interface)
        target_link_libraries(bthome PUBLIC zephyr_interface)
    endif()
endif()