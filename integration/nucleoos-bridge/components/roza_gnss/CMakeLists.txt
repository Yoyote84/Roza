idf_component_register(
    SRCS "roza_gnss.cpp" "roza_gnss_nmea.cpp"
    INCLUDE_DIRS "include"
    REQUIRES driver nucleo_app
)
