project(chord-memory)

add_library(${PROJECT_NAME} STATIC
        ${PROJECT_NAME}.cpp
        ${PROJECT_NAME}-editor.cpp
        )

target_include_directories(${PROJECT_NAME} PUBLIC .)

target_link_libraries(${PROJECT_NAME} PUBLIC
        conduit-shared
        clap
        clap-helpers
        fmt
        juce::juce_gui_basics
        sst::clap_juce_shim
        sst-basic-blocks
        sst-jucegui)
