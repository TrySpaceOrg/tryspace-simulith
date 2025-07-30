cmake_minimum_required(VERSION 3.25)
project(Simulith C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find ZeroMQ via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZeroMQ REQUIRED libzmq)

# Compiler flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
endif()

# Create Unity library with specific compiler flags
add_library(unity STATIC unity/unity.c)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(unity PRIVATE -Wno-float-equal)
endif()

include_directories(
    include
    unity
    ${ZeroMQ_INCLUDE_DIRS}
)

# Simulith library sources
set(SIMULITH_SOURCES
    src/simulith_common.c
    src/simulith_client.c
    src/simulith_server.c
    src/simulith_time.c
    src/simulith_can.c
    src/simulith_gpio.c
    src/simulith_i2c.c
    src/simulith_pwm.c
    src/simulith_spi.c
    src/simulith_uart.c
)

# Build Simulith static library
add_library(simulith STATIC ${SIMULITH_SOURCES})
target_link_libraries(simulith ${ZeroMQ_LIBRARIES})

# Build Simulith director (dynamically loads components)
add_executable(simulith_director_standalone 
                src/simulith_director.c)
target_link_libraries(simulith_director_standalone simulith ${ZeroMQ_LIBRARIES} dl)

# Build Simulith server standalone
add_executable(simulith_server_standalone 
                src/simulith_common.c
                src/simulith_server.c
                src/simulith_server_standalone.c)
target_link_libraries(simulith_server_standalone ${ZeroMQ_LIBRARIES})

# Optionally add tests subdirectory
option(BUILD_SIMULITH_TESTS "Build Simulith tests" OFF)
if(BUILD_SIMULITH_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
