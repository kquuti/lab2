cmake_minimum_required(VERSION 3.16)
project(ReferenceCollection LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Логіка програми: без Qt, лише стандартна бібліотека
add_library(refs_core STATIC
  src/core/Reference.cpp
  src/core/Collection.cpp
  src/core/Citation.cpp
  src/core/Storage.cpp
)
target_include_directories(refs_core PUBLIC src)

# Програма з графічним інтерфейсом
add_executable(refs
  src/main.cpp
  src/gui/MainWindow.cpp
  src/gui/ReferenceDialog.cpp
  src/gui/ContextDialog.cpp
  src/gui/ExportDialog.cpp
)
target_link_libraries(refs PRIVATE refs_core Qt6::Widgets)

# Тести логіки
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(core_tests tests/test_core.cpp)
target_link_libraries(core_tests PRIVATE refs_core GTest::gtest_main)