/* * Simulith Director
 * This is the main entry point for the Simulith simulation framework
 *  Configuration Management
 *    Read configuration files (likely JSON/YAML for structured data)
 *    Define which component simulators to load
 *    Configure simulation parameters (time step, duration, etc.)
 *    Handle 42 integration settings
 *  Plugin/Component Loading
 *    Dynamic loading of component simulators (shared libraries or static linking)
 *    Initialize/cleanup component interfaces
 *    Manage component lifecycle within single process
 *  Simulith Integration
 *    Connect to Simulith server for time synchronization
 *    Handle time step coordination
 *    Manage simulation state (start/stop/pause)
 *  Data Management
 *    Shared memory or direct memory access between components
 *    Data flow coordination between simulators and 42
 *    State management and logging
*/

#include "simulith.h"
#include "simulith_director.h"
#include <errno.h>
#include <unistd.h>  // For access() function
#include <string.h>  // For strcmp() function

// Global director config for callback access
director_config_t g_director_config;

int parse_args(int argc, char *argv[], director_config_t *config) 
{
    // Set defaults
    strcpy(config->config_file, "spacecraft.conf");
    strcpy(config->components_dir, "./components");  // Default components directory
    strcpy(config->fortytwo_config, "./InOut");      // Default 42 configuration directory (Docker path)
    config->time_step_ms = 100;  // 100ms default
    config->duration_s = 0;      // Run indefinitely 
    config->verbose = 0;
    config->enable_42 = 1;       // Enable 42 by default now that we have the correct path
    config->fortytwo_initialized = 0;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--enable-42") == 0) {
            config->enable_42 = 1;
            printf("42 dynamics simulation enabled via command line\n");
        } else if (strcmp(argv[i], "--42-config") == 0 && i + 1 < argc) {
            strcpy(config->fortytwo_config, argv[++i]);
            printf("42 config directory set to: %s\n", config->fortytwo_config);
        } else if (strcmp(argv[i], "--verbose") == 0) {
            config->verbose = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Simulith Director Options:\n");
            printf("  --enable-42        Enable 42 dynamics simulation\n");
            printf("  --42-config DIR    Set 42 configuration directory (default: ./InOut)\n");
            printf("  --verbose          Enable verbose output\n");
            printf("  --help             Show this help message\n");
            return -1;  // Exit after showing help
        }
    }
    
    return 0;
}

int load_configuration(const char *config_file) 
{
    printf("Loading configuration from: %s\n", config_file);
    // TODO: Load and parse configuration file
    return 0;
}

int initialize_simulith_connection() 
{
    printf("Initializing Simulith connection...\n");
    // TODO: Connect to Simulith server
    return 0;
}

int load_components(director_config_t* config) 
{
    printf("Loading simulation components from: %s\n", config->components_dir);
    
    config->component_count = 0;
    config->lib_count = 0;
    
    DIR* dir = opendir(config->components_dir);
    if (!dir) {
        printf("Warning: Could not open components directory: %s (errno: %d)\n", 
               config->components_dir, errno);
        return 0;  // Not fatal - can run without components
    }
    
    printf("Successfully opened components directory\n");
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL && config->component_count < MAX_COMPONENTS) {
        printf("Found directory entry: %s\n", entry->d_name);
        
        // Look for .so files
        if (strstr(entry->d_name, ".so") == NULL) {
            printf("  Skipping non-.so file: %s\n", entry->d_name);
            continue;
        }
        
        printf("  Found .so file: %s\n", entry->d_name);
        
        // Build full path
        char lib_path[512];
        snprintf(lib_path, sizeof(lib_path), "%s/%s", config->components_dir, entry->d_name);
        
        printf("Loading component library: %s\n", lib_path);
        
        // Load the shared library
        void* lib_handle = dlopen(lib_path, RTLD_LAZY);
        if (!lib_handle) {
            printf("Warning: Failed to load %s: %s\n", lib_path, dlerror());
            continue;
        }
        
        // Store library handle for cleanup
        if (config->lib_count < MAX_COMPONENT_LIBS) {
            config->lib_handles[config->lib_count++] = lib_handle;
        }
        
        // Get the component interface function
        typedef const component_interface_t* (*get_component_interface_fn)(void);
        
        // Use union to safely convert between object and function pointers
        union {
            void* obj;
            get_component_interface_fn func;
        } symbol_cast;
        
        symbol_cast.obj = dlsym(lib_handle, "get_component_interface");
        get_component_interface_fn get_interface = symbol_cast.func;
        
        if (!get_interface) {
            printf("Warning: Library %s does not export get_component_interface: %s\n", 
                   lib_path, dlerror());
            continue;
        }
        
        // Get the component interface
        const component_interface_t* interface = get_interface();
        if (!interface) {
            printf("Warning: Library %s returned NULL interface\n", lib_path);
            continue;
        }
        
        // Register the component
        config->components[config->component_count].interface = interface;
        config->components[config->component_count].state = NULL;
        config->components[config->component_count].lib_handle = lib_handle;
        config->components[config->component_count].active = 1;
        
        printf("Registered component: %s - %s\n", 
               interface->name, interface->description);
        config->component_count++;
    }
    
    closedir(dir);
    
    printf("Loaded %d components from shared libraries\n", config->component_count);
    return 0;
}

