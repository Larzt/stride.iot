idf_component_register(
    INCLUDE_DIRS "include"
    REQUIRES locator
)

target_compile_options(${COMPONENT_LIB} INTERFACE "-frtti")
