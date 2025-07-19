#include "simulith.h"

int main(int argc, char *argv[]) 
{
    printf("Starting Simulith Server...\n");
    simulith_server_init(PUB_ADDR, REP_ADDR, 1, INTERVAL_NS);
    simulith_server_run();
    simulith_server_shutdown();
    return 0;
}
