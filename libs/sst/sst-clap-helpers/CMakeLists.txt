project(sst-juce-gui-shim VERSION 0.9.0 LANGUAGES C CXX)

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

set(CLAP_JUCE_SHIM_SOURCE ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "Where the source is")
message(STATUS "CLAP_JUCE_SHIM_SOURCE is ${CLAP_JUCE_SHIM_SOURCE}")

function(add_clap_juce_shim)
    set(oneValueArgs
            JUCE_PATH
            )
    cmake_parse_arguments(JSHIM "" "${oneValueArgs}" "" ${ARGN} )

    if (NOT DEFINED JSHIM_JUCE_PATH)
        message(FATAL_ERROR "sst-clap-helpers: add_clap_juce_shim requires a JUCE PATH")
    endif()

    set(JUCE_MODULES_ONLY ON CACHE BOOL "Only use juce modules")
    add_subdirectory(${JUCE_PATH} juce_for_shim EXCLUDE_FROM_ALL)

    add_library(clap_juce_shim STATIC ${CLAP_JUCE_SHIM_SOURCE}/src/sst/clap_juce_shim/clap_juce_shim_impl.cpp)
    if (APPLE)
        target_sources(clap_juce_shim PRIVATE ${CLAP_JUCE_SHIM_SOURCE}/src/sst/clap_juce_shim/clap_juce_shim_impl.mm)
    endif()
    target_include_directories(clap_juce_shim PUBLIC ${CLAP_JUCE_SHIM_SOURCE}/include)

    if (APPLE)
        target_compile_definitions(clap_juce_shim PUBLIC SHIM_MAC=1)
    elseif (LINUX)
        target_compile_definitions(clap_juce_shim PUBLIC SHIM_LINUX=1)
    else()
        target_compile_definitions(clap_juce_shim PUBLIC SHIM_WINDOWS=1)
    endif()

    if (NOT TARGET clap-core)
        message(FATAL_ERROR "I need clap")
    endif()
    target_link_libraries(clap_juce_shim PRIVATE clap-core juce::juce_gui_basics)
    add_library(sst::clap_juce_shim ALIAS clap_juce_shim)
endfunction(add_clap_juce_shim)

function(target_library_make_clap)
    set(oneValueArgs
            TARGET
            CLAP_NAME
            CLAP_MACOS_BUNDLE_GUI_IDENTIFIER
            CLAP_MACOS_BUNDLE_VERSION
            COPY_AFTER_BUILD
            )
    cmake_parse_arguments(TCLP "" "${oneValueArgs}" "" ${ARGN} )

    if (NOT DEFINED TCLP_TARGET)
        message(FATAL_ERROR "You must define TARGET in target_library_is_clap")
    endif()

    if (NOT DEFINED TCLP_CLAP_NAME)
        message(STATUS "Using target name as clap name in target_libarry_is_clap")
        set(TCLP_CLAP_NAME TCLP_TARGET)
    endif()

    if (NOT DEFINED TCLP_CLAP_MACOS_BUNDLE_GUI_IDENTIFIER)
        message(STATUS "Using sst default macos bundle prefix in target_libarry_is_clap")
        set(TCLP_CLAP_MACOS_BUNDLE_GUI_IDENTIFIER "org.surge-synth-team.${TCLP_TARGET}")
    endif()

    if (NOT DEFINED TCLP_CLAP_MACOS_BUNDLE_VERSION)
        message(STATUS "No bundle version in target_library_is_clap; using 0.1")
        set(TCLP_CLAP_MACOS_BUNDLE_VERSION "0.1")
    endif()

    if(APPLE)
        set_target_properties(${TCLP_TARGET} PROPERTIES
                BUNDLE True
                BUNDLE_EXTENSION clap
                LIBRARY_OUTPUT_NAME ${TCLP_CLAP_NAME}
                MACOSX_BUNDLE_GUI_IDENTIFIER ${TCLP_CLAP_MACOS_BUNDLE_GUI_IDENTIFIER}
                MACOSX_BUNDLE_BUNDLE_NAME ${TCLP_CLAP_NAME}
                MACOSX_BUNDLE_BUNDLE_VERSION "${TCLP_CLAP_MACOS_BUNDLE_VERSION}"
                MACOSX_BUNDLE_SHORT_VERSION_STRING "${TCLP_CLAP_MACOS_BUNDLE_VERSION}"
                MACOSX_BUNDLE_INFO_PLIST ${CLAP_JUCE_SHIM_SOURCE}/cmake/generic-clap.plist.in
                )

        if (NOT ${CMAKE_GENERATOR} STREQUAL "Xcode")
            add_custom_command(TARGET ${TCLP_TARGET} POST_BUILD
                    WORKING_DIRECTORY $<TARGET_PROPERTY:${TCLP_TARGET},LIBRARY_OUTPUT_DIRECTORY>
                    COMMAND SetFile -a B "$<TARGET_PROPERTY:${TCLP_TARGET},LIBRARY_OUTPUT_NAME>.$<TARGET_PROPERTY:${TCLP_TARGET},BUNDLE_EXTENSION>")
        endif()
        if (${COPY_AFTER_BUILD})
            message(STATUS "Will copy plugin after every build" )
            set(products_folder ${CMAKE_BINARY_DIR})
            add_custom_command(TARGET ${TCLP_TARGET} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E echo "Installing ${products_folder}/${TCLP_CLAP_NAME}.clap to ~/Library/Audio/Plug-Ins/CLAP/"
                    COMMAND ${CMAKE_COMMAND} -E make_directory "~/Library/Audio/Plug-Ins/CLAP"
                    COMMAND ${CMAKE_COMMAND} -E copy_directory "${products_folder}/${TCLP_CLAP_NAME}.clap" "~/Library/Audio/Plug-Ins/CLAP/${TCLP_CLAP_NAME}.clap"
                    )
        endif()
    elseif(UNIX)
        set_target_properties(${TCLP_TARGET} PROPERTIES OUTPUT_NAME ${TCLP_CLAP_NAME} SUFFIX ".clap" PREFIX "")
        if (${COPY_AFTER_BUILD})
            message(STATUS "Will copy plugin after every build" )
            set(products_folder ${CMAKE_BINARY_DIR})
            add_custom_command(TARGET ${TCLP_TARGET} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E echo "Installing ${products_folder}/${TCLP_TARGET}.clap to ~/.clap"
                    COMMAND ${CMAKE_COMMAND} -E make_directory "~/.clap"
                    COMMAND ${CMAKE_COMMAND} -E copy "${products_folder}/${TCLP_TARGET}.clap" "~/.clap"
                    )
        endif()

    else()
        set_target_properties(${TCLP_TARGET}
                PROPERTIES
                OUTPUT_NAME ${TCLP_CLAP_NAME}
                SUFFIX ".clap" PREFIX ""
                LIBRARY_OUTPUT_DIRECTORY CLAP
                )
    endif()
