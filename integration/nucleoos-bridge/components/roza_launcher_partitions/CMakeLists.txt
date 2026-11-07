set(NUCLEO_APP_SOURCE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../../../../nucleoos/firmware/components/nucleo_app")

if(NOT EXISTS "${NUCLEO_APP_SOURCE_DIR}/app_gfx.h")
    message(FATAL_ERROR "Pinned NucleoOS app framework headers are missing.")
endif()

idf_component_register(
    SRCS "roza_launcher_partitions.cpp"
    INCLUDE_DIRS "."
    REQUIRES nucleo_app m5stack__m5gfx spi_flash
)

target_include_directories(${COMPONENT_LIB} PRIVATE "${NUCLEO_APP_SOURCE_DIR}")
