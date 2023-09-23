project(clap-event-monitor)

add_to_conduit(SOURCE
        ${PROJECT_NAME}.cpp
        ${PROJECT_NAME}-editor.cpp
        INCLUDE .
        )