endfunction(target_library_make_clap)

function(target_vst3_copy_after_build)
    set(oneValueArgs
            TARGET
            )
    cmake_parse_arguments(TCLP "" "${oneValueArgs}" "" ${ARGN} )

    if(APPLE)
        message(STATUS "Will copy VST3 plugin after every build" )
        set(products_folder ${CMAKE_BINARY_DIR})
        add_custom_command(TARGET ${TCLP_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo "Installing ${products_folder}/$<TARGET_PROPERTY:${TCLP_TARGET},LIBRARY_OUTPUT_NAME>.vst3 to ~/Library/Audio/Plug-Ins/VST3/"
                COMMAND ${CMAKE_COMMAND} -E make_directory "~/Library/Audio/Plug-Ins/VST3"
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${products_folder}/$<TARGET_PROPERTY:${TCLP_TARGET},LIBRARY_OUTPUT_NAME>.vst3" "~/Library/Audio/Plug-Ins/VST3/$<TARGET_PROPERTY:${TCLP_TARGET},LIBRARY_OUTPUT_NAME>.vst3"
            )
    elseif(UNIX)
        message(STATUS "Implement Copy after Build for VST3")
    endif()
endfunction(target_vst3_copy_after_build)

function(target_auv2_copy_after_build)
    set(oneValueArgs
            TARGET
            )
    cmake_parse_arguments(TCLP "" "${oneValueArgs}" "" ${ARGN} )

    if(APPLE)
        message(STATUS "Will copy auv2 plugin after every build" )
        set(products_folder ${CMAKE_BINARY_DIR})
        add_custom_command(TARGET ${TCLP_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "~/Library/Audio/Plug-Ins/Components"
                COMMAND ${CMAKE_COMMAND} -E echo installing "${products_folder}/$<TARGET_PROPERTY:${TCLP_TARGET},LIBRARY_OUTPUT_NAME>.component" "~/Library/Audio/Plug-Ins/Components/$<TARGET_PROPERTY:${TCLP_TARGET},LIBRARY_OUTPUT_NAME>.component"
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${products_folder}/$<TARGET_PROPERTY:${TCLP_TARGET},LIBRARY_OUTPUT_NAME>.component" "~/Library/Audio/Plug-Ins/Components/$<TARGET_PROPERTY:${TCLP_TARGET},LIBRARY_OUTPUT_NAME>.component"
                )
    elseif(UNIX)
        message(STATUS "Implement Copy after Build for VST3")
    endif()
endfunction(target_auv2_copy_after_build)