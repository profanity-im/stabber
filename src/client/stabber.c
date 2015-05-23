#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "server/server.h"
#include "server/prime.h"

typedef struct server_args_t {
    int port;
} StabberArgs;

int
stbbr_start(int port)
{
    return server_run(port);
}

int
stbbr_auth_passwd(char *password)
{
    prime_required_passwd(password);
    return 1;
}

int
stbbr_for(char *id, char *stream)
{
    prime_for(id, stream);
    return 1;
}

void
stbbr_stop(void)
{
    server_stop();
}
