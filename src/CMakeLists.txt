project(conduit-src)

add_library(conduit-shared STATIC
        conduit-shared/shared-symbols.cpp)
target_include_directories(conduit-shared PUBLIC .)
target_compile_definitions(conduit-shared PUBLIC -DCONDUIT_SOURCE_DIR=\"${CONDUIT_SOURCE_DIR}\")
target_link_libraries(conduit-shared PUBLIC
        clap
        clap-helpers
        conduit-resources
        conduit-version-info
        fmt
        tinyxml
        simde
        juce::juce_gui_basics
        sst::clap_juce_shim
        sst-basic-blocks
        sst-cpputils
        sst-jucegui
        sst-filters
        sst-effects
        sst-voicemanager
        sst-plugininfra
        sst-waveshapers
        mts-esp-client
        clap-wrapper-extensions
        )

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(conduit-shared PUBLIC -Wall -Werror -Wno-sign-compare -Wno-ignored-attributes -Wno-ignored-qualifiers)
endif()

add_library(conduit-impl STATIC)
target_link_libraries(conduit-impl PUBLIC conduit-shared)
target_compile_definitions(conduit-impl PUBLIC $<$<CONFIG:Debug>:CONDUIT_DEBUG_BUILD>)

function(add_to_conduit)
    set(multiValArgs SOURCE INCLUDE)

    cmake_parse_arguments(A2C "" "" "${multiValArgs}" ${ARGN})

    target_sources(conduit-impl PRIVATE ${A2C_SOURCE})
    target_include_directories(conduit-impl PRIVATE ${A2C_INCLUDE})
endfunction(add_to_conduit)

add_subdirectory(polysynth)
add_subdirectory(polymetric-delay)
add_subdirectory(chord-memory)
add_subdirectory(ring-modulator)
add_subdirectory(clap-event-monitor)
