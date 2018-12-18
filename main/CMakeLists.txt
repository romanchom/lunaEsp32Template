set(COMPONENT_SRCS "main.cpp")

set(COMPONENT_EMBED_TXTFILES 
    certificates/ca_cert.pem
    certificates/my_cert.pem
    certificates/my_key.pem
)

register_component()
