set(NUCLEO_APP_SOURCE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../../../../nucleoos/firmware/components/nucleo_app")
set(DISPLAY_DRIVERS_SOURCE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../../../../launcher/lib/DisplayDrivers/src")

if(NOT EXISTS "${NUCLEO_APP_SOURCE_DIR}/app_gfx.h")
    message(FATAL_ERROR "Pinned NucleoOS app framework headers are missing.")
endif()
if(NOT EXISTS "${DISPLAY_DRIVERS_SOURCE_DIR}/backends/m5gfx_hal.cpp")
    message(FATAL_ERROR "Pinned Launcher DisplayDrivers M5GFX backend is missing.")
endif()

idf_component_register(
    SRCS
        "roza_launcher_input.cpp"
        "roza_launcher_hal.cpp"
        "roza_launcher_diagnostics.cpp"
        "roza_launcher_display_app.cpp"
        "${DISPLAY_DRIVERS_SOURCE_DIR}/backends/m5gfx_hal.cpp"
    INCLUDE_DIRS
        "include"
        "${DISPLAY_DRIVERS_SOURCE_DIR}"
    REQUIRES
        nucleo_app
        nucleo_kbd
        nucleo_power
        nucleo_storage
        m5stack__m5gfx
        arduino-esp32
    WHOLE_ARCHIVE
)

# Compile the real pinned Launcher backend against NucleoOS's live graphics target.
# USE_CANVAS is explicitly disabled by the ROZA patch: NucleoOS owns the only canvas.
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    USE_M5GFX=1
    ROZA_NUCLEO_TARGET=1
    DISPLAY_DEVICE_M5STACK_CARDPUTER=1
)
target_include_directories(${COMPONENT_LIB} PRIVATE "${NUCLEO_APP_SOURCE_DIR}")
