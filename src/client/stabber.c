#include "server/server.h"

int
stbbr_start(int port)
{
    return server_run(port);
}
