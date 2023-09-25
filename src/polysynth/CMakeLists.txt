project(polysynth)

add_to_conduit(SOURCE
        ${PROJECT_NAME}.cpp
        ${PROJECT_NAME}-editor.cpp
        voice.cpp
        INCLUDE .)
