cmake_minimum_required(VERSION 3.11)
project(imgui_gradient-tests)

add_executable(${PROJECT_NAME} tests.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

# Set warning level
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -pedantic-errors -Wconversion -Wsign-conversion)
endif()

# Maybe enable warnings as errors
set(WARNINGS_AS_ERRORS_FOR_IMGUI_GRADIENT OFF CACHE BOOL "ON iff you want to treat warnings as errors")

if(WARNINGS_AS_ERRORS_FOR_IMGUI_GRADIENT)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /WX)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
    endif()
endif()

add_subdirectory(.. ${CMAKE_CURRENT_SOURCE_DIR}/build/imgui_gradient)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui_gradient::imgui_gradient)

# ---Add doctest---
include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest
    GIT_TAG 1da23a3e8119ec5cce4f9388e91b065e20bf06f5
)
FetchContent_MakeAvailable(doctest)
target_link_libraries(${PROJECT_NAME} PRIVATE doctest::doctest)

# ---Ignore .vscode/settings.json in Git---
find_package(Git QUIET)

if(GIT_FOUND)
    get_filename_component(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)

    if(EXISTS "${PARENT_DIR}/.git")
        execute_process(COMMAND ${GIT_EXECUTABLE} update-index --assume-unchanged .vscode/settings.json
            WORKING_DIRECTORY ${PARENT_DIR}
            RESULT_VARIABLE ERRORS)

        if(NOT ERRORS EQUAL "0")
            message("Git assume-unchanged failed: ${ERRORS}")
        endif()
    else()
        message("No Git repository found.")
    endif()
else()
    message("Git executable not found.")
endif()

# ---IMGUI_DEFINE_MATH_OPERATORS---
target_compile_definitions(imgui_gradient PRIVATE IMGUI_DEFINE_MATH_OPERATORS)
target_compile_definitions(${PROJECT_NAME} PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

# ---Add quick_imgui---
include(FetchContent)
FetchContent_Declare(
    quick_imgui
    GIT_REPOSITORY https://github.com/Coollab-Art/quick_imgui
    GIT_TAG 42d99ada8bd474ae6ca8e98193807daf39b6b93d
)
FetchContent_MakeAvailable(quick_imgui)
cmake_policy(SET CMP0079 NEW)
target_link_libraries(imgui_gradient PUBLIC quick_imgui::quick_imgui)