#include <math.h>
// Helper to normalize a 3-element vector (C style)
static void normalize_vec3(double v[3]) {
    double mag = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (mag > 1e-12) {
        v[0] /= mag;
        v[1] /= mag;
        v[2] /= mag;
    }
}
#include <math.h>
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
#include "simulith_42_context.h"
#include "simulith_42_commands.h"
#include <errno.h>
#include <unistd.h>  // For access() function
#include <string.h>  // For strcmp() function

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

// UDP publisher config
#define UDP_PUBLISH_INTERVAL_TICKS 10 // Default: publish every 10 ticks (assuming 100ms tick = 1s)
static int g_udp_sock = -1;
static struct sockaddr_in g_udp_addr;
static int g_udp_publish_counter = 0;

// Include 42 headers for accessing spacecraft data
#ifdef __cplusplus
extern "C" {
#endif
#include "42.h"
#ifdef __cplusplus
}
#endif

// Global director config for callback access
director_config_t g_director_config;

// Global command queue for 42 commands
static simulith_42_cmd_queue_t g_command_queue = {0};

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

void populate_42_context(simulith_42_context_t* context)
{
    // Initialize context
    memset(context, 0, sizeof(simulith_42_context_t));
    
    // Check if 42 is enabled and initialized
    if (!g_director_config.enable_42 || !g_director_config.fortytwo_initialized) {
        context->valid = 0;
        return;
    }
    
    // Check if we have at least one spacecraft
    extern long Nsc;           // Number of spacecraft from 42
    extern struct SCType *SC;  // Spacecraft array from 42
    extern double SimTime, DynTime; // Time variables from 42
    
    if (Nsc == 0 || !SC || !SC[0].Exists) {
        context->valid = 0;
        return;
    }
    
    // Use first spacecraft (SC[0]) for context
    struct SCType *spacecraft = &SC[0];
    
    // Time information
    context->sim_time = SimTime;
    context->dyn_time = DynTime;
    
    // Copy attitude quaternion (42 uses [q1,q2,q3,q0] format, we use [qx,qy,qz,qw])
    for (int i = 0; i < 4; i++) {
        context->qn[i] = spacecraft->qn[i];
    }
    
    // Copy angular rates
    for (int i = 0; i < 3; i++) {
        context->wn[i] = spacecraft->wn[i];
    }
    
    // Copy position and velocity
    for (int i = 0; i < 3; i++) {
        context->pos_n[i] = spacecraft->PosN[i];
        context->vel_n[i] = spacecraft->VelN[i];
        context->pos_r[i] = spacecraft->PosR[i];
        context->vel_r[i] = spacecraft->VelR[i];
    }
    
    // Copy and normalize environmental vectors
    for (int i = 0; i < 3; i++) {
        context->sun_vector_body[i] = spacecraft->svb[i];
        context->mag_field_body[i] = spacecraft->bvb[i];
        context->sun_vector_inertial[i] = spacecraft->svn[i];
        context->mag_field_inertial[i] = spacecraft->bvn[i];
    }
    // Normalize all vectors that should be unit vectors
    normalize_vec3(context->sun_vector_body);
    normalize_vec3(context->sun_vector_inertial);
    normalize_vec3(context->mag_field_body);
    normalize_vec3(context->mag_field_inertial);
    
    // Copy spacecraft properties
    context->mass = spacecraft->mass;
    for (int i = 0; i < 3; i++) {
        context->cm[i] = spacecraft->cm[i];
        for (int j = 0; j < 3; j++) {
            context->inertia[i][j] = spacecraft->I[i][j];
        }
    }
    
    // Environmental conditions
    context->eclipse = spacecraft->Eclipse;
    context->atmo_density = spacecraft->AtmoDensity;
    
    // Spacecraft identification
    context->spacecraft_id = spacecraft->ID;
    context->exists = spacecraft->Exists;
    strncpy(context->label, spacecraft->Label, sizeof(context->label) - 1);
    context->label[sizeof(context->label) - 1] = '\0';
    
    // Mark context as valid
    context->valid = 1;
}

