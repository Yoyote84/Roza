cmake_minimum_required(VERSION 3.16)

# Resolve the repository root explicitly. The CI build runs from this directory
# inside a container, while NucleoOS is checked out as a sibling at repo root.
get_filename_component(ROZA_REPOSITORY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(NUCLEOOS_FIRMWARE_DIR "${ROZA_REPOSITORY_ROOT}/nucleoos/firmware")

if(NOT EXISTS "${NUCLEOOS_FIRMWARE_DIR}/main/main.c")
    message(FATAL_ERROR
        "NucleoOS source missing. Expected: ${NUCLEOOS_FIRMWARE_DIR}/main/main.c"
    )
endif()

include("${NUCLEOOS_FIRMWARE_DIR}/version/version.cmake")

set(EXTRA_COMPONENT_DIRS
    "${NUCLEOOS_FIRMWARE_DIR}/components"
    "${CMAKE_CURRENT_LIST_DIR}/components/roza_launcher_partitions"
    "${CMAKE_CURRENT_LIST_DIR}/components/roza_launcher_sd"
    "${CMAKE_CURRENT_LIST_DIR}/components/roza_launcher_hal"
    "${CMAKE_CURRENT_LIST_DIR}/components/roza_gnss"
    "${CMAKE_CURRENT_LIST_DIR}/components/roza_lora"
    "${CMAKE_CURRENT_LIST_DIR}/../arduino-idf-smoke/components/launcher_platform"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(roza_nucleoos_launcher_bridge)
