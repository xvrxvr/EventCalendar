set(srcs
    protocol_examples_utils.c
    hadrware.cpp
    R503.cpp
    Touchscreen.cpp
    wifi_module.cpp
    setup_data.cpp
    web_server.cpp
    main.cpp
)

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "."
                    )

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
