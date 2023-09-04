cmake_minimum_required(VERSION 3.15)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.11 CACHE STRING "Build for 10.1")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(conduit-polysynth VERSION 0.9.0 LANGUAGES C CXX)

if (APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
    set(CMAKE_OBJC_VISIBILITY_PRESET hidden)
    set(CMAKE_OBJCXX_VISIBILITY_PRESET hidden)
endif()

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

set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES TRUE CACHE BOOL "Get em")
set(CLAP_WRAPPER_DONT_ADD_TARGETS TRUE CACHE BOOL "I'll targetize")
add_subdirectory(libs/clap-wrapper)

set(IMPL_TARGET ${PROJECT_NAME}_impl)
add_library(${IMPL_TARGET} STATIC
        src/conduit-polysynth.cpp
        src/saw-voice.cpp
        src/juce-editor.cpp
        )
target_link_libraries(${IMPL_TARGET} PUBLIC
        clap-core
        clap-helpers
        readerwriterqueue
        fmt
        juce::juce_gui_basics
        sst::clap_juce_shim
        sst-basic-blocks
        sst-jucegui)

set(CLAP_TARGET ${PROJECT_NAME}_clap)
add_library(${CLAP_TARGET} MODULE
        src/conduit-polysynth-entry.cpp
        )
target_link_libraries(${CLAP_TARGET} ${IMPL_TARGET})
target_library_make_clap(TARGET ${CLAP_TARGET}
        CLAP_NAME "Conduit Polysynth"
        CLAP_MACOS_BUNDLE_IDENTIFIER org.surge-synth-team.conduit.poly.clap
        CLAP_MACOS_BUNDLE_VERSION 0.1
        COPY_AFTER_BUILD ${COPY_AFTER_BUILD}
        )

set(VST3_TARGET ${PROJECT_NAME}_vst3)
add_library(${VST3_TARGET} MODULE)
if (WIN32)
    # Our windows strategy includes the CLAP in the vst3 dll so
    target_sources(${VST3_TARGET} PRIVATE src/conduit-polysynth-entry.cpp)
    target_link_libraries(${VST3_TARGET} PRIVATE ${IMPL_TARGET})
endif()
add_dependencies(${VST3_TARGET} ${CLAP_TARGET})
target_add_vst3_wrapper(TARGET ${VST3_TARGET}
        OUTPUT_NAME "Conduit PolySynth"
        SUPPORTS_ALL_NOTE_EXPRESSIONS TRUE
        MACOS_EMBEDDED_CLAP_LOCATION "${CMAKE_BINARY_DIR}/Conduit PolySynth.clap"
        )
if (${COPY_AFTER_BUILD})
    target_vst3_copy_after_build(TARGET ${VST3_TARGET})
endif()