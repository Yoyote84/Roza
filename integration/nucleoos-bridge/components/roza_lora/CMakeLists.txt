set(NUCLEO_APP_SOURCE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../../../../nucleoos/firmware/components/nucleo_app")

if(NOT EXISTS "${NUCLEO_APP_SOURCE_DIR}/app_gfx.h")
    message(FATAL_ERROR "Pinned NucleoOS app framework headers are missing.")
endif()

idf_component_register(
    SRCS "roza_lora.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        driver
        esp_driver_gpio
        esp_driver_i2c
        esp_driver_spi
        nucleo_app
        nucleo_kbd
        jgromes__radiolib
)

target_include_directories(${COMPONENT_LIB} PRIVATE "${NUCLEO_APP_SOURCE_DIR}")
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)
