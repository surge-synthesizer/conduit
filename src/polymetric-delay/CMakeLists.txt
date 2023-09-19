project(polymetric-delay)

add_library(${PROJECT_NAME} STATIC
        ${PROJECT_NAME}.cpp
        ${PROJECT_NAME}-editor.cpp
        )

target_include_directories(${PROJECT_NAME} PUBLIC .)

target_link_libraries(${PROJECT_NAME} PUBLIC
        conduit-shared)
