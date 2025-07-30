#ifndef SIMULITH_COMPONENT_H
#define SIMULITH_COMPONENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Component return codes
#define COMPONENT_SUCCESS 0
#define COMPONENT_ERROR -1

// Forward declaration of component state
typedef struct component_state component_state_t;

// Component interface structure
typedef struct {
    const char* name;           // Component name (must be unique)
    const char* description;    // Component description
    
    // Lifecycle functions
    int (*init)(component_state_t** state);
    void (*tick)(component_state_t* state, uint64_t tick_time_ns);
    void (*cleanup)(component_state_t* state);
    
    // Optional configuration function
    int (*configure)(component_state_t* state, const char* config);
} component_interface_t;

// Component registration function type
typedef const component_interface_t* (*get_component_interface_fn)(void);

// Macro to help implement component registration
#define REGISTER_COMPONENT(name) \
    const component_interface_t* get_##name##_component_interface(void)

#ifdef __cplusplus
}
#endif

#endif // SIMULITH_COMPONENT_H
