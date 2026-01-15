cmake_minimum_required(VERSION 3.16)

# Projeto: Black Hole Simulator
# Lema: "Se não roda em hardware de 2010, você está fazendo errado."
project(BlackHoleSimulator
    VERSION 0.1.0
    DESCRIPTION "Real-time Black Hole Ray Marcher"
    LANGUAGES C CXX
)

# ==============================================================================
# Configurações Globais
# ==============================================================================

# Impedir builds dentro da fonte (source code pollution)
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "
    ############################################################################
    # ERRO: In-source builds não são permitidas.
    # Por favor, crie um diretório de build separado:
    #   mkdir build
    #   cd build
    #   cmake ..
    ############################################################################
    ")
endif()

# Definir C11 e C++17 como padrão
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON) # Necessário para "Kernel Style" (GNU extensions como ##__VA_ARGS__)

set(CMAKE_CXX_STANDARD 17) # Para códigos .cpp (ferramentas, wrappers)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Caminho para módulos CMake customizados
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Diretórios de Saída (Binários e Bibliotecas)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# ==============================================================================
# Módulos Utilitários
# ==============================================================================

include(CompilerWarnings)
include(Sanitizers)
include(ShaderCompiler)

# Opções de Build
option(BHS_ENABLE_TESTING "Habilitar testes unitários" ON)
if(BHS_ENABLE_TESTING)
    enable_testing()
endif()
option(BHS_ENABLE_SANITIZERS "Habilitar Address/Undefined Sanitizers" OFF)

# Configurar Sanitizers se solicitado
if(BHS_ENABLE_SANITIZERS)
    enable_sanitizers(bhs_options)
endif()

# ==============================================================================
# Adicionar Subdiretórios (Camadas)
# ==============================================================================

# 1. Math (Base, zero dependencies)
add_subdirectory(math)

# 2. Framework (Depende de Math)
add_subdirectory(framework)

# 3. Engine (Depende de Math, Framework)
add_subdirectory(engine)

# 4. Source/App (Glue code, Executable)
add_subdirectory(src)

# ==============================================================================
# Testing
# ==============================================================================

if(BHS_ENABLE_TESTING)
    add_subdirectory(tests)
endif()

message(STATUS "Build configurada para: ${CMAKE_SYSTEM_NAME} (${CMAKE_BUILD_TYPE})")