// Command Queue Functions
static int enqueue_command(const simulith_42_command_t* cmd)
{
    if (g_command_queue.count >= SIMULITH_42_CMD_QUEUE_SIZE) {
        printf("Warning: 42 command queue full, dropping command\n");
        return -1;
    }
    
    g_command_queue.commands[g_command_queue.head] = *cmd;
    g_command_queue.head = (g_command_queue.head + 1) % SIMULITH_42_CMD_QUEUE_SIZE;
    g_command_queue.count++;
    return 0;
}

static int dequeue_command(simulith_42_command_t* cmd)
{
    if (g_command_queue.count == 0) {
        return -1; // Queue empty
    }
    
    *cmd = g_command_queue.commands[g_command_queue.tail];
    g_command_queue.tail = (g_command_queue.tail + 1) % SIMULITH_42_CMD_QUEUE_SIZE;
    g_command_queue.count--;
    return 0;
}

// Command Interface Functions (called by simulators)
int simulith_42_send_mtb_command(int spacecraft_id, const double dipole[3], int enable_mask)
{
    simulith_42_command_t cmd = {0};
    cmd.type = SIMULITH_42_CMD_MTB_TORQUE;
    cmd.spacecraft_id = spacecraft_id;
    cmd.valid = 1;
    
    for (int i = 0; i < 3; i++) {
        cmd.cmd.mtb.dipole[i] = dipole[i];
    }
    cmd.cmd.mtb.enable_mask = enable_mask;
    
    return enqueue_command(&cmd);
}

int simulith_42_send_wheel_command(int spacecraft_id, const double torque[4], int enable_mask)
{
    simulith_42_command_t cmd = {0};
    cmd.type = SIMULITH_42_CMD_WHEEL_TORQUE;
    cmd.spacecraft_id = spacecraft_id;
    cmd.valid = 1;
    
    for (int i = 0; i < 4; i++) {
        cmd.cmd.wheel.torque[i] = torque[i];
    }
    cmd.cmd.wheel.enable_mask = enable_mask;
    
    return enqueue_command(&cmd);
}

int simulith_42_send_thruster_command(int spacecraft_id, const double thrust[3], const double torque[3], int enable_mask)
{
    simulith_42_command_t cmd = {0};
    cmd.type = SIMULITH_42_CMD_THRUSTER;
    cmd.spacecraft_id = spacecraft_id;
    cmd.valid = 1;
    
    for (int i = 0; i < 3; i++) {
        cmd.cmd.thruster.thrust[i] = thrust[i];
        cmd.cmd.thruster.torque[i] = torque[i];
    }
    cmd.cmd.thruster.enable_mask = enable_mask;
    
    return enqueue_command(&cmd);
}

