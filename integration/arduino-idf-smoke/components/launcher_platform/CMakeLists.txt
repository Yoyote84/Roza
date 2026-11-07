set(LAUNCHER_PLATFORM_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../launcher/src/idf")

if(NOT EXISTS "${LAUNCHER_PLATFORM_DIR}/launcher_platform.cpp")
    message(FATAL_ERROR
        "Launcher submodule source missing. Run git submodule update --init --recursive."
    )
endif()

idf_component_register(
    SRCS "${LAUNCHER_PLATFORM_DIR}/launcher_platform.cpp"
    INCLUDE_DIRS "${LAUNCHER_PLATFORM_DIR}"
    REQUIRES arduino-esp32 driver esp_timer
)
