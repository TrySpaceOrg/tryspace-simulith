# Unity source and include path
set(UNITY_SRC ./unity/unity.c)
set(UNITY_INC ./unity)

# Include paths for tests
include_directories(
    ${UNITY_INC}
    ../include
    ${ZeroMQ_INCLUDE_DIRS}
)

# Main simulith tests executable
add_executable(test_simulith test_simulith.c ${UNITY_SRC})
target_link_libraries(test_simulith simulith ${ZeroMQ_LIBRARIES} pthread)
add_test(NAME SimulithTests COMMAND test_simulith)

# Transport tests executable
add_executable(test_transport test_transport.c ${UNITY_SRC})
target_link_libraries(test_transport simulith ${ZeroMQ_LIBRARIES})
add_test(NAME TransportTests COMMAND test_transport)
