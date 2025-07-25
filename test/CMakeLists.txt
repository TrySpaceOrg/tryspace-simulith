# Unity source and include path
set(UNITY_SRC ../unity/unity.c)
set(UNITY_INC ../unity)

# Include paths for tests
include_directories(
    ${UNITY_INC}
    ../include
    ${ZeroMQ_INCLUDE_DIRS}
)

# Main simulith tests executable
add_executable(test_simulith test_simulith.c ${UNITY_SRC})
target_link_libraries(test_simulith simulith ${ZeroMQ_LIBRARIES} pthread)
add_test(NAME SimulithTest COMMAND test_simulith)

# CAN tests executable
add_executable(test_can test_can.c ${UNITY_SRC})
target_link_libraries(test_can simulith ${ZeroMQ_LIBRARIES})
add_test(NAME CANTest COMMAND test_can)

# GPIO tests executable
add_executable(test_gpio test_gpio.c ${UNITY_SRC})
target_link_libraries(test_gpio simulith ${ZeroMQ_LIBRARIES})
add_test(NAME GPIOTest COMMAND test_gpio)

# I2C tests executable
add_executable(test_i2c test_i2c.c ${UNITY_SRC})
target_link_libraries(test_i2c simulith ${ZeroMQ_LIBRARIES})
add_test(NAME I2CTest COMMAND test_i2c)

# PWM tests executable
add_executable(test_pwm test_pwm.c ${UNITY_SRC})
target_link_libraries(test_pwm simulith ${ZeroMQ_LIBRARIES})
add_test(NAME PWMTest COMMAND test_pwm)

# SPI tests executable
add_executable(test_spi test_spi.c ${UNITY_SRC})
target_link_libraries(test_spi simulith ${ZeroMQ_LIBRARIES})
add_test(NAME SPITest COMMAND test_spi)

# UART tests executable
add_executable(test_uart test_uart.c ${UNITY_SRC})
target_link_libraries(test_uart simulith ${ZeroMQ_LIBRARIES})
add_test(NAME UARTTest COMMAND test_uart)
