# Simulith CMake configuration
cmake_minimum_required(VERSION 3.25)
project(Simulith VERSION 0.0 LANGUAGES C)

# Enable compile_commands.json for IDEs and static analysis
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find ZeroMQ via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZeroMQ REQUIRED libzmq)

# Global compiler flags (applied to all targets)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Werror)
endif()

# Create Unity library with specific compiler flags and always build with -fPIC
add_library(unity STATIC unity/unity.c)
set_target_properties(unity PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(unity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/unity)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(unity PRIVATE -Wno-float-equal)
endif()

# Simulith library sources
set(SIMULITH_SOURCES
    src/simulith_common.c
    src/simulith_client.c
    src/simulith_server.c
    src/simulith_time.c
    src/simulith_transport.c
)

# Build Simulith static library with -fPIC, used by HWLIB
add_library(simulith STATIC ${SIMULITH_SOURCES})
set_target_properties(simulith PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(simulith PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/unity
    ${CMAKE_CURRENT_SOURCE_DIR}/42/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/42/Kit/Include
    ${ZeroMQ_INCLUDE_DIRS}
)
target_link_libraries(simulith PUBLIC ${ZeroMQ_LIBRARIES})

# Build 42 as a library
set(FORTYTWO_SOURCES
    42/Source/42exec.c
    42/Source/42init.c
    42/Source/42dynamics.c
    42/Source/42environs.c
    42/Source/42ephem.c
    42/Source/42perturb.c
    42/Source/42actuators.c
    42/Source/42sensors.c
    42/Source/42fsw.c       # Re-enable FSW with stubs
    42/Source/42cmd.c
    42/Source/42report.c    # Re-enable report with stubs
    42/Source/42joints.c
    42/Source/42jitter.c
    42/Source/42ipc.c       # Re-enable IPC with stubs
    # Kit sources
    42/Kit/Source/dcmkit.c
    42/Kit/Source/envkit.c
    42/Kit/Source/fswkit.c
    42/Kit/Source/iokit.c
    42/Kit/Source/mathkit.c
    42/Kit/Source/meshkit.c
    42/Kit/Source/msis86kit.c
    42/Kit/Source/nrlmsise00kit.c
    42/Kit/Source/orbkit.c
    42/Kit/Source/sigkit.c
    42/Kit/Source/sphkit.c
    42/Kit/Source/timekit.c
    # 42/Kit/Source/texkit.c  # Exclude texture kit due to missing M_PI
    # Stub implementations for optional features
    src/42_stubs.c
    # Add other core 42 source files as needed, excluding:
    # - 42main.c (has its own main)
    # - 42gl.c, 42glut.c, 42glfw.c (GUI components)
    # - AcApp.c (separate application)
    # - gmseckit.c, glkit.c (optional dependencies)
)

# Build 42 as a library
add_library(fortytwo STATIC ${FORTYTWO_SOURCES})
set_target_properties(fortytwo PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(fortytwo PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/42/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/42/Kit/Include
)
target_compile_definitions(fortytwo PRIVATE 
    __linux__                # Set platform define
    _GNU_SOURCE              # Enable GNU extensions
    _USE_MATH_DEFINES        # Enable M_PI and other math constants
)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    # Relax warnings for 42 since it's third-party code
    target_compile_options(fortytwo PRIVATE 
        -Wno-unused-variable 
        -Wno-unused-function 
        -Wno-unused-but-set-variable
        -Wno-format-zero-length
        -Wno-format-overflow
        -Wno-array-parameter
        -Wno-implicit-function-declaration
        -Wno-builtin-declaration-mismatch
        -Wno-misleading-indentation
        -Wno-redundant-decls     # Fix atanh redeclaration issue
        -Wno-strict-prototypes   # Fix prototype warnings
        -Wno-switch-default      # Relax switch default case warnings
        -Wno-float-equal         # Relax floating-point equality warnings
        -Wno-strict-overflow     # Relax strict overflow warnings
        -Wno-implicit-fallthrough # Relax implicit fallthrough warnings
        -Wno-sign-compare           # Relax signed/unsigned comparison warnings
        -Wno-error  # Turn off -Werror for 42
    )
endif()

# Build Simulith director (dynamically loads components)
add_executable(simulith_director_standalone src/simulith_director.c)
target_link_libraries(simulith_director_standalone PRIVATE simulith fortytwo ${ZeroMQ_LIBRARIES} dl m)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    # Handle warnings for director due to 42 integration
    target_compile_options(simulith_director_standalone PRIVATE 
        -Wno-redundant-decls     # Handle 42 header conflicts
        -Wno-strict-prototypes   # Fix prototype warnings from 42 headers
        -Wno-discarded-qualifiers # Fix const qualifier warnings
    )
endif()

# Build Simulith server standalone
add_executable(simulith_server_standalone 
    src/simulith_common.c
    src/simulith_server.c
    src/simulith_server_standalone.c)
target_link_libraries(simulith_server_standalone PRIVATE simulith ${ZeroMQ_LIBRARIES})


# Optionally add tests subdirectory
option(BUILD_SIMULITH_TESTS "Build Simulith tests" OFF)
if(BUILD_SIMULITH_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
