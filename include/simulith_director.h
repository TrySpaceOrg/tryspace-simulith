#ifndef SIMULITH_DIRECTOR_H
#define SIMULITH_DIRECTOR_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>

#include "simulith_component.h"

#ifdef __cplusplus
extern "C" {
#endif

// Configuration constants
#define MAX_COMPONENTS 32
#define MAX_COMPONENT_LIBS 32

// Component registry entry
typedef struct {
    const component_interface_t* interface;
    component_state_t* state;
    void* lib_handle;  // Handle to the loaded shared library
    int active;
} component_entry_t;

// Director configuration structure
typedef struct 
{
    char config_file[256];
    char components_dir[256];  // Directory containing component shared libraries
    int time_step_ms;
    int duration_s;
    int verbose;
    
    // Component management
    component_entry_t components[MAX_COMPONENTS];
    int component_count;
    
    // Library handles for cleanup
    void* lib_handles[MAX_COMPONENT_LIBS];
    int lib_count;
} director_config_t;

// Function declarations

/**
 * Parse command line arguments
 * @param argc Argument count
 * @param argv Argument values
 * @param config Configuration structure to populate
 * @return 0 on success, -1 on error
 */
int parse_args(int argc, char *argv[], director_config_t *config);

/**
 * Load configuration from file
 * @param config_file Path to configuration file
 * @return 0 on success, -1 on error
 */
int load_configuration(const char *config_file);

/**
 * Initialize Simulith connection
 * @return 0 on success, -1 on error
 */
int initialize_simulith_connection(void);

/**
 * Load components from shared libraries
 * @param config Director configuration
 * @return 0 on success, -1 on error
 */
int load_components(director_config_t* config);

/**
 * Initialize all loaded components
 * @param config Director configuration
 * @return 0 on success, -1 on error
 */
int initialize_components(director_config_t* config);

/**
 * Cleanup all components and close libraries
 * @param config Director configuration
 */
void cleanup_components(director_config_t* config);

/**
 * Tick callback function for Simulith time stepping
 * @param tick_time_ns Current simulation time in nanoseconds
 */
void on_tick(uint64_t tick_time_ns);

// Global director configuration (for callback access)
extern director_config_t g_director_config;

#ifdef __cplusplus
}
#endif

#endif // SIMULITH_DIRECTOR_H
