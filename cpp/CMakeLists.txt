add_library(neural_tunnel STATIC 
    ../neural_tunnel.cpp
    ../probe_engine.cpp
    ../quantum_masking.cpp
)
add_executable(neural_tunnel_server ../neural_tunnel_server.cpp)
target_link_libraries(neural_tunnel_server neural_tunnel core)
add_executable(neural_tunnel_client ../neural_tunnel_client.cpp)
target_link_libraries(neural_tunnel_client neural_tunnel core)
