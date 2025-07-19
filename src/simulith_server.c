#include "simulith.h"

#define MAX_CLIENTS 32

typedef struct
{
    char id[64];
    int  responded;
} ClientState;

static void       *server_context             = NULL;
static void       *publisher                  = NULL;
static void       *responder                  = NULL;
static uint64_t    current_time_ns            = 0;
static uint64_t    tick_interval_ns           = 0;
static int         expected_clients           = 0;
static ClientState client_states[MAX_CLIENTS] = {0};

static int is_client_id_taken(const char *id)
{
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (client_states[i].id[0] != '\0' && strcmp(client_states[i].id, id) == 0)
        {
            simulith_log("Client ID '%s' is already in use\n", id);
            return 1;
        }
    }
    return 0;
}

int simulith_server_init(const char *pub_bind, const char *rep_bind, int client_count, uint64_t interval_ns)
{
    // Validate parameters
    if (client_count <= 0 || client_count > MAX_CLIENTS)
    {
        simulith_log("Invalid client count: %d (must be between 1 and %d)\n", client_count, MAX_CLIENTS);
        return -1;
    }

    if (interval_ns == 0)
    {
        simulith_log("Invalid interval: must be greater than 0\n");
        return -1;
    }

    expected_clients = client_count;
    tick_interval_ns = interval_ns;

    server_context = zmq_ctx_new();
    if (!server_context)
    {
        perror("zmq_ctx_new failed");
        return -1;
    }

    publisher = zmq_socket(server_context, ZMQ_PUB);
    if (!publisher || zmq_bind(publisher, pub_bind) != 0)
    {
        perror("Publisher socket setup failed");
        return -1;
    }

    responder = zmq_socket(server_context, ZMQ_REP);
    if (!responder || zmq_bind(responder, rep_bind) != 0)
    {
        perror("Responder socket setup failed");
        return -1;
    }

    // Initialize client states
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        client_states[i].id[0]     = '\0';
        client_states[i].responded = 0;
    }

    simulith_log("Simulith server initialized. Clients expected: %d\n", expected_clients);
    return 0;
}

static void broadcast_time(void)
{
    static uint64_t last_log_time = 0;
    static const uint64_t LOG_INTERVAL_NS = 10000000000; // Log every 10 seconds
    
    zmq_send(publisher, &current_time_ns, sizeof(current_time_ns), 0);
    
    // Only log time broadcasts every LOG_INTERVAL_NS
    if (current_time_ns - last_log_time >= LOG_INTERVAL_NS) {
        simulith_log("Simulation time: %.3f seconds\n", current_time_ns / 1e9);
        last_log_time = current_time_ns;
    }
}

static int all_clients_responded(void)
{
    int count = 0;
    for (int i = 0; i < expected_clients; ++i)
    {
        if (client_states[i].responded)
            count++;
    }
    return count == expected_clients;
}

static void reset_responses(void)
{
    for (int i = 0; i < expected_clients; ++i)
    {
        client_states[i].responded = 0;
    }
}

static void handle_ack(const char *client_id)
{
    for (int i = 0; i < expected_clients; ++i)
    {
        if (client_states[i].id[0] != '\0' && strcmp(client_states[i].id, client_id) == 0)
        {
            client_states[i].responded = 1;
            return;
        }
    }
    simulith_log("ACK received from unknown client: %s\n", client_id);
}

void simulith_server_run(void)
{
    simulith_log("Waiting for clients to be ready...\n");

    // Wait for all clients to send "READY"
    int ready_clients = 0;
    while (ready_clients < expected_clients)
    {
        char buffer[64] = {0};
        int  size       = zmq_recv(responder, buffer, sizeof(buffer) - 1, 0);
        if (size > 0)
        {
            buffer[size] = '\0';

            // Parse READY message
            char *space = strchr(buffer, ' ');
            if (!space || strncmp(buffer, "READY", 5) != 0)
            {
                simulith_log("Invalid handshake message: %s\n", buffer);
                zmq_send(responder, "ERR", 3, 0);
                continue;
            }

            // Extract client ID (skip "READY " prefix)
            char *client_id = space + 1;
            if (strlen(client_id) == 0)
            {
                simulith_log("Empty client ID in handshake\n");
                zmq_send(responder, "ERR", 3, 0);
                continue;
            }

            // Check for duplicate client ID
            if (is_client_id_taken(client_id))
            {
                simulith_log("Rejecting duplicate client ID: %s\n", client_id);
                zmq_send(responder, "DUP_ID", 6, 0);
                continue;
            }

            // Find empty slot and store client ID
            int slot = -1;
            for (int i = 0; i < MAX_CLIENTS; ++i)
            {
                if (client_states[i].id[0] == '\0')
                {
                    slot = i;
                    break;
                }
            }

            if (slot == -1)
            {
                simulith_log("No available slots for new client\n");
                zmq_send(responder, "ERR", 3, 0);
                continue;
            }

            // Register client
            strncpy(client_states[slot].id, client_id, sizeof(client_states[slot].id) - 1);
            client_states[slot].id[sizeof(client_states[slot].id) - 1] = '\0';
            client_states[slot].responded                              = 0;
            ready_clients++;

            zmq_send(responder, "ACK", 3, 0);
            simulith_log("Registered client %s (%d/%d)\n", client_id, ready_clients, expected_clients);
        }
    }

    simulith_log("All clients ready. Starting time broadcast.\n");

    // Reset client responded flags for tick ACKs
    reset_responses();

    while (1)
    {
        broadcast_time();
        reset_responses();

        while (!all_clients_responded())
        {
            char buffer[64] = {0};
            int  size       = zmq_recv(responder, buffer, sizeof(buffer) - 1, 0);
            if (size > 0)
            {
                buffer[size] = '\0';
                handle_ack(buffer);
                zmq_send(responder, "ACK", 3, 0);
            }
        }

        current_time_ns += tick_interval_ns;
    }
}

void simulith_server_shutdown(void)
{
    if (publisher)
        zmq_close(publisher);
    if (responder)
        zmq_close(responder);
    if (server_context)
        zmq_ctx_term(server_context);
    publisher      = NULL;
    responder      = NULL;
    server_context = NULL;
    simulith_log("Simulith server shut down\n");
}
