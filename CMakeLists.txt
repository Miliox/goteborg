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

if (APPLE)
    # Bundled is necessary to handle Retina Displays
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE
        ${SOURCE}
        src/main.cpp
    )

    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION . COMPONENT Runtime
        RUNTIME DESTINATION bin COMPONENT Runtime
    )

    set(APPS "\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app")

    # Directories to look for dependencies
    set(DIRS ${CMAKE_BINARY_DIR})

    install(CODE "include(BundleUtilities)
        fixup_bundle(\"${APPS}\" \"\" \"${DIRS}\")")

    set(CPACK_GENERATOR "DRAGNDROP")
    include(CPack)
else()
    add_executable(${PROJECT_NAME}
        ${SOURCE}
        src/main.cpp
    )
endif()

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
