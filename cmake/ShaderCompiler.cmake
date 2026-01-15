# cmake/ShaderCompiler.cmake
# Compila Shaders/Kernels (OpenCL C) para SPIR-V usando Clang

function(target_compile_shaders target_name)
    # Detectar Clang
    find_program(CLANG_EXECUTABLE NAMES clang clang-15 clang-14 clang-12 REQUIRED)

    set(SHADER_SRC_DIR "${CMAKE_SOURCE_DIR}/src/assets/shaders")
    set(SHADER_OUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/shaders")

    # Encontrar arquivos de shader
    file(GLOB SHADER_SOURCES "${SHADER_SRC_DIR}/*.c")

    if(NOT SHADER_SOURCES)
        message(WARNING "Nenhum shader encontrado em ${SHADER_SRC_DIR}")
        return()
    endif()

    file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

    set(SPV_FILES "")

    # Flags baseadas no Makefile original
    # CLANG_SPIRV_FLAGS := -c -target spirv64 -x cl -cl-std=CL2.0 
    #                      -Xclang -finclude-default-header -O3 
    #                      -I . -D BHS_SHADER_COMPILER -D BHS_USE_FLOAT
    
    set(SPIRV_FLAGS
        -c
        -target spirv64
        -x cl
        -cl-std=CL2.0
        -Xclang -finclude-default-header
        -O3
        -I ${CMAKE_SOURCE_DIR} # Para incluir headers compartilhados como "math/vec4.h" se necessario
        -D BHS_SHADER_COMPILER
        -D BHS_USE_FLOAT
        -Wno-unused-value
    )

    foreach(SRC ${SHADER_SOURCES})
        get_filename_component(FILENAME ${SRC} NAME_WE)
        set(SPV_OUT "${SHADER_OUT_DIR}/${FILENAME}.spv")

        add_custom_command(
            OUTPUT ${SPV_OUT}
            COMMAND ${CLANG_EXECUTABLE} ${SPIRV_FLAGS} ${SRC} -o ${SPV_OUT}
            DEPENDS ${SRC}
            COMMENT "Compilando Shader (SPIR-V): ${FILENAME}.c -> ${FILENAME}.spv"
            VERBATIM
        )

        list(APPEND SPV_FILES ${SPV_OUT})
    endforeach()

    # Adicionar target customizado para garantir que shaders sejam compilados
    add_custom_target(${target_name}_shaders ALL DEPENDS ${SPV_FILES})
    
    # Adicionar dependência ao target principal
    add_dependencies(${target_name} ${target_name}_shaders)

endfunction()
