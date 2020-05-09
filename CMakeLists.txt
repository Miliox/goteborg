cmake_minimum_required (VERSION 2.8)

project(goteborg)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)
add_compile_options(-Wall -Wextra -Werror)

# Conan Package Manager Setup
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(SOURCE
    ${SOURCE}
    include/address.hpp
    include/alu.hpp
    include/common.hpp
    include/cpu.hpp
    include/emulator.hpp
    include/gpu.hpp
    include/interrupt.hpp
    include/main.hpp
    include/mmu.hpp
    include/mmuimpl.hpp
    include/registers.hpp

    src/alu.cpp
    src/cpu.cpp
    src/gpu.cpp
    src/mmuimpl.cpp
    src/emulator.cpp
)

file(GLOB_RECURSE RES_SOURCES "res/*")

if (APPLE)
    # Bundled is necessary to handle Retina Displays
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE
        ${SOURCE}
        src/main.cpp
        ${RES_SOURCES}
    )

    #individually set the file's path properties
    foreach(RES_FILE ${RES_SOURCES})
        #Get the relative path from the data-folder to the particular file
        file(RELATIVE_PATH RES_PATH "${PROJECT_SOURCE_DIR}/res/" ${RES_FILE})

        #Set it's location inside the app package (under Resources)
        set_property(SOURCE ${RES_FILE} PROPERTY MACOSX_PACKAGE_LOCATION "Resources/")

    endforeach(RES_FILE)

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
