# CMakeLists.txt для cpp/traffic
add_library(trafficmask_traffic
    traffic_processor.cpp
)

target_include_directories(trafficmask_traffic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(trafficmask_traffic
    Threads::Threads
)
