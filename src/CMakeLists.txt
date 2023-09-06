project(conduit-src)

add_library(conduit-shared STATIC conduit-shared/shared-symbols.cpp)
target_include_directories(conduit-shared PUBLIC .)
target_link_libraries(conduit-shared PUBLIC clap clap-helpers sst-basic-blocks fmt)

add_subdirectory(polysynth)

add_library(conduit-impl INTERFACE)
target_link_libraries(conduit-impl INTERFACE polysynth)