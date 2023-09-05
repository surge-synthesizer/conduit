project(polysynth)

add_library(${PROJECT_NAME} STATIC
        polysynth.cpp
        saw-voice.cpp
        polysynth-editor.cpp
        )

target_include_directories(${PROJECT_NAME} PUBLIC .)

target_link_libraries(${PROJECT_NAME} PUBLIC
        conduit-shared
        clap-core
        clap-helpers
        readerwriterqueue
        fmt
        juce::juce_gui_basics
        sst::clap_juce_shim
        sst-basic-blocks
        sst-jucegui)