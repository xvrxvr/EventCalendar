file(GLOB main_srcs "*.cpp")
file(GLOB ch_srcs  "challenges/*.cpp")
file(GLOB ico_srcs "icons/*.cpp")
file(GLOB zip_srcs "zip/*.cpp")

set(srcs
    ${main_srcs}
    protocol_examples_utils.c
    ${ch_srcs}
    ${ico_srcs}
    ${zip_srcs}
)

file(GLOB web_srcs "web/*")

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "."
                    EMBED_FILES ${web_srcs} test_bg.jpg
                    )

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
