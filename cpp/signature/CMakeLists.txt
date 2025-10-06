# CMakeLists.txt для cpp/signature
add_library(trafficmask_signature
    signature_engine.cpp
)

target_include_directories(trafficmask_signature PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(trafficmask_signature
    Threads::Threads
)
