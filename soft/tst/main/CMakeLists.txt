set(srcs
    test_main.cpp
)

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "."
                    )

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
