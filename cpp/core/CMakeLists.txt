# CMakeLists.txt для cpp/core
add_library(trafficmask_core
    main.cpp
    engine.cpp
)

target_include_directories(trafficmask_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(trafficmask_core
    Threads::Threads
)
