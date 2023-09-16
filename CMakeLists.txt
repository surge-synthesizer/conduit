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

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Never want shared if not specified")
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
add_subdirectory(libs/fmt EXCLUDE_FROM_ALL)

add_subdirectory(libs/sst/sst-clap-helpers)
if (NOT DEFINED JUCE_PATH)
    set(JUCE_PATH "${CMAKE_SOURCE_DIR}/libs/JUCE")
endif()
add_clap_juce_shim(JUCE_PATH ${JUCE_PATH})

add_library(simde INTERFACE)
target_include_directories(simde INTERFACE libs/simde)

add_subdirectory(libs/sst/sst-jucegui)
add_subdirectory(libs/sst/sst-basic-blocks)
set(SST_PLUGININFRA_FILESYSTEM_FORCE_PLATFORM TRUE CACHE BOOL "Use System FS")
add_subdirectory(libs/sst/sst-plugininfra)
add_subdirectory(libs/sst/sst-cpputils)
add_subdirectory(libs/sst/sst-filters)

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

            CLAP_TARGET_FOR_CONFIG ${PROJECT_NAME}_clap
    )

    if (${COPY_AFTER_BUILD})
        message(STATUS "conduit: will install auv2")
        target_auv2_copy_after_build(TARGET ${AUV2_TARGET})
    endif()
endif()

set(SYNTH_STANDALONE_TARGET ${PROJECT_NAME}_polysynth_standalone)
add_executable(${SYNTH_STANDALONE_TARGET})
target_sources(${SYNTH_STANDALONE_TARGET} PRIVATE src/conduit-clap-entry.cpp)
target_link_libraries(${SYNTH_STANDALONE_TARGET} PRIVATE conduit-impl)
target_add_standalone_wrapper(TARGET ${SYNTH_STANDALONE_TARGET}
        OUTPUT_NAME "Conduit Polysynth"
        STATICALLY_LINKED_CLAP_ENTRY True
        PLUGIN_ID "org.surge-synth-team.conduit.polysynth")


set(DELAY_STANDALONE_TARGET ${PROJECT_NAME}_delay_standalone)
add_executable(${DELAY_STANDALONE_TARGET})
target_sources(${DELAY_STANDALONE_TARGET} PRIVATE src/conduit-clap-entry.cpp)
target_link_libraries(${DELAY_STANDALONE_TARGET} PRIVATE conduit-impl)
target_add_standalone_wrapper(TARGET ${DELAY_STANDALONE_TARGET}
        OUTPUT_NAME "Conduit Polymetric Delay"
        STATICALLY_LINKED_CLAP_ENTRY True
        PLUGIN_ID "org.surge-synth-team.conduit.polymetric-delay")

set(CHORDMEM_STANDALONE_TARGET ${PROJECT_NAME}_chordmem_standalone)
add_executable(${CHORDMEM_STANDALONE_TARGET})
target_sources(${CHORDMEM_STANDALONE_TARGET} PRIVATE src/conduit-clap-entry.cpp)
target_link_libraries(${CHORDMEM_STANDALONE_TARGET} PRIVATE conduit-impl)
target_add_standalone_wrapper(TARGET ${CHORDMEM_STANDALONE_TARGET}
        OUTPUT_NAME "Conduit Chord Memory"
        STATICALLY_LINKED_CLAP_ENTRY True
        PLUGIN_ID "org.surge-synth-team.conduit.chord-memory")


## Finally set up an ALL target which builds and collects
function(add_to_all)
    cmake_parse_arguments(AA "" "" "TARGETS" ${ARGN} )

    set(pn ${PROJECT_NAME}_all)
    if (NOT TARGET ${pn})
        add_custom_target(${pn})
    endif()

    add_dependencies(${pn} ${AA_TARGETS})

    set(coldir "${CMAKE_BINARY_DIR}/conduit_products")
    foreach(TGT ${AA_TARGETS})
        if (APPLE)
            add_custom_command(TARGET ${pn}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory ${coldir}
                COMMAND ${CMAKE_COMMAND} -E copy_directory $<TARGET_FILE_DIR:${TGT}>/../../../$<TARGET_PROPERTY:${TGT},MACOSX_BUNDLE_BUNDLE_NAME>.$<TARGET_PROPERTY:${TGT},BUNDLE_EXTENSION>/
                    ${coldir}/$<TARGET_PROPERTY:${TGT},MACOSX_BUNDLE_BUNDLE_NAME>.$<TARGET_PROPERTY:${TGT},BUNDLE_EXTENSION>/
                )
        else()
            add_custom_command(TARGET ${pn}
                    POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${coldir}
                    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TGT}> ${coldir}
                    )
        endif()
    endforeach ()
endfunction(add_to_all)

add_to_all(TARGETS ${PROJECT_NAME}_clap ${PROJECT_NAME}_vst3)
if (APPLE)
    add_to_all(TARGETS ${PROJECT_NAME}_auv2)
endif()
add_to_all(TARGETS ${SYNTH_STANDALONE_TARGET} ${DELAY_STANDALONE_TARGET} ${CHORDMEM_STANDALONE_TARGET})


#add_executable(csd_standalone)
#target_add_standalone_wrapper(TARGET csd_standalone
#    OUTPUT_NAME "Clap Saw Demo"
#    HOSTED_CLAP_NAME "clap-saw-demo"
#    PLUGIN_ID "org.surge-synth-team.clap-saw-demo")
