cmake_minimum_required(VERSION 3.15)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# I don't want to deal with filesystem from two places right now
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "Build for 10.1")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(conduit VERSION 0.1.0 LANGUAGES C CXX)

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
set(SST_PLUGININFRA_FILESYSTEM_FORCE_PLATFORM TRUE CACHE BOOL "Use System FS")
add_subdirectory(libs/sst/sst-plugininfra)

set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES TRUE CACHE BOOL "Get em")
set(CLAP_WRAPPER_DONT_ADD_TARGETS TRUE CACHE BOOL "I'll targetize")
set(CLAP_WRAPPER_BUILD_AUV2 TRUE CACHE BOOL "It's only logical")
add_subdirectory(libs/clap-wrapper)

set(CONDUIT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "Top level source")

include(cmake/CmakeRC.cmake)
add_subdirectory(resources)

include(cmake/git-info.cmake)

add_subdirectory(src)

set(CLAP_TARGET ${PROJECT_NAME}_clap)
add_library(${CLAP_TARGET} MODULE
        src/conduit-clap-entry.cpp
        )
target_link_libraries(${CLAP_TARGET} conduit-impl)
target_library_make_clap(TARGET ${CLAP_TARGET}
        CLAP_NAME "Conduit"
        CLAP_MACOS_BUNDLE_IDENTIFIER org.surge-synth-team.conduit.clap
        CLAP_MACOS_BUNDLE_VERSION ${PROJECT_VERSION}
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
if (WIN32)
    # Question - do we want this default in the helpers
    set_target_properties(${VST3_TARGET}
            PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY VST3
            )
endif()
if (${COPY_AFTER_BUILD})
    target_vst3_copy_after_build(TARGET ${VST3_TARGET})
endif()

if (APPLE)
    set(AUV2_TARGET ${PROJECT_NAME}_auv2)
    add_library(${AUV2_TARGET} MODULE)
    target_sources(${AUV2_TARGET} PRIVATE src/conduit-clap-entry.cpp)
    target_link_libraries(${AUV2_TARGET} PRIVATE conduit-impl)
    target_add_auv2_wrapper(
            TARGET ${AUV2_TARGET}
            OUTPUT_NAME "Conduit"
            BUNDLE_IDENTIFIER "org.surge-synth-team.conduit.auv2"
            BUNDLE_VERSION ${PROJECT_VERSION}

            MANUFACTURER_NAME "Surge Synth Team"
            MANUFACTURER_CODE "SSTx"

            CLAP_TARGET_FOR_CONFIG ${PROJECT_NAME}_clap
    )

    if (${COPY_AFTER_BUILD})
        message(STATUS "conduit: will install auv2")
        target_auv2_copy_after_build(TARGET ${AUV2_TARGET})
    endif()
endif()

add_custom_target(${PROJECT_NAME}_all)
add_dependencies(${PROJECT_NAME}_all ${PROJECT_NAME}_clap ${PROJECT_NAME}_vst3)
if (APPLE)
    add_dependencies(${PROJECT_NAME}_all ${PROJECT_NAME}_auv2)
endif()