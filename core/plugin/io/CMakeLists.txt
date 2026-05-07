file(GLOB SRCS "stride_buzzer.cc" "*.cc")

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "include"
    REQUIRES driver
)
