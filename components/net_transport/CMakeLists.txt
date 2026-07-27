idf_component_register(
    SRCS "transport_factory.c" "transport_socket.c"
    INCLUDE_DIRS "."
    REQUIRES lwip
)
