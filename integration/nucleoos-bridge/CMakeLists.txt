cmake_minimum_required(VERSION 3.16)

# Build the real NucleoOS firmware components together with Launcher 3.0.0's
# real ESP-IDF platform component. NucleoOS remains the sole app_main owner.
set(NUCLEOOS_FIRMWARE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../nucleoos/firmware")

if(NOT EXISTS "${NUCLEOOS_FIRMWARE_DIR}/main/main.c")
    message(FATAL_ERROR "NucleoOS submodule is missing. Run: git submodule update --init --recursive")
endif()

# Keep NucleoOS's version metadata and dependency resolution semantics.
include("${NUCLEOOS_FIRMWARE_DIR}/version/version.cmake")

set(EXTRA_COMPONENT_DIRS
    "${NUCLEOOS_FIRMWARE_DIR}/components"
    "${CMAKE_CURRENT_LIST_DIR}/../arduino-idf-smoke/components/launcher_platform"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(roza_nucleoos_launcher_bridge)
