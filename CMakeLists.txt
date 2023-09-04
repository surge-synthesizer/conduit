cmake_minimum_required(VERSION 3.15)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.11 CACHE STRING "Build for 10.1")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(clap-saw-juicy VERSION 0.9.0 LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# use asan as an option (currently mac only)
option(USE_SANITIZER "Build and link with ASAN" FALSE)

# Copy on mac (could expand to other platforms)
option(COPY_AFTER_BUILD "Copy the clap to ~/Library on MACOS, ~/.clap on linux" FALSE)

add_subdirectory(libs/clap EXCLUDE_FROM_ALL)
add_subdirectory(libs/clap-helpers EXCLUDE_FROM_ALL)
add_subdirectory(libs/readerwriterqueue EXCLUDE_FROM_ALL)
add_subdirectory(libs/fmt EXCLUDE_FROM_ALL)

add_subdirectory(libs/sst/sst-clap-helpers)
if (NOT DEFINED JUCE_PATH)
    set(JUCE_PATH "${CMAKE_SOURCE_DIR}/libs/JUCE")
endif()
add_clap_juce_shim(JUCE_PATH ${JUCE_PATH})

add_subdirectory(libs/sst/sst-jucegui)
add_subdirectory(libs/sst/sst-basic-blocks)

add_library(${PROJECT_NAME} MODULE
        src/clap-saw-juicy.cpp
        src/saw-voice.cpp
        src/clap-saw-juicy-entry.cpp
        src/juce-editor.cpp
        )
target_library_is_clap(TARGET ${PROJECT_NAME}
        CLAP_NAME ${PROJECT_NAME}
        CLAP_MACOS_BUNDLE_IDENTIFIER org.surge-synth-team.conduit.poly.clap
        CLAP_MACOS_BUNDLE_VERSION 0.1
        COPY_AFTER_BUILD TRUE
        )
target_link_libraries(${PROJECT_NAME}
        clap-core
        clap-helpers
        readerwriterqueue
        fmt
        juce::juce_gui_basics
        sst::clap_juce_shim
        sst-basic-blocks
        sst-jucegui)
