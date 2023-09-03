project(sst-juce-gui-shim VERSION 0.9.0 LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

function(stub_juce_library)
    set(oneValueArgs
            JUCE_PATH
            )
    cmake_parse_arguments(JSHIM "" "${oneValueArgs}" "" ${ARGN} )

    set(underlib "juceshim")
    add_library(${underlib} STATIC)
    foreach(JM IN ITEMS juce_gui_basics juce_graphics juce_core juce_events juce_data_structures)
        message(STATUS "sst clap juce shim: synthesizing juce::${JM}")
        if (APPLE)
            target_sources(${underlib} PRIVATE
                    ${JSHIM_JUCE_PATH}/modules/${JM}/${JM}.mm)
        else()
            target_sources(${underlib} PRIVATE
                    ${JSHIM_JUCE_PATH}/modules/${JM}/${JM}.cpp)
        endif()
        target_compile_definitions(${underlib} PUBLIC
                JUCE_MODULE_AVAILABLE_${JM}=1)
        add_library(singleshim_${JM} INTERFACE)
        target_link_libraries(singleshim_${JM} INTERFACE ${underlib})
        add_library(juce::${JM} ALIAS singleshim_${JM})
    endforeach()

    target_compile_definitions(${underlib} PUBLIC
            NDEBUG=$<IF:$<CONFIG:Release>,1,0>
            JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
            JUCE_STANDALONE_APPLICATION=0
            JUCE_USE_CURL=0
            JUCE_WEB_BROWSER=0
            JUCE_USE_FLAC=0
            JUCE_USE_OGGVORBIS=0
            JUCE_USE_WINDOWS_MEDIA_FORMAT=0
            )
    target_include_directories(${underlib} PUBLIC ${JUCE_PATH}/modules)
    if (APPLE)
        target_link_libraries(${underlib} PUBLIC
                "-framework CoreFoundation"
                "-framework AppKit"
                "-framework CoreGraphics"
                "-framework Security"
                "-framework IOKit"
                "-framework CoreImage"
                "-framework CoreVideo"
                "-framework QuartzCore")
        endif()

    add_library(sst::juceshim ALIAS ${underlib})
endfunction(stub_juce_library)

add_library(clap_juce_shim STATIC src/sst/clap_juce_shim/clap_juce_shim_impl.cpp)
if (APPLE)
    target_sources(clap_juce_shim PRIVATE src/sst/clap_juce_shim/clap_juce_shim_impl.mm)
endif()
target_include_directories(clap_juce_shim PUBLIC include)

if (NOT TARGET clap-core)
    message(FATAL_ERROR "I need clap")
endif()
target_link_libraries(clap_juce_shim PRIVATE clap-core juce::juce_gui_basics)
add_library(sst::clap_juce_shim ALIAS clap_juce_shim)