set(NUCLEOOS_MAIN_DIR "\${CMAKE_CURRENT_LIST_DIR}/../../../nucleoos/firmware/main")

if(NOT EXISTS "\${NUCLEOOS_MAIN_DIR}/main.c")
    message(FATAL_ERROR "NucleoOS main.c is missing; initialize the nucleoos submodule.")
endif()

# Compile NucleoOS's actual boot source, not a second test app_main.
# ROZA adds one native, read-only partition inspector to the same app registry.
idf_component_register(
    SRCS "\${NUCLEOOS_MAIN_DIR}/main.c"
    INCLUDE_DIRS "\${NUCLEOOS_MAIN_DIR}"
    REQUIRES
        nucleo_sdcontent
        nucleo_storage
        nucleo_guest
        nucleo_registry
        nucleo_eventbus
        nucleo_ui
        nucleo_setup
        nucleo_httpd
        nucleo_recorder
        nucleo_discovery
        nvs_flash
        nucleo_app
        nucleo_anima
        nucleo_power
        nucleo_auth
        nucleo_usbmsc
        nucleo_usbnet
        nucleo_evilportal
        nucleo_arb
        nucleo_voice
        nucleo_tts
        nucleo_kbd
        nucleo_codec
        nucleo_imu
        nucleo_ble
        nucleo_ducky
        nucleo_weather
        launcher_platform
        roza_launcher_partitions
)
