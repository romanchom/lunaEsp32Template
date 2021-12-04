idf_component_register(
    SRCS
        "main.cpp"
    EMBED_TXTFILES
        "certificates/ca_cert.pem"
        "certificates/my_cert.pem"
        "certificates/my_key.pem"
)

add_subdirectory(libLunaEsp32)

target_link_libraries(${COMPONENT_LIB}
    PUBLIC libLunaEsp32
)
