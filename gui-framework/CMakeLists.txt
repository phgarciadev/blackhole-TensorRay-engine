 # gui-framework/CMakeLists.txt

add_library(bhs_gui-framework STATIC)

# Coletar fontes recursivamente (excluindo testes)
file(GLOB_RECURSE gui-framework_SOURCES "*.c")
list(FILTER gui-framework_SOURCES EXCLUDE REGEX "tests/.*")
list(FILTER gui-framework_SOURCES EXCLUDE REGEX "rhi_vulkan.c") # Código legado/morto

target_sources(bhs_gui-framework PRIVATE ${gui-framework_SOURCES})

# Includes
target_include_directories(bhs_gui-framework PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# Dependências
# Dependências de Sistema
find_package(Vulkan REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-cursor xkbcommon)

target_link_libraries(bhs_gui-framework 
    PUBLIC 
        bhs_math
    PRIVATE
        Vulkan::Vulkan
        ${WAYLAND_LIBRARIES}
)

target_include_directories(bhs_gui-framework PRIVATE ${WAYLAND_INCLUDE_DIRS})

add_library(BHS::gui-framework ALIAS bhs_gui-framework)

if(BHS_ENABLE_TESTING)
    add_subdirectory(tests)
endif()
