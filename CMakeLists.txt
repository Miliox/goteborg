cmake_minimum_required (VERSION 2.8)

project(goteborg)

add_definitions('-g')
add_definitions('-Wall')
add_definitions('-Werror')
add_definitions('-std=c++17')

set(SOURCE
    ${SOURCE}
    src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE})
target_include_directories(${PROJECT_NAME} PUBLIC include)
