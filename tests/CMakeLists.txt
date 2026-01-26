# tests/CMakeLists.txt

# Math Unit Tests
if(EXISTS "${CMAKE_SOURCE_DIR}/math/tests/test_core.c")
    add_executable(unit_tests_math "${CMAKE_SOURCE_DIR}/math/tests/test_core.c")
    target_link_libraries(unit_tests_math PRIVATE bhs_math) 
    # Linkar math apenas. Se precisar de gui, adicionar.
    target_include_directories(unit_tests_math PRIVATE ${CMAKE_SOURCE_DIR})
    add_test(NAME MathCoreTest COMMAND unit_tests_math)
endif()

# Global Integration Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_scene_lifecycle.c")
    add_executable(integration_tests "${CMAKE_CURRENT_SOURCE_DIR}/test_scene_lifecycle.c")
    target_link_libraries(integration_tests PRIVATE bhs_engine bhs_gui bhs_math)
    target_include_directories(integration_tests PRIVATE ${CMAKE_SOURCE_DIR})
    add_test(NAME SceneLifecycleTest COMMAND integration_tests)
endif()

message(STATUS "Testes configurados.")
