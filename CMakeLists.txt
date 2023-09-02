cmake_minimum_required(VERSION 3.15)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.11 CACHE STRING "Build for 10.1")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE Type Unspecified; picking Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

project(clap-saw-juicy VERSION 0.9.0 LANGUAGES C CXX OBJC OBJCXX)


set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)


# use asan as an option (currently mac only)
option(USE_SANITIZER "Build and link with ASAN" FALSE)

# Copy on mac (could expand to other platforms)
option(COPY_AFTER_BUILD "Copy the clap to ~/Library on MACOS, ~/.clap on linux" FALSE)

add_subdirectory(libs/clap EXCLUDE_FROM_ALL)
add_subdirectory(libs/clap-helpers EXCLUDE_FROM_ALL)
add_subdirectory(libs/readerwriterqueue EXCLUDE_FROM_ALL)

## FAKE JUCE SUPPORT
if (NOT DEFINED JUCE_PATH)
    set(JUCE_PATH "${CMAKE_SOURCE_DIR}/libs/JUCE")
endif()
add_library(juce_gui_sub STATIC)
if (APPLE)
    target_sources(juce_gui_sub PRIVATE
            ${JUCE_PATH}/modules/juce_core/juce_core.mm
            ${JUCE_PATH}/modules/juce_gui_basics/juce_gui_basics.mm
            ${JUCE_PATH}/modules/juce_graphics/juce_graphics.mm
            ${JUCE_PATH}/modules/juce_events/juce_events.mm
            ${JUCE_PATH}/modules/juce_data_structures/juce_data_structures.mm
            )
    target_compile_definitions(juce_gui_sub PUBLIC JUCE_MAC=1)
else()
    target_sources(juce_gui_sub PRIVATE
            ${JUCE_PATH}/modules/juce_gui_basics/juce_gui_basics.cpp
            ${JUCE_PATH}/modules/juce_graphics/juce_graphics.cpp
            ${JUCE_PATH}/modules/juce_core/juce_core.cpp
            ${JUCE_PATH}/modules/juce_events/juce_events.cpp
            ${JUCE_PATH}/modules/juce_data_structures/juce_data_structures.cpp
            )
endif()
target_compile_definitions(juce_gui_sub PUBLIC
        NDEBUG=$<IF:$<CONFIG:Release>,1,0>
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_MODULE_AVAILABLE_juce_gui_basics=1
        JUCE_MODULE_AVAILABLE_juce_graphics=1
        JUCE_MODULE_AVAILABLE_juce_core=1
        JUCE_MODULE_AVAILABLE_juce_events=1
        JUCE_MODULE_AVAILABLE_juce_data_structures=1
        JUCE_STANDALONE_APPLICATION=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_FLAC=0
        JUCE_USE_OGGVORBIS=0
        JUCE_USE_WINDOWS_MEDIA_FORMAT=0
        )
target_include_directories(juce_gui_sub PUBLIC ${JUCE_PATH}/modules)
## END FAKE JUCE SUPPORT

add_library(${PROJECT_NAME} MODULE
        src/clap-saw-demo.cpp
        src/saw-voice.cpp
        src/clap-saw-demo-pluginentry.cpp
        src/juce-editor.cpp
        src/juce-details-mac.mm
)
target_link_libraries(${PROJECT_NAME} clap-core clap-helpers readerwriterqueue juce_gui_sub)

target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_GUI=0)

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            BUNDLE True
            BUNDLE_EXTENSION clap
            MACOSX_BUNDLE_GUI_IDENTIFIER org.surge-synth-team.${PROJECT_NAME}
            MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
            MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/cmake/${PROJECT_NAME}.plist.in
            )
    target_link_libraries(${PROJECT_NAME} "-framework CoreFoundation" "-framework AppKit" "-framework CoreGraphics"
            "-framework Security" "-framework IOKit" "-framework CoreImage" "-framework CoreVideo" "-framework QuartzCore")

    target_compile_definitions(${PROJECT_NAME} PRIVATE IS_MAC=1)
    target_compile_options(${PROJECT_NAME} PRIVATE
            -Werror
            $<$<BOOL:${USE_SANITIZER}>:-fsanitize=address>
            $<$<BOOL:${USE_SANITIZER}>:-fsanitize=undefined>
            )
    target_link_options(${PROJECT_NAME} PRIVATE
            $<$<BOOL:${USE_SANITIZER}>:-fsanitize=address>
            $<$<BOOL:${USE_SANITIZER}>:-fsanitize=undefined>
            )

    if (${COPY_AFTER_BUILD})
        message(STATUS "Will copy plugin after every build" )
        set(products_folder ${CMAKE_BINARY_DIR})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo "Installing ${products_folder}/${PROJECT_NAME}.clap to ~/Library/Audio/Plug-Ins/CLAP/"
                COMMAND ${CMAKE_COMMAND} -E make_directory "~/Library/Audio/Plug-Ins/CLAP"
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${products_folder}/${PROJECT_NAME}.clap" "~/Library/Audio/Plug-Ins/CLAP/${PROJECT_NAME}.clap"
                )
    endif()
elseif(UNIX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE IS_LINUX=1)
    target_sources(${PROJECT_NAME} PRIVATE src/linux-vstgui-adapter.cpp)
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".clap" PREFIX "")
    if (${COPY_AFTER_BUILD})
        message(STATUS "Will copy plugin after every build" )
        set(products_folder ${CMAKE_BINARY_DIR})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo "Installing ${products_folder}/${PROJECT_NAME}.clap to ~/.clap"
                COMMAND ${CMAKE_COMMAND} -E make_directory "~/.clap"
                COMMAND ${CMAKE_COMMAND} -E copy "${products_folder}/${PROJECT_NAME}.clap" "~/.clap"
                )
    endif()

else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE IS_WIN=1)
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".clap" PREFIX "")
endif()