int initialize_components(director_config_t* config)
{
    printf("Initializing components...\n");
    
    for (int i = 0; i < config->component_count; i++) {
        component_entry_t* entry = &config->components[i];
        if (entry->active && entry->interface && entry->interface->init) {
            printf("Initializing component: %s\n", entry->interface->name);
            
            int result = entry->interface->init(&entry->state);
            if (result != COMPONENT_SUCCESS) {
                printf("Failed to initialize component: %s\n", entry->interface->name);
                entry->active = 0;
                return -1;
            }
        }
    }
    
    printf("All components initialized successfully\n");
    return 0;
}

int initialize_42(director_config_t* config)
{
    if (!config->enable_42) {
        printf("42 simulation disabled\n");
        return 0;
    }
    
    printf("Initializing 42 dynamics simulation...\n");
    
    // Check if 42 configuration directory exists
    if (access(config->fortytwo_config, F_OK) != 0) {
        printf("Warning: 42 config directory not found: %s\n", config->fortytwo_config);
        printf("42 simulation will be disabled for this run\n");
        config->enable_42 = 0;
        return 0;
    }
    
    // Check if required configuration file exists
    char config_file_path[512];
    snprintf(config_file_path, sizeof(config_file_path), "%s/Inp_Sim.txt", config->fortytwo_config);
    if (access(config_file_path, R_OK) != 0) {
        printf("Warning: Required 42 config file not found: %s\n", config_file_path);
        printf("42 simulation will be disabled for this run\n");
        config->enable_42 = 0;
        return 0;
    }
    
    // Create minimal argc/argv for 42 initialization
    char* fortytwo_argv[3];
    fortytwo_argv[0] = (char*)"simulith_director";
    fortytwo_argv[1] = (char*)config->fortytwo_config;  // Pass the config directory path
    fortytwo_argv[2] = NULL;
    
    printf("Calling 42 InitSim with config path: %s\n", config->fortytwo_config);
    InitSim(2, fortytwo_argv);  // Pass 2 arguments: program name and config path
    
    config->fortytwo_initialized = 1;
    printf("42 simulation initialized successfully\n");
    return 0;
}

void cleanup_components(director_config_t* config)
{
    printf("Cleaning up components...\n");
    
    for (int i = 0; i < config->component_count; i++) {
        component_entry_t* entry = &config->components[i];
        if (entry->active && entry->interface && entry->interface->cleanup) {
            printf("Cleaning up component: %s\n", entry->interface->name);
            entry->interface->cleanup(entry->state);
            entry->state = NULL;
        }
        entry->active = 0;
    }
    
    // Close shared library handles
    for (int i = 0; i < config->lib_count; i++) {
        if (config->lib_handles[i]) {
            dlclose(config->lib_handles[i]);
            config->lib_handles[i] = NULL;
        }
    }
    config->lib_count = 0;
}

void on_tick(uint64_t tick_time_ns)
{
    // Execute 42 dynamics simulation step
    if (g_director_config.enable_42 && g_director_config.fortytwo_initialized) {
        int result = SimStep();
        if (result < 0) 
        {
            printf("42 simulation step failed\n");
        }
    }
    
    // Run each of the configured simulators
    for (int i = 0; i < g_director_config.component_count; i++) {
        component_entry_t* entry = &g_director_config.components[i];
        if (entry->active && entry->interface && entry->interface->tick && entry->state) {
            entry->interface->tick(entry->state, tick_time_ns);
        }
    }
}

int main(int argc, char *argv[]) 
{
    printf("Simulith Director starting...\n");
    
    int parse_result = parse_args(argc, argv, &g_director_config);
    if (parse_result < 0) {
        return 0;  // Help was shown or parsing failed
    } else if (parse_result > 0) {
        fprintf(stderr, "Failed to parse arguments\n");
        return 1;
    }
    
    if (load_configuration(g_director_config.config_file) != 0) {
        fprintf(stderr, "Failed to load configuration\n");
        return 1;
    }

    if (load_components(&g_director_config) != 0) 
    {
        fprintf(stderr, "Failed to load components\n");
        return 1;
    }

    if (initialize_components(&g_director_config) != 0)
    {
        fprintf(stderr, "Failed to initialize components\n");
        cleanup_components(&g_director_config);
        return 1;
    }

    if (initialize_42(&g_director_config) != 0)
    {
        fprintf(stderr, "Warning: 42 simulation initialization had issues, continuing without it\n");
        g_director_config.enable_42 = 0;
        g_director_config.fortytwo_initialized = 0;
    }

    // Wait a second for the Simulith server to start up
    sleep(1);

    if (simulith_client_init(LOCAL_PUB_ADDR, LOCAL_REP_ADDR, "tryspace-director", INTERVAL_NS) != 0) 
    {
        printf("Failed to initialize Simulith client\n");
        cleanup_components(&g_director_config);
        return 1;
    }

    // Handshake with Simulith server
    if (simulith_client_handshake() != 0) 
    {
        printf("Failed to handshake with Simulith server\n");
        simulith_client_shutdown();
        cleanup_components(&g_director_config);
        return 1;
    }

    simulith_client_run_loop(on_tick);
    
    printf("Simulith director shutting down...\n");
    
    // Cleanup
    simulith_client_shutdown();
    cleanup_components(&g_director_config);
    
    return 0;
}