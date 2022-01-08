idf_component_register(
    SRCS
        "main.cpp"
)

add_subdirectory(../../../libLunaEsp32 libLunaEsp32)

target_link_libraries(${COMPONENT_LIB}
    PUBLIC libLunaEsp32
)
