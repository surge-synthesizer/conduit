cmake_minimum_required(VERSION 3.15)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.11 CACHE STRING "Build for 10.1")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(conduit VERSION 0.9.0 LANGUAGES C CXX)

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

add_subdirectory(src)

set(CLAP_TARGET ${PROJECT_NAME}_clap)
add_library(${CLAP_TARGET} MODULE
        src/conduit-clap-entry.cpp
        )
target_link_libraries(${CLAP_TARGET} conduit-impl)
target_library_make_clap(TARGET ${CLAP_TARGET}
        CLAP_NAME "Conduit"
        CLAP_MACOS_BUNDLE_IDENTIFIER org.surge-synth-team.conduit.clap
        CLAP_MACOS_BUNDLE_VERSION 0.1
        COPY_AFTER_BUILD ${COPY_AFTER_BUILD}
        )

set(VST3_TARGET ${PROJECT_NAME}_vst3)
add_library(${VST3_TARGET} MODULE)
target_sources(${VST3_TARGET} PRIVATE src/conduit-clap-entry.cpp)
target_link_libraries(${VST3_TARGET} PRIVATE conduit-impl)
target_add_vst3_wrapper(TARGET ${VST3_TARGET}
        OUTPUT_NAME "Conduit"
        SUPPORTS_ALL_NOTE_EXPRESSIONS TRUE
        )
if (${COPY_AFTER_BUILD})
    target_vst3_copy_after_build(TARGET ${VST3_TARGET})
endif()

add_custom_target(${PROJECT_NAME}_all)
add_dependencies(${PROJECT_NAME}_all ${PROJECT_NAME}_clap ${PROJECT_NAME}_vst3)