// Process commands and apply them to 42
static void process_42_commands(void)
{
    simulith_42_command_t cmd;
    extern long Nsc;
    extern struct SCType *SC;
    
    if (!g_director_config.enable_42 || !g_director_config.fortytwo_initialized || !SC) {
        return;
    }
    
    // Process all queued commands
    while (dequeue_command(&cmd) == 0) {
        if (!cmd.valid || cmd.spacecraft_id >= Nsc || !SC[cmd.spacecraft_id].Exists) {
            continue;
        }
        
        struct SCType *spacecraft = &SC[cmd.spacecraft_id];
        
        switch (cmd.type) {
            case SIMULITH_42_CMD_MTB_TORQUE:
                // Apply MTB command to 42
                for (int i = 0; i < spacecraft->Nmtb && i < 3; i++) {
                    if (cmd.cmd.mtb.enable_mask & (1 << i)) {
                        // Set MTB dipole moment (42 stores this in MTB[i].M)
                        // Note: You may need to adjust this based on 42's MTB structure
                        if (spacecraft->MTB) {
                            spacecraft->MTB[i].M = cmd.cmd.mtb.dipole[i];
                        }
                    }
                }
                if (g_director_config.verbose) {
                    printf("Applied MTB command: dipole=[%.6f, %.6f, %.6f], mask=0x%X\n",
                           cmd.cmd.mtb.dipole[0], cmd.cmd.mtb.dipole[1], cmd.cmd.mtb.dipole[2],
                           cmd.cmd.mtb.enable_mask);
                }
                break;
                
            case SIMULITH_42_CMD_WHEEL_TORQUE:
                // Apply wheel command to 42
                for (int i = 0; i < spacecraft->Nw && i < 4; i++) {
                    if (cmd.cmd.wheel.enable_mask & (1 << i)) {
                        // Set wheel torque (42 stores this in Whl[i].Tcmd)
                        if (spacecraft->Whl) {
                            spacecraft->Whl[i].Tcmd = cmd.cmd.wheel.torque[i];
                        }
                    }
                }
                if (g_director_config.verbose) {
                    printf("Applied wheel command: torque=[%.6f, %.6f, %.6f, %.6f], mask=0x%X\n",
                           cmd.cmd.wheel.torque[0], cmd.cmd.wheel.torque[1], 
                           cmd.cmd.wheel.torque[2], cmd.cmd.wheel.torque[3],
                           cmd.cmd.wheel.enable_mask);
                }
                break;
                
            case SIMULITH_42_CMD_THRUSTER:
                // Apply thruster command to 42
                for (int i = 0; i < spacecraft->Nthr && i < 3; i++) {
                    if (cmd.cmd.thruster.enable_mask & (1 << i)) {
                        // Set thruster command (42 uses ThrustLevelCmd for proportional mode)
                        if (spacecraft->Thr) {
                            // Normalize thrust to 0-1 range for ThrustLevelCmd
                            double thrust_level = cmd.cmd.thruster.thrust[i] / spacecraft->Thr[i].Fmax;
                            if (thrust_level > 1.0) thrust_level = 1.0;
                            if (thrust_level < 0.0) thrust_level = 0.0;
                            spacecraft->Thr[i].ThrustLevelCmd = thrust_level;
                        }
                    }
                }
                if (g_director_config.verbose) {
                    printf("Applied thruster command: thrust=[%.6f, %.6f, %.6f], torque=[%.6f, %.6f, %.6f], mask=0x%X\n",
                           cmd.cmd.thruster.thrust[0], cmd.cmd.thruster.thrust[1], cmd.cmd.thruster.thrust[2],
                           cmd.cmd.thruster.torque[0], cmd.cmd.thruster.torque[1], cmd.cmd.thruster.torque[2],
                           cmd.cmd.thruster.enable_mask);
                }
                break;
                
            default:
                printf("Warning: Unknown 42 command type: %d\n", cmd.type);
                break;
        }
    }
}

