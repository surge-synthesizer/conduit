add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/geninclude/version.cpp
        DEPENDS ${CMAKE_SOURCE_DIR}/cmake/version.h
        ${CMAKE_SOURCE_DIR}/cmake/version.cpp.in
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND}
        -D PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        -D PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        -D CONDUITSRC=${CMAKE_SOURCE_DIR}
        -D CONDUITBLD=${CMAKE_BINARY_DIR}
        -D AZURE_PIPELINE=${AZURE_PIPELINE}
        -D WIN32=${WIN32}
        -D CMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}
        -D CMAKE_CXX_COMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
        -P ${CMAKE_SOURCE_DIR}/cmake/versiontools.cmake
)

# Platform Specific Compile Settings
add_library(conduit-version-info)
target_sources(conduit-version-info PRIVATE ${CMAKE_BINARY_DIR}/geninclude/version.cpp)
target_include_directories(conduit-version-info PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


