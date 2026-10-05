cmake_minimum_required(VERSION 3.21)
project(ShellEmulator VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
include(CTest)

add_library(shell_core src/commandparser.cpp src/shell.cpp)
target_include_directories(shell_core PUBLIC src)
target_link_libraries(shell_core PUBLIC Qt6::Core)

add_library(shell_gui src/mainwindow.cpp)
target_link_libraries(shell_gui PUBLIC shell_core Qt6::Widgets)

qt_add_executable(shell_emulator MACOSX_BUNDLE src/main.cpp)
target_link_libraries(shell_emulator PRIVATE shell_gui)
set_target_properties(shell_emulator PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "Shell Emulator"
    MACOSX_BUNDLE_GUI_IDENTIFIER "edu.mirea.shell.variant17"
)

foreach(target shell_core shell_gui shell_emulator)
    if(MSVC)
        target_compile_options(${target} PUBLIC /utf-8 PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

if(BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
    foreach(test_name parser shell gui)
        add_executable(test_${test_name} tests/test_${test_name}.cpp)
        target_link_libraries(test_${test_name} PRIVATE shell_gui Qt6::Test)
        add_test(NAME ${test_name} COMMAND test_${test_name})
        set_tests_properties(${test_name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endforeach()
endif()
