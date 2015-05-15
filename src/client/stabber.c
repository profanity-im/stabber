#include "server/server.h"

int
stabber_start(int port)
{
    return server_run(port);
}
