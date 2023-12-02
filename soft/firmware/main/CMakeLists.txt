set(srcs
    common.cpp
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
    web_ajax_bodies.cpp
    activity.cpp
    input_hw_touch.cpp
    input_hw_fg.cpp
    bg_image.cpp
    box_draw.cpp
    text_draw.cpp
    interactive.cpp
    challenge_mgr.cpp
    animation.cpp
    grid_mgr.cpp
    keyboard.cpp
    main.cpp
    challenges/equest.cpp
    icons/fg_ico.cpp
)

file(GLOB web_srcs "web/*")

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "."
                    EMBED_FILES ${web_srcs} test_bg.jpg
                    )

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
