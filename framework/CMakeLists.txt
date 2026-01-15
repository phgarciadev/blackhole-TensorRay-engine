 # framework/CMakeLists.txt

add_library(bhs_framework STATIC)

# Coletar fontes recursivamente (excluindo testes)
file(GLOB_RECURSE FRAMEWORK_SOURCES "*.c")
list(FILTER FRAMEWORK_SOURCES EXCLUDE REGEX "tests/.*")
list(FILTER FRAMEWORK_SOURCES EXCLUDE REGEX "rhi_vulkan.c") # Código legado/morto

target_sources(bhs_framework PRIVATE ${FRAMEWORK_SOURCES})

# Includes
target_include_directories(bhs_framework PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# Dependências
# Dependências de Sistema
find_package(Vulkan REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-cursor xkbcommon)

target_link_libraries(bhs_framework 
    PUBLIC 
        bhs_math
    PRIVATE
        Vulkan::Vulkan
        ${WAYLAND_LIBRARIES}
)

target_include_directories(bhs_framework PRIVATE ${WAYLAND_INCLUDE_DIRS})

add_library(BHS::Framework ALIAS bhs_framework)

if(BHS_ENABLE_TESTING)
    add_subdirectory(tests)
endif()