void on_tick(uint64_t tick_time_ns)
{
    // Populate 42 context for this tick
    simulith_42_context_t context_42;
    populate_42_context(&context_42);
    
    // Run each of the configured simulators with 42 context
    // (Do this BEFORE 42 step so simulators can queue commands)
    for (int i = 0; i < g_director_config.component_count; i++) {
        component_entry_t* entry = &g_director_config.components[i];
        if (entry->active && entry->interface && entry->interface->tick && entry->state) {
            entry->interface->tick(entry->state, tick_time_ns, &context_42);
        }
    }
    
    // Process any commands from simulators before 42 step
    process_42_commands();
    
    // Execute 42 dynamics simulation step
    if (g_director_config.enable_42 && g_director_config.fortytwo_initialized) {
        int result = SimStep();
        if (result < 0) 
        {
            printf("42 simulation step failed\n");
        }
    }

    // --- UDP Telemetry Publisher ---
    g_udp_publish_counter = (g_udp_publish_counter + 1) % UDP_PUBLISH_INTERVAL_TICKS;
    if (g_udp_sock >= 0 && context_42.valid && g_udp_publish_counter == 0)
    {
        // Packet structure matches XTCE SIM_42_TRUTH_DATA:
        // DYN_TIME, POSITION_N_1/2/3, SVB_1/2/3, BVB_1/2/3, HVB_1/2/3, WN_1/2/3, QN_1/2/3/4, MASS, CM_1/2/3, INERTIA_11/12/13/21/22/23/31/32/33, ECLIPSE, ATMO_DENSITY
        unsigned char packet[276]; // Exact size: 34 doubles (8 bytes each) + 1 int (4 bytes) = 276 bytes
        memset(packet, 0, sizeof(packet));
        int offset = 0;
        // 1. DYN_TIME
        double d_dyn_time = context_42.dyn_time;
        memcpy(packet+offset, &d_dyn_time, sizeof(double)); offset += sizeof(double);
        // 2-4. POSITION_N_1/2/3
        for (int i = 0; i < 3; i++) { double d = context_42.pos_n[i]; memcpy(packet+offset, &d, sizeof(double)); offset += sizeof(double); }
        // 5-7. SVB_1/2/3
        for (int i = 0; i < 3; i++) { double d = context_42.sun_vector_body[i]; memcpy(packet+offset, &d, sizeof(double)); offset += sizeof(double); }
        // 8-10. BVB_1/2/3
        for (int i = 0; i < 3; i++) { double d = context_42.mag_field_body[i]; memcpy(packet+offset, &d, sizeof(double)); offset += sizeof(double); }
        // 11-13. HVB_1/2/3
        for (int i = 0; i < 3; i++) { double d = context_42.hvb[i]; memcpy(packet+offset, &d, sizeof(double)); offset += sizeof(double); }
        // 14-16. WN_1/2/3
        for (int i = 0; i < 3; i++) { double d = context_42.wn[i]; memcpy(packet+offset, &d, sizeof(double)); offset += sizeof(double); }
        // 17-20. QN_1/2/3/4
        for (int i = 0; i < 4; i++) { double d = context_42.qn[i]; memcpy(packet+offset, &d, sizeof(double)); offset += sizeof(double); }
        // 21. MASS
        double d_mass = context_42.mass;
        memcpy(packet+offset, &d_mass, sizeof(double)); offset += sizeof(double);
        // 22-24. CM_1/2/3
        for (int i = 0; i < 3; i++) { double d = context_42.cm[i]; memcpy(packet+offset, &d, sizeof(double)); offset += sizeof(double); }
        // 25-33. INERTIA_11/12/13/21/22/23/31/32/33
        for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) { double d = context_42.inertia[i][j]; memcpy(packet+offset, &d, sizeof(double)); offset += sizeof(double); }
        // 34. ECLIPSE (int)
        int ecl = context_42.eclipse;
        memcpy(packet+offset, &ecl, sizeof(int)); offset += sizeof(int);
        // 35. ATMO_DENSITY
        double d_atmo_density = context_42.atmo_density;
        memcpy(packet+offset, &d_atmo_density, sizeof(double)); offset += sizeof(double);
        // Send exactly 276 bytes
        sendto(g_udp_sock, packet, 276, 0, (struct sockaddr*)&g_udp_addr, sizeof(g_udp_addr));
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

    // UDP Telemetry Socket Init
    g_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_udp_sock < 0) 
    {
        perror("UDP socket creation failed");
    } 
    else 
    {
        memset(&g_udp_addr, 0, sizeof(g_udp_addr));
        g_udp_addr.sin_family = AF_INET;
        g_udp_addr.sin_port = htons(50042); // Default port for 42 telemetry

        // Resolve tryspace-gsw hostname
        const char* gsw_hostname = "tryspace-gsw";
        struct hostent* gsw_host = gethostbyname(gsw_hostname);
        if (gsw_host && gsw_host->h_addrtype == AF_INET) {
            memcpy(&g_udp_addr.sin_addr, gsw_host->h_addr_list[0], gsw_host->h_length);
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &g_udp_addr.sin_addr, ip_str, sizeof(ip_str));
            printf("UDP telemetry publisher initialized for YAMCS at %s:50042\n", ip_str);
        } else {
            printf("Warning: Could not resolve hostname '%s', defaulting to 127.0.0.1\n", gsw_hostname);
            g_udp_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        }
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