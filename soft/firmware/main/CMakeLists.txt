set(srcs
    protocol_examples_utils.c
    hadrware.cpp
    R503.cpp
    Touchscreen.cpp
    wifi_module.cpp
    setup_data.cpp
    web_server.cpp
    web_files_access.cpp
    web_vars_inc.cpp
    web_gadgets.cpp
    web_vars.cpp
    activity.cpp
    main.cpp
)

file(GLOB web_srcs "web/*")

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "."
                    EMBED_FILES ${web_srcs}
                    )

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
