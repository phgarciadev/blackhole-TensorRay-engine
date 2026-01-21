 # gui/CMakeLists.txt

add_library(bhs_gui STATIC)

# Coletar fontes recursivamente (excluindo testes)
file(GLOB_RECURSE gui_SOURCES "*.c")
list(FILTER gui_SOURCES EXCLUDE REGEX "tests/.*")
list(FILTER gui_SOURCES EXCLUDE REGEX "rhi_vulkan.c") # Código legado/morto

target_sources(bhs_gui PRIVATE ${gui_SOURCES})

# Includes
target_include_directories(bhs_gui PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# Dependências
# Dependências de Sistema
find_package(Vulkan REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-cursor xkbcommon)

target_link_libraries(bhs_gui 
    PUBLIC 
        bhs_math
    PRIVATE
        Vulkan::Vulkan
        ${WAYLAND_LIBRARIES}
)

target_include_directories(bhs_gui PRIVATE ${WAYLAND_INCLUDE_DIRS})

add_library(BHS::gui ALIAS bhs_gui)

if(BHS_ENABLE_TESTING)
    add_subdirectory(tests)
endif()
