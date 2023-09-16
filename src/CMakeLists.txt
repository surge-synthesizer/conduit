project(conduit-src)

add_library(conduit-shared STATIC
        conduit-shared/shared-symbols.cpp
        conduit-shared/editor-base.cpp)
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
        sst-jucegui
        clap-wrapper-extensions
        )

add_subdirectory(polysynth)
add_subdirectory(polymetric-delay)
add_subdirectory(chord-memory)
add_subdirectory(ring-modulator)

add_library(conduit-impl INTERFACE)
target_link_libraries(conduit-impl INTERFACE
        polysynth
        polymetric-delay
        chord-memory
        ring-modulator
        conduit-shared
        )