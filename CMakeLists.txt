cmake_minimum_required (VERSION 2.8)

project(goteborg)

add_definitions('-g')
add_definitions('-Wall')
add_definitions('-Werror')
add_definitions('-std=c++17')

# Conan Package Manager Setup
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(SOURCE
    ${SOURCE}
    src/lr35902.cpp
    src/mmuimpl.cpp
)

add_executable(${PROJECT_NAME}
    ${SOURCE}
    src/main.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC include ${CONAN_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})

set(TEST
    ${TEST}
    ${SOURCE}
)

add_executable(${PROJECT_NAME}-test
    ${TEST}
    test/main.cpp
    test/mmuimpl-tests.cpp
)

target_include_directories(${PROJECT_NAME}-test
    PUBLIC include ${CONAN_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}-test ${CONAN_LIBS})
