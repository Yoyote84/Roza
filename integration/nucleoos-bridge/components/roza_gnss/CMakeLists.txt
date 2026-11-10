set(NUCLEO_APP_SOURCE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../../../../nucleoos/firmware/components/nucleo_app")

if(NOT EXISTS "${NUCLEO_APP_SOURCE_DIR}/app_gfx.h")
    message(FATAL_ERROR "Pinned NucleoOS app framework headers are missing.")
endif()

idf_component_register(
    SRCS "roza_gnss.cpp" "roza_gnss_nmea.cpp"
    INCLUDE_DIRS "include"
    REQUIRES driver nucleo_app nucleo_setup
)

# roza_gnss.cpp draws through NucleoOS's shared app display API.
target_include_directories(${COMPONENT_LIB} PRIVATE "${NUCLEO_APP_SOURCE_DIR}")
