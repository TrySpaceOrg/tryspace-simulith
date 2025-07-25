#include "simulith.h"

int main(int argc, char *argv[]) 
{
    printf("Starting Simulith Server...\n");
    simulith_server_init(SERVER_PUB_ADDR, SERVER_REP_ADDR, 1, INTERVAL_NS);
    simulith_server_run();
    simulith_server_shutdown();
    return 0;
}
