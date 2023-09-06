project(conduit-src)

add_library(conduit-shared STATIC conduit-shared/shared-symbols.cpp)
target_include_directories(conduit-shared PUBLIC .)
target_link_libraries(conduit-shared PUBLIC
        clap
        clap-helpers
        fmt
        readerwriterqueue
        juce::juce_gui_basics
        sst::clap_juce_shim
        sst-basic-blocks
        sst-jucegui)

add_subdirectory(polysynth)
add_subdirectory(polymetric-delay)

add_library(conduit-impl INTERFACE)
target_link_libraries(conduit-impl INTERFACE polysynth polymetric-delay